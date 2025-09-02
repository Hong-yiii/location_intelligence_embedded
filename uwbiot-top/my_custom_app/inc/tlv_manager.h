#ifndef TLV_MANAGER_H
#define TLV_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

// TLV message types
typedef enum {
    kMsg_Initialize_iOS     = 0x0A,
    kMsg_Initialize_Android = 0xA5,
    kMsg_ConfigureAndStart  = 0x0B,
    kMsg_Stop              = 0x0C
} MessageId_t;

// TLV response types
typedef enum {
    kRsp_AccessoryConfigurationData = 0x01,
    kRsp_AccessoryUwbDidStart      = 0x02,
    kRsp_AccessoryUwbDidStop       = 0x03
} ResponseId_t;

// Interface types
typedef enum {
    UWB_HIF_BLE,
    UWB_HIF_UART
} UWB_Hif_t;

// TLV manager functions
bool tlvMngInit(void);
bool tlvBuilderInit(void);
void tlvManagerDeinit(void);

// Getter functions for internal resources
void* tlvGetMutex(void);

// TLV communication functions
bool tlvSendRaw(uint8_t deviceId, uint8_t* buf, uint16_t size);
void tlvSendDoneCb(void);
void tlvRecv(uint8_t deviceId, UWB_Hif_t interface, uint8_t* tlv, uint8_t tlvSize);

// Session management functions
bool handleDeviceInit(void);
bool handleStopSession(uint8_t deviceId);
void handleDisconnection(uint8_t deviceId);

#endif // TLV_MANAGER_H
