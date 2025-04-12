#pragma once

#include <cstdint>
#include <chrono>

namespace drone {
namespace sensors {

class Ultrasonic {
public:
    Ultrasonic(int triggerPin, int echoPin);
    ~Ultrasonic();

    // Initialization
    bool init();

    // Measurement
    float getDistance();  // Returns distance in meters
    bool isInRange() const { return lastDistance_ >= MIN_DISTANCE && lastDistance_ <= MAX_DISTANCE; }

private:
    // GPIO pins
    const int triggerPin_;
    const int echoPin_;

    // Constants
    static constexpr float SPEED_OF_SOUND = 343.0f;  // meters per second at 20°C
    static constexpr float MIN_DISTANCE = 0.02f;     // 2cm minimum range
    static constexpr float MAX_DISTANCE = 4.0f;      // 4m maximum range
    static constexpr float TIMEOUT_SECONDS = 0.025f; // 25ms timeout

    // Last measurement
    float lastDistance_;

    // GPIO operations
    bool setupGPIO();
    void cleanup();
    void sendTrigger();
    std::chrono::microseconds waitForEcho();

    // Utility functions
    float calculateDistance(std::chrono::microseconds echoTime);
};

} // namespace sensors
} // namespace drone 