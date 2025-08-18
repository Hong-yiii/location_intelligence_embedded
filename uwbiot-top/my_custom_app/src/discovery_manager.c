#include "discovery_manager.h"
#include "phOsalUwb.h"
#include "UwbApi.h"

// Discovery session ID and constants
#define DISCOVERY_SESSION_ID 0xAABBCCDD
#define DEMO_RANGING_APP_DEVICE_MAC_ADD_MODE_SHORT 0x0

static DiscoveryContext gDiscoveryCtx;
static DiscoveryCallback gDiscoveryCallback;
static uint32_t gDiscoveryTimer;

// Internal functions
static void startDiscoveryWindow(void);
static void handleWindowTimeout(uint32_t TimerId, void *pContext);

void DiscoveryManager_Init(void) {
    memset(&gDiscoveryCtx, 0, sizeof(DiscoveryContext));
    gDiscoveryCtx.state = DISCOVERY_STATE_IDLE;
    gDiscoveryCallback = NULL;
    
    // Create timer for role switching
    gDiscoveryTimer = phOsalUwb_Timer_Create(0);
}

void DiscoveryManager_Start(const uint8_t* macAddr) {
    if (gDiscoveryCtx.isActive) {
        return;  // Already in discovery
    }
    
    // Initialize discovery context
    memcpy(gDiscoveryCtx.macAddr, macAddr, 2);
    gDiscoveryCtx.state = DISCOVERY_STATE_CONTROLLER;  // Start as controller
    gDiscoveryCtx.retryCount = 0;
    gDiscoveryCtx.isActive = true;
    
    // Start first discovery window
    startDiscoveryWindow();
}

void DiscoveryManager_Stop(void) {
    if (!gDiscoveryCtx.isActive) {
        return;
    }
    
    // Stop timer and reset state
    phOsalUwb_Timer_Stop(gDiscoveryTimer);
    gDiscoveryCtx.isActive = false;
    gDiscoveryCtx.state = DISCOVERY_STATE_IDLE;
}

void DiscoveryManager_SwitchRole(void) {
    if (!gDiscoveryCtx.isActive) {
        return;
    }
    
    // Toggle between controller and controlee
    if (gDiscoveryCtx.state == DISCOVERY_STATE_CONTROLLER) {
        gDiscoveryCtx.state = DISCOVERY_STATE_CONTROLEE;
        phOsalUwb_Timer_Start(gDiscoveryTimer, DISCOVERY_CONTROLEE_WINDOW_MS, handleWindowTimeout, NULL);
    } else {
        gDiscoveryCtx.state = DISCOVERY_STATE_CONTROLLER;
        phOsalUwb_Timer_Start(gDiscoveryTimer, DISCOVERY_CONTROLLER_WINDOW_MS, handleWindowTimeout, NULL);
    }
    
    // Update window start time
    phOsalUwb_GetTickCount((unsigned long*)&gDiscoveryCtx.windowStartTime);
    
    // Start UWB in new role
    startDiscoveryWindow();
}

static void startDiscoveryWindow(void) {
    phRangingParams_t rangingParams = {0};
    uint32_t sessionHandle;
    
    // Configure ranging parameters for discovery
    rangingParams.deviceRole = (gDiscoveryCtx.state == DISCOVERY_STATE_CONTROLLER) ? 
                             kUWB_DeviceRole_Initiator : kUWB_DeviceRole_Responder;
    rangingParams.multiNodeMode = kUWB_MultiNodeMode_UniCast;
    rangingParams.macAddrMode = DEMO_RANGING_APP_DEVICE_MAC_ADD_MODE_SHORT;
    rangingParams.deviceType = (gDiscoveryCtx.state == DISCOVERY_STATE_CONTROLLER) ?
                             kUWB_DeviceType_Controller : kUWB_DeviceType_Controlee;
    rangingParams.scheduledMode = kUWB_ScheduledMode_TimeScheduled;
    rangingParams.rangingRoundUsage = kUWB_RangingRoundUsage_DS_TWR;
    
    // Copy MAC address
    memcpy(rangingParams.deviceMacAddr, gDiscoveryCtx.macAddr, 2);
    
    // Initialize discovery session
    UwbApi_SessionInit(DISCOVERY_SESSION_ID, UWBD_RANGING_SESSION, &sessionHandle);
    UwbApi_SetRangingParams(sessionHandle, &rangingParams);
    UwbApi_StartRangingSession(sessionHandle);
}

void DiscoveryManager_HandleDiscoverySignal(const uint8_t* peerMacAddr) {
    if (!gDiscoveryCtx.isActive || gDiscoveryCtx.state != DISCOVERY_STATE_CONTROLEE) {
        return;  // Ignore if not in controlee mode
    }
    
    // Send response
    // Note: Response is handled automatically by UWB stack in DS-TWR
    
    if (gDiscoveryCallback) {
        gDiscoveryCallback(peerMacAddr, false);  // We're controlee
    }
}

void DiscoveryManager_HandleResponse(const uint8_t* peerMacAddr) {
    if (!gDiscoveryCtx.isActive || gDiscoveryCtx.state != DISCOVERY_STATE_CONTROLLER) {
        return;  // Ignore if not in controller mode
    }
    
    // Match found
    gDiscoveryCtx.state = DISCOVERY_STATE_MATCHED;
    phOsalUwb_Timer_Stop(gDiscoveryTimer);
    
    if (gDiscoveryCallback) {
        gDiscoveryCallback(peerMacAddr, true);  // We're controller
    }
}

static void handleWindowTimeout(uint32_t TimerId, void *pContext) {
    if (!gDiscoveryCtx.isActive) {
        return;
    }
    
    gDiscoveryCtx.retryCount++;
    
    if (gDiscoveryCtx.retryCount >= DISCOVERY_RETRY_COUNT) {
        // Discovery failed after max retries
        DiscoveryManager_Stop();
        return;
    }
    
    // Switch roles and try again
    DiscoveryManager_SwitchRole();
}

void DiscoveryManager_RegisterCallback(DiscoveryCallback callback) {
    gDiscoveryCallback = callback;
} 