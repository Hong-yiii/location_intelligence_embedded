#include "discovery_manager.h"
#include "phOsalUwb.h"
#include "UwbApi.h"
#include "phNxpLogApis_App.h"

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
    NXPLOG_APP_I("Initializing Discovery Manager...");
    
    memset(&gDiscoveryCtx, 0, sizeof(DiscoveryContext));
    gDiscoveryCtx.state = DISCOVERY_STATE_IDLE;
    gDiscoveryCallback = NULL;
    
    // Create timer for role switching
    NXPLOG_APP_I("Creating discovery timer...");
    gDiscoveryTimer = phOsalUwb_Timer_Create(0);
    
    NXPLOG_APP_I("Discovery Manager initialized successfully");
}

void DiscoveryManager_Start(const uint8_t* macAddr) {
    if (gDiscoveryCtx.isActive) {
        NXPLOG_APP_W("Discovery already active, ignoring start request");
        return;
    }
    
    NXPLOG_APP_I("Starting discovery with MAC: 0x%02X%02X", macAddr[0], macAddr[1]);
    
    // Initialize discovery context
    memcpy(gDiscoveryCtx.macAddr, macAddr, 2);
    gDiscoveryCtx.state = DISCOVERY_STATE_CONTROLLER;  // Start as controller
    gDiscoveryCtx.retryCount = 0;
    gDiscoveryCtx.isActive = true;
    
    NXPLOG_APP_I("Starting first discovery window as CONTROLLER");
    startDiscoveryWindow();
}

void DiscoveryManager_Stop(void) {
    if (!gDiscoveryCtx.isActive) {
        NXPLOG_APP_W("Discovery not active, ignoring stop request");
        return;
    }
    
    NXPLOG_APP_I("Stopping discovery (state: %s, retries: %d)", 
                 gDiscoveryCtx.state == DISCOVERY_STATE_CONTROLLER ? "CONTROLLER" : "CONTROLEE",
                 gDiscoveryCtx.retryCount);
    
    // Stop timer and reset state
    phOsalUwb_Timer_Stop(gDiscoveryTimer);
    gDiscoveryCtx.isActive = false;
    gDiscoveryCtx.state = DISCOVERY_STATE_IDLE;
    
    NXPLOG_APP_I("Discovery stopped successfully");
}

void DiscoveryManager_SwitchRole(void) {
    if (!gDiscoveryCtx.isActive) {
        NXPLOG_APP_W("Discovery not active, ignoring role switch request");
        return;
    }
    
    NXPLOG_APP_I("Switching discovery role (current: %s, retries: %d)", 
                 gDiscoveryCtx.state == DISCOVERY_STATE_CONTROLLER ? "CONTROLLER" : "CONTROLEE",
                 gDiscoveryCtx.retryCount);
    
    // Toggle between controller and controlee
    if (gDiscoveryCtx.state == DISCOVERY_STATE_CONTROLLER) {
        gDiscoveryCtx.state = DISCOVERY_STATE_CONTROLEE;
        NXPLOG_APP_I("Starting CONTROLEE window (%d ms)", DISCOVERY_CONTROLEE_WINDOW_MS);
        phOsalUwb_Timer_Start(gDiscoveryTimer, DISCOVERY_CONTROLEE_WINDOW_MS, handleWindowTimeout, NULL);
    } else {
        gDiscoveryCtx.state = DISCOVERY_STATE_CONTROLLER;
        NXPLOG_APP_I("Starting CONTROLLER window (%d ms)", DISCOVERY_CONTROLLER_WINDOW_MS);
        phOsalUwb_Timer_Start(gDiscoveryTimer, DISCOVERY_CONTROLLER_WINDOW_MS, handleWindowTimeout, NULL);
    }
    
    // Update window start time
    phOsalUwb_GetTickCount((unsigned long*)&gDiscoveryCtx.windowStartTime);
    
    // Start UWB in new role
    NXPLOG_APP_I("Starting UWB discovery in new role");
    startDiscoveryWindow();
}

static void startDiscoveryWindow(void) {
    NXPLOG_APP_I("Starting discovery window...");
    
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
    
    NXPLOG_APP_I("Configured for role: %s, type: %s", 
                 rangingParams.deviceRole == kUWB_DeviceRole_Initiator ? "INITIATOR" : "RESPONDER",
                 rangingParams.deviceType == kUWB_DeviceType_Controller ? "CONTROLLER" : "CONTROLEE");
    
    // Copy MAC address
    memcpy(rangingParams.deviceMacAddr, gDiscoveryCtx.macAddr, 2);
    
    // Initialize discovery session
    NXPLOG_APP_I("Initializing UWB session (ID: 0x%08X)...", DISCOVERY_SESSION_ID);
    UwbApi_SessionInit(DISCOVERY_SESSION_ID, UWBD_RANGING_SESSION, &sessionHandle);
    
    NXPLOG_APP_I("Setting ranging parameters...");
    UwbApi_SetRangingParams(sessionHandle, &rangingParams);
    
    NXPLOG_APP_I("Starting ranging session...");
    UwbApi_StartRangingSession(sessionHandle);
}

void DiscoveryManager_HandleDiscoverySignal(const uint8_t* peerMacAddr) {
    if (!gDiscoveryCtx.isActive || gDiscoveryCtx.state != DISCOVERY_STATE_CONTROLEE) {
        NXPLOG_APP_W("Ignoring discovery signal (active: %d, state: %s)", 
                     gDiscoveryCtx.isActive,
                     gDiscoveryCtx.state == DISCOVERY_STATE_CONTROLLER ? "CONTROLLER" : "CONTROLEE");
        return;
    }
    
    NXPLOG_APP_I("Received discovery signal from MAC: 0x%02X%02X", peerMacAddr[0], peerMacAddr[1]);
    
    // Send response
    // Note: Response is handled automatically by UWB stack in DS-TWR
    NXPLOG_APP_I("Automatic DS-TWR response will be sent by UWB stack");
    
    if (gDiscoveryCallback) {
        NXPLOG_APP_I("Notifying discovery callback (role: CONTROLEE)");
        gDiscoveryCallback(peerMacAddr, false);  // We're controlee
    }
}

void DiscoveryManager_HandleResponse(const uint8_t* peerMacAddr) {
    if (!gDiscoveryCtx.isActive || gDiscoveryCtx.state != DISCOVERY_STATE_CONTROLLER) {
        NXPLOG_APP_W("Ignoring discovery response (active: %d, state: %s)", 
                     gDiscoveryCtx.isActive,
                     gDiscoveryCtx.state == DISCOVERY_STATE_CONTROLLER ? "CONTROLLER" : "CONTROLEE");
        return;
    }
    
    NXPLOG_APP_I("Received discovery response from MAC: 0x%02X%02X", peerMacAddr[0], peerMacAddr[1]);
    
    // Match found
    NXPLOG_APP_I("Discovery match found! Stopping discovery timer");
    gDiscoveryCtx.state = DISCOVERY_STATE_MATCHED;
    phOsalUwb_Timer_Stop(gDiscoveryTimer);
    
    if (gDiscoveryCallback) {
        NXPLOG_APP_I("Notifying discovery callback (role: CONTROLLER)");
        gDiscoveryCallback(peerMacAddr, true);  // We're controller
    }
}

static void handleWindowTimeout(uint32_t TimerId, void *pContext) {
    if (!gDiscoveryCtx.isActive) {
        NXPLOG_APP_W("Window timeout ignored - discovery not active");
        return;
    }
    
    NXPLOG_APP_I("Discovery window timeout (state: %s, retry: %d/%d)", 
                 gDiscoveryCtx.state == DISCOVERY_STATE_CONTROLLER ? "CONTROLLER" : "CONTROLEE",
                 gDiscoveryCtx.retryCount + 1, DISCOVERY_RETRY_COUNT);
    
    gDiscoveryCtx.retryCount++;
    
    if (gDiscoveryCtx.retryCount >= DISCOVERY_RETRY_COUNT) {
        NXPLOG_APP_W("Discovery failed after %d retries - stopping", DISCOVERY_RETRY_COUNT);
        DiscoveryManager_Stop();
        return;
    }
    
    NXPLOG_APP_I("Switching roles and retrying discovery...");
    DiscoveryManager_SwitchRole();
}

void DiscoveryManager_RegisterCallback(DiscoveryCallback callback) {
    gDiscoveryCallback = callback;
} 