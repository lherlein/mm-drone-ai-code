#include "sensors/MPU6050.hpp"
#include <wiringPiI2C.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <cmath>

namespace drone {
namespace sensors {

MPU6050::MPU6050() : i2cFd_(-1) {}

MPU6050::~MPU6050() {
    if (i2cFd_ >= 0) {
        close(i2cFd_);
    }
}

bool MPU6050::init() {
    // Initialize I2C
    i2cFd_ = wiringPiI2CSetup(MPU6050_ADDR);
    if (i2cFd_ < 0) {
        return false;
    }

    // Wake up the device
    if (!writeReg(PWR_MGMT_1, 0x00)) {
        return false;
    }

    // Configure gyroscope (±250°/s)
    if (!writeReg(GYRO_CONFIG, 0x00)) {
        return false;
    }

    // Configure accelerometer (±2g)
    if (!writeReg(ACCEL_CONFIG, 0x00)) {
        return false;
    }

    // Set DLPF to 42Hz (reduces noise)
    if (!writeReg(CONFIG, 0x03)) {
        return false;
    }

    // Wait for sensors to stabilize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return true;
}

bool MPU6050::calibrate() {
    constexpr int NUM_SAMPLES = 1000;
    float sumAx = 0, sumAy = 0, sumAz = 0;
    float sumGx = 0, sumGy = 0, sumGz = 0;

    // Collect samples
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        if (!update()) {
            return false;
        }
        sumAx += rawData_.ax;
        sumAy += rawData_.ay;
        sumAz += rawData_.az;
        sumGx += rawData_.gx;
        sumGy += rawData_.gy;
        sumGz += rawData_.gz;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    // Calculate offsets
    offsets_.ax = sumAx / NUM_SAMPLES;
    offsets_.ay = sumAy / NUM_SAMPLES;
    offsets_.az = (sumAz / NUM_SAMPLES) - 16384.0f; // Remove gravity (1g)
    offsets_.gx = sumGx / NUM_SAMPLES;
    offsets_.gy = sumGy / NUM_SAMPLES;
    offsets_.gz = sumGz / NUM_SAMPLES;

    return true;
}

bool MPU6050::update() {
    uint8_t buffer[14];
    if (!readRegs(ACCEL_XOUT_H, buffer, 14)) {
        return false;
    }

    // Combine high and low bytes
    rawData_.ax = (buffer[0] << 8) | buffer[1];
    rawData_.ay = (buffer[2] << 8) | buffer[3];
    rawData_.az = (buffer[4] << 8) | buffer[5];
    rawData_.temp = (buffer[6] << 8) | buffer[7];
    rawData_.gx = (buffer[8] << 8) | buffer[9];
    rawData_.gy = (buffer[10] << 8) | buffer[11];
    rawData_.gz = (buffer[12] << 8) | buffer[13];

    // Calculate time delta
    auto now = std::chrono::high_resolution_clock::now();
    auto nowUs = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();
    float dt = (lastUpdate_ == 0) ? 0.0f : (nowUs - lastUpdate_) / 1000000.0f;
    lastUpdate_ = nowUs;

    // Update attitude
    if (dt > 0) {
        updateAttitude(dt);
    }

    return true;
}

void MPU6050::updateAttitude(float dt) {
    // Convert raw values to physical units
    constexpr float ACCEL_SCALE = 1.0f / 16384.0f;  // For ±2g range
    constexpr float GYRO_SCALE = 1.0f / 131.0f;     // For ±250°/s range

    // Apply calibration offsets and scaling
    float ax = (rawData_.ax - offsets_.ax) * ACCEL_SCALE;
    float ay = (rawData_.ay - offsets_.ay) * ACCEL_SCALE;
    float az = (rawData_.az - offsets_.az) * ACCEL_SCALE;
    float gx = (rawData_.gx - offsets_.gx) * GYRO_SCALE;
    float gy = (rawData_.gy - offsets_.gy) * GYRO_SCALE;
    float gz = (rawData_.gz - offsets_.gz) * GYRO_SCALE;

    // Calculate accelerometer angles
    float accel_roll = atan2(ay, az) * 180.0f / M_PI;
    float accel_pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0f / M_PI;

    // Complementary filter
    roll_ = ALPHA * (roll_ + gx * dt) + (1 - ALPHA) * accel_roll;
    pitch_ = ALPHA * (pitch_ + gy * dt) + (1 - ALPHA) * accel_pitch;
    yaw_ += gz * dt;  // Simple integration for yaw
}

bool MPU6050::writeReg(uint8_t reg, uint8_t value) {
    return wiringPiI2CWriteReg8(i2cFd_, reg, value) >= 0;
}

uint8_t MPU6050::readReg(uint8_t reg) {
    return wiringPiI2CReadReg8(i2cFd_, reg);
}

bool MPU6050::readRegs(uint8_t reg, uint8_t* buffer, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        int value = wiringPiI2CReadReg8(i2cFd_, reg + i);
        if (value < 0) {
            return false;
        }
        buffer[i] = static_cast<uint8_t>(value);
    }
    return true;
}

} // namespace sensors
} // namespace drone 