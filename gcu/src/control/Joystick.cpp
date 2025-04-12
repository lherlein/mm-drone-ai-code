#include "control/Joystick.hpp"
#include <linux/joystick.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <iostream>

namespace drone {
namespace gcu {
namespace control {

namespace {
    constexpr const char* JOYSTICK_PATH = "/dev/input/js0";
    constexpr size_t MAX_AXES = 8;
    constexpr size_t MAX_BUTTONS = 16;
}

Joystick::Joystick()
    : fd_(-1)
    , axes_(MAX_AXES, 0.0f)
    , buttons_(MAX_BUTTONS, false) {
}

Joystick::~Joystick() {
    closeDevice();
}

bool Joystick::init() {
    return openDevice();
}

void Joystick::update() {
    if (fd_ < 0) return;

    struct js_event event;
    while (read(fd_, &event, sizeof(event)) > 0) {
        switch (event.type & ~JS_EVENT_INIT) {
            case JS_EVENT_AXIS:
                if (event.number < axes_.size()) {
                    axes_[event.number] = event.value / 32767.0f;
                }
                break;

            case JS_EVENT_BUTTON:
                if (event.number < buttons_.size()) {
                    buttons_[event.number] = event.value;
                }
                break;
        }
    }
}

float Joystick::getAxis(size_t axis) const {
    return (axis < axes_.size()) ? axes_[axis] : 0.0f;
}

bool Joystick::getButton(size_t button) const {
    return (button < buttons_.size()) ? buttons_[button] : false;
}

bool Joystick::openDevice() {
    // Try to open the default joystick device
    fd_ = open(JOYSTICK_PATH, O_RDONLY | O_NONBLOCK);
    if (fd_ < 0) {
        // If default device not found, try to find any available joystick
        DIR* dir = opendir("/dev/input");
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                if (strncmp("js", entry->d_name, 2) == 0) {
                    std::string path = "/dev/input/" + std::string(entry->d_name);
                    fd_ = open(path.c_str(), O_RDONLY | O_NONBLOCK);
                    if (fd_ >= 0) {
                        break;
                    }
                }
            }
            closedir(dir);
        }
    }

    if (fd_ >= 0) {
        // Get joystick name
        char name[128];
        if (ioctl(fd_, JSIOCGNAME(sizeof(name)), name) >= 0) {
            name_ = name;
        } else {
            name_ = "Unknown Joystick";
        }

        // Initialize axes and buttons
        char num_axes;
        char num_buttons;
        ioctl(fd_, JSIOCGAXES, &num_axes);
        ioctl(fd_, JSIOCGBUTTONS, &num_buttons);

        axes_.resize(num_axes, 0.0f);
        buttons_.resize(num_buttons, false);

        return true;
    }

    std::cerr << "Failed to open joystick device" << std::endl;
    return false;
}

void Joystick::closeDevice() {
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

} // namespace control
} // namespace gcu
} // namespace drone 