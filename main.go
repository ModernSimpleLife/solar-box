package main

import (
	"os"
	"time"

	"github.com/sirupsen/logrus"
)

const (
	// UpdateInterval defines the delay between updates from the charge controller
	UpdateInterval    = time.Minute
	SMSLoggerInterval = time.Minute * 30
)

func main() {
	if len(os.Args) != 2 {
		logrus.Fatalf("usage: %s <csv_path>", os.Args[0])
	}

	csvPath := os.Args[1]
	csvLogger, err := NewCSVLogger(csvPath)
	if err != nil {
		logrus.Fatal(err)
	}

	loggers := []Logger{
		csvLogger,
		NewTerminalLogger(),
	}

	defer func() {
		for _, logger := range loggers {
			logger.Close()
		}
	}()

	controller, err := NewRenogyController()
	if err != nil {
		logrus.Fatal(err)
	}

	interval := time.Tick(UpdateInterval)
	requester := NewSMSRequester()

	for {
		select {
		case <-interval:
			err = controller.Update()
			if err != nil {
				logrus.Warnf("failed to update controller's state: %v", err)
				continue
			}

			state := controller.State()
			for _, logger := range loggers {
				logger.Log(state)
			}
		case respond := <-requester.C:
			respond(controller.State())
		}
	}
}
