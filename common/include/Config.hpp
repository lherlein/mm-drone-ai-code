#pragma once
#include <string>
#include <cstdint>

namespace drone {

struct Config {
    // Communication settings
    std::string gcu_address;
    uint16_t gcu_port;
    uint16_t local_port;

    // WiFi settings
    uint8_t wifi_channel{6};     // Default to channel 6
    std::string wifi_interface;  // Optional: specify interface name
    bool wifi_force_monitor{false}; // Force monitor mode even if already set

    // Flight controller settings
    float pid_roll_p{1.0f};
    float pid_roll_i{0.0f};
    float pid_roll_d{0.2f};
    float pid_pitch_p{1.0f};
    float pid_pitch_i{0.0f};
    float pid_pitch_d{0.2f};
    float pid_yaw_p{2.0f};
    float pid_yaw_i{0.0f};
    float pid_yaw_d{0.0f};
    float pid_altitude_p{1.0f};
    float pid_altitude_i{0.1f};
    float pid_altitude_d{0.1f};
};

} // namespace drone 