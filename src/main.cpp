#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEBeacon.h>
#include <Arduino.h>
#include <ModbusMaster.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define PROJECT_NAME "solar-charge-controller"
#define RS232_TX_PIN 35
#define RS232_RX_PIN 34

/* BLEServer *pServer = BLEDevice::createServer();
BLEService *pService = pServer->createService(SERVICE_UUID);
BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       ); */

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pBatterySOC;
ModbusMaster node;

void setupBluetooth()
{
    Serial.begin(115200);
    Serial.println("Starting BLE Server!");

    BLEDevice::init(PROJECT_NAME);
    pServer = BLEDevice::createServer();
    pService = pServer->createService(SERVICE_UUID);
    pBatterySOC = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ);
    pService->start();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->start();
    Serial.println("Characteristic defined! Now you can read it in the Client!");
}

void setupRS232()
{
    Serial1.begin(9600, SERIAL_8N1, RS232_RX_PIN, RS232_TX_PIN);
    node.begin(2, Serial1);
}

void setup()
{
    setupBluetooth();
    setupRS232();
}

void loop()
{
    uint8_t result;
    uint16_t data;

    result = node.readHoldingRegisters(0x100, 2);
    if (result == node.ku8MBSuccess)
    {
        data = node.getResponseBuffer(0);
        pBatterySOC->setValue(data);
    }
}