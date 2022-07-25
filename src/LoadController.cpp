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

    if (activate)
    {
        this->lastActivatedAt = millis();
    }
}

void LoadController::enable(bool flag)
{
    Serial.printf("Load is %s\n", flag ? "ENABLED" : "DISABLED");
    this->enabled = flag;

    // If disabled, turn off load immediately. The controller won't close the relay.
    // Else, close the relay and let the future update logic to decide when to turn off.
    // When there's no input, the relay will at least be closed for LOAD_TIMEOUT_IN_MILLIS
    this->trigger(flag);
}

bool LoadController::isEnabled()
{
    return this->enabled;
}

void LoadController::update(ChargeControllerState &state)
{
    if (!this->enabled)
    {
        return;
    }

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
