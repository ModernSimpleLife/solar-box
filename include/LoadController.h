#pragma once
#include <Arduino.h>
#include "ChargeController.h"

#define LOAD_PV_WATT_CUTOFF 8
#define BATTERY_SOC_CUTOFF 10
#define SECOND_IN_MILLIS ((uint64_t)1000)
#define MINUTE_IN_MILLIS (SECOND_IN_MILLIS * 60)
#define HOUR_IN_MILLIS (MINUTE_IN_MILLIS * 60)
#define LOAD_TIMEOUT_IN_MILLIS (1 * HOUR_IN_MILLIS)
#define DELAY_AFTER_ERROR_IN_MILLIS (1 * HOUR_IN_MILLIS)

/**
 * INACTIVE -> ACTIVE
 *                |> LOW_BATTERY
 *                |> LOW_PV_INPUT
 */
enum LoadControllerState
{
    INACTIVE,
    ACTIVE,
    LOW_PV_INPUT,
    LOW_BATTERY,
};

class LoadController
{
public:
    void begin(uint8_t loadPin);
    void enable(bool flag);
    void update(ChargeControllerState &state);
    bool isActive();
    const char *getState();

private:
    uint64_t lastHighPVInputAt = 0;
    uint64_t lastErrorAt = 0;
    LoadControllerState state = LoadControllerState::INACTIVE;
    uint8_t loadPin;
    void trigger(bool activate);
};