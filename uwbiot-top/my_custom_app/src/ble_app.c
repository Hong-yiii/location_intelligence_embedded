/* BLE Application Module
 * Based on demo_nearby_interaction implementation
 */

#include "ble_app.h"
#include "phOsalUwb.h"
#include "UwbApi.h"
#include "AppInternal.h"
#include "UwbApi_Utility.h"

/* BLE Host Stack */
#include "gatt_server_interface.h"
#include "gatt_client_interface.h"
#include "gap_interface.h"
#include "gatt_db_handles.h"

/* Profile / Services */
#include "private_profile_interface.h"

/* Connection Manager */
#include "ble_conn_manager.h"

#include "board.h"
#include "ApplMain.h"
#include "demo_ble_server.h"

#include "UWBT_PowerMode.h"
#include "UWBT_Config.h"
#include "uwb_types.h"

#include "phTmlUwb_transport.h"
#include "phOsalUwb.h"

/* Include app config to ensure BLE variables are available */
#include "app_config.h"

/* Missing type definitions */
#ifndef gAppMaxConnections_c
#define gAppMaxConnections_c 1
#endif

#ifndef QPPS_VALUE_NTF_OFF
#define QPPS_VALUE_NTF_OFF 0
#endif

/* Advertising type enumeration */
typedef enum
{
    fastAdvState_c,
    slowAdvState_c,
    defaultAdvState_c
} advType_t;

/* Advertising state structure */
typedef struct advState_tag
{
    bool_t advOn;
    advType_t advType;
} advState_t;

/* Peer information structure */
typedef struct appPeerInfo_tag
{
    uint8_t deviceId;
    uint8_t ntf_cfg;
} appPeerInfo_t;

/* BLE configuration variables - fallback definitions in case app_config.c is not compiled */
#ifndef gAdvParams
gapAdvertisingParameters_t gAdvParams = {
    /* minInterval */ 0x0020,
    /* maxInterval */ 0x0040,
    /* advertisingType */ 0x00,
    /* ownAddressType */ 0x00,
    /* directedAddressType */ 0x00,
    /* directedAddress */ {0, 0, 0, 0, 0, 0},
    /* channelMap */ 0x07,
    /* filterPolicy */ 0x00
};
#endif

#ifndef gAppAdvertisingData
static const uint8_t adData0[1] = {0x06};
static const gapAdStructure_t advScanStruct[3] = {
    {.length = NumberOfElements(adData0) + 1, .adType = 0x01, .aData = (uint8_t *)adData0},
    {.length = 17, .adType = 0x07, .aData = (uint8_t *)uuid_service_qpps},
    {.adType = 0x08, .length = 3 + 1, .aData = (uint8_t *)"Tag"}
};

gapAdvertisingData_t gAppAdvertisingData = {NumberOfElements(advScanStruct), (void *)advScanStruct};
#endif

#ifndef gAppScanRspData
static uint8_t localNameFallback[10] = {"NXP_UWB"};
static const gapAdStructure_t advScanRspStruct[1] = {
    {.length = 9 + 1, .adType = 0x09, .aData = localNameFallback}
};

gapScanResponseData_t gAppScanRspData = {NumberOfElements(advScanRspStruct), (void *)advScanRspStruct};
#endif

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

// BLE connection state (similar to demo)
static advState_t mAdvState;
static qppsConfig_t qppServiceConfig = {service_qpps};
static uint16_t cpHandles[1] = {value_qpps_rx};
static uint16_t cpReadHandles[1] = {value_nearby_data};
static appPeerInfo_t mPeerInformation[gAppMaxConnections_c];
static uint32_t mAdvTimeout;  // Missing variable declaration

// Forward declarations
static void BleApp_AdvertiseTask(void *args);
static void BleApp_AdvertisingCallback(gapAdvertisingEvent_t *pAdvertisingEvent);
static void BleApp_ConnectionCallback(deviceId_t peerDeviceId, gapConnectionEvent_t *pConnectionEvent);
static void BleApp_GattServerCallback(deviceId_t deviceId, gattServerEvent_t *pServerEvent);
static void BleApp_Config(void);
static void BleApp_Advertise(void);
static void BleApp_ReceivedDataHandler(deviceId_t deviceId, uint8_t *aValue, uint16_t valueLength);
static uint8_t BleApp_GetConnectedPeerCount(void);

void BleApp_Init(void) {
    if (gBleState.isInitialized) {
        return;
    }

    NXPLOG_APP_I("Initializing BLE stack...");

    // Initialize BLE stack with NXP BLE functions (like demo)
    if (Ble_Initialize(BleApp_GenericCallback) != gBleSuccess_c) {
        NXPLOG_APP_E("Failed to initialize BLE stack");
        return;
    }

    // Configure BLE stack (like nearby_interaction demo)
    BleApp_Config();

    // Initialize QPP service (required for BLE communication)
    if (Qpp_Start(&qppServiceConfig) != gBleSuccess_c) {
        NXPLOG_APP_E("Failed to start QPP service");
        return;
    }

    gBleState.isInitialized = true;
    NXPLOG_APP_I("BLE stack initialized successfully");
}

void BleApp_Start(void) {
    if (!gBleState.isInitialized) {
        NXPLOG_APP_E("BLE not initialized");
        return;
    }

    NXPLOG_APP_I("Starting BLE advertising...");

    // Set advertising type to default (like nearby_interaction demo)
    mAdvState.advType = defaultAdvState_c;

    // Start advertising (like demo pattern)
    BleApp_Advertise();

    gBleState.isAdvertising = true;
    phOsalUwb_GetTickCount((unsigned long*)&gBleState.lastAdvertiseTime);
    NXPLOG_APP_I("BLE advertising started - ready for iPhone connection");
}

void BleApp_Stop(void) {
    if (!gBleState.isInitialized || !gBleState.isAdvertising) {
        return;
    }

    // Stop advertising using GAP interface
    if (Gap_StopAdvertising() != gBleSuccess_c) {
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

    // BLE events are handled through the callback system
    // No need to poll for events - they're delivered via callbacks
    // This function can be used for any periodic BLE-related tasks
}

bool BleApp_IsInitialized(void) {
    return gBleState.isInitialized;
}

bool BleApp_IsAdvertising(void) {
    return mAdvState.advOn;
}

static void BleApp_AdvertiseTask(void *args) {
    while (1) {
        if (gBleState.isInitialized && !mAdvState.advOn) {
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

void BleApp_GenericCallback(gapGenericEvent_t *pGenericEvent) {
    if (!gBleState.isInitialized) {
        return;
    }

    if (!pGenericEvent) {
        return;
    }

    switch (pGenericEvent->eventType) {
    case gInitializationComplete_c: {
        BleApp_Config();
    } break;

    case gAdvertisingParametersSetupComplete_c: {
        (void)Gap_SetAdvertisingData(&gAppAdvertisingData, &gAppScanRspData);
    } break;

    case gAdvertisingDataSetupComplete_c: {
        (void)Gap_SetTxPowerLevel(gAdvertisingPowerLeveldBm_c, gTxPowerAdvChannel_c);
    } break;

    case gTxPowerLevelSetComplete_c: {
        (void)App_StartAdvertising(BleApp_AdvertisingCallback, BleApp_ConnectionCallback);
    } break;

    case gAdvertisingSetupFailed_c: {
        NXPLOG_APP_E("BLE advertising setup failed");
    } break;

    default:
        break;
    }
}

// tlvBuilderInit is implemented in tlv_manager.c

// tlvMngInit is implemented in tlv_manager.c

// TLV task and message handling are implemented in tlv_manager.c

// tlvSendRaw and tlvRecv are implemented in tlv_manager.c

// handleDeviceInit is implemented in tlv_handlers.c

// Calibration data setup moved to tlv_handlers.c

// handleStopSession is implemented in tlv_handlers.c

// handleShutDown is implemented in tlv_handlers.c

/************************************************************************************
*************************************************************************************
* Private functions (similar to demo)
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
* \brief        Configures BLE Stack after initialization. Usually used for
*               configuring advertising, scanning, white list, services, et al.
*
********************************************************************************** */
static void BleApp_Config()
{
    /* Common GAP configuration */
    BleConnManager_GapCommonConfig();

    /* Register for callbacks*/
    GattServer_RegisterHandlesForWriteNotifications(NumberOfElements(cpHandles), cpHandles);
    GattServer_RegisterHandlesForReadNotifications(NumberOfElements(cpReadHandles), cpReadHandles);
    App_RegisterGattServerCallback(BleApp_GattServerCallback);

    /* TODO: Load required BLE configurations from flash*/

    mAdvState.advOn = FALSE;
    for (uint8_t i = 0; i < gAppMaxConnections_c; i++) {
        mPeerInformation[i].deviceId = gInvalidDeviceId_c;
    }

    Qpp_Start(&qppServiceConfig);
    mAdvState.advType = defaultAdvState_c;
}

/*! *********************************************************************************
* \brief        Configures GAP Advertise parameters. Advertise will start after
*               the parameters are set.
*
********************************************************************************** */
static void BleApp_Advertise(void)
{
    switch (mAdvState.advType) {
    case defaultAdvState_c: {
        gAdvParams.minInterval  = UWBT_CfgReadBleInterval();
        gAdvParams.maxInterval  = UWBT_CfgReadBleInterval();
        gAdvParams.filterPolicy = gProcessAll_c;
        mAdvTimeout             = gReducedPowerAdvTime_c;
    } break;

    default:
        // Use default parameters
        gAdvParams.minInterval  = gReducedPowerMinAdvInterval_c;
        gAdvParams.maxInterval  = gReducedPowerMaxAdvInterval_c;
        gAdvParams.filterPolicy = gProcessAll_c;
        mAdvTimeout             = gReducedPowerAdvTime_c;
        break;
    }

    /* Set advertising parameters*/
    if (Gap_SetAdvertisingParameters(&gAdvParams) != gBleSuccess_c) {
        NXPLOG_APP_E("Failed to set advertising parameters");
        return;
    }

    /* Set advertising data */
    if (Gap_SetAdvertisingData(&gAppAdvertisingData, &gAppScanRspData) != gBleSuccess_c) {
        NXPLOG_APP_E("Failed to set advertising data");
        return;
    }

    /* Set TX power level */
    if (Gap_SetTxPowerLevel(gAdvertisingPowerLeveldBm_c, gTxPowerAdvChannel_c) != gBleSuccess_c) {
        NXPLOG_APP_E("Failed to set TX power level");
        return;
    }

    /* Start advertising */
    if (App_StartAdvertising(BleApp_AdvertisingCallback, BleApp_ConnectionCallback) != gBleSuccess_c) {
        NXPLOG_APP_E("Failed to start advertising");
        return;
    }
}

/*! *********************************************************************************
* \brief        Handles BLE Advertising callback from host stack.
*
* \param[in]    pAdvertisingEvent    Pointer to gapAdvertisingEvent_t.
********************************************************************************** */
static void BleApp_AdvertisingCallback(gapAdvertisingEvent_t *pAdvertisingEvent)
{
    switch (pAdvertisingEvent->eventType) {
    case gAdvertisingStateChanged_c: {
        mAdvState.advOn = !mAdvState.advOn;
        PRINTF("BLE Start adv\r\n");
        if (mAdvState.advOn) {
            // Advertising started successfully
            NXPLOG_APP_I("BLE advertising started");
        }
    } break;

    case gAdvertisingCommandFailed_c: {
        NXPLOG_APP_E("BLE advertising command failed");
    } break;

    default:
        break;
    }
}

/*! *********************************************************************************
* \brief        Handles BLE Connection callback from host stack.
*
* \param[in]    peerDeviceId        Peer device ID.
* \param[in]    pConnectionEvent    Pointer to gapConnectionEvent_t.
********************************************************************************** */
static void BleApp_ConnectionCallback(deviceId_t peerDeviceId, gapConnectionEvent_t *pConnectionEvent)
{
    /* Connection Manager to handle Host Stack interactions */
    BleConnManager_GapPeripheralEvent(peerDeviceId, pConnectionEvent);

    switch (pConnectionEvent->eventType) {
    case gConnEvtConnected_c: {
        mPeerInformation[peerDeviceId].deviceId = peerDeviceId;

        /* Subscribe client*/
        (void)Qpp_Subscribe(peerDeviceId);

        LOG_I("BLE Connected to peer #%d\r\n", peerDeviceId + 1);

        /* Restart Advertising while max connection is not reached */
        if (BleApp_GetConnectedPeerCount() < gAppMaxConnections_c) {
            BleApp_Start();
        }

    } break;

    case gConnEvtDisconnected_c: {
        /* qpps Unsubscribe client */
        Qpp_Unsubscribe();
        mPeerInformation[peerDeviceId].ntf_cfg  = QPPS_VALUE_NTF_OFF;
        mPeerInformation[peerDeviceId].deviceId = gInvalidDeviceId_c;
        PRINTF("BLE Disconnected\r\n");

        /* in any case restart advertising since a connection has been released */
        BleApp_Start();
    } break;

    default:
        break;
    }
}

/*! *********************************************************************************
* \brief        Handles GATT server callback from host stack.
*
* \param[in]    deviceId        Peer device ID.
* \param[in]    pServerEvent    Pointer to gattServerEvent_t.
********************************************************************************** */
static void BleApp_GattServerCallback(deviceId_t deviceId, gattServerEvent_t *pServerEvent)
{
    uint16_t handle;
    uint8_t status;

    switch (pServerEvent->eventType) {
    case gEvtAttributeWritten_c: {
        handle = pServerEvent->eventData.attributeWrittenEvent.handle;
        status = gAttErrCodeNoError_c;
        GattServer_SendAttributeWrittenStatus(deviceId, handle, status);
        if (handle == value_qpps_rx) {
            BleApp_ReceivedDataHandler(deviceId,
                pServerEvent->eventData.attributeWrittenEvent.aValue,
                pServerEvent->eventData.attributeWrittenEvent.cValueLength);
        }
    } break;

    case gEvtAttributeWrittenWithoutResponse_c: {
        handle = pServerEvent->eventData.attributeWrittenEvent.handle;

        if (handle == value_qpps_rx) {
            BleApp_ReceivedDataHandler(deviceId,
                pServerEvent->eventData.attributeWrittenEvent.aValue,
                pServerEvent->eventData.attributeWrittenEvent.cValueLength);
        }
    } break;

    case gEvtCharacteristicCccdWritten_c: {
        handle = pServerEvent->eventData.charCccdWrittenEvent.handle;
        if (handle == cccd_qpps_tx) {
            mPeerInformation[deviceId].ntf_cfg = pServerEvent->eventData.charCccdWrittenEvent.newCccd;
        }
    } break;

    case gEvtAttributeRead_c: {
        handle = pServerEvent->eventData.attributeReadEvent.handle;
        LOG_I("Attribute read by peer device \r\n");
        GattServer_SendAttributeReadStatus(deviceId, handle, gAttErrCodeNoError_c);
    } break;

    default:
        break;
    }
}

static void BleApp_ReceivedDataHandler(deviceId_t deviceId, uint8_t *aValue, uint16_t valueLength)
{
    // Forward to TLV handler (similar to demo)
    tlvRecv((uint8_t)deviceId, UWB_HIF_BLE, aValue, valueLength);
}

static uint8_t BleApp_GetConnectedPeerCount(void)
{
    uint8_t nb_peer = 0;
    for (uint8_t i = 0; i < gAppMaxConnections_c; i++) {
        if (mPeerInformation[i].deviceId != gInvalidDeviceId_c)
            nb_peer++;
    }
    return nb_peer;
}
