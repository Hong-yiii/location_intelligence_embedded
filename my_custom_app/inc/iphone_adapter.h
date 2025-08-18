#ifndef IPHONE_ADAPTER_H
#define IPHONE_ADAPTER_H

#include "UwbApi.h"
#include "session_manager.h"
#include <stdbool.h>

// iPhone-specific constants
#define IPHONE_SESSION_ID_BASE 0x11223340
#define IPHONE_MAC_ADDR_LEN 8
#define IPHONE_DEFAULT_RANGING_INTERVAL 100  // 100ms

// iPhone connection states
typedef enum {
    IPHONE_STATE_DISCONNECTED = 0,
    IPHONE_STATE_BLE_ADVERTISING,
    IPHONE_STATE_BLE_CONNECTED,
    IPHONE_STATE_UWB_CONFIGURING,
    IPHONE_STATE_UWB_ACTIVE,
    IPHONE_STATE_ERROR
} iPhoneState;

// iPhone session context
typedef struct {
    int sessionIndex;
    uint32_t sessionId;
    uint32_t sessionHandle;
    iPhoneState state;
    uint8_t macAddr[IPHONE_MAC_ADDR_LEN];
    uint8_t deviceId;           // BLE device ID
    bool isConfigured;
    uint32_t lastActivityTime;
} iPhoneContext;

// iPhone adapter functions
bool iPhoneAdapter_Init(void);
void iPhoneAdapter_Deinit(void);

// Connection management
bool iPhoneAdapter_StartAdvertising(void);
void iPhoneAdapter_StopAdvertising(void);
bool iPhoneAdapter_HandleConnection(uint8_t deviceId);
void iPhoneAdapter_HandleDisconnection(uint8_t deviceId);

// Session management
bool iPhoneAdapter_CreateSession(int sessionIndex);
bool iPhoneAdapter_ConfigureSession(int sessionIndex, const uint8_t* shareableData, uint16_t dataLength);
bool iPhoneAdapter_StartRanging(int sessionIndex);
bool iPhoneAdapter_StopRanging(int sessionIndex);

// Protocol handling
void iPhoneAdapter_HandleTLVMessage(uint8_t deviceId, uint8_t* data, uint16_t length);
void iPhoneAdapter_HandleRangingData(int sessionIndex, phRangingData_t* rangingData);

// BLE message handling
bool iPhoneAdapter_SendInitializeResponse(uint8_t deviceId);
bool iPhoneAdapter_SendStartResponse(uint8_t deviceId);
bool iPhoneAdapter_SendStopResponse(uint8_t deviceId);
bool iPhoneAdapter_SendErrorResponse(uint8_t deviceId, uint8_t errorCode);

// State management
iPhoneState iPhoneAdapter_GetState(int sessionIndex);
void iPhoneAdapter_SetState(int sessionIndex, iPhoneState newState);
bool iPhoneAdapter_IsActive(int sessionIndex);

// Configuration
bool iPhoneAdapter_GetUwbConfigData(uint8_t* configData, uint16_t* configDataLength);
bool iPhoneAdapter_ApplyShareableData(int sessionIndex, const uint8_t* shareableData, uint16_t dataLength);

// Utility functions
const char* iPhoneAdapter_GetStateString(iPhoneState state);
void iPhoneAdapter_PrintStatus(int sessionIndex);

// Task management
void iPhoneAdapter_Task(void* args);
void iPhoneAdapter_ProcessBLEEvents(void);

// Callback registration
typedef void (*iPhoneEventCallback)(int sessionIndex, iPhoneState state, void* data);
void iPhoneAdapter_RegisterCallback(iPhoneEventCallback callback);

#endif // IPHONE_ADAPTER_H 