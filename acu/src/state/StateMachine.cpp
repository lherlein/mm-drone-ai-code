#include "state/StateMachine.hpp"
#include <iostream>

namespace drone {
namespace state {

StateMachine::StateMachine()
    : flightController_(nullptr)
    , sensorManager_(nullptr)
    , commManager_(nullptr)
    , currentState_(State::INITIALIZING)
    , previousState_(State::INITIALIZING)
    , lastStateChange_(std::chrono::steady_clock::now())
    , lastHeartbeat_(std::chrono::steady_clock::now()) {}

StateMachine::~StateMachine() {
    if (flightController_) {
        flightController_->emergencyStop();
    }
}

void StateMachine::update() {
    // Handle state-specific logic
    switch (currentState_) {
        case State::INITIALIZING:
            handleInitializing();
            break;
        case State::CALIBRATING:
            handleCalibrating();
            break;
        case State::IDLE:
            handleIdle();
            break;
        case State::ARMED:
            handleArmed();
            break;
        case State::FLYING:
            handleFlying();
            break;
        case State::EMERGENCY:
            handleEmergency();
            break;
        case State::ERROR:
            handleError();
            break;
    }

    // Check for emergency conditions in all states except EMERGENCY and ERROR
    if (currentState_ != State::EMERGENCY && currentState_ != State::ERROR) {
        if (!checkSensors() || !checkCommunication() || 
            !checkBattery() || !checkAttitude()) {
            setState(State::EMERGENCY);
        }
    }
}

void StateMachine::setState(State newState) {
    if (newState != currentState_) {
        previousState_ = currentState_;
        currentState_ = newState;
        lastStateChange_ = std::chrono::steady_clock::now();
        handleStateTransition();
    }
}

void StateMachine::handleStateTransition() {
    std::cout << "State transition: " 
              << static_cast<int>(previousState_) 
              << " -> " 
              << static_cast<int>(currentState_) 
              << std::endl;

    switch (currentState_) {
        case State::ARMED:
            flightController_->start();
            break;
        case State::EMERGENCY:
        case State::ERROR:
            flightController_->emergencyStop();
            break;
        default:
            break;
    }
}

void StateMachine::handleInitializing() {
    if (!flightController_ || !sensorManager_ || !commManager_) {
        setState(State::ERROR);
        return;
    }

    // Check if all components are initialized
    if (flightController_->init()) {
        setState(State::CALIBRATING);
    }
}

void StateMachine::handleCalibrating() {
    if (sensorManager_->isCalibrated()) {
        setState(State::IDLE);
    }
}

void StateMachine::handleIdle() {
    // Wait for arm command from GCU
    // Transition handled by communication manager
}

void StateMachine::handleArmed() {
    // Check if takeoff command received
    // Transition to FLYING handled by communication manager
}

void StateMachine::handleFlying() {
    // Normal flight operations
    flightController_->update();
}

void StateMachine::handleEmergency() {
    static const auto emergencyStart = std::chrono::steady_clock::now();
    auto emergencyDuration = std::chrono::steady_clock::now() - emergencyStart;

    // After emergency timeout, check if we can recover
    if (emergencyDuration > EMERGENCY_RECOVERY_TIME) {
        if (checkSensors() && checkCommunication() && 
            checkBattery() && checkAttitude()) {
            setState(State::IDLE);
        }
    }
}

void StateMachine::handleError() {
    // Terminal state - requires system restart
}

bool StateMachine::checkSensors() const {
    if (!sensorManager_) return false;
    return sensorManager_->isCalibrated();
}

bool StateMachine::checkCommunication() const {
    if (!commManager_) return false;
    
    auto now = std::chrono::steady_clock::now();
    auto heartbeatAge = now - lastHeartbeat_;
    
    return heartbeatAge <= HEARTBEAT_TIMEOUT;
}

bool StateMachine::checkBattery() const {
    if (!sensorManager_) return false;
    return sensorManager_->getBatteryVoltage() >= MIN_BATTERY_VOLTAGE;
}

bool StateMachine::checkAttitude() const {
    if (!sensorManager_) return false;
    
    auto telemetry = sensorManager_->getTelemetryData();
    return std::abs(telemetry.roll) <= MAX_SAFE_ANGLE &&
           std::abs(telemetry.pitch) <= MAX_SAFE_ANGLE;
}

} // namespace state
} // namespace drone 