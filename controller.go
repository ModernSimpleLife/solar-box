package main

import (
	"fmt"
	"time"

	"github.com/goburrow/modbus"
)

type ControllerState struct {
	// Temperature retrieves the current charge controller's temperature in Celsius
	Temperature float64 `csv:"temperature_in_celsius"`

	// PVVoltage retrieves the current Photovoltaic's voltage
	PVVoltage float64 `csv:"pv_voltage"`

	// PVCurrent retrieves the current Photovoltaic's current in amps
	PVCurrent float64 `csv:"pv_current_in_amps"`

	// PVPower retrieves the current Photovoltaic's charging power in watts
	PVPower float64 `csv:"pv_power_in_watts"`

	// BatteryVoltage retrieves the current battery's voltage
	BatteryVoltage float64 `csv:"battery_voltage"`

	// BatterySOC retrieves the battery's state of charge in percentage
	BatterySOC float64 `csv:"battery_soc_in_percentage"`

	// ChargingCurrent retrieves the amount amps go to the battery
	ChargingCurrent float64 `csv:"charging_current_in_amps"`
}

type RenogyController struct {
	client        modbus.Client
	handler       *modbus.RTUClientHandler
	lastUpdatedAt time.Time
	state         ControllerState
}

func NewRenogyController() (Controller, error) {
	// Modbus RTU
	handler := modbus.NewRTUClientHandler("/dev/ttyUSB0")
	handler.BaudRate = 9600
	handler.DataBits = 8
	handler.Parity = "N"
	handler.StopBits = 1
	handler.Timeout = 2 * time.Second

	err := handler.Connect()
	if err != nil {
		return nil, fmt.Errorf("failed to connect to Renogy charge controller: %v", err)
	}

	client := modbus.NewClient(handler)

	controller := RenogyController{
		handler: handler,
		client:  client,
	}

	return &controller, nil
}

func (controller *RenogyController) Close() error {
	return controller.handler.Close()
}

func (controller *RenogyController) State() ControllerState {
	return controller.state
}

func (controller *RenogyController) Update() error {
	var err error
	newState := controller.state

	newState.BatterySOC, err = controller.BatterySOC() 
	if err != nil {
		return err
	}

	newState.BatteryVoltage, err = controller.BatteryVoltage() 
	if err != nil {
		return err
	}

	newState.ChargingCurrent, err = controller.ChargingCurrent() 
	if err != nil {
		return err
	}

	newState.PVCurrent, err = controller.PVCurrent() 
	if err != nil {
		return err
	}

	newState.PVVoltage, err = controller.PVVoltage() 
	if err != nil {
		return err
	}

	newState.PVPower, err = controller.PVPower() 
	if err != nil {
		return err
	}

	newState.Temperature, err = controller.Temperature() 
	if err != nil {
		return err
	}

	controller.lastUpdatedAt = time.Now()
	controller.state = newState
	return nil
}

func (controller *RenogyController) readUint16(address uint16) (uint16, error) {
	results, err := controller.client.ReadHoldingRegisters(address, 2)
	if err != nil {
		return 0, err
	}

	val := (uint16(results[0]) << 8) | uint16(results[1])
	return val, nil
}

func (controller *RenogyController) Temperature() (float64, error) {
	results, err := controller.client.ReadHoldingRegisters(0x103, 2)
	if err != nil {
		return 0, err
	}

	// Index 0 contains the controller's temperature
	value := results[0]
	// Actual temperature value (b7: sign bit; b0-b6: temperature value)
	sign := value >> 7
	temp := float64(value & 0xfe)

	// negative temperature
	if sign == 1 {
		temp *= -1
	}

	return temp, nil
}

func (controller *RenogyController) PVVoltage() (float64, error) {
	val, err := controller.readUint16(0x107)
	if err != nil {
		return 0, err
	}

	return float64(val) * 0.1, nil
}

func (controller *RenogyController) PVCurrent() (float64, error) {
	val, err := controller.readUint16(0x108)
	if err != nil {
		return 0, err
	}

	return float64(val) * 0.01, nil
}

func (controller *RenogyController) PVPower() (float64, error) {
	val, err := controller.readUint16(0x109)
	if err != nil {
		return 0, err
	}

	return float64(val), nil
}

func (controller *RenogyController) BatterySOC() (float64, error) {
	val, err := controller.readUint16(0x100)
	if err != nil {
		return 0, err
	}

	return float64(val), nil
}

func (controller *RenogyController) BatteryVoltage() (float64, error) {
	val, err := controller.readUint16(0x101)
	if err != nil {
		return 0, err
	}

	return float64(val) * 0.1, nil
}

func (controller *RenogyController) ChargingCurrent() (float64, error) {
	val, err := controller.readUint16(0x102)
	if err != nil {
		return 0, err
	}

	return float64(val) * 0.01, nil
}
