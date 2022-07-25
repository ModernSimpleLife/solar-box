#include "LoadController.h"

void LoadController::begin(uint8_t loadPin)
{
    this->loadPin = loadPin;
    pinMode(loadPin, OUTPUT);
}

void LoadController::trigger(bool activate)
{
    digitalWrite(this->loadPin, activate ? HIGH : LOW);

    if (activate)
    {
        this->lastActivatedAt = millis();
    }
}

void LoadController::update(ChargeControllerState &state)
{
    if (state.pvPower > LOAD_PV_WATT_CUTOFF)
    {
        this->trigger(true);
    }
    else
    {
        uint64_t elapsed = millis() - this->lastActivatedAt;
        if (elapsed > LOAD_TIMEOUT_IN_MILLIS)
        {
            this->trigger(false);
        }
    }
}
