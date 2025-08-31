#include "tlv_manager.h"
#include "phOsalUwb.h"
#include "AppInternal.h"
#include "UwbApi_Utility.h"
#include "gatt_db_handles.h"
#include "private_profile_interface.h"
#include <string.h>

// TLV task configuration
#define TLV_MNG_STACK_SIZE 400
#define TLV_MNG_PRIO       (tskIDLE_PRIORITY + 4)
#define TLV_MAX_DATA_SIZE  50
#define TLV_MAX_WAIT_MS    10000

// Global state
static intptr_t gTlvQueue = 0;
static void* gTlvMutex = NULL;
static void* gTlvSemaphore = NULL;
static UWBOSAL_TASK_HANDLE gTlvTaskHandle = NULL;
static UWB_Hif_t gCurrentInterface = UWB_HIF_BLE;

// Forward declarations
static void tlvManagerTask(void* args);
static void handleTlvMessage(uint8_t deviceId, uint8_t* data);

bool tlvMngInit(void) {
    phOsalUwb_ThreadCreationParams_t threadParams;

    // Create message queue
    gTlvQueue = phOsalUwb_msgget(1);
    if (!gTlvQueue) {
        NXPLOG_APP_E("Failed to create TLV message queue");
        return false;
    }

    // Create mutex
    if (phOsalUwb_CreateMutex(&gTlvMutex) != UWBSTATUS_SUCCESS) {
        NXPLOG_APP_E("Failed to create TLV mutex");
        return false;
    }

    // Create task
    threadParams.stackdepth = TLV_MNG_STACK_SIZE;
    PHOSALUWB_SET_TASKNAME(threadParams, "TlvMng");
    threadParams.pContext = NULL;
    threadParams.priority = TLV_MNG_PRIO;

    if (phOsalUwb_Thread_Create((void**)&gTlvTaskHandle, &tlvManagerTask, &threadParams) != 0) {
        NXPLOG_APP_E("Failed to create TLV manager task");
        return false;
    }

    NXPLOG_APP_I("TLV manager initialized successfully");
    return true;
}

bool tlvBuilderInit(void) {
    // Create semaphore for send/receive synchronization
    if (phOsalUwb_CreateSemaphore(&gTlvSemaphore, 0) != UWBSTATUS_SUCCESS) {
        NXPLOG_APP_E("Failed to create TLV semaphore");
        return false;
    }

    NXPLOG_APP_I("TLV builder initialized successfully");
    return true;
}

void tlvManagerTask(void* args) {
    uint8_t data[TLV_MAX_DATA_SIZE] = {0};
    phLibUwb_Message_t evt = {0};

    while (1) {
        // Wait for TLV messages
        if (phOsalUwb_msgrcv(gTlvQueue, &evt, MAX_DELAY) == UWBSTATUS_FAILED) {
            NXPLOG_APP_W("Failed to receive TLV message");
            continue;
        }

        // Copy message data
        phOsalUwb_MemCopy(data, (uint8_t*)evt.pMsgData, evt.Size);
        NXPLOG_APP_D("Received TLV message from device %d", evt.eMsgType);

        // Process message
        handleTlvMessage((uint8_t)evt.eMsgType, data);
    }
}

void handleTlvMessage(uint8_t deviceId, uint8_t* data) {
    if (!data) {
        NXPLOG_APP_W("TLV message data is NULL");
        return;
    }

    phOsalUwb_LockMutex(gTlvMutex);

    // Process message based on type
    switch (*data) {
    case kMsg_Initialize_iOS:
        NXPLOG_APP_I("Received iOS initialization message");
        handleDeviceInit();
        break;

    case kMsg_ConfigureAndStart:
        NXPLOG_APP_I("Received configure and start message");
        // Configure UWB session with shareable data
        break;

    case kMsg_Stop:
        NXPLOG_APP_I("Received stop message");
        handleStopSession(deviceId);
        break;

    default:
        NXPLOG_APP_W("Unknown TLV message type: 0x%02X", *data);
        break;
    }

    phOsalUwb_UnlockMutex(gTlvMutex);
}

bool tlvSendRaw(uint8_t deviceId, uint8_t* buf, uint16_t size) {
    bool success = true;

    // Send over BLE
    if (gCurrentInterface == UWB_HIF_BLE) {
        // Send data using BLE service - use platform BLE function
        bleResult_t bleResult = Qpp_SendData((deviceId_t)deviceId, service_qpps, (uint16_t)size, buf);
        tlvSendDoneCb();

        if (bleResult != gBleSuccess_c) {
            NXPLOG_APP_E("Failed to send TLV data over BLE: 0x%X", bleResult);
            success = false;
            goto end;
        }

        NXPLOG_APP_D("TLV data sent successfully over BLE (%d bytes)", size);
    } else {
        NXPLOG_APP_E("Unsupported interface: %d", gCurrentInterface);
        success = false;
        goto end;
    }

    // Wait for send completion
    if (phOsalUwb_ConsumeSemaphore_WithTimeout(gTlvSemaphore, TLV_MAX_WAIT_MS) != UWBSTATUS_SUCCESS) {
        NXPLOG_APP_E("TLV send timeout");
        success = false;
    }

end:
    return success;
}

void tlvSendDoneCb(void) {
    phOsalUwb_ProduceSemaphore(gTlvSemaphore);
}

void tlvRecv(uint8_t deviceId, UWB_Hif_t interface, uint8_t* tlv, uint8_t tlvSize) {
    gCurrentInterface = interface;

    // Queue message for processing
    phLibUwb_Message_t msg = {
        .eMsgType = deviceId,
        .Size = tlvSize,
        .pMsgData = tlv
    };

    phOsalUwb_msgsnd(gTlvQueue, &msg, NO_DELAY);
}

void tlvManagerDeinit(void) {
    // Stop task
    if (gTlvTaskHandle) {
        phOsalUwb_Thread_Delete(gTlvTaskHandle);
        gTlvTaskHandle = NULL;
    }

    // Clean up resources
    if (gTlvMutex) {
        phOsalUwb_DeleteMutex(&gTlvMutex);
        gTlvMutex = NULL;
    }

    if (gTlvSemaphore) {
        phOsalUwb_DeleteSemaphore(&gTlvSemaphore);
        gTlvSemaphore = NULL;
    }

    NXPLOG_APP_I("TLV manager deinitialized");
}
