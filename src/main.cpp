#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Arduino.h>
#include <ModbusMaster.h>

#define PROJECT_NAME "Solar Box"
#define RS232_TX_PIN 17
#define RS232_RX_PIN 16
#define RELAY_PIN 15
#define MOCK

#define LOAD_PV_WATT_CUTOFF 10
#define LOAD_TIMEOUT_IN_MILLIS ((uint64_t)1 * 60 * 60 * 1000)

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

class LoadController : public BLECharacteristicCallbacks
{
public:
    uint64_t lastActivatedAt = 0;

    void trigger(bool activate)
    {
        digitalWrite(RELAY_PIN, activate ? HIGH : LOW);

        if (activate)
        {
            this->lastActivatedAt = millis();
        }
    }

    virtual void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
    {
        this->trigger(*(bool *)pCharacteristic->getData());
    }

    void setCurrentPVPower(uint16_t watts)
    {
        if (watts > LOAD_PV_WATT_CUTOFF)
        {
            this->trigger(true);
        }
    }

    void update()
    {
        uint64_t elapsed = millis() - this->lastActivatedAt;
        if (elapsed > LOAD_TIMEOUT_IN_MILLIS)
        {
            this->trigger(false);
        }
    }
} loadController;

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

public:
    void begin()
    {
        BLECharacteristic *pCharacteristic;

        Serial.println("Initializing relay");
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, LOW);

#ifndef MOCK
        // Init rs232 and modbus
        // FIXME: This line will get stuck when there's data in the RX pin, pressing the reset button fixes it.
        Serial2.begin(9600, SERIAL_8N1, RS232_RX_PIN, RS232_TX_PIN);
        this->node.begin(1, Serial2);
#endif

        Serial.println("Initializing ble");
        BLEDevice::init(PROJECT_NAME);

        this->pServer = BLEDevice::createServer();
        this->pServer->setCallbacks(this);

        // Init battery service
        this->pBatteryService = this->pServer->createService(BLEUUID((uint16_t)0x180F));
        this->pBatteryLevelCharacteristic = this->pBatteryService->createCharacteristic(BLEUUID((uint16_t)0x2A19),
                                                                                        BLECharacteristic::PROPERTY_READ |
                                                                                            BLECharacteristic::PROPERTY_NOTIFY);
        this->pBatteryService->addCharacteristic(this->pBatteryLevelCharacteristic);
        this->pBatteryService->start();

        // Init PV service
        this->pPVService = this->pServer->createService(BLEUUID("b871a2ee-1651-47ac-a22c-e340d834c1ef"));
        this->pPVVoltageCharacteristic = this->pPVService->createCharacteristic(BLEUUID("46e98325-92b7-4e5f-84c9-8edcbd9338db"),
                                                                                BLECharacteristic::PROPERTY_READ |
                                                                                    BLECharacteristic::PROPERTY_NOTIFY);
        this->pPVCurrentCharacteristic = this->pPVService->createCharacteristic(BLEUUID("91b3d4db-550b-464f-8127-16eeb209dd1d"),
                                                                                BLECharacteristic::PROPERTY_READ |
                                                                                    BLECharacteristic::PROPERTY_NOTIFY);
        this->pPVPowerCharacteristic = this->pPVService->createCharacteristic(BLEUUID("2c85bbb9-0e1a-4bbb-8315-f7cc29831515"),
                                                                              BLECharacteristic::PROPERTY_READ |
                                                                                  BLECharacteristic::PROPERTY_NOTIFY);
        this->pPVService->addCharacteristic(this->pPVVoltageCharacteristic);
        this->pPVService->addCharacteristic(this->pPVCurrentCharacteristic);
        this->pPVService->addCharacteristic(this->pPVPowerCharacteristic);
        this->pPVService->start();

        // Init trigger service to take inputs from clients
        this->pTriggerService = this->pServer->createService(BLEUUID("385cc70e-8a8c-4827-abc0-d01385aa0574"));
        this->pTriggerLoadCharacteristic = this->pTriggerService->createCharacteristic(BLEUUID("287651ed-3fda-42f4-92c6-7aaca7da634c"),
                                                                                       BLECharacteristic::PROPERTY_WRITE |
                                                                                           BLECharacteristic::PROPERTY_READ |
                                                                                           BLECharacteristic::PROPERTY_NOTIFY);
        this->pTriggerLoadCharacteristic->setCallbacks(&loadController);
        this->pTriggerService->addCharacteristic(this->pTriggerLoadCharacteristic);
        this->pTriggerService->start();
        uint16_t value = 0;
        this->pTriggerLoadCharacteristic->setValue(value);

        BLEAdvertising *pAdvertising = this->pServer->getAdvertising();
        pAdvertising->addServiceUUID(this->pBatteryService->getUUID());
        pAdvertising->addServiceUUID(this->pPVService->getUUID());
        pAdvertising->addServiceUUID(this->pTriggerService->getUUID());
        pAdvertising->setScanResponse(false);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->start();

        Serial.println("Initialized Solar Box successfully");
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
        loadController.setCurrentPVPower(value);
    }
} solarBox;

void setup()
{
    Serial.begin(9600);
    while (!Serial)
    {
    }
    Serial.println("Starting solar charge monitor");
    solarBox.begin();
}

void loop()
{
    solarBox.update();
    loadController.update();
    delay(1000);
}