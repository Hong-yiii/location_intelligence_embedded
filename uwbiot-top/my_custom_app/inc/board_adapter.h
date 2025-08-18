#ifndef BOARD_ADAPTER_H
#define BOARD_ADAPTER_H

#include "UwbApi.h"
#include "session_manager.h"
#include "discovery_manager.h"
#include <stdbool.h>

// Board-specific constants
#define BOARD_SESSION_ID_BASE 0x22334400
#define BOARD_MAC_ADDR_LEN 2
#define BOARD_DEFAULT_RANGING_INTERVAL 200  // 200ms
#define MAX_BOARD_SESSIONS 4

// Board session states
typedef enum {
    BOARD_STATE_IDLE = 0,
    BOARD_STATE_DISCOVERING,
    BOARD_STATE_DISCOVERED,
    BOARD_STATE_CONNECTING,
    BOARD_STATE_ACTIVE,
    BOARD_STATE_ERROR
} BoardState;

// Board role for discovery
typedef enum {
    BOARD_ROLE_CONTROLLER = 0,
    BOARD_ROLE_CONTROLEE
} BoardRole;

// Board session context
typedef struct {
    int sessionIndex;
    uint32_t sessionId;
    uint32_t sessionHandle;
    BoardState state;
    uint8_t macAddr[BOARD_MAC_ADDR_LEN];
    BoardRole currentRole;
    uint8_t channel;
    bool isDiscovered;
    uint32_t discoveryStartTime;
    uint32_t lastRangingTime;
    float lastDistance;
} BoardContext;

// Board discovery context
typedef struct {
    bool isActive;
    uint8_t discoveredBoards;
    uint8_t targetBoardCount;
    BoardContext boards[MAX_BOARD_SESSIONS];
    uint32_t discoveryTimeout;
    BoardRole currentDiscoveryRole;
} BoardDiscoveryContext;

// Board adapter functions
bool BoardAdapter_Init(void);
void BoardAdapter_Deinit(void);

// Discovery management
bool BoardAdapter_StartDiscovery(uint8_t targetBoardCount);
void BoardAdapter_StopDiscovery(void);
bool BoardAdapter_IsDiscoveryActive(void);
uint8_t BoardAdapter_GetDiscoveredBoardCount(void);

// Role management for discovery
void BoardAdapter_SwitchDiscoveryRole(void);
BoardRole BoardAdapter_GetCurrentRole(void);
void BoardAdapter_HandleDiscoveryTimeout(void);

// Session management
bool BoardAdapter_CreateSession(int sessionIndex, const uint8_t* macAddr);
bool BoardAdapter_StartRanging(int sessionIndex);
bool BoardAdapter_StopRanging(int sessionIndex);
bool BoardAdapter_TerminateSession(int sessionIndex);

// Ranging configuration
bool BoardAdapter_ConfigureRanging(int sessionIndex, BoardRole role);
bool BoardAdapter_SetRangingParameters(int sessionIndex, uint8_t channel, uint16_t interval);

// Event handling
void BoardAdapter_HandleDiscoverySignal(const uint8_t* peerMacAddr);
void BoardAdapter_HandleDiscoveryResponse(const uint8_t* peerMacAddr);
void BoardAdapter_HandleRangingData(int sessionIndex, phRangingData_t* rangingData);

// State management
BoardState BoardAdapter_GetState(int sessionIndex);
void BoardAdapter_SetState(int sessionIndex, BoardState newState);
bool BoardAdapter_IsSessionActive(int sessionIndex);

// Board management
BoardContext* BoardAdapter_GetBoardContext(int sessionIndex);
bool BoardAdapter_AddDiscoveredBoard(const uint8_t* macAddr);
int BoardAdapter_FindBoardByMac(const uint8_t* macAddr);

// Utility functions
const char* BoardAdapter_GetStateString(BoardState state);
const char* BoardAdapter_GetRoleString(BoardRole role);
void BoardAdapter_PrintDiscoveryStatus(void);
void BoardAdapter_PrintBoardStatus(int sessionIndex);

// Task management
void BoardAdapter_Task(void* args);
void BoardAdapter_ProcessDiscoveryEvents(void);

// Callback registration
typedef void (*BoardEventCallback)(int sessionIndex, BoardState state, void* data);
void BoardAdapter_RegisterCallback(BoardEventCallback callback);

// Discovery callback integration
void BoardAdapter_OnDiscoveryMatch(const uint8_t* peerMacAddr, bool isController);

#endif // BOARD_ADAPTER_H 