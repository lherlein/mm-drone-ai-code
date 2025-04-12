#pragma once

#include <string>
#include <vector>
#include <memory>
#include "Config.hpp"

namespace drone {
namespace wifi {

class WiFiSetup {
public:
    static bool initialize(const Config& config);
    static void cleanup();
    
    static const std::string& getInterface() { return selectedInterface; }
    static bool isInitialized() { return initialized; }

private:
    static bool setupWiFiInterface();
    static bool checkInterfaceCapabilities(const std::string& iface);
    static bool enableMonitorMode(const std::string& iface);
    static bool setChannel(const std::string& iface, int channel);
    static bool startPacketMonitoring(const std::string& iface);
    
    static std::string executeCommand(const std::string& cmd);
    static void logError(const std::string& msg);
    static void logWarning(const std::string& msg);
    
    static std::string selectedInterface;
    static bool initialized;
};

} // namespace wifi
} // namespace drone 