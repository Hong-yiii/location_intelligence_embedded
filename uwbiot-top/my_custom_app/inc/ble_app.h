#ifndef BLE_APP_H
#define BLE_APP_H

#include <stdbool.h>
#include <stdint.h>

// Include tlv_manager.h to get UWB_Hif_t definition
#include "tlv_manager.h"

// BLE stack includes and types
#include "ble_general.h"

// Use the proper BLE stack types (already defined in ble_general.h)
// gapGenericEvent_t is already defined in the BLE stack

// BLE constants for compatibility
#ifndef BLE_SUCCESS
#define BLE_SUCCESS gBleSuccess_c
#endif

#ifndef BLE_ERROR_NOT_INITIALIZED
#define BLE_ERROR_NOT_INITIALIZED gBleUnavailable_c
#endif

// BLE configuration variables are defined in ble_app.c with fallbacks

// GATT database initialization
bleResult_t GattDb_Init(void);

// BLE application initialization and management
void BleApp_Init(void);
void BleApp_Start(void);
void BleApp_Stop(void);
void BleApp_ProcessEvents(void);
bool BleApp_IsInitialized(void);
bool BleApp_IsAdvertising(void);
void BleApp_GenericCallback(gapGenericEvent_t *pGenericEvent);

// TLV management (similar to demo)
bool tlvBuilderInit(void);
bool tlvMngInit(void);
bool tlvSendRaw(uint8_t deviceId, uint8_t *data, uint16_t length);
void tlvRecv(uint8_t deviceId, UWB_Hif_t interface, uint8_t *tlv, uint8_t tlvSize);

// Device initialization (similar to demo)
bool handleDeviceInit(void);
bool handleStopSession(uint8_t deviceId);
bool handleShutDown(void);

#endif // BLE_APP_H
