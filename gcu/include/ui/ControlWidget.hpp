#pragma once

#include "protocol/Packet.hpp"
#include <QWidget>

namespace drone {
namespace gcu {
namespace ui {

class ControlWidget : public QWidget {
    Q_OBJECT

public:
    explicit ControlWidget(QWidget* parent = nullptr);
    ~ControlWidget();

public slots:
    void updateInputs();

signals:
    void controlDataChanged(const protocol::ControlData& controlData);

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    protocol::ControlData currentControlData_;
    bool keyboardControls_[4];  // [throttle up, throttle down, yaw left, yaw right]

    void drawControlSticks(QPainter& painter);
    void drawControlValues(QPainter& painter);
    void updateFromKeyboard();
    void applyDeadzone(float& value, float deadzone);

    static constexpr float DEADZONE = 0.1f;
    static constexpr float KEYBOARD_STEP = 0.1f;
};

} // namespace ui
} // namespace gcu
} // namespace drone 