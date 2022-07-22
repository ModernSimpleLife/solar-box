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
#define RELAY_PIN 16
#define MOCK

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

class TriggerCallback : public BLECharacteristicCallbacks
{
    virtual void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
    {
        bool triggerOn = *(bool *)pCharacteristic->getData();
        Serial.printf("Load is %s\n", triggerOn ? "ACTIVE" : "INACTIVE");
        digitalWrite(RELAY_PIN, triggerOn ? HIGH : LOW);
    }
};

class SolarBox : public BLEServerCallbacks
{
private:
    BLEServer *pServer;
    uint8_t clientConnectedCount = 0;
    ModbusMaster node;

    BLEService *pBatteryService;
    BLECharacteristic *pBatteryLevelCharacteristic;

    BLEService *pPVService;
    BLECharacteristic *pPVVoltageCharacteristic;
    BLECharacteristic *pPVCurrentCharacteristic;
    BLECharacteristic *pPVPowerCharacteristic;

    BLEService *pTriggerService;
    BLECharacteristic *pTriggerLoadCharacteristic;
    TriggerCallback triggerCallback;

    SolarBox()
    {
    }

public:
    static SolarBox &
    getInstance()
    {
        static bool initialized = false;
        static SolarBox instance;
        BLECharacteristic *pCharacteristic;

        if (initialized)
        {
            return instance;
        }

#ifndef MOCK
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, LOW);

        // Init rs232 and modbus
        Serial2.begin(9600, SERIAL_8N1, RS232_RX_PIN, RS232_TX_PIN);
        instance.node.begin(1, Serial2);
#endif

        BLEDevice::init(PROJECT_NAME);

        instance.pServer = BLEDevice::createServer();
        instance.pServer->setCallbacks(&instance);

        // Init battery service
        instance.pBatteryService = instance.pServer->createService(BLEUUID((uint16_t)0x180F));
        instance.pBatteryLevelCharacteristic = instance.pBatteryService->createCharacteristic(BLEUUID((uint16_t)0x2A19),
                                                                                              BLECharacteristic::PROPERTY_READ |
                                                                                                  BLECharacteristic::PROPERTY_NOTIFY);
        instance.pBatteryService->addCharacteristic(instance.pBatteryLevelCharacteristic);
        instance.pBatteryService->start();

        // Init PV service
        instance.pPVService = instance.pServer->createService(BLEUUID("b871a2ee-1651-47ac-a22c-e340d834c1ef"));
        instance.pPVVoltageCharacteristic = instance.pPVService->createCharacteristic(BLEUUID("46e98325-92b7-4e5f-84c9-8edcbd9338db"),
                                                                                      BLECharacteristic::PROPERTY_READ |
                                                                                          BLECharacteristic::PROPERTY_NOTIFY);
        instance.pPVCurrentCharacteristic = instance.pPVService->createCharacteristic(BLEUUID("91b3d4db-550b-464f-8127-16eeb209dd1d"),
                                                                                      BLECharacteristic::PROPERTY_READ |
                                                                                          BLECharacteristic::PROPERTY_NOTIFY);
        instance.pPVPowerCharacteristic = instance.pPVService->createCharacteristic(BLEUUID("2c85bbb9-0e1a-4bbb-8315-f7cc29831515"),
                                                                                    BLECharacteristic::PROPERTY_READ |
                                                                                        BLECharacteristic::PROPERTY_NOTIFY);
        instance.pPVService->addCharacteristic(instance.pPVVoltageCharacteristic);
        instance.pPVService->addCharacteristic(instance.pPVCurrentCharacteristic);
        instance.pPVService->addCharacteristic(instance.pPVPowerCharacteristic);
        instance.pPVService->start();

        // Init trigger service to take inputs from clients
        instance.pTriggerService = instance.pServer->createService(BLEUUID("385cc70e-8a8c-4827-abc0-d01385aa0574"));
        instance.pTriggerLoadCharacteristic = instance.pTriggerService->createCharacteristic(BLEUUID("287651ed-3fda-42f4-92c6-7aaca7da634c"),
                                                                                             BLECharacteristic::PROPERTY_WRITE |
                                                                                                 BLECharacteristic::PROPERTY_READ |
                                                                                                 BLECharacteristic::PROPERTY_NOTIFY);
        instance.pTriggerLoadCharacteristic->setCallbacks(&instance.triggerCallback);
        instance.pTriggerService->addCharacteristic(instance.pTriggerLoadCharacteristic);
        instance.pTriggerService->start();
        uint16_t value = 0;
        instance.pTriggerLoadCharacteristic->setValue(value);

        BLEAdvertising *pAdvertising = instance.pServer->getAdvertising();
        pAdvertising->addServiceUUID(instance.pBatteryService->getUUID());
        pAdvertising->addServiceUUID(instance.pPVService->getUUID());
        pAdvertising->addServiceUUID(instance.pTriggerService->getUUID());
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->start();

        initialized = true;
        Serial.println("Initialized Solar Box successfully");
        return instance;
    }

    virtual void onConnect(BLEServer *pServer)
    {
        this->clientConnectedCount++;
        pServer->startAdvertising();
        Serial.printf("Client got connected. %u clients are connected", this->clientConnectedCount);
    }

    virtual void onDisconnect(BLEServer *pServer)
    {
        this->clientConnectedCount--;
        Serial.printf("Client got disconnected. %u clients are connected\n", this->clientConnectedCount);
    }

    void update()
    {
        uint8_t result;
        static uint16_t value = 0;

        if (this->clientConnectedCount == 0)
        {
            return;
        }

        // Battery Voltage
#ifdef MOCK
        value++;
        if (value > 10)
        {
            value = 0;
        }
        Serial.printf("Sending: %u\n", value);
#endif

#ifndef MOCK
        result = node.readHoldingRegisters(0x101, 2);
        value = result == node.ku8MBSuccess ? voltageToSOC(node.getResponseBuffer(0)) : 0;
#endif
        this->pBatteryLevelCharacteristic->setValue(value);
        this->pBatteryLevelCharacteristic->notify();

// PV Voltage
#ifndef MOCK
        result = node.readHoldingRegisters(0x107, 2);
        value = result == node.ku8MBSuccess ? node.getResponseBuffer(0) : 0;
#endif
        float voltage = static_cast<float>(value) * 0.1;
        this->pPVVoltageCharacteristic->setValue(voltage);
        this->pPVVoltageCharacteristic->notify();

// PV Current
#ifndef MOCK
        result = node.readHoldingRegisters(0x108, 2);
        value = result == node.ku8MBSuccess ? node.getResponseBuffer(0) : 0;
#endif
        float current = static_cast<float>(value) * 0.01;
        this->pPVCurrentCharacteristic->setValue(current);
        this->pPVCurrentCharacteristic->notify();

// PV Power
#ifndef MOCK
        result = node.readHoldingRegisters(0x109, 2);
        value = result == node.ku8MBSuccess ? node.getResponseBuffer(0) : 0;
#endif
        this->pPVPowerCharacteristic->setValue(value);
        this->pPVPowerCharacteristic->notify();
    }
};

void setup()
{
    Serial.begin(9600);
    Serial.println("Starting solar charge monitor");
}

void loop()
{
    SolarBox &solarBox = SolarBox::getInstance();

    solarBox.update();
    delay(1000);
}