#pragma once

#include <string>
#include <vector>

namespace drone {
namespace gcu {
namespace control {

class Joystick {
public:
    Joystick();
    ~Joystick();

    bool init();
    void update();
    
    float getAxis(size_t axis) const;
    bool getButton(size_t button) const;
    
    bool isConnected() const { return fd_ >= 0; }
    const std::string& getName() const { return name_; }

private:
    int fd_;
    std::string name_;
    std::vector<float> axes_;
    std::vector<bool> buttons_;
    
    bool openDevice();
    void closeDevice();
    void readEvents();
};

} // namespace control
} // namespace gcu
} // namespace drone 