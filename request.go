package main

import (
	"encoding/json"
	"os/exec"
	"strings"
	"time"

	"github.com/sirupsen/logrus"
)

const SMSRequestPattern = "solar:stats"

var SMSLookupDuration = time.Second * 5

type SMSTime time.Time

func (t *SMSTime) UnmarshalJSON(b []byte) error {
	s := strings.Trim(string(b), "\"")
	parsed, err := time.Parse("2006-01-02 15:04:05", s)
	if err != nil {
		return err
	}
	*t = SMSTime(parsed)
	return nil
}

func (t SMSTime) MarshalJSON() ([]byte, error) {
	return json.Marshal(time.Time(t))
}

type SMSMessage struct {
	Number     string  `json:"number"`
	ReceivedAt SMSTime `json:"received"`
	Body       string  `json:"body"`
}

type RespondFunc func(ControllerState)

type SMSRequester struct {
	C <-chan RespondFunc
}

func NewSMSRequester() *SMSRequester {
	c := make(chan RespondFunc, 1)
	req := SMSRequester{
		C: c,
	}

	// TODO: handle tear down for this loop
	go req.loop(c)
	return &req
}

// getRequesters retrieves current requesters for the past 5 seconds.
// If none, returns an empty list
func (req *SMSRequester) getRequesters() []string {
	cmd := exec.Command("termux-sms-list")
	buf, err := cmd.Output()
	if err != nil {
		logrus.Warnf("failed to run termux-sms-list result: %v", err)
		return nil
	}

	var msgs []SMSMessage
	err = json.Unmarshal(buf, &msgs)
	if err != nil {
		logrus.Warnf("failed to parse termux-sms-list result: %v", err)
		return nil
	}

	var numbers []string
	for _, msg := range msgs {
		if time.Since(time.Time(msg.ReceivedAt)) <= SMSLookupDuration && strings.Contains(msg.Body, SMSRequestPattern) {
			numbers = append(numbers, msg.Number)
		}
	}

	return numbers
}

func (req *SMSRequester) loop(ch chan<- RespondFunc) {
	for range time.Tick(SMSLookupDuration) {
		numbers := req.getRequesters()
		if len(numbers) == 0 {
			continue
		}

		select {
		case ch <- func(state ControllerState) {
			cmd := exec.Command("termux-sms-send", "-n", strings.Join(numbers, ","), state.String())
			err := cmd.Run()
			if err != nil {
				logrus.Warnf("failed to log as sms to %s: %v", numbers, err)
			}
		}:
		default:
			logrus.Warn("detected a slow reader for sms requester notification")
		}
	}
}
