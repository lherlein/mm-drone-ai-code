#pragma once

#include <QApplication>
#include <memory>

namespace drone {
namespace gcu {

// Forward declarations
namespace communication { class CommunicationManager; }
namespace ui { class MainWindow; }
namespace control { class InputManager; }

class GCUApplication {
public:
    GCUApplication(int argc, char* argv[]);
    ~GCUApplication();

    bool init();
    int run();
    void shutdown();

private:
    std::unique_ptr<QApplication> app_;
    std::unique_ptr<ui::MainWindow> mainWindow_;
    std::unique_ptr<communication::CommunicationManager> commManager_;
    std::unique_ptr<control::InputManager> inputManager_;

    void setupConnections();
    void updateTelemetry(const protocol::TelemetryData& telemetry);
    void updateConnectionStatus(bool connected);
};

} // namespace gcu
} // namespace drone 