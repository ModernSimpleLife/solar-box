#pragma once
#include <Arduino.h>
#include "ChargeController.h"

#define LOAD_PV_WATT_CUTOFF 10
// #define LOAD_TIMEOUT_IN_MILLIS ((uint64_t)1 * 60 * 60 * 1000)
#define LOAD_TIMEOUT_IN_MILLIS ((uint64_t)5 * 1000)

class LoadController
{
public:
    void begin(uint8_t loadPin);
    void enable(bool flag);
    void update(ChargeControllerState &state);
    bool isEnabled();

private:
    uint64_t lastActivatedAt = 0;
    uint8_t loadPin;
    bool enabled = true;
    void trigger(bool activate);
};