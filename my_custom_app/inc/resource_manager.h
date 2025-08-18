#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

// UWB channel definitions
#define UWB_CHANNEL_5 5
#define UWB_CHANNEL_6 6
#define UWB_CHANNEL_8 8
#define UWB_CHANNEL_9 9

// iPhone typically uses channels 5 and 9, so avoid those for boards
#define IPHONE_PREFERRED_CHANNEL UWB_CHANNEL_5
#define BOARD_CHANNELS_START UWB_CHANNEL_6  // Use 6, 8 for boards

#define MAX_CHANNELS 4
#define MAX_TIME_SLOTS 10

// Channel allocation
typedef struct {
    uint8_t channel;
    bool inUse;
    int sessionIndex;           // Which session is using this channel
    uint32_t sessionId;         // Session ID for verification
} ChannelAllocation;

// Time slot management
typedef struct {
    uint32_t startTime;         // Start time in milliseconds
    uint32_t duration;          // Duration in milliseconds
    int sessionIndex;           // Which session owns this slot
    bool isActive;
} TimeSlot;

// Resource manager structure
typedef struct {
    ChannelAllocation channels[MAX_CHANNELS];
    TimeSlot timeSlots[MAX_TIME_SLOTS];
    uint8_t nextAvailableChannel;
    uint32_t currentTime;
    bool isInitialized;
} ResourceManager;

// Resource manager functions
bool ResourceManager_Init(void);
void ResourceManager_Deinit(void);

// Channel management
uint8_t ResourceManager_AllocateChannel(int sessionIndex, uint32_t sessionId, bool preferredForIPhone);
bool ResourceManager_ReleaseChannel(int sessionIndex);
uint8_t ResourceManager_GetSessionChannel(int sessionIndex);
bool ResourceManager_IsChannelAvailable(uint8_t channel);

// Time slot management
bool ResourceManager_AllocateTimeSlot(int sessionIndex, uint32_t duration);
bool ResourceManager_ReleaseTimeSlot(int sessionIndex);
uint32_t ResourceManager_GetNextAvailableTime(uint32_t duration);
bool ResourceManager_IsTimeSlotConflict(uint32_t startTime, uint32_t duration);

// Resource queries
bool ResourceManager_HasAvailableResources(void);
uint8_t ResourceManager_GetAvailableChannelCount(void);
uint8_t ResourceManager_GetAvailableTimeSlotCount(void);

// Resource coordination
bool ResourceManager_ResolveResourceConflict(int sessionIndex1, int sessionIndex2);
void ResourceManager_OptimizeResourceAllocation(void);

// Priority management
bool ResourceManager_HandleHighPriorityRequest(int sessionIndex, uint32_t sessionId);
void ResourceManager_RebalanceResources(void);

// Utility functions
void ResourceManager_PrintResourceStatus(void);
const char* ResourceManager_GetChannelStatusString(uint8_t channel);

#endif // RESOURCE_MANAGER_H 