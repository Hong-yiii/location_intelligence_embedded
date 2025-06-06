/* Copyright 2022 NXP
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only
 * be used strictly in accordance with the applicable license terms.  By
 * expressly accepting such terms or by downloading, installing, activating
 * and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms.  If
 * you do not agree to be bound by the applicable license terms, then you may
 * not retain, install, activate or otherwise use the software.
 */
#include "uwb_board.h"

#if (1 == UWB_BOARD_ENABLE_EXT_FLASH_BASED_FW_DOWNLOAD)
/* Flash based FW Download */
#include "phNxpLogApis_App.h"

#if UWBIOT_UWBD_SR1XXT
#include "uwb_extfl_provider_interface.h"

/* clang-format off */
/*Default crc table */
 static const uint16_t gCrcXmodemTable[256] = {
 0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b,
 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 0x1231, 0x210,  0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
 0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 0x2462, 0x3443, 0x420, 0x1401,
 0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
 0x3653, 0x2672, 0x1611, 0x630,  0x76d7, 0x66f6, 0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738,
 0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5, 0x6886, 0x78a7, 0x840,  0x1861, 0x2802, 0x3823,
 0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96,
 0x1a71, 0xa50,  0x3a33, 0x2a12, 0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
 0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0xc60,  0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd,
 0xad2a, 0xbd0b, 0x8d68, 0x9d49, 0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0xe70,
 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb,
 0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0xa1,   0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
 0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x2b1,  0x1290, 0x22f3, 0x32d2,
 0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
 0x34e2, 0x24c3, 0x14a0, 0x481,  0x7466, 0x6447, 0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8,
 0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x691,  0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
 0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827,
 0x18c0, 0x8e1,  0x3882, 0x28a3, 0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
 0x4a75, 0x5a54, 0x6a37, 0x7a16, 0xaf1,  0x1ad0, 0x2ab3, 0x3a92, 0xfd2e, 0xed0f, 0xdd6c, 0xcd4d,
 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0xcc1,
 0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
 0x2e93, 0x3eb2, 0xed1,  0x1ef0,};
/* clang-format on */

UWBStatus_t uwb_fwdl_provider_init(uwb_fwdl_provider_ctx_t *pCtx)
{
    if (pCtx == NULL) {
        LOG_E("uwbs fwdl provider context is NULL");
        return kUWBSTATUS_FAILED;
    }

    if (uwb_bus_flash_init(&pCtx->flashCtx) != kUWB_bus_Status_OK) {
        LOG_E("Error: uwb_fwdl_provider_init Failed");
        return kUWBSTATUS_FAILED;
    }

    return kUWBSTATUS_SUCCESS;
}

bool uwb_fwdl_provider_setmode(uwb_fwdl_provider_ctx_t *pCtx, eFirmwareMode mode)
{
    if (pCtx == NULL) {
        LOG_E("uwbs fwdl provider context is NULL");
        return FALSE;
    }

    pCtx->mode = mode;

    switch (mode) {
    case MAINLINE_FW:
        pCtx->fwStartAddr = MAINLINE_FW_ADDR;
        pCtx->fwLenAddr   = MAINLINE_FW_LEN_ADDR;
        pCtx->fwCrcAddr   = MAINLINE_FW_CRC_ADDR;
        return TRUE;

    case FACTORY_FW:
        pCtx->fwStartAddr = FACTORY_FW_ADDR;
        pCtx->fwLenAddr   = FACTORY_FW_LEN_ADDR;
        pCtx->fwCrcAddr   = FACTORY_FW_CRC_ADDR;
        return TRUE;
    default:
        return FALSE;
    }

    return TRUE;
}

UWBStatus_t uwb_fwdl_provider_deinit(uwb_fwdl_provider_ctx_t *pCtx)
{
    if (pCtx == NULL) {
        LOG_E("uwbs fwdl provider context is NULL");
        return kUWBSTATUS_FAILED;
    }

    if (uwb_bus_flash_deinit(&pCtx->flashCtx) != kUWB_bus_Status_OK) {
        LOG_E("Error: uwb_bus_flash_deinit Failed");
        return kUWBSTATUS_FAILED;
    }

    return kUWBSTATUS_SUCCESS;
}

UWBStatus_t uwb_fwdl_provider_get_fwLength(uwb_fwdl_provider_ctx_t *pCtx, uint32_t *pfwLen)
{
    if (pCtx == NULL) {
        LOG_E("uwbs fwdl provider context is NULL");
        return kUWBSTATUS_FAILED;
    }

    uwb_bus_flash_read(&pCtx->flashCtx, (uint8_t *)pfwLen, 4, pCtx->fwLenAddr);

    return kUWBSTATUS_SUCCESS;
}

UWBStatus_t uwb_fwdl_provider_crc_check(uwb_fwdl_provider_ctx_t *pCtx, uint16_t *pFwCrc)
{
    // uint8_t data = 0;
    if (pCtx == NULL) {
        LOG_E("uwbs fwdl provider context is NULL");
        return kUWBSTATUS_FAILED;
    }

    uwb_bus_flash_read(&pCtx->flashCtx, (uint8_t *)pFwCrc, 2, pCtx->fwCrcAddr);

    return kUWBSTATUS_SUCCESS;
}

UWBStatus_t uwb_fwdl_provider_data_write(uwb_fwdl_provider_ctx_t *pCtx, const uint8_t *pBuf, size_t bufLen)
{
    if (pCtx == NULL) {
        LOG_E("uwbs fwdl provider context is NULL");
        return kUWBSTATUS_FAILED;
    }

    if (pBuf == NULL || bufLen == 0) {
        return kUWBSTATUS_FAILED;
    }

    uwb_bus_flash_write(&pCtx->flashCtx, pBuf, bufLen, pCtx->flashStartAddr);

    return kUWBSTATUS_SUCCESS;
}

UWBStatus_t uwb_fwdl_provider_data_read(uwb_fwdl_provider_ctx_t *pCtx, uint8_t *pBuf, uint32_t bufLen)
{
    if (pCtx == NULL) {
        LOG_E("uwbs fwdl provider context is NULL");
        return kUWBSTATUS_FAILED;
    }

    uwb_bus_flash_read(&pCtx->flashCtx, pBuf, bufLen, pCtx->flashStartAddr);

    return kUWBSTATUS_SUCCESS;
}

UWBStatus_t uwb_fwdl_provider_data_trx(uwb_fwdl_provider_ctx_t *pCtx,
    const uint8_t *pTxBuf,
    size_t txBufLen,
    uint8_t *pRxBuf,
    size_t rxBufLen,
    uint32_t FlashAddr)
{
    if (uwb_fwdl_provider_data_write(pCtx, pTxBuf, txBufLen) != kUWBSTATUS_SUCCESS) {
        LOG_E("uwb_fwdl_provider_data_trx write data failed");
        return kUWBSTATUS_FAILED;
    }

    if (uwb_fwdl_provider_data_read(pCtx, pRxBuf, rxBufLen) != kUWBSTATUS_SUCCESS) {
        LOG_E("uwb_fwdl_provider_data_trx read data failed");
        return kUWBSTATUS_FAILED;
    }
    return kUWBSTATUS_SUCCESS;
}

UWBStatus_t uwb_fwdl_provider_generate_crc(
    uwb_fwdl_provider_ctx_t *pCtx, const uint8_t *pBuf, const uint32_t bufLen, uint16_t *pCrc)
{
    if (pCtx == NULL) {
        LOG_E("uwbs fwdl provider context is NULL");
        return kUWBSTATUS_FAILED;
    }

    if (pBuf == NULL || bufLen == 0) {
        return kUWBSTATUS_FAILED;
    }

    *pCrc = 0;
    for (size_t i = 0; i < bufLen; ++i) {
        uint8_t default_crc = ((*pCrc >> 8) ^ (0xff & pBuf[i]));
        *pCrc               = (uint16_t)((*pCrc << 8) ^ gCrcXmodemTable[default_crc]);
    }
    return kUWBSTATUS_SUCCESS;
}

UWBStatus_t uwb_fwdl_provider_flash_erase(uwb_fwdl_provider_ctx_t *pCtx, const uint32_t FwLen)
{
    if (pCtx == NULL) {
        LOG_E("uwbs fwdl provider context is NULL");
        return kUWBSTATUS_FAILED;
    }

    if (uwb_bus_flash_erase(&pCtx->flashCtx, pCtx->fwCrcAddr, FwLen) != kUWB_bus_Status_OK) {
        return kUWBSTATUS_FAILED;
    }

    return kUWBSTATUS_SUCCESS;
}

UWBStatus_t uwb_fwdl_provider_verify_fw(
    uwb_fwdl_provider_ctx_t *pCtx, const uint8_t *pBuf, const uint32_t bufLen, uint32_t StartAddr)
{
    uint32_t i = 0, err = 0, wr = 0;
    uint8_t data = 0;

    if (pCtx == NULL) {
        LOG_E("uwbs fwdl provider context is NULL");
        return kUWBSTATUS_FAILED;
    }

    if (pBuf == NULL || bufLen == 0) {
        return kUWBSTATUS_FAILED;
    }

    for (i = 0; i < (EXT_FLASH_SIZE); i++) {
        pCtx->flashStartAddr = StartAddr + i;
        uwb_bus_flash_read(&pCtx->flashCtx, &data, 1, pCtx->flashStartAddr);
        // LOG_I("Data in memory address 0x%x, is 0x%x\r\n", i, data);
        if (data != pBuf[i]) {
            LOG_E("Data error in address 0x%x, the value in memory is 0x%x org:0x%x\r\n", i, data, pBuf[i]);
            err++;
            if (err > DataError) {
                LOG_E("ERROR: Incorrect Firmware");
                return kUWBSTATUS_FAILED;
            }
        }
        wr++;
        if (wr == bufLen) {
            return kUWBSTATUS_SUCCESS;
        }
    }
    if (err == 0) {
        LOG_I("All data Read is correct!\r\n");
    }

    return kUWBSTATUS_SUCCESS;
}

UWBStatus_t uwb_fwdl_provider_generate_fwDetails(
    uwb_fwdl_provider_ctx_t *pCtx, uint8_t *fwDetails, uint16_t Crc, uint32_t fwLen)
{
    if (pCtx == NULL) {
        LOG_E("uwbs fwdl provider context is NULL");
        return kUWBSTATUS_FAILED;
    }
    if (Crc == 0 || fwLen == 0) {
        return kUWBSTATUS_FAILED;
    }

    for (int i = 0; i < 2; i++) {
        fwDetails[i] = Crc >> (8 * i);
    }
    for (int i = 0; i < 4; i++) {
        fwDetails[2 + i] = fwLen >> (8 * i);
    }
    return kUWBSTATUS_SUCCESS;
}

#endif // UWBIOT_UWBD_SR1XXT
#endif // UWB_BOARD_ENABLE_EXT_FLASH_BASED_FW_DOWNLOAD
