#include "session_manager.h"
#include "resource_manager.h"
#include "phOsalUwb.h"
#include "phNxpLogApis_App.h"
#include <string.h>

// Global session manager instance
static SessionManager gSessionManager = {0};
static SessionEventCallback gSessionCallback = NULL;

// Forward declarations of static functions
static uint32_t generateSessionId(DeviceType deviceType, int sessionIndex);
static bool SessionManager_ValidateSessionIndex(int sessionIndex);
static const char* SessionManager_GetSessionTypeString(SessionType type);
static bool SessionManager_AllocateResources(int sessionIndex);
static void SessionManager_ReleaseResources(int sessionIndex);

// Session ID generation
static uint32_t generateSessionId(DeviceType deviceType, int sessionIndex) {
    uint32_t baseId = (deviceType == DEVICE_TYPE_IPHONE) ? 0x11223340 : 0x22334400;
    return baseId + sessionIndex;
}

bool SessionManager_Init(void) {
    if (gSessionManager.isInitialized) {
        return true;
    }
    
    NXPLOG_APP_I("Initializing session manager context...");
    memset(&gSessionManager, 0, sizeof(SessionManager));
    
    NXPLOG_APP_I("Initializing %d session slots...", MAX_SESSIONS);
    for (int i = 0; i < MAX_SESSIONS; i++) {
        gSessionManager.sessions[i].sessionId = 0;
        gSessionManager.sessions[i].sessionHandle = 0;
        gSessionManager.sessions[i].type = SESSION_TYPE_NONE;
        gSessionManager.sessions[i].state = SESSION_STATE_IDLE;
        gSessionManager.sessions[i].deviceType = DEVICE_TYPE_UNKNOWN;
        gSessionManager.sessions[i].isActive = false;
        gSessionManager.sessions[i].channel = 0;
        gSessionManager.sessions[i].rangingInterval = 0;
        gSessionManager.sessions[i].lastDistance = 0.0f;
        
        NXPLOG_APP_I("  Slot %d: Reserved=%s", i, 
                     i == IPHONE_SESSION_SLOT ? "iPhone" : "Board");
    
    }
    
    // Create mutex for thread safety
    if (phOsalUwb_CreateMutex(&gSessionManager.rangingMutex) != UWBSTATUS_SUCCESS) {
        NXPLOG_APP_E("Failed to create ranging mutex");
        return false;
    }
    
    gSessionManager.activeSessions = 0;
    gSessionManager.totalSessions = 0;
    gSessionManager.isInitialized = true;
    
    NXPLOG_APP_I("Session Manager initialized successfully");
    return true;
}

void SessionManager_Deinit(void) {
    if (!gSessionManager.isInitialized) {
        return;
    }
    
    // Stop and terminate all active sessions
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (gSessionManager.sessions[i].isActive) {
            SessionManager_TerminateSession(i);
        }
    }
    
    // Destroy mutex
    if (gSessionManager.rangingMutex) {
        phOsalUwb_DeleteMutex(&gSessionManager.rangingMutex);
        gSessionManager.rangingMutex = NULL;
    }
    
    gSessionManager.isInitialized = false;
    NXPLOG_APP_I("Session Manager deinitialized");
}

int SessionManager_CreateSession(DeviceType deviceType, const uint8_t* macAddr, uint8_t macAddrLen) {
    if (!gSessionManager.isInitialized) {
        NXPLOG_APP_E("Session Manager not initialized");
        return -1;
    }
    
    if (gSessionManager.totalSessions >= MAX_SESSIONS) {
        NXPLOG_APP_E("Maximum sessions exceeded (%d)", MAX_SESSIONS);
        return -1;
    }
    
    // Count board sessions and check iPhone slot
    int boardSessions = 0;
    bool iPhoneSlotOccupied = (gSessionManager.sessions[IPHONE_SESSION_SLOT].type != SESSION_TYPE_NONE);
    
    for (int i = 1; i < MAX_SESSIONS; i++) {  // Start from 1 since slot 0 is for iPhone
        if (gSessionManager.sessions[i].type == SESSION_TYPE_BOARD_DS_TWR) {
            boardSessions++;
        }
    }
    
    // Find available session slot
    int sessionIndex = -1;
    
    if (deviceType == DEVICE_TYPE_IPHONE) {
        // iPhone must use slot 0
        if (iPhoneSlotOccupied) {
            NXPLOG_APP_E("iPhone session already exists in slot %d", IPHONE_SESSION_SLOT);
            return -1;
        }
        sessionIndex = IPHONE_SESSION_SLOT;
    } else {
        if (boardSessions >= MAX_BOARD_SESSIONS) {
            NXPLOG_APP_E("Maximum board sessions exceeded (%d)", MAX_BOARD_SESSIONS);
            return -1;
        }
        
        // Find available slot for board (skip slot 0)
        for (int i = 1; i < MAX_SESSIONS; i++) {
            if (gSessionManager.sessions[i].type == SESSION_TYPE_NONE) {
                sessionIndex = i;
                break;
            }
        }
    }
    
    if (sessionIndex == -1) {
        NXPLOG_APP_E("No available session slots");
        return -1;
    }
    
    // Initialize session context
    SessionContext* session = &gSessionManager.sessions[sessionIndex];
    session->sessionId = generateSessionId(deviceType, sessionIndex);
    session->sessionHandle = 0;  // Will be set when UWB session is initialized
    session->type = (deviceType == DEVICE_TYPE_IPHONE) ? SESSION_TYPE_IPHONE_NI : SESSION_TYPE_BOARD_DS_TWR;
    session->state = SESSION_STATE_IDLE;
    session->deviceType = deviceType;
    session->macAddrLen = macAddrLen;
    session->isActive = false;
    session->lastRangingTime = 0;
    session->lastDistance = 0.0f;
    
    // Copy MAC address
    if (macAddr && macAddrLen > 0) {
        memcpy(session->macAddr, macAddr, (macAddrLen > 8) ? 8 : macAddrLen);
    }
    
    // Set default ranging interval
    session->rangingInterval = (deviceType == DEVICE_TYPE_IPHONE) ? 100 : 200;
    
    gSessionManager.totalSessions++;
    
    NXPLOG_APP_I("Created session %d: Type=%s, DeviceType=%d", 
                 sessionIndex, 
                 SessionManager_GetSessionTypeString(session->type),
                 deviceType);
    
    return sessionIndex;
}

bool SessionManager_StartSession(int sessionIndex) {
    if (!SessionManager_ValidateSessionIndex(sessionIndex)) {
        return false;
    }
    
    SessionContext* session = &gSessionManager.sessions[sessionIndex];
    
    if (session->state == SESSION_STATE_ACTIVE) {
        NXPLOG_APP_W("Session %d already active", sessionIndex);
        return true;
    }
    
    // Initialize UWB session first (like demo)
    tUWBAPI_STATUS status = UwbApi_SessionInit(session->sessionId, UWBD_RANGING_SESSION, &session->sessionHandle);
    if (status != UWBAPI_STATUS_OK) {
        NXPLOG_APP_E("UwbApi_SessionInit failed for session %d: 0x%02X", sessionIndex, status);
        return false;
    }
    
    // Only allocate resources after successful UWB session initialization
    if (!SessionManager_AllocateResources(sessionIndex)) {
        NXPLOG_APP_E("Failed to allocate resources for session %d", sessionIndex);
        UwbApi_SessionDeinit(session->sessionHandle);
        session->sessionHandle = 0;
        return false;
    }
    
    // Update session state
    SessionManager_UpdateSessionState(sessionIndex, SESSION_STATE_INITIALIZING);
    session->isActive = true;
    gSessionManager.activeSessions++;
    
    NXPLOG_APP_I("Started session %d: Handle=0x%08X", sessionIndex, session->sessionHandle);
    
    return true;
}

bool SessionManager_StopSession(int sessionIndex) {
    if (!SessionManager_ValidateSessionIndex(sessionIndex)) {
        return false;
    }
    
    SessionContext* session = &gSessionManager.sessions[sessionIndex];
    
    if (!session->isActive) {
        return true;
    }
    
    // Stop UWB ranging session
    if (session->sessionHandle != 0) {
        tUWBAPI_STATUS status = UwbApi_StopRangingSession(session->sessionHandle);
        if (status != UWBAPI_STATUS_OK) {
            NXPLOG_APP_E("UwbApi_StopRangingSession failed for session %d: 0x%02X", sessionIndex, status);
        }
    }
    
    SessionManager_UpdateSessionState(sessionIndex, SESSION_STATE_IDLE);
    session->isActive = false;
    
    if (gSessionManager.activeSessions > 0) {
        gSessionManager.activeSessions--;
    }
    
    NXPLOG_APP_I("Stopped session %d", sessionIndex);
    return true;
}

bool SessionManager_TerminateSession(int sessionIndex) {
    if (!SessionManager_ValidateSessionIndex(sessionIndex)) {
        return false;
    }
    
    SessionContext* session = &gSessionManager.sessions[sessionIndex];
    
    // Stop session if active
    SessionManager_StopSession(sessionIndex);
    
    // Deinitialize UWB session
    if (session->sessionHandle != 0) {
        tUWBAPI_STATUS status = UwbApi_SessionDeinit(session->sessionHandle);
        if (status != UWBAPI_STATUS_OK) {
            NXPLOG_APP_E("UwbApi_SessionDeinit failed for session %d: 0x%02X", sessionIndex, status);
        }
    }
    
    // Release resources
    SessionManager_ReleaseResources(sessionIndex);
    
    // Reset session context
    memset(session, 0, sizeof(SessionContext));
    session->type = SESSION_TYPE_NONE;
    session->state = SESSION_STATE_IDLE;
    session->deviceType = DEVICE_TYPE_UNKNOWN;
    
    if (gSessionManager.totalSessions > 0) {
        gSessionManager.totalSessions--;
    }
    
    NXPLOG_APP_I("Terminated session %d", sessionIndex);
    return true;
}

SessionContext* SessionManager_GetSession(int sessionIndex) {
    if (!SessionManager_ValidateSessionIndex(sessionIndex)) {
        return NULL;
    }
    return &gSessionManager.sessions[sessionIndex];
}

SessionState SessionManager_GetSessionState(int sessionIndex) {
    if (!SessionManager_ValidateSessionIndex(sessionIndex)) {
        return SESSION_STATE_ERROR;
    }
    return gSessionManager.sessions[sessionIndex].state;
}

int SessionManager_GetActiveSessionCount(void) {
    return gSessionManager.activeSessions;
}

bool SessionManager_IsSessionActive(int sessionIndex) {
    if (!SessionManager_ValidateSessionIndex(sessionIndex)) {
        return false;
    }
    return gSessionManager.sessions[sessionIndex].isActive;
}

void SessionManager_UpdateSessionState(int sessionIndex, SessionState newState) {
    if (!SessionManager_ValidateSessionIndex(sessionIndex)) {
        return;
    }
    
    SessionContext* session = &gSessionManager.sessions[sessionIndex];
    NXPLOG_APP_D("Session %d state: %s -> %s", 
                 sessionIndex,
                 SessionManager_GetSessionStateString(session->state),
                 SessionManager_GetSessionStateString(newState));
    
    session->state = newState;
    
    // Notify callback if registered
    if (gSessionCallback) {
        gSessionCallback(sessionIndex, newState, session);
    }
}

void SessionManager_HandleRangingData(int sessionIndex, phRangingData_t* rangingData) {
    if (!SessionManager_ValidateSessionIndex(sessionIndex) || !rangingData) {
        return;
    }
    
    phOsalUwb_LockMutex(gSessionManager.rangingMutex);
    
    SessionContext* session = &gSessionManager.sessions[sessionIndex];
    phOsalUwb_GetTickCount((unsigned long*)&session->lastRangingTime);
    
    // Extract distance from ranging data
    if (rangingData->ranging_meas.range_meas_twr[0].status == UWBAPI_STATUS_OK) {
        session->lastDistance = (float)rangingData->ranging_meas.range_meas_twr[0].distance;
        
        NXPLOG_APP_I("Session %d ranging: Distance=%.2f cm, MAC=%02X%02X", 
                     sessionIndex,
                     session->lastDistance,
                     session->macAddr[0], session->macAddr[1]);
    } else {
        NXPLOG_APP_W("Session %d ranging failed: Status=0x%02X", 
                     sessionIndex, rangingData->ranging_meas.range_meas_twr[0].status);
    }
    
    phOsalUwb_UnlockMutex(gSessionManager.rangingMutex);
}

void SessionManager_HandleSessionError(int sessionIndex, uint32_t errorCode) {
    if (!SessionManager_ValidateSessionIndex(sessionIndex)) {
        return;
    }
    
    NXPLOG_APP_E("Session %d error: 0x%08X", sessionIndex, errorCode);
    SessionManager_UpdateSessionState(sessionIndex, SESSION_STATE_ERROR);
    
    // TODO: Implement error recovery logic
}

static bool SessionManager_AllocateResources(int sessionIndex) {
    if (!SessionManager_ValidateSessionIndex(sessionIndex)) {
        return false;
    }
    
    SessionContext* session = &gSessionManager.sessions[sessionIndex];
    bool isIPhone = (session->deviceType == DEVICE_TYPE_IPHONE);
    
    // Allocate channel
    uint8_t channel = ResourceManager_AllocateChannel(sessionIndex, session->sessionId, isIPhone);
    if (channel == 0) {
        NXPLOG_APP_E("Failed to allocate channel for session %d", sessionIndex);
        return false;
    }
    
    session->channel = channel;
    
    // Allocate time slot
    if (!ResourceManager_AllocateTimeSlot(sessionIndex, session->rangingInterval)) {
        NXPLOG_APP_E("Failed to allocate time slot for session %d", sessionIndex);
        ResourceManager_ReleaseChannel(sessionIndex);
        return false;
    }
    
    NXPLOG_APP_I("Allocated resources for session %d: Channel=%d", sessionIndex, channel);
    return true;
}

static void SessionManager_ReleaseResources(int sessionIndex) {
    if (!SessionManager_ValidateSessionIndex(sessionIndex)) {
        return;
    }
    
    ResourceManager_ReleaseChannel(sessionIndex);
    ResourceManager_ReleaseTimeSlot(sessionIndex);
    
    NXPLOG_APP_I("Released resources for session %d", sessionIndex);
}

void SessionManager_ProcessEvents(void) {
    // Process any pending session events
    // This can be extended to handle queued events, timeouts, etc.
    
    static uint32_t lastProcessTime = 0;
    uint32_t currentTime;
    phOsalUwb_GetTickCount((unsigned long*)&currentTime);
    
    // Process every 1 second
    if (currentTime - lastProcessTime < 1000) {
        return;
    }
    
    lastProcessTime = currentTime;
    
    NXPLOG_APP_D("Processing session events...");
    
    // Check for session timeouts or health issues
    int activeCount = 0;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        SessionContext* session = &gSessionManager.sessions[i];
        
        if (!session->isActive) {
            continue;
        }
        
        activeCount++;
        NXPLOG_APP_D("Session %d: Type=%s, State=%s, LastRanging=%lu ms ago", 
                     i,
                     SessionManager_GetSessionTypeString(session->type),
                     SessionManager_GetSessionStateString(session->state),
                     session->lastRangingTime > 0 ? currentTime - session->lastRangingTime : 0);
        
        // Check for ranging timeout (no data for 10 seconds)
        if (session->lastRangingTime > 0 && 
            (currentTime - session->lastRangingTime) > 10000) {
            NXPLOG_APP_W("Session %d ranging timeout - no data for %lu ms", 
                        i, currentTime - session->lastRangingTime);
            // Could trigger recovery here
        }
    }
    
    NXPLOG_APP_D("Active sessions: %d/%d", activeCount, MAX_SESSIONS);
}

static bool SessionManager_ValidateSessionIndex(int sessionIndex) {
    if (sessionIndex < 0 || sessionIndex >= MAX_SESSIONS) {
        NXPLOG_APP_E("Invalid session index: %d", sessionIndex);
        return false;
    }
    return true;
}

static const char* SessionManager_GetSessionTypeString(SessionType type) {
    switch (type) {
    case SESSION_TYPE_NONE: return "None";
    case SESSION_TYPE_IPHONE_NI: return "iPhone-NI";
    case SESSION_TYPE_BOARD_DS_TWR: return "Board-DSTWR";
    default: return "Unknown";
    }
}



void SessionManager_RegisterCallback(SessionEventCallback callback) {
    gSessionCallback = callback;
} 