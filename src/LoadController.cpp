#include "LoadController.h"

static const char *stateToStr(LoadControllerState state)
{
    static const char *table[] = {
        "INACTIVE",
        "ACTIVE",
        "LOW_PV_INPUT",
        "LOW_BATTERY",
    };

    return table[state];
}

void LoadController::begin(uint8_t loadPin)
{
    this->loadPin = loadPin;
    pinMode(loadPin, OUTPUT);
    this->enable(true);
}

const char *LoadController::getState()
{
    return stateToStr(this->state);
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
        if (this->state != LoadControllerState::INACTIVE)
        {
            return;
        }

        this->trigger(true);
        this->lastHighPVInputAt = millis();
        this->lastErrorAt = 0;
        this->state = LoadControllerState::ACTIVE;
    }
    else
    {
        this->trigger(false);
        this->state = LoadControllerState::INACTIVE;
    }
}

bool LoadController::isActive()
{
    return this->state != LoadControllerState::INACTIVE;
}

void LoadController::update(ChargeControllerState &state)
{
    LoadControllerState nextState = this->state;

    switch (this->state)
    {
    case LoadControllerState::INACTIVE:
        break;
    case LoadControllerState::ACTIVE:
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
            nextState = LoadControllerState::ACTIVE;
        }
        break;
    case LoadControllerState::LOW_PV_INPUT:
        if (state.pvPower >= LOAD_PV_WATT_CUTOFF)
        {
            nextState = LoadControllerState::ACTIVE;
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
    else if (nextState == LoadControllerState::ACTIVE)
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

    printf("Changing state from %s to %s\n", stateToStr(this->state), stateToStr(nextState));
    this->state = nextState;
}
