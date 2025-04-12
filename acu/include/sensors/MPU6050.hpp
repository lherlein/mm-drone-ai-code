#pragma once

#include <cstdint>
#include <array>

namespace drone {
namespace sensors {

class MPU6050 {
public:
    MPU6050();
    ~MPU6050();

    // Initialization and calibration
    bool init();
    bool calibrate();

    // Data reading
    bool update();
    float getRoll() const { return roll_; }
    float getPitch() const { return pitch_; }
    float getYaw() const { return yaw_; }

    // Raw data access
    struct RawData {
        int16_t ax, ay, az;    // Accelerometer
        int16_t gx, gy, gz;    // Gyroscope
        int16_t temp;          // Temperature
    };
    RawData getRawData() const { return rawData_; }

private:
    // I2C communication
    static constexpr uint8_t MPU6050_ADDR = 0x68;
    int i2cFd_;
    bool writeReg(uint8_t reg, uint8_t value);
    uint8_t readReg(uint8_t reg);
    bool readRegs(uint8_t reg, uint8_t* buffer, size_t length);

    // Sensor registers
    static constexpr uint8_t ACCEL_XOUT_H = 0x3B;
    static constexpr uint8_t GYRO_XOUT_H = 0x43;
    static constexpr uint8_t PWR_MGMT_1 = 0x6B;
    static constexpr uint8_t CONFIG = 0x1A;
    static constexpr uint8_t GYRO_CONFIG = 0x1B;
    static constexpr uint8_t ACCEL_CONFIG = 0x1C;

    // Data processing
    void processIMUData();
    void updateAttitude(float dt);

    // Sensor data
    RawData rawData_;
    float roll_{0}, pitch_{0}, yaw_{0};
    
    // Calibration offsets
    struct {
        float ax{0}, ay{0}, az{0};
        float gx{0}, gy{0}, gz{0};
    } offsets_;

    // Complementary filter coefficient
    static constexpr float ALPHA = 0.96f;

    // Previous update timestamp for dt calculation
    uint64_t lastUpdate_{0};
};

} // namespace sensors
} // namespace drone 