/* Copyright 2020,2022 NXP
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only
 * be used strictly in accordance with the applicable license terms.  By
 * expressly accepting such terms or by downloading, installing, activating
 * and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms.  If
 * you do not agree to be bound by the applicable license terms, then you may
 * not retain, install, activate or otherwise use the software.
 */

#ifndef UWB_LOGGING_H
#define UWB_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <phUwb_BuildConfig.h>

#include <uwb_board.h>

void phUwb_LogInit(void);
void phUwb_LogDeInit(void);

#ifndef PRINTF
#error PRINTF must be defined by now
#endif

#define LOG(...)         \
    PRINTF(__VA_ARGS__); \
    PRINTF("\n");

#define ALOGD(...)          LOG(__VA_ARGS__)
#define ALOGE(...)          LOG(__VA_ARGS__)
#define ALOGD_IF(cond, ...) LOG(__VA_ARGS__)
// Make Zero to disable
// Define Logging Levels
#define UWB_LOG_SILENT_LEVEL 0x00
#define UWB_LOG_ERROR_LEVEL  0x01
#define UWB_LOG_WARN_LEVEL   0x02
#define UWB_LOG_INFO_LEVEL   0x03
#define UWB_LOG_DEBUG_LEVEL  0x04
#define UWB_LOG_TX_LEVEL     0x05
#define UWB_LOG_RX_LEVEL     0x06

#include "phNxpLogDefault.h"

#define LOG_PRINT nLog

EXTERNC void nLog(const char *tag, int level, const char *fmt, ...);
EXTERNC void nLog_au8(const char *tag, int level, const char *message, const unsigned char *array, size_t array_len);
/* show a spinnner that keeps roatiting at one fixed place, just to show progress */
EXTERNC void phUwb_LogPrint_Spinner(void);

/* for log call back from test frameworks
 * Not to be used in production */
typedef void (*fpUwb_LogPrintCb_t)(const char *szString);
EXTERNC void phUwb_LogPrintSetCb(fpUwb_LogPrintCb_t fpCbLogSzFn);

#define LOG_PRI nLog

#define phUwb_LogMsg(trace_set_mask, ...) LOG(__VA_ARGS__)

EXTERNC void phUwb_LogSetColor(int level);
EXTERNC void phUwb_LogReSetColor(void);

#ifdef __cplusplus
} // closing brace for extern "C"
#endif

#endif // UWB_LOGGING_H
