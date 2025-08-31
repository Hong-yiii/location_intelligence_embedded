#include "iphone_adapter.h"
#include "session_manager.h"
#include "resource_manager.h"
#include "ble_app.h"  // Includes tlv_manager.h for UWB_HIF_BLE
#include "phNxpLogApis_App.h"
#include "phOsalUwb.h"
#include <string.h>

// Global instances (simple MVP approach)
static iPhoneAdapter gIPhoneAdapter = {0};
static iPhoneSessionContext gIPhoneSession = {0};

bool iPhoneAdapter_Init(void) {
    if (gIPhoneAdapter.isInitialized) {
        return true;
    }
    
    NXPLOG_APP_I("Initializing iPhone Adapter...");
    
    // Initialize adapter state
    memset(&gIPhoneAdapter, 0, sizeof(iPhoneAdapter));
    memset(&gIPhoneSession, 0, sizeof(iPhoneSessionContext));
    
    // Initialize adapter parameters
    gIPhoneAdapter.bleAdvInterval = 100; // 100ms
    gIPhoneAdapter.bleAdvTimeout = 30000; // 30 seconds
    gIPhoneAdapter.maxRetries = 3;
    gIPhoneAdapter.retryDelay = 1000; // 1 second
    
    NXPLOG_APP_I("BLE parameters: Interval=%d ms, Timeout=%d ms, Retries=%d",
                 gIPhoneAdapter.bleAdvInterval,
                 gIPhoneAdapter.bleAdvTimeout,
                 gIPhoneAdapter.maxRetries);
    
    // Initialize session parameters (but don't create session yet)
    gIPhoneSession.sessionIndex = -1; // No session yet
    gIPhoneSession.sessionId = IPHONE_SESSION_ID_BASE;
    gIPhoneSession.state = IPHONE_STATE_DISCONNECTED;
    gIPhoneSession.channel = 5; // iPhone preferred channel
    gIPhoneSession.rangingInterval = IPHONE_DEFAULT_RANGING_INTERVAL;
    gIPhoneSession.isConnected = false;
    gIPhoneSession.lastConnectAttempt = 0;
    gIPhoneSession.connectRetries = 0;
    
    gIPhoneAdapter.isInitialized = true;
    gIPhoneAdapter.isAdvertising = false;
    
    NXPLOG_APP_I("iPhone Adapter initialized successfully");
    return true;
}

void iPhoneAdapter_Deinit(void) {
    if (!gIPhoneAdapter.isInitialized) {
        return;
    }
    
    NXPLOG_APP_I("Deinitializing iPhone Adapter...");
    
    // Stop BLE advertising if active
    if (gIPhoneAdapter.isAdvertising) {
        iPhoneAdapter_StopAdvertising();
    }
    
    // Terminate iPhone session if active
    if (gIPhoneSession.state != IPHONE_STATE_DISCONNECTED) {
        SessionManager_TerminateSession(gIPhoneSession.sessionIndex);
    }
    
    // Clear state
    memset(&gIPhoneAdapter, 0, sizeof(iPhoneAdapter));
    memset(&gIPhoneSession, 0, sizeof(iPhoneSessionContext));
    
    NXPLOG_APP_I("iPhone Adapter deinitialized");
}

bool iPhoneAdapter_StartAdvertising(void) {
    if (!gIPhoneAdapter.isInitialized) {
        NXPLOG_APP_E("iPhone Adapter not initialized");
        return false;
    }
    
    if (gIPhoneAdapter.isAdvertising) {
        NXPLOG_APP_W("BLE advertising already active");
        return true;
    }
    
    NXPLOG_APP_I("Starting BLE advertising for iPhone discovery...");
    
    // TODO: Start actual BLE advertising (simplified for MVP)
    // This would integrate with the BLE stack from demo_nearby_interaction
    
    gIPhoneAdapter.isAdvertising = true;
    gIPhoneSession.state = IPHONE_STATE_BLE_ADVERTISING;
    
    return true;
}

void iPhoneAdapter_StopAdvertising(void) {
    if (!gIPhoneAdapter.isInitialized) {
        return;
    }
    
    if (!gIPhoneAdapter.isAdvertising) {
        return;
    }
    
    NXPLOG_APP_I("Stopping BLE advertising");
    
    // TODO: Stop actual BLE advertising
    
    gIPhoneAdapter.isAdvertising = false;
    
    if (gIPhoneSession.state == IPHONE_STATE_BLE_ADVERTISING) {
        gIPhoneSession.state = IPHONE_STATE_DISCONNECTED;
    }
}

static bool createIPhoneSession(uint8_t deviceId) {
    // For BLE connections, we don't have the actual MAC address yet
    // Create a dummy MAC address based on deviceId for now
    uint8_t dummyMacAddr[IPHONE_MAC_ADDR_LEN] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, deviceId, 0xFF};

    // Create new session
    int sessionIndex = SessionManager_CreateSession(DEVICE_TYPE_IPHONE, dummyMacAddr, IPHONE_MAC_ADDR_LEN);
    if (sessionIndex < 0) {
        NXPLOG_APP_E("Failed to create iPhone session");
        return false;
    }

    // Update session context
    gIPhoneSession.sessionIndex = sessionIndex;
    gIPhoneSession.sessionId = IPHONE_SESSION_ID_BASE + sessionIndex;
    memcpy(gIPhoneSession.macAddr, dummyMacAddr, IPHONE_MAC_ADDR_LEN);
    gIPhoneSession.state = IPHONE_STATE_BLE_CONNECTED;
    gIPhoneSession.isConnected = true;
    gIPhoneSession.lastConnectAttempt = 0;
    gIPhoneSession.connectRetries = 0;
    gIPhoneSession.errorCount = 0;

    NXPLOG_APP_I("Created iPhone session %d with ID 0x%08X", 
                 sessionIndex, gIPhoneSession.sessionId);
    return true;
}

static void handleBLEConnection(uint8_t deviceId) {
    if (gIPhoneSession.state != IPHONE_STATE_BLE_ADVERTISING) {
        NXPLOG_APP_W("Unexpected BLE connection in state %d", gIPhoneSession.state);
        return;
    }

    // Create session if needed
    if (gIPhoneSession.sessionIndex < 0) {
        if (!createIPhoneSession(deviceId)) {
            return;
        }
    }

    // Start UWB session
    if (!SessionManager_StartSession(gIPhoneSession.sessionIndex)) {
        NXPLOG_APP_E("Failed to start UWB session");
        gIPhoneSession.state = IPHONE_STATE_ERROR;
        return;
    }

    gIPhoneSession.state = IPHONE_STATE_UWB_INITIALIZING;
    NXPLOG_APP_I("iPhone UWB session initializing...");
}

static void handleBLEDisconnection(void) {
    if (!gIPhoneSession.isConnected) {
        return;
    }

    NXPLOG_APP_I("iPhone disconnected - cleaning up session");

    // Stop UWB session
    if (gIPhoneSession.sessionIndex >= 0) {
        SessionManager_StopSession(gIPhoneSession.sessionIndex);
        SessionManager_TerminateSession(gIPhoneSession.sessionIndex);
        gIPhoneSession.sessionIndex = -1;
    }

    // Reset state
    gIPhoneSession.isConnected = false;
    gIPhoneSession.state = IPHONE_STATE_DISCONNECTED;
    gIPhoneSession.errorCount = 0;

    // Restart advertising if auto-reconnect enabled
    if (gIPhoneAdapter.autoReconnect) {
        NXPLOG_APP_I("Auto-reconnect enabled - restarting advertising");
        iPhoneAdapter_StartAdvertising();
    }
}

static void handleUWBError(void) {
    gIPhoneSession.errorCount++;
    
    if (gIPhoneSession.errorCount > gIPhoneAdapter.maxRetries) {
        NXPLOG_APP_E("Too many UWB errors - entering recovery");
        gIPhoneSession.state = IPHONE_STATE_RECOVERY;
        gIPhoneSession.recoveryStartTime = 0;
        phOsalUwb_GetTickCount((unsigned long*)&gIPhoneSession.recoveryStartTime);
        return;
    }

    // Try to restart UWB session
    NXPLOG_APP_W("UWB error - retrying session (attempt %d/%d)", 
                 gIPhoneSession.errorCount, gIPhoneAdapter.maxRetries);
    
    if (gIPhoneSession.sessionIndex >= 0) {
        SessionManager_StopSession(gIPhoneSession.sessionIndex);
        if (SessionManager_StartSession(gIPhoneSession.sessionIndex)) {
            gIPhoneSession.state = IPHONE_STATE_UWB_INITIALIZING;
        } else {
            gIPhoneSession.state = IPHONE_STATE_ERROR;
        }
    }
}

void iPhoneAdapter_ProcessBLEEvents(void) {
    if (!gIPhoneAdapter.isInitialized) {
        return;
    }

    uint32_t currentTime;
    phOsalUwb_GetTickCount((unsigned long*)&currentTime);

    // Process BLE events
    BLE_Event_t event;
    while (BLE_GetNextEvent(&event) == BLE_SUCCESS) {
        switch (event.type) {
        case BLE_EVENT_CONNECTED:
            NXPLOG_APP_I("iPhone BLE connected");
            handleBLEConnection(event.data.deviceId);
            break;

        case BLE_EVENT_DISCONNECTED:
            NXPLOG_APP_I("iPhone BLE disconnected");
            handleBLEDisconnection();
            break;

        case BLE_EVENT_DATA_RECEIVED:
            // Process TLV messages
            tlvRecv(event.data.deviceId, UWB_HIF_BLE, event.data.data, event.data.length);
            break;

        case BLE_EVENT_ADVERTISE_TIMEOUT:
            NXPLOG_APP_I("BLE advertising timeout");
            break;

        case BLE_EVENT_ERROR:
            NXPLOG_APP_E("BLE error occurred");
            break;
        }
    }

    // State machine processing
    switch (gIPhoneSession.state) {
    case IPHONE_STATE_DISCONNECTED:
        // Start advertising if auto-reconnect enabled
        if (gIPhoneAdapter.autoReconnect && !gIPhoneAdapter.isAdvertising) {
            iPhoneAdapter_StartAdvertising();
        }
        break;

    case IPHONE_STATE_BLE_ADVERTISING:
        // Check advertising timeout
        if ((currentTime - gIPhoneSession.lastConnectAttempt) > gIPhoneAdapter.bleAdvTimeout) {
            NXPLOG_APP_W("BLE advertising timeout");
            iPhoneAdapter_StopAdvertising();
            
            if (gIPhoneAdapter.autoReconnect) {
                // Wait retry delay before restarting
                phOsalUwb_Delay(gIPhoneAdapter.retryDelay);
                iPhoneAdapter_StartAdvertising();
            }
        }
        break;

    case IPHONE_STATE_UWB_INITIALIZING:
        // Check UWB initialization timeout
        if ((currentTime - gIPhoneSession.lastConnectAttempt) > 5000) {
            NXPLOG_APP_E("UWB initialization timeout");
            handleUWBError();
        }
        break;

    case IPHONE_STATE_RECOVERY:
        // Check recovery timeout
        if ((currentTime - gIPhoneSession.recoveryStartTime) > gIPhoneAdapter.recoveryTimeout) {
            NXPLOG_APP_I("Recovery timeout - attempting reconnection");
            gIPhoneSession.state = IPHONE_STATE_DISCONNECTED;
            gIPhoneSession.errorCount = 0;
            if (gIPhoneAdapter.autoReconnect) {
                iPhoneAdapter_StartAdvertising();
            }
        }
        break;

    default:
        break;
    }
}

// Status functions
iPhoneState iPhoneAdapter_GetConnectionState(void) {
    return gIPhoneSession.state;
}

bool iPhoneAdapter_IsConnected(void) {
    return (gIPhoneSession.state >= IPHONE_STATE_BLE_CONNECTED);
}

uint32_t iPhoneAdapter_GetSessionId(void) {
    return gIPhoneSession.sessionId;
}

int iPhoneAdapter_GetSessionIndex(void) {
    return gIPhoneSession.sessionIndex;
} 