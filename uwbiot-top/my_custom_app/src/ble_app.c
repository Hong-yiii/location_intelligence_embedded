/* BLE Application Module
 * Based on demo_nearby_interaction implementation
 */

#include "ble_app.h"
#include "phOsalUwb.h"
#include "UwbApi.h"
#include "AppInternal.h"
#include "UwbApi_Utility.h"

// BLE-specific global variables

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
    BLE_Event_t event = {0};  // Initialize to avoid uninitialized warning
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

// tlvBuilderInit is implemented in tlv_manager.c

// tlvMngInit is implemented in tlv_manager.c

// TLV task and message handling are implemented in tlv_manager.c

// tlvSendRaw and tlvRecv are implemented in tlv_manager.c

// handleDeviceInit is implemented in tlv_handlers.c

// Calibration data setup moved to tlv_handlers.c

// handleStopSession is implemented in tlv_handlers.c

// handleShutDown is implemented in tlv_handlers.c

// BLE Platform Function Stubs (to be implemented by BLE stack)
int BLE_Init(void) {
    // TODO: Implement BLE initialization
    NXPLOG_APP_W("BLE_Init not implemented - returning success for now");
    return BLE_SUCCESS;
}

int BLE_StartAdvertising(uint8_t *data, uint16_t length, uint32_t interval) {
    // TODO: Implement BLE advertising start
    NXPLOG_APP_W("BLE_StartAdvertising not implemented - returning success for now");
    return BLE_SUCCESS;
}

int BLE_StopAdvertising(void) {
    // TODO: Implement BLE advertising stop
    NXPLOG_APP_W("BLE_StopAdvertising not implemented - returning success for now");
    return BLE_SUCCESS;
}

int BLE_GetNextEvent(BLE_Event_t *event) {
    // TODO: Implement BLE event retrieval
    NXPLOG_APP_W("BLE_GetNextEvent not implemented - returning failure");
    return BLE_ERROR_NOT_INITIALIZED;
}

int BLE_QueueEvent(BLE_Event_t *event) {
    // TODO: Implement BLE event queuing
    NXPLOG_APP_W("BLE_QueueEvent not implemented - returning success for now");
    return BLE_SUCCESS;
}

int BLE_SendData(uint8_t deviceId, uint8_t *data, uint16_t length) {
    // TODO: Implement BLE data sending
    NXPLOG_APP_W("BLE_SendData not implemented - returning success for now");
    return BLE_SUCCESS;
}
