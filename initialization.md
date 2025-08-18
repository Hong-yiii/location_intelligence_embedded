# Multi-Session UWB Initialization Guide

## 📋 Overview

This guide details the initialization process for a multi-session UWB implementation supporting:
1. Board-to-Board DS-TWR (Double-Sided Two-Way Ranging)
2. iPhone Nearby Interaction
3. Simultaneous operation of multiple sessions

## 🔧 Core Components Initialization

### 1. Task Creation and Management
```c
// Task parameters for each session type
#define TASK_STACK_SIZE 400
#define TASK_PRIORITY 4

typedef struct {
    UWBOSAL_TASK_HANDLE taskHandle;
    char* taskName;
    uint32_t stackSize;
    uint8_t priority;
    void* context;
} TaskConfig;

// Initialize different tasks for different session types
TaskConfig taskConfigs[] = {
    { NULL, "BoardRanging", TASK_STACK_SIZE, TASK_PRIORITY, NULL },
    { NULL, "NearbyInt", TASK_STACK_SIZE, TASK_PRIORITY, NULL },
};
```

### 2. UWB Context Initialization
```c
phUwbappContext_t appCtx = {0};

// Set firmware mode and callbacks
appCtx.fwImageCtx.fwMode = MAINLINE_FW;
appCtx.pCallback = AppCallback;         // Main ranging callback
appCtx.pCdcCallback = NULL;            // CDC mode callback (if needed)
appCtx.pMcttCallback = NULL;           // MCTT mode callback (if needed)
appCtx.seHandle = NULL;                // Secure element handle

// Initialize UWB stack
status = UwbApi_Init_New(&appCtx);
```

### 3. Hardware Configuration
```c
// Channel configuration
uint8_t channels[] = {5, 9};  // Standard channels for ranging
uint8_t TX_POWER[11] = {
    0x02,  // Number of parameters
    0x01,  // TX ID 1
    0x00, 0x00, 0x00, 0x00,  // TX_POWER settings for ID 1
    0x02,  // TX ID 2
    0x00, 0x00, 0x00, 0x00   // TX_POWER settings for ID 2
};

// Clock accuracy settings
uint8_t CLK_ACCURACY[7] = {
    0x03,  // Number of parameters
    0x00, 0x00,  // CAP1
    0x00, 0x00,  // CAP2
    0x00, 0x00   // GM CURRENT CONTROL
};
```

## 🔄 Session Management

### 1. Session Types and IDs
```c
typedef enum {
    SESSION_BOARD_RANGING = 0x11223344,
    SESSION_NEARBY_INT = 0x11223345,
} SessionIds;

typedef struct {
    uint32_t sessionId;
    uint32_t sessionHandle;
    SessionType type;
    bool isActive;
} SessionTracker;
```

### 2. MAC Address Management
```c
// Board-to-Board MAC addresses
typedef struct {
    uint8_t shortAddr[2];    // For board-to-board
    uint8_t extAddr[8];      // For iPhone NI
} DeviceAddress;

// Example MAC configuration
DeviceAddress localAddr = {
    .shortAddr = {0x22, 0x22},
    .extAddr = {0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x1, 0x1}
};
```

## 🛠 Session-Specific Initialization

### 1. Board-to-Board Ranging Session
```c
status = UwbApi_SessionInit(SESSION_BOARD_RANGING, UWBD_RANGING_SESSION, &sessionHandle);

// Configure ranging parameters
phRangingParams_t rangingParams = {
    .deviceRole = kUWB_DeviceRole_Initiator,  // or Responder
    .multiNodeMode = kUWB_MultiNodeMode_UniCast,
    .macAddrMode = DEMO_RANGING_APP_DEVICE_MAC_ADD_MODE_SHORT,
    .deviceType = kUWB_DeviceType_Controller,  // or Controlee
    .scheduledMode = kUWB_ScheduledMode_TimeScheduled,
    .rangingRoundUsage = kUWB_RangingRoundUsage_DS_TWR
};

status = UwbApi_SetRangingParams(sessionHandle, &rangingParams);
```

### 2. iPhone Nearby Interaction Session
```c
status = UwbApi_SessionInit(SESSION_NEARBY_INT, UWBD_RANGING_SESSION, &sessionHandle);

// NI-specific configuration
const UWB_AppParams_List_t niParams[] = {
    UWB_SET_APP_PARAM_VALUE(DEVICE_TYPE, kUWB_DeviceType_Controlee),
    UWB_SET_APP_PARAM_ARRAY(DEVICE_MAC_ADDRESS, localAddr.extAddr, MAC_EXT_ADD_LEN),
};

status = UwbApi_SetAppConfigMultipleParams(sessionHandle, 
    sizeof(niParams)/sizeof(niParams[0]), niParams);
```

## 📡 Callback System

### 1. Main Callback Structure
```c
void AppCallback(eNotificationType opType, void *pData) {
    switch (opType) {
    case UWBD_RANGING_DATA:
        handleRangingData((phRangingData_t *)pData);
        break;
    case UWBD_SESSION_DATA:
        handleSessionStatus((phUwbSessionInfo_t *)pData);
        break;
    case UWBD_GENERIC_ERROR_NTF:
        handleError((phGenericError_t *)pData);
        break;
    }
}
```

### 2. Data Handling Functions
```c
void handleRangingData(phRangingData_t *pRangingData) {
    if (pRangingData->ranging_measure_type == MEASUREMENT_TYPE_TWOWAY) {
        // Handle board-to-board ranging
        processBoardRanging(pRangingData);
    } else {
        // Handle iPhone ranging
        processNearbyInteraction(pRangingData);
    }
}
```

## 🔍 Resource Management

### 1. Channel Management
```c
typedef struct {
    uint8_t channel;
    bool inUse;
    SessionType sessionType;
} ChannelAssignment;

ChannelAssignment channelPool[] = {
    {5, false, SESSION_TYPE_NONE},
    {9, false, SESSION_TYPE_NONE}
};
```

### 2. Time Slot Management
```c
typedef struct {
    uint32_t startTime;
    uint32_t duration;
    uint32_t sessionHandle;
} TimeSlot;

#define MAX_TIME_SLOTS 10
TimeSlot timeSlots[MAX_TIME_SLOTS];
```

## 🚀 Initialization Sequence

1. **System Startup**
```c
void initializeSystem(void) {
    // Initialize UWB stack
    initUwbStack();
    
    // Create management tasks
    createSessionTasks();
    
    // Initialize resource managers
    initializeChannelManager();
    initializeTimeSlotManager();
}
```

2. **Session Creation**
```c
void createSessions(void) {
    // Create board-to-board session
    createBoardRangingSession();
    
    // Create iPhone NI session if needed
    createNearbyInteractionSession();
}
```

3. **Start Operations**
```c
void startOperations(void) {
    // Start ranging sessions
    for (each session) {
        UwbApi_StartRangingSession(sessionHandle);
    }
    
    // Start monitoring
    startSessionMonitoring();
}
```

## ⚠️ Error Handling

```c
void handleError(phGenericError_t *error) {
    switch (error->errorType) {
    case SESSION_ERROR:
        recoverSession(error->sessionHandle);
        break;
    case HARDWARE_ERROR:
        restartUwbStack();
        break;
    case RESOURCE_ERROR:
        reallocateResources();
        break;
    }
}
```

## 📊 Monitoring and Logging

```c
// Logging macros for different levels
#define LOG_INFO(fmt, ...)  NXPLOG_APP_I(fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) NXPLOG_APP_E(fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) NXPLOG_APP_D(fmt, ##__VA_ARGS__)

// Session monitoring
void monitorSessions(void) {
    for (each active session) {
        checkSessionHealth();
        logSessionMetrics();
        handleTimeouts();
    }
}
```

## 🔄 Cleanup and Shutdown

```c
void cleanupSessions(void) {
    // Stop all ranging
    for (each active session) {
        UwbApi_StopRangingSession(sessionHandle);
        UwbApi_SessionDeinit(sessionHandle);
    }
    
    // Cleanup resources
    releaseChannels();
    releaseTimeSlots();
    
    // Shutdown UWB stack
    UwbApi_ShutDown();
}
```
