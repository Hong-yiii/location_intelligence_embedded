#ifndef DISCOVERY_MANAGER_H
#define DISCOVERY_MANAGER_H

#include "UwbApi.h"
#include "session_manager.h"

// Discovery timing constants
#define DISCOVERY_CONTROLLER_WINDOW_MS 8000  // 8 seconds
#define DISCOVERY_CONTROLEE_WINDOW_MS 5000   // 5 seconds
#define DISCOVERY_RETRY_COUNT 3              // Number of full cycles before timeout

// Discovery states
typedef enum {
    DISCOVERY_STATE_IDLE,
    DISCOVERY_STATE_CONTROLLER,
    DISCOVERY_STATE_CONTROLEE,
    DISCOVERY_STATE_MATCHED
} DiscoveryState;

// Discovery role configuration
typedef struct {
    uint8_t macAddr[2];           // Short MAC address
    DiscoveryState state;         // Current discovery state
    uint32_t windowStartTime;     // Start time of current window
    uint8_t retryCount;          // Number of cycles attempted
    bool isActive;               // Whether discovery is active
} DiscoveryContext;

// Discovery manager functions
void DiscoveryManager_Init(void);
void DiscoveryManager_Start(const uint8_t* macAddr);
void DiscoveryManager_Stop(void);

// Role management
void DiscoveryManager_SwitchRole(void);
void DiscoveryManager_HandleTimeout(void);

// Discovery events
void DiscoveryManager_HandleDiscoverySignal(const uint8_t* peerMacAddr);
void DiscoveryManager_HandleResponse(const uint8_t* peerMacAddr);

// Callback type for discovery events
typedef void (*DiscoveryCallback)(const uint8_t* peerMacAddr, bool isController);
void DiscoveryManager_RegisterCallback(DiscoveryCallback callback);

#endif // DISCOVERY_MANAGER_H 