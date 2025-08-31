/*! *********************************************************************************
 * \addtogroup GATT_DB
 * @{
 ********************************************************************************** */
/*! *********************************************************************************
* Copyright (c) 2014, Freescale Semiconductor, Inc.
* Copyright 2016-2018,2022-2023 NXP
* All rights reserved.
*
* \file
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */
#ifndef UWBIOT_APP_BUILD__MY_CUSTOM_APP
#include "UWBIOT_APP_BUILD.h"
#endif

#ifdef UWBIOT_APP_BUILD__MY_CUSTOM_APP
/************************************************************************************
*************************************************************************************
* Default local configuration
*************************************************************************************
************************************************************************************/
#ifndef gGattDbDynamic_d
#define gGattDbDynamic_d 0
#endif

#if defined(SOTA_ENABLED) && gGattDbDynamic_d
#error "gGattDbDynamic_d cannot be used when SOTA_ENABLED is enabled"
#endif

#if defined(SOTA_BLOB_APP) && !defined(SOTA_ENABLED)
#error "SOTA_ENABLED should be defined when SOTA_BLOB_APP is enabled"
#endif

#if defined(SOTA_BLOB_BLE_HOST) && !defined(SOTA_ENABLED)
#error "SOTA_ENABLED should be defined when SOTA_BLOB_BLE_HOST is enabled"
#endif

/************************************************************************************
*************************************************************************************
* Include
*************************************************************************************
************************************************************************************/
#include "gatt_database.h"
#include "gatt_db_app_interface.h"
#include "gatt_types.h"
#include "gap_types.h"
#include "app_config.h"

/************************************************************************************
*************************************************************************************
* GATT Database Symbol Definitions - Required by X-macros
* These must be defined BEFORE the X-macro includes
*************************************************************************************
************************************************************************************/
// GATT characteristic properties
uint8_t char_qpps_rx_prop = 0x08; // gGattCharPropWrite_c equivalent

// UUID definitions - must match gatt_uuid128_uwb_ble.h
uint8_t uuid_service_qpps[16] = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E};
uint8_t uuid_qpps_characteristics_rx[16] = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E};
uint8_t uuid_qpps_characteristics_tx[16] = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E};
uint8_t uuid_service_nearby[16] = {0xba, 0x2d, 0x9c, 0x68, 0x73, 0x30, 0x33, 0x86, 0xb2, 0x4b, 0x17, 0x08, 0x40, 0x3e, 0xfe, 0x48};
uint8_t uuid_nearby_AccessoryData[16] = {0x28, 0x33, 0xf5, 0x75, 0x73, 0x80, 0x4e, 0x9a, 0x21, 0x47, 0xef, 0xd8, 0xd5, 0xd9, 0xe8, 0x95};

#if !defined(SOTA_BLOB_BLE_HOST)
#include "board.h"

#if gGattDbDynamic_d
#include "gatt_db_dynamic.h"
#else
/*! Macros and X-Macros */
#include "gatt_db_macros.h"
#include "gatt_db_x_macros.h"
#include "gatt_db_handles.h"
#endif
#endif



/************************************************************************************
*************************************************************************************
* X-Macro expansions - enums, structs and memory allocations
*************************************************************************************
************************************************************************************/
#if (!gGattDbDynamic_d && !defined(SOTA_ENABLED)) || defined(SOTA_BLOB_APP)

/*! Allocate the arrays for Attribute Values */
#include "gatt_uuid_decl_x.h"
#include "gatt_alloc_x_uwb_ble.h"

/*! Declare the Attribute database */
static gattDbAttribute_t static_gattDatabase[] = {
#include "gatt_decl_x.h"
};

gattDbAttribute_t *gattDatabase = static_gattDatabase;

/*! Declare structure to compute the database size */
typedef struct sizeCounterStruct_tag
{
#include "gatt_size_x.h"
} sizeCounterStruct_t;

/*! Compute the database size at compile time */
#define localGattDbAttributeCount_d ((sizeof(sizeCounterStruct_t)) / 4U)
uint16_t gGattDbAttributeCount_c;

#else
gattDbAttribute_t *gattDatabase;
uint16_t gGattDbAttributeCount_c;
#endif

#define DEVICE_NAME "NXP_Custom_UWB"

/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
* \brief    Function performing runtime initialization of the GATT database.
*
* \remarks  This function should be called only once at device startup.
*
********************************************************************************** */
bleResult_t GattDb_Init(void)
{
    /* Update char_qpps_rx property value based on application */
    extern uint8_t char_qpps_rx_prop;
    char_qpps_rx_valueArray[0] = char_qpps_rx_prop;
    memset(value_device_name_valueArray, 0, 12);
    memcpy(value_device_name_valueArray, DEVICE_NAME, strlen(DEVICE_NAME));
    static_gattDatabase[6].valueLength = strlen(DEVICE_NAME);
#if (!gGattDbDynamic_d && !defined(SOTA_ENABLED)) || defined(SOTA_BLOB_APP)
    static bool_t mAlreadyInit = FALSE;
    if (mAlreadyInit) {
        return gBleAlreadyInitialized_c;
    }
    mAlreadyInit = TRUE;

    /*! Assign the database size to the global */
    gGattDbAttributeCount_c = localGattDbAttributeCount_d;

    /*! Attribute-specific initialization by X-Macro expansion */
#include "gatt_init_x.h"

    return gBleSuccess_c;
#else
#if defined(SOTA_BLOB_BLE_HOST)
    gGattDbAttributeCount_c = GattDb_GetAttributeCount();
    gattDatabase            = GattDb_GetDatabase();
    return gBleSuccess_c;
#else
    return GattDbDynamic_Init();
#endif /* SOTA_BLOB_BLE_HOST */
#endif
}

#if !defined(SOTA_ENABLED) || defined(SOTA_BLOB_BLE_HOST)
/*! *********************************************************************************
* \brief    Database searching function, return the index for a given attribute handle.
*
* \param[in] handle  The attribute handle.
*
* \return  The index of the given attribute in the database or gGattDbInvalidHandleIndex_d.
*
********************************************************************************** */
uint16_t GattDb_GetIndexOfHandle(uint16_t handle)
{
    uint16_t init = (handle >= gGattDbAttributeCount_c) ? (gGattDbAttributeCount_c - 1U) : handle;
    for (uint16_t j = init; j != 0xFFFFU && gattDatabase[j].handle >= handle; j--) {
        if (gattDatabase[j].handle == handle) {
            return j;
        }
    }
    return gGattDbInvalidHandleIndex_d;
}

#endif /* !defined(SOTA_ENABLED) || defined(SOTA_BLOB_BLE_HOST) */

#if defined(SOTA_BLOB_APP)
/*! *********************************************************************************
* \brief   Returns the address of the database.
*
*
* \return  The address of the database
*
********************************************************************************** */
gattDbAttribute_t *GattDb_GetDatabase()
{
    return gattDatabase;
}
/*! *********************************************************************************
* \brief   Returns the size of the database.
*
*
* \return  The size of the database
*
********************************************************************************** */
uint16_t GattDb_GetAttributeCount()
{
    return gGattDbAttributeCount_c;
}
#endif /* SOTA_BLOB_APP */
#endif /* UWBIOT_APP_BUILD__MY_CUSTOM_APP */
/*! *********************************************************************************
* @}
********************************************************************************** */
