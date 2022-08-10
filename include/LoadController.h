#pragma once
#include <Arduino.h>
#include "ChargeController.h"

#define LOAD_PV_WATT_CUTOFF 10
#define BATTERY_SOC_CUTOFF 15
#define HOUR_IN_MILLIS ((uint64_t)1 * 60 * 60 * 1000)
#define LOAD_TIMEOUT_IN_MILLIS (1 * HOUR_IN_MILLIS)
#define DELAY_AFTER_ERROR_IN_MILLIS (1 * HOUR_IN_MILLIS)

/**
 * DISABLED -> ENABLED
 *                |> LOW_BATTERY
 *                |> LOW_PV_INPUT
 */
enum LoadControllerState
{
    Disabled,
    Enabled,
    LOW_PV_INPUT,
    LOW_BATTERY,
};

class LoadController
{
public:
    void begin(uint8_t loadPin);
    void enable(bool flag);
    void update(ChargeControllerState &state);
    bool isEnabled();

private:
    uint64_t lastHighPVInputAt = 0;
    uint64_t lastErrorAt = 0;
    LoadControllerState state = LoadControllerState::Disabled;
    uint8_t loadPin;
    void trigger(bool activate);
};