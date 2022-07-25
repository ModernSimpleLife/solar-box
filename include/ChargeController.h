#pragma once
#include <Arduino.h>
#include <ModbusMaster.h>

struct ChargeControllerState
{
public:
    float pvVoltage = 0.0;
    float pvCurrent = 0.0;
    uint16_t pvPower = 0;
    float batteryVoltage = 0.0;
    float batterySOC = 0.0;
};

class ChargeController
{
public:
    virtual void update() = 0;
    virtual ChargeControllerState state()
    {
        return this->currentState;
    }

protected:
    ChargeControllerState currentState;
};

class RenogyChargeController : public ChargeController
{
public:
    void begin(Stream &serial);
    virtual void update();

private:
    ModbusMaster node;
};