# Flight Control System
Version: 1.0.0  
Last Updated: 2024

## Table of Contents
1. [System Overview](#system-overview)
2. [Direct Control Implementation](#direct-control-implementation)
3. [Control Surface Integration](#control-surface-integration)
4. [Sensor Integration](#sensor-integration)
5. [Future IMU-Assisted Flight](#future-imu-assisted-flight)

## System Overview

The flight control system manages the drone's control surfaces based on commands from the Ground Control Unit (GCU). The system initially implements direct control mapping while providing a foundation for future IMU-assisted flight capabilities.

### Control Architecture

```mermaid
graph TB
    subgraph Ground Control Unit
        GCU[Ground Control<br>50Hz]
    end

    subgraph Flight Control System
        CP[Command Processor<br>200Hz]
        SM[Surface Manager<br>200Hz]
    end

    subgraph Control Surfaces
        TH[Thrust<br>ESC]
        EL[Elevator<br>Servo]
        RU[Rudder<br>Servo]
        AI[Ailerons<br>Servo]
    end

    GCU -->|Control Commands| CP
    CP -->|Thrust Command| TH
    CP -->|Elevator Command| EL
    CP -->|Rudder Command| RU
    CP -->|Aileron Command| AI

    classDef current fill:#f9f,stroke:#333,stroke-width:2px;
    class CP,SM current;
```

## Direct Control Implementation

### Command Structure
```mermaid
graph LR
    H[Header<br>4 bytes] --> T[Timestamp<br>8 bytes]
    T --> TH[Thrust<br>2 bytes]
    TH --> EL[Elevator<br>2 bytes]
    EL --> RU[Rudder<br>2 bytes]
    RU --> AI[Ailerons<br>2 bytes]
    AI --> CRC[Checksum<br>4 bytes]
```

### Control Ranges
| Control Surface | Range | Resolution | Update Rate |
|----------------|-------|------------|-------------|
| Thrust (ESC)   | 0-100%| 0.1%      | 200Hz       |
| Elevator       | ±30°  | 0.1°      | 200Hz       |
| Rudder         | ±30°  | 0.1°      | 200Hz       |
| Ailerons       | ±30°  | 0.1°      | 200Hz       |

### Control Flow

```mermaid
sequenceDiagram
    participant GCU as Ground Control
    participant CP as Command Processor
    participant SM as Surface Manager
    participant CS as Control Surfaces
    
    loop Every 20ms (50Hz)
        GCU->>CP: Control Commands
        CP->>CP: Validate Commands
        CP->>SM: Surface Commands
        SM->>CS: PWM Signals
    end
    
    Note over CP,SM: Running at 200Hz
    Note over SM,CS: Hardware PWM
```

## Control Surface Integration

### Surface Manager Operation

```mermaid
stateDiagram-v2
    [*] --> Initialize
    Initialize --> Idle
    
    Idle --> ProcessCommand: New Command
    ProcessCommand --> ValidateRange: Command Valid
    ProcessCommand --> Idle: Invalid Command
    
    ValidateRange --> ApplyCommand: In Range
    ValidateRange --> LimitCommand: Out of Range
    LimitCommand --> ApplyCommand
    
    ApplyCommand --> UpdatePWM
    UpdatePWM --> Idle
```

### PWM Configuration
- Frequency: 50Hz (20ms period)
- Resolution: 12-bit (4096 steps)
- Pulse Range: 1000μs - 2000μs
- Neutral Positions:
  - Thrust (ESC): 1000μs (0% power)
  - Control Surfaces (Elevator, Rudder, Ailerons): 1500μs (centered)

## Sensor Integration

### Data Collector Interface

```mermaid
graph TB
    subgraph Data Collection
        DC[Data Collector<br>200Hz]
        TB[Telemetry Buffer]
    end

    subgraph Flight Control
        CP[Command Processor]
        SM[Surface Manager]
    end

    subgraph Telemetry
        TP[Telemetry Processor<br>50Hz]
    end

    DC -->|Surface Positions| TB
    SM -->|Actual Positions| DC
    CP -->|Command History| DC
    TB -->|Flight Data| TP
```

### Telemetry Data Structure
- Current surface positions
- Command values
- Command execution timestamps
- Surface response times
- Error conditions

## Future IMU-Assisted Flight

### PID Control Overview

```mermaid
graph LR
    SP[Setpoint] --> SUM((+/-))
    FB[Feedback] --> SUM
    SUM --> PID[PID Controller]
    PID --> OUT[Output]
    OUT --> SURF[Control Surface]
    SURF --> SENS[IMU Sensor]
    SENS --> FB
```

### PID Implementation Strategy

1. **Attitude Control Loops**
```mermaid
graph TB
    subgraph Roll Control
        RP[Roll PID<br>200Hz] --> AI[Aileron Input]
        RG[Roll Gyro] --> RP
    end
    
    subgraph Pitch Control
        PP[Pitch PID<br>200Hz] --> EL[Elevator Input]
        PG[Pitch Gyro] --> PP
    end
    
    subgraph Yaw Control
        YP[Yaw PID<br>200Hz] --> RU[Rudder Input]
        YG[Yaw Gyro] --> YP
    end
```

2. **Control Parameters**
   - **Roll Control**
     - Kp: Initial attitude correction
     - Ki: Trim correction
     - Kd: Rate damping
     - Update Rate: 200Hz
   
   - **Pitch Control**
     - Kp: Initial attitude correction
     - Ki: Trim correction
     - Kd: Rate damping
     - Update Rate: 200Hz
   
   - **Yaw Control**
     - Kp: Initial heading correction
     - Ki: Trim correction
     - Kd: Rate damping
     - Update Rate: 200Hz

### IMU Data Integration

```mermaid
sequenceDiagram
    participant IMU
    participant DC as Data Collector
    participant PID as PID Controllers
    participant SM as Surface Manager
    
    loop Every 5ms (200Hz)
        IMU->>DC: Attitude Data
        DC->>PID: Processed IMU Data
        PID->>SM: Surface Commands
        SM->>DC: Surface Feedback
    end
```

### Implementation Phases

1. **Phase 1: Data Collection**
   - Collect IMU data during direct control
   - Monitor control surface response
   - Gather flight characteristics
   - No active control

2. **Phase 2: Single-Axis Stabilization**
   - Implement roll stabilization
   - PID tuning for roll axis
   - Manual control override
   - Performance validation

3. **Phase 3: Full Attitude Control**
   - Add pitch and yaw control
   - Multi-axis PID tuning
   - Mode switching logic
   - Safety monitoring

4. **Phase 4: Advanced Features**
   - Altitude hold
   - Heading hold
   - Return-to-home assistance
   - Autonomous capabilities

### Safety Considerations
- Gradual PID engagement
- Control authority limits
- Manual override capability
- Sensor failure detection
- PID output limiting
- Anti-windup protection 