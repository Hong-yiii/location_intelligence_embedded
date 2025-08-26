#include "board_adapter.h"
#include "session_manager.h"
#include "resource_manager.h"
#include "discovery_manager.h"
#include "phNxpLogApis_App.h"
#include "phOsalUwb.h"
#include <string.h>

// UWB Event Types (simple definitions)
#define UWB_EVENT_SESSION_STARTED  1
#define UWB_EVENT_SESSION_STOPPED  2
#define UWB_EVENT_RANGING_DATA      3

// Global board discovery context
static BoardDiscoveryContext gBoardDiscovery = {0};

// Event callback
static BoardEventCallback gBoardEventCallback = NULL;

// Forward declarations
static void onBoardDiscovered(uint16_t macAddr, BoardRole role);
static void handleBoardSessionEvent(uint32_t sessionId, int eventType, void* pData);
static void initializeBoardSession(int sessionIndex, uint16_t macAddr, BoardRole role);

bool BoardAdapter_Init(void) {
    if (gBoardDiscovery.isActive) {
        return true;
    }
    
    NXPLOG_APP_I("Initializing Board Adapter...");
    
    NXPLOG_APP_I("Initializing discovery context...");
    memset(&gBoardDiscovery, 0, sizeof(BoardDiscoveryContext));
    
    // Initialize board contexts
    NXPLOG_APP_I("Initializing board contexts...");
    for (int i = 0; i < MAX_BOARD_SESSIONS; i++) {
        gBoardDiscovery.boards[i].sessionIndex = i + 1; // Reserve slot 0 for iPhone
        gBoardDiscovery.boards[i].sessionId = BOARD_SESSION_ID_BASE + i;
        gBoardDiscovery.boards[i].state = BOARD_STATE_IDLE;
        gBoardDiscovery.boards[i].currentRole = BOARD_ROLE_CONTROLLER; // Default role
        gBoardDiscovery.boards[i].channel = UWB_CHANNEL_6 + (i % 2); // Alternate channels 6,8
        gBoardDiscovery.boards[i].isDiscovered = false;
        NXPLOG_APP_I("  Board %d: Session ID=0x%08X, Channel=%d", 
                     i, gBoardDiscovery.boards[i].sessionId, gBoardDiscovery.boards[i].channel);
    }
    
    // Initialize discovery manager
    DiscoveryManager_Init();
    
    gBoardDiscovery.isActive = true;
    gBoardDiscovery.discoveredBoards = 0;
    gBoardDiscovery.currentDiscoveryRole = BOARD_ROLE_CONTROLLER;
    
    NXPLOG_APP_I("Board Adapter initialized successfully");
    return true;
}

void BoardAdapter_Deinit(void) {
    if (!gBoardDiscovery.isActive) {
        return;
    }
    
    NXPLOG_APP_I("Deinitializing Board Adapter...");
    
    // Stop discovery if active
    if (gBoardDiscovery.isActive) {
        BoardAdapter_StopDiscovery();
    }
    
    // Terminate all active sessions
    for (int i = 0; i < MAX_BOARD_SESSIONS; i++) {
        if (gBoardDiscovery.boards[i].state != BOARD_STATE_IDLE) {
            SessionManager_TerminateSession(gBoardDiscovery.boards[i].sessionIndex);
        }
    }
    
    // Deinitialize discovery manager
    DiscoveryManager_Stop();
    
    // Clear discovery context
    memset(&gBoardDiscovery, 0, sizeof(BoardDiscoveryContext));
    
    NXPLOG_APP_I("Board Adapter deinitialized");
}

bool BoardAdapter_StartDiscovery(uint8_t targetBoardCount) {
    if (!gBoardDiscovery.isActive) {
        NXPLOG_APP_E("Board Adapter not initialized");
        return false;
    }
    
    if (gBoardDiscovery.isActive) {
        NXPLOG_APP_W("Discovery already active");
        return true;
    }
    
    NXPLOG_APP_I("Starting board discovery (target: %d boards)", targetBoardCount);
    
    gBoardDiscovery.targetBoardCount = targetBoardCount;
    gBoardDiscovery.discoveredBoards = 0;
    
    // Start discovery manager with dummy MAC (not used for board discovery)
    uint8_t dummyMac[2] = {0x00, 0x00};
    DiscoveryManager_Start(dummyMac);
    
    gBoardDiscovery.isActive = true;
    
    // Set all board sessions to discovering state
    for (int i = 0; i < MAX_BOARD_SESSIONS; i++) {
        if (gBoardDiscovery.boards[i].state == BOARD_STATE_IDLE) {
            gBoardDiscovery.boards[i].state = BOARD_STATE_DISCOVERING;
        }
    }
    
    return true;
}

void BoardAdapter_StopDiscovery(void) {
    if (!gBoardDiscovery.isActive) {
        NXPLOG_APP_W("Discovery not active");
        return;
    }
    
    NXPLOG_APP_I("Stopping board discovery");
    
    // Stop discovery manager
    DiscoveryManager_Stop();
    
    gBoardDiscovery.isActive = false;
    
    // Reset discovering board sessions to idle
    for (int i = 0; i < MAX_BOARD_SESSIONS; i++) {
        if (gBoardDiscovery.boards[i].state == BOARD_STATE_DISCOVERING) {
            gBoardDiscovery.boards[i].state = BOARD_STATE_IDLE;
        }
    }
}

void BoardAdapter_ProcessDiscoveryEvents(void) {
    if (!gBoardDiscovery.isActive || !gBoardDiscovery.isActive) {
        return;
    }
    
    // Process any discovery events
    NXPLOG_APP_D("Processing discovery events...");
    
    // Check for discovered boards that need session creation
    for (int i = 0; i < MAX_BOARD_SESSIONS; i++) {
        BoardContext* board = &gBoardDiscovery.boards[i];
        
        if (board->state == BOARD_STATE_DISCOVERED) {
            NXPLOG_APP_I("Board %d discovered (MAC: 0x%04X, Role: %s)", 
                         i, *(uint16_t*)board->macAddr,
                         board->currentRole == BOARD_ROLE_CONTROLLER ? "CONTROLLER" : "CONTROLEE");
            NXPLOG_APP_I("Attempting to create UWB session...");
            
            // Create session for discovered board
            if (SessionManager_CreateSession(DEVICE_TYPE_NXP_BOARD,
                                           board->macAddr,
                                           BOARD_MAC_ADDR_LEN) >= 0) {
                board->state = BOARD_STATE_CONNECTING;
            } else {
                NXPLOG_APP_E("Failed to create session for board %d", i);
                board->state = BOARD_STATE_ERROR;
            }
        }
    }
}

uint8_t BoardAdapter_GetDiscoveredBoardCount(void) {
    return gBoardDiscovery.discoveredBoards;
}

void BoardAdapter_PrintDiscoveryStatus(void) {
    NXPLOG_APP_I("=== Board Discovery Status ===");
    NXPLOG_APP_I("Discovery Active: %s", gBoardDiscovery.isActive ? "YES" : "NO");
    NXPLOG_APP_I("Discovered Boards: %d/%d", gBoardDiscovery.discoveredBoards, MAX_BOARD_SESSIONS);
    
    for (int i = 0; i < MAX_BOARD_SESSIONS; i++) {
        BoardContext* board = &gBoardDiscovery.boards[i];
        const char* stateStr = BoardAdapter_GetStateString(board->state);
        
        if (board->state != BOARD_STATE_IDLE) {
            NXPLOG_APP_I("  Board %d: %s (MAC: 0x%04X, Ch: %d, Role: %s)",
                         i, stateStr, *(uint16_t*)board->macAddr, board->channel,
                         board->currentRole == BOARD_ROLE_CONTROLLER ? "CTRL" : "CTLEE");
        }
    }
}

void BoardAdapter_OnDiscoveryMatch(const uint8_t* peerMacAddr, bool isController) {
    NXPLOG_APP_I("Discovery match callback - MAC: 0x%04X, Role: %s",
                 *(uint16_t*)peerMacAddr, isController ? "CONTROLLER" : "CONTROLEE");
    
    // Find an available board slot
    for (int i = 0; i < MAX_BOARD_SESSIONS; i++) {
        if (gBoardDiscovery.boards[i].state == BOARD_STATE_DISCOVERING) {
            NXPLOG_APP_I("Found available slot %d for discovered board", i);
            
            // Copy MAC address
            memcpy(gBoardDiscovery.boards[i].macAddr, peerMacAddr, BOARD_MAC_ADDR_LEN);
            gBoardDiscovery.boards[i].state = BOARD_STATE_DISCOVERED;
            gBoardDiscovery.boards[i].currentRole = isController ? BOARD_ROLE_CONTROLLER : BOARD_ROLE_CONTROLEE;
            gBoardDiscovery.boards[i].isDiscovered = true;
            gBoardDiscovery.discoveredBoards++;
            
            NXPLOG_APP_I("Board registered in slot %d (Total discovered: %d/%d)", 
                         i, gBoardDiscovery.discoveredBoards, MAX_BOARD_SESSIONS);
            break;
        }
    }
}

// Callback for discovery manager (internal)
static void onBoardDiscovered(uint16_t macAddr, BoardRole role) {
    uint8_t macBytes[2];
    macBytes[0] = (macAddr >> 8) & 0xFF;
    macBytes[1] = macAddr & 0xFF;
    
    BoardAdapter_OnDiscoveryMatch(macBytes, role == BOARD_ROLE_CONTROLLER);
}

static void handleBoardSessionEvent(uint32_t sessionId, int eventType, void* pData) {
    // Find the board session
    for (int i = 0; i < MAX_BOARD_SESSIONS; i++) {
        if (gBoardDiscovery.boards[i].sessionId == sessionId) {
            switch (eventType) {
                case UWB_EVENT_SESSION_STARTED:
                    gBoardDiscovery.boards[i].state = BOARD_STATE_ACTIVE;
                    NXPLOG_APP_I("Board session %d started", i);
                    break;
                case UWB_EVENT_SESSION_STOPPED:
                    gBoardDiscovery.boards[i].state = BOARD_STATE_DISCOVERED;
                    NXPLOG_APP_I("Board session %d stopped", i);
                    break;
                case UWB_EVENT_RANGING_DATA:
                                         // Handle ranging data
                     if (pData) {
                         phRangingData_t* rangingData = (phRangingData_t*)pData;
                         if (rangingData->no_of_measurements > 0) {
                             gBoardDiscovery.boards[i].lastDistance = rangingData->ranging_meas.range_meas_twr[0].distance / 100.0f;
                             phOsalUwb_GetTickCount((unsigned long*)&gBoardDiscovery.boards[i].lastRangingTime);
                             NXPLOG_APP_I("Board %d ranging: %.2f meters", i, gBoardDiscovery.boards[i].lastDistance);
                         }
                     }
                    break;
            }
            break;
        }
    }
}

static void initializeBoardSession(int sessionIndex, uint16_t macAddr, BoardRole role) {
    BoardContext* board = &gBoardDiscovery.boards[sessionIndex];
    
    *(uint16_t*)board->macAddr = macAddr;
    board->currentRole = role;
    
    // Set basic configuration for DS-TWR
    // Note: Actual UWB configuration would be done through UWB API calls
}

// Utility functions
const char* BoardAdapter_GetStateString(BoardState state) {
    switch (state) {
        case BOARD_STATE_IDLE: return "IDLE";
        case BOARD_STATE_DISCOVERING: return "DISCOVERING";
        case BOARD_STATE_DISCOVERED: return "DISCOVERED";
        case BOARD_STATE_CONNECTING: return "CONNECTING";
        case BOARD_STATE_ACTIVE: return "ACTIVE";
        case BOARD_STATE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

const char* BoardAdapter_GetRoleString(BoardRole role) {
    switch (role) {
        case BOARD_ROLE_CONTROLLER: return "CONTROLLER";
        case BOARD_ROLE_CONTROLEE: return "CONTROLEE";
        default: return "UNKNOWN";
    }
}

// Stub implementations for other functions declared in header
bool BoardAdapter_IsDiscoveryActive(void) { return gBoardDiscovery.isActive; }
void BoardAdapter_SwitchDiscoveryRole(void) { /* Implementation needed */ }
BoardRole BoardAdapter_GetCurrentRole(void) { return gBoardDiscovery.currentDiscoveryRole; }
void BoardAdapter_HandleDiscoveryTimeout(void) { /* Implementation needed */ }
bool BoardAdapter_CreateSession(int sessionIndex, const uint8_t* macAddr) { return false; /* Implementation needed */ }
bool BoardAdapter_StartRanging(int sessionIndex) { return false; /* Implementation needed */ }
bool BoardAdapter_StopRanging(int sessionIndex) { return false; /* Implementation needed */ }
bool BoardAdapter_TerminateSession(int sessionIndex) { return false; /* Implementation needed */ }
bool BoardAdapter_ConfigureRanging(int sessionIndex, BoardRole role) { return false; /* Implementation needed */ }
bool BoardAdapter_SetRangingParameters(int sessionIndex, uint8_t channel, uint16_t interval) { return false; /* Implementation needed */ }
void BoardAdapter_HandleDiscoverySignal(const uint8_t* peerMacAddr) { /* Implementation needed */ }
void BoardAdapter_HandleDiscoveryResponse(const uint8_t* peerMacAddr) { /* Implementation needed */ }
void BoardAdapter_HandleRangingData(int sessionIndex, phRangingData_t* rangingData) { /* Implementation needed */ }
BoardState BoardAdapter_GetState(int sessionIndex) { return gBoardDiscovery.boards[sessionIndex].state; }
void BoardAdapter_SetState(int sessionIndex, BoardState newState) { gBoardDiscovery.boards[sessionIndex].state = newState; }
bool BoardAdapter_IsSessionActive(int sessionIndex) { return gBoardDiscovery.boards[sessionIndex].state == BOARD_STATE_ACTIVE; }
BoardContext* BoardAdapter_GetBoardContext(int sessionIndex) { return &gBoardDiscovery.boards[sessionIndex]; }
bool BoardAdapter_AddDiscoveredBoard(const uint8_t* macAddr) { return false; /* Implementation needed */ }
int BoardAdapter_FindBoardByMac(const uint8_t* macAddr) { return -1; /* Implementation needed */ }
void BoardAdapter_PrintBoardStatus(int sessionIndex) { /* Implementation needed */ }
void BoardAdapter_Task(void* args) { /* Implementation needed */ }
void BoardAdapter_RegisterCallback(BoardEventCallback callback) { gBoardEventCallback = callback; } 