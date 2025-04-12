# Drone Control System

This repository contains the code for a drone control system, consisting of an Aerial Control Unit (ACU) running on a Raspberry Pi and a Ground Control Unit (GCU).

## System Requirements

### Development Environment
- CMake 3.10 or higher
- C++17 compatible compiler (GCC 7+ or Clang 5+)
- Git

### Raspberry Pi Requirements
- Raspberry Pi 3B+ or 4
- Raspbian OS (64-bit recommended)
- I2C and SPI enabled
- Required packages:
  ```bash
  sudo apt update
  sudo apt install -y \
    build-essential \
    cmake \
    git \
    i2c-tools \
    python3-dev \
    python3-pip \
    libgpiod-dev
  ```

## Project Structure

```
.
├── acu/            # Aerial Control Unit code
├── common/         # Shared libraries and utilities
├── docs/          # Documentation
└── gcu/           # Ground Control Unit code
```

## Building the Project

### Building on Development Machine

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/drone-control.git
   cd drone-control
   ```

2. Create and enter build directory:
   ```bash
   mkdir build && cd build
   ```

3. Configure and build:
   ```bash
   cmake ..
   make -j$(nproc)
   ```

### Building on Raspberry Pi

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/drone-control.git
   cd drone-control
   ```

2. Create and enter build directory:
   ```bash
   mkdir build && cd build
   ```

3. Configure and build:
   ```bash
   cmake .. -DTARGET_PLATFORM=RPI
   make -j4
   ```

4. Install (optional):
   ```bash
   sudo make install
   ```

## Running the System

### On Raspberry Pi (ACU)

1. Enable required interfaces:
   ```bash
   # Enable I2C
   sudo raspi-config nonint do_i2c 0
   
   # Enable SPI
   sudo raspi-config nonint do_spi 0
   ```

2. Run the ACU:
   ```bash
   # If installed
   acu_control

   # Or from build directory
   ./acu/acu_control
   ```

### On Development Machine (GCU)

1. Run the GCU:
   ```bash
   # If installed
   gcu_control

   # Or from build directory
   ./gcu/gcu_control
   ```

## Configuration

The system uses configuration files located in `/etc/drone-control/` when installed, or can be specified via command line:

```bash
acu_control --config path/to/config.json
```

Example configuration file:
```json
{
  "communication": {
    "gcu_address": "192.168.1.100",
    "gcu_port": 14550,
    "local_port": 14551
  },
  "control": {
    "pid": {
      "roll": {"p": 1.0, "i": 0.0, "d": 0.2},
      "pitch": {"p": 1.0, "i": 0.0, "d": 0.2},
      "yaw": {"p": 2.0, "i": 0.0, "d": 0.0}
    }
  }
}
```

## Development

### Code Style
- Follow C++17 standards
- Use clang-format for code formatting
- Follow Google C++ Style Guide

### Adding New Features
1. Create feature branch
2. Implement changes
3. Add tests
4. Submit pull request

## Troubleshooting

### Common Issues

1. Permission denied for GPIO/I2C/SPI:
   ```bash
   sudo usermod -aG gpio,i2c,spi $USER
   # Log out and back in
   ```

2. Communication issues:
   - Check firewall settings
   - Verify IP addresses and ports
   - Test network connectivity

3. Build issues:
   ```bash
   # Clean build
   rm -rf build/
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   make VERBOSE=1
   ```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

1. Fork the repository
2. Create feature branch
3. Commit changes
4. Push to branch
5. Submit pull request

## Support

For support, please:
1. Check documentation in `/docs`
2. Search existing issues
3. Create new issue if needed 