/*! *********************************************************************************
* \addtogroup App Config
* @{
********************************************************************************** */
/*! *********************************************************************************
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2021-2023 NXP
* All rights reserved.
*
* \file
*
* This file contains configuration data for the application and stack
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */
#if defined(UWBIOT_APP_BUILD__MY_CUSTOM_APP) || defined(UWBIOT_APP_BUILD__DEMO_NEARBY_INTERACTION)
/************************************************************************************
*************************************************************************************
* Include
*************************************************************************************
************************************************************************************/
#include "gap_interface.h"
#include "ble_constants.h"
#include "gatt_db_handles.h"
#include "ble_app.h"

/************************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
************************************************************************************/
#define smpEdiv               0x1F99
// mcEncryptionKeySize_c moved to ble_app.c
#define LOCAL_NAME_SIZE       24
/************************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
************************************************************************************/
/* clang-format off */
// Note: UUID symbols are defined in gatt_database.c to avoid X-macro conflicts
// The X-macro system is fragile and requires symbols to be defined before inclusion
/* clang-format on */

/* Default Advertising Parameters. Values can be changed at runtime
    to align with profile requirements */
gapAdvertisingParameters_t gAdvParams = {
    /* minInterval */ gGapAdvertisingIntervalDefault_c,
    /* maxInterval */ gGapAdvertisingIntervalDefault_c,
    /* advertisingType */ gAdvConnectableUndirected_c,
    /* ownAddressType */ gBleAddrTypePublic_c,
    /* directedAddressType */ gBleAddrTypePublic_c,
    /* directedAddress */ {0, 0, 0, 0, 0, 0},
    /* channelMap */ (gapAdvertisingChannelMapFlags_t)(gGapAdvertisingChannelMapDefault_c),
    /* filterPolicy */ gProcessAll_c};

/* Scanning and Advertising Data */
static const uint8_t adData0[1] = {(gapAdTypeFlags_t)(gLeGeneralDiscoverableMode_c | gBrEdrNotSupported_c)};
static const gapAdStructure_t advScanStruct[3] = {
    {.length = NumberOfElements(adData0) + 1, .adType = gAdFlags_c, .aData = (uint8_t *)adData0},
    {.length    = NumberOfElements(uuid_service_qpps) + 1,
        .adType = gAdComplete128bitServiceList_c,
        .aData  = (uint8_t *)uuid_service_qpps},
    {.adType = gAdShortenedLocalName_c, .length = 3 + 1, .aData = (uint8_t *)"Tag"}};

uint8_t localName[LOCAL_NAME_SIZE]   = {"NXP_SR150"};
gapAdStructure_t advScanRspStruct[1] = {{.length = 9 + 1, .adType = gAdCompleteLocalName_c, .aData = localName}};

gapAdvertisingData_t gAppAdvertisingData = {NumberOfElements(advScanStruct), (void *)advScanStruct};

gapScanResponseData_t gAppScanRspData = {NumberOfElements(advScanRspStruct), (void *)advScanRspStruct};

/* SMP Data */
gapPairingParameters_t gPairingParameters = {
    .withBonding                 = gAppUseBonding_d,
    .securityModeAndLevel        = gSecurityMode_1_Level_3_c,
    .maxEncryptionKeySize        = 16,  // mcEncryptionKeySize_c moved to ble_app.c
    .localIoCapabilities         = gIoDisplayOnly_c,
    .oobAvailable                = FALSE,
    .centralKeys                 = gLtk_c,
    .peripheralKeys              = (gapSmpKeyFlags_t)(gLtk_c | gIrk_c),
    .leSecureConnectionSupported = TRUE,
    .useKeypressNotifications    = FALSE,
};

// SMP keys data moved to ble_app.c for proper linking

/* Device Security Requirements */
static const gapSecurityRequirements_t masterSecurity            = gGapDefaultSecurityRequirements_d;
static const gapServiceSecurityRequirements_t serviceSecurity[3] = {
    /* qpps */
    {.requirements     = {.securityModeLevel = gSecurityMode_1_Level_3_c,
         .authorization                  = FALSE,
         .minimumEncryptionKeySize       = gDefaultEncryptionKeySize_d},
        .serviceHandle = service_qpps},

    /* qpps */
    {.requirements     = {.securityModeLevel = gSecurityMode_1_Level_3_c,
         .authorization                  = FALSE,
         .minimumEncryptionKeySize       = gDefaultEncryptionKeySize_d},
        .serviceHandle = service_nearby}};

gapDeviceSecurityRequirements_t deviceSecurityRequirements = {.pMasterSecurityRequirements = (void *)&masterSecurity,
    .cNumServices                                                                          = 3,
    .aServiceSecurityRequirements                                                          = (void *)serviceSecurity};

#endif /* UWBIOT_APP_BUILD__MY_CUSTOM_APP || UWBIOT_APP_BUILD__DEMO_NEARBY_INTERACTION */
