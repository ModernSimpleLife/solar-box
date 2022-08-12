// #define ENABLE_OTA
// #include <soc/soc.h>
// #include <soc/rtc_cntl_reg.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Arduino.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#ifdef ENABLE_OTA
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <ESPmDNS.h>
#endif
#include "ChargeController.h"
#include "LoadController.h"
#include "Publisher.h"

#define PROJECT_NAME "Solar Box"
#define RS232_TX_PIN 12
#define RS232_RX_PIN 13
#define RELAY_PIN 15
#define FLASHLIGHT_PIN 4
SoftwareSerial rs232Serial(RS232_RX_PIN, RS232_TX_PIN);

#ifdef ENABLE_OTA
const char *WIFI_SSID = "Herman";
const char *WIFI_PASS = "pooppoop";
AsyncWebServer otaServer(80);
#endif

class FlashlightCallback : public BLECharacteristicCallbacks
{
    virtual void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
    {
        bool activate = *(bool *)pCharacteristic->getData();
        Serial.printf("Flashlight is %s\n", activate ? "ON" : "OFF");
        digitalWrite(FLASHLIGHT_PIN, activate ? HIGH : LOW);
    }
};

class SolarBox : public BLEServerCallbacks, public BLECharacteristicCallbacks
{
private:
    BLEServer *pServer;
    BLEService *pTriggerService;
    BLECharacteristic *pTriggerLoadCharacteristic;
    BLECharacteristic *pTriggerFlashlightCharacteristic;
    BLECharacteristic *pTriggerStateCharacteristic;
    FlashlightCallback flashlightCb;
    uint8_t clientConnectedCount = 0;
    LoadController loadController;
    RenogyChargeController chargeController;
    BLEPublisher publisher;

public:
    void begin()
    {
        BLECharacteristic *pCharacteristic;
        loadController.begin(RELAY_PIN);
        rs232Serial.begin(9600, SWSERIAL_8N1);
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
        this->pTriggerFlashlightCharacteristic = this->pTriggerService->createCharacteristic(BLEUUID("c660aa6e-38f4-446b-b5eb-af6140864610"),
                                                                                             BLECharacteristic::PROPERTY_WRITE |
                                                                                                 BLECharacteristic::PROPERTY_READ |
                                                                                                 BLECharacteristic::PROPERTY_NOTIFY);
        this->pTriggerFlashlightCharacteristic->setCallbacks(&flashlightCb);

        this->pTriggerStateCharacteristic = this->pTriggerService->createCharacteristic(BLEUUID("287651ed-3fda-42f4-92c6-7aaca7da634d"),
                                                                                        BLECharacteristic::PROPERTY_READ |
                                                                                            BLECharacteristic::PROPERTY_NOTIFY);
        this->pTriggerService->addCharacteristic(this->pTriggerLoadCharacteristic);
        this->pTriggerService->addCharacteristic(this->pTriggerFlashlightCharacteristic);
        this->pTriggerService->addCharacteristic(this->pTriggerStateCharacteristic);
        this->pTriggerService->start();
        uint16_t value = 0;
        this->pTriggerLoadCharacteristic->setValue(value);
        this->pTriggerFlashlightCharacteristic->setValue(value);
        this->pTriggerStateCharacteristic->setValue(this->loadController.getState());

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
        Serial.printf("Client got connected. %u clients are connected\n", this->clientConnectedCount);
    }

    virtual void onDisconnect(BLEServer *pServer)
    {
        this->clientConnectedCount--;
        Serial.printf("Client got disconnected. %u clients are connected\n", this->clientConnectedCount);
    }

    virtual void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
    {
        this->loadController.enable(*(bool *)pCharacteristic->getData());
    }

    void update()
    {
        this->chargeController.update();
        ChargeControllerState state = this->chargeController.state();
        this->loadController.update(state);

        if (this->clientConnectedCount > 0)
        {
            // the built-in red led works with inverted signals
            this->publisher.publish(state);
            uint16_t loadControllerEnabled = this->loadController.isActive() ? 1 : 0;
            printf("Load is %s\n", this->loadController.isActive() ? "ENABLED" : "DISABLED");
            this->pTriggerLoadCharacteristic->setValue(loadControllerEnabled);
            this->pTriggerLoadCharacteristic->notify();
            this->pTriggerStateCharacteristic->setValue(this->loadController.getState());
            this->pTriggerStateCharacteristic->notify();
        }
    }
} solarBox;

void setup()
{
    // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    setCpuFrequencyMhz(80);
    Serial.begin(9600);
    Serial.printf("Set CPU frequency to %u Mhz\n", getCpuFrequencyMhz());
    Serial.println("Starting solar charge monitor");
    pinMode(FLASHLIGHT_PIN, OUTPUT);

#ifdef ENABLE_OTA
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    if (!MDNS.begin("esp32"))
    {
        Serial.println("Error setting up MDNS responder!");
    }

    AsyncElegantOTA.begin(&otaServer);
    otaServer.begin();
    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
#endif

    solarBox.begin();
}

void loop()
{
    solarBox.update();
    delay(1000);
}