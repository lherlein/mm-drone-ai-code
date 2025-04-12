#include "control/PWMController.hpp"
#include <wiringPi.h>
#include <algorithm>

namespace drone {
namespace control {

PWMController::PWMController() {
    currentValues_.fill(0);
}

PWMController::~PWMController() {
    emergencyStop();
}

bool PWMController::init() {
    // Setup all PWM pins
    for (int pin : PWM_PINS) {
        setupPWMPin(pin);
    }
    
    // Initialize all outputs to zero
    emergencyStop();
    
    return true;
}

void PWMController::setupPWMPin(int pin) {
    pinMode(pin, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);  // Mark-Space mode
    pwmSetRange(PWM_RANGE);
    pwmSetClock(19200000 / (PWM_RANGE * PWM_FREQUENCY));  // 19.2MHz clock
}

void PWMController::setOutput(Channel channel, uint16_t value) {
    // Clamp input value
    value = std::min<uint16_t>(value, 4095);
    
    // Store current value
    int channelIdx = static_cast<int>(channel);
    currentValues_[channelIdx] = value;
    
    // Convert to PWM value and write
    uint16_t pwmValue = scaleToPWM(value);
    writePWM(PWM_PINS[channelIdx], pwmValue);
}

uint16_t PWMController::getOutput(Channel channel) const {
    return currentValues_[static_cast<int>(channel)];
}

void PWMController::emergencyStop() {
    // Set all outputs to zero
    for (size_t i = 0; i < PWM_PINS.size(); ++i) {
        currentValues_[i] = 0;
        writePWM(PWM_PINS[i], 0);
    }
}

uint16_t PWMController::scaleToPWM(uint16_t value) const {
    // Scale from 0-4095 to SERVO_MIN_US-SERVO_MAX_US
    float scaled = SERVO_MIN_US + (value / 4095.0f) * (SERVO_MAX_US - SERVO_MIN_US);
    
    // Convert microseconds to PWM value
    // PWM period is 20ms (50Hz), so scale accordingly
    return static_cast<uint16_t>((scaled / 20000.0f) * PWM_RANGE);
}

void PWMController::writePWM(int pin, uint16_t value) {
    pwmWrite(pin, value);
}

} // namespace control
} // namespace drone 