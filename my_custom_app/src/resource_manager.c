#include "resource_manager.h"
#include "phNxpLogApis_App.h"
#include <string.h>

// Global resource manager instance
static ResourceManager gResourceManager = {0};

// Available channels (avoiding iPhone preferred channels)
static const uint8_t availableChannels[] = {
    UWB_CHANNEL_5,  // iPhone preferred
    UWB_CHANNEL_6,  // Board channel 1
    UWB_CHANNEL_8,  // Board channel 2
    UWB_CHANNEL_9   // iPhone alternate / Board channel 3
};

bool ResourceManager_Init(void) {
    if (gResourceManager.isInitialized) {
        return true;
    }
    
    // Initialize channel allocations
    memset(&gResourceManager, 0, sizeof(ResourceManager));
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        gResourceManager.channels[i].channel = availableChannels[i];
        gResourceManager.channels[i].inUse = false;
        gResourceManager.channels[i].sessionIndex = -1;
        gResourceManager.channels[i].sessionId = 0;
    }
    
    // Initialize time slots
    for (int i = 0; i < MAX_TIME_SLOTS; i++) {
        gResourceManager.timeSlots[i].startTime = 0;
        gResourceManager.timeSlots[i].duration = 0;
        gResourceManager.timeSlots[i].sessionIndex = -1;
        gResourceManager.timeSlots[i].isActive = false;
    }
    
    gResourceManager.nextAvailableChannel = BOARD_CHANNELS_START;
    gResourceManager.currentTime = 0;
    gResourceManager.isInitialized = true;
    
    NXPLOG_APP_I("Resource Manager initialized successfully");
    return true;
}

void ResourceManager_Deinit(void) {
    if (!gResourceManager.isInitialized) {
        return;
    }
    
    // Release all allocated resources
    for (int i = 0; i < MAX_CHANNELS; i++) {
        gResourceManager.channels[i].inUse = false;
        gResourceManager.channels[i].sessionIndex = -1;
        gResourceManager.channels[i].sessionId = 0;
    }
    
    for (int i = 0; i < MAX_TIME_SLOTS; i++) {
        gResourceManager.timeSlots[i].isActive = false;
        gResourceManager.timeSlots[i].sessionIndex = -1;
    }
    
    gResourceManager.isInitialized = false;
    NXPLOG_APP_I("Resource Manager deinitialized");
}

uint8_t ResourceManager_AllocateChannel(int sessionIndex, uint32_t sessionId, bool preferredForIPhone) {
    if (!gResourceManager.isInitialized) {
        NXPLOG_APP_E("Resource Manager not initialized");
        return 0;
    }
    
    // Check if session already has a channel
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (gResourceManager.channels[i].inUse && 
            gResourceManager.channels[i].sessionIndex == sessionIndex) {
            NXPLOG_APP_W("Session %d already has channel %d", sessionIndex, gResourceManager.channels[i].channel);
            return gResourceManager.channels[i].channel;
        }
    }
    
    uint8_t selectedChannel = 0;
    
    if (preferredForIPhone) {
        // iPhone prefers channel 5, then 9
        if (ResourceManager_IsChannelAvailable(IPHONE_PREFERRED_CHANNEL)) {
            selectedChannel = IPHONE_PREFERRED_CHANNEL;
        } else if (ResourceManager_IsChannelAvailable(UWB_CHANNEL_9)) {
            selectedChannel = UWB_CHANNEL_9;
        }
    }
    
    // If no preferred channel available, find any available channel
    if (selectedChannel == 0) {
        for (int i = 0; i < MAX_CHANNELS; i++) {
            if (!gResourceManager.channels[i].inUse) {
                selectedChannel = gResourceManager.channels[i].channel;
                break;
            }
        }
    }
    
    if (selectedChannel == 0) {
        NXPLOG_APP_E("No available channels for session %d", sessionIndex);
        return 0;
    }
    
    // Allocate the channel
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (gResourceManager.channels[i].channel == selectedChannel) {
            gResourceManager.channels[i].inUse = true;
            gResourceManager.channels[i].sessionIndex = sessionIndex;
            gResourceManager.channels[i].sessionId = sessionId;
            break;
        }
    }
    
    NXPLOG_APP_I("Allocated channel %d to session %d (iPhone=%s)", 
                 selectedChannel, sessionIndex, preferredForIPhone ? "Yes" : "No");
    
    return selectedChannel;
}

bool ResourceManager_ReleaseChannel(int sessionIndex) {
    if (!gResourceManager.isInitialized) {
        return false;
    }
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (gResourceManager.channels[i].inUse && 
            gResourceManager.channels[i].sessionIndex == sessionIndex) {
            
            uint8_t channel = gResourceManager.channels[i].channel;
            gResourceManager.channels[i].inUse = false;
            gResourceManager.channels[i].sessionIndex = -1;
            gResourceManager.channels[i].sessionId = 0;
            
            NXPLOG_APP_I("Released channel %d from session %d", channel, sessionIndex);
            return true;
        }
    }
    
    NXPLOG_APP_W("No channel found for session %d to release", sessionIndex);
    return false;
}

uint8_t ResourceManager_GetSessionChannel(int sessionIndex) {
    if (!gResourceManager.isInitialized) {
        return 0;
    }
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (gResourceManager.channels[i].inUse && 
            gResourceManager.channels[i].sessionIndex == sessionIndex) {
            return gResourceManager.channels[i].channel;
        }
    }
    
    return 0;
}

bool ResourceManager_IsChannelAvailable(uint8_t channel) {
    if (!gResourceManager.isInitialized) {
        return false;
    }
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (gResourceManager.channels[i].channel == channel) {
            return !gResourceManager.channels[i].inUse;
        }
    }
    
    return false;
}

bool ResourceManager_AllocateTimeSlot(int sessionIndex, uint32_t duration) {
    if (!gResourceManager.isInitialized) {
        NXPLOG_APP_E("Resource Manager not initialized");
        return false;
    }
    
    // Check if session already has a time slot
    for (int i = 0; i < MAX_TIME_SLOTS; i++) {
        if (gResourceManager.timeSlots[i].isActive && 
            gResourceManager.timeSlots[i].sessionIndex == sessionIndex) {
            NXPLOG_APP_W("Session %d already has time slot %d", sessionIndex, i);
            return true;
        }
    }
    
    // Find available time slot
    uint32_t startTime = ResourceManager_GetNextAvailableTime(duration);
    if (startTime == 0) {
        NXPLOG_APP_E("No available time slots for session %d", sessionIndex);
        return false;
    }
    
    // Allocate time slot
    for (int i = 0; i < MAX_TIME_SLOTS; i++) {
        if (!gResourceManager.timeSlots[i].isActive) {
            gResourceManager.timeSlots[i].startTime = startTime;
            gResourceManager.timeSlots[i].duration = duration;
            gResourceManager.timeSlots[i].sessionIndex = sessionIndex;
            gResourceManager.timeSlots[i].isActive = true;
            
            NXPLOG_APP_I("Allocated time slot %d to session %d: Start=%lu, Duration=%lu", 
                         i, sessionIndex, startTime, duration);
            return true;
        }
    }
    
    NXPLOG_APP_E("No available time slot structures");
    return false;
}

bool ResourceManager_ReleaseTimeSlot(int sessionIndex) {
    if (!gResourceManager.isInitialized) {
        return false;
    }
    
    for (int i = 0; i < MAX_TIME_SLOTS; i++) {
        if (gResourceManager.timeSlots[i].isActive && 
            gResourceManager.timeSlots[i].sessionIndex == sessionIndex) {
            
            gResourceManager.timeSlots[i].isActive = false;
            gResourceManager.timeSlots[i].sessionIndex = -1;
            gResourceManager.timeSlots[i].startTime = 0;
            gResourceManager.timeSlots[i].duration = 0;
            
            NXPLOG_APP_I("Released time slot %d from session %d", i, sessionIndex);
            return true;
        }
    }
    
    NXPLOG_APP_W("No time slot found for session %d to release", sessionIndex);
    return false;
}

uint32_t ResourceManager_GetNextAvailableTime(uint32_t duration) {
    if (!gResourceManager.isInitialized) {
        return 0;
    }
    
    uint32_t currentTime = gResourceManager.currentTime;
    uint32_t proposedStart = currentTime;
    
    // Simple time slot allocation - stagger sessions by 50ms intervals
    // This is a basic implementation; could be made more sophisticated
    
    bool conflictFound;
    do {
        conflictFound = false;
        
        for (int i = 0; i < MAX_TIME_SLOTS; i++) {
            if (!gResourceManager.timeSlots[i].isActive) {
                continue;
            }
            
            if (ResourceManager_IsTimeSlotConflict(proposedStart, duration)) {
                conflictFound = true;
                proposedStart += 50; // Move 50ms forward
                break;
            }
        }
    } while (conflictFound);
    
    return proposedStart;
}

bool ResourceManager_IsTimeSlotConflict(uint32_t startTime, uint32_t duration) {
    if (!gResourceManager.isInitialized) {
        return true;
    }
    
    uint32_t endTime = startTime + duration;
    
    for (int i = 0; i < MAX_TIME_SLOTS; i++) {
        if (!gResourceManager.timeSlots[i].isActive) {
            continue;
        }
        
        uint32_t slotStart = gResourceManager.timeSlots[i].startTime;
        uint32_t slotEnd = slotStart + gResourceManager.timeSlots[i].duration;
        
        // Check for overlap
        if ((startTime < slotEnd) && (endTime > slotStart)) {
            return true; // Conflict found
        }
    }
    
    return false; // No conflict
}

bool ResourceManager_HasAvailableResources(void) {
    if (!gResourceManager.isInitialized) {
        return false;
    }
    
    // Check if we have at least one available channel and time slot
    bool hasChannel = (ResourceManager_GetAvailableChannelCount() > 0);
    bool hasTimeSlot = (ResourceManager_GetAvailableTimeSlotCount() > 0);
    
    return hasChannel && hasTimeSlot;
}

uint8_t ResourceManager_GetAvailableChannelCount(void) {
    if (!gResourceManager.isInitialized) {
        return 0;
    }
    
    uint8_t count = 0;
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (!gResourceManager.channels[i].inUse) {
            count++;
        }
    }
    
    return count;
}

uint8_t ResourceManager_GetAvailableTimeSlotCount(void) {
    if (!gResourceManager.isInitialized) {
        return 0;
    }
    
    uint8_t count = 0;
    for (int i = 0; i < MAX_TIME_SLOTS; i++) {
        if (!gResourceManager.timeSlots[i].isActive) {
            count++;
        }
    }
    
    return count;
}

bool ResourceManager_ResolveResourceConflict(int sessionIndex1, int sessionIndex2) {
    if (!gResourceManager.isInitialized) {
        return false;
    }
    
    // Simple conflict resolution: move session2 to different channel/time
    NXPLOG_APP_W("Resolving resource conflict between sessions %d and %d", sessionIndex1, sessionIndex2);
    
    // Try to move session2 to a different channel
    uint8_t currentChannel = ResourceManager_GetSessionChannel(sessionIndex2);
    ResourceManager_ReleaseChannel(sessionIndex2);
    
    // Find alternative channel
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (gResourceManager.channels[i].channel != currentChannel && 
            !gResourceManager.channels[i].inUse) {
            
            gResourceManager.channels[i].inUse = true;
            gResourceManager.channels[i].sessionIndex = sessionIndex2;
            
            NXPLOG_APP_I("Moved session %d to channel %d", sessionIndex2, gResourceManager.channels[i].channel);
            return true;
        }
    }
    
    NXPLOG_APP_E("Failed to resolve resource conflict");
    return false;
}

void ResourceManager_OptimizeResourceAllocation(void) {
    if (!gResourceManager.isInitialized) {
        return;
    }
    
    // Basic optimization - could be enhanced
    NXPLOG_APP_D("Optimizing resource allocation...");
    
    // Update current time
    gResourceManager.currentTime = 0; // Could get actual timestamp
    
    // Could implement more sophisticated optimization here
    // For now, just log current status
    ResourceManager_PrintResourceStatus();
}

bool ResourceManager_HandleHighPriorityRequest(int sessionIndex, uint32_t sessionId) {
    if (!gResourceManager.isInitialized) {
        return false;
    }
    
    NXPLOG_APP_I("Handling high priority request for session %d", sessionIndex);
    
    // For iPhone sessions (high priority), try to get preferred channel
    bool isIPhone = (sessionIndex == 0); // iPhone is always session 0
    
    if (isIPhone) {
        // If iPhone preferred channel is in use, try to move the other session
        if (!ResourceManager_IsChannelAvailable(IPHONE_PREFERRED_CHANNEL)) {
            // Find which session is using the preferred channel
            for (int i = 0; i < MAX_CHANNELS; i++) {
                if (gResourceManager.channels[i].channel == IPHONE_PREFERRED_CHANNEL && 
                    gResourceManager.channels[i].inUse) {
                    
                    int conflictSession = gResourceManager.channels[i].sessionIndex;
                    NXPLOG_APP_I("Moving session %d to make room for iPhone", conflictSession);
                    
                    return ResourceManager_ResolveResourceConflict(sessionIndex, conflictSession);
                }
            }
        }
    }
    
    return true;
}

void ResourceManager_RebalanceResources(void) {
    if (!gResourceManager.isInitialized) {
        return;
    }
    
    NXPLOG_APP_D("Rebalancing resources...");
    
    // Could implement resource rebalancing logic here
    // For now, just optimize
    ResourceManager_OptimizeResourceAllocation();
}

void ResourceManager_PrintResourceStatus(void) {
    if (!gResourceManager.isInitialized) {
        return;
    }
    
    NXPLOG_APP_I("=== Resource Manager Status ===");
    
    // Print channel status
    NXPLOG_APP_I("Channels:");
    for (int i = 0; i < MAX_CHANNELS; i++) {
        ChannelAllocation* ch = &gResourceManager.channels[i];
        NXPLOG_APP_I("  Channel %d: %s (Session %d)", 
                     ch->channel,
                     ch->inUse ? "USED" : "FREE",
                     ch->inUse ? ch->sessionIndex : -1);
    }
    
    // Print time slot status
    NXPLOG_APP_I("Time Slots:");
    int activeSlots = 0;
    for (int i = 0; i < MAX_TIME_SLOTS; i++) {
        if (gResourceManager.timeSlots[i].isActive) {
            TimeSlot* ts = &gResourceManager.timeSlots[i];
            NXPLOG_APP_I("  Slot %d: Session %d, Start=%lu, Duration=%lu", 
                         i, ts->sessionIndex, ts->startTime, ts->duration);
            activeSlots++;
        }
    }
    
    NXPLOG_APP_I("Available: %d channels, %d time slots", 
                 ResourceManager_GetAvailableChannelCount(),
                 ResourceManager_GetAvailableTimeSlotCount());
    
    NXPLOG_APP_I("==============================");
}

const char* ResourceManager_GetChannelStatusString(uint8_t channel) {
    if (!gResourceManager.isInitialized) {
        return "Unknown";
    }
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (gResourceManager.channels[i].channel == channel) {
            return gResourceManager.channels[i].inUse ? "In Use" : "Available";
        }
    }
    
    return "Invalid";
} 