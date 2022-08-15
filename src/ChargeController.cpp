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
        {124, 13},
        {123, 12},
        {122, 11},
        {121, 10},
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
    float voltage = 0;

    // Battery Voltage
    result = node.readHoldingRegisters(0x101, 2);
    if (result == node.ku8MBSuccess)
    {
        value = node.getResponseBuffer(0);
        voltage = static_cast<float>(value) * 0.1;
        this->currentState.batteryVoltage = voltage;
        this->currentState.batterySOC = voltageToSOC(value);
    }
    Serial.printf("Got battery voltage: %u (0x%02x)\n", value, result);

    // PV Voltage
    result = node.readHoldingRegisters(0x107, 2);
    if (result == node.ku8MBSuccess)
    {
        value = node.getResponseBuffer(0);
        voltage = static_cast<float>(value) * 0.1;
        this->currentState.pvVoltage = voltage;
    }
    Serial.printf("Got pv voltage: %u (0x%02x)\n", value, result);

    // PV Current
    result = node.readHoldingRegisters(0x108, 2);
    if (result == node.ku8MBSuccess)
    {
        value = node.getResponseBuffer(0);
        float current = static_cast<float>(value) * 0.01;
        this->currentState.pvCurrent = current;
    }
    Serial.printf("Got pv current: %u (0x%02x)\n", value, result);

    // PV Power
    result = node.readHoldingRegisters(0x109, 2);
    if (result == node.ku8MBSuccess)
    {
        value = node.getResponseBuffer(0);
        this->currentState.pvPower = value;
    }
    Serial.printf("Got pv power: %u (0x%02x)\n", value, result);
}