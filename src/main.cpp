#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Arduino.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include "ChargeController.h"
#include "LoadController.h"
#include "Publisher.h"

#define PROJECT_NAME "Solar Box"
#define RS232_TX_PIN 12
#define RS232_RX_PIN 13
#define RELAY_PIN 15
SoftwareSerial rs232Serial(RS232_RX_PIN, RS232_TX_PIN);

class SolarBox : public BLEServerCallbacks, public BLECharacteristicCallbacks
{
private:
    BLEServer *pServer;
    BLEService *pTriggerService;
    BLECharacteristic *pTriggerLoadCharacteristic;
    uint8_t clientConnectedCount = 0;
    LoadController loadController;
    RenogyChargeController chargeController;
    BLEPublisher publisher;

public:
    void begin()
    {
        BLECharacteristic *pCharacteristic;
        loadController.begin(RELAY_PIN);
        chargeController.begin(rs232Serial);

        BLEDevice::init(PROJECT_NAME);
        Serial.println("Initializing solar box");

        this->pServer = BLEDevice::createServer();
        this->pServer->setCallbacks(this);
        this->publisher.begin(this->pServer);

        // Init trigger service to take inputs from clients
        this->pTriggerService = this->pServer->createService(BLEUUID("385cc70e-8a8c-4827-abc0-d01385aa0574"));
        this->pTriggerLoadCharacteristic = this->pTriggerService->createCharacteristic(BLEUUID("287651ed-3fda-42f4-92c6-7aaca7da634c"),
                                                                                       BLECharacteristic::PROPERTY_WRITE |
                                                                                           BLECharacteristic::PROPERTY_READ |
                                                                                           BLECharacteristic::PROPERTY_NOTIFY);
        this->pTriggerLoadCharacteristic->setCallbacks(this);
        this->pTriggerService->addCharacteristic(this->pTriggerLoadCharacteristic);
        this->pTriggerService->start();
        uint16_t value = 0;
        this->pTriggerLoadCharacteristic->setValue(value);

        BLEAdvertising *pAdvertising = this->pServer->getAdvertising();
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

    virtual void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
    {
        this->loadController.trigger(*(bool *)pCharacteristic->getData());
    }

    void update()
    {
        this->chargeController.update();
        ChargeControllerState state = this->chargeController.state();
        this->loadController.update(state);

        if (this->clientConnectedCount > 0)
        {
            this->publisher.publish(state);
        }
    }
} solarBox;

void setup()
{
    Serial.begin(9600);
    Serial.println("Starting solar charge monitor");
    solarBox.begin();
}

void loop()
{
    solarBox.update();
    delay(1000);
}