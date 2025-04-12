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

    // Map joystick axes to control values (convert -1.0 to 1.0 range to 0-4095)
    currentControlData_.ailerons = static_cast<uint16_t>((joystick_->getAxis(0) + 1.0f) * 2047.5f);      // Left stick X (roll)
    currentControlData_.elevator = static_cast<uint16_t>((-joystick_->getAxis(1) + 1.0f) * 2047.5f);     // Left stick Y (pitch)
    currentControlData_.rudder = static_cast<uint16_t>((joystick_->getAxis(3) + 1.0f) * 2047.5f);        // Right stick X (yaw)
    currentControlData_.thrust = static_cast<uint16_t>((-joystick_->getAxis(4) + 1.0f) * 2047.5f);       // Right stick Y (throttle)
}

void InputManager::updateKeyboard() {
    // Implement keyboard controls if needed
    // This would typically be done through key event handlers
}

void InputManager::normalizeAxes() {
    // Convert to float for processing (-1.0 to 1.0 range)
    float ailerons = static_cast<float>(currentControlData_.ailerons) / 2047.5f - 1.0f;
    float elevator = static_cast<float>(currentControlData_.elevator) / 2047.5f - 1.0f;
    float rudder = static_cast<float>(currentControlData_.rudder) / 2047.5f - 1.0f;
    float thrust = static_cast<float>(currentControlData_.thrust) / 2047.5f - 1.0f;

    // Apply deadzone
    applyDeadzone(ailerons, DEADZONE);
    applyDeadzone(elevator, DEADZONE);
    applyDeadzone(rudder, DEADZONE);
    applyDeadzone(thrust, DEADZONE);

    // Limit thrust change rate
    static float lastThrust = 0.0f;
    float thrustDiff = thrust - lastThrust;
    if (std::abs(thrustDiff) > MAX_THROTTLE_CHANGE) {
        thrust = lastThrust + (thrustDiff > 0 ? MAX_THROTTLE_CHANGE : -MAX_THROTTLE_CHANGE);
    }
    lastThrust = thrust;

    // Convert back to uint16_t range (0-4095)
    currentControlData_.ailerons = static_cast<uint16_t>((ailerons + 1.0f) * 2047.5f);
    currentControlData_.elevator = static_cast<uint16_t>((elevator + 1.0f) * 2047.5f);
    currentControlData_.rudder = static_cast<uint16_t>((rudder + 1.0f) * 2047.5f);
    currentControlData_.thrust = static_cast<uint16_t>((thrust + 1.0f) * 2047.5f);
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