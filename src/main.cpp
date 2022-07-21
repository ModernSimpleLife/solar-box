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

class TriggerCallback : public BLECharacteristicCallbacks
{
    virtual void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
    {
        bool triggerOn = *(bool *)pCharacteristic->getData();
        digitalWrite(RELAY_PIN, triggerOn ? HIGH : LOW);
    }
};

class SolarBox : public BLEServerCallbacks
{
private:
    BLEServer *pServer;
    bool clientConnected = false;
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
        pinMode(RELAY_PIN, OUTPUT);

        // Init rs232 and modbus
        Serial2.begin(9600, SERIAL_8N1, RS232_RX_PIN, RS232_TX_PIN);
        node.begin(1, Serial2);

        BLEDevice::init(PROJECT_NAME);
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

        BLEAdvertising *pAdvertising = instance.pServer->getAdvertising();
        pAdvertising->addServiceUUID(instance.pBatteryService->getUUID());
        pAdvertising->addServiceUUID(instance.pPVService->getUUID());
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->start();
    }

    virtual void onConnect(BLEServer *pServer)
    {
        this->clientConnected = true;
        pServer->startAdvertising();
        Serial.println("Client got connected");
    }

    virtual void onDisconnect(BLEServer *pServer)
    {
        this->clientConnected = false;
        Serial.println("Client got disconnected");
    }

    void update()
    {
        uint8_t result;
        uint16_t value;

        if (!this->clientConnected)
        {
            return;
        }

        // Battery Voltage
        result = node.readHoldingRegisters(0x101, 2);
        if (result == node.ku8MBSuccess)
        {
            value = voltageToSOC(node.getResponseBuffer(0));
            this->pBatteryLevelCharacteristic->setValue(value);
            this->pBatteryLevelCharacteristic->notify();
        }

        // PV Voltage
        result = node.readHoldingRegisters(0x107, 2);
        if (result == node.ku8MBSuccess)
        {
            value = node.getResponseBuffer(0);
            float voltage = static_cast<float>(value) * 0.1;
            this->pPVVoltageCharacteristic->setValue(voltage);
            this->pPVVoltageCharacteristic->notify();
        }

        // PV Current
        result = node.readHoldingRegisters(0x108, 2);
        if (result == node.ku8MBSuccess)
        {
            value = node.getResponseBuffer(0);
            float current = static_cast<float>(value) * 0.01;
            this->pPVCurrentCharacteristic->setValue(current);
            this->pPVCurrentCharacteristic->notify();
        }

        // PV Power
        result = node.readHoldingRegisters(0x108, 2);
        if (result == node.ku8MBSuccess)
        {
            value = node.getResponseBuffer(0);
            this->pPVPowerCharacteristic->setValue(value);
            this->pPVPowerCharacteristic->notify();
        }
    }
};

void setup()
{
    Serial.begin(9600);
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
    SolarBox &solarBox = SolarBox::getInstance();

    solarBox.update();
    delay(1000);
}