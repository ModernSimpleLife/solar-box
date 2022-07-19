#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEBeacon.h>
#include <Arduino.h>
#include <BLE2902.h>
#include <ModbusMaster.h>

#define PROJECT_NAME "Solar Box"
#define RS232_TX_PIN 17
#define RS232_RX_PIN 16
#define BATTERY_SERVICE_UUID BLEUUID((uint16_t)0x180F)
#define BATTERY_CHARACTERISTIC_UUID BLEUUID((uint16_t)0x2A19)
#define BATTERY_DESCRIPTOR_UUID BLEUUID((uint16_t)0x2901)

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pBatterySOC;
BLEDescriptor batteryLevelDescriptor(BATTERY_DESCRIPTOR_UUID);
ModbusMaster node;

class SolarBLECallbacks : BLEServerCallbacks
{

    virtual void onConnect(BLEServer *pServer)
    {
        Serial.println("Client got connected");
    }

    virtual void onDisconnect(BLEServer *pServer)
    {
        Serial.println("Client got disconnected");
    }
};

SolarBLECallbacks bleCallbacks;

void setupBluetooth()
{
    BLEDevice::init(PROJECT_NAME);
    // Create the BLE Server
    pServer = BLEDevice::createServer();
    // Create the BLE Service
    pService = pServer->createService(BATTERY_SERVICE_UUID);

    pBatterySOC = pService->createCharacteristic(BATTERY_CHARACTERISTIC_UUID,
                                                 BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

    batteryLevelDescriptor.setValue("Percentage 0 - 100");
    pBatterySOC->addDescriptor(&batteryLevelDescriptor);
    // Let client decides when to get notified to save power on the server side
    pBatterySOC->addDescriptor(new BLE2902());

    pService->addCharacteristic(pBatterySOC);
    pServer->setCallbacks((BLEServerCallbacks *)&bleCallbacks);
    pService->start();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(BATTERY_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->start();
}

void setupRS232()
{
    Serial2.begin(9600, SERIAL_8N1, RS232_RX_PIN, RS232_TX_PIN);
    node.begin(1, Serial2);
}

void setup()
{
    Serial.begin(9600);
    setupRS232();
    setupBluetooth();
    Serial.println("Starting solar charge monitor");
}

uint16_t voltageToSOC(uint16_t voltage)
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

void loop()
{
    uint8_t result;
    uint16_t soc;

    Serial.println("Looping");
    result = node.readHoldingRegisters(0x101, 2);
    if (result == node.ku8MBSuccess)
    {
        soc = voltageToSOC(node.getResponseBuffer(0));
        Serial.printf("Current battery SOC is %u%%\n", soc);
        pBatterySOC->setValue(soc);
        pBatterySOC->notify();
    }
    else
    {
        Serial.println("failed to get the current battery SOC");
    }

    delay(5000);
}