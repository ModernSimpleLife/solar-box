package main

import (
	"encoding/csv"
	"fmt"
	"os"
	"os/exec"
	"strings"
	"time"

	"github.com/sirupsen/logrus"
)

type Logger interface {
	// Log logs the controller state in an arbitrary form
	Log(state ControllerState)

	// Close closes the underlying resources
	Close() error
}

var csvHeaders = []string{
	"temperature_in_celsius",
	"pv_voltage",
	"pv_current_in_amps",
	"pv_power_in_watts",
	"battery_voltage",
	"battery_soc_in_percentage",
	"charging_current_in_amps",
}

type CSVLogger struct {
	file      *os.File
	csvWriter *csv.Writer
}

func NewCSVLogger(path string) (Logger, error) {
	var file *os.File
	var records [][]string
	var err error

	defer func() {
		if err != nil && file != nil {
			file.Close()
		}
	}()

	file, err = os.OpenFile(path, os.O_RDWR|os.O_CREATE, os.ModePerm)
	if err != nil {
		return nil, fmt.Errorf("failed to open %s: %v", path, err)
	}

	csvReader := csv.NewReader(file)
	records, err = csvReader.ReadAll()
	if err != nil {
		return nil, fmt.Errorf("failed to parse %s as csv: %v", path, err)
	}

	csvWriter := csv.NewWriter(file)
	// No header yet, create one
	if len(records) == 0 {
		err = csvWriter.Write(csvHeaders)
		if err != nil {
			return nil, fmt.Errorf("failed to write csv headers: %v", err)
		}

		csvWriter.Flush()
	}

	logger := CSVLogger{
		file:      file,
		csvWriter: csvWriter,
	}

	return &logger, nil
}

func (logger *CSVLogger) Close() error {
	return logger.file.Close()
}

func (logger *CSVLogger) Log(state ControllerState) {
	err := logger.csvWriter.Write([]string{
		fmt.Sprint(state.Temperature),
		fmt.Sprint(state.PVVoltage),
		fmt.Sprint(state.PVCurrent),
		fmt.Sprint(state.PVPower),
		fmt.Sprint(state.BatteryVoltage),
		fmt.Sprint(state.BatterySOC),
		fmt.Sprint(state.ChargingCurrent),
	})

	if err != nil {
		logrus.Warnf("failed to log as csv: %v", err)
		return
	}

	logger.csvWriter.Flush()
}

type TerminalLogger struct{}

func NewTerminalLogger() Logger {
	return &TerminalLogger{}
}

func (logger *TerminalLogger) Log(state ControllerState) {
	logrus.Infoln(&state)
}

func (logger *TerminalLogger) Close() error {
	return nil
}

type SMSLogger struct {
	dest       string
	lastSentAt time.Time
	interval   time.Duration
}

func NewSMSLogger(dest []string, interval time.Duration) Logger {
	return &SMSLogger{
		dest:     strings.Join(dest, ","),
		interval: interval,
	}
}

func (logger *SMSLogger) Log(state ControllerState) {
	now := time.Now()
	if now.Sub(logger.lastSentAt) <= logger.interval {
		return
	}

	logger.lastSentAt = now
	cmd := exec.Command("termux-sms-send", "-n", logger.dest, state.String())
	err := cmd.Run()
	if err != nil {
		logrus.Warnf("failed to log as sms to %s: %v", logger.dest, err)
		return
	}
}

func (logger *SMSLogger) Close() error {
	return nil
}
