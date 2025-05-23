#include "UwbApi.h"
#include "callback.h"

int main(void) {
    // Initialize board, clocks, peripherals here if needed

    // Initialize UWB stack with application callback
    UwbApi_Init(MyUwbCallback);

    // Main loop
    while (1) {
        // Application logic or low-power wait
    }

    // Shutdown UWB stack on exit (not reached in this example)
    UwbApi_ShutDown();
    return 0;
} 