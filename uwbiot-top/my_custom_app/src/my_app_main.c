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

// Hardware includes (for additional initialization)
#include "board.h"
#include "TimersManager.h"

// Application headers
#include "session_manager.h"
#include "resource_manager.h"
#include "iphone_adapter.h"
#include "board_adapter.h"
#include "discovery_manager.h"
#include "ble_app.h"
#include "gatt_database.h"

// BLE includes (similar to demo)
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// Main task include (for main_task function)
#include "ApplMain.h"

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
static tUWBAPI_STATUS configureUwbParams(void);
static void printApplicationStatus(void);

// UWB configuration parameters
static const uint8_t UWB_CHANNELS[] = {5, 9};  // iPhone preferred and alternate channels
static const uint8_t TX_POWER[11] = {
    0x02,  // Number of parameters
    0x01,  // TX ID 1
    0x00, 0x00, 0x00, 0x00,  // TX_POWER settings for ID 1
    0x02,  // TX ID 2
    0x00, 0x00, 0x00, 0x00   // TX_POWER settings for ID 2
};
static const uint8_t CLK_ACCURACY[7] = {
    0x03,  // Number of parameters
    0x00, 0x00,  // CAP1
    0x00, 0x00,  // CAP2
    0x00, 0x00   // GM CURRENT CONTROL
};

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

// Main application task - proper initialization sequence
OSAL_TASK_RETURN_TYPE MultiSessionTask(void *args) {
    PRINT_APP_NAME("Multi-Session UWB Application");
    tUWBAPI_STATUS status = UWBAPI_STATUS_FAILED;

    // 1. Initialize platform and BLE stack (following demo pattern exactly)
    NXPLOG_APP_I("About to call main_task(0) for BLE initialization...");

    // Ensure gSmpKeys is accessible before BLE init
    extern gapSmpKeys_t gSmpKeys;
    NXPLOG_APP_I("gSmpKeys.cLtkSize = %d", gSmpKeys.cLtkSize);

    main_task(0);  // This initializes BLE stack like demo_nearby_interaction
    NXPLOG_APP_I("main_task(0) returned - BLE stack should be initialized");

    // Small delay to let BLE initialization complete
    vTaskDelay(100);

    // 2. Initialize TLV components FIRST (required for BLE communication)
    NXPLOG_APP_I("Initializing TLV components...");
    if (!tlvBuilderInit()) {
        NXPLOG_APP_E("Failed to initialize TLV builder");
        goto exit_demo;
    }

    if (!tlvMngInit()) {
        NXPLOG_APP_E("Failed to initialize TLV manager");
        goto exit_demo;
    }
    NXPLOG_APP_I("TLV components initialized successfully");

    // 3. Initialize GATT database FIRST (required before BLE stack)
    GattDb_Init();
    NXPLOG_APP_I("GATT database initialized");

    // 4. Initialize BLE application components (hardware setup only, like demo)
    BleApp_Init();
    NXPLOG_APP_I("BLE application initialized");

    // 5. Start BLE advertising (BLE stack already initialized by main_task)
    NXPLOG_APP_I("About to start BLE advertising...");
    BleApp_Start();
    NXPLOG_APP_I("BleApp_Start() returned - BLE advertising should be active");
    NXPLOG_APP_I("BLE advertising started - ready for iPhone connection");

    // 6. Initialize UWB stack AFTER BLE
    NXPLOG_APP_I("Initializing UWB stack...");
    phUwbappContext_t appCtx = {0};
    appCtx.pCallback = MultiSessionAppCallback;
    appCtx.pCdcCallback = NULL;
    appCtx.pMcttCallback = NULL;
    appCtx.seHandle = NULL;

    status = UwbApi_Init_New(&appCtx);
    if (status != UWBAPI_STATUS_OK) {
        NXPLOG_APP_E("UwbApi_Init_New failed: 0x%02X", status);
        goto exit_demo;
    }

    // 6. Configure UWB parameters
    status = configureUwbParams();
    if (status != UWBAPI_STATUS_OK) {
        NXPLOG_APP_E("UWB configuration failed: 0x%02X", status);
        goto exit_demo;
    }
    gAppState.isInitialized = true;
    NXPLOG_APP_I("UWB stack initialized successfully");

    // 7. Initialize application components
    if (!initializeApplication()) {
        NXPLOG_APP_E("Failed to initialize application components");
        status = UWBAPI_STATUS_FAILED;
        goto exit_demo;
    }

    // 8. Start board discovery
    if (!BoardAdapter_StartDiscovery(MAX_BOARD_SESSIONS)) {
        NXPLOG_APP_W("Failed to start board discovery - continuing with iPhone-only mode");
    }
    
    NXPLOG_APP_I("System initialization complete:");
    NXPLOG_APP_I("- UWB stack ready for ranging");
    NXPLOG_APP_I("- TLV components ready for iPhone communication");
    NXPLOG_APP_I("- BLE advertising active for iPhone discovery");
    
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
                BleApp_Start();
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



static tUWBAPI_STATUS configureUwbParams(void) {
    tUWBAPI_STATUS status = UWBAPI_STATUS_OK;
    uint16_t bitmask;
    phCalibPayload_t calib;

    // Configure TX power for each channel
    bitmask = (1<<1);  // TX_POWER_ID
    uint8_t numChannels = sizeof(UWB_CHANNELS) / sizeof(UWB_CHANNELS[0]);
    for (int i = 0; i < numChannels; i++) {
        uint8_t channel = UWB_CHANNELS[i];
        uint8_t txPower[sizeof(TX_POWER)];
        memcpy(txPower, TX_POWER, sizeof(TX_POWER));

        // Read calibration data
        status = UwbApi_ReadOtpCalibDataCmd(channel, bitmask, &calib);
        if (status != UWBAPI_STATUS_OK) {
            NXPLOG_APP_E("Failed to read calibration data for channel %d", channel);
            return status;
        }

        // Apply calibration
        txPower[4] = calib.TX_POWER_ID[0] + (2.1-0.6+0.5)*4;
        txPower[2] = calib.TX_POWER_ID[1];

        // Set TX power
        status = UwbApi_SetCalibration(channel, TX_POWER_PER_ANTENNA, txPower, sizeof(txPower));
        if (status != UWBAPI_STATUS_OK) {
            NXPLOG_APP_E("Failed to set TX power for channel %d", channel);
            return status;
        }
    }

    // Configure clock accuracy
    bitmask = (1<<2);  // XTAL_CAP
    uint8_t clkAccuracy[sizeof(CLK_ACCURACY)];
    memcpy(clkAccuracy, CLK_ACCURACY, sizeof(CLK_ACCURACY));

    // Read calibration data (channel 9 is dummy for XTAL_CAP)
    status = UwbApi_ReadOtpCalibDataCmd(9, bitmask, &calib);
    if (status != UWBAPI_STATUS_OK) {
        NXPLOG_APP_E("Failed to read XTAL calibration data");
        return status;
    }

    // Apply calibration
    clkAccuracy[1] = calib.XTAL_CAP_VALUES[0];
    clkAccuracy[3] = calib.XTAL_CAP_VALUES[1];
    clkAccuracy[5] = calib.XTAL_CAP_VALUES[2];

    // Set clock accuracy
    status = UwbApi_SetCalibration(9, RF_CLK_ACCURACY_CALIB, clkAccuracy, sizeof(clkAccuracy));
    if (status != UWBAPI_STATUS_OK) {
        NXPLOG_APP_E("Failed to set clock accuracy");
        return status;
    }

    NXPLOG_APP_I("UWB parameters configured successfully");
    return status;
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