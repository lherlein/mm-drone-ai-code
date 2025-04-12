#pragma once

#include "protocol/Packet.hpp"
#include <QMainWindow>
#include <memory>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QTimer;
QT_END_NAMESPACE

namespace drone {
namespace gcu {
namespace ui {

// Forward declarations
class AttitudeWidget;
class TelemetryWidget;
class ControlWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

public slots:
    void updateTelemetry(const protocol::TelemetryData& telemetry);
    void updateConnectionStatus(bool connected);

signals:
    void controlDataChanged(const protocol::ControlData& controlData);
    void armingRequested(bool arm);
    void emergencyStopRequested();

private:
    std::unique_ptr<AttitudeWidget> attitudeWidget_;
    std::unique_ptr<TelemetryWidget> telemetryWidget_;
    std::unique_ptr<ControlWidget> controlWidget_;
    
    QLabel* connectionStatus_;
    QPushButton* armButton_;
    QPushButton* emergencyStopButton_;
    QTimer* updateTimer_;

    void setupUi();
    void setupConnections();
    void createStatusBar();
    void updateStatusBar(bool connected);
};

} // namespace ui
} // namespace gcu
} // namespace drone 