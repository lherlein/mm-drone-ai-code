#pragma once

#include "sensors/MPU6050.hpp"
#include "sensors/GPS.hpp"
#include "sensors/Ultrasonic.hpp"
#include "protocol/Packet.hpp"
#include <memory>
#include <mutex>

namespace drone {
namespace sensors {

class SensorManager {
public:
    explicit SensorManager(const Config& config);
    ~SensorManager();

    // Lifecycle methods
    bool start();
    void stop();
    void update();

    // Sensor data accessors
    protocol::TelemetryData getTelemetryData();
    bool isCalibrated() const { return isCalibrated_; }
    float getAltitude() const;
    float getBatteryVoltage() const;

private:
    // Sensor instances
    std::unique_ptr<MPU6050> imu_;
    std::unique_ptr<GPS> gps_;
    std::unique_ptr<Ultrasonic> ultrasonic_;

    // Calibration
    bool performCalibration();
    bool isCalibrated_;

    // Thread safety
    mutable std::mutex dataMutex_;

    // Sensor data cache
    struct {
        float roll{0}, pitch{0}, yaw{0};
        float latitude{0}, longitude{0}, altitude{0};
        float ultrasonicDistance{0};
        float batteryVoltage{0};
    } sensorData_;

    // Battery monitoring
    static constexpr float BATTERY_VOLTAGE_PIN = 4;  // ADC pin for battery voltage
    static constexpr float VOLTAGE_DIVIDER_RATIO = 11.0f;  // For 4S LiPo
    float readBatteryVoltage();

    // Update methods
    void updateIMU();
    void updateGPS();
    void updateUltrasonic();
    void updateBatteryVoltage();
};

} // namespace sensors
} // namespace drone 