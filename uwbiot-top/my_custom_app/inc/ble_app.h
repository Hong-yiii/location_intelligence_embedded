#ifndef BLE_APP_H
#define BLE_APP_H

#include <stdbool.h>
#include <stdint.h>

// BLE application initialization and management
void BleApp_Init(void);
void BleApp_Start(void);
void BleApp_Stop(void);
void BleApp_GenericCallback(void *pGenericEvent);

// TLV management (similar to demo)
bool tlvBuilderInit(void);
bool tlvMngInit(void);
bool tlvSendRaw(uint8_t deviceId, uint8_t *data, uint16_t length);
void tlvRecv(uint8_t deviceId, uint8_t hif, uint8_t *data, uint16_t length);

// Device initialization (similar to demo)
bool handleDeviceInit(void);
bool handleStopSession(uint8_t deviceId);
bool handleShutDown(void);

#endif // BLE_APP_H
