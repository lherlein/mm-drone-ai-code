# UDP Protocol Specification
Version: 1.0.0  
Last Updated: 2024

## Table of Contents
1. [Introduction to UDP](#introduction-to-udp)
2. [Protocol Overview](#protocol-overview)
3. [Packet Structures](#packet-structures)
4. [Communication Patterns](#communication-patterns)
5. [Reliability Mechanisms](#reliability-mechanisms)
6. [Implementation Guide](#implementation-guide)

## Introduction to UDP

### What is UDP?
User Datagram Protocol (UDP) is a lightweight, connectionless networking protocol. Think of it as sending postcards through the mail:
- No guarantee of delivery
- No guarantee of order
- No built-in error checking
- No connection setup required

### Why UDP for Drone Control?
```mermaid
graph LR
    subgraph Advantages
        A1[Low Latency]
        A2[Simple Implementation]
        A3[No Connection Overhead]
    end
    
    subgraph Disadvantages
        D1[No Delivery Guarantee]
        D2[No Order Guarantee]
        D3[No Flow Control]
    end
    
    subgraph Our Solutions
        S1[Latest Packet Priority]
        S2[Checksum Verification]
        S3[Heartbeat System]
    end
    
    D1 --> S1
    D2 --> S1
    D3 --> S3
```

## Protocol Overview

### Communication Layers

```mermaid
graph TB
    subgraph Application Layer
        AL[Custom Drone Protocol]
    end
    
    subgraph Transport Layer
        TL[UDP Protocol<br>Port 5760]
    end
    
    subgraph Network Layer
        NL[IP Protocol<br>Monitor Mode]
    end
    
    subgraph Link Layer
        LL[WiFi<br>RTL8812AU]
    end
    
    AL --> TL
    TL --> NL
    NL --> LL
```

### Basic Packet Flow

```mermaid
sequenceDiagram
    participant GCU as Ground Control
    participant Air as Drone
    
    Note over GCU,Air: Normal Operation (50Hz)
    GCU->>Air: Control Packet
    Air->>GCU: Telemetry Packet
    
    Note over GCU,Air: Heartbeat (2Hz)
    GCU->>Air: Heartbeat Packet
    Air->>GCU: Heartbeat Response
```

## Packet Structures

### Common Header Format
```mermaid
graph LR
    M[Magic Number<br>0xAA55<br>2 bytes] --> V[Version<br>2 bytes]
    V --> T[Type<br>1 byte]
    T --> L[Length<br>1 byte]
    L --> TS[Timestamp<br>8 bytes]
```

### 1. Control Packet (Type 0x01)
```mermaid
graph LR
    H[Header<br>18 bytes] --> C[Control Data<br>8 bytes]
    C --> CRC[CRC32<br>4 bytes]
    
    subgraph Control Data Structure
        T[Thrust<br>2 bytes]
        E[Elevator<br>2 bytes]
        R[Rudder<br>2 bytes]
        A[Ailerons<br>2 bytes]
    end
```

### 2. Telemetry Packet (Type 0x02)
```mermaid
graph LR
    H[Header<br>18 bytes] --> T[Telemetry Data<br>Variable]
    T --> CRC[CRC32<br>4 bytes]
    
    subgraph Telemetry Data Structure
        ATT[Attitude<br>12 bytes]
        POS[Position<br>12 bytes]
        CTRL[Control Status<br>8 bytes]
        BAT[Battery<br>4 bytes]
    end
```

### 3. Heartbeat Packet (Type 0x03)
```mermaid
graph LR
    H[Header<br>18 bytes] --> S[Status<br>2 bytes]
    S --> CRC[CRC32<br>4 bytes]
```

## Communication Patterns

### Normal Operation Mode

```mermaid
sequenceDiagram
    participant GCU
    participant Drone
    
    rect rgb(200, 255, 200)
    note right of GCU: Control Cycle (20ms)
    GCU->>Drone: Control Packet
    Drone->>GCU: Telemetry Packet
    end
    
    rect rgb(200, 200, 255)
    note right of GCU: Heartbeat Cycle (500ms)
    GCU->>Drone: Heartbeat
    Drone->>GCU: Heartbeat Response
    end
```

### Packet Processing Flow

```mermaid
stateDiagram-v2
    [*] --> Receive
    Receive --> ValidateHeader: New Packet
    ValidateHeader --> ProcessPayload: Valid Header
    ValidateHeader --> Discard: Invalid Header
    
    ProcessPayload --> ValidateCRC: Payload Complete
    ValidateCRC --> HandlePacket: CRC Valid
    ValidateCRC --> Discard: CRC Invalid
    
    HandlePacket --> UpdateState: Control/Config
    HandlePacket --> SendResponse: Requires Response
    
    UpdateState --> [*]
    SendResponse --> [*]
    Discard --> [*]
```

## Reliability Mechanisms

### Real-time Processing Priority

```mermaid
graph TB
    subgraph Sender
        S1[Generate Timestamp]
        S2[Transmit Immediately]
    end
    
    subgraph Receiver
        R1[Validate Timestamp]
        R2[Process if Latest]
        R3[Discard if Stale]
    end
    
    S1 --> S2
    S2 --> R1
    R1 --> R2
    R1 --> R3
```

### Error Detection

1. **Timestamp Validation**
   ```mermaid
   graph LR
       T[Timestamp Check] --> V[Validate Freshness]
       V --> P[Process/Discard]
   ```

2. **Real-time Processing**
   - Immediate processing of fresh packets
   - No packet ordering or buffering
   - Stale packet detection and discard
   - Microsecond timestamp precision

### Connection Monitoring

```mermaid
stateDiagram-v2
    [*] --> Connected: Valid Heartbeat
    Connected --> Warning: Missing Heartbeat
    Warning --> Disconnected: Timeout (1500ms)
    Warning --> Connected: Heartbeat Resumed
    Disconnected --> [*]: Emergency Protocol
```

## Implementation Guide

### Packet Assembly Example (Python-like Pseudocode)
```python
def create_control_packet(thrust, elevator, rudder, ailerons):
    packet = bytearray()
    
    # Header (14 bytes)
    packet.extend(MAGIC_NUMBER)        # 2 bytes
    packet.extend(PROTOCOL_VERSION)    # 2 bytes
    packet.append(PACKET_TYPE_CONTROL) # 1 byte
    packet.append(CONTROL_LENGTH)      # 1 byte
    packet.extend(get_timestamp())     # 8 bytes
    
    # Control Data (8 bytes)
    packet.extend(struct.pack('H', thrust))    # 2 bytes
    packet.extend(struct.pack('H', elevator))  # 2 bytes
    packet.extend(struct.pack('H', rudder))    # 2 bytes
    packet.extend(struct.pack('H', ailerons))  # 2 bytes
    
    # CRC32 (4 bytes)
    crc = calculate_crc32(packet)
    packet.extend(struct.pack('I', crc))
    
    return packet
```

### Receiving Process
1. **Packet Processing**
   ```mermaid
   graph TB
       R[Receive] --> T[Timestamp Check]
       T --> F{Fresh?}
       F -->|Yes| P[Process]
       F -->|No| D[Discard]
   ```

2. **Validation Steps**
   - Magic number check
   - Version compatibility
   - Timestamp freshness
   - CRC validation

### Error Handling
1. **Packet Processing**
   - Immediate processing of fresh packets
   - No reordering or buffering
   - Stale packet detection
   - Latest data priority

2. **Corruption**
   - CRC32 verification
   - Header validation
   - Length checking
   - Type validation

3. **Timing**
   - Timestamp validation
   - Freshness window
   - Stale packet rejection

### Best Practices
1. **Real-time Processing**
   - Zero-buffering policy
   - Immediate validation
   - Direct processing pipeline
   - Minimal overhead

2. **Timing**
   - Use monotonic clock
   - Maintain time sync
   - Microsecond precision
   - Rolling freshness window

3. **Error Recovery**
   - Immediate error detection
   - No retry mechanism
   - Latest packet priority
   - Fast failure detection

### Error Handling
1. **Packet Loss**
   - Use sequence gaps to detect
   - Implement packet request system
   - Maintain last known good state

2. **Corruption**
   - CRC32 verification
   - Header validation
   - Length checking
   - Type validation

3. **Out of Order**
   - Sequence number tracking
   - Reordering buffer
   - Timeout handling

### Best Practices
1. **Buffer Management**
   - Pre-allocate buffers
   - Implement ring buffers
   - Handle buffer overflow

2. **Timing**
   - Use monotonic clock
   - Handle wraparound
   - Maintain precision

3. **Error Recovery**
   - Implement backoff
   - State reconciliation
   - Connection re-establishment 