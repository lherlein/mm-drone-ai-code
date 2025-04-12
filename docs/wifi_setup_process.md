# WiFi Setup Process

This document details the initialization process for WiFi interfaces on both the Aerial Control Unit (ACU) and Ground Control Unit (GCU). The setup ensures proper WiFi configuration and monitor mode operation.

## Overview

The setup process:
1. Identifies available WiFi interfaces
2. Verifies interface capabilities
3. Configures monitor mode
4. Validates connection parameters

## Prerequisites

### ACU (Raspberry Pi)
Required hardware:
- Compatible WiFi adapter supporting monitor mode
  - Recommended: Ralink RT5372, Atheros AR9271, or similar
  - Must support 2.4GHz band operation
  - Must support monitor mode

Required packages:
```bash
sudo apt install -y \
  iw \
  wireless-tools \
  aircrack-ng
```

### GCU (Ground Station)
Required hardware:
- Compatible WiFi adapter supporting monitor mode
- Same frequency band support as ACU

Required packages:
- Linux: Same as ACU
- Windows: Npcap with monitor mode support

## Setup Process

### 1. Interface Detection

```cpp
bool setupWiFiInterface() {
    // List all wireless interfaces
    std::string cmd = "iw dev | grep Interface | cut -f 2 -d\" \"";
    std::vector<std::string> interfaces = executeCommand(cmd);
    
    if (interfaces.empty()) {
        logError("No wireless interfaces found");
        return false;
    }

    // Find suitable interface
    for (const auto& iface : interfaces) {
        if (checkInterfaceCapabilities(iface)) {
            selectedInterface = iface;
            return true;
        }
    }
    
    return false;
}
```

### 2. Capability Verification

```cpp
bool checkInterfaceCapabilities(const std::string& iface) {
    // Check if interface supports monitor mode
    std::string cmd = "iw " + iface + " info | grep \"Supported interface modes\" -A 8";
    std::string output = executeCommand(cmd);
    
    if (output.find("* monitor") == std::string::npos) {
        logWarning("Interface " + iface + " does not support monitor mode");
        return false;
    }
    
    return true;
}
```

### 3. Monitor Mode Configuration

```cpp
bool enableMonitorMode(const std::string& iface) {
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
    return executeCommand(check) == "monitor";
}
```

### 4. Channel Configuration

```cpp
bool setChannel(const std::string& iface, int channel) {
    // Disable regulatory rules first
    std::string disable_rules = "iw " + iface + " set monitor none";
    if (!executeCommand(disable_rules).empty()) {
        logError("Failed to disable regulatory rules");
        return false;
    }

    // Set channel
    std::string cmd = "iw " + iface + " set channel " + std::to_string(channel);
    return executeCommand(cmd).empty();
}
```

## Implementation

### Common Setup Class

```cpp
class WiFiSetup {
public:
    static bool initialize(const Config& config) {
        // 1. Detect interfaces
        if (!setupWiFiInterface()) {
            return false;
        }
        
        // 2. Enable monitor mode
        if (!enableMonitorMode(selectedInterface)) {
            return false;
        }
        
        // 3. Set channel
        if (!setChannel(selectedInterface, config.channel)) {
            return false;
        }
        
        // 4. Start packet monitoring
        return startPacketMonitoring(selectedInterface);
    }
    
private:
    static std::string selectedInterface;
};
```

## Error Handling

The setup process includes comprehensive error handling:

1. Interface not found:
   ```
   ERROR: No suitable wireless interface found
   HINT: Ensure WiFi adapter is connected and supported
   ```

2. Monitor mode failure:
   ```
   ERROR: Failed to enable monitor mode on <interface>
   HINT: Check driver support and permissions
   ```

3. Channel setting failure:
   ```
   ERROR: Failed to set channel <number> on <interface>
   HINT: Verify channel is supported in your region
   ```

## Troubleshooting

### Common Issues

1. Permission Denied
   ```bash
   # Add user to netdev group
   sudo usermod -aG netdev $USER
   
   # Or run with sudo
   sudo acu_control
   ```

2. Interface Not Found
   ```bash
   # List all interfaces
   iw dev
   
   # Check driver status
   lsmod | grep <driver_name>
   ```

3. Monitor Mode Failed
   ```bash
   # Check current interface status
   iw <interface> info
   
   # Try manual configuration
   sudo airmon-ng check kill
   sudo airmon-ng start <interface>
   ```

## Platform-Specific Notes

### Raspberry Pi (ACU)
- Some built-in WiFi chips may not support monitor mode
- Consider using external USB WiFi adapter
- May require firmware updates:
  ```bash
  sudo rpi-update
  sudo reboot
  ```

### Linux (GCU)
- NetworkManager may interfere with monitor mode
- Consider disabling for dedicated interface:
  ```bash
  sudo nmcli dev set <interface> managed no
  ```

### Windows (GCU)
- Requires Npcap with WinPcap compatibility
- May need to start service:
  ```batch
  sc start npcap
  ```

## Security Considerations

1. Monitor mode increases system exposure
2. Use dedicated interface when possible
3. Implement proper firewall rules
4. Consider MAC address randomization

## Testing

Verify setup with:
```bash
# Check interface mode
iw <interface> info

# Verify packet capture
tcpdump -i <interface> -n
``` 