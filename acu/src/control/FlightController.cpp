#include "control/FlightController.hpp"
#include <chrono>
#include <cmath>

namespace drone {
namespace control {

namespace {
    constexpr float MAX_SAFE_ANGLE = 45.0f;  // Maximum safe tilt angle in degrees
    constexpr float MIN_SAFE_VOLTAGE = 14.0f; // Minimum safe battery voltage for 4S LiPo
    constexpr float MAX_INTEGRAL = 20.0f;     // Maximum integral term for PID
    constexpr float CONTROL_RATE = 200.0f;    // Control loop rate in Hz
    constexpr float DT = 1.0f / CONTROL_RATE; // Control loop period in seconds
}

FlightController::FlightController(const Config& config)
    : sensorManager_(nullptr)
    , armed_(false)
    , emergencyMode_(false) {
    pwm_ = std::make_unique<PWMController>();
}

FlightController::~FlightController() {
    stop();
}

bool FlightController::init() {
    return pwm_->init();
}

void FlightController::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!armed_ && !emergencyMode_) {
        armed_ = true;
    }
}

void FlightController::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    armed_ = false;
    emergencyStop();
}

void FlightController::update() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!armed_ || emergencyMode_) {
        return;
    }

    // Perform safety checks
    if (!performSafetyChecks()) {
        handleSafetyViolation();
        return;
    }

    // Update control loops
    updateAttitudeControl();
    updateAltitudeControl();
}

void FlightController::setControlInputs(const protocol::ControlData& control) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!armed_ || emergencyMode_) {
        return;
    }

    // Convert control inputs from raw values to physical units
    target_.roll = (control.ailerons - 2048) * (MAX_SAFE_ANGLE / 2048.0f);
    target_.pitch = (control.elevator - 2048) * (MAX_SAFE_ANGLE / 2048.0f);
    target_.yaw = (control.rudder - 2048) * (180.0f / 2048.0f);
    target_.thrust = control.thrust;
}

void FlightController::emergencyStop() {
    pwm_->emergencyStop();
    emergencyMode_ = true;
    armed_ = false;
}

protocol::TelemetryData FlightController::getTelemetryData() const {
    if (sensorManager_) {
        auto telemetry = sensorManager_->getTelemetryData();
        
        // Add control surface positions
        telemetry.thrust_actual = pwm_->getOutput(PWMController::Channel::MOTOR);
        telemetry.elevator_actual = pwm_->getOutput(PWMController::Channel::ELEVATOR);
        telemetry.rudder_actual = pwm_->getOutput(PWMController::Channel::RUDDER);
        telemetry.ailerons_actual = pwm_->getOutput(PWMController::Channel::AILERONS);
        
        return telemetry;
    }
    return protocol::TelemetryData{};
}

void FlightController::updateAttitudeControl() {
    auto telemetry = sensorManager_->getTelemetryData();

    // Calculate errors
    float rollError = target_.roll - telemetry.roll;
    float pitchError = target_.pitch - telemetry.pitch;
    float yawError = target_.yaw - telemetry.yaw;

    // Update PID controllers
    float rollOutput = updatePID(pidState_.roll, rollError, 
                               pid_.rollKp, pid_.rollKi, pid_.rollKd, DT);
    float pitchOutput = updatePID(pidState_.pitch, pitchError,
                                pid_.pitchKp, pid_.pitchKi, pid_.pitchKd, DT);
    float yawOutput = updatePID(pidState_.yaw, yawError,
                              pid_.yawKp, pid_.yawKi, pid_.yawKd, DT);

    // Apply outputs
    applyMotorOutputs(rollOutput, pitchOutput, yawOutput, target_.thrust);
}

void FlightController::updateAltitudeControl() {
    if (target_.thrust < 100) {  // Below minimum throttle, disable altitude hold
        return;
    }

    float currentAlt = sensorManager_->getAltitude();
    float altError = target_.altitude - currentAlt;

    float altitudeOutput = updatePID(pidState_.altitude, altError,
                                   pid_.altitudeKp, pid_.altitudeKi, pid_.altitudeKd, DT);

    // Adjust thrust based on altitude control
    uint16_t adjustedThrust = static_cast<uint16_t>(target_.thrust + altitudeOutput);
    adjustedThrust = std::min<uint16_t>(adjustedThrust, 4095);
    pwm_->setOutput(PWMController::Channel::MOTOR, adjustedThrust);
}

float FlightController::updatePID(PIDState& state, float error, 
                                float kp, float ki, float kd, float dt) {
    // Proportional term
    float p = kp * error;

    // Integral term with anti-windup
    state.integral += error * dt;
    state.integral = std::max(-MAX_INTEGRAL, std::min(MAX_INTEGRAL, state.integral));
    float i = ki * state.integral;

    // Derivative term
    float d = kd * (error - state.lastError) / dt;
    state.lastError = error;

    return p + i + d;
}

void FlightController::applyMotorOutputs(float rollOutput, float pitchOutput,
                                       float yawOutput, float thrust) {
    // Convert control outputs to servo positions (centered at 2048)
    uint16_t ailerons = static_cast<uint16_t>(2048 + rollOutput);
    uint16_t elevator = static_cast<uint16_t>(2048 + pitchOutput);
    uint16_t rudder = static_cast<uint16_t>(2048 + yawOutput);

    // Clamp values to valid range
    ailerons = std::min<uint16_t>(std::max<uint16_t>(ailerons, 0), 4095);
    elevator = std::min<uint16_t>(std::max<uint16_t>(elevator, 0), 4095);
    rudder = std::min<uint16_t>(std::max<uint16_t>(rudder, 0), 4095);

    // Apply outputs
    pwm_->setOutput(PWMController::Channel::AILERONS, ailerons);
    pwm_->setOutput(PWMController::Channel::ELEVATOR, elevator);
    pwm_->setOutput(PWMController::Channel::RUDDER, rudder);
    pwm_->setOutput(PWMController::Channel::MOTOR, static_cast<uint16_t>(thrust));
}

bool FlightController::performSafetyChecks() const {
    if (!sensorManager_) {
        return false;
    }

    auto telemetry = sensorManager_->getTelemetryData();

    // Check battery voltage
    if (telemetry.battery_voltage < MIN_SAFE_VOLTAGE) {
        return false;
    }

    // Check attitude limits
    if (std::abs(telemetry.roll) > MAX_SAFE_ANGLE ||
        std::abs(telemetry.pitch) > MAX_SAFE_ANGLE) {
        return false;
    }

    return true;
}

void FlightController::handleSafetyViolation() {
    emergencyStop();
}

} // namespace control
} // namespace drone 