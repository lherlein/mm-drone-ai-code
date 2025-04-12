#pragma once

#include "protocol/Packet.hpp"
#include <QObject>
#include <memory>
#include <chrono>

namespace drone {
namespace gcu {
namespace control {

class Joystick;

class InputManager : public QObject {
    Q_OBJECT

public:
    explicit InputManager(QObject* parent = nullptr);
    ~InputManager();

    bool init();
    void update();

signals:
    void controlDataChanged(const protocol::ControlData& controlData);

private:
    std::unique_ptr<Joystick> joystick_;
    protocol::ControlData currentControlData_;
    std::chrono::steady_clock::time_point lastUpdate_;
    
    void updateJoystick();
    void updateKeyboard();
    void normalizeAxes();
    void applyDeadzone(float& value, float deadzone);

    static constexpr float DEADZONE = 0.1f;
    static constexpr float MAX_THROTTLE_CHANGE = 0.1f; // Maximum throttle change per update
};

} // namespace control
} // namespace gcu
} // namespace drone 