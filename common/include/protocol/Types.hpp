#pragma once

#include <cstdint>
#include <array>

namespace drone {
namespace protocol {

// Control data structure (from GCU to ACU)
struct ControlData {
    uint16_t ailerons;    // Roll control (0-4095)
    uint16_t elevator;    // Pitch control (0-4095)
    uint16_t rudder;      // Yaw control (0-4095)
    uint16_t thrust;      // Throttle control (0-4095)
    uint16_t aux1;        // Auxiliary channel 1
    uint16_t aux2;        // Auxiliary channel 2
    uint32_t timestamp;   // Milliseconds since epoch
    bool armed;           // Arming state
    bool emergency_stop;  // Emergency stop flag
};

// Telemetry data structure (from ACU to GCU)
struct TelemetryData {
    // Attitude
    float roll;           // Degrees
    float pitch;          // Degrees
    float yaw;           // Degrees
    
    // Position
    double latitude;      // Degrees
    double longitude;     // Degrees
    float altitude;       // Meters above sea level
    float relative_alt;   // Meters above ground
    
    // Velocities
    float vx;            // m/s in body frame
    float vy;            // m/s in body frame
    float vz;            // m/s in body frame
    
    // System status
    float battery_voltage;    // Volts
    float battery_current;    // Amperes
    uint8_t battery_remaining;// Percentage
    
    // Control surface positions
    uint16_t thrust_actual;   // 0-4095
    uint16_t elevator_actual; // 0-4095
    uint16_t rudder_actual;   // 0-4095
    uint16_t ailerons_actual; // 0-4095
    
    uint32_t timestamp;       // Milliseconds since epoch
};

// Heartbeat data structure (bidirectional)
struct HeartbeatData {
    uint32_t timestamp;       // Milliseconds since epoch
    uint16_t cpu_load;        // Percentage * 100
    uint16_t ram_usage;       // Percentage * 100
    uint32_t uptime;         // Seconds
};

// Configuration data structure
struct ConfigData {
    float pid_gains[12];     // PID gains for various control loops
    uint16_t control_rates[4];// Control rates for different axes
    uint16_t filters[4];     // Filter settings
    uint8_t mode;           // Flight mode
    uint8_t flags;          // Configuration flags
};

} // namespace protocol
} // namespace drone 