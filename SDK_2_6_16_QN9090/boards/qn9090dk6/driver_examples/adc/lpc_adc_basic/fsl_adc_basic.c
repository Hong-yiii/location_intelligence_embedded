/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_adc.h"
#include "fsl_clock.h"
#include "fsl_power.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_flash.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEMO_ADC_BASE ADC0
#define DEMO_ADC_TEMPERATURE_SENSOR_CHANNEL 7U
#define DEMO_ADC_SAMPLE_CHANNEL_NUMBER 0U
#define THREE_HUNDRED_MICROSECONDS 300
#define DEMO_ADC_CLOCK_DIVIDER 7U

#define DEMO_ADC_CLOCK_MODE kADC_ClockAsynchronousMode

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static void ADC_Configuration(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/
adc_result_info_t adcResultInfoStruct;
const uint32_t g_Adc_12bitFullRange = 4096U;

/*******************************************************************************
 * Code
 ******************************************************************************/

static void ADC_ClockPower_Configuration(void)
{
    /* Enable ADC clock */
    CLOCK_EnableClock(kCLOCK_Adc0);

    /* Power on the ADC converter. */
    POWER_EnablePD(kPDRUNCFG_PD_LDO_ADC_EN);

    /* Enable LDO ADC 1v1 */
    PMC->PDRUNCFG |= PMC_PDRUNCFG_ENA_LDO_ADC_MASK;

    CLOCK_AttachClk(kFRO12M_to_ADC_CLK);
    CLOCK_SetClkDiv(kCLOCK_DivAdcClk, 3U, false);
}


/*!
 * @brief Main function
 */
int main(void)
{
    /* Initialize board hardware. */
    /* Security code to allow debug access */
    SYSCON->CODESECURITYPROT = 0x87654320;

    /* reset FLEXCOMM for USART */
    RESET_PeripheralReset(kFC0_RST_SHIFT_RSTn);

#if (BOARD_BOOTCLOCKRUN_CORE_CLOCK > 32000000U)
    /* When the CPU clock frequency is increased,
     * the Set Read command shall be called before the frequency change. */
    FLASH_SetReadMode(FLASH, true);
#endif
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    BOARD_InitPins();

    /* Enable the power and clock for ADC. */
    ADC_ClockPower_Configuration();
    PRINTF("ADC basic example.\r\n");

#if !(defined(FSL_FEATURE_ADC_HAS_NO_CALIB_FUNC) && FSL_FEATURE_ADC_HAS_NO_CALIB_FUNC)
#if defined(FSL_FEATURE_ADC_HAS_CALIB_REG) && FSL_FEATURE_ADC_HAS_CALIB_REG
    /* Calibration after power up. */
    if (ADC_DoSelfCalibration(DEMO_ADC_BASE))
#else
    uint32_t frequency;
#if defined(SYSCON_ADCCLKDIV_DIV_MASK)
    frequency = CLOCK_GetFreq(DEMO_ADC_CLOCK_SOURCE) / CLOCK_GetClkDivider(kCLOCK_DivAdcClk);
#else
    frequency = CLOCK_GetFreq(DEMO_ADC_CLOCK_SOURCE);
#endif /* SYSCON_ADCCLKDIV_DIV_MASK */
    /* Calibration after power up. */
    if (ADC_DoSelfCalibration(DEMO_ADC_BASE, frequency))
#endif /* FSL_FEATURE_ADC_HAS_CALIB_REG */
    {
        PRINTF("ADC_DoSelfCalibration() Done.\r\n");
    }
    else
    {
        PRINTF("ADC_DoSelfCalibration() Failed.\r\n");
    }
#endif /* FSL_FEATURE_ADC_HAS_NO_CALIB_FUNC */

    /* Configure the converter and work mode. */
    ADC_Configuration();
    PRINTF("Configuration Done.\r\n");

#if defined(FSL_FEATURE_ADC_HAS_CTRL_RESOL) & FSL_FEATURE_ADC_HAS_CTRL_RESOL
    PRINTF("ADC Full Range: %d\r\n", g_Adc_12bitFullRange);
#endif /* FSL_FEATURE_ADC_HAS_CTRL_RESOL */
    while (1)
    {
        /* Get the input from terminal and trigger the converter by software. */
        GETCHAR();
        ADC_DoSoftwareTriggerConvSeqA(DEMO_ADC_BASE);

        /* Wait for the converter to be done. */
        while (!ADC_GetChannelConversionResult(DEMO_ADC_BASE, DEMO_ADC_SAMPLE_CHANNEL_NUMBER, &adcResultInfoStruct))
        {
        }
        PRINTF("adcResultInfoStruct.result        = %d\r\n", adcResultInfoStruct.result);
        PRINTF("adcResultInfoStruct.channelNumber = %d\r\n", adcResultInfoStruct.channelNumber);
        PRINTF("adcResultInfoStruct.overrunFlag   = %d\r\n", adcResultInfoStruct.overrunFlag ? 1U : 0U);
        PRINTF("\r\n");
    }
}

static void ADC_Configuration(void)
{
    adc_config_t adcConfigStruct;
    adc_conv_seq_config_t adcConvSeqConfigStruct;

/* Configure the converter. */
#if defined(FSL_FEATURE_ADC_HAS_CTRL_ASYNMODE) & FSL_FEATURE_ADC_HAS_CTRL_ASYNMODE
    adcConfigStruct.clockMode = DEMO_ADC_CLOCK_MODE;
#endif /* FSL_FEATURE_ADC_HAS_CTRL_ASYNMODE */
    adcConfigStruct.clockDividerNumber = DEMO_ADC_CLOCK_DIVIDER;
#if defined(FSL_FEATURE_ADC_HAS_CTRL_RESOL) & FSL_FEATURE_ADC_HAS_CTRL_RESOL
    adcConfigStruct.resolution = kADC_Resolution12bit;
#endif /* FSL_FEATURE_ADC_HAS_CTRL_RESOL */
#if defined(FSL_FEATURE_ADC_HAS_CTRL_BYPASSCAL) & FSL_FEATURE_ADC_HAS_CTRL_BYPASSCAL
    adcConfigStruct.enableBypassCalibration = false;
#endif /* FSL_FEATURE_ADC_HAS_CTRL_BYPASSCAL */
#if defined(FSL_FEATURE_ADC_HAS_CTRL_TSAMP) & FSL_FEATURE_ADC_HAS_CTRL_TSAMP
    adcConfigStruct.sampleTimeNumber = 0U;
#endif /* FSL_FEATURE_ADC_HAS_CTRL_TSAMP */
#if (defined(FSL_FEATURE_ADC_HAS_GPADC_CTRL0_GPADC_TSAMP) && FSL_FEATURE_ADC_HAS_GPADC_CTRL0_GPADC_TSAMP)
    adcConfigStruct.extendSampleTimeNumber = kADC_ExtendSampleTimeNotUsed;
#endif /* FSL_FEATURE_ADC_HAS_GPADC_CTRL0_GPADC_TSAMP */
#if defined(FSL_FEATURE_ADC_HAS_CTRL_LPWRMODE) & FSL_FEATURE_ADC_HAS_CTRL_LPWRMODE
    adcConfigStruct.enableLowPowerMode = false;
#endif /* FSL_FEATURE_ADC_HAS_CTRL_LPWRMODE */
#if defined(FSL_FEATURE_ADC_HAS_TRIM_REG) & FSL_FEATURE_ADC_HAS_TRIM_REG
    adcConfigStruct.voltageRange = kADC_HighVoltageRange;
#endif /* FSL_FEATURE_ADC_HAS_TRIM_REG */
    ADC_Init(DEMO_ADC_BASE, &adcConfigStruct);

#if !(defined(FSL_FEATURE_ADC_HAS_NO_INSEL) && FSL_FEATURE_ADC_HAS_NO_INSEL)
    /* Use the temperature sensor input to channel 0. */
    ADC_EnableTemperatureSensor(DEMO_ADC_BASE, false);
#endif /* FSL_FEATURE_ADC_HAS_NO_INSEL. */

    /* Enable channel DEMO_ADC_SAMPLE_CHANNEL_NUMBER's conversion in Sequence A. */
    adcConvSeqConfigStruct.channelMask =
        (1U << DEMO_ADC_SAMPLE_CHANNEL_NUMBER); /* Includes channel DEMO_ADC_SAMPLE_CHANNEL_NUMBER. */
    adcConvSeqConfigStruct.triggerMask      = 0U;
    adcConvSeqConfigStruct.triggerPolarity  = kADC_TriggerPolarityPositiveEdge;
    adcConvSeqConfigStruct.enableSingleStep = false;
    adcConvSeqConfigStruct.enableSyncBypass = false;
    adcConvSeqConfigStruct.interruptMode    = kADC_InterruptForEachSequence;
    ADC_SetConvSeqAConfig(DEMO_ADC_BASE, &adcConvSeqConfigStruct);
#if defined(FSL_FEATURE_ADC_REQUIRE_DELAY) && FSL_FEATURE_ADC_REQUIRE_DELAY
    /* A problem with the ADC requires a delay after setup, see RFT 1340 */
    CLOCK_uDelay(THREE_HUNDRED_MICROSECONDS);
#endif                                       /* FSL_FEATURE_ADC_REQUIRE_DELAY */
    ADC_EnableConvSeqA(DEMO_ADC_BASE, true); /* Enable the conversion sequence A. */
}
