#include "ChargeController.h"

static uint16_t voltageToSOC(uint16_t voltage)
{
    static uint16_t socTable[][2] = {
        {136, 100},
        {134, 99},
        {133, 90},
        {132, 70},
        {131, 40},
        {130, 30},
        {129, 20},
        {128, 17},
        {125, 14},
        {120, 9},
        {100, 0},
    };
    static size_t socTableLength = sizeof(socTable) / sizeof(socTable[0]);
    for (size_t i = 0; i < socTableLength; i++)
    {
        if (voltage >= socTable[i][0])
        {
            return socTable[i][1];
        }
    }

    return 0;
}

void RenogyChargeController::begin(Stream &serial)
{
    // TODO: fix slave id assignment to be dynamic
    this->node.begin(1, serial);
}

void RenogyChargeController::update()
{
    uint8_t result;
    uint16_t value = 0;

    // Battery Voltage
    result = node.readHoldingRegisters(0x101, 2);
    value = result == node.ku8MBSuccess ? node.getResponseBuffer(0) : 0;
    float voltage = static_cast<float>(value) * 0.1;
    Serial.printf("Got battery voltage: %u (0x%02x)\n", value, result);
    this->currentState.batteryVoltage = voltage;

    // PV Voltage
    result = node.readHoldingRegisters(0x107, 2);
    value = result == node.ku8MBSuccess ? node.getResponseBuffer(0) : 0;
    Serial.printf("Got pv voltage: %u (0x%02x)\n", value, result);
    voltage = static_cast<float>(value) * 0.1;
    this->currentState.pvVoltage = voltage;

    // PV Current
    result = node.readHoldingRegisters(0x108, 2);
    value = result == node.ku8MBSuccess ? node.getResponseBuffer(0) : 0;
    Serial.printf("Got pv current: %u (0x%02x)\n", value, result);
    float current = static_cast<float>(value) * 0.01;
    this->currentState.pvCurrent = current;

    // PV Power
    result = node.readHoldingRegisters(0x109, 2);
    value = result == node.ku8MBSuccess ? node.getResponseBuffer(0) : 0;
    Serial.printf("Got pv power: %u (0x%02x)\n", value, result);
    this->currentState.pvPower = value;
}