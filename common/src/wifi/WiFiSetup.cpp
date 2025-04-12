#include "wifi/WiFiSetup.hpp"
#include "Config.hpp"
#include <array>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <cstdio>

namespace drone {
namespace wifi {

std::string WiFiSetup::selectedInterface;
bool WiFiSetup::initialized = false;

bool WiFiSetup::initialize(const Config& config) {
    if (initialized) {
        return true;
    }

    // 1. Detect interfaces
    if (!setupWiFiInterface()) {
        return false;
    }
    
    // 2. Enable monitor mode
    if (!enableMonitorMode(selectedInterface)) {
        return false;
    }
    
    // 3. Set channel
    if (!setChannel(selectedInterface, config.wifi_channel)) {
        return false;
    }
    
    // 4. Start packet monitoring
    if (!startPacketMonitoring(selectedInterface)) {
        return false;
    }

    initialized = true;
    return true;
}

void WiFiSetup::cleanup() {
    if (!initialized || selectedInterface.empty()) {
        return;
    }

    // Bring interface down
    executeCommand("ip link set " + selectedInterface + " down");
    
    // Reset to managed mode
    executeCommand("iw " + selectedInterface + " set type managed");
    
    // Bring interface back up
    executeCommand("ip link set " + selectedInterface + " up");
    
    initialized = false;
    selectedInterface.clear();
}

bool WiFiSetup::setupWiFiInterface() {
    // List all wireless interfaces
    std::string cmd = "iw dev | grep Interface | cut -f 2 -d\" \"";
    std::string output = executeCommand(cmd);
    
    if (output.empty()) {
        logError("No wireless interfaces found");
        return false;
    }

    // Split output into interface names
    std::vector<std::string> interfaces;
    size_t pos = 0;
    while ((pos = output.find('\n')) != std::string::npos) {
        std::string iface = output.substr(0, pos);
        if (!iface.empty()) {
            interfaces.push_back(iface);
        }
        output.erase(0, pos + 1);
    }
    if (!output.empty()) {
        interfaces.push_back(output);
    }

    // Find suitable interface
    for (const auto& iface : interfaces) {
        if (checkInterfaceCapabilities(iface)) {
            selectedInterface = iface;
            return true;
        }
    }
    
    logError("No suitable wireless interface found");
    return false;
}

bool WiFiSetup::checkInterfaceCapabilities(const std::string& iface) {
    // Check if interface supports monitor mode
    std::string cmd = "iw " + iface + " info | grep \"Supported interface modes\" -A 8";
    std::string output = executeCommand(cmd);
    
    if (output.find("* monitor") == std::string::npos) {
        logWarning("Interface " + iface + " does not support monitor mode");
        return false;
    }
    
    return true;
}

bool WiFiSetup::enableMonitorMode(const std::string& iface) {
    std::vector<std::string> commands = {
        "ip link set " + iface + " down",
        "iw " + iface + " set monitor none",
        "ip link set " + iface + " up"
    };
    
    for (const auto& cmd : commands) {
        if (!executeCommand(cmd).empty()) {
            logError("Failed to execute: " + cmd);
            return false;
        }
    }
    
    // Verify mode was set
    std::string check = "iw " + iface + " info | grep type | cut -d' ' -f2";
    std::string mode = executeCommand(check);
    if (mode.find("monitor") == std::string::npos) {
        logError("Failed to verify monitor mode");
        return false;
    }
    
    return true;
}

bool WiFiSetup::setChannel(const std::string& iface, int channel) {
    // Disable regulatory rules first
    std::string disable_rules = "iw " + iface + " set monitor none";
    if (!executeCommand(disable_rules).empty()) {
        logError("Failed to disable regulatory rules");
        return false;
    }

    // Set channel
    std::string cmd = "iw " + iface + " set channel " + std::to_string(channel);
    if (!executeCommand(cmd).empty()) {
        logError("Failed to set channel " + std::to_string(channel));
        return false;
    }

    return true;
}

bool WiFiSetup::startPacketMonitoring(const std::string& iface) {
    // Optional: Start tcpdump or similar tool for debugging
    return true;
}

std::string WiFiSetup::executeCommand(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    
    // Open pipe to command
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    
    // Read output
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    return result;
}

void WiFiSetup::logError(const std::string& msg) {
    std::cerr << "ERROR: " << msg << std::endl;
}

void WiFiSetup::logWarning(const std::string& msg) {
    std::cerr << "WARNING: " << msg << std::endl;
}

} // namespace wifi
} // namespace drone 