# UWB Implementation Analysis

## Channel Management and Collision Avoidance

### Available Channels
The system uses 4 UWB channels for communication:
```c
static const uint8_t availableChannels[] = {
    UWB_CHANNEL_5,  // iPhone preferred
    UWB_CHANNEL_6,  // Board channel 1
    UWB_CHANNEL_8,  // Board channel 2
    UWB_CHANNEL_9   // iPhone alternate / Board channel 3
};
```

### Channel Allocation Strategy
1. **Device-Specific Preferences**:
   - iPhones prefer Channel 5 (primary) or Channel 9 (alternate)
   - Other boards use Channels 6, 8, or 9
   - Channel allocation is managed by `ResourceManager_AllocateChannel()`

2. **Time Slot Management**:
   - Sessions are staggered by 50ms intervals
   - Implemented in `ResourceManager_GetNextAvailableTime()`
   - Conflict detection via `ResourceManager_IsTimeSlotConflict()`

3. **Collision Resolution**:
   ```c
   bool ResourceManager_ResolveResourceConflict(int sessionIndex1, int sessionIndex2) {
       // Moves session2 to a different channel if conflict occurs
   }
   ```

### Session Management
1. **Maximum Sessions**: 5 simultaneous sessions (defined by `MAXIMUM_SESSION_COUNT`)
2. **Session Types**:
   ```c
   typedef enum session_type {
       UWBD_RANGING_SESSION = 0x00,
       UWBD_RANGING_WITH_INBAND_DATA_TRANSFER = 0x01,
       UWBD_DATA_TRANSFER = 0x02,
       // ... other types
   } eSessionType;
   ```

## Device Identification

### MAC Address Handling
1. **Address Modes**:
   ```c
   typedef enum {
       kUWB_MacAddressMode_2bytes = 0,
       kUWB_MacAddressMode_8bytes = 2,
   } UWB_MacAddressMode_t;
   ```

2. **Device Types**:
   ```c
   typedef enum {
       kUWB_DeviceType_Controlee = 0,
       kUWB_DeviceType_Controller = 1,
   } UWB_DeviceType_t;
   ```

### Session Identification
1. **Session IDs**:
   ```c
   static uint32_t generateSessionId(DeviceType deviceType, int sessionIndex) {
       uint32_t baseId = (deviceType == DEVICE_TYPE_IPHONE) ? 0x11223340 : 0x22334400;
       return baseId + sessionIndex;
   }
   ```

2. **Device Roles**:
   ```c
   typedef enum {
       kUWB_DeviceRole_Responder = 0,
       kUWB_DeviceRole_Initiator = 1,
       kUWB_DeviceRole_UT_Sync_Anchor = 2,
       // ... other roles
   } UWB_DeviceRole_t;
   ```

## Firmware Compatibility

### Device Support
1. **Chip Types**:
   ```c
   typedef enum {
       kChipType_NA = 0x00,
       kChipType_SR150 = 0x01,
       kChipType_SR040 = 0x02,
       kChipType_SR160 = 0x03
   } demo_chip_type_t;
   ```

2. **Board Types**:
   ```c
   typedef enum {
       kBoardType_NA = 0x00,
       kBoardType_Shield = 0x01,
       kBoardType_FinderV3 = 0x03
   } demo_board_type_t;
   ```

### Protocol Support
1. **PRF Modes**:
   ```c
   typedef enum UWB_PrfMode {
       kUWB_PrfMode_62_4MHz = 0,  // BPRF
       kUWB_PrfMode_124_8MHz = 1, // HPRF
       kUWB_PrfMode_249_6MHz = 2  // HPRF mode with high data rate
   } UWB_PrfMode_t;
   ```

2. **Data Rates**:
   ```c
   typedef enum UWB_PsduDataRate {
       kUWB_PsduDataRate_6_81Mbps = 0,
       kUWB_PsduDataRate_7_80Mbps = 1,
       kUWB_PsduDataRate_27_2Mbps = 2,
       kUWB_PsduDataRate_31_2Mbps = 3,
       kUWB_PsduDataRate_850Kbps = 4
   } UWB_PsduDataRate_t;
   ```

## Mesh Network Considerations

### Limitations
1. **Channel Constraints**:
   - Only 4 available UWB channels
   - Channel 5 reserved for iPhone devices
   - Maximum 5 simultaneous sessions

2. **Resource Management**:
   - Time slot allocation required for each session
   - Channel conflicts must be actively managed
   - Session priority handling (iPhone sessions get priority)

### Recommendations for Mesh Implementation
1. **Channel Rotation**:
   - Implement dynamic channel hopping
   - Use Channels 6, 8, and 9 for board-to-board communication
   - Reserve Channel 5 for iPhone interactions

2. **Session Management**:
   - Implement session timeouts (currently 10 seconds)
   - Use resource optimization: `ResourceManager_OptimizeResourceAllocation()`
   - Handle session conflicts via `ResourceManager_ResolveResourceConflict()`

3. **Firmware Considerations**:
   - All boards should use the same firmware version
   - Ensure compatible PRF modes and data rates
   - Consider board type compatibility when deploying

## Channel Rotation Strategy for Complete Graph Topology

### Complete Graph Requirements
- 4 boards, each needing connections to 3 other boards
- Total of 6 edges (sessions) needed for complete connectivity
- Each board needs to maintain 3 active sessions

### Channel Rotation Implementation

1. **Initial Channel Assignment**:
```c
gBoardDiscovery.boards[i].channel = UWB_CHANNEL_6 + (i % 2); // Alternate channels 6,8
```

2. **Session Scheduling**:
- Available channels: 6, 8, 9 (Channel 5 reserved for iPhone)
- Time slots staggered by 50ms intervals
- Maximum 5 simultaneous sessions supported

3. **Rotation Strategy**:

Board Pairs:
```
A-B: Channel 6
B-C: Channel 8
C-D: Channel 6
D-A: Channel 8
A-C: Channel 9
B-D: Channel 9
```

Time Slot Allocation:
```
T0:    A-B (Ch6)  C-D (Ch8)
T50ms: B-C (Ch8)  D-A (Ch6)
T100ms: A-C (Ch9) B-D (Ch9)
```

4. **Implementation Details**:
```c
// Resource allocation with time slots
bool ResourceManager_AllocateTimeSlot(int sessionIndex, uint32_t duration) {
    uint32_t startTime = ResourceManager_GetNextAvailableTime(duration);
    // Stagger sessions by 50ms
    if (ResourceManager_IsTimeSlotConflict(startTime, duration)) {
        startTime += 50; // Move to next slot
    }
}

// Channel allocation
uint8_t ResourceManager_AllocateChannel(int sessionIndex, uint32_t sessionId, bool preferredForIPhone) {
    // Rotate through available channels
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (!gResourceManager.channels[i].inUse) {
            selectedChannel = gResourceManager.channels[i].channel;
            break;
        }
    }
}
```

### Session Management

1. **Session Creation**:
```c
// Create sessions for each board pair
for (int i = 0; i < boardCount; i++) {
    for (int j = i + 1; j < boardCount; j++) {
        int sessionIndex = SessionManager_CreateSession(
            DEVICE_TYPE_NXP_BOARD,
            boards[j].macAddr,
            BOARD_MAC_ADDR_LEN
        );
    }
}
```

2. **Resource Optimization**:
```c
void ResourceManager_OptimizeResourceAllocation(void) {
    // Update current time
    uint32_t oldTime = gResourceManager.currentTime;
    gResourceManager.currentTime = xTaskGetTickCount();
    
    // Count active resources
    int activeChannels = MAX_CHANNELS - ResourceManager_GetAvailableChannelCount();
    int activeTimeSlots = MAX_TIME_SLOTS - ResourceManager_GetAvailableTimeSlotCount();
}
```

### Timing Considerations

1. **Session Intervals**:
```c
// Default ranging intervals
session->rangingInterval = 200; // 200ms for board sessions
```

2. **Timeout Handling**:
```c
// Check for ranging timeout (no data for 10 seconds)
if (session->lastRangingTime > 0 && 
    (currentTime - session->lastRangingTime) > 10000) {
    // Trigger recovery
}
```

## Signal Filtering and Security Mechanisms

### 1. MAC Address Filtering

```c
typedef enum UWB_MacAddressMode {
    kUWB_MacAddressMode_2bytes = 0,  // Short address mode
    kUWB_MacAddressMode_8bytes = 2   // Extended address mode
} UWB_MacAddressMode_t;
```

MAC address filtering works at two levels:

1. **Hardware Level**:
- Each UWB frame includes source and destination MAC addresses
- Hardware automatically filters frames not matching device's MAC
- Supports both 2-byte (short) and 8-byte (extended) addressing

2. **Software Level**:
```c
typedef struct SessionContext {
    uint8_t macAddr[MAC_ADDR_LENGTH];
    uint8_t macAddrLen;
    // ... other fields
} SessionContext;

// Verification during ranging data processing
void SessionManager_HandleRangingData(int sessionIndex, phRangingData_t* rangingData) {
    if (memcmp(session->macAddr, rangingData->mac_addr, session->macAddrLen) != 0) {
        // Reject data from unexpected device
        return;
    }
}
```

### 2. Session ID Verification

Session IDs are unique identifiers for each ranging session:

```c
static uint32_t generateSessionId(DeviceType deviceType, int sessionIndex) {
    // Different base IDs for different device types
    uint32_t baseId = (deviceType == DEVICE_TYPE_IPHONE) ? 0x11223340 : 0x22334400;
    return baseId + sessionIndex;
}
```

Verification process:
1. Each UWB frame carries a session ID
2. Hardware and firmware validate session ID matches active session
3. Frames with invalid session IDs are rejected
4. Prevents cross-session interference

### 3. Time Slot Validation

Time slot management ensures temporal separation of signals:

```c
typedef struct TimeSlot {
    uint32_t startTime;
    uint32_t duration;
    int sessionIndex;
    bool isActive;
} TimeSlot;

bool ResourceManager_IsTimeSlotConflict(uint32_t startTime, uint32_t duration) {
    uint32_t endTime = startTime + duration;
    
    for (int i = 0; i < MAX_TIME_SLOTS; i++) {
        if (!timeSlots[i].isActive) continue;
        
        uint32_t slotStart = timeSlots[i].startTime;
        uint32_t slotEnd = slotStart + timeSlots[i].duration;
        
        // Check for overlap
        if ((startTime < slotEnd) && (endTime > slotStart)) {
            return true; // Conflict found
        }
    }
    return false;
}
```

Key aspects:
1. Each session is assigned specific time slots
2. 50ms staggering between slots
3. Hardware timing synchronization
4. Signals outside assigned slots are rejected

### 4. STS (Secure Timing Sequence) Security

STS provides cryptographic security for UWB signals:

```c
typedef enum UWB_StsConfig {
    kUWB_StsConfig_StaticSts = 0,               // Static key
    kUWB_StsConfig_DynamicSts = 1,              // Dynamic key
    kUWB_StsConfig_DynamicSts_Ctrlee_key = 2,   // Dynamic with controlee key
    kUWB_StsConfig_ProvisionSts = 3,            // Provisioned key
    kUWB_StsConfig_ProvisionSts_Ctrlee_key = 4  // Provisioned with controlee key
} UWB_StsConfig_t;
```

STS Security Features:
1. **Cryptographic Protection**:
   - Each frame includes an STS sequence
   - Sequence is generated using session-specific keys
   - Prevents signal spoofing and replay attacks

2. **Key Management**:
   ```c
   #define SUB_SESSION_KEY_LEN_16B 16
   #define SUB_SESSION_KEY_LEN_32B 32
   #define MAX_SUB_SESSION_KEY_LEN SUB_SESSION_KEY_LEN_32B
   ```
   - Supports both 16-byte and 32-byte keys
   - Keys can be static or dynamic
   - Optional controlee-specific keys

3. **Verification Process**:
   - Hardware validates STS sequence
   - Invalid sequences are rejected immediately
   - Provides protection against:
     * Signal spoofing
     * Replay attacks
     * Man-in-the-middle attacks

### Combined Effect

When a signal is received on a shared channel:

1. **Initial Hardware Filtering**:
   - MAC address check
   - Basic timing validation
   - STS sequence verification

2. **Firmware Validation**:
   ```c
   bool validateRangingData(phRangingData_t* data, SessionContext* session) {
       // 1. MAC Address Check
       if (!validateMacAddress(data->mac_addr, session->macAddr)) {
           return false;
       }
       
       // 2. Session ID Verification
       if (data->sessionHandle != session->sessionHandle) {
           return false;
       }
       
       // 3. Time Slot Check
       if (!isWithinTimeSlot(data->timestamp, session->timeSlot)) {
           return false;
       }
       
       // 4. STS Verification already done by hardware
       return true;
   }
   ```

3. **Error Handling**:
   - Invalid signals are logged for debugging
   - Session statistics are updated
   - Recovery mechanisms triggered if needed

This multi-layer filtering ensures that even when channels are shared:
- Only valid signals from expected devices are processed
- Temporal separation is maintained
- Cryptographic security prevents spoofing
- Cross-session interference is minimized

## Scaling to N Edges and Channel Sharing

### Scaling Challenges

1. **Channel Limitations**:
- Only 3 available channels (6, 8, 9) for board-to-board communication
- Channel 5 reserved for iPhone connectivity
- Maximum 5 simultaneous sessions supported by firmware

2. **Signal Identification and Filtering**:
```c
typedef struct phRangingMesr {
    uint8_t mac_addr[MAC_ADDR_LENGTH];    // Device MAC address
    uint8_t status;                       // Status of measurement
    uint8_t slot_index;                   // Slot number within ranging round
    uint16_t distance;                    // Distance in centimeters
} phRangingMesr_t;
```

Signals are filtered by:
- MAC Address matching
- Session ID verification
- Time slot allocation
- STS (Secure Timing Sequence) configuration

### Channel Sharing Strategy

1. **Time Division Multiple Access (TDMA)**:
```c
// Time slot allocation with 50ms staggering
uint32_t ResourceManager_GetNextAvailableTime(uint32_t duration) {
    uint32_t proposedStart = currentTime;
    do {
        if (ResourceManager_IsTimeSlotConflict(proposedStart, duration)) {
            proposedStart += 50; // Move 50ms forward
        }
    } while (conflictFound);
}
```

2. **Session Identification**:
```c
static uint32_t generateSessionId(DeviceType deviceType, int sessionIndex) {
    uint32_t baseId = (deviceType == DEVICE_TYPE_IPHONE) ? 0x11223340 : 0x22334400;
    return baseId + sessionIndex;
}
```

3. **Wrong Signal Rejection**:
- MAC address filtering at hardware level
- Session ID verification for each ranging measurement
- Time slot validation
- STS-based security (prevents signal spoofing)

### Scaling Solutions

1. **Dynamic Channel Assignment**:
```c
typedef struct ChannelSchedule {
    uint8_t channel;
    uint32_t timeSlot;
    uint32_t duration;
    uint8_t priority;
} ChannelSchedule_t;

// Example channel rotation for N nodes
void rotateChannels(int nodeCount) {
    int maxSimultaneousSessions = min(5, nodeCount * (nodeCount-1) / 2);
    int timeSlotDuration = 50; // ms
    
    for (int i = 0; i < nodeCount; i++) {
        for (int j = i + 1; j < nodeCount; j++) {
            int channelIndex = (i + j) % 3; // Rotate through channels 6,8,9
            int timeSlot = ((i * nodeCount + j) % maxSimultaneousSessions) * timeSlotDuration;
            // Schedule session
        }
    }
}
```

2. **Priority-based Session Management**:
```c
bool ResourceManager_HandleHighPriorityRequest(int sessionIndex, uint32_t sessionId) {
    // For critical paths in the mesh
    if (isHighPriority) {
        // Try to get preferred channel
        if (!ResourceManager_IsChannelAvailable(preferredChannel)) {
            // Move lower priority session to different channel/time
            return ResourceManager_ResolveResourceConflict(sessionIndex, conflictSession);
        }
    }
}
```

3. **Adaptive Time Slot Allocation**:
```c
// Adjust time slots based on network size
uint32_t calculateTimeSlot(int totalNodes, int nodeIndex) {
    uint32_t baseInterval = 50; // Base 50ms interval
    uint32_t scaleFactor = (totalNodes > 4) ? (totalNodes / 4) : 1;
    return baseInterval * scaleFactor;
}
```

### Best Practices for N-node Mesh

1. **Session Priority**:
- Assign higher priority to critical paths
- Use shorter ranging intervals for important connections
- Implement fallback channels for priority sessions

2. **Resource Management**:
- Dynamically adjust time slots based on network size
- Implement session rotation for fair resource sharing
- Use adaptive ranging intervals based on network load

3. **Error Handling**:
```c
void SessionManager_HandleSessionError(int sessionIndex, uint32_t errorCode) {
    // Implement error recovery logic
    if (errorCode == UWBAPI_STATUS_CHANNEL_BUSY) {
        // Try alternative channel or time slot
    } else if (errorCode == UWBAPI_STATUS_SESSION_NOT_EXIST) {
        // Reinitialize session
    }
}
```

4. **Performance Optimization**:
- Use shorter ranging intervals for nearby nodes
- Implement dynamic channel hopping for interference avoidance
- Monitor session quality and adjust parameters accordingly

## References
All code references are from:
- `session_manager.c`
- `resource_manager.c`
- `UwbApi_Types.h`
- `board_adapter.c`
- `my_app_main.c`
