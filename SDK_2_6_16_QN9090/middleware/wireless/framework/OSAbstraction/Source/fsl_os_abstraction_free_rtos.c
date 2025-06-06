/*! *********************************************************************************
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017, 2019, 2022 NXP
* All rights reserved.
*
* \file
*
* This is the source file for the OS Abstraction layer for freertos.
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "EmbeddedTypes.h"
#include "fsl_os_abstraction.h"
#include "fsl_os_abstraction_free_rtos.h"
#include <string.h>
#include "GenericList.h"
#include "fsl_common.h"
#include "Panic.h"
#include "MemManager.h"
#include "FunctionLib.h"
#include "PWR_Interface.h"



/* Only for debug purposes to check if restored stack is identical to the one
 * on power down */
#undef StackCorruptionDetect_d

#ifdef StackCorruptionDetect_d
#include "SecLib.h"
#endif

#define DmaMemMove_d 1
#if (DmaMemMove_d != 0)
#include "fsl_dma.h"
#endif
/*! *********************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
********************************************************************************** */
#define millisecToTicks(millisec) ((millisec * configTICK_RATE_HZ + 999)/1000)

#ifdef DEBUG_ASSERT
#define OS_ASSERT(condition) if(!(condition))while(1);
#else
#define OS_ASSERT(condition) (void)(condition);
#endif

#if (osNumberOfEvents)
#define osObjectAlloc_c 1
#else
#define osObjectAlloc_c 0
#endif

#if (configSUPPORT_STACK_UNRETAINED == 1)
/* MAX_TASB_NB must be sufficient to hold have a one task_tb entry per FreeRTOS task */
#define MAX_TASK_NB 8u
#if (DmaMemMove_d != 0)
#define DMA_MEMCPY_CHANNEL 0
#endif
#endif
/*! @brief Converts milliseconds to ticks*/
#define MSEC_TO_TICK(msec)  (((uint32_t)(msec)+500uL/(uint32_t)configTICK_RATE_HZ) \
                             *(uint32_t)configTICK_RATE_HZ/1000uL)
#define TICKS_TO_MSEC(tick) ((uint64_t)(tick)*1000uL/(uint32_t)configTICK_RATE_HZ)
/************************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
************************************************************************************/

typedef struct osEventStruct_tag
{
    uint32_t inUse;
    event_t event;
}osEventStruct_t;

typedef struct osObjStruct_tag
{
    uint32_t inUse;
    uint32_t osObj;
}osObjStruct_t;

typedef struct osObjectInfo_tag
{
    void* pHeap;
    uint32_t objectStructSize;
    uint32_t objNo;
} osObjectInfo_t;

#if!defined OsaBasePri_d
#define OsaBasePri_d 0
#endif

#if (configSUPPORT_STACK_UNRETAINED == 1)
typedef struct  {
    task_handle_t tsk_hdl;
    uint32_t *stack_top;
    uint32_t *retained_stk;
    uint32_t retained_sz;
#if DmaMemMove_d != 0
    dma_transfer_config_t dma_cfg;
#endif
#ifdef StackCorruptionDetect_d
    uint8_t stack_digest[SHA256_HASH_SIZE];
#endif
} compressed_task_stack_t;
#endif
/*! *********************************************************************************
*************************************************************************************
* Private prototypes
*************************************************************************************
********************************************************************************** */
#if osObjectAlloc_c
static void* osObjectAlloc(const osObjectInfo_t* pOsObjectInfo);
static bool_t osObjectIsAllocated(const osObjectInfo_t* pOsObjectInfo, void* pObjectStruct);
static void osObjectFree(const osObjectInfo_t* pOsObjectInfo, void* pObjectStruct);
#endif
extern void main_task(void const *argument);
extern void hardware_init(void);
void startup_task(void* argument);


/*! *********************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
********************************************************************************** */
const uint8_t gUseRtos_c = USE_RTOS;  // USE_RTOS = 0 for BareMetal and 1 for OS

#if OsaBasePri_d
static uint32_t g_base_priority_array[OSA_MAX_ISR_CRITICAL_SECTION_DEPTH];
static int32_t  g_base_priority_top = 0;
#endif

#if (configSUPPORT_STACK_UNRETAINED == 1)
static compressed_task_stack_t task_tb[MAX_TASK_NB] = { 0 };
static uint8_t nb_tasks_created = 0;
static uint32_t stack_words_copied;
static uint32_t stack_words_restored;

static uint32_t pd_cnt = 0;
static uint32_t wu_cnt = 0;

#if (DmaMemMove_d != 0)
dma_handle_t g_DMA_memcpy_handle;
DMA_ALLOCATE_LINK_DESCRIPTORS(dma_desc, (MAX_TASK_NB+1));
#endif
#endif
/*! *********************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
********************************************************************************** */

#if osNumberOfEvents
osEventStruct_t osEventHeap[osNumberOfEvents];
const osObjectInfo_t osEventInfo = {osEventHeap, sizeof(osEventStruct_t),osNumberOfEvents};
#endif


/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */

/*FUNCTION**********************************************************************
 *
 * Function Name : startup_task
 * Description   : Wrapper over main_task..
 *
 *END**************************************************************************/
void startup_task(void* argument)
{
    main_task(argument);
    while(1);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_TaskGetId
 * Description   : This function is used to get current active task's handler.
 *
 *END**************************************************************************/
osaTaskId_t OSA_TaskGetId(void)
{
    return xTaskGetCurrentTaskHandle();
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_TaskYield
 * Description   : When a task calls this function, it will give up CPU and put
 * itself to the tail of ready list.
 *
 *END**************************************************************************/
osaStatus_t OSA_TaskYield(void)
{
    taskYIELD();
    return osaStatus_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_TaskGetPriority
 * Description   : This function returns task's priority by task handler.
 *
 *END**************************************************************************/
osaTaskPriority_t OSA_TaskGetPriority(osaTaskId_t taskId)
{
  return (osaTaskPriority_t)(PRIORITY_RTOS_TO_OSA(uxTaskPriorityGet((task_handle_t)taskId)));
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_TaskSetPriority
 * Description   : This function sets task's priority by task handler.
 *
 *END**************************************************************************/
osaStatus_t OSA_TaskSetPriority(osaTaskId_t taskId, osaTaskPriority_t taskPriority)
{
    vTaskPrioritySet((task_handle_t)taskId, PRIORITY_OSA_TO_RTOS(taskPriority));
    return osaStatus_Success;
}


/*FUNCTION**********************************************************************
*
* Function Name : OSA_TaskCreate
* Description   : This function is used to create a task and make it ready.
* Param[in]     :  threadDef  - Definition of the thread.
*                  task_param - Parameter to pass to the new thread.
* Return Thread handle of the new thread, or NULL if failed.
*
*END**************************************************************************/
osaTaskId_t OSA_TaskCreate(osaThreadDef_t *thread_def,osaTaskParam_t task_param)
{
    osaTaskId_t taskId = NULL;
    task_handle_t task_handle = NULL;

    if (xTaskCreate(
                  (task_t)thread_def->pthread,  /* pointer to the task */
                  (char const*)thread_def->tname, /* task name for kernel awareness debugging */
                  thread_def->stacksize/sizeof(portSTACK_TYPE), /* task stack size */
                  (task_param_t)task_param, /* optional task startup argument */
                  PRIORITY_OSA_TO_RTOS(thread_def->tpriority),  /* initial priority */
                  &task_handle /* optional task handle to create */
                    ) == pdPASS)
    {
        taskId = (osaTaskId_t)task_handle;
    }

    return taskId;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_TaskDestroy
 * Description   : This function destroy a task.
 * Param[in]     :taskId - Thread handle.
 * Return osaStatus_Success if the task is destroyed, otherwise return osaStatus_Error.
 *
 *END**************************************************************************/
osaStatus_t OSA_TaskDestroy(osaTaskId_t taskId)
{
  osaStatus_t status;
  uint16_t oldPriority;
  /*Change priority to avoid context switches*/
  oldPriority = OSA_TaskGetPriority(OSA_TaskGetId());
  (void)OSA_TaskSetPriority(OSA_TaskGetId(), OSA_PRIORITY_REAL_TIME);
#if INCLUDE_vTaskDelete /* vTaskDelete() enabled */
  vTaskDelete((task_handle_t)taskId);
#if (configSUPPORT_STACK_UNRETAINED == 1)
  for (uint8_t i = 0; i<MAX_TASK_NB; i++)
  {
      if ((osaTaskId_t)task_tb[i].tsk_hdl == taskId)
      {
          task_tb[i].tsk_hdl = NULL;
          nb_tasks_created--;
      }
  }
#endif
  status = osaStatus_Success;
#else
  status = osaStatus_Error; /* vTaskDelete() not available */
#endif
  (void)OSA_TaskSetPriority(OSA_TaskGetId(), oldPriority);

  return status;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_TimeDelay
 * Description   : This function is used to suspend the active thread for the given number of milliseconds.
 *
 *END**************************************************************************/
void OSA_TimeDelay(uint32_t millisec)
{
    vTaskDelay(millisecToTicks(millisec));
}
/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_TimeGetMsec
 * Description   : This function gets current time in milliseconds.
 *
 *END**************************************************************************/
uint32_t OSA_TimeGetMsec(void)
{
    portTickType ticks;

    if (__get_IPSR())
    {
        ticks = xTaskGetTickCountFromISR();
    }
    else
    {
        ticks = xTaskGetTickCount();
    }

    return TICKS_TO_MSEC(ticks);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_GetTickCount
 * Description   : This function gets number of ticks.
 *
 *END**************************************************************************/
uint32_t OSA_GetTickCount(void)
{
    portTickType ticks;

    if (__get_IPSR())
    {
        ticks = xTaskGetTickCountFromISR();
    }
    else
    {
        ticks = xTaskGetTickCount();
    }
    return (uint32_t)ticks;
}
/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_SemaphoreCreate
 * Description   : This function is used to create a semaphore.
 * Return         : Semaphore handle of the new semaphore, or NULL if failed.
  *
 *END**************************************************************************/
osaSemaphoreId_t OSA_SemaphoreCreate(uint32_t initValue)
{
#if osNumberOfSemaphores
    semaphore_t sem;
    sem = xSemaphoreCreateCounting(0xFF, initValue);
    return (osaSemaphoreId_t)sem;
#else
    (void)initValue;
    return NULL;
#endif

}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_SemaphoreDestroy
 * Description   : This function is used to destroy a semaphore.
 * Return        : osaStatus_Success if the semaphore is destroyed successfully, otherwise return osaStatus_Error.
 *
 *END**************************************************************************/
osaStatus_t OSA_SemaphoreDestroy(osaSemaphoreId_t semId)
{
#if osNumberOfSemaphores
  semaphore_t sem = (semaphore_t)semId;
  if(sem == NULL)
  {
    return osaStatus_Error;
  }
  vSemaphoreDelete(sem);
  return osaStatus_Success;
#else
  (void)semId;
  return osaStatus_Error;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_SemaphoreWait
 * Description   : This function checks the semaphore's counting value, if it is
 * positive, decreases it and returns osaStatus_Success, otherwise, timeout
 * will be used for wait. The parameter timeout indicates how long should wait
 * in milliseconds. Pass osaWaitForever_c to wait indefinitely, pass 0 will
 * return osaStatus_Timeout immediately if semaphore is not positive.
 * This function returns osaStatus_Success if the semaphore is received, returns
 * osaStatus_Timeout if the semaphore is not received within the specified
 * 'timeout', returns osaStatus_Error if any errors occur during waiting.
 *
 *END**************************************************************************/
osaStatus_t OSA_SemaphoreWait(osaSemaphoreId_t semId, uint32_t millisec)
{
#if osNumberOfSemaphores
  uint32_t timeoutTicks;

  if (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED)
  {
      return osaStatus_Success;
  }

  if(semId == NULL)
  {
    return osaStatus_Error;
  }
  semaphore_t sem = (semaphore_t)semId;

  /* Convert timeout from millisecond to tick. */
  if (millisec == osaWaitForever_c)
  {
    timeoutTicks = portMAX_DELAY;
  }
  else
  {
    timeoutTicks = MSEC_TO_TICK(millisec);
  }

  if (xSemaphoreTake(sem, timeoutTicks)==pdFALSE)
  {
    return osaStatus_Timeout; /* timeout */
  }
  else
  {
    return osaStatus_Success; /* semaphore taken */
  }
#else
  (void)semId;
  (void)millisec;
  return osaStatus_Error;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_SemaphorePost
 * Description   : This function is used to wake up one task that wating on the
 * semaphore. If no task is waiting, increase the semaphore. The function returns
 * osaStatus_Success if the semaphre is post successfully, otherwise returns
 * osaStatus_Error.
 *
 *END**************************************************************************/
osaStatus_t OSA_SemaphorePost(osaSemaphoreId_t semId)
{
#if osNumberOfSemaphores
  osaStatus_t status = osaStatus_Error;

  if (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED)
  {
      return osaStatus_Success;
  }

  if(semId)
  {
    semaphore_t sem = (semaphore_t)semId;

    if (__get_IPSR())
    {
      portBASE_TYPE taskToWake = pdFALSE;

      if (pdTRUE==xSemaphoreGiveFromISR(sem, &taskToWake))
      {
        if (pdTRUE == taskToWake)
        {
          portYIELD_FROM_ISR(taskToWake);
        }
        status = osaStatus_Success;
      }
      else
      {
        status = osaStatus_Error;
      }
    }
    else
    {
      if (pdTRUE == xSemaphoreGive(sem))
      {
        status = osaStatus_Success; /* sync object given */
      }
      else
      {
        status = osaStatus_Error;
      }
    }
  }
  return status;
#else
  (void)semId;
  return osaStatus_Error;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_MutexCreate
 * Description   : This function is used to create a mutex.
 * Return        : Mutex handle of the new mutex, or NULL if failed.
 *
 *END**************************************************************************/
osaMutexId_t OSA_MutexCreate(void)
{
#if osNumberOfMutexes
    mutex_t mutex;
    mutex = xSemaphoreCreateMutex();
    return (osaMutexId_t)mutex;
#else
    return NULL;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_MutexLock
 * Description   : This function checks the mutex's status, if it is unlocked,
 * lock it and returns osaStatus_Success, otherwise, wait for the mutex.
 * This function returns osaStatus_Success if the mutex is obtained, returns
 * osaStatus_Error if any errors occur during waiting. If the mutex has been
 * locked, pass 0 as timeout will return osaStatus_Timeout immediately.
 *
 *END**************************************************************************/
osaStatus_t OSA_MutexLock(osaMutexId_t mutexId, uint32_t millisec)
{
#if osNumberOfMutexes
    uint32_t timeoutTicks;
    mutex_t mutex = (mutex_t)mutexId;

    if (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED)
    {
        return osaStatus_Success;
    }

    if(mutexId == NULL)
    {
     return osaStatus_Error;
    }
    /* If pMutex has been locked by current task, return error. */
    if (xSemaphoreGetMutexHolder(mutex) == xTaskGetCurrentTaskHandle())
    {
        return osaStatus_Error;
    }

    /* Convert timeout from millisecond to tick. */
    if (millisec == osaWaitForever_c)
    {
        timeoutTicks = portMAX_DELAY;
    }
    else
    {
        timeoutTicks = MSEC_TO_TICK(millisec);
    }

    if (xSemaphoreTake(mutex, timeoutTicks)==pdFALSE)
    {
        return osaStatus_Timeout; /* timeout */
    }
    else
    {
        return osaStatus_Success; /* semaphore taken */
    }
#else
    (void)mutexId;
    (void)millisec;
    return osaStatus_Error;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_MutexUnlock
 * Description   : This function is used to unlock a mutex.
 *
 *END**************************************************************************/
osaStatus_t OSA_MutexUnlock(osaMutexId_t mutexId)
{
#if osNumberOfMutexes
  mutex_t mutex = (mutex_t)mutexId;

  if (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED)
  {
      return osaStatus_Success;
  }

  if(mutexId == NULL)
  {
    return osaStatus_Error;
  }
  /* If pMutex is not locked by current task, return error. */
  if (xSemaphoreGetMutexHolder(mutex) != xTaskGetCurrentTaskHandle())
  {
    return osaStatus_Error;
  }

  if (xSemaphoreGive(mutex)==pdPASS)
  {
    return osaStatus_Success;
  }
  else
  {
    return osaStatus_Error;
  }
#else
  (void)mutexId;
  return osaStatus_Error;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_MutexDestroy
 * Description   : This function is used to destroy a mutex.
 * Return        : osaStatus_Success if the lock object is destroyed successfully, otherwise return osaStatus_Error.
 *
 *END**************************************************************************/
osaStatus_t OSA_MutexDestroy(osaMutexId_t mutexId)
{
#if osNumberOfMutexes
  mutex_t mutex = (mutex_t)mutexId;
  if(mutexId == NULL)
  {
    return osaStatus_Error;
  }
  vSemaphoreDelete(mutex);
  return osaStatus_Success;
#else
  (void)mutexId;
  return osaStatus_Error;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_EventCreate
 * Description   : This function is used to create a event object.
 * Return        : Event handle of the new event, or NULL if failed.
 *
 *END**************************************************************************/
osaEventId_t OSA_EventCreate(bool_t autoClear)
{
#if osNumberOfEvents
  osaEventId_t eventId;
  osEventStruct_t* pEventStruct;
  OSA_InterruptDisable();
  eventId = pEventStruct = osObjectAlloc(&osEventInfo);
  OSA_InterruptEnable();
  if(eventId == NULL)
  {
    return NULL;
  }

  pEventStruct->event.eventHandler = xEventGroupCreate();
  if (pEventStruct->event.eventHandler)
  {
    pEventStruct->event.autoClear = autoClear;
  }
  else
  {
    OSA_InterruptDisable();
    osObjectFree(&osEventInfo, eventId);
    OSA_InterruptEnable();
    eventId = NULL;
  }
  return eventId;
#else
  (void)autoClear;
  return NULL;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_EventSet
 * Description   : Set one or more event flags of an event object.
 * Return        : osaStatus_Success if set successfully, osaStatus_Error if failed.
 *
 *END**************************************************************************/
osaStatus_t OSA_EventSet(osaEventId_t eventId, osaEventFlags_t flagsToSet)
{
#if osNumberOfEvents
  osEventStruct_t* pEventStruct;
  portBASE_TYPE taskToWake = pdFALSE;
  if(osObjectIsAllocated(&osEventInfo, eventId) == FALSE)
  {
    return osaStatus_Error;
  }
  pEventStruct = (osEventStruct_t*)eventId;
  if(pEventStruct->event.eventHandler == NULL)
  {
    return osaStatus_Error;
  }
  if (__get_IPSR())
  {
    if (pdPASS != xEventGroupSetBitsFromISR(pEventStruct->event.eventHandler, (event_flags_t)flagsToSet, &taskToWake))
    {
      panic(0,(uint32_t)OSA_EventSet,0,0);
      return osaStatus_Error;
    }
    if (pdTRUE == taskToWake)
    {
      portYIELD_FROM_ISR(taskToWake);
    }
  }
  else
  {
    xEventGroupSetBits(pEventStruct->event.eventHandler, (event_flags_t)flagsToSet);
  }
  return osaStatus_Success;
#else
  (void)eventId;
  (void)flagsToSet;
  return osaStatus_Error;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_EventClear
 * Description   : Clear one or more event flags of an event object.
 * Return        :osaStatus_Success if clear successfully, osaStatus_Error if failed.
 *
 *END**************************************************************************/
osaStatus_t OSA_EventClear(osaEventId_t eventId, osaEventFlags_t flagsToClear)
{
#if osNumberOfEvents

  osEventStruct_t* pEventStruct;
  if(osObjectIsAllocated(&osEventInfo, eventId) == FALSE)
  {
    return osaStatus_Error;
  }
  pEventStruct = (osEventStruct_t*)eventId;
  if(pEventStruct->event.eventHandler == NULL)
  {
    return osaStatus_Error;
  }

  if (__get_IPSR())
  {
    xEventGroupClearBitsFromISR(pEventStruct->event.eventHandler, (event_flags_t)flagsToClear);
  }
  else
  {
    xEventGroupClearBits(pEventStruct->event.eventHandler, (event_flags_t)flagsToClear);
  }

  return osaStatus_Success;
#else
  (void)eventId;
  (void)flagsToClear;
  return osaStatus_Error;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_EventWait
 * Description   : This function checks the event's status, if it meets the wait
 * condition, return osaStatus_Success, otherwise, timeout will be used for
 * wait. The parameter timeout indicates how long should wait in milliseconds.
 * Pass osaWaitForever_c to wait indefinitely, pass 0 will return the value
 * osaStatus_Timeout immediately if wait condition is not met. The event flags
 * will be cleared if the event is auto clear mode. Flags that wakeup waiting
 * task could be obtained from the parameter setFlags.
 * This function returns osaStatus_Success if wait condition is met, returns
 * osaStatus_Timeout if wait condition is not met within the specified
 * 'timeout', returns osaStatus_Error if any errors occur during waiting.
 *
 *END**************************************************************************/
osaStatus_t OSA_EventWait(osaEventId_t eventId, osaEventFlags_t flagsToWait, bool_t waitAll, uint32_t millisec, osaEventFlags_t *pSetFlags)
{
#if osNumberOfEvents
  osEventStruct_t* pEventStruct;
  BaseType_t clearMode;
  uint32_t timeoutTicks;
  event_flags_t flagsSave;
  if(osObjectIsAllocated(&osEventInfo, eventId) == FALSE)
  {
    return osaStatus_Error;
  }

  /* Clean FreeRTOS cotrol flags */
  flagsToWait = flagsToWait & 0x00FFFFFF;

  pEventStruct = (osEventStruct_t*)eventId;
  if(pEventStruct->event.eventHandler == NULL)
  {
    return osaStatus_Error;
  }

  /* Convert timeout from millisecond to tick. */
  if (millisec == osaWaitForever_c)
  {
    timeoutTicks = portMAX_DELAY;
  }
  else
  {
    timeoutTicks = millisec/portTICK_PERIOD_MS;
  }

  clearMode = (pEventStruct->event.autoClear) ? pdTRUE: pdFALSE;

  flagsSave = xEventGroupWaitBits(pEventStruct->event.eventHandler,(event_flags_t)flagsToWait,clearMode,(BaseType_t)waitAll,timeoutTicks);

  flagsSave &= (event_flags_t)flagsToWait;
  if(pSetFlags)
  {
    *pSetFlags = (osaEventFlags_t)flagsSave;
  }

  if (flagsSave)
  {
    return osaStatus_Success;
  }
  else
  {
    return osaStatus_Timeout;
  }
#else
  (void)eventId;
  (void)flagsToWait;
  (void)waitAll;
  (void)millisec;
  (void)pSetFlags;
  return osaStatus_Error;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_EventDestroy
 * Description   : This function is used to destroy a event object. Return
 * osaStatus_Success if the event object is destroyed successfully, otherwise
 * return osaStatus_Error.
 *
 *END**************************************************************************/
osaStatus_t OSA_EventDestroy(osaEventId_t eventId)
{
#if osNumberOfEvents
  osEventStruct_t* pEventStruct;
  if(osObjectIsAllocated(&osEventInfo, eventId) == FALSE)
  {
    return osaStatus_Error;
  }
  pEventStruct = (osEventStruct_t*)eventId;
  if(pEventStruct->event.eventHandler == NULL)
  {
    return osaStatus_Error;
  }
  vEventGroupDelete(pEventStruct->event.eventHandler);
  OSA_InterruptDisable();
  osObjectFree(&osEventInfo, eventId);
  OSA_InterruptEnable();
  return osaStatus_Success;
#else
  (void)eventId;
  return osaStatus_Error;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_MsgQCreate
 * Description   : This function is used to create a message queue.
 * Return        : the handle to the message queue if create successfully, otherwise
 * return NULL.
 *
 *END**************************************************************************/
osaMsgQId_t OSA_MsgQCreate( uint32_t  msgNo )
{
#if osNumberOfMessageQs
    msg_queue_handler_t msg_queue_handler;

    /* Create the message queue where each element is a pointer to the message item. */
    msg_queue_handler = xQueueCreate(msgNo,sizeof(osaMsg_t));
    return (osaMsgQId_t)msg_queue_handler;
#else
    (void)msgNo;
    return NULL;
#endif
}


/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_MsgQPut
 * Description   : This function is used to put a message to a message queue.
* Return         : osaStatus_Success if the message is put successfully, otherwise return osaStatus_Error.
 *
 *END**************************************************************************/
osaStatus_t OSA_MsgQPut(osaMsgQId_t msgQId, void* pMessage)
{
#if osNumberOfMessageQs
  msg_queue_handler_t handler;
  osaStatus_t osaStatus;
  if(msgQId == NULL)
  {
    return osaStatus_Error;
  }
  handler = (msg_queue_handler_t)msgQId;
  {
    if (__get_IPSR())
    {
      portBASE_TYPE taskToWake = pdFALSE;

      if (pdTRUE == xQueueSendToBackFromISR(handler, pMessage, &taskToWake))
      {
        if (pdTRUE == taskToWake)
        {
          portYIELD_FROM_ISR(taskToWake);
        }
        osaStatus = osaStatus_Success;
      }
      else
      {
        osaStatus =  osaStatus_Error;
      }

    }
    else
    {
      osaStatus = (xQueueSendToBack(handler, pMessage, 0)== pdPASS)?(osaStatus_Success):(osaStatus_Error);
    }
  }
  return osaStatus;
#else
  (void)msgQId;
  (void)pMessage;
  return osaStatus_Error;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_MsgQGet
 * Description   : This function checks the queue's status, if it is not empty,
 * get message from it and return osaStatus_Success, otherwise, timeout will
 * be used for wait. The parameter timeout indicates how long should wait in
 * milliseconds. Pass osaWaitForever_c to wait indefinitely, pass 0 will return
 * osaStatus_Timeout immediately if queue is empty.
 * This function returns osaStatus_Success if message is got successfully,
 * returns osaStatus_Timeout if message queue is empty within the specified
 * 'timeout', returns osaStatus_Error if any errors occur during waiting.
 *
 *END**************************************************************************/
osaStatus_t OSA_MsgQGet(osaMsgQId_t msgQId, void *pMessage, uint32_t millisec)
{
#if osNumberOfMessageQs
  osaStatus_t osaStatus;
  msg_queue_handler_t handler;
  uint32_t timeoutTicks;
  if( msgQId == NULL )
  {
    return osaStatus_Error;
  }
  handler = (msg_queue_handler_t)msgQId;
  if (millisec == osaWaitForever_c)
  {
    timeoutTicks = portMAX_DELAY;
  }
  else
  {
    timeoutTicks = MSEC_TO_TICK(millisec);
  }
  if (xQueueReceive(handler, pMessage, timeoutTicks)!=pdPASS)
  {
    osaStatus =  osaStatus_Timeout; /* not able to send it to the queue? */
  }
  else
  {
    osaStatus = osaStatus_Success;
  }
  return osaStatus;
#else
  (void)msgQId;
  (void)pMessage;
  (void)millisec;
  return osaStatus_Error;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_MsgQDestroy
 * Description   : This function is used to destroy the message queue.
 * Return        : osaStatus_Success if the message queue is destroyed successfully, otherwise return osaStatus_Error.
 *
 *END**************************************************************************/
osaStatus_t OSA_MsgQDestroy(osaMsgQId_t msgQId)
{
#if osNumberOfMessageQs
  msg_queue_handler_t handler;
  if(msgQId == NULL )
  {
    return osaStatus_Error;
  }
  handler = (msg_queue_handler_t)msgQId;
  vQueueDelete(handler);
  return osaStatus_Success;
#else
  (void)msgQId;
  return osaStatus_Error;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_InterruptEnable
 * Description   : self explanatory.
 *
 *END**************************************************************************/
void OSA_InterruptEnable(void)
{
#if OsaBasePri_d
  if (__get_IPSR())
  {
    if(g_base_priority_top)
    {
    g_base_priority_top--;
    portCLEAR_INTERRUPT_MASK_FROM_ISR(g_base_priority_array[g_base_priority_top]);
    }
  }
  else
  {
    portEXIT_CRITICAL();
  }
#else
  OSA_EnableIRQGlobal();
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_InterruptDisable
 * Description   : self explanatory.
 *
 *END**************************************************************************/
void OSA_InterruptDisable(void)
{
#if OsaBasePri_d
  if (__get_IPSR())
  {
    if(g_base_priority_top < OSA_MAX_ISR_CRITICAL_SECTION_DEPTH)
    {
      g_base_priority_array[g_base_priority_top] = portSET_INTERRUPT_MASK_FROM_ISR();
      g_base_priority_top++;
    }
  }
  else
  {
    portENTER_CRITICAL();
  }
#else
  OSA_DisableIRQGlobal();
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_InterruptEnableRestricted
 * Description   : Disable interrupts except high-priority ones
 *
 *END**************************************************************************/
void OSA_InterruptEnableRestricted(uint32_t *pu8OldIntLevel)
{
    /* Disable interrupts for duration of this function */
    OSA_DisableIRQGlobal();

    /* Store old priority level */
    *pu8OldIntLevel = __get_BASEPRI();

    /* Update priority level, but only if it is a more restrictive value */
    __set_BASEPRI_MAX((3U << (8U - __NVIC_PRIO_BITS)) & 0xffU);

    /* Restore interrupts */
    OSA_EnableIRQGlobal();
}
/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_InterruptEnableRestore
 * Description   : Restore interrupts that were previously restricted by a call
 *                 to OSA_InterruptEnableRestricted
 *
 *END**************************************************************************/
void OSA_InterruptEnableRestore(uint32_t *pu8OldIntLevel)
{
    // write value direct into register ARM to ARM, no translations required
    __set_BASEPRI(*pu8OldIntLevel);
}


uint32_t gInterruptDisableCount = 0;
/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_EnableIRQGlobal
 * Description   : enable interrupts using PRIMASK register.
 *
 *END**************************************************************************/
void OSA_EnableIRQGlobal(void)
{
    if (gInterruptDisableCount > 0)
    {
        gInterruptDisableCount--;

        if (gInterruptDisableCount == 0)
        {
            __enable_irq();
        }
        /* call core API to enable the global interrupt*/
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_DisableIRQGlobal
 * Description   : disable interrupts using PRIMASK register.
 *
 *END**************************************************************************/
void OSA_DisableIRQGlobal(void)
{
    /* call core API to disable the global interrupt*/
    __disable_irq();

    /* update counter*/
    gInterruptDisableCount++;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_InstallIntHandler
 * Description   : This function is used to install interrupt handler.
 *
 *END**************************************************************************/
void OSA_InstallIntHandler(uint32_t IRQNumber, void (*handler)(void))
{

#if defined ( __IAR_SYSTEMS_ICC__ )
    _Pragma ("diag_suppress = Pm138")
#endif
    InstallIRQHandler((IRQn_Type)IRQNumber, (uint32_t)handler);
#if defined ( __IAR_SYSTEMS_ICC__ )
    _Pragma ("diag_remark = PM138")
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_TimeInit
 * Description   : Empty
 *
 *END**************************************************************************/
void OSA_TimeInit(void)
{

}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_InIsrContext
 * Description   : self explanatory.
 *
 *END**************************************************************************/
bool_t OSA_InIsrContext(void)
{
    return __get_IPSR();
}

/*! *********************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
********************************************************************************** */
#ifndef FSL_OSA_MAIN_FUNC_DISABLE
OSA_TASK_DEFINE(startup_task, gMainThreadPriority_c, 1, gMainThreadStackSize_c, 0)  ;
int main (void)
{
    /* Initialize MCU clock */
    hardware_init();
    OSA_TaskCreate(OSA_TASK(startup_task), NULL);
    vTaskStartScheduler();

    return 0;
}
#endif


/*! *********************************************************************************
* \brief     Allocates a osObjectStruct_t block in the osObjectHeap array.
* \param[in] pointer to the object info struct.
* Object can be semaphore, mutex, message Queue, event
* \return Pointer to the allocated osObjectStruct_t, NULL if failed.
*
* \pre
*
* \post
*
* \remarks Function is unprotected from interrupts.
*
********************************************************************************** */
#if osObjectAlloc_c
static void* osObjectAlloc(const osObjectInfo_t* pOsObjectInfo)
{
    uint32_t i;
    uint8_t* pObj = (uint8_t*)pOsObjectInfo->pHeap;
    for( i=0 ; i < pOsObjectInfo->objNo ; i++, pObj += pOsObjectInfo->objectStructSize)
    {
        if(((osObjStruct_t*)pObj)->inUse == 0)
        {
            ((osObjStruct_t*)pObj)->inUse = 1;
            return (void*)pObj;
        }
    }
    return NULL;
}
#endif

/*! *********************************************************************************
* \brief     Verifies the object is valid and allocated in the osObjectHeap array.
* \param[in] the pointer to the object info struct.
* \param[in] the pointer to the object struct.
* Object can be semaphore, mutex,  message Queue, event
* \return TRUE if the object is valid and allocated, FALSE otherwise
*
* \pre
*
* \post
*
* \remarks Function is unprotected from interrupts.
*
********************************************************************************** */
#if osObjectAlloc_c
static bool_t osObjectIsAllocated(const osObjectInfo_t* pOsObjectInfo, void* pObjectStruct)
{
    uint32_t i;
    uint8_t* pObj = (uint8_t*)pOsObjectInfo->pHeap;
    for( i=0 ; i < pOsObjectInfo->objNo ; i++ , pObj += pOsObjectInfo->objectStructSize)
    {
        if(pObj == pObjectStruct)
        {
            if(((osObjStruct_t*)pObj)->inUse)
            {
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}
#endif

/*! *********************************************************************************
* \brief     Frees an osObjectStruct_t block from the osObjectHeap array.
* \param[in] pointer to the object info struct.
* \param[in] Pointer to the allocated osObjectStruct_t to free.
* Object can be semaphore, mutex, message Queue, event
* \return none.
*
* \pre
*
* \post
*
* \remarks Function is unprotected from interrupts.
*
********************************************************************************** */
#if osObjectAlloc_c
static void osObjectFree(const osObjectInfo_t* pOsObjectInfo, void* pObjectStruct)
{
    uint32_t i;
    uint8_t* pObj = (uint8_t*)pOsObjectInfo->pHeap;
    for( i=0; i < pOsObjectInfo->objNo; i++, pObj += pOsObjectInfo->objectStructSize )
    {
        if(pObj == pObjectStruct)
        {
            ((osObjStruct_t*)pObj)->inUse = 0;
            break;
        }
    }
}
#endif

/*! *********************************************************************************
* \brief    FreeRTOS application malloc failed hook
*
*
* \remarks  Function is called by FreeRTOS if there is not enough space in the
*           heap for task stack allocation or for OS object allocation
*
********************************************************************************** */
#if (configUSE_MALLOC_FAILED_HOOK==1)
void vApplicationMallocFailedHook (void)
{
    panic(0,(uint32_t)vApplicationMallocFailedHook,0,0);
}
#endif

#if (configSUPPORT_STACK_UNRETAINED == 1)

#define TOP_UNRETAINED_STACK   ((uint32_t*)0x04004000)
#define ROUND_NB_WORDS(x)      (((x) + sizeof(uint32_t)-1)/sizeof(uint32_t))

#if defined(__IAR_SYSTEMS_ICC__)
#pragma location = ".rtos.stacks"
uint32_t rtos_stacks[4];
static uint32_t *unretained_stack_alloc = &rtos_stacks[0];

#elif defined(__GNUC__) || defined (__CC_ARM)
extern uint32_t      __rtos_stacks_base;
extern uint32_t      __top_RAM_Bank0;

uint32_t rtos_stacks[4] __attribute__ ((section (".rtos.stacks")));
static uint32_t *unretained_stack_alloc = &rtos_stacks[0];
#endif


/*! *********************************************************************************
* \brief    Application Stack allocator
*
* \param[in] size of stack to allocate in number of StackType_t (uint32_t).
* \param[in] is_idle_task boolean argument telling if the stack to allocate is that of the Idle task.
*            A special case is handled for IDLE task because it must be saved  entirely, so is allocated
*            from the standard heap.
* \return   Pointer on the start of stack (stack base).
* \remarks  FreeRTOS stacks are allocated in a a potentially unretained RAM bank.
*
********************************************************************************** */
StackType_t * pvApplicationGetTaskStackMemory( size_t TaskStackSize, uint8_t is_idle_task )
{
    uint32_t * stack_base= NULL;
    do {
        uint32_t * ptr;

        if (is_idle_task != 0)
        {
            stack_base = MEM_BufferAllocWithId(TaskStackSize*sizeof(StackType_t), 0u);
            break;
        }
        ptr = unretained_stack_alloc + ROUND_NB_WORDS(TaskStackSize);
        if (ptr > TOP_UNRETAINED_STACK)
        {
            stack_base = NULL;
            break;
        }
        stack_base = unretained_stack_alloc;
        unretained_stack_alloc = ptr;
    } while (0);
    return (StackType_t*)stack_base;
}

/*
 * Find a free slot in the compressed_task_stack table
 */
static compressed_task_stack_t * AllocateTaskHandle(void)
{
    compressed_task_stack_t * hdl = NULL;

    if (nb_tasks_created < MAX_TASK_NB)
    {
#if INCLUDE_vTaskDelete /* vTaskDelete() enabled */
        /* if OSA_TaskDestroy is never used, could be simpler */
        for (uint8_t i = 0; i < MAX_TASK_NB; i++)
        {
            /* Find a free slot */
            if (task_tb[i].tsk_hdl == NULL)
            {
                hdl = &task_tb[i];
                break;
            }
        }
#else
        hdl = &task_tb[nb_tasks_created];
#endif
        nb_tasks_created ++;
    }
    return hdl;
}
#ifdef PWR_StackCompressDBG
static uint32_t pd_cycles = 0;
static uint32_t wu_cycles = 0;
#endif
void vApplicationRegisterNewTaskHandleAndStack(void *tsk_hdl,
                                               StackType_t *stack_base,
                                               size_t stack_word_depth)
{
    compressed_task_stack_t * hdl = NULL;
    do {
        hdl = AllocateTaskHandle();
        if (hdl == NULL)
        {
            assert(hdl != NULL);
            break;
        }

        hdl->tsk_hdl = tsk_hdl;
        hdl->stack_top = stack_base + stack_word_depth;
    } while (0);

}

#if (DmaMemMove_d != 0)

#define DMA_XFER_DONE 0xf98ed76c
#define MEM2MEMXFER(_nb_) DMA_CHANNEL_XFER(true, false, false, false, sizeof(uint32_t), kDMA_AddressInterleave1xWidth, kDMA_AddressInterleave1xWidth, (_nb_))
static const uint32_t xfer_done_pattern = DMA_XFER_DONE;
volatile static uint32_t xfer_completed[1];
void OSA_WaitForStackRestoration(void)
{
    while (xfer_completed[0] != xfer_done_pattern)
    {
    }
}
#endif

void OSA_LowPowerCompressStackToRetainedLocation(void)
{
    uint8_t i;
    compressed_task_stack_t * hdl;
    const uint32_t * srcAddr;
    uint32_t       *dstAddr;
    uint32_t retained_sz = 0;
#ifdef PWR_StackCompressDBG
    DEBUG_DWT_CYCLE_CNT_START()
    pd_cycles = 0;
#endif
    pd_cnt++;
#if (DmaMemMove_d != 0)
    uint8_t nb_dma_desc = 0;
    dma_descriptor_t *prev_dma_desc = NULL;
#endif
    PWR_DBG_LOG("NbTasks=%d PD cnt=%d", nb_tasks_created, pd_cnt);

    for (i = 0; i<nb_tasks_created;i++)
    {
       hdl = &task_tb[i];
#if INCLUDE_vTaskDelete
       if (hdl->tsk_hdl == NULL)
           continue;
#endif
       /* get point to which stack has currently receded in task */
       /* known to be first word of TCB */
       uint32_t stack_used_limit = *(uint32_t*)hdl->tsk_hdl;
       srcAddr = (const uint32_t*)stack_used_limit;
       /* Need to retain space between stack_used_limit and stack top i.e stack base + stack size */
       retained_sz = (uint32_t)hdl->stack_top - stack_used_limit;
       if (hdl->retained_stk != NULL)
       {
           if (hdl->retained_sz < retained_sz)
           {
               /* keep same buffer otherwise free current one and allocate new storage */
               MEM_BufferFree(hdl->retained_stk);
               hdl->retained_stk = NULL;
           }
       }
       if (hdl->retained_stk == NULL)
       {
           hdl->retained_stk = MEM_BufferAllocWithId(retained_sz, 0u);
           if (hdl->retained_stk == NULL)
           {
               assert(0);
           }
           hdl->retained_sz = retained_sz;
       }
       dstAddr = hdl->retained_stk;
#if (DmaMemMove_d != 0)
        if (nb_dma_desc == 0)
        {
            DMA_Init(DMA0);
            DMA_EnableChannel(DMA0, 0);
            DMA_CreateHandle(&g_DMA_memcpy_handle, DMA0, DMA_MEMCPY_CHANNEL);
            /* When we want to power down we do not want an IRQ to fire and wake the MCU up again */
            DMA_DisableChannelInterrupts(DMA0, DMA_MEMCPY_CHANNEL);
            DMA_SetupDescriptor(
                    &dma_desc[nb_dma_desc],
                    MEM2MEMXFER(4u),
                    &xfer_done_pattern,
                    (void*)xfer_completed,
                    NULL); /* Chain DMA transfers backwards */
            prev_dma_desc = &dma_desc[nb_dma_desc];
            nb_dma_desc ++;
        }
        DMA_SetupDescriptor(
                &dma_desc[nb_dma_desc],
                MEM2MEMXFER(retained_sz),
                srcAddr,
                dstAddr,
                prev_dma_desc); /* Chain DMA transfers backwards */
        stack_words_copied += retained_sz/sizeof(uint32_t);
        PWR_DBG_LOG("stack=0x%x->0x%x sz=%d", srcAddr, dstAddr, hdl->retained_sz);

        prev_dma_desc = &dma_desc[nb_dma_desc];
        nb_dma_desc ++;
#else
        FLib_MemCpyAligned32bit(dstAddr, (const uint8_t*)srcAddr, hdl->retained_sz);

#endif
#ifdef StackCorruptionDetect_d
        SHA256_Hash((const uint8_t*)srcAddr, hdl->retained_sz, hdl->stack_digest);
#endif
    }
#if (DmaMemMove_d != 0)
    if (nb_dma_desc > 0)
    {
        DMA_SubmitChannelDescriptor(&g_DMA_memcpy_handle, &dma_desc[nb_dma_desc-1]);
        DMA_StartTransfer(&g_DMA_memcpy_handle);
        OSA_WaitForStackRestoration();
    }

#endif
#ifdef PWR_StackCompressDBG
    DEBUG_DWT_CYCLE_CNT_GET(pd_cycles);
#endif


}

void OSA_LowPowerRestoreStacksToActualLocation(void)
{
    uint8_t i;
    compressed_task_stack_t * hdl;
    uint32_t stack_used_limit;
    const uint32_t * srcAddr;
    uint32_t       *dstAddr;
#if (DmaMemMove_d != 0)
    uint8_t nb_dma_desc = 0;
#endif
#if (DmaMemMove_d != 0)
    dma_descriptor_t *prev_dma_desc = NULL;
#endif
    PWR_DBG_LOG("WU cnt=%d stack_words_copied=%d", wu_cnt, stack_words_copied);

    if (stack_words_copied != 0u)
    {
    	stack_words_restored = 0u;
        wu_cnt++;
        if (wu_cnt != pd_cnt)
        {
            assert(0);
        }
#ifdef PWR_StackCompressDBG
        DEBUG_DWT_CYCLE_CNT_START()
        wu_cycles = 0;
#endif
        for (i = 0; i<nb_tasks_created;i++)
        {
            hdl = &task_tb[i];
#if INCLUDE_vTaskDelete /* vTaskDelete() enabled */
            if (hdl->tsk_hdl == NULL)
                continue;
#endif
            stack_used_limit = *(uint32_t*)hdl->tsk_hdl; /* points to pxTopOfStack */
            dstAddr = (uint32_t*)stack_used_limit;
            srcAddr = hdl->retained_stk;

#if (DmaMemMove_d != 0)
            if (nb_dma_desc == 0)
            {
                DMA_Init(DMA0);
                DMA_EnableChannel(DMA0, 0);
                DMA_CreateHandle(&g_DMA_memcpy_handle, DMA0, DMA_MEMCPY_CHANNEL);
                /* When we want to power down we do not want an IRQ to fire and wake the MCU up again */
                DMA_DisableChannelInterrupts(DMA0, DMA_MEMCPY_CHANNEL);
                xfer_completed[0] = ~xfer_done_pattern;
                DMA_SetupDescriptor(&dma_desc[nb_dma_desc],
                					MEM2MEMXFER(4u),
									&xfer_done_pattern,
									(void*)xfer_completed,
									NULL); /* Chain DMA transfers backwards */
                prev_dma_desc = &dma_desc[nb_dma_desc];
                nb_dma_desc ++;
            }
            DMA_SetupDescriptor(&dma_desc[nb_dma_desc],
            					MEM2MEMXFER(hdl->retained_sz),
								srcAddr,
								dstAddr,
								prev_dma_desc); /* Chain DMA transfers backwards */
            PWR_DBG_LOG("stack=0x%x<-0x%x sz=%d",  dstAddr, srcAddr, hdl->retained_sz);
            prev_dma_desc = &dma_desc[nb_dma_desc];
            nb_dma_desc ++;
#else
            FLib_MemCpyAligned32bit(dstAddr, srcAddr, hdl->retained_sz);
#endif
            stack_words_restored += hdl->retained_sz/sizeof(uint32_t);

        }
        assert(stack_words_copied == stack_words_restored);
        stack_words_copied = 0;
#if (DmaMemMove_d != 0)
        DMA_SubmitChannelDescriptor(&g_DMA_memcpy_handle, &dma_desc[nb_dma_desc-1]);
        DMA_StartTransfer(&g_DMA_memcpy_handle);
        OSA_WaitForStackRestoration();
#endif

#ifdef StackCorruptionDetect_d
        uint8_t digest[SHA256_HASH_SIZE];
        for (i = 0; i<nb_tasks_created;i++)
        {
           hdl = &task_tb[i];
#if INCLUDE_vTaskDelete /* vTaskDelete() enabled */
           if (hdl->tsk_hdl == NULL)
               continue;
#endif
           stack_used_limit = *(uint32_t*)hdl->tsk_hdl; /* points to pxTopOfStack */
           dstAddr = (uint32_t*)stack_used_limit;
           srcAddr = hdl->retained_stk;
           SHA256_Hash((const uint8_t*)dstAddr, hdl->retained_sz, digest);
           if (!FLib_MemCmp(digest, hdl->stack_digest, SHA256_HASH_SIZE))
           {
        	   assert(0);
           }
        }
#endif
#ifdef PWR_StackCompressDBG
        DEBUG_DWT_CYCLE_CNT_GET(wu_cycles);
        PWR_DBG_LOG("WU stack cycles=%d", wu_cycles);
#endif
    }
    else
    {
#ifdef PWR_StackCompressDBG
    	/* If stack retention is supposed to be on it is ok, otherwise it denotes of an error */
    	uint32_t stack_retention = PWR_u32GetRamRetention() & 0x00000001u;
        if (!stack_retention)
        {
    		DbgLogDump(true);
    	}
        if ( (*(uint32_t*)0x04000800 != 0x12345678) ||
        		(*(uint32_t*)0x04000804 != 0x9abcdef0))
        {
        	assert(0);
        }
#endif
    }
}


#endif
