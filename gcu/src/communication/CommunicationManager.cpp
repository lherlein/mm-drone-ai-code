#include "communication/CommunicationManager.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <random>
#include <sstream>

namespace drone {
namespace gcu {
namespace communication {

CommunicationManager::CommunicationManager(QObject* parent)
    : QObject(parent)
    , socket_fd_(-1)
    , running_(false)
    , connected_(false) {
}

CommunicationManager::~CommunicationManager() {
    stop();
}

bool CommunicationManager::init() {
    if (!setupSocket()) {
        return false;
    }
    return true;
}

void CommunicationManager::start() {
    if (running_) return;
    
    running_ = true;
    receive_thread_ = std::make_unique<std::thread>(&CommunicationManager::receiveLoop, this);
    discovery_thread_ = std::make_unique<std::thread>(&CommunicationManager::discoveryLoop, this);
}

void CommunicationManager::stop() {
    if (!running_) return;
    
    running_ = false;
    if (receive_thread_ && receive_thread_->joinable()) {
        receive_thread_->join();
    }
    if (discovery_thread_ && discovery_thread_->joinable()) {
        discovery_thread_->join();
    }
    closeSocket();
}

void CommunicationManager::sendControlData(const protocol::ControlData& controlData) {
    protocol::Packet control = protocol::Packet::createControl(controlData);
    std::lock_guard<std::mutex> lock(send_mutex_);
    outgoing_packets_.push(control);
}

void CommunicationManager::receiveLoop() {
    std::vector<uint8_t> buffer(MAX_PACKET_SIZE);
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);
    
    auto lastHeartbeat = std::chrono::steady_clock::now();
    
    while (running_) {
        // Send heartbeat periodically
        auto now = std::chrono::steady_clock::now();
        if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
            sendHeartbeat();
            lastHeartbeat = now;
        }
        
        // Send any queued packets
        {
            std::lock_guard<std::mutex> lock(send_mutex_);
            while (!outgoing_packets_.empty()) {
                const auto& packet = outgoing_packets_.front();
                std::vector<uint8_t> data = packet.serialize();
                
                struct sockaddr_in dest_addr;
                dest_addr.sin_family = AF_INET;
                dest_addr.sin_port = htons(drone_port_);
                inet_pton(AF_INET, drone_address_.c_str(), &dest_addr.sin_addr);
                
                sendto(socket_fd_, data.data(), data.size(), 0,
                      (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                      
                outgoing_packets_.pop();
            }
        }
        
        // Receive incoming packets
        ssize_t bytes_received = recvfrom(socket_fd_, buffer.data(), buffer.size(), 0,
                                        (struct sockaddr*)&sender_addr, &sender_len);
                                        
        if (bytes_received > 0) {
            try {
                protocol::Packet packet = protocol::Packet::deserialize(buffer.data(), bytes_received);
                if (packet.validate()) {
                    handleIncomingPacket(packet);
                }
            } catch (const std::exception& e) {
                std::cerr << "Error processing received packet: " << e.what() << std::endl;
            }
        } else if (bytes_received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
        }
        
        validateConnection();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void CommunicationManager::handleIncomingPacket(const protocol::Packet& packet) {
    switch (packet.getType()) {
        case protocol::PacketType::TELEMETRY:
            emit telemetryReceived(packet.getTelemetryData());
            break;
            
        case protocol::PacketType::HEARTBEAT:
            last_heartbeat_ = std::chrono::steady_clock::now();
            if (!connected_) {
                connected_ = true;
                emit connectionStatusChanged(true);
            }
            break;
            
        default:
            break;
    }
}

void CommunicationManager::sendHeartbeat() {
    protocol::HeartbeatData heartbeat;
    heartbeat.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    auto packet = protocol::Packet::createHeartbeat(heartbeat);
    std::lock_guard<std::mutex> lock(send_mutex_);
    outgoing_packets_.push(packet);
}

void CommunicationManager::validateConnection() {
    auto now = std::chrono::steady_clock::now();
    bool newStatus = (now - last_heartbeat_) <= HEARTBEAT_TIMEOUT;
    
    if (connected_ != newStatus) {
        connected_ = newStatus;
        emit connectionStatusChanged(connected_);
    }
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

void CommunicationManager::discoveryLoop() {
    while (running_) {
        // Clean up old discovered drones
        {
            std::lock_guard<std::mutex> lock(drones_mutex_);
            auto now = std::chrono::steady_clock::now();
            for (auto it = discovered_drones_.begin(); it != discovered_drones_.end();) {
                if (now - it->second.last_seen > CONNECTION_TIMEOUT) {
                    it = discovered_drones_.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // Validate active connections
        validateConnections();

        std::this_thread::sleep_for(DISCOVERY_INTERVAL);
    }
}

void CommunicationManager::handleBeacon(const std::vector<uint8_t>& data) {
    // Parse beacon data (ID, capabilities, version)
    if (data.size() < 12) return;  // Minimum size check
    
    std::string id(reinterpret_cast<const char*>(data.data()), 8);
    uint32_t capabilities = *reinterpret_cast<const uint32_t*>(data.data() + 8);
    
    if (!validateDroneId(id)) return;
    
    {
        std::lock_guard<std::mutex> lock(drones_mutex_);
        auto& drone = discovered_drones_[id];
        drone.id = id;
        drone.capabilities = capabilities;
        drone.last_seen = std::chrono::steady_clock::now();
        drone.state = ConnectionState::DISCOVERY;
        
        emit droneDiscovered(id, capabilities);
    }
}

void CommunicationManager::handleSyn(const std::vector<uint8_t>& data) {
    // Parse SYN data and send ACK with address assignment
    if (data.size() < 8) return;  // Minimum size check
    
    std::string id(reinterpret_cast<const char*>(data.data()), 8);
    
    std::lock_guard<std::mutex> lock(drones_mutex_);
    auto it = discovered_drones_.find(id);
    if (it != discovered_drones_.end() && it->second.state == ConnectionState::DISCOVERY) {
        it->second.state = ConnectionState::CONNECTING;
        it->second.address = assignAddress();
        it->second.token = generateToken();
        sendAck(it->second);
    }
}

void CommunicationManager::handleSynAck(const std::vector<uint8_t>& data) {
    // Validate SYNACK and finalize connection
    if (data.size() < 16) return;  // Minimum size check
    
    std::string id(reinterpret_cast<const char*>(data.data()), 8);
    uint64_t token = *reinterpret_cast<const uint64_t*>(data.data() + 8);
    
    std::lock_guard<std::mutex> lock(drones_mutex_);
    auto it = discovered_drones_.find(id);
    if (it != discovered_drones_.end() && 
        it->second.state == ConnectionState::CONNECTING &&
        it->second.token == token) {
        
        active_drones_[id] = it->second;
        active_drones_[id].state = ConnectionState::ACTIVE;
        discovered_drones_.erase(it);
        
        emit droneConnected(id, active_drones_[id].address);
        emit connectionStatusChanged(true);
    }
}

void CommunicationManager::sendAck(const DroneInfo& drone) {
    std::vector<uint8_t> data;
    data.resize(24);  // ID + token + address
    
    std::copy(drone.id.begin(), drone.id.end(), data.begin());
    *reinterpret_cast<uint64_t*>(data.data() + 8) = drone.token;
    std::copy(drone.address.begin(), drone.address.end(), data.data() + 16);
    
    protocol::Packet ack = protocol::Packet::createAck(data);
    
    std::lock_guard<std::mutex> lock(send_mutex_);
    outgoing_packets_.push(ack);
}

void CommunicationManager::validateConnections() {
    std::lock_guard<std::mutex> lock(drones_mutex_);
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = active_drones_.begin(); it != active_drones_.end();) {
        if (now - it->second.last_seen > CONNECTION_TIMEOUT) {
            emit droneDisconnected(it->first);
            it = active_drones_.erase(it);
        } else {
            ++it;
        }
    }
    
    if (active_drones_.empty()) {
        emit connectionStatusChanged(false);
    }
}

std::string CommunicationManager::assignAddress() {
    // Simple address assignment from 172.16.0.100 to 172.16.0.254
    static uint8_t last_assigned = 99;
    last_assigned = (last_assigned % 154) + 100;  // Wrap around to 100 if we reach 254
    
    std::stringstream ss;
    ss << NETWORK_PREFIX << static_cast<int>(last_assigned);
    return ss.str();
}

uint64_t CommunicationManager::generateToken() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
}

bool CommunicationManager::validateDroneId(const std::string& id) {
    // TODO: Implement drone ID validation against whitelist
    return true;
}

} // namespace communication
} // namespace gcu
} // namespace drone 