#include "GCUApplication.hpp"
#include "communication/CommunicationManager.hpp"
#include "ui/MainWindow.hpp"
#include "control/InputManager.hpp"
#include <QTimer>
#include <iostream>

namespace drone {
namespace gcu {

GCUApplication::GCUApplication(int argc, char* argv[])
    : app_(std::make_unique<QApplication>(argc, argv)) {
}

GCUApplication::~GCUApplication() {
    shutdown();
}

bool GCUApplication::init() {
    // Create components
    mainWindow_ = std::make_unique<ui::MainWindow>();
    commManager_ = std::make_unique<communication::CommunicationManager>();
    inputManager_ = std::make_unique<control::InputManager>();

    // Initialize components
    if (!commManager_->init("192.168.1.10", 5760, 5761)) {
        std::cerr << "Failed to initialize communication manager" << std::endl;
        return false;
    }

    if (!inputManager_->init()) {
        std::cerr << "Failed to initialize input manager" << std::endl;
        return false;
    }

    // Setup connections
    setupConnections();

    // Start communication
    commManager_->start();

    // Show main window
    mainWindow_->show();

    // Setup update timer for input polling
    QTimer* timer = new QTimer(app_.get());
    QObject::connect(timer, &QTimer::timeout, inputManager_.get(), &control::InputManager::update);
    timer->start(20); // 50Hz update rate

    return true;
}

int GCUApplication::run() {
    return app_->exec();
}

void GCUApplication::shutdown() {
    if (commManager_) {
        commManager_->stop();
    }
}

void GCUApplication::setupConnections() {
    // Connect telemetry updates
    QObject::connect(commManager_.get(), &communication::CommunicationManager::telemetryReceived,
            mainWindow_.get(), &ui::MainWindow::updateTelemetry);

    // Connect connection status updates
    QObject::connect(commManager_.get(), &communication::CommunicationManager::connectionStatusChanged,
            mainWindow_.get(), &ui::MainWindow::updateConnectionStatus);

    // Connect control inputs
    QObject::connect(inputManager_.get(), &control::InputManager::controlDataChanged,
            commManager_.get(), &communication::CommunicationManager::sendControlData);

    // Connect arming and emergency stop
    QObject::connect(mainWindow_.get(), &ui::MainWindow::armingRequested,
            [this](bool arm) {
                protocol::ControlData data = {};
                data.armed = arm;
                commManager_->sendControlData(data);
            });

    QObject::connect(mainWindow_.get(), &ui::MainWindow::emergencyStopRequested,
            [this]() {
                protocol::ControlData data = {};
                data.emergency_stop = true;
                commManager_->sendControlData(data);
            });
}

void GCUApplication::updateTelemetry(const protocol::TelemetryData& telemetry) {
    mainWindow_->updateTelemetry(telemetry);
}

void GCUApplication::updateConnectionStatus(bool connected) {
    mainWindow_->updateConnectionStatus(connected);
}

} // namespace gcu
} // namespace drone 