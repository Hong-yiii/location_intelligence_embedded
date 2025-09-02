/* BLE Application Module
 * Based on demo_nearby_interaction implementation
 */

#include "ble_app.h"
#include "phOsalUwb.h"
#include "UwbApi.h"
#include "AppInternal.h"
#include "UwbApi_Utility.h"
#include "ApplMain.h"  /* For App_StartAdvertising */

/* BLE Host Stack includes are now in ble_app.h */

/* SMP Keys data - moved from app_config.c for proper linking */
#define smpEdiv 0x1F99
#define mcEncryptionKeySize_c 16

/* BLE initialization state */
static bool bleStackInitialized = FALSE;

static uint8_t smpLtk[gcSmpMaxLtkSize_c] = {
    0xD6, 0x93, 0xE8, 0xA4, 0x23, 0x55, 0x48, 0x99, 0x1D, 0x77, 0x61, 0xE6, 0x63, 0x2B, 0x10, 0x8E};

static uint8_t smpRand[gcSmpMaxRandSize_c] = {0x26, 0x1E, 0xF6, 0x09, 0x97, 0x2E, 0xAD, 0x7E};

static uint8_t smpIrk[gcSmpIrkSize_c] = {
    0x0A, 0x2D, 0xF4, 0x65, 0xE3, 0xBD, 0x7B, 0x49, 0x1E, 0xB4, 0xC0, 0x95, 0x95, 0x13, 0x46, 0x73};

static uint8_t smpCsrk[gcSmpCsrkSize_c] = {
    0x90, 0xD5, 0x06, 0x95, 0x92, 0xED, 0x91, 0xD7, 0xA8, 0x9E, 0x2C, 0xDC, 0x4A, 0x93, 0x5B, 0xF9};

/* SMP Keys definition - override weak definition in BLE stack */
#pragma GCC visibility push(default)
__attribute__((used)) gapSmpKeys_t gSmpKeys = {
    .cLtkSize  = mcEncryptionKeySize_c,
    .aLtk      = (void *)smpLtk,
    .aIrk      = (void *)smpIrk,
    .aCsrk     = (void *)smpCsrk,
    .aRand     = (void *)smpRand,
    .cRandSize = gcSmpMaxRandSize_c,
    .ediv      = smpEdiv,
};
#pragma GCC visibility pop

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

/* Forward declarations for functions */
void UWB_Interrupt_ISR(void);

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

// BLE state management (removed - not needed like demo)

// BLE connection state (similar to demo)
static advState_t mAdvState;
static qppsConfig_t qppServiceConfig = {service_qpps};
static uint16_t cpHandles[1] = {value_qpps_rx};
static uint16_t cpReadHandles[1] = {value_nearby_data};
static appPeerInfo_t mPeerInformation[gAppMaxConnections_c];
static uint32_t mAdvTimeout;  // Missing variable declaration

// Forward declarations
static void BleApp_AdvertisingCallback(gapAdvertisingEvent_t *pAdvertisingEvent);
static void BleApp_ConnectionCallback(deviceId_t peerDeviceId, gapConnectionEvent_t *pConnectionEvent);
static void BleApp_GattServerCallback(deviceId_t deviceId, gattServerEvent_t *pServerEvent);
static void BleApp_Config(void);
static void BleApp_Advertise(void);
static void BleApp_ReceivedDataHandler(deviceId_t deviceId, uint8_t *aValue, uint16_t valueLength);
static uint8_t BleApp_GetConnectedPeerCount(void);

void BleApp_Init(void)
{
#if gBtnSupported_d && (gBtn_Count_c > 0)
    Btn_Init(App_ButtonCallBack);
#endif
    UWB_Interrupt_ISR_Init();

#if defined(CPU_JN518X) && (cPWR_UsePowerDownMode)
    TMR_TimeStampInit();
    PWR_RegisterLowPowerExitCallback(UWBT_ExitPowerDownCb);
    PWR_RegisterLowPowerEnterCallback(UWBT_EnterLowPowerCb);
#endif

    // gSmpKeys is defined in this file, no need to force link
}



void BleApp_Start(void)
{
    NXPLOG_APP_I("BleApp_Start: Called, bleStackInitialized=%d", bleStackInitialized);

    if (!bleStackInitialized) {
        NXPLOG_APP_E("BleApp_Start: BLE stack not initialized, cannot start advertising");
        return;
    }

#if gAppUseBonding_d
#if defined(gAppUsePrivacy_d) && (gAppUsePrivacy_d > 0)
    if (gcBondedDevices > 0) {
        mAdvState.advType = fastWhiteListAdvState_c;
    }
    else
#endif
    {
#endif
        mAdvState.advType = defaultAdvState_c;
#if gAppUseBonding_d
    }
#endif

#if (gAppNtagSupported_d)
    NtagApp_NdefPairingWr(PERIPHERAL_AND_CENTRAL_ROLE, NTAG_LOCAL_DEV_NAME, strlen(NTAG_LOCAL_DEV_NAME));
#endif

    /* Actually start the advertising */
    NXPLOG_APP_I("BleApp_Start: Calling App_StartAdvertising...");
    if (App_StartAdvertising(BleApp_AdvertisingCallback, BleApp_ConnectionCallback) != gBleSuccess_c) {
        NXPLOG_APP_E("BleApp_Start: Failed to start advertising!");
        return;
    }
    NXPLOG_APP_I("BleApp_Start: App_StartAdvertising returned success");

    BleApp_Advertise();
}

void BleApp_Stop(void)
{
    if (!bleStackInitialized) {
        return;
    }

    (void)Gap_StopAdvertising();
}

void BleApp_ProcessEvents(void)
{
    if (!bleStackInitialized) {
        return;
    }

    // BLE events are handled through the callback system
    // No need to poll for events - they're delivered via callbacks
    // This function can be used for any periodic BLE-related tasks
}

bool BleApp_IsInitialized(void)
{
    return bleStackInitialized;
}

bool BleApp_IsAdvertising(void)
{
    return mAdvState.advOn;
}


void BleApp_GenericCallback(gapGenericEvent_t *pGenericEvent)
{
    /* Safety check - make sure we have a valid event */
    if (pGenericEvent == NULL) {
        return;
    }

    /* Debug: Log that we received a BLE event */
    NXPLOG_APP_I("BleApp_GenericCallback: eventType=0x%X", pGenericEvent->eventType);

    /* Call BLE Conn Manager */
    BleConnManager_GenericEvent(pGenericEvent);

    switch (pGenericEvent->eventType) {
    case gInitializationComplete_c: {
        /* Only configure if we haven't already */
        static bool bleConfigured = FALSE;
        if (!bleConfigured) {
            BleApp_Config();
            bleConfigured = TRUE;
            bleStackInitialized = TRUE;
        }
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
        break;
    }

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
    NXPLOG_APP_I("BleApp_Config: Starting BLE configuration");

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

    NXPLOG_APP_I("BleApp_Config: BLE configuration completed");
}

/*! *********************************************************************************
* \brief        Configures GAP Advertise parameters. Advertise will start after
*               the parameters are set.
*
********************************************************************************** */
static void BleApp_Advertise(void)
{
    NXPLOG_APP_I("BleApp_Advertise: Starting advertising with type=%d", mAdvState.advType);
    NXPLOG_APP_I("BleApp_Advertise: Current adv params - minInterval=0x%X, maxInterval=0x%X",
                 gAdvParams.minInterval, gAdvParams.maxInterval);

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
    NXPLOG_APP_I("BleApp_AdvertisingCallback: eventType=0x%X", pAdvertisingEvent->eventType);

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
    NXPLOG_APP_I("BleApp_ConnectionCallback: peerId=%d, eventType=0x%X",
                 peerDeviceId, pConnectionEvent->eventType);

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

/* Hardware callback stubs */
#if gBtnSupported_d && (gBtn_Count_c > 0)
void App_ButtonCallBack(uint8_t events)
{
    // Button callback implementation
}
#endif

/* UWB interrupt functions */
void UWB_Interrupt_ISR_Init(void)
{
    UWBT_InterruptISRInit(UWB_Interrupt_ISR);
}

void UWB_Interrupt_ISR(void)
{
    /** these re-initialization is not required here,
     * when device wakes up from Low Power Mode.
     *
     * When device wakes up from Low Power Mode,
     * the Low Power Mode task will do the required
     * re-initialization.
     */
}



#if defined(CPU_JN518X) && (cPWR_UsePowerDownMode)
void UWBT_ExitPowerDownCb(void)
{
    // Power down exit callback
}

void UWBT_EnterLowPowerCb(void)
{
    // Power down enter callback
}
#endif
