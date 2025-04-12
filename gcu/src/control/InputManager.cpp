#include "control/InputManager.hpp"
#include "control/Joystick.hpp"
#include <QKeyEvent>
#include <cmath>

namespace drone {
namespace gcu {
namespace control {

namespace {
    constexpr float DEADZONE = 0.1f;
    constexpr float MAX_THROTTLE_CHANGE = 0.1f; // Maximum throttle change per update
}

InputManager::InputManager(QObject* parent)
    : QObject(parent)
    , joystick_(nullptr)
    , lastUpdate_(std::chrono::steady_clock::now()) {
    // Initialize control data
    currentControlData_ = {};
}

InputManager::~InputManager() = default;

bool InputManager::init() {
    joystick_ = std::make_unique<Joystick>();
    return joystick_->init();
}

void InputManager::update() {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration_cast<std::chrono::duration<float>>(
        now - lastUpdate_).count();
    lastUpdate_ = now;

    // Update inputs
    updateJoystick();
    updateKeyboard();

    // Normalize and apply deadzone
    normalizeAxes();

    // Emit updated control data
    emit controlDataChanged(currentControlData_);
}

void InputManager::updateJoystick() {
    if (!joystick_) return;

    joystick_->update();

    // Map joystick axes to control values
    currentControlData_.roll = joystick_->getAxis(0);      // Left stick X
    currentControlData_.pitch = -joystick_->getAxis(1);    // Left stick Y (inverted)
    currentControlData_.yaw = joystick_->getAxis(3);       // Right stick X
    currentControlData_.throttle = -joystick_->getAxis(4); // Right stick Y (inverted)
}

void InputManager::updateKeyboard() {
    // Implement keyboard controls if needed
    // This would typically be done through key event handlers
}

void InputManager::normalizeAxes() {
    // Apply deadzone to all axes
    applyDeadzone(currentControlData_.roll, DEADZONE);
    applyDeadzone(currentControlData_.pitch, DEADZONE);
    applyDeadzone(currentControlData_.yaw, DEADZONE);
    applyDeadzone(currentControlData_.throttle, DEADZONE);

    // Limit throttle change rate
    static float lastThrottle = 0.0f;
    float throttleDiff = currentControlData_.throttle - lastThrottle;
    if (std::abs(throttleDiff) > MAX_THROTTLE_CHANGE) {
        currentControlData_.throttle = lastThrottle + 
            (throttleDiff > 0 ? MAX_THROTTLE_CHANGE : -MAX_THROTTLE_CHANGE);
    }
    lastThrottle = currentControlData_.throttle;
}

void InputManager::applyDeadzone(float& value, float deadzone) {
    if (std::abs(value) < deadzone) {
        value = 0.0f;
    } else {
        // Normalize remaining range
        value = (value - std::copysign(deadzone, value)) / (1.0f - deadzone);
    }
}

} // namespace control
} // namespace gcu
} // namespace drone 