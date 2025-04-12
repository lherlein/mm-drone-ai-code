#pragma once

#include <QWidget>

namespace drone {
namespace gcu {
namespace ui {

class AttitudeWidget : public QWidget {
    Q_OBJECT

public:
    explicit AttitudeWidget(QWidget* parent = nullptr);
    ~AttitudeWidget();

    void updateAttitude(float roll, float pitch, float yaw);

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override;

private:
    float roll_;
    float pitch_;
    float yaw_;

    void drawHorizon(QPainter& painter);
    void drawCompass(QPainter& painter);
    void drawIndicators(QPainter& painter);
};

} // namespace ui
} // namespace gcu
} // namespace drone 