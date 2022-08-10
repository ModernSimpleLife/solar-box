#include "LoadController.h"

void LoadController::begin(uint8_t loadPin)
{
    this->loadPin = loadPin;
    pinMode(loadPin, OUTPUT);
    this->enable(true);
}

void LoadController::trigger(bool activate)
{
    Serial.printf("Load is %s\n", activate ? "ACTIVE" : "INACTIVE");
    digitalWrite(this->loadPin, activate ? LOW : HIGH);
}

void LoadController::enable(bool flag)
{
    if (flag)
    {
        if (this->state != LoadControllerState::Disabled)
        {
            return;
        }

        this->trigger(true);
        this->lastHighPVInputAt = millis();
        this->lastErrorAt = 0;
        this->state = LoadControllerState::Enabled;
    }
    else
    {
        this->trigger(false);
        this->state = LoadControllerState::Disabled;
    }
}

bool LoadController::isEnabled()
{
    return this->state != LoadControllerState::Disabled;
}

void LoadController::update(ChargeControllerState &state)
{
    LoadControllerState nextState = this->state;

    switch (this->state)
    {
    case LoadControllerState::Disabled:
        break;
    case LoadControllerState::Enabled:
        if (state.batterySOC < BATTERY_SOC_CUTOFF)
        {
            nextState = LoadControllerState::LOW_BATTERY;
        }
        else if (state.pvPower < LOAD_PV_WATT_CUTOFF)
        {
            uint64_t delaySinceLastHighPVInput = millis() - this->lastHighPVInputAt;
            if (delaySinceLastHighPVInput > LOAD_TIMEOUT_IN_MILLIS)
            {
                nextState = LoadControllerState::LOW_PV_INPUT;
            }
        }
        else
        {
            this->lastHighPVInputAt = millis();
        }
        break;
    case LoadControllerState::LOW_BATTERY:
        if (state.batterySOC >= BATTERY_SOC_CUTOFF)
        {
            nextState = LoadControllerState::Enabled;
        }
        break;
    case LoadControllerState::LOW_PV_INPUT:
        if (state.pvPower >= LOAD_PV_WATT_CUTOFF)
        {
            nextState = LoadControllerState::Enabled;
        }
        break;
    }

    if (nextState == this->state)
    {
        return;
    }

    if (nextState == LoadControllerState::LOW_BATTERY || nextState == LoadControllerState::LOW_PV_INPUT)
    {
        this->lastErrorAt = millis();
        this->trigger(false);
    }
    else if (nextState == LoadControllerState::Enabled)
    {
        uint64_t delaySinceLastError = millis() - this->lastErrorAt;
        if (delaySinceLastError > DELAY_AFTER_ERROR_IN_MILLIS)
        {
            this->trigger(true);
        }
        else
        {
            // Within error delay window, cancel state transition
            nextState = this->state;
        }
    }

    this->state = nextState;
}
