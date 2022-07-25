#pragma once

#include <Arduino.h>
#include <Stream.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class BLESerial : public Stream
{
public:
    bool begin();
    int available(void);
    int peek(void);
    int read(void);
    size_t write(uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);
    void flush();
    void end(void);

private:
    String local_name;
    BLEServer *pServer = NULL;
    BLEService *pService;
    BLECharacteristic *pTxCharacteristic;
    bool deviceConnected = false;
    uint8_t txValue = 0;

    std::string receiveBuffer;

    friend class BLESerialServerCallbacks;
    friend class BLESerialCharacteristicCallbacks;
};