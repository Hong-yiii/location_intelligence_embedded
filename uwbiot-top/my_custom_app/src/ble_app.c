/* BLE Application Module
 * Based on demo_nearby_interaction implementation
 */

#include "ble_app.h"
#include "phOsalUwb.h"
#include "UwbApi.h"
#include "AppInternal.h"
#include "UwbApi_Utility.h"

// Forward declarations
static void setupCalibrationData(void);

// Global variables for TLV management
static void *mTlvMutex = NULL;
static intptr_t tlvMngQueue = 0;
static bool gDeviceInitialized = false;

// Device state management
typedef enum {
    notCreated = 0,
    notStarted,
    Started
} UwbHandlerState;

#define MAX_CONNECTIONS 4
static UwbHandlerState mSessionState[MAX_CONNECTIONS] = {notCreated};
static uint32_t mSessionHandle[MAX_CONNECTIONS];
static uint8_t mDevice[MAX_CONNECTIONS];
static uint16_t mMacAddr[MAX_CONNECTIONS];

// Forward declarations
static void tlvMngTask(void *args);
static void handleTLV(uint8_t deviceId, uint8_t *data);

// BLE state management
static struct {
    bool isInitialized;
    bool isAdvertising;
    uint32_t lastAdvertiseTime;
    uint32_t advertiseInterval;
    UWBOSAL_TASK_HANDLE advertiseTaskHandle;
} gBleState = {
    .isInitialized = false,
    .isAdvertising = false,
    .lastAdvertiseTime = 0,
    .advertiseInterval = 100,  // 100ms advertising interval
    .advertiseTaskHandle = NULL
};

// Forward declaration
static void BleApp_AdvertiseTask(void *args);

void BleApp_Init(void) {
    if (gBleState.isInitialized) {
        return;
    }

    // Initialize BLE stack
    if (BLE_Init() != BLE_SUCCESS) {
        NXPLOG_APP_E("Failed to initialize BLE stack");
        return;
    }

    // Create advertising task
    phOsalUwb_ThreadCreationParams_t threadParams;
    threadParams.stackdepth = 400;
    PHOSALUWB_SET_TASKNAME(threadParams, "BleAdv");
    threadParams.pContext = NULL;
    threadParams.priority = tskIDLE_PRIORITY + 3;

    if (phOsalUwb_Thread_Create((void **)&gBleState.advertiseTaskHandle, 
                               &BleApp_AdvertiseTask, &threadParams) != 0) {
        NXPLOG_APP_E("Failed to create BLE advertising task");
        return;
    }

    gBleState.isInitialized = true;
    NXPLOG_APP_I("BLE stack initialized");
}

void BleApp_Start(void) {
    if (!gBleState.isInitialized) {
        NXPLOG_APP_E("BLE not initialized");
        return;
    }

    // Configure advertising data
    uint8_t advData[] = {
        0x02, 0x01, 0x06,  // General discoverable
        0x03, 0x03, 0xFF, 0x00,  // Complete list of 16-bit UUIDs
        0x0A, 0x09, 'N', 'X', 'P', '_', 'U', 'W', 'B', 0x00, 0x00  // Complete local name
    };

    // Start advertising
    if (BLE_StartAdvertising(advData, sizeof(advData), gBleState.advertiseInterval) != BLE_SUCCESS) {
        NXPLOG_APP_E("Failed to start BLE advertising");
        return;
    }

    gBleState.isAdvertising = true;
    phOsalUwb_GetTickCount((unsigned long*)&gBleState.lastAdvertiseTime);
    NXPLOG_APP_I("BLE advertising started");
}

void BleApp_Stop(void) {
    if (!gBleState.isInitialized || !gBleState.isAdvertising) {
        return;
    }

    if (BLE_StopAdvertising() != BLE_SUCCESS) {
        NXPLOG_APP_E("Failed to stop BLE advertising");
        return;
    }

    gBleState.isAdvertising = false;
    NXPLOG_APP_I("BLE advertising stopped");
}

void BleApp_ProcessEvents(void) {
    if (!gBleState.isInitialized) {
        return;
    }

    // Process BLE events
    BLE_Event_t event;
    while (BLE_GetNextEvent(&event) == BLE_SUCCESS) {
        switch (event.type) {
        case BLE_EVENT_CONNECTED:
            NXPLOG_APP_I("BLE device connected");
            gBleState.isAdvertising = false;
            break;

        case BLE_EVENT_DISCONNECTED:
            NXPLOG_APP_I("BLE device disconnected - restarting advertising");
            BleApp_Start();  // Restart advertising
            break;

        case BLE_EVENT_ADVERTISE_TIMEOUT:
            NXPLOG_APP_D("BLE advertising timeout - restarting");
            BleApp_Start();  // Restart advertising
            break;

        default:
            break;
        }
    }
}

bool BleApp_IsInitialized(void) {
    return gBleState.isInitialized;
}

bool BleApp_IsAdvertising(void) {
    return gBleState.isAdvertising;
}

static void BleApp_AdvertiseTask(void *args) {
    while (1) {
        if (gBleState.isInitialized && !gBleState.isAdvertising) {
            // Check if it's time to restart advertising
            uint32_t currentTime;
            phOsalUwb_GetTickCount((unsigned long*)&currentTime);
            
            if ((currentTime - gBleState.lastAdvertiseTime) > 1000) {  // 1 second timeout
                NXPLOG_APP_D("Auto-restarting BLE advertising");
                BleApp_Start();
            }
        }
        phOsalUwb_Delay(100);  // Check every 100ms
    }
}

void BleApp_GenericCallback(void *pGenericEvent) {
    if (!gBleState.isInitialized) {
        return;
    }

    BLE_Event_t *event = (BLE_Event_t *)pGenericEvent;
    if (!event) {
        return;
    }

    // Process event in the BLE task context
    BLE_QueueEvent(event);
}

bool tlvBuilderInit(void) {
    // Initialize TLV builder
    // This would normally initialize TLV data structures
    return true;
}

bool tlvMngInit(void) {
    phOsalUwb_ThreadCreationParams_t threadParams;
    
    // Create message queue
    tlvMngQueue = phOsalUwb_msgget(1);
    if (!tlvMngQueue) {
        NXPLOG_APP_E("Could not create queue tlvMngQueue");
        return false;
    }

    // Create mutex
    if (phOsalUwb_CreateMutex(&mTlvMutex) != UWBSTATUS_SUCCESS) {
        NXPLOG_APP_E("Could not create TLV mutex");
        return false;
    }

    // Create TLV management task
    threadParams.stackdepth = 400;
    PHOSALUWB_SET_TASKNAME(threadParams, "TlvMng");
    threadParams.pContext = NULL;
    threadParams.priority = tskIDLE_PRIORITY + 4;
    
    UWBOSAL_TASK_HANDLE mTlvMngHnd;
    if (phOsalUwb_Thread_Create((void **)&mTlvMngHnd, &tlvMngTask, &threadParams) != 0) {
        NXPLOG_APP_E("Could not create tlvMng task");
        return false;
    }
    
    return true;
}

static void tlvMngTask(void *args) {
    while (1) {
        uint8_t data[50] = {0};
        phLibUwb_Message_t evt = {0};
        
        if (phOsalUwb_msgrcv(tlvMngQueue, &evt, MAX_DELAY) == UWBSTATUS_FAILED) {
            continue;
        }
        
        phOsalUwb_MemCopy(data, (uint8_t *)evt.pMsgData, evt.Size);
        handleTLV((uint8_t)evt.eMsgType, data);
    }
}

static void handleTLV(uint8_t deviceId, uint8_t *data) {
    if (data == NULL) {
        NXPLOG_APP_W("handleTLV data is NULL");
        return;
    }

    // Lock mutex
    (void)phOsalUwb_LockMutex(mTlvMutex);
    
    // Handle different TLV message types
    switch (*data) {
    case 0x01: // Example: Initialize message
        NXPLOG_APP_I("Received initialize message from device %d", deviceId);
        // Handle initialization
        break;
    case 0x02: // Example: Configure and start
        NXPLOG_APP_I("Received configure and start message from device %d", deviceId);
        // Handle configuration
        break;
    case 0x03: // Example: Stop message
        NXPLOG_APP_I("Received stop message from device %d", deviceId);
        handleStopSession(deviceId);
        break;
    default:
        NXPLOG_APP_W("Unknown TLV command: 0x%02X", *data);
        break;
    }
    
    // Unlock mutex
    (void)phOsalUwb_UnlockMutex(mTlvMutex);
}

bool tlvSendRaw(uint8_t deviceId, uint8_t *data, uint16_t length) {
    // Send raw TLV data over BLE
    // This would normally send data via BLE characteristic
    NXPLOG_APP_D("Sending TLV data to device %d, length: %d", deviceId, length);
    return true;
}

void tlvRecv(uint8_t deviceId, uint8_t hif, uint8_t *data, uint16_t length) {
    // Receive TLV data from BLE
    phLibUwb_Message_t msg;
    msg.eMsgType = deviceId;
    msg.pMsgData = data;
    msg.Size = length;
    
    if (phOsalUwb_msgsnd(tlvMngQueue, &msg, 0) != UWBSTATUS_SUCCESS) {
        NXPLOG_APP_E("Failed to send TLV message to queue");
    }
}

bool handleDeviceInit(void) {
    if (!gDeviceInitialized) {
        NXPLOG_APP_I("BLE connection established - UWB already initialized for multi-session mode");
        
        // UWB stack is already initialized by main task for board discovery
        // We just need to mark it as ready for iPhone sessions
        gDeviceInitialized = true;
        NXPLOG_APP_I("UWB device ready for iPhone sessions");
        
        // Set calibration data (similar to demo)
        // This includes TX power and clock accuracy calibration
        setupCalibrationData();
    }
    
    return true;
}

// Setup calibration data similar to the demo
static void setupCalibrationData(void) {
    // TX_POWER_ID calibration
    uint16_t bitmask = (1<<1); // TX_POWER_ID
    phCalibPayload_t calib;
    uint8_t channels[] = {5, 9};
    uint8_t channel;
    uint8_t TX_POWER[11] = {0x02 /* Number of parameters */,
                           0x01 /* for TX ID 1 */, 0x00, 0x00, 0x00, 0x00,
                           0x02 /* for TX ID 2 */, 0x00, 0x00, 0x00, 0x00};
    
    uint8_t CLK_ACCURACY[7] = { 0x03 /* Number of parameters */,
                                0x00, 0x00 /* CAP1 */,
                                0x00, 0x00 /* CAP2 */,
                                0x00, 0x00 /* GM CURRENT CONTROL */};

    for(int i = 0; i < sizeof(channels); i++) {
        channel = channels[i];
        UwbApi_ReadOtpCalibDataCmd(channel, bitmask, &calib);
        TX_POWER[4] = calib.TX_POWER_ID[0] + (2.1-0.6+0.5)*4;
        TX_POWER[2] = calib.TX_POWER_ID[1];
        UwbApi_SetCalibration(channel, TX_POWER_PER_ANTENNA, TX_POWER, sizeof(TX_POWER));
    }

    // XTAL_CAP calibration
    bitmask = (1<<2); // XTAL_CAP
    UwbApi_ReadOtpCalibDataCmd(9, bitmask, &calib);    // channel 9 is dummy for XTAL_CAP
    CLK_ACCURACY[1] = calib.XTAL_CAP_VALUES[0];
    CLK_ACCURACY[3] = calib.XTAL_CAP_VALUES[1];
    CLK_ACCURACY[5] = calib.XTAL_CAP_VALUES[2];
    UwbApi_SetCalibration(9, RF_CLK_ACCURACY_CALIB, CLK_ACCURACY, sizeof(CLK_ACCURACY));
    
    NXPLOG_APP_I("Calibration data configured");
}

bool handleStopSession(uint8_t deviceId) {
    bool status = true;
    tUWBAPI_STATUS operation = UWBAPI_STATUS_OK;

    if (deviceId >= MAX_CONNECTIONS) {
        NXPLOG_APP_E("Invalid device ID: %d", deviceId);
        return false;
    }

    while (mSessionState[deviceId] != notCreated) {
        switch (mSessionState[deviceId]) {
        case notStarted:
            NXPLOG_APP_D("Deleting session: 0x%08X", mSessionHandle[deviceId]);
            operation = UwbApi_SessionDeinit(mSessionHandle[deviceId]);
            if (operation == UWBAPI_STATUS_OK || operation == UWBAPI_STATUS_SESSION_NOT_EXIST) {
                mSessionState[deviceId] = notCreated;
                status = true;
            } else {
                status = false;
            }
            break;
            
        case Started:
            NXPLOG_APP_D("Stopping session: 0x%08X", mSessionHandle[deviceId]);
            operation = UwbApi_StopRangingSession(mSessionHandle[deviceId]);
            if (operation == UWBAPI_STATUS_OK || operation == UWBAPI_STATUS_SESSION_NOT_EXIST) {
                mSessionState[deviceId] = notStarted;
                status = true;
            } else {
                status = false;
            }
            break;
            
        default:
            NXPLOG_APP_E("Stop session wrong state: %d", mSessionState[deviceId]);
            status = false;
            break;
        }
    }
    
    return status;
}

bool handleShutDown(void) {
    bool status = true;
    tUWBAPI_STATUS operation = UWBAPI_STATUS_OK;

    operation = UwbApi_ShutDown();
    if (operation == UWBAPI_STATUS_OK) {
        NXPLOG_APP_I("UWB device shutdown complete");
        gDeviceInitialized = false;
        
        // Reset session states
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            mSessionState[i] = notCreated;
            mSessionHandle[i] = 0xA5A5A5A5;
            mDevice[i] = 0;
            mMacAddr[i] = 0xA5A5;
        }
    } else {
        status = false;
        NXPLOG_APP_E("Error shutting down UWB: 0x%02X", operation);
    }
    
    return status;
}
