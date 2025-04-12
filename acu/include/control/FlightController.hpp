#pragma once

#include "Config.hpp"
#include "control/PWMController.hpp"
#include "sensors/SensorManager.hpp"
#include "protocol/Packet.hpp"
#include <memory>
#include <mutex>

namespace drone {
namespace control {

class FlightController {
public:
    explicit FlightController(const Config& config);
    ~FlightController();

    // Lifecycle methods
    bool init();
    void start();
    void stop();
    void update();

    // Control methods
    void setControlInputs(const protocol::ControlData& control);
    void emergencyStop();

    // Status
    bool isArmed() const { return armed_; }
    protocol::TelemetryData getTelemetryData() const;

    // Component setters
    void setSensorManager(sensors::SensorManager* manager) { sensorManager_ = manager; }

private:
    // Components
    std::unique_ptr<PWMController> pwm_;
    sensors::SensorManager* sensorManager_;

    // State
    bool armed_;
    bool emergencyMode_;

    // Control parameters
    struct {
        float rollKp{1.0f}, rollKi{0.0f}, rollKd{0.2f};
        float pitchKp{1.0f}, pitchKi{0.0f}, pitchKd{0.2f};
        float yawKp{2.0f}, yawKi{0.0f}, yawKd{0.0f};
        float altitudeKp{1.0f}, altitudeKi{0.1f}, altitudeKd{0.1f};
    } pid_;

    // Control targets
    struct {
        float roll{0}, pitch{0}, yaw{0};
        float altitude{0};
        uint16_t thrust{0};
    } target_;

    // PID state
    struct PIDState {
        float lastError{0};
        float integral{0};
    };

    struct {
        PIDState roll, pitch, yaw, altitude;
    } pidState_;

    // Thread safety
    mutable std::mutex mutex_;

    // Control methods
    void updateAttitudeControl();
    void updateAltitudeControl();
    void applyMotorOutputs(float rollOutput, float pitchOutput, 
                          float yawOutput, float altitudeOutput);

    // PID control
    float updatePID(PIDState& state, float error, float kp, float ki, float kd, float dt);

    // Safety checks
    bool performSafetyChecks() const;
    void handleSafetyViolation();
};

} // namespace control
} // namespace drone 