#include "sensors/SensorManager.hpp"
#include "control/FlightController.hpp"
#include "communication/CommunicationManager.hpp"
#include "state/StateMachine.hpp"
#include "wifi/WiFiSetup.hpp"
#include "Config.hpp"
#include <iostream>
#include <signal.h>
#include <chrono>
#include <thread>

using namespace drone;

namespace {
    volatile sig_atomic_t running = 1;

    void signalHandler(int) {
        running = 0;
    }

    void cleanup() {
        wifi::WiFiSetup::cleanup();
    }

    Config loadConfig() {
        Config config;
        // Load configuration from file or environment
        return config;
    }
}

int main() {
    try {
        // Set up signal handling
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        std::atexit(cleanup);

        // Load configuration
        auto config = loadConfig();

        // Initialize WiFi
        std::cout << "Initializing WiFi..." << std::endl;
        if (!wifi::WiFiSetup::initialize(config)) {
            std::cerr << "Failed to initialize WiFi" << std::endl;
            return 1;
        }

        // Create system components
        sensors::SensorManager sensorManager(config);
        control::FlightController flightController(config);
        communication::CommunicationManager commManager(config);
        state::StateMachine stateMachine;

        // Set up component relationships
        flightController.setSensorManager(&sensorManager);
        commManager.setFlightController(&flightController);
        stateMachine.setFlightController(&flightController);
        stateMachine.setSensorManager(&sensorManager);
        stateMachine.setCommunicationManager(&commManager);

        // Initialize components
        std::cout << "Initializing sensor manager..." << std::endl;
        if (!sensorManager.start()) {
            std::cerr << "Failed to initialize sensor manager" << std::endl;
            return 1;
        }

        std::cout << "Initializing flight controller..." << std::endl;
        if (!flightController.init()) {
            std::cerr << "Failed to initialize flight controller" << std::endl;
            return 1;
        }

        std::cout << "Initializing communication manager..." << std::endl;
        if (!commManager.init()) {
            std::cerr << "Failed to initialize communication manager" << std::endl;
            return 1;
        }

        // Start communication
        commManager.start();

        std::cout << "ACU system initialized and running" << std::endl;

        // Main control loop
        const auto updateInterval = std::chrono::milliseconds(5);  // 200Hz
        auto lastUpdate = std::chrono::steady_clock::now();

        while (running) {
            auto now = std::chrono::steady_clock::now();
            if (now - lastUpdate >= updateInterval) {
                // Update all components
                sensorManager.update();
                commManager.update();
                stateMachine.update();

                lastUpdate = now;
            }

            // Small sleep to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // Graceful shutdown
        std::cout << "Shutting down..." << std::endl;
        commManager.stop();
        flightController.stop();
        sensorManager.stop();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
} 