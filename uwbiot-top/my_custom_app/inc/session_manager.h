#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "UwbApi.h"
#include "phOsalUwb.h"
#include <stdbool.h>

// Maximum sessions supported (1 iPhone + 4 boards)
#define MAX_SESSIONS 5
#define MAX_BOARD_SESSIONS 4
#define IPHONE_SESSION_RESERVED_SLOT 0

// Session types
typedef enum {
    SESSION_TYPE_NONE = 0,
    SESSION_TYPE_IPHONE_NI,      // iPhone Nearby Interaction
    SESSION_TYPE_BOARD_DS_TWR    // Board-to-board DS-TWR
} SessionType;

// Session states
typedef enum {
    SESSION_STATE_IDLE = 0,
    SESSION_STATE_DISCOVERING,
    SESSION_STATE_INITIALIZING,
    SESSION_STATE_ACTIVE,
    SESSION_STATE_ERROR,
    SESSION_STATE_TERMINATED
} SessionState;

// Device types
typedef enum {
    DEVICE_TYPE_UNKNOWN = 0,
    DEVICE_TYPE_IPHONE,
    DEVICE_TYPE_NXP_BOARD
} DeviceType;

// Session configuration
typedef struct {
    uint32_t sessionId;
    uint32_t sessionHandle;
    SessionType type;
    SessionState state;
    DeviceType deviceType;
    uint8_t macAddr[8];          // Extended MAC for iPhone, short for boards
    uint8_t macAddrLen;          // 2 for short, 8 for extended
    uint8_t channel;             // UWB channel assigned
    uint16_t rangingInterval;    // Ranging interval in ms
    bool isActive;
    uint32_t lastRangingTime;    // Last successful ranging timestamp
    float lastDistance;          // Last measured distance
} SessionContext;

// Session manager structure
typedef struct {
    SessionContext sessions[MAX_SESSIONS];
    uint8_t activeSessions;
    uint8_t totalSessions;
    bool isInitialized;
    UWBOSAL_TASK_HANDLE managerTaskHandle;
    void* rangingMutex;
} SessionManager;

// Session manager functions
bool SessionManager_Init(void);
void SessionManager_Deinit(void);

// Session lifecycle
int SessionManager_CreateSession(DeviceType deviceType, const uint8_t* macAddr, uint8_t macAddrLen);
bool SessionManager_StartSession(int sessionIndex);
bool SessionManager_StopSession(int sessionIndex);
bool SessionManager_TerminateSession(int sessionIndex);

// Session queries
SessionContext* SessionManager_GetSession(int sessionIndex);
SessionState SessionManager_GetSessionState(int sessionIndex);
int SessionManager_GetActiveSessionCount(void);
bool SessionManager_IsSessionActive(int sessionIndex);

// Session management
void SessionManager_UpdateSessionState(int sessionIndex, SessionState newState);
void SessionManager_HandleRangingData(int sessionIndex, phRangingData_t* rangingData);
void SessionManager_HandleSessionError(int sessionIndex, uint32_t errorCode);

// Task management
void SessionManager_ProcessEvents(void);

// Callback registration
typedef void (*SessionEventCallback)(int sessionIndex, SessionState state, void* data);
void SessionManager_RegisterCallback(SessionEventCallback callback);

#endif // SESSION_MANAGER_H