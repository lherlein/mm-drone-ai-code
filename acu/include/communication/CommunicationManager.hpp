#pragma once

#include "protocol/Packet.hpp"
#include "control/FlightController.hpp"
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

namespace drone {
namespace communication {

class CommunicationManager {
public:
    explicit CommunicationManager(const struct Config& config);
    ~CommunicationManager();

    // Prevent copying
    CommunicationManager(const CommunicationManager&) = delete;
    CommunicationManager& operator=(const CommunicationManager&) = delete;

    // Initialization and lifecycle
    bool init();
    void start();
    void stop();
    void update();

    // Dependencies
    void setFlightController(control::FlightController* controller);

    // Status
    bool isConnected() const;
    std::chrono::steady_clock::time_point getLastHeartbeat() const;

private:
    // Network configuration
    std::string gcu_address_;
    uint16_t gcu_port_;
    uint16_t local_port_;
    int socket_fd_;

    // Dependencies
    control::FlightController* flight_controller_;

    // Threading
    std::unique_ptr<std::thread> receive_thread_;
    std::atomic<bool> running_;
    
    // Packet queues
    std::queue<Packet> outgoing_packets_;
    std::mutex outgoing_mutex_;

    // Connection state
    std::atomic<bool> connected_;
    mutable std::mutex heartbeat_mutex_;
    std::chrono::steady_clock::time_point last_heartbeat_;

    // Thread functions
    void receiveLoop();
    void handleIncomingPacket(const Packet& packet);
    void sendPacket(const Packet& packet);
    void sendTelemetry();
    void sendHeartbeat();

    // Helper functions
    bool setupSocket();
    void closeSocket();
    bool validateConnection();
};

} // namespace communication
} // namespace drone 