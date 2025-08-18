#ifndef IPHONE_ADAPTER_H
#define IPHONE_ADAPTER_H

#include "UwbApi.h"
#include <stdbool.h>

// iPhone-specific constants (from demo_nearby_interaction)
#define IPHONE_SESSION_ID_BASE 0x11223340
#define IPHONE_MAC_ADDR_LEN 8
#define IPHONE_DEFAULT_RANGING_INTERVAL 100  // 100ms

// iPhone connection states (simplified)
typedef enum {
    IPHONE_STATE_DISCONNECTED = 0,
    IPHONE_STATE_BLE_ADVERTISING,
    IPHONE_STATE_BLE_CONNECTED,
    IPHONE_STATE_UWB_ACTIVE,
    IPHONE_STATE_ERROR
} iPhoneState;

// Simple iPhone context (based on demo pattern)
typedef struct {
    int sessionIndex;
    uint32_t sessionId;
    uint32_t sessionHandle;
    iPhoneState state;
    uint8_t macAddr[IPHONE_MAC_ADDR_LEN];
    uint8_t channel;
    uint16_t rangingInterval;
    bool isConnected;
    float lastDistance;
    uint32_t lastRangingTime;
} iPhoneSessionContext;

// Simple adapter context
typedef struct {
    bool isInitialized;
    bool isAdvertising;
    uint16_t bleAdvInterval;
    uint32_t bleAdvTimeout;
} iPhoneAdapter;

// Basic iPhone adapter functions (MVP)
bool iPhoneAdapter_Init(void);
void iPhoneAdapter_Deinit(void);

// BLE functions (simplified for MVP)
bool iPhoneAdapter_StartAdvertising(void);
void iPhoneAdapter_StopAdvertising(void);
void iPhoneAdapter_ProcessBLEEvents(void);

// Status functions
iPhoneState iPhoneAdapter_GetConnectionState(void);
bool iPhoneAdapter_IsConnected(void);
uint32_t iPhoneAdapter_GetSessionId(void);
int iPhoneAdapter_GetSessionIndex(void);

#endif // IPHONE_ADAPTER_H 