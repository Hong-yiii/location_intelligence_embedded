/* ********************************************************************************* */
/* Copyright 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ********************************************************************************** */

#ifndef GATT_ALLOC_X_UWB_BLE_H
#define GATT_ALLOC_X_UWB_BLE_H

#define PRIMARY_SERVICE           XALLOC_PRIMARY_SERVICE
#define PRIMARY_SERVICE_UUID32    XALLOC_PRIMARY_SERVICE_UUID32
#define PRIMARY_SERVICE_UUID128   XALLOC_PRIMARY_SERVICE_UUID128
#define SECONDARY_SERVICE         XALLOC_SECONDARY_SERVICE
#define SECONDARY_SERVICE_UUID32  XALLOC_SECONDARY_SERVICE_UUID32
#define SECONDARY_SERVICE_UUID128 XALLOC_SECONDARY_SERVICE_UUID128
#define INCLUDE                   XALLOC_INCLUDE
#define INCLUDE_CUSTOM            XALLOC_INCLUDE_CUSTOM
#define CHARACTERISTIC            XALLOC_CHARACTERISTIC
#define CHARACTERISTIC_UUID32     XALLOC_CHARACTERISTIC_UUID32
#define CHARACTERISTIC_UUID128    XALLOC_CHARACTERISTIC_UUID128
#define VALUE                     XALLOC_VALUE
#define VALUE_UUID32              XALLOC_VALUE_UUID32
#define VALUE_UUID128             XALLOC_VALUE_UUID128
#define VALUE_VARLEN              XALLOC_VALUE_VARLEN
#define VALUE_UUID32_VARLEN       XALLOC_VALUE_UUID32_VARLEN
#define VALUE_UUID128_VARLEN      XALLOC_VALUE_UUID128_VARLEN
#define CCCD                      XALLOC_CCCD
#define DESCRIPTOR                XALLOC_DESCRIPTOR
#define DESCRIPTOR_UUID32         XALLOC_DESCRIPTOR
#define DESCRIPTOR_UUID128        XALLOC_DESCRIPTOR

#include "gatt_db_uwb_ble.h"

#undef PRIMARY_SERVICE
#undef PRIMARY_SERVICE_UUID32
#undef PRIMARY_SERVICE_UUID128
#undef SECONDARY_SERVICE
#undef SECONDARY_SERVICE_UUID32
#undef SECONDARY_SERVICE_UUID128
#undef INCLUDE
#undef INCLUDE_CUSTOM
#undef CHARACTERISTIC
#undef CHARACTERISTIC_UUID32
#undef CHARACTERISTIC_UUID128
#undef VALUE
#undef VALUE_UUID32
#undef VALUE_UUID128
#undef VALUE_VARLEN
#undef VALUE_UUID32_VARLEN
#undef VALUE_UUID128_VARLEN
#undef CCCD
#undef DESCRIPTOR
#undef DESCRIPTOR_UUID32
#undef DESCRIPTOR_UUID128

#endif /* GATT_ALLOC_X_UWB_BLE_H */
