#include "Publisher.h"

void BLEPublisher::begin(BLEServer *pServer)
{

    // Init battery service
    this->pBatteryService = pServer->createService(BLEUUID((uint16_t)0x180F));
    this->pBatteryLevelCharacteristic = this->pBatteryService->createCharacteristic(BLEUUID((uint16_t)0x2A19),
                                                                                    BLECharacteristic::PROPERTY_READ |
                                                                                        BLECharacteristic::PROPERTY_NOTIFY);
    this->pBatteryService->addCharacteristic(this->pBatteryLevelCharacteristic);
    this->pBatteryService->start();

    // Init PV service
    this->pPVService = pServer->createService(BLEUUID("b871a2ee-1651-47ac-a22c-e340d834c1ef"));
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

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(this->pBatteryService->getUUID());
    pAdvertising->addServiceUUID(this->pPVService->getUUID());
}

void BLEPublisher::publish(ChargeControllerState &state)
{
    this->pBatteryLevelCharacteristic->setValue(state.batterySOC);
    this->pBatteryLevelCharacteristic->notify();
    printf("Published battery SOC: %u%%\n", state.batterySOC);

    this->pPVVoltageCharacteristic->setValue(state.pvVoltage);
    this->pPVVoltageCharacteristic->notify();
    printf("Published PV Voltage: %.2fV\n", state.pvVoltage);

    this->pPVCurrentCharacteristic->setValue(state.pvCurrent);
    this->pPVCurrentCharacteristic->notify();
    printf("Published PV Current: %.2fA\n", state.pvCurrent);

    this->pPVPowerCharacteristic->setValue(state.pvPower);
    this->pPVPowerCharacteristic->notify();
    printf("Published PV Power: %uW\n", state.pvPower);
}