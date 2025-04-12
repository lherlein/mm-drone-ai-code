#include "communication/CommunicationManager.hpp"
#include "protocol/Packet.hpp"
#include "control/FlightController.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>

namespace drone {
namespace communication {

using namespace protocol;  // Add access to protocol types

namespace {
    constexpr auto TELEMETRY_INTERVAL = std::chrono::milliseconds(50);  // 20Hz
    constexpr auto HEARTBEAT_INTERVAL = std::chrono::milliseconds(100); // 10Hz
    constexpr auto HEARTBEAT_TIMEOUT = std::chrono::milliseconds(500);  // 2Hz minimum
    constexpr size_t MAX_PACKET_SIZE = 1024;
}

CommunicationManager::CommunicationManager(const struct Config& config)
    : gcu_address_(config.gcu_address)
    , gcu_port_(config.gcu_port)
    , local_port_(config.local_port)
    , socket_fd_(-1)
    , flight_controller_(nullptr)
    , running_(false)
    , connected_(false) {
}

CommunicationManager::~CommunicationManager() {
    stop();
}

bool CommunicationManager::init() {
    if (!setupSocket()) {
        std::cerr << "Failed to setup UDP socket" << std::endl;
        return false;
    }
    return true;
}

void CommunicationManager::start() {
    if (running_) return;
    
    running_ = true;
    receive_thread_ = std::make_unique<std::thread>(&CommunicationManager::receiveLoop, this);
}

void CommunicationManager::stop() {
    running_ = false;
    
    if (receive_thread_ && receive_thread_->joinable()) {
        receive_thread_->join();
    }
    
    closeSocket();
}

void CommunicationManager::update() {
    static auto last_telemetry = std::chrono::steady_clock::now();
    static auto last_heartbeat = std::chrono::steady_clock::now();
    
    auto now = std::chrono::steady_clock::now();
    
    // Send telemetry at regular intervals
    if (now - last_telemetry >= TELEMETRY_INTERVAL) {
        sendTelemetry();
        last_telemetry = now;
    }
    
    // Send heartbeat at regular intervals
    if (now - last_heartbeat >= HEARTBEAT_INTERVAL) {
        sendHeartbeat();
        last_heartbeat = now;
    }
    
    // Check connection status
    validateConnection();
}

bool CommunicationManager::setupSocket() {
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Set non-blocking mode
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK);
    
    // Bind to local address
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(local_port_);
    
    if (bind(socket_fd_, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
        closeSocket();
        return false;
    }
    
    return true;
}

void CommunicationManager::closeSocket() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

void CommunicationManager::receiveLoop() {
    std::vector<uint8_t> buffer(MAX_PACKET_SIZE);
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);
    
    while (running_) {
        ssize_t bytes_received = recvfrom(socket_fd_, buffer.data(), buffer.size(), 0,
                                        (struct sockaddr*)&sender_addr, &sender_len);
                                        
        if (bytes_received > 0) {
            try {
                Packet packet = Packet::deserialize(buffer.data(), bytes_received);
                if (packet.validate()) {
                    handleIncomingPacket(packet);
                }
            } catch (const std::exception& e) {
                std::cerr << "Error processing received packet: " << e.what() << std::endl;
            }
        } else if (bytes_received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void CommunicationManager::handleIncomingPacket(const Packet& packet) {
    switch (packet.getType()) {
        case PacketType::CONTROL:
            if (flight_controller_) {
                const auto& control_data = packet.getControlData();
                flight_controller_->setControlInputs(control_data);
            }
            break;
            
        case PacketType::HEARTBEAT:
            {
                std::lock_guard<std::mutex> lock(heartbeat_mutex_);
                last_heartbeat_ = std::chrono::steady_clock::now();
                connected_ = true;
            }
            break;
            
        case PacketType::CONFIG:
            // Handle configuration updates
            break;
            
        default:
            std::cerr << "Received unknown packet type" << std::endl;
            break;
    }
}

void CommunicationManager::sendPacket(const Packet& packet) {
    std::vector<uint8_t> buffer = packet.serialize();
    
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(gcu_port_);
    inet_pton(AF_INET, gcu_address_.c_str(), &dest_addr.sin_addr);
    
    sendto(socket_fd_, buffer.data(), buffer.size(), 0,
           (struct sockaddr*)&dest_addr, sizeof(dest_addr));
}

void CommunicationManager::sendTelemetry() {
    if (!flight_controller_) return;
    
    protocol::TelemetryData telemetry;
    // Populate telemetry data from flight controller
    // flight_controller_->getTelemetryData(&telemetry);
    
    auto packet = Packet::createTelemetry(telemetry);
    sendPacket(packet);
}

void CommunicationManager::sendHeartbeat() {
    protocol::HeartbeatData heartbeat;
    heartbeat.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    auto packet = Packet::createHeartbeat(heartbeat);
    sendPacket(packet);
}

bool CommunicationManager::validateConnection() {
    std::lock_guard<std::mutex> lock(heartbeat_mutex_);
    auto now = std::chrono::steady_clock::now();
    
    if (now - last_heartbeat_ > HEARTBEAT_TIMEOUT) {
        connected_ = false;
    }
    
    return connected_;
}

void CommunicationManager::setFlightController(control::FlightController* controller) {
    flight_controller_ = controller;
}

bool CommunicationManager::isConnected() const {
    return connected_;
}

std::chrono::steady_clock::time_point CommunicationManager::getLastHeartbeat() const {
    std::lock_guard<std::mutex> lock(heartbeat_mutex_);
    return last_heartbeat_;
}

} // namespace communication
} // namespace drone 