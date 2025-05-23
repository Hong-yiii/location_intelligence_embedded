/* my_custom_app/src/my_app_main.c */
#include "phUwb_BuildConfig.h" // Should be one of the first
#include "UWBIOT_APP_BUILD.h"  // To get your UWBIOT_APP_BUILD__MY_CUSTOM_APP define

#ifdef UWBIOT_APP_BUILD__MY_CUSTOM_APP // Guard your entire custom application logic

#include "UwbApi.h"
#include <AppInternal.h> // For UWBIOT_EXAMPLE_END, PRINT_APP_NAME etc.
#include "phOsalUwb.h"
// Add other necessary headers for your application (drivers, custom logic headers, etc.)
// e.g., #include "my_custom_logic.h"

#define MY_CUSTOM_APP_TASK_NAME "MyCustomAppTask"
#define MY_CUSTOM_APP_TASK_STACK_SIZE 2048 // Adjust as needed
#define MY_CUSTOM_APP_TASK_PRIO 4        // Adjust as needed

OSAL_TASK_RETURN_TYPE MyCustomApp_Task(void *args)
{
    PRINT_APP_NAME("My Custom UWB Application");
    // Suppress unused parameter warning if args is not used
    (void)args;

    tUWBAPI_STATUS status = UWBAPI_STATUS_FAILED;

    // --- Your Custom Application Initialization ---
    // E.g., Initialize UWB stack, peripherals, BLE (if used)
    // status = UwbApi_Init(&AppCallback); // Example
    // if (status != UWBAPI_STATUS_SUCCESS) {
    //     NXPLOG_APP_E("MyCustomApp_Task: UwbApi_Init failed");
    //     goto end;
    // }
    // ... more init steps ...

    NXPLOG_APP_I("My Custom Application Initialized Successfully.");
    status = UWBAPI_STATUS_SUCCESS;

    // --- Your Custom Application Main Loop ---
    while (1)
    {
        // Your application logic here
        // E.g., Start ranging, handle events, process data

        // Delay or wait for events
        phOsalUwb_Delay(1000); // Example: 1 second delay
    }

// end: // Optional label for cleanup if needed
    UWBIOT_EXAMPLE_END(status); // Macro to signify end, might print status
    return OSAL_TASK_RETURN_PARAM; // Or appropriate return for your OSAL
}

UWBOSAL_TASK_HANDLE uwb_demo_start(void)
{
    phOsalUwb_ThreadCreationParams_t threadparams;
    UWBOSAL_TASK_HANDLE taskHandle = NULL;
    int create_status = 0;

    threadparams.stackdepth = MY_CUSTOM_APP_TASK_STACK_SIZE;
    PHOSALUWB_SET_TASKNAME(threadparams, MY_CUSTOM_APP_TASK_NAME);
    threadparams.pContext = NULL;
    threadparams.priority = MY_CUSTOM_APP_TASK_PRIO;

    create_status = phOsalUwb_Thread_Create((void **)&taskHandle, &MyCustomApp_Task, &threadparams);

    if (0 != create_status) {
        NXPLOG_APP_E("Failed to create task %s", threadparams.taskname);
        return NULL; // Or handle error appropriately
    }
    return taskHandle;
}

#endif /* UWBIOT_APP_BUILD__MY_CUSTOM_APP */ 