#pragma once

#include "protocol/Types.hpp"
#include <cstdint>
#include <vector>
#include <chrono>
#include <array>
#include <stdexcept>

namespace drone {
namespace protocol {

// Magic number for packet identification
constexpr uint32_t PACKET_MAGIC = 0x44524F4E; // "DRON" in ASCII

// Packet types
enum class PacketType : uint8_t {
    CONTROL = 0x01,
    TELEMETRY = 0x02,
    HEARTBEAT = 0x03,
    CONFIG = 0x04
};

// Packet header structure
struct PacketHeader {
    uint32_t magic;      // Magic number for packet identification
    uint8_t version;     // Protocol version
    PacketType type;     // Packet type
    uint16_t length;     // Length of payload in bytes
    uint32_t timestamp;  // Milliseconds since epoch
    uint32_t crc;        // CRC32 of payload
};

// Control data structure (8 bytes)
struct ControlData {
    uint16_t thrust;    // 0-4095 (12-bit resolution)
    uint16_t elevator;  // 0-4095
    uint16_t rudder;    // 0-4095
    uint16_t ailerons;  // 0-4095
};

// Telemetry data structure (36 bytes)
struct TelemetryData {
    // Attitude (12 bytes)
    float roll;
    float pitch;
    float yaw;
    
    // Position (12 bytes)
    float latitude;
    float longitude;
    float altitude;
    
    // Control status (8 bytes)
    uint16_t thrust_actual;
    uint16_t elevator_actual;
    uint16_t rudder_actual;
    uint16_t ailerons_actual;
    
    // Battery (4 bytes)
    float battery_voltage;
};

// Heartbeat data structure (2 bytes)
struct HeartbeatData {
    uint16_t status;  // Status flags
};

class Packet {
public:
    // Factory methods for creating packets
    static Packet createControl(const ControlData& data);
    static Packet createTelemetry(const TelemetryData& data);
    static Packet createHeartbeat(const HeartbeatData& data);
    static Packet createConfig(const ConfigData& data);

    // Deserialization
    static Packet deserialize(const uint8_t* data, size_t size);
    
    // Validation
    bool validate() const;
    bool isStale(std::chrono::milliseconds maxAge) const;
    
    // Accessors
    PacketType getType() const { return header_.type; }
    uint32_t getTimestamp() const { return header_.timestamp; }
    
    // Data accessors
    const ControlData& getControlData() const;
    const TelemetryData& getTelemetryData() const;
    const HeartbeatData& getHeartbeatData() const;
    const ConfigData& getConfigData() const;
    
    // Serialization
    std::vector<uint8_t> serialize() const;

private:
    PacketHeader header_;
    std::vector<uint8_t> payload_;
    mutable ControlData control_data_;
    mutable TelemetryData telemetry_data_;
    mutable HeartbeatData heartbeat_data_;
    mutable ConfigData config_data_;
    mutable bool data_deserialized_ = false;

    void deserializeDataIfNeeded() const;
    
    // Private constructor used by factory methods
    Packet(PacketType type, const std::vector<uint8_t>& payload);
    
    // CRC calculation
    static uint32_t calculateCRC(const uint8_t* data, size_t size);
};

} // namespace protocol
} // namespace drone 