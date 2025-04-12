#pragma once

#include "protocol/Packet.hpp"
#include <QObject>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <chrono>
#include <map>
#include <vector>

namespace drone {
namespace gcu {
namespace communication {

class CommunicationManager : public QObject {
    Q_OBJECT

public:
    enum class ConnectionState {
        DISCONNECTED,
        DISCOVERY,
        REQUESTING,
        CONNECTING,
        CONNECTED,
        ACTIVE
    };

    struct DroneInfo {
        std::string id;
        uint32_t capabilities;
        uint16_t version;
        uint8_t signal_strength;
        std::string address;
        ConnectionState state;
        std::chrono::steady_clock::time_point last_seen;
        uint64_t token;
    };

    explicit CommunicationManager(QObject* parent = nullptr);
    ~CommunicationManager();

    bool init();
    void start();
    void stop();
    bool isConnected() const { return !active_drones_.empty(); }
    void sendControlData(const protocol::ControlData& controlData);

signals:
    void telemetryReceived(const protocol::TelemetryData& telemetry);
    void connectionStatusChanged(bool connected);
    void droneDiscovered(const std::string& id, uint32_t capabilities);
    void droneConnected(const std::string& id, const std::string& address);
    void droneDisconnected(const std::string& id);

private:
    int socket_fd_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> receive_thread_;
    std::unique_ptr<std::thread> discovery_thread_;
    
    std::mutex drones_mutex_;
    std::map<std::string, DroneInfo> discovered_drones_;
    std::map<std::string, DroneInfo> active_drones_;
    
    std::mutex send_mutex_;
    std::queue<protocol::Packet> outgoing_packets_;

    void discoveryLoop();
    void receiveLoop();
    void handleBeacon(const std::vector<uint8_t>& data);
    void handleSyn(const std::vector<uint8_t>& data);
    void handleSynAck(const std::vector<uint8_t>& data);
    void sendAck(const DroneInfo& drone);
    void validateConnections();
    bool setupSocket();
    void closeSocket();
    std::string assignAddress();
    uint64_t generateToken();
    bool validateDroneId(const std::string& id);

    static constexpr auto DISCOVERY_INTERVAL = std::chrono::seconds(1);
    static constexpr auto CONNECTION_TIMEOUT = std::chrono::seconds(5);
    static constexpr auto CLEANUP_INTERVAL = std::chrono::seconds(10);
    static constexpr size_t MAX_PACKET_SIZE = 1024;
    static constexpr const char* NETWORK_PREFIX = "172.16.0.";
};

} // namespace communication
} // namespace gcu
} // namespace drone 