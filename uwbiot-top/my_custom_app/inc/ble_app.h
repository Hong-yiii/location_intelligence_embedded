#ifndef BLE_APP_H
#define BLE_APP_H

#include <stdbool.h>
#include <stdint.h>

// BLE stack return codes
#define BLE_SUCCESS 0
#define BLE_ERROR_NOT_INITIALIZED 1
#define BLE_ERROR_ALREADY_INITIALIZED 2
#define BLE_ERROR_INVALID_PARAM 3
#define BLE_ERROR_BUSY 4

// BLE event types
typedef enum {
    BLE_EVENT_CONNECTED,
    BLE_EVENT_DISCONNECTED,
    BLE_EVENT_ADVERTISE_TIMEOUT,
    BLE_EVENT_DATA_RECEIVED,
    BLE_EVENT_ERROR
} BleEventType;

// BLE event structure
typedef struct {
    BleEventType type;
    union {
        struct {
            uint8_t deviceId;
            uint8_t *data;
            uint16_t length;
        } data;
        uint32_t error;
    };
} BLE_Event_t;

// BLE stack functions (implemented by platform)
int BLE_Init(void);
int BLE_StartAdvertising(uint8_t *data, uint16_t length, uint32_t interval);
int BLE_StopAdvertising(void);
int BLE_GetNextEvent(BLE_Event_t *event);
int BLE_QueueEvent(BLE_Event_t *event);

// BLE application initialization and management
void BleApp_Init(void);
void BleApp_Start(void);
void BleApp_Stop(void);
void BleApp_ProcessEvents(void);
bool BleApp_IsInitialized(void);
bool BleApp_IsAdvertising(void);
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
