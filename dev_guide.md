# Developer Guide: Multi-Session UWB Implementation

## 👋 Introduction

Welcome to the Multi-Session UWB project! This guide will help you understand our codebase, which extends NXP's UWB stack to support simultaneous sessions with both iPhones (using Nearby Interaction) and NXP boards (using direct UWB communication).

## 📁 Project Structure

### Key Directories

```
uwbiot-top/
├── demos/                    # Reference implementations
│   ├── SR1XX/               # Core demo implementations
│   │   ├── demo_nearby_interaction/       # iPhone interaction demo
│   │   ├── demo_ranging_controller/       # Board-to-board ranging (controller)
│   │   └── demo_ranging_controlee/        # Board-to-board ranging (controlee)
├── libs/
│   ├── uci-core/            # UWB Controller Interface
│   ├── uwb-iot/             # IoT-specific UWB APIs
│   └── halimpl/             # Hardware Abstraction Layer
├── my_custom_app/           # Our multi-session implementation
    ├── inc/                 # Header files
    └── src/                 # Source files
```

## 🔑 Key Components

### 1. Session Management
- **Location**: `my_custom_app/src/session_manager.c`
- **Key Functions**:
  ```c
  createSession(DeviceType type)      // Create new session
  terminateSession(SessionId id)      // End session
  getSessionState(SessionId id)       // Get session status
  ```
- **Important Notes**:
  - Supports up to 5 concurrent sessions
  - Each session has independent configuration
  - Check `MAXIMUM_SESSION_COUNT` for session limits

### 2. Resource Management
- **Location**: `my_custom_app/src/resource_manager.c`
- **Key Functions**:
  ```c
  allocateChannel(SessionId id)       // Assign UWB channel
  scheduleRanging(SessionId id)       // Schedule ranging timeslots
  ```
- **Important Notes**:
  - iPhone sessions use default channels
  - NXP board sessions use separate channels
  - Time slot management prevents conflicts

### 3. Protocol Adapters

#### iPhone (Nearby Interaction)
- **Reference Implementation**: `demos/SR1XX/demo_nearby_interaction/`
- **Key Files**:
  - `demo_nearby_interaction.c` - Core NI protocol
  - `inc/NearbyInteraction.h` - Protocol definitions
- **Important Notes**:
  - Uses BLE for initial connection
  - Follows Apple's NI protocol
  - Check Apple's documentation for protocol details

#### NXP Board Communication
- **Reference Implementation**: `demos/SR1XX/demo_ranging_controller/`
- **Key Files**:
  - `demo_ranging_controller.c` - Direct UWB communication
  - `app_ranging_cfg.h` - Ranging configuration

## 🛠 Implementation Guide

### 1. Getting Started
1. Study the reference implementations:
   - `demo_nearby_interaction` for iPhone protocol
   - `demo_ranging_controller` for board-to-board communication

2. Understand the core APIs:
   - `libs/uwb-iot/uwb_api/` - Main UWB API
   - `libs/uci-core/` - Controller interface

### 2. Key Implementation Files
```c
my_custom_app/
├── inc/
│   ├── session_manager.h    // Session management
│   ├── resource_manager.h   // Resource coordination
│   └── protocol_adapters.h  // Protocol-specific handling
└── src/
    ├── my_app_main.c       // Application entry point
    ├── session_manager.c    // Session implementation
    └── resource_manager.c   // Resource handling
```

### 3. Important Functions to Study

#### Session Management
```c
// in session_manager.c
void handleSessionEvent(SessionId id, UwbEvent event) {
    // Handles session-specific events
}

void processRangingData(SessionId id, RangingData data) {
    // Processes ranging results
}
```

#### Resource Management
```c
// in resource_manager.c
void allocateResources(SessionConfig config) {
    // Manages UWB channels and timing
}

void handleResourceConflict(SessionId id1, SessionId id2) {
    // Resolves resource conflicts
}
```

## 📚 Reference Documentation

### NXP Documentation
- UWB Stack: `docs/uwb_stack.html`
- UCI Protocol: `docs/uci_protocol.html`
- Hardware Reference: `docs/hardware_ref.html`

### Apple Documentation
- [Nearby Interaction Framework](https://developer.apple.com/documentation/nearbyinteraction)
- [UWB Ranging Guidelines](https://developer.apple.com/documentation/nearbyinteraction/implementing_spatial_interactions_with_third-party_accessories)

## 🔍 Debugging Tips

1. **Session Issues**
   - Check `mSessionState` array for session status
   - Verify `mSessionHandle` values
   - Monitor BLE connection status for iPhone sessions

2. **Ranging Problems**
   - Check UWB channel allocation
   - Verify timing configurations
   - Monitor ranging intervals

3. **Resource Conflicts**
   - Check channel assignments
   - Verify time slot allocation
   - Monitor resource manager logs

## 🚀 Getting Started Steps

1. **Setup Development Environment**
   ```bash
   cd uwbiot-top
   cmake -B build
   ```

2. **Build the Project**
   ```bash
   cd build
   make
   ```

3. **Flash the Firmware**
   ```bash
   make flash
   ```

4. **Monitor Output**
   ```bash
   make monitor
   ```

## ⚠️ Common Pitfalls

1. **Session Management**
   - Don't exceed `MAXIMUM_SESSION_COUNT`
   - Always clean up terminated sessions
   - Handle session errors properly

2. **Resource Allocation**
   - Check for channel conflicts
   - Verify timing slot availability
   - Handle resource exhaustion

3. **Protocol Handling**
   - Follow Apple's NI protocol exactly
   - Handle BLE disconnections gracefully
   - Maintain proper ranging intervals

## 🤝 Contributing

1. Follow the existing code structure
2. Add comprehensive comments
3. Update relevant documentation
4. Add unit tests for new features
5. Test with both iPhone and NXP board scenarios

## 🔗 Additional Resources

1. **UWB Specifications**
   - FiRa Consortium Documentation
   - IEEE 802.15.4z Standard

2. **Development Tools**
   - NXP MCUXpresso IDE
   - UWB Protocol Analyzer
   - BLE Packet Sniffer

3. **Testing Tools**
   - MCTT/PCTT Test Framework
   - UWB Certification Tools

## 📞 Support

- Technical Issues: [Link to Issue Tracker]
- Documentation: [Link to Wiki]
- Team Contact: [Contact Information]

Would you like me to expand on any particular section or add more specific details about certain components?
