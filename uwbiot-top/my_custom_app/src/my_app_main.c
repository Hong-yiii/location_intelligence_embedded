/* Copyright 2024 NXP
 * Multi-Session UWB Application
 * Supports simultaneous ranging with 4 UWB boards + 1 iPhone
 */

#include "phUwb_BuildConfig.h"
#include "UwbApi.h"
#include "AppInternal.h"
#include "phOsalUwb.h"

#ifndef UWBIOT_APP_BUILD__MY_CUSTOM_APP
#include "UWBIOT_APP_BUILD.h"
#endif

// Application headers
#include "session_manager.h"
#include "resource_manager.h"
#include "iphone_adapter.h"
#include "board_adapter.h"
#include "discovery_manager.h"
#include "ble_app.h"

// BLE includes (similar to demo)
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// Task configuration - reduced size to avoid demo skip condition
#define MULTI_SESSION_TASK_SIZE 4096  // Increased for firmware download + multi-session operations  /* Reduced to 1024 to match demo pattern */
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
static bool initializeUWBStack(void);
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
        NXPLOG_APP_E("UWB Error - Status: 0x%02X", pError->status);
        
        // Forward to session manager for error handling
        break;
    }
    case UWBD_DEVICE_RESET: {
        NXPLOG_APP_W("Device reset detected - stopping application...");
        // Handle device reset recovery
        gAppState.isRunning = false;
        break;
    }
    default:
        // Forward other notifications to common handler
        AppCallback(opType, pData);
        break;
    }
}

// Main application task - parallel board ranging and iPhone discovery
OSAL_TASK_RETURN_TYPE MultiSessionTask(void *args) {
    PRINT_APP_NAME("Multi-Session UWB Application");
    tUWBAPI_STATUS status = UWBAPI_STATUS_FAILED;

    // Initialize UWB stack first for board ranging
    NXPLOG_APP_I("Initializing UWB stack...");
    status = UwbApi_Init(MultiSessionAppCallback);
    if (status != UWBAPI_STATUS_OK) {
        NXPLOG_APP_E("UwbApi_Init failed: 0x%02X", status);
        goto exit_demo;
    }
    gAppState.isInitialized = true;

    // Initialize core components
    if (!initializeApplication()) {
        NXPLOG_APP_E("Failed to initialize application components");
        status = UWBAPI_STATUS_FAILED;
        goto exit_demo;
    }

    // Start board discovery immediately
    if (!BoardAdapter_StartDiscovery(MAX_BOARD_SESSIONS)) {
        NXPLOG_APP_E("Failed to start board discovery");
        status = UWBAPI_STATUS_FAILED;
        goto exit_demo;
    }
    
    // Initialize iPhone-related components in parallel
    if (!tlvBuilderInit()) {
        NXPLOG_APP_W("Failed to initialize TLV builder - iPhone features disabled");
    } else if (!tlvMngInit()) {
        NXPLOG_APP_W("Failed to initialize TLV manager - iPhone features disabled");
    } else {
        // Initialize and start BLE stack for continuous iPhone discovery
        BleApp_Init();
        BleApp_Start();
        NXPLOG_APP_I("BLE advertising started - ready for iPhone connection");
    }
    
    NXPLOG_APP_I("Multi-session operation started:");
    NXPLOG_APP_I("- UWB active and ranging with boards");
    NXPLOG_APP_I("- BLE advertising for iPhone connection");
    
    // Main application loop
    gAppState.isRunning = true;
    phOsalUwb_GetTickCount((unsigned long*)&gAppState.startTime);

    while (gAppState.isRunning) {
        // Process board sessions and discovery
        SessionManager_ProcessEvents();
        BoardAdapter_ProcessDiscoveryEvents();
        
        // Process iPhone connection attempts (if BLE available)
        if (BleApp_IsInitialized()) {
            BleApp_ProcessEvents();
            
            // Restart advertising if needed
            if (!BleApp_IsAdvertising()) {
                BleApp_StartAdvertising();
            }
        }
        
        // Optimize resource allocation
        ResourceManager_OptimizeResourceAllocation();
        
        // Small delay to prevent busy waiting
        phOsalUwb_Delay(50); // Reduced for more responsive iPhone discovery
        
        // Print status occasionally
        static uint32_t lastStatusTime = 0;
        uint32_t currentTime;
        phOsalUwb_GetTickCount((unsigned long*)&currentTime);
        if (currentTime - lastStatusTime > 10000) {
            printApplicationStatus();
            lastStatusTime = currentTime;
        }
    }
    
    status = UWBAPI_STATUS_OK;

exit_demo:
    UWBIOT_EXAMPLE_END(status);
}



// Lightweight initialization - only for essential components
static bool initializeApplication(void) {
    NXPLOG_APP_I("Initializing application components...");
    
    // Initialize only lightweight components
    // UWB initialization will be triggered by BLE connection
    
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
    
    // Initialize iPhone adapter (lightweight)
    if (!iPhoneAdapter_Init()) {
        NXPLOG_APP_E("iPhoneAdapter_Init failed");
        return false;
    }
    
    // Initialize board adapter (lightweight)
    if (!BoardAdapter_Init()) {
        NXPLOG_APP_E("BoardAdapter_Init failed");
        return false;
    }
    
    // Initialize discovery manager
    DiscoveryManager_Init();
    DiscoveryManager_RegisterCallback(BoardAdapter_OnDiscoveryMatch);
    
    NXPLOG_APP_I("Application components initialized successfully");
    return true;
}



static void printApplicationStatus(void) {
    uint32_t currentTime;
    phOsalUwb_GetTickCount((unsigned long*)&currentTime);
    uint32_t uptime = currentTime - gAppState.startTime;
    
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
    NXPLOG_APP_I("=== Starting Multi-Session UWB Application ===");
    NXPLOG_APP_I("Build: %s %s", __DATE__, __TIME__);
    NXPLOG_APP_I("Task Size: %d bytes, Priority: %d", MULTI_SESSION_TASK_SIZE, MULTI_SESSION_TASK_PRIO);
    
    phOsalUwb_ThreadCreationParams_t threadparams;
    UWBOSAL_TASK_HANDLE taskHandle = NULL;
    int pthread_create_status = 0;
    
    // Initialize thread parameters
    memset(&threadparams, 0, sizeof(phOsalUwb_ThreadCreationParams_t));
    threadparams.stackdepth = MULTI_SESSION_TASK_SIZE;
    PHOSALUWB_SET_TASKNAME(threadparams, MULTI_SESSION_TASK_NAME);
    threadparams.pContext = NULL;
    threadparams.priority = MULTI_SESSION_TASK_PRIO;
    
    NXPLOG_APP_I("Creating task '%s'...", MULTI_SESSION_TASK_NAME);
    
    pthread_create_status = phOsalUwb_Thread_Create(
        (void **)&taskHandle, &MultiSessionTask, &threadparams);
    
    if (0 != pthread_create_status) {
        NXPLOG_APP_E("Failed to create task '%s' (error: %d)", 
                     MULTI_SESSION_TASK_NAME, pthread_create_status);
        return NULL;
    }
    
    NXPLOG_APP_I("Task created successfully (handle: 0x%08X)", (uint32_t)taskHandle);
    NXPLOG_APP_I("=======================================");
    
    return taskHandle;
} 