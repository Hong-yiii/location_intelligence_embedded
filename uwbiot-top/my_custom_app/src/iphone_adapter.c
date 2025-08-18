#include "iphone_adapter.h"
#include "session_manager.h"
#include "resource_manager.h"
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
    
    // Initialize iPhone session context (based on demo pattern)
    gIPhoneSession.sessionIndex = 0; // iPhone gets slot 0
    gIPhoneSession.sessionId = IPHONE_SESSION_ID_BASE;
    gIPhoneSession.state = IPHONE_STATE_DISCONNECTED;
    gIPhoneSession.channel = 5; // iPhone preferred channel
    gIPhoneSession.rangingInterval = IPHONE_DEFAULT_RANGING_INTERVAL;
    gIPhoneSession.isConnected = false;
    
    // Initialize BLE advertising parameters (simplified)
    gIPhoneAdapter.bleAdvInterval = 100; // 100ms
    gIPhoneAdapter.bleAdvTimeout = 30000; // 30 seconds
    
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

void iPhoneAdapter_ProcessBLEEvents(void) {
    if (!gIPhoneAdapter.isInitialized || !gIPhoneAdapter.isAdvertising) {
        return;
    }
    
    // TODO: Process BLE events (simplified for MVP)
    // This would handle:
    // - BLE connection events
    // - TLV message parsing
    // - iPhone configuration messages
    // - Ranging start/stop commands
    
    // Simplified state machine for MVP
    static uint32_t lastCheckTime = 0;
    uint32_t currentTime;
    phOsalUwb_GetTickCount((unsigned long*)&currentTime);
    
    // Check for connection events every second (simplified)
    if (currentTime - lastCheckTime > 1000) {
        lastCheckTime = currentTime;
        
        // Simulate iPhone connection detection (for MVP testing)
        if (gIPhoneSession.state == IPHONE_STATE_BLE_ADVERTISING) {
            // In real implementation, this would be triggered by actual BLE events
            // For now, just log that we're ready for iPhone connection
            NXPLOG_APP_D("Ready for iPhone connection...");
        }
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