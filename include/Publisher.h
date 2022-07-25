#pragma once
#include <Arduino.h>
#include "ChargeController.h"
#include <BLEServer.h>

class Publisher
{
public:
    virtual void publish(ChargeControllerState &state) = 0;
};

class BLEPublisher : public Publisher
{
public:
    void begin(BLEServer *pServer);
    void publish(ChargeControllerState &state);

private:
    uint8_t clientConnectedCount = 0;

    BLEService *pBatteryService;
    BLECharacteristic *pBatteryLevelCharacteristic;

    BLEService *pPVService;
    BLECharacteristic *pPVVoltageCharacteristic;
    BLECharacteristic *pPVCurrentCharacteristic;
    BLECharacteristic *pPVPowerCharacteristic;

    BLEService *pTriggerService;
    BLECharacteristic *pTriggerLoadCharacteristic;
};