#ifndef IPHONE_ADAPTER_H
#define IPHONE_ADAPTER_H

#include "UwbApi.h"
#include <stdbool.h>

// iPhone-specific constants (from demo_nearby_interaction)
#define IPHONE_SESSION_ID_BASE 0x11223340
#define IPHONE_MAC_ADDR_LEN 8
#define IPHONE_DEFAULT_RANGING_INTERVAL 100  // 100ms

// iPhone connection states
typedef enum {
    IPHONE_STATE_DISCONNECTED = 0,
    IPHONE_STATE_BLE_ADVERTISING,
    IPHONE_STATE_BLE_CONNECTING,
    IPHONE_STATE_BLE_CONNECTED,
    IPHONE_STATE_UWB_INITIALIZING,
    IPHONE_STATE_UWB_ACTIVE,
    IPHONE_STATE_ERROR,
    IPHONE_STATE_RECOVERY
} iPhoneState;

// iPhone session context
typedef struct {
    int sessionIndex;              // Dynamic session slot
    uint32_t sessionId;           // Base ID + slot
    uint32_t sessionHandle;       // UWB session handle
    iPhoneState state;           // Current state
    uint8_t macAddr[IPHONE_MAC_ADDR_LEN];  // iPhone MAC
    uint8_t channel;             // UWB channel
    uint16_t rangingInterval;    // Ranging interval
    bool isConnected;            // BLE connection state
    float lastDistance;          // Last ranging result
    uint32_t lastRangingTime;    // Last ranging timestamp
    uint32_t lastConnectAttempt; // Last connection attempt
    uint8_t connectRetries;      // Connection retry count
    uint32_t errorCount;         // Consecutive errors
    uint32_t recoveryStartTime;  // Recovery start time
} iPhoneSessionContext;

// iPhone adapter context
typedef struct {
    bool isInitialized;          // Adapter initialized
    bool isAdvertising;          // BLE advertising active
    uint16_t bleAdvInterval;     // Advertising interval
    uint32_t bleAdvTimeout;      // Advertising timeout
    uint8_t maxRetries;          // Max connection retries
    uint32_t retryDelay;         // Delay between retries
    uint32_t recoveryTimeout;    // Recovery timeout
    bool autoReconnect;          // Auto reconnect on disconnect
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