#include "sensors/SensorManager.hpp"
#include "Config.hpp"
#include <wiringPi.h>
#include <chrono>
#include <thread>

namespace drone {
namespace sensors {

SensorManager::SensorManager(const Config& config)
    : isCalibrated_(false) {
    // Initialize WiringPi
    wiringPiSetup();

    // Create sensor instances
    imu_ = std::make_unique<MPU6050>();
    gps_ = std::make_unique<GPS>();
    ultrasonic_ = std::make_unique<Ultrasonic>(13, 16);  // GPIO13 = Trigger, GPIO16 = Echo
}

SensorManager::~SensorManager() {
    stop();
}

bool SensorManager::start() {
    // Initialize all sensors
    if (!imu_->init() || !gps_->init() || !ultrasonic_->init()) {
        return false;
    }

    // Start GPS background thread
    gps_->start();

    // Perform initial calibration
    if (!performCalibration()) {
        return false;
    }

    return true;
}

void SensorManager::stop() {
    gps_->stop();
}

void SensorManager::update() {
    updateIMU();
    updateGPS();
    updateUltrasonic();
    updateBatteryVoltage();
}

bool SensorManager::performCalibration() {
    // Calibrate IMU
    if (!imu_->calibrate()) {
        return false;
    }

    // Wait for GPS fix (with timeout)
    const auto startTime = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(30);

    while (!gps_->hasFix()) {
        if (std::chrono::steady_clock::now() - startTime > timeout) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    isCalibrated_ = true;
    return true;
}

protocol::TelemetryData SensorManager::getTelemetryData() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    protocol::TelemetryData telemetry;
    
    // Copy attitude data
    telemetry.roll = sensorData_.roll;
    telemetry.pitch = sensorData_.pitch;
    telemetry.yaw = sensorData_.yaw;
    
    // Copy position data
    telemetry.latitude = sensorData_.latitude;
    telemetry.longitude = sensorData_.longitude;
    telemetry.altitude = sensorData_.altitude;
    
    // Copy battery data
    telemetry.battery_voltage = sensorData_.batteryVoltage;
    
    return telemetry;
}

float SensorManager::getAltitude() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return sensorData_.altitude;
}

float SensorManager::getBatteryVoltage() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return sensorData_.batteryVoltage;
}

void SensorManager::updateIMU() {
    if (imu_->update()) {
        std::lock_guard<std::mutex> lock(dataMutex_);
        sensorData_.roll = imu_->getRoll();
        sensorData_.pitch = imu_->getPitch();
        sensorData_.yaw = imu_->getYaw();
    }
}

void SensorManager::updateGPS() {
    auto gpsData = gps_->getData();
    if (gpsData.fix) {
        std::lock_guard<std::mutex> lock(dataMutex_);
        sensorData_.latitude = gpsData.latitude;
        sensorData_.longitude = gpsData.longitude;
        sensorData_.altitude = gpsData.altitude;
    }
}

void SensorManager::updateUltrasonic() {
    float distance = ultrasonic_->getDistance();
    if (ultrasonic_->isInRange()) {
        std::lock_guard<std::mutex> lock(dataMutex_);
        sensorData_.ultrasonicDistance = distance;
    }
}

void SensorManager::updateBatteryVoltage() {
    float voltage = readBatteryVoltage();
    std::lock_guard<std::mutex> lock(dataMutex_);
    sensorData_.batteryVoltage = voltage;
}

float SensorManager::readBatteryVoltage() {
    // Read ADC value (assuming 10-bit ADC)
    int adcValue = analogRead(BATTERY_VOLTAGE_PIN);
    
    // Convert to voltage (assuming 3.3V reference)
    float voltage = (adcValue / 1023.0f) * 3.3f;
    
    // Apply voltage divider ratio
    return voltage * VOLTAGE_DIVIDER_RATIO;
}

} // namespace sensors
} // namespace drone 