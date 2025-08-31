# Multi-Session UWB Initialization Guide

## 📋 Overview

This guide details the initialization process for a multi-session UWB implementation supporting:
1. Board-to-Board DS-TWR (Double-Sided Two-Way Ranging)
2. iPhone Nearby Interaction
3. Simultaneous operation of multiple sessions

## ⚠️ Critical Bug Fixes

### Array Size Calculation Bug (FIXED)
**Issue**: `sizeof(UWB_CHANNELS)` returned bytes instead of element count
**Impact**: Buffer overflow and undefined behavior during UWB parameter configuration
**Fix**: Used `sizeof(UWB_CHANNELS)/sizeof(UWB_CHANNELS[0])`

### BLE Implementation Bug (FIXED)
**Issue**: `tlvSendRaw()` had placeholder implementation that didn't send data
**Impact**: iPhone Nearby Interaction would fail completely
**Fix**: Implemented proper BLE send using `Qpp_SendData()` with error handling

### GATT Database Bootstrap (FIXED)
**Issue**: Missing GATT database causing compilation errors
**Impact**: BLE functionality completely broken, no services/characteristics defined
**Fix**: Bootstrapped complete GATT database from demo_nearby_interaction using **direct symbol definitions in gatt_database.c** to resolve X-macro compilation order dependencies

#### **Why X-Macros Are Problematic**
X-macros are a powerful but fragile compile-time code generation technique:
- **Compilation Order Sensitivity**: Symbols must be defined before X-macro expansion occurs
- **Single Definition Rule**: Each symbol can only be defined once across the entire build
- **Hidden Dependencies**: X-macros create implicit timing dependencies that aren't visible in the code
- **Build System Interference**: MCUExpresso's auto-generated build files can disrupt the required sequence

**The Fix**: Define GATT symbols directly in `gatt_database.c` BEFORE the X-macro includes to ensure proper compilation order.

## 🔧 Core Components Initialization

### 1. Task Configuration
```c
// Main task configuration
#define MULTI_SESSION_TASK_SIZE 4096  // Increased for multi-session operations
#define MULTI_SESSION_TASK_NAME "MultiSessionUWB"
#define MULTI_SESSION_TASK_PRIO 4

// BLE advertising task (continuous iPhone discovery)
#define BLE_ADV_TASK_SIZE 400
#define BLE_ADV_TASK_NAME "BleAdv"
#define BLE_ADV_TASK_PRIO 3
#define BLE_ADV_INTERVAL 100  // 100ms advertising interval
#define BLE_ADV_TIMEOUT 30000 // 30s timeout before retry

// Task parameters
typedef struct {
    phOsalUwb_ThreadCreationParams_t threadParams;
    UWBOSAL_TASK_HANDLE taskHandle;
    bool isRunning;
} TaskContext;
```

### 2. Session Configuration
```c
// Session slots
#define MAX_SESSIONS 5
#define MAX_BOARD_SESSIONS 4
#define IPHONE_SESSION_SLOT 0  // Fixed iPhone slot

// Session priorities
typedef enum {
    SESSION_PRIORITY_HIGH = 0,  // iPhone session
    SESSION_PRIORITY_NORMAL,    // Board sessions
} SessionPriority;

// Channel configuration
#define IPHONE_PREFERRED_CHANNEL 5  // Primary iPhone channel
#define IPHONE_ALTERNATE_CHANNEL 9  // Fallback iPhone channel
```

### 3. UWB Stack Initialization
```c
// Initialize UWB stack with callback
status = UwbApi_Init(MultiSessionAppCallback);
if (status != UWBAPI_STATUS_OK) {
    NXPLOG_APP_E("UwbApi_Init failed: 0x%02X", status);
    return false;
}

// Configure UWB parameters (FIXED: proper array size calculation)
status = configureUwbParams();
if (status != UWBAPI_STATUS_OK) {
    NXPLOG_APP_E("UWB configuration failed: 0x%02X", status);
    return false;
}

// Initialize TLV builder for iPhone communication
if (!tlvBuilderInit() || !tlvMngInit()) {
    NXPLOG_APP_E("Failed to initialize TLV components");
    return false;
}

// Initialize GATT database (FIXED: bootstrapped from demo)
GattDb_Init();  // Initialize BLE GATT database with services and characteristics

// Initialize BLE stack for continuous iPhone discovery
BleApp_Init();
BleApp_Start();  // Start advertising immediately (FIXED: proper BLE send implementation)

// Start board discovery
BoardAdapter_StartDiscovery(MAX_BOARD_SESSIONS);
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
