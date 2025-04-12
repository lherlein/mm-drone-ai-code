#pragma once

#include "control/FlightController.hpp"
#include "sensors/SensorManager.hpp"
#include "communication/CommunicationManager.hpp"
#include <chrono>

namespace drone {
namespace state {

class StateMachine {
public:
    // Drone states
    enum class State {
        INITIALIZING,
        CALIBRATING,
        IDLE,
        ARMED,
        FLYING,
        EMERGENCY,
        ERROR
    };

    StateMachine();
    ~StateMachine();

    // State management
    void update();
    State getState() const { return currentState_; }
    
    // Component setters
    void setFlightController(control::FlightController* controller) {
        flightController_ = controller;
    }
    void setSensorManager(sensors::SensorManager* manager) {
        sensorManager_ = manager;
    }
    void setCommunicationManager(communication::CommunicationManager* manager) {
        commManager_ = manager;
    }

private:
    // Components
    control::FlightController* flightController_;
    sensors::SensorManager* sensorManager_;
    communication::CommunicationManager* commManager_;

    // State tracking
    State currentState_;
    State previousState_;
    std::chrono::steady_clock::time_point lastStateChange_;
    std::chrono::steady_clock::time_point lastHeartbeat_;

    // State transition methods
    void setState(State newState);
    void handleStateTransition();

    // State handlers
    void handleInitializing();
    void handleCalibrating();
    void handleIdle();
    void handleArmed();
    void handleFlying();
    void handleEmergency();
    void handleError();

    // Safety checks
    bool checkSensors() const;
    bool checkCommunication() const;
    bool checkBattery() const;
    bool checkAttitude() const;

    // Constants
    static constexpr auto HEARTBEAT_TIMEOUT = std::chrono::milliseconds(500);
    static constexpr auto EMERGENCY_RECOVERY_TIME = std::chrono::seconds(5);
    static constexpr float MIN_BATTERY_VOLTAGE = 14.0f;  // 4S LiPo minimum
    static constexpr float MAX_SAFE_ANGLE = 45.0f;       // Maximum safe tilt angle
};

} // namespace state
} // namespace drone 