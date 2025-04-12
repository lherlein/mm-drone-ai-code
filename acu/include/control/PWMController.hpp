#pragma once

#include <cstdint>
#include <array>

namespace drone {
namespace control {

class PWMController {
public:
    // PWM channels
    enum class Channel {
        MOTOR = 0,
        ELEVATOR = 1,
        RUDDER = 2,
        AILERONS = 3
    };

    PWMController();
    ~PWMController();

    // Initialization
    bool init();

    // PWM control (input range 0-4095)
    void setOutput(Channel channel, uint16_t value);
    uint16_t getOutput(Channel channel) const;

    // Emergency stop
    void emergencyStop();

private:
    // PWM configuration
    static constexpr int PWM_FREQUENCY = 50;  // 50Hz for servos
    static constexpr int PWM_RANGE = 4096;    // 12-bit resolution
    
    // GPIO pins for each channel
    static constexpr std::array<int, 4> PWM_PINS = {18, 19, 20, 21};
    
    // Current output values
    std::array<uint16_t, 4> currentValues_;
    
    // Servo limits (in microseconds)
    static constexpr int SERVO_MIN_US = 1000;  // 1ms
    static constexpr int SERVO_MAX_US = 2000;  // 2ms
    
    // Utility functions
    void setupPWMPin(int pin);
    uint16_t scaleToPWM(uint16_t value) const;
    void writePWM(int pin, uint16_t value);
};

} // namespace control
} // namespace drone 