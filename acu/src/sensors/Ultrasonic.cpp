#include "sensors/Ultrasonic.hpp"
#include <wiringPi.h>
#include <thread>

namespace drone {
namespace sensors {

Ultrasonic::Ultrasonic(int triggerPin, int echoPin)
    : triggerPin_(triggerPin)
    , echoPin_(echoPin)
    , lastDistance_(0.0f) {}

Ultrasonic::~Ultrasonic() {
    cleanup();
}

bool Ultrasonic::init() {
    return setupGPIO();
}

bool Ultrasonic::setupGPIO() {
    // Set pin modes
    pinMode(triggerPin_, OUTPUT);
    pinMode(echoPin_, INPUT);

    // Initialize trigger pin to low
    digitalWrite(triggerPin_, LOW);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    return true;
}

void Ultrasonic::cleanup() {
    // Reset pins to input mode
    pinMode(triggerPin_, INPUT);
    pinMode(echoPin_, INPUT);
}

float Ultrasonic::getDistance() {
    // Send trigger pulse
    sendTrigger();

    // Wait for echo and measure time
    auto echoTime = waitForEcho();

    // Calculate distance
    lastDistance_ = calculateDistance(echoTime);

    return lastDistance_;
}

void Ultrasonic::sendTrigger() {
    // Send 10us trigger pulse
    digitalWrite(triggerPin_, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin_, LOW);
}

std::chrono::microseconds Ultrasonic::waitForEcho() {
    using namespace std::chrono;

    // Convert timeout to microseconds
    auto timeoutUs = microseconds(static_cast<long>(TIMEOUT_SECONDS * 1e6));
    
    // Wait for echo to start
    auto startWait = steady_clock::now();
    while (digitalRead(echoPin_) == LOW) {
        if (duration_cast<microseconds>(steady_clock::now() - startWait) > timeoutUs) {
            return microseconds(0);
        }
    }

    // Measure echo pulse width
    auto start = steady_clock::now();
    while (digitalRead(echoPin_) == HIGH) {
        if (duration_cast<microseconds>(steady_clock::now() - start) > timeoutUs) {
            return microseconds(0);
        }
    }
    auto end = steady_clock::now();

    return duration_cast<microseconds>(end - start);
}

float Ultrasonic::calculateDistance(std::chrono::microseconds echoTime) {
    // Convert echo time to seconds
    float seconds = echoTime.count() / 1e6f;

    // Calculate distance using speed of sound
    // Divide by 2 because sound travels to target and back
    float distance = (SPEED_OF_SOUND * seconds) / 2.0f;

    // Clamp to valid range
    if (distance < MIN_DISTANCE) {
        return MIN_DISTANCE;
    } else if (distance > MAX_DISTANCE) {
        return MAX_DISTANCE;
    }

    return distance;
}

} // namespace sensors
} // namespace drone 