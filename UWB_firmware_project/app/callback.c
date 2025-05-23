#include "UwbApi.h"
#include <stdio.h>

void MyUwbCallback(eNotificationType opType, void *pData) {
    switch (opType) {
        case UWB_EVENT_SESSION_STARTED:
            printf("UWB session started!\n");
            break;
        case UWB_EVENT_RANGING_RESULT:
            printf("Ranging result received.\n");
            // Cast pData to the appropriate struct and process
            break;
        case UWB_EVENT_SESSION_ENDED:
            printf("UWB session ended.\n");
            break;
        default:
            printf("Other UWB event: %d\n", opType);
            break;
    }
} 