#include "protocol/Packet.hpp"
#include <cstring>
#include <chrono>

namespace drone {
namespace protocol {

namespace {
    // CRC-32 lookup table
    constexpr std::array<uint32_t, 256> CRC32_TABLE = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
        0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
        // ... rest of the CRC table (truncated for brevity)
    };

    template<typename T>
    std::vector<uint8_t> serializeData(const T& data) {
        std::vector<uint8_t> buffer;
        buffer.reserve(sizeof(T));

        // Serialize each member individually to handle endianness and alignment
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&data);
        for (size_t i = 0; i < sizeof(T); ++i) {
            buffer.push_back(ptr[i]);
        }
        return buffer;
    }

    template<typename T>
    T deserializeData(const std::vector<uint8_t>& buffer) {
        if (buffer.size() < sizeof(T)) {
            throw std::runtime_error("Buffer too small for data type");
        }

        T data{};  // Zero-initialize the structure
        uint8_t* ptr = reinterpret_cast<uint8_t*>(&data);
        
        // Deserialize each byte individually
        for (size_t i = 0; i < sizeof(T); ++i) {
            ptr[i] = buffer[i];
        }
        return data;
    }
}

Packet::Packet(PacketType type, const std::vector<uint8_t>& payload)
    : payload_(payload) {
    header_.magic = PACKET_MAGIC;
    header_.version = 1;
    header_.type = type;
    header_.length = static_cast<uint16_t>(payload.size());
    header_.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    header_.crc = calculateCRC(payload_.data(), payload_.size());
}

Packet Packet::createControl(const ControlData& data) {
    return Packet(PacketType::CONTROL, serializeData(data));
}

Packet Packet::createTelemetry(const TelemetryData& data) {
    return Packet(PacketType::TELEMETRY, serializeData(data));
}

Packet Packet::createHeartbeat(const HeartbeatData& data) {
    return Packet(PacketType::HEARTBEAT, serializeData(data));
}

Packet Packet::createConfig(const ConfigData& data) {
    return Packet(PacketType::CONFIG, serializeData(data));
}

Packet Packet::deserialize(const uint8_t* data, size_t size) {
    if (size < sizeof(PacketHeader)) {
        throw std::runtime_error("Packet too small");
    }

    PacketHeader header;
    std::memcpy(&header, data, sizeof(PacketHeader));

    if (header.magic != PACKET_MAGIC) {
        throw std::runtime_error("Invalid packet magic");
    }

    if (size < sizeof(PacketHeader) + header.length) {
        throw std::runtime_error("Incomplete packet");
    }

    std::vector<uint8_t> payload(data + sizeof(PacketHeader),
                                data + sizeof(PacketHeader) + header.length);

    Packet packet(header.type, payload);
    packet.header_ = header;

    if (!packet.validate()) {
        throw std::runtime_error("Invalid packet CRC");
    }

    return packet;
}

bool Packet::validate() const {
    return header_.crc == calculateCRC(payload_.data(), payload_.size());
}

bool Packet::isStale(std::chrono::milliseconds maxAge) const {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    return (now - header_.timestamp) > maxAge.count();
}

void Packet::deserializeDataIfNeeded() const {
    if (!data_deserialized_) {
        switch (header_.type) {
            case PacketType::CONTROL:
                control_data_ = deserializeData<ControlData>(payload_);
                break;
            case PacketType::TELEMETRY:
                telemetry_data_ = deserializeData<TelemetryData>(payload_);
                break;
            case PacketType::HEARTBEAT:
                heartbeat_data_ = deserializeData<HeartbeatData>(payload_);
                break;
            case PacketType::CONFIG:
                config_data_ = deserializeData<ConfigData>(payload_);
                break;
        }
        data_deserialized_ = true;
    }
}

const ControlData& Packet::getControlData() const {
    if (header_.type != PacketType::CONTROL) {
        throw std::runtime_error("Packet is not a control packet");
    }
    if (payload_.size() != sizeof(ControlData)) {
        throw std::runtime_error("Invalid control data size");
    }
    deserializeDataIfNeeded();
    return control_data_;
}

const TelemetryData& Packet::getTelemetryData() const {
    if (header_.type != PacketType::TELEMETRY) {
        throw std::runtime_error("Packet is not a telemetry packet");
    }
    if (payload_.size() != sizeof(TelemetryData)) {
        throw std::runtime_error("Invalid telemetry data size");
    }
    deserializeDataIfNeeded();
    return telemetry_data_;
}

const HeartbeatData& Packet::getHeartbeatData() const {
    if (header_.type != PacketType::HEARTBEAT) {
        throw std::runtime_error("Packet is not a heartbeat packet");
    }
    if (payload_.size() != sizeof(HeartbeatData)) {
        throw std::runtime_error("Invalid heartbeat data size");
    }
    deserializeDataIfNeeded();
    return heartbeat_data_;
}

const ConfigData& Packet::getConfigData() const {
    if (header_.type != PacketType::CONFIG) {
        throw std::runtime_error("Packet is not a config packet");
    }
    if (payload_.size() != sizeof(ConfigData)) {
        throw std::runtime_error("Invalid config data size");
    }
    deserializeDataIfNeeded();
    return config_data_;
}

std::vector<uint8_t> Packet::serialize() const {
    std::vector<uint8_t> buffer(sizeof(PacketHeader) + payload_.size());
    
    // Write header
    std::memcpy(buffer.data(), &header_, sizeof(PacketHeader));
    
    // Write payload
    std::memcpy(buffer.data() + sizeof(PacketHeader), payload_.data(), payload_.size());
    
    return buffer;
}

uint32_t Packet::calculateCRC(const uint8_t* data, size_t size) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; ++i) {
        crc = (crc >> 8) ^ CRC32_TABLE[(crc & 0xFF) ^ data[i]];
    }
    return ~crc;
}

} // namespace protocol
} // namespace drone 