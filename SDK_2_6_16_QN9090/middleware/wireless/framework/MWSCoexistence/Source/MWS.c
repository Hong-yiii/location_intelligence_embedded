/*! *********************************************************************************
* Copyright 2017 NXP
* All rights reserved.
*
* \file
*
* This is the source file for the Mobile Wireless Standard Interface.
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

/************************************************************************************
*************************************************************************************
* Includes
*************************************************************************************
************************************************************************************/
#include "MWS.h"
#include "fsl_os_abstraction.h"
#include "GPIO_Adapter.h"

#if gMWS_UseCoexistence_d
#include "TimersManager.h"
#endif

#if gMWS_FsciEnabled_d
#include "FsciInterface.h"
#include "MemManager.h"
#endif

#ifdef CPU_QN908X
#include "controller_interface.h"
#include "fsl_iocon.h"
#endif
#if (gMWS_UseCoexistence_d) && (!gTimestamp_Enabled_d)
#warning The MWS Coexistence uses the Timestamp service. Please enable the TMR Timestamp (gTimestamp_Enabled_d).
#endif
/************************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
************************************************************************************/

/************************************************************************************
*************************************************************************************
* Private prototypes
*************************************************************************************
************************************************************************************/
#if gMWS_Enabled_d || gMWS_UseCoexistence_d
/*! *********************************************************************************
* \brief  MWS helper function used for chaining the active protocols, ordered by priority
*
* \param[in]  protocol - the protocol 
* \param[in]  priority - the priority to be set to above protocol
*
* \return  None
*
********************************************************************************** */
static void MWS_SetPriority (mwsProtocols_t protocol, uint8_t priority);
#endif

#if gMWS_UseCoexistence_d
/*! *********************************************************************************
* \brief  Interrupt Service Routine for handling the changes of the RF Deny pin
*
* \return  None
*
********************************************************************************** */
static void rf_deny_pin_changed(void);
#endif

#if gMWS_FsciEnabled_d
/*! *********************************************************************************
* \brief  Initialization function that registers MWS opcode group and the 
*         corresponding message handler
*
* \return  None
*
********************************************************************************** */
void MWS_FsciInit (void);

/*! *********************************************************************************
* \brief  MWS FSCI message handler
*
* \param[in]  pData - pointer to data message 
* \param[in]  param - pointer to additional parameters (if any)
* \param[in]  fsciInterface - FSCI interface used
*
* \return  None
*
********************************************************************************** */
void MWS_FsciMsgHandler (void* pData, void* param, uint32_t fsciInterface);
#endif

#if (gMWS_UseCoexistence_d == 1) && (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)
/*! *********************************************************************************
* \brief  This function is used to enable the RF_DENY pin interrupt
*
* \param[in]  mode - pin interrupt mode to be set (rising edge, falling edge, etc)
*
* \return  None
*
********************************************************************************** */
static void MWS_EnableRfDenyPinInterrupt(mwsPinInterruptMode_t mode);

/*! *********************************************************************************
* \brief  This function is used to disable the RF_DENY pin interrupt
*
* \return  None
*
********************************************************************************** */
static void MWS_DisableRfDenyPinInterrupt(void);
#endif


/************************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
************************************************************************************/
#if gMWS_Enabled_d || gMWS_UseCoexistence_d
/* Stores the id of the protocol with the Highest priority */
static mwsProtocols_t  mFirstMwsPrio = gMWS_None_c;
/* Stores the priority level for every protocol id */
static uint8_t mProtocolPriority[gMWS_None_c] =
{
    gMWS_BLEPriority_d, 
    gMWS_802_15_4Priority_d, 
    gMWS_ANTPriority_d, 
    gMWS_GENFSKPriority_d
};
/* Stores the id of the protocol with the next priority */
static mwsProtocols_t mProtocolNextPrio[gMWS_None_c] =
{
    gMWS_None_c, gMWS_None_c, gMWS_None_c, gMWS_None_c
};
#endif

#if gMWS_Enabled_d
/* Stores the id of the protocol which uses the XCVR */
static mwsProtocols_t mActiveProtocol = gMWS_None_c;
/* Stores MWS callback functions for every protocol */
static pfMwsCallback mMwsCallbacks[gMWS_None_c] = {NULL, NULL, NULL, NULL};
static uint32_t mwsLockCount = 0;
#endif

#if gMWS_UseCoexistence_d
/* Assume that the Coexistence GPIO pins are controlled by hardware */
static gpioInputPinConfig_t  *rf_deny    = NULL;
static gpioOutputPinConfig_t *rf_active  = NULL;
static gpioOutputPinConfig_t *rf_status  = NULL;
static mwsRfState_t mXcvrRfState;
static uint8_t mCoexFlags;
static uint8_t mCoexEnabled;
/* Stores Coexistence callback functions for every protocol */
static pfMwsCallback mCoexCallbacks[gMWS_None_c] = {NULL, NULL, NULL, NULL};

/* Signals that the MWS module is initialized or not */
static uint8_t mws_initialized = 0;
#if gMWS_EnableCoexistenceStats_d && (gMWS_EnableCoexistenceStats_d == 1)
static mwsCoexStats_t coexStats;
#endif
#endif

/************************************************************************************
*************************************************************************************
* Exported symbols
*************************************************************************************
************************************************************************************/
#if defined(gMWS_UseOverridableTimings) && (gMWS_UseOverridableTimings == 1)
volatile uint32_t gMWS_CoexPrioSignalTime_d = (15); /* us */
#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)
volatile uint32_t gMWS_CoexRfActiveAssertTime_d = (15); /* us */

#if defined(gMWS_CoexGrantDelay) && (gMWS_CoexGrantDelay != 0)
volatile uint32_t gMWS_CoexGrantPinSampleDelay = (gMWS_CoexGrantDelay); /* usec */
#endif

#endif /* (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d) */

volatile uint32_t gMWS_CoexConfirmWaitTime_d = (55); /* us. Note: Should be lower than gMWS_CoexRfActiveAssertTime_d! */
volatile uint32_t gMWS_CoexRfDenyActiveState_d = (1);
#endif /* defined(gMWS_UseOverridableTimings) && (gMWS_UseOverridableTimings == 1) */

/*** Callback example:

uint32_t protocolCallback ( mwsEvents_t event )
{
    uint32_t status = 0;

    switch(event)
    {
    case gMWS_Init_c:
        status = protocolInittFunction();
        break;
    case gMWS_Active_c:
        status = protocolSetActiveFunction();
        break;
    case gMWS_Abort_c:
        status = protocolAbortFunction();
        break;
    case gMWS_Idle_c:
        status = protocolNotifyIdleFunction();
        break;
    case gMWS_GetInactivityDuration_c:
        status = protocolGetInactiveDurationFunction();
        break;
    default:
        status = gMWS_InvalidParameter_c;
        break;
    }
    return status;
}
*/

/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
* \brief  This function will register a protocol stack into MWS
*
* \param[in]  protocol - One of the supported MWS protocols
* \param[in]  cb       - The callback function used by the MWS to signal events to 
*                        the protocol stack
*
* \return  mwsStatus_t
*
********************************************************************************** */
mwsStatus_t MWS_Register (mwsProtocols_t protocol, pfMwsCallback cb)
{
    mwsStatus_t status = gMWS_Success_c;
#if gMWS_Enabled_d
    static uint8_t initialized = 0;
    
    if( !initialized )
    {
        mActiveProtocol = gMWS_None_c;
        mFirstMwsPrio = gMWS_None_c;
        mwsLockCount = 0;
        initialized = 1;
    }
    
    if( (protocol >= gMWS_None_c) || (NULL == cb) )
    {
        status = gMWS_InvalidParameter_c;
    }
    else if( NULL == mMwsCallbacks[protocol] )
    {
        mMwsCallbacks[protocol] = cb;
        MWS_SetPriority(protocol, mProtocolPriority[protocol]);
        cb( gMWS_Init_c ); /* Signal the protocol */
    }
    else
    {
        /* Already registered! Only update callback */
        mMwsCallbacks[protocol] = cb;
    }
#endif
    return status;
}

/*! *********************************************************************************
* \brief  This function will poll all other protocols for their inactivity period, 
*         and will return the minimum time until the first protocol needs to be serviced.
*
* \param[in]  currentProtocol - One of the supported MWS protocols
*
* \return  the minimum inactivity duration (in microseconds)
*
********************************************************************************** */
uint32_t MWS_GetInactivityDuration (mwsProtocols_t currentProtocol)
{
    uint32_t duration = 0xFFFFFFFFU;
#if gMWS_Enabled_d
    uint32_t i, temp;

    /* Get the minimum inactivity duration from all protocols */
    for (i=0; i<NumberOfElements(mMwsCallbacks); i++)
    {
        if( (i != currentProtocol) && (mMwsCallbacks[i]) )
        {
            temp = mMwsCallbacks[i](gMWS_GetInactivityDuration_c);
            if( temp < duration )
            {
                duration = temp;
            }
        }
    }
#endif
    return duration;
}

/*! *********************************************************************************
* \brief  This function try to acquire access to the XCVR for the specified protocol
*
* \param[in]  protocol - One of the supported MWS protocols
* \param[in]  force    - If set, the active protocol will be preempted
*
* \return  If access to the XCVR is not granted, gMWS_Denied_c is returned.
*
********************************************************************************** */
mwsStatus_t MWS_Acquire (mwsProtocols_t protocol, uint8_t force)
{
    mwsStatus_t status = gMWS_Success_c;
#if gMWS_Enabled_d

    OSA_InterruptDisable();
    
    if (gMWS_None_c == mActiveProtocol)
    {
        mActiveProtocol = protocol;
        mwsLockCount = 1;
        mMwsCallbacks[mActiveProtocol](gMWS_Active_c);
    }
    else if( mActiveProtocol == protocol )
    { 
        mwsLockCount++;
    }
    else
    {
        /* Lower value means higher priority */
        if( (force)
#if gMWS_UsePrioPreemption_d
           || (mProtocolPriority[mActiveProtocol] > mProtocolPriority[protocol])
#endif
          )
        {
            status = (mwsStatus_t)mMwsCallbacks[mActiveProtocol](gMWS_Abort_c);
            mActiveProtocol = protocol;
            mwsLockCount = 1;
            mMwsCallbacks[mActiveProtocol](gMWS_Active_c);
        }
        else
        {
            status = gMWS_Denied_c;
        }
    }

    OSA_InterruptEnable();
#endif
    return status; 
}

/*! *********************************************************************************
* \brief  This function will release access to the XCVR, and will notify other 
*         protocols that the resource is idle.
*
* \param[in]  protocol - One of the supported MWS protocols
*
* \return  mwsStatus_t
*
********************************************************************************** */
mwsStatus_t MWS_Release (mwsProtocols_t protocol)
{
    mwsStatus_t status = gMWS_Success_c;
#if gMWS_Enabled_d

    if( mActiveProtocol != gMWS_None_c )
    {
        if (protocol == mActiveProtocol)
        {
            mwsLockCount--;
            
            if( 0 == mwsLockCount )
            {
                mMwsCallbacks[mActiveProtocol](gMWS_Release_c);
                mActiveProtocol = gMWS_None_c;
                
                /* Notify other protocols that XCVR is Idle, based on the priority */
                status = MWS_SignalIdle(protocol);
            }
        }
        else
        {
            /* Another protocol is using the XCVR */
            status = gMWS_Denied_c;
        }
    }
    else
    {
        status = MWS_SignalIdle(protocol);
    }
#endif
    return status;
}

/*! *********************************************************************************
* \brief  Force the active protocol to Idle state.
*
* \return  mwsStatus_t
*
********************************************************************************** */
mwsStatus_t MWS_Abort (void)
{
    mwsStatus_t status = gMWS_Success_c;
#if gMWS_Enabled_d

    if( mActiveProtocol != gMWS_None_c )
    {
        if( mMwsCallbacks[mActiveProtocol](gMWS_Abort_c) )
        {
            status = gMWS_Error_c;
        }
        mActiveProtocol = gMWS_None_c;
        mwsLockCount = 0;
    }
#endif
    return status;
}

/*! *********************************************************************************
* \brief  Returns the protocol that is currently using the XCVR
*
* \return  One of the supported MWS protocols
*
********************************************************************************** */
mwsProtocols_t MWS_GetActiveProtocol (void)
{
#if gMWS_Enabled_d
    return mActiveProtocol;
#else
    return gMWS_None_c;
#endif
}

/*! *********************************************************************************
* \brief  This function will notify other protocols that the specified protocol is 
*         Idle and the XCVR is unused.
*
* \param[in]  protocol - One of the supported MWS protocols
*
* \return  mwsStatus_t
*
********************************************************************************** */
mwsStatus_t MWS_SignalIdle  (mwsProtocols_t protocol)
{
    mwsStatus_t status = gMWS_Success_c;
#if gMWS_Enabled_d
    uint32_t i = mFirstMwsPrio;
    
    while( i != gMWS_None_c )
    {
        if( mActiveProtocol != gMWS_None_c )
        {
            break;
        }

        if( (i != protocol) && (NULL != mMwsCallbacks[i]) )
        {
            if( mMwsCallbacks[i](gMWS_Idle_c) )
            {
                status = gMWS_Error_c;
            }
        }
        i = mProtocolNextPrio[i];
    }
#endif
    return status;
}

/*! *********************************************************************************
* \brief  Initialize the MWS External Coexistence driver
*
* \param[in]  rfDenyPin   - Pointer to the GPIO input pin used to signal RF access 
*                           granted/denied. Set to NULL if controlled by hardware.
* \param[in]  rfActivePin - Pointer to the GPIO output pin used to signal RF activity
*                           Set to NULL if controlled by hardware.
* \param[in]  rfStatusPin - Pointer to the GPIO output pin to signal the RF activyty type
*                           Set to NULL if controlled by hardware.
*
* \return  mwsStatus_t
*
********************************************************************************** */
mwsStatus_t MWS_CoexistenceInit(void *rfDenyPin, void *rfActivePin, void *rfStatusPin)
{
    mwsStatus_t status = gMWS_Success_c;
#if gMWS_UseCoexistence_d
    
    rf_active = (gpioOutputPinConfig_t*)rfActivePin;
    rf_status = (gpioOutputPinConfig_t*)rfStatusPin;
    rf_deny   = (gpioInputPinConfig_t*)rfDenyPin;
    
#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)
    /* Disable no-blocking state pin edge interrup on deny pin as there is no use for it. The deny signal will 
       unblock once the request signal goes down */
#if defined(gMWS_CoexRfDenyActiveState_d)
#if (gMWS_CoexRfDenyActiveState_d == 1)
    rf_deny->interruptModeSelect = pinInt_RisingEdge_c;
#else
    rf_deny->interruptModeSelect = pinInt_FallingEdge_c;
#endif
#else
    rf_deny->interruptModeSelect = (gMWS_CoexRfDenyActiveState_d == 1) ? 
                                        pinInt_RisingEdge_c : pinInt_FallingEdge_c;
#endif
#endif    
    
    if( rf_active )
    {
        GpioOutputPinInit(rf_active, 1);
    }
    
    if( rf_status )
    {
        GpioOutputPinInit(rf_status, 1);
    }
    
    if( rf_deny )
    {
        GpioInputPinInit(rf_deny, 1);
    }
    
    if( !mws_initialized )
    {
        mws_initialized = 1;

        MWS_CoexistenceSetPriority(gMWS_CoexRxPrio_d, gMWS_CoexTxPrio_d);
        MWS_CoexistenceEnable();
        MWS_CoexistenceReleaseAccess();
        TMR_TimeStampInit();

        /* Check if the RF Confirm signal must be handled by Software */
        if( rf_deny )
        {
#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)
          MWS_DisableRfDenyPinInterrupt();
#endif
          if (gpio_success != GpioInstallIsr(rf_deny_pin_changed, gMWS_GpioIsrPrio_d, gMWS_GpioNvicPrio_d >> 1, rf_deny))
          {
              status = gMWS_InvalidParameter_c;
          }
        }

#if gMWS_FsciEnabled_d
        MWS_FsciInit();
#endif
    }
#endif
    return status;
}

/*! *********************************************************************************
* \brief  Enable Coexistence signals.
*
********************************************************************************** */
void MWS_CoexistenceEnable (void)
{
#ifdef WIFI_88W8801_RESET_WA
    int proto_idx;
#endif
#ifdef CPU_QN908X
    IOCON_PinMuxSet(IOCON, 0, 10, IOCON_FUNC6); //BLE_TX indicator, active high
    IOCON_PinMuxSet(IOCON, 0, 11, IOCON_FUNC6); //BLE_RX indicator, active high
    
    /* BLE Core slot timing reference - optional
       Can be routed to PA18 and PA26 */
    // IOCON_PinMuxSet(IOCON, 0, 18, IOCON_FUNC6); //BLE_SYNC, pluse
    // IOCON_PinMuxSet(IOCON, 0, 26, IOCON_FUNC6); //BLE_SYNC, pluse
    
    /* BLE Core is processing an event - optional
       Can be routed to PA19 and PA27. !!! CAREFULL with PA19 is it also used by SW2 button !!!  */
    // IOCON_PinMuxSet(IOCON, 0, 19, IOCON_FUNC6); //BLE_IN_PROC, active high.
    // IOCON_PinMuxSet(IOCON, 0, 27, IOCON_FUNC6); //BLE_IN_PROC, active high
    
    /* Priority pins - optional */
    // IOCON_PinMuxSet(IOCON, 0, 6, IOCON_FUNC5); //BLE_PTI[0]
    // IOCON_PinMuxSet(IOCON, 0, 7, IOCON_FUNC5); //BLE_PTI[1]
    // IOCON_PinMuxSet(IOCON, 0, 8, IOCON_FUNC5); //BLE_PTI[2]
    // IOCON_PinMuxSet(IOCON, 0, 9, IOCON_FUNC5); //BLE_PTI[3]
    
    /* WLAN signaling input pins - optional */
    // IOCON_PinMuxSet(IOCON, 0, 0, IOCON_FUNC6); //WLAN Tx, active high, input
    // IOCON_PinMuxSet(IOCON, 0, 1, IOCON_FUNC6); //WLAN_rx, active high, input
    
    BLE_EnableWlanCoex();
#else /* CPU_QN908X */
#if gMWS_UseCoexistence_d
    OSA_InterruptDisable();
#ifdef WIFI_88W8801_RESET_WA
    /* 
     * This is probably called after a disable of the
     * coexistence to accomodate for a WiFi FW initialization.
     * as such, re-enable the DENY pin interrupt here.
     * This can't be done before MWS is initialized, since the
     * IRQ handler is not yet installed.
     */
    if (mws_initialized != 0)
    {
#if defined(gMWS_CoexRfDenyActiveState_d)
    #if (gMWS_CoexRfDenyActiveState_d == 1)
        /* deny is when line goes from LOW to HIGH */
        MWS_EnableRfDenyPinInterrupt(gMWS_PinInterruptRisingEdge_c);
    #else
        /* deny is when line goes from HIGH to LOW */
        MWS_EnableRfDenyPinInterrupt(gMWS_PinInterruptFallingEdge_c);
    #endif
#else
    MWS_EnableRfDenyPinInterrupt(gMWS_CoexRfDenyActiveState_d == 1 ?
            gMWS_PinInterruptRisingEdge_c : gMWS_PinInterruptFallingEdge_c);
#endif
        /* 
         * Set it here: we want the protocol(s) to re-request access,
         * but if mCoexEnabled is not set, then nothing will happen: 
         * MWS_CoexistenceRequestAccess() will return success, without
         * signalling anything to the arbiter.
         */
        mCoexEnabled = 1;

        /* Now, send an abort all protocols */
        for(proto_idx=0; proto_idx<gMWS_None_c; proto_idx++ )
        {
            if (mCoexCallbacks[proto_idx])
            {
                mCoexCallbacks[proto_idx](gMWS_Abort_c);
            }
        }
        if (rf_active)
        {
            GpioClearPinOutput(rf_active);
        }

        if (rf_status)
        {
            GpioClearPinOutput(rf_status);
        }
    }
#endif
    mCoexEnabled = 1;
    OSA_InterruptEnable();
#endif
#endif /* CPU_QN908X */
}

/*! *********************************************************************************
* \brief  Disable Coexistence signals.
*
********************************************************************************** */
void MWS_CoexistenceDisable (void)
{
#if gMWS_UseCoexistence_d
    OSA_InterruptDisable();
    MWS_CoexistenceReleaseAccess();
#ifdef WIFI_88W8801_RESET_WA
    /*
     * 88W8801 is quirky about the state of the REQ & PRIO pins,
     * they need to be high (+3V3) whenever the firmware is initialized.
     * As such, force them high here, they will be set to their proper
     * states, once MWS is re-enabled. This needs to be done after the release,
     * because the MWS_CoexistenceReleaseAccess() puts both to GND.
     */
    if (rf_active)
    {
        GpioSetPinOutput(rf_active);
    }

    if (rf_status)
    {
        GpioSetPinOutput(rf_status);
    }
#endif
    mCoexEnabled = 0;    
    OSA_InterruptEnable();
#endif
}

/*! *********************************************************************************
* \brief  This function will register a protocol stack into MWS Coexistence module
*
* \param[in]  protocol - One of the supported MWS protocols
*
* \param[in]  cb       - The callback function used by the MWS Coexistence to signal 
*                        events to the protocol stack
*
* \return  mwsStatus_t
*
********************************************************************************** */
mwsStatus_t MWS_CoexistenceRegister (mwsProtocols_t protocol, pfMwsCallback cb)
{
    mwsStatus_t status = gMWS_Success_c;
#if gMWS_UseCoexistence_d
    
    if( (protocol >= gMWS_None_c) || (NULL == cb) )
    {
        status = gMWS_InvalidParameter_c;
    }
    else if( NULL == mCoexCallbacks[protocol] )
    {
        mCoexCallbacks[protocol] = cb;
        MWS_SetPriority(protocol, mProtocolPriority[protocol]);
        cb( gMWS_Init_c ); /* Signal the protocol */

        if( rf_deny )
        {
            if( GpioReadPinInput(rf_deny) == gMWS_CoexRfDenyActiveState_d )
            {
                cb(gMWS_Abort_c);
            }
            else
            {
                cb(gMWS_Idle_c);
            }
        }
    }
    else
    {
        /* Already registered! Only update callback */
        mCoexCallbacks[protocol] = cb;
    }
#endif    
    return status;
}

/*! *********************************************************************************
* \brief  This function returns the state of RF Deny pin.
*
* \return  uint8_t !gMWS_CoexRfDenyActiveState_d - RF Deny Inactive,
*                   gMWS_CoexRfDenyActiveState_d - RF Deny Active
*
********************************************************************************** */
uint8_t MWS_CoexistenceDenyState(void)
{
    uint8_t retVal = !gMWS_CoexRfDenyActiveState_d;
#if gMWS_UseCoexistence_d
    if( rf_deny )
    {
        if( GpioReadPinInput(rf_deny) == gMWS_CoexRfDenyActiveState_d )
        {
            retVal = gMWS_CoexRfDenyActiveState_d;
        }
    }
#endif
    return retVal;
}

/*! *********************************************************************************
* \brief  This function will register a protocol stack into MWS Coexistence module
*
* \param[in]  rxPrio - The priority of the RX sequence
* \param[in]  txPrio - The priority of the TX sequence
*
********************************************************************************** */
void MWS_CoexistenceSetPriority(mwsRfSeqPriority_t rxPrio, mwsRfSeqPriority_t txPrio)
{
#if gMWS_UseCoexistence_d
    OSA_InterruptDisable();
    
    if( rxPrio == gMWS_HighPriority )
    {
        mCoexFlags |= (1 << gMWS_RxState_c);
    }
    else
    {
        mCoexFlags &= ~(1 << gMWS_RxState_c);
    }
    
    if( txPrio == gMWS_HighPriority )
    {
        mCoexFlags |= 1 << gMWS_TxState_c;
    }
    else
    {
        mCoexFlags &= ~(1 << gMWS_TxState_c);
    }
    
    OSA_InterruptEnable();
#endif
}

/*! *********************************************************************************
* \brief  Request for permission to access the medium for the specified RF sequence
*
* \param[in]  newState - The RF sequence type
*
* \return  If RF access is not granted, gMWS_Denied_c is returned.
*
********************************************************************************** */
mwsStatus_t MWS_CoexistenceRequestAccess(mwsRfState_t newState)
{
    mwsStatus_t status = gMWS_Success_c;
#if gMWS_UseCoexistence_d
    uint64_t timestamp;    

#ifdef WIFI_ALWAYS_GRANT_RX_LO_PRIO
    uint8_t askForGrant = 1;
    /*
     * For 88W8801, the guideline from CnS is to always grant
     * receive, especially since the 15.4 & the WiFi
     * antennas are different. So instead of going through all
     */
    if ((newState == gMWS_RxState_c) && /* doing an RX */
        ((mCoexFlags & (1 << newState)) == 0)) /* with low priority */
    {
        askForGrant = 0;
#if gMWS_EnableCoexistenceStats_d && (gMWS_EnableCoexistenceStats_d == 1)
        coexStats.numRxRequests++;
        coexStats.numRxGrantImmediate++;
#endif

    }
    if (mCoexEnabled && askForGrant)
#else
    if (mCoexEnabled)
#endif
    {
        OSA_InterruptDisable();
        /* Set Priority signal */
        if( (rf_status) && (mCoexFlags & (1 << newState)) )
        {
            GpioSetPinOutput(rf_status);
        }
        
        /* Signal that protocol is about to become active */
        if( rf_active )
        {
            GpioSetPinOutput(rf_active);
        }
#if gMWS_EnableCoexistenceStats_d && (gMWS_EnableCoexistenceStats_d == 1)
        if (newState == gMWS_RxState_c)
        {
            coexStats.numRxRequests++;
        }
        else
        {
            coexStats.numTxRequests++;
        }
#endif
        timestamp = TMR_GetTimestamp();    
        
#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)
        /* wait gMWS_CoexPrioSignalTime_d us */
        while( (TMR_GetTimestamp() - timestamp) < gMWS_CoexPrioSignalTime_d ) {;};

        /* Set RF sequence type: RX/TX */
        if( rf_status )
        {
            if( newState == gMWS_RxState_c )
            {
                /* Set status line LOW to signal RX sequence */
                GpioClearPinOutput(rf_status);
            }
            else
            {
                /* Set status line HIGH to signal TX sequence */
                GpioSetPinOutput(rf_status);
            }
        }
#endif // gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d
                        
        OSA_InterruptEnable();
        
        /* Wait for confirm signal */
        if( rf_deny )
        {
            status = gMWS_Denied_c; /* assume access is denied */
            
#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)
            /* Wait for PTA to assess access request and disable rf_deny pin interrupt */
            while( (TMR_GetTimestamp() - timestamp) < gMWS_CoexConfirmWaitTime_d ) {;};
#endif            
#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)
    #if defined(gMWS_CoexGrantDelay) && (gMWS_CoexGrantDelay != 0)
        while( (TMR_GetTimestamp() - timestamp) < gMWS_CoexGrantPinSampleDelay ) {;};
    #endif
#endif
            do
            {
                if(GpioReadPinInput(rf_deny) != gMWS_CoexRfDenyActiveState_d)
                { 
                  /* PTA granted the access */
                  status = gMWS_Success_c;
                  mXcvrRfState = newState;                    
#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)
  /* Enable pin interrupt when PTA will deny the access */
  #if defined(gMWS_CoexRfDenyActiveState_d)
  #if (gMWS_CoexRfDenyActiveState_d == 1)
                  /* deny is when line goes from LOW to HIGH */
                  MWS_EnableRfDenyPinInterrupt(gMWS_PinInterruptRisingEdge_c);
  #else
                  /* deny is when line goes from HIGH to LOW */
                  MWS_EnableRfDenyPinInterrupt(gMWS_PinInterruptFallingEdge_c);
  #endif
  #else
                  MWS_EnableRfDenyPinInterrupt(gMWS_CoexRfDenyActiveState_d == 1 ? 
                                        gMWS_PinInterruptRisingEdge_c : gMWS_PinInterruptFallingEdge_c);
  #endif
#endif
#if gMWS_EnableCoexistenceStats_d && (gMWS_EnableCoexistenceStats_d == 1)
                  if (newState == gMWS_RxState_c)
                  {
                    coexStats.numRxGrantWait++;
                  }
                  else
                  {
                    coexStats.numTxGrantWait++;
                  }
#endif
                    break;
                }
            }
#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)          
            while( (TMR_GetTimestamp() - timestamp) <  gMWS_CoexRfActiveAssertTime_d);
#else
            /* In PTA mode we need to release access and request again as the deny line 
               will remain high as long as the request line does */ 
            while(0);
#endif      
            if( status != gMWS_Success_c )
            {
                MWS_CoexistenceReleaseAccess();
#if gMWS_EnableCoexistenceStats_d && (gMWS_EnableCoexistenceStats_d == 1)
                if (newState == gMWS_RxState_c)
                {
                    coexStats.numRxGrantWaitTimeout++;
                }
                else
                {
                    coexStats.numTxGrantWaitTimeout++;
                }
#endif
            }
        }
    }
#endif // gMWS_UseCoexistence_d
    return status;
}

/*! *********************************************************************************
* \brief  This function will signal the change of the RF state, and request for permission
*
* \param[in]  newState - The new state in which the XCVR will transition
*
* \return  If RF access is not granted, gMWS_Denied_c is returned.
*
********************************************************************************** */
mwsStatus_t MWS_CoexistenceChangeAccess(mwsRfState_t newState)
{
    mwsStatus_t status = gMWS_Success_c;

#if gMWS_UseCoexistence_d
    if( mCoexEnabled )
    {
        if( mXcvrRfState == gMWS_IdleState_c )
        {
            status = gMWS_Denied_c;
        }
        else
        {
            mXcvrRfState = newState;
#if (gMWS_Coex_Model_d == gMWS_Coex_Status_Prio_d)            
            if( rf_status )
            {        
                if( newState == gMWS_RxState_c )
                {
                    /* Set status line LOW to signal RX sequence */
                    GpioClearPinOutput(rf_status);
                }
                else
                {
                    /* Set status line HIGH to signal TX sequence */
                    GpioSetPinOutput(rf_status);
                }
            }
#endif			
        }
    }
#endif
    return status;
}

/*! *********************************************************************************
* \brief  Signal externally that the Radio is not using the medium anymore.
*
********************************************************************************** */
void MWS_CoexistenceReleaseAccess(void)
{
#if gMWS_UseCoexistence_d
    if( mCoexEnabled )
    {
#if (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)
    /* Disable the RF_DENY pin interrupt */        
    MWS_DisableRfDenyPinInterrupt();      
#endif      
        OSA_InterruptDisable();
        mXcvrRfState = gMWS_IdleState_c;
#if gMWS_EnableCoexistenceStats_d && (gMWS_EnableCoexistenceStats_d == 1)
        coexStats.numReleases++;
#endif
        if( rf_active )
        {
            GpioClearPinOutput(rf_active);
        }
        
        if( rf_status )
        {
            GpioClearPinOutput(rf_status);
        }                
        OSA_InterruptEnable();
    }
#endif
}

/************************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
* \brief  This function is used to enable the RF_DENY pin interrupt
*
* \param[in]  mode - pin interrupt mode to be set (rising edge, falling edge, etc)
*
* \return  None
*
********************************************************************************** */
#if (gMWS_UseCoexistence_d == 1) && (gMWS_Coex_Model_d == gMWS_Coex_Prio_Only_d)
static void MWS_EnableRfDenyPinInterrupt(mwsPinInterruptMode_t mode)
{ 
  if(rf_deny)
  {
    switch(mode)
    {
    case gMWS_PinInterruptFallingEdge_c:
      rf_deny->interruptModeSelect = pinInt_FallingEdge_c;
      break;
    
    case gMWS_PinInterruptRisingEdge_c:
      rf_deny->interruptModeSelect = pinInt_RisingEdge_c;
      break;
    
    case gMWS_PinInterruptEitherEdge_c:                  
      rf_deny->interruptModeSelect = pinInt_EitherEdge_c;
      break;
      
    default:
      rf_deny->interruptModeSelect = pinInt_Invalid_c;
      break;
    }
    GpioSetPinInterrupt(rf_deny->gpioPort, rf_deny->gpioPin, rf_deny->interruptModeSelect);
  }
}

/*! *********************************************************************************
* \brief  This function is used to disable the RF_DENY pin interrupt
*
* \return  None
*
********************************************************************************** */
static void MWS_DisableRfDenyPinInterrupt(void)
{
  if(rf_deny)
  {        
    rf_deny->interruptModeSelect = pinInt_Disabled_c;
    GpioSetPinInterrupt(rf_deny->gpioPort, rf_deny->gpioPin, rf_deny->interruptModeSelect);
  }
}
#endif

/*! *********************************************************************************
* \brief  MWS helper function used for chaining the active protocols, ordered by priority
*
* \param[in]  protocol - the protocol 
* \param[in]  priority - the priority to be set to the above protocol
*
* \return  None
*
********************************************************************************** */
#if gMWS_Enabled_d || gMWS_UseCoexistence_d
static void MWS_SetPriority (mwsProtocols_t protocol, uint8_t priority)
{
    mwsProtocols_t i;
    
    if( (mFirstMwsPrio == gMWS_None_c) || (priority <= mProtocolPriority[mFirstMwsPrio]) )
    {
        /* Insert at the begining of the list */
        mProtocolNextPrio[protocol] = mFirstMwsPrio;
        mFirstMwsPrio = protocol;
    }
    else
    {
        i = mFirstMwsPrio;
        
        while( i != gMWS_None_c )
        {
            if( mProtocolNextPrio[i] == gMWS_None_c )
            {
                /* Insert at the end of the list */
                mProtocolNextPrio[protocol] = gMWS_None_c;
                mProtocolNextPrio[i] = protocol;
                i = gMWS_None_c;
            }
            else if (priority <= mProtocolPriority[mProtocolNextPrio[i]])
            {
                mProtocolNextPrio[protocol] = mProtocolNextPrio[i];
                mProtocolNextPrio[i] = protocol;
                i = gMWS_None_c;
            }
            else
            {
                i = mProtocolNextPrio[i];
            }
        }
    }
}
#endif

/*! *********************************************************************************
* \brief  Interrupt Service Routine for handling the changes of the RF Deny pin
*
* \return  None
*
********************************************************************************** */
#if gMWS_UseCoexistence_d
static void rf_deny_pin_changed(void)
{
    uint32_t i;
       
    GpioClearPinIntFlag(rf_deny);
    
    if( mCoexEnabled )
    {
        if( GpioReadPinInput(rf_deny) == gMWS_CoexRfDenyActiveState_d )       
        {
            /* Abort all protocols */
            for( i=0; i<gMWS_None_c; i++ )
            {
                if( mCoexCallbacks[i] )
                {
                    mCoexCallbacks[i](gMWS_Abort_c);
                }
            }
#if gMWS_EnableCoexistenceStats_d && (gMWS_EnableCoexistenceStats_d == 1)
            coexStats.numAborts++;
#endif
        }
        else
        {
            i = mFirstMwsPrio;
            
            while( i != gMWS_None_c )
            {
                if( NULL != mCoexCallbacks[i] )
                {
                    mCoexCallbacks[i](gMWS_Idle_c);
                }
                i = mProtocolNextPrio[i];
            }       
        }        
    }
}
#endif /* gMWS_UseCoexistence_d */

/*! *********************************************************************************
* \brief  Initialization function that registers MWS opcode group and the 
*         corresponding message handler
*
* \return  None
*
********************************************************************************** */
#if gMWS_FsciEnabled_d
void MWS_FsciInit( void )
{
    FSCI_RegisterOpGroup(gMWS_FsciReqOG_d,
                         gFsciMonitorMode_c,
                         MWS_FsciMsgHandler,
                         NULL,
                         gMWS_FsciInterface_d);
}
#endif

/*! *********************************************************************************
* \brief  MWS FSCI message handler
*
* \param[in]  pData - pointer to data message 
* \param[in]  param - pointer to additional parameters (if any)
* \param[in]  fsciInterface - FSCI interface used
*
* \return  None
*
********************************************************************************** */
#if gFsciIncluded_c && gMWS_FsciEnabled_d
void MWS_FsciMsgHandler( void* pData, void* param, uint32_t fsciInterface )
{
    gFsciStatus_t status = gFsciSuccess_c;

    switch(((clientPacket_t*)pData)->structured.header.opCode)
    {
    case mFsciMsgCoexEnableReq_c:
        MWS_CoexistenceEnable();
        break;

    case mFsciMsgCoexDisableReq_c:
        MWS_CoexistenceDisable();
        break;

    default:
        status = gFsciUnknownOpcode_c;
        FSCI_Error( gFsciUnknownOpcode_c, fsciInterface );
        break;
    }
    
    if( gFsciUnknownOpcode_c == status )
    {
        MEM_BufferFree(pData);
    }
    else
    {
        /* Reuse the received message */
        ((clientPacket_t*)pData)->structured.header.opGroup = gMWS_FsciCnfOG_d;
        ((clientPacket_t*)pData)->structured.header.len = 1;
        ((clientPacket_t*)pData)->structured.payload[0] = status;
        FSCI_transmitFormatedPacket((clientPacket_t*)pData, fsciInterface);
    }
}
#endif

#if gMWS_UseCoexistence_d
/*! *********************************************************************************
* \brief  MWS state query helper
*
* \return  0 if coexistence is enabled, != 0 otherwise
*
********************************************************************************** */
uint8_t MWS_CoexistenceIsEnabled(void)
{
    return mCoexEnabled;
}
#endif /* gMWS_UseCoexistence_d */

#if gMWS_UseCoexistence_d
#if gMWS_EnableCoexistenceStats_d && (gMWS_EnableCoexistenceStats_d == 1)
/*! *********************************************************************************
* \brief  MWS stats query helper
*
* * \param[in/out]  stats - pointer where the acquired statistics will be written
*
* \return  0 if stats were succesfully retrieved, != 0 otherwise
*
********************************************************************************** */
uint8_t MWS_GetCoexStats(mwsCoexStats_t *stats)
{
    mwsStatus_t status = gMWS_Error_c;
    if (stats)
    {
        status = gMWS_Success_c;

        memset(stats, 0, sizeof(mwsCoexStats_t));

        stats->numTxRequests = coexStats.numTxRequests;
        stats->numTxGrantWait = coexStats.numTxGrantWait;
        stats->numTxGrantWaitTimeout = coexStats.numTxGrantWaitTimeout;

        stats->numRxRequests = coexStats.numRxRequests;
        stats->numRxGrantImmediate = coexStats.numRxGrantImmediate;
        stats->numRxGrantWait = coexStats.numRxGrantWait;
        stats->numRxGrantWaitTimeout = coexStats.numRxGrantWaitTimeout;

        stats->numReleases = coexStats.numReleases;
        stats->numAborts = coexStats.numAborts;
    }
    return (uint8_t)status;
}
#endif /*  gMWS_EnableCoexistenceStats_d && (gMWS_EnableCoexistenceStats_d == 1) */
#endif /* gMWS_UseCoexistence_d */
