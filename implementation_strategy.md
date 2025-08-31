# Implementation Strategy for Simultaneous UWB Sessions

## 📚 Codebase Structure Analysis

### Core Components

1. **Hardware Layer**
   - `/boards/` - Hardware-specific configurations
   - `/libs/halimpl/` - Hardware Abstraction Layer
   - Target hardware: Rhodes4 board (primary)

2. **Middleware & Core Libraries**
   - `/libs/uci-core/` - UWB Controller Interface
   - `/libs/uwb-iot/` - IoT-specific UWB APIs
   - `/se_mw/` - Secure Element middleware
   - FreeRTOS RTOS integration

3. **Application Layer**
   - `/demos/` - Reference implementations
   - `/my_custom_app/` - Custom application target
   - OSAL (Operating System Abstraction Layer) for task management

### Build System

1. **Compilation Flow**
   - CMake-based build system
   - Board-specific configurations
   - FreeRTOS integration
   - Security features via mbedTLS
   - Firmware image management

2. **Key Build Components**
   - Base firmware from `/firmware_images/`
   - Board-specific configurations
   - Application code compilation
   - Secure Element integration

## 🎯 Implementation Strategy

### 1. Session Management Architecture

#### Existing Infrastructure
- Support for up to 5 concurrent sessions
- Independent session handles
- BLE layer supports multiple connections
- Session state tracking arrays

#### Required Extensions
```c
// Session Management Structures
typedef struct {
    uint32_t sessionHandle;
    UwbHandlerState state;
    DeviceType deviceType;    // iPhone or NXP_Board
    uint16_t macAddr;
    SessionConfig config;     // Channel, timing, etc.
} SessionContext;

// Task Management
typedef struct {
    UWBOSAL_TASK_HANDLE taskHandle;
    SessionType sessionType; // iPhone_NI or NXP_Direct
    uint8_t channelNumber;
    bool isActive;
} TaskContext;
```

### 2. Task Architecture

1. **Main Controller Task**
   - Session management
   - Resource allocation
   - Event distribution
   - Error handling

2. **iPhone Session Task**
   - Nearby Interaction protocol
   - BLE handling
   - Session state management
   - Based on demo_nearby_interaction

3. **NXP Board Session Task**
   - Direct UWB communication
   - Discovery protocol
   - Session management
   - Based on demo_ranging_controller/controlee

### 3. Bidirectional Communication Strategy

1. **Single Session Approach (DS-TWR)**
   - Uses Double-Sided Two-Way Ranging (DS-TWR)
   - Single session handles both transmission and reception
   - Advantages:
     * More efficient resource usage
     * Simpler session management
     * Lower overhead
   - Limitations:
     * Fixed controller/controlee roles
     * Less flexible for dynamic role changes
     * Limited to point-to-point communication

2. **Dual Session Approach**
   - Separate sessions for controller and controlee roles
   - Advantages:
     * More flexible role management
     * Supports multiple simultaneous peers
     * Independent session control
   - Limitations:
     * Uses more session slots (2 per peer)
     * More complex resource management
     * Higher overhead

3. **Implementation Decision**
   - Recommended: Start with DS-TWR (Single Session)
   - Rationale:
     * More efficient resource usage
     * Sufficient for basic bidirectional needs
     * Can upgrade to dual sessions if needed
   - Migration path to dual sessions if required

### 4. Session Management Strategy

1. **Session Types and Priority**
   ```c
   typedef enum {
       SESSION_TYPE_NONE = 0,
       SESSION_TYPE_IPHONE_NI,      // iPhone Nearby Interaction (Highest Priority)
       SESSION_TYPE_BOARD_DS_TWR    // Board-to-board DS-TWR
   } SessionType;
   ```

2. **Fixed Session Allocation**
   ```c
   #define MAX_SESSIONS 5
   #define MAX_BOARD_SESSIONS 4
   #define IPHONE_SESSION_SLOT 0  // iPhone always gets slot 0 (highest priority)
   ```

3. **Resource Allocation Strategy**
   ```c
   // Session Manager Structure
   typedef struct {
       SessionContext sessions[MAX_SESSIONS];
       uint8_t activeSessions;
       uint8_t totalSessions;
       bool isInitialized;
       UWBOSAL_TASK_HANDLE managerTaskHandle;
       void* rangingMutex;
   } SessionManager;

   // Channel Allocation Rules
   - iPhone gets priority for Channel 5
   - Will force other sessions to move if needed
   - Board sessions cannot use iPhone's preferred channel
   - Channel 9 as iPhone fallback
   ```

4. **Session Context**
   ```c
   typedef struct {
       uint32_t sessionId;
       uint32_t sessionHandle;
       SessionType type;
       SessionState state;
       DeviceType deviceType;
       uint8_t macAddr[8];
       uint8_t macAddrLen;
       uint8_t channel;
       uint16_t rangingInterval;
       bool isActive;
       uint32_t lastRangingTime;
       float lastDistance;
   } SessionContext;
   ```

5. **iPhone Session States**
   ```c
   typedef enum {
       IPHONE_STATE_DISCONNECTED = 0,
       IPHONE_STATE_BLE_ADVERTISING,
       IPHONE_STATE_BLE_CONNECTING,
       IPHONE_STATE_BLE_CONNECTED,
       IPHONE_STATE_UWB_INITIALIZING,
       IPHONE_STATE_UWB_ACTIVE,
       IPHONE_STATE_ERROR,
       IPHONE_STATE_RECOVERY
   } iPhoneState;
   ```

### 5. Resource Management

1. **UWB Channel Allocation**
   - iPhone sessions: Default channels
   - NXP sessions: Separate channels
   - Dynamic channel management

2. **Timing Coordination**
   - Staggered ranging intervals
   - Session prioritization
   - Collision avoidance

3. **Memory Management**
   - Static allocation for critical structures
   - Dynamic allocation for session data
   - Resource pools for common objects

### 6. Implementation Phases

#### Phase 1: Infrastructure Setup
1. Create session management framework
2. Implement task architecture
3. Setup resource management
4. Add logging and debugging infrastructure

#### Phase 2: iPhone Integration
1. Port nearby_interaction demo code
2. Integrate BLE handling (it can be implemented exactly the same way for now)
3. Implement session management
4. Add error handling and recovery

#### Phase 3: NXP Board Integration
1. Implement discovery protocol
2. Add direct UWB communication
3. Setup session management
4. Integrate with main controller

#### Phase 4: Simultaneous Operation
1. Implement resource sharing
2. Add timing coordination
3. Test multiple sessions
4. Optimize performance

### 7. Error Handling & Recovery

1. **Session-Level Errors**
   - Connection loss
   - Timeout handling
   - Resource exhaustion
   - State recovery

2. **System-Level Errors**
   - Hardware failures
   - Resource conflicts
   - Stack overflow
   - Watchdog integration


## 🚀 Next Steps

1. Setup development environment
2. Create basic task architecture
3. Implement session management
4. Begin iPhone integration
5. Add NXP board support
6. Test simultaneous operation
7. Optimize and refine
