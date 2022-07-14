#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEBeacon.h>
#include <Arduino.h>
#include <BLE2902.h>
#include <ModbusMaster.h>

#define PROJECT_NAME "Solar Box"
#define RS232_TX_PIN 1
#define RS232_RX_PIN 3
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
    pServer->getAdvertising()->addServiceUUID(BATTERY_SERVICE_UUID);
    pService->start();
    // Start advertising
    pServer->getAdvertising()->start();
}

void setupRS232()
{
    Serial1.begin(9600, SERIAL_8N1, RS232_RX_PIN, RS232_TX_PIN);
    node.begin(2, Serial1);
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting solar charge monitor");
    setupRS232();
    setupBluetooth();
}

void loop()
{
    uint8_t result;
    uint16_t data;

    result = node.readHoldingRegisters(0x100, 2);
    if (result == node.ku8MBSuccess)
    {
        data = node.getResponseBuffer(0);
        Serial.printf("Current battery SOC is %u%%\n", data);
        pBatterySOC->setValue(data);
        pBatterySOC->notify();
    }
    else
    {
        Serial.println("failed to get the current battery SOC");
    }

    delay(5000);
}