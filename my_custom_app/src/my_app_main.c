/* Copyright 2024 NXP
 * Multi-Session UWB Application
 * Supports simultaneous ranging with 4 UWB boards + 1 iPhone
 */

#include "phUwb_BuildConfig.h"
#include "UwbApi.h"
#include "AppInternal.h"
#include "phOsalUwb.h"

// Application headers
#include "session_manager.h"
#include "resource_manager.h"
#include "iphone_adapter.h"
#include "board_adapter.h"
#include "discovery_manager.h"

// Task configuration
#define MULTI_SESSION_TASK_SIZE 1024
#define MULTI_SESSION_TASK_NAME "MultiSessionUWB"
#define MULTI_SESSION_TASK_PRIO 4

// Application state
typedef struct {
    bool isInitialized;
    bool isRunning;
    uint32_t startTime;
    uint32_t rangingCount;
} AppState;

static AppState gAppState = {0};

// Forward declarations
static bool initializeApplication(void);
static void shutdownApplication(void);
static void handleApplicationError(const char* error);
static void printApplicationStatus(void);

// Main application callback
void MultiSessionAppCallback(eNotificationType opType, void *pData) {
    switch (opType) {
    case UWBD_RANGING_DATA: {
        phRangingData_t *pRangingData = (phRangingData_t *)pData;
        
        // Route ranging data to appropriate session handler
        // For now, we'll handle it generically and let session manager route it
        // TODO: Determine which session this data belongs to
        NXPLOG_APP_I("Ranging data received - Distance: %d cm, Status: 0x%02X", 
                     pRangingData->ranging_meas.range_meas_twr[0].distance,
                     pRangingData->ranging_meas.range_meas_twr[0].status);
        
        gAppState.rangingCount++;
        break;
    }
    case UWBD_SESSION_DATA: {
        phUwbSessionInfo_t *pSessionInfo = (phUwbSessionInfo_t *)pData;
        NXPLOG_APP_I("Session notification - Handle: 0x%08X, State: %d", 
                     pSessionInfo->sessionHandle, pSessionInfo->state);
        
        // Forward to session manager
        // TODO: Map session handle to session index
        break;
    }
    case UWBD_GENERIC_ERROR_NTF: {
        phGenericError_t *pError = (phGenericError_t *)pData;
        NXPLOG_APP_E("UWB Error - Session: 0x%08X, Error: 0x%02X", 
                     pError->sessionHandle, pError->errorType);
        
        // Forward to session manager for error handling
        break;
    }
    case UWBD_DEVICE_RESET: {
        NXPLOG_APP_W("Device reset detected - reinitializing...");
        // Handle device reset recovery
        handleApplicationError("Device reset");
        break;
    }
    default:
        // Forward other notifications to common handler
        AppCallback(opType, pData);
        break;
    }
}

// Main application task
OSAL_TASK_RETURN_TYPE MultiSessionTask(void *args) {
    tUWBAPI_STATUS status = UWBAPI_STATUS_FAILED;
    
    PRINT_APP_NAME("Multi-Session UWB Application");
    NXPLOG_APP_I("Starting multi-session UWB application...");
    
    // Initialize application
    if (!initializeApplication()) {
        NXPLOG_APP_E("Failed to initialize application");
        goto exit;
    }
    
    NXPLOG_APP_I("Application initialized successfully");
    NXPLOG_APP_I("Starting discovery and ranging operations...");
    
    gAppState.isRunning = true;
    gAppState.startTime = phOsalUwb_GetTimestamp();
    
    // Start iPhone discovery (parallel)
    if (!iPhoneAdapter_StartAdvertising()) {
        NXPLOG_APP_W("Failed to start iPhone advertising - continuing without iPhone support");
    }
    
    // Start board discovery (sequential)
    if (!BoardAdapter_StartDiscovery(MAX_BOARD_SESSIONS)) {
        NXPLOG_APP_E("Failed to start board discovery");
        goto exit;
    }
    
    // Main application loop
    while (gAppState.isRunning) {
        // Process session manager events
        SessionManager_ProcessEvents();
        
        // Process iPhone adapter events
        iPhoneAdapter_ProcessBLEEvents();
        
        // Process board adapter events
        BoardAdapter_ProcessDiscoveryEvents();
        
        // Print status periodically (every 10 seconds)
        static uint32_t lastStatusTime = 0;
        uint32_t currentTime = phOsalUwb_GetTimestamp();
        if (currentTime - lastStatusTime > 10000) {
            printApplicationStatus();
            lastStatusTime = currentTime;
        }
        
        // Small delay to prevent busy waiting
        phOsalUwb_DelayMs(100);
    }
    
    status = UWBAPI_STATUS_OK;
    
exit:
    NXPLOG_APP_I("Shutting down application...");
    shutdownApplication();
    
    if (status == UWBAPI_STATUS_OK) {
        NXPLOG_APP_I("Application completed successfully");
    } else {
        NXPLOG_APP_E("Application completed with errors");
    }
    
    UWBIOT_EXAMPLE_END(status);
}

static bool initializeApplication(void) {
    phUwbappContext_t appCtx = {0};
    tUWBAPI_STATUS status;
    
    // Initialize UWB stack
    appCtx.fwImageCtx.fwMode = MAINLINE_FW;
    appCtx.pCallback = MultiSessionAppCallback;
    appCtx.pCdcCallback = NULL;
    appCtx.pMcttCallback = NULL;
    appCtx.seHandle = NULL;
    
    status = UwbApi_Init_New(&appCtx);
    if (status != UWBAPI_STATUS_OK) {
        NXPLOG_APP_E("UwbApi_Init_New failed: 0x%02X", status);
        return false;
    }
    
    // Initialize resource manager
    if (!ResourceManager_Init()) {
        NXPLOG_APP_E("ResourceManager_Init failed");
        return false;
    }
    
    // Initialize session manager
    if (!SessionManager_Init()) {
        NXPLOG_APP_E("SessionManager_Init failed");
        return false;
    }
    
    // Initialize iPhone adapter
    if (!iPhoneAdapter_Init()) {
        NXPLOG_APP_E("iPhoneAdapter_Init failed");
        return false;
    }
    
    // Initialize board adapter
    if (!BoardAdapter_Init()) {
        NXPLOG_APP_E("BoardAdapter_Init failed");
        return false;
    }
    
    // Initialize discovery manager
    DiscoveryManager_Init();
    
    // Register callbacks
    DiscoveryManager_RegisterCallback(BoardAdapter_OnDiscoveryMatch);
    
    gAppState.isInitialized = true;
    return true;
}

static void shutdownApplication(void) {
    if (!gAppState.isInitialized) {
        return;
    }
    
    gAppState.isRunning = false;
    
    // Stop all discovery and ranging
    BoardAdapter_StopDiscovery();
    iPhoneAdapter_StopAdvertising();
    DiscoveryManager_Stop();
    
    // Shutdown adapters
    BoardAdapter_Deinit();
    iPhoneAdapter_Deinit();
    
    // Shutdown managers
    SessionManager_Deinit();
    ResourceManager_Deinit();
    
    // Shutdown UWB stack
    if (UwbApi_ShutDown() != UWBAPI_STATUS_OK) {
        NXPLOG_APP_E("UwbApi_ShutDown failed");
    }
    
    gAppState.isInitialized = false;
}

static void handleApplicationError(const char* error) {
    NXPLOG_APP_E("Application error: %s", error);
    
    // Stop current operations
    gAppState.isRunning = false;
    
    // TODO: Implement recovery logic if needed
    // For now, just shutdown gracefully
}

static void printApplicationStatus(void) {
    uint32_t uptime = phOsalUwb_GetTimestamp() - gAppState.startTime;
    
    NXPLOG_APP_I("=== Multi-Session UWB Status ===");
    NXPLOG_APP_I("Uptime: %lu ms", uptime);
    NXPLOG_APP_I("Total ranging measurements: %lu", gAppState.rangingCount);
    NXPLOG_APP_I("Active sessions: %d", SessionManager_GetActiveSessionCount());
    NXPLOG_APP_I("Discovered boards: %d", BoardAdapter_GetDiscoveredBoardCount());
    
    // Print resource status
    ResourceManager_PrintResourceStatus();
    
    // Print discovery status
    BoardAdapter_PrintDiscoveryStatus();
    
    NXPLOG_APP_I("===============================");
}

// Task creation interface
UWBOSAL_TASK_HANDLE uwb_demo_start(void) {
    phOsalUwb_ThreadCreationParams_t threadparams;
    UWBOSAL_TASK_HANDLE taskHandle;
    int pthread_create_status = 0;
    
    threadparams.stackdepth = MULTI_SESSION_TASK_SIZE;
    PHOSALUWB_SET_TASKNAME(threadparams, MULTI_SESSION_TASK_NAME);
    threadparams.pContext = NULL;
    threadparams.priority = MULTI_SESSION_TASK_PRIO;
    
    pthread_create_status = phOsalUwb_Thread_Create(
        (void **)&taskHandle, &MultiSessionTask, &threadparams);
    
    if (0 != pthread_create_status) {
        NXPLOG_APP_E("Failed to create task %s", threadparams.taskname);
    }
    
    return taskHandle;
} 