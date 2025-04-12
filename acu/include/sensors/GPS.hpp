#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <mutex>

namespace drone {
namespace sensors {

class GPS {
public:
    explicit GPS(const std::string& device = "/dev/ttyAMA0");
    ~GPS();

    // Lifecycle methods
    bool init();
    void start();
    void stop();

    // Data access
    struct GPSData {
        double latitude{0};
        double longitude{0};
        float altitude{0};
        float speed{0};
        int satellites{0};
        bool fix{false};
        float hdop{0};  // Horizontal dilution of precision
    };

    GPSData getData() const;
    bool hasFix() const { return data_.fix; }

private:
    // Serial port
    std::string devicePath_;
    int serialFd_;

    // GPS data
    mutable std::mutex dataMutex_;
    GPSData data_;

    // Background reading
    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> readThread_;
    void readLoop();

    // NMEA parsing
    void parseNMEA(const std::string& sentence);
    void parseGGA(const std::string& sentence);
    void parseRMC(const std::string& sentence);
    double parseLatLon(const std::string& value, char direction);

    // Utility functions
    bool openSerial();
    void closeSerial();
    std::string readLine();
};

} // namespace sensors
} // namespace drone 