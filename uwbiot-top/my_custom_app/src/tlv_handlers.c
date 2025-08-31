#include "tlv_manager.h"
#include "iphone_adapter.h"
#include "session_manager.h"
#include "resource_manager.h"
#include "UwbApi.h"
#include "phOsalUwb.h"
#include "phNxpLogApis_App.h"
#include <string.h>

// Maximum sessions supported (1 iPhone + 4 boards)
#define MAX_SESSIONS 5
#define MAX_BOARD_SESSIONS 4
#define IPHONE_SESSION_SLOT 0  // iPhone always gets slot 0 (highest priority)

// TLV configuration
#define TLV_HEADER_SIZE 3  // Tag(1) + Length(2)
#define SHAREABLE_DATA_LENGTH_OFFSET 5
#define SHAREABLE_DATA_HEADER_LENGTH 5

// Session state tracking
typedef enum {
    notCreated,
    notStarted,
    Started
} UwbHandlerState;

// Shareable data structure (from demo)
typedef struct {
    uint32_t version;
    uint8_t config_data_length;
    char country_code[2];
    uint32_t session_id;
    uint8_t preamble_id;
    uint8_t channel_number;
    uint16_t num_slots_per_rround;
    uint16_t slot_duration;
    uint16_t ranging_duration;
    uint8_t ranging_round_control;
    uint8_t sts_init_iv[6];
    uint16_t dest_address;
} shareable_data_t;

// Global state
static UwbHandlerState gSessionState[MAX_SESSIONS] = {notCreated};
static uint32_t gSessionHandle[MAX_SESSIONS] = {0};
static uint16_t gMacAddr[MAX_SESSIONS] = {0};

bool handleDeviceInit(void) {
    tUWBAPI_STATUS status = UWBAPI_STATUS_OK;
    phUwbDevInfo_t devInfo;

    // Get device info
    status = UwbApi_GetDeviceInfo(&devInfo);
    if (status != UWBAPI_STATUS_OK) {
        NXPLOG_APP_E("Failed to get device info");
        return false;
    }

    // Prepare configuration response
    uint8_t response[32] = {0};
    response[0] = kRsp_AccessoryConfigurationData;
    response[1] = 0x00;  // Length LSB
    response[2] = 0x00;  // Length MSB

    // Add version info
    response[3] = devInfo.fwMajor;
    response[4] = devInfo.fwMinor;
    response[5] = devInfo.fwRc;

    // Add supported channels
    response[6] = 0x02;  // Number of channels
    response[7] = 5;     // Channel 5 (iPhone preferred)
    response[8] = 9;     // Channel 9 (alternate)

    // Calculate response length
    uint16_t length = 6;  // Version + channels
    response[1] = length & 0xFF;
    response[2] = (length >> 8) & 0xFF;

    // Send response
    if (!tlvSendRaw(0, response, length + TLV_HEADER_SIZE)) {
        NXPLOG_APP_E("Failed to send configuration response");
        return false;
    }

    NXPLOG_APP_I("Device initialization response sent");
    return true;
}

bool handleStopSession(uint8_t deviceId) {
    if (deviceId >= MAX_SESSIONS) {
        NXPLOG_APP_E("Invalid device ID: %d", deviceId);
        return false;
    }

    // Stop session if active
    if (gSessionState[deviceId] == Started) {
        tUWBAPI_STATUS status = UwbApi_StopRangingSession(gSessionHandle[deviceId]);
        if (status != UWBAPI_STATUS_OK) {
            NXPLOG_APP_E("Failed to stop session %d", deviceId);
            return false;
        }
    }

    // Release resources
    ResourceManager_ReleaseChannel(deviceId);
    gSessionState[deviceId] = notCreated;
    gSessionHandle[deviceId] = 0;

    // Send stop confirmation
    uint8_t response[3] = {
        kRsp_AccessoryUwbDidStop,
        0x00,  // Length LSB
        0x00   // Length MSB
    };

    if (!tlvSendRaw(deviceId, response, 3)) {
        NXPLOG_APP_E("Failed to send stop confirmation");
        return false;
    }

    NXPLOG_APP_I("Session %d stopped", deviceId);
    return true;
}

void handleDisconnection(uint8_t deviceId) {
    if (deviceId >= MAX_SESSIONS) {
        NXPLOG_APP_E("Invalid device ID: %d", deviceId);
        return;
    }

    // Stop session if active
    if (gSessionState[deviceId] == Started) {
        handleStopSession(deviceId);
    }

    // Clean up session state
    gSessionState[deviceId] = notCreated;
    gSessionHandle[deviceId] = 0;
    gMacAddr[deviceId] = 0;

    NXPLOG_APP_I("Device %d disconnected", deviceId);
}

bool handleConfigureAndStart(uint8_t deviceId, uint8_t* data, uint16_t length) {
    if (!data || length < SHAREABLE_DATA_HEADER_LENGTH) {
        NXPLOG_APP_E("Invalid configuration data");
        return false;
    }

    // Parse shareable data
    shareable_data_t* shareableData = (shareable_data_t*)(data + 1);
    uint16_t dataLength = *(data + SHAREABLE_DATA_LENGTH_OFFSET);

    // Configure session
    phUwbProfileInfo_t profileInfo = {0};
    profileInfo.deviceRole = kUWB_DeviceRole_Responder;
    profileInfo.deviceType = kUWB_DeviceType_Controlee;

    // Create session first
    tUWBAPI_STATUS status = UwbApi_SessionInit(shareableData->session_id, UWBD_RANGING_SESSION, &gSessionHandle[deviceId]);
    if (status != UWBAPI_STATUS_OK) {
        NXPLOG_APP_E("Failed to initialize session %d", deviceId);
        return false;
    }

    // Configure UWB parameters with shareable data
    status = UwbApi_ConfigureData_iOS(
        data + 1,                  // Shareable data pointer
        dataLength + SHAREABLE_DATA_HEADER_LENGTH,  // Data length
        &profileInfo,              // Profile info
        0, NULL,                   // No vendor params
        0, NULL                    // No debug params
    );

    if (status != UWBAPI_STATUS_OK) {
        NXPLOG_APP_E("Failed to configure session %d", deviceId);
        UwbApi_SessionDeinit(gSessionHandle[deviceId]);
        return false;
    }

    // Allocate resources - prefer iPhone's channel if this is iPhone
    bool preferredForIPhone = (deviceId == IPHONE_SESSION_SLOT);
    uint8_t allocatedChannel = ResourceManager_AllocateChannel(deviceId, gSessionHandle[deviceId], preferredForIPhone);

    if (allocatedChannel == 0xFF) {  // Invalid channel
        NXPLOG_APP_E("Failed to allocate channel for session %d", deviceId);
        UwbApi_SessionDeinit(gSessionHandle[deviceId]);
        return false;
    }

    // Start session
    status = UwbApi_StartRangingSession(gSessionHandle[deviceId]);
    if (status != UWBAPI_STATUS_OK) {
        NXPLOG_APP_E("Failed to start session %d", deviceId);
        ResourceManager_ReleaseChannel(deviceId);
        UwbApi_SessionDeinit(gSessionHandle[deviceId]);
        return false;
    }

    // Update state
    gSessionState[deviceId] = Started;
    gMacAddr[deviceId] = shareableData->dest_address;

    // Send start confirmation
    uint8_t response[3] = {
        kRsp_AccessoryUwbDidStart,
        0x00,  // Length LSB
        0x00   // Length MSB
    };

    if (!tlvSendRaw(deviceId, response, 3)) {
        NXPLOG_APP_E("Failed to send start confirmation");
        return false;
    }

    NXPLOG_APP_I("Session %d started", deviceId);
    return true;
}
