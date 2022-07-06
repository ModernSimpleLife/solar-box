package main

import "os"

type Logger interface {
	// Log logs the controller state in an arbitrary form
	Log(state ControllerState)

	// Close closes the underlying resources
	Close() error
}

type CSVLogger struct {
	file *os.File
}

func NewCSVLogger(path string) (*CSVLogger, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}

	logger := CSVLogger {
		file: file,
	}

	return &logger, nil
}

func New
