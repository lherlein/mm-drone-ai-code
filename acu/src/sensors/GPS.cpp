#include "sensors/GPS.hpp"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <cstring>

namespace drone {
namespace sensors {

namespace {
    std::vector<std::string> split(const std::string& str, char delim) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, delim)) {
            tokens.push_back(token);
        }
        return tokens;
    }
}

GPS::GPS(const std::string& device)
    : devicePath_(device), serialFd_(-1) {}

GPS::~GPS() {
    stop();
    closeSerial();
}

bool GPS::init() {
    return openSerial();
}

void GPS::start() {
    if (!running_) {
        running_ = true;
        readThread_ = std::make_unique<std::thread>(&GPS::readLoop, this);
    }
}

void GPS::stop() {
    if (running_) {
        running_ = false;
        if (readThread_ && readThread_->joinable()) {
            readThread_->join();
        }
    }
}

GPS::GPSData GPS::getData() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return data_;
}

bool GPS::openSerial() {
    serialFd_ = open(devicePath_.c_str(), O_RDWR | O_NOCTTY);
    if (serialFd_ < 0) {
        return false;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(serialFd_, &tty) != 0) {
        closeSerial();
        return false;
    }

    // Set baud rate (9600 for GT-U7)
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);

    // 8N1 mode
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    // No flow control
    tty.c_cflag &= ~CRTSCTS;

    // Enable receiver, ignore status lines
    tty.c_cflag |= CREAD | CLOCAL;

    // Raw input
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG);

    // Raw output
    tty.c_oflag &= ~OPOST;

    // Set read timeout
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;  // 0.1 seconds

    if (tcsetattr(serialFd_, TCSANOW, &tty) != 0) {
        closeSerial();
        return false;
    }

    return true;
}

void GPS::closeSerial() {
    if (serialFd_ >= 0) {
        close(serialFd_);
        serialFd_ = -1;
    }
}

void GPS::readLoop() {
    while (running_) {
        std::string line = readLine();
        if (!line.empty()) {
            parseNMEA(line);
        }
    }
}

std::string GPS::readLine() {
    std::string line;
    char c;
    while (running_ && serialFd_ >= 0) {
        if (read(serialFd_, &c, 1) == 1) {
            if (c == '\n') {
                return line;
            } else if (c != '\r') {
                line += c;
            }
        }
    }
    return line;
}

void GPS::parseNMEA(const std::string& sentence) {
    if (sentence.empty() || sentence[0] != '$') {
        return;
    }

    // Calculate checksum
    size_t asterisk = sentence.find('*');
    if (asterisk == std::string::npos || asterisk + 3 >= sentence.length()) {
        return;
    }

    uint8_t checksum = 0;
    for (size_t i = 1; i < asterisk; ++i) {
        checksum ^= sentence[i];
    }

    char expectedChecksum[3];
    sprintf(expectedChecksum, "%02X", checksum);
    if (sentence.substr(asterisk + 1, 2) != expectedChecksum) {
        return;
    }

    // Parse different sentence types
    if (sentence.substr(3, 3) == "GGA") {
        parseGGA(sentence);
    } else if (sentence.substr(3, 3) == "RMC") {
        parseRMC(sentence);
    }
}

void GPS::parseGGA(const std::string& sentence) {
    auto tokens = split(sentence, ',');
    if (tokens.size() < 15) return;

    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Parse fix quality
    data_.fix = (tokens[6] != "0");
    
    // Parse position if we have a fix
    if (data_.fix) {
        data_.latitude = parseLatLon(tokens[2], tokens[3][0]);
        data_.longitude = parseLatLon(tokens[4], tokens[5][0]);
        data_.altitude = tokens[9].empty() ? 0 : std::stof(tokens[9]);
        data_.satellites = tokens[7].empty() ? 0 : std::stoi(tokens[7]);
        data_.hdop = tokens[8].empty() ? 0 : std::stof(tokens[8]);
    }
}

void GPS::parseRMC(const std::string& sentence) {
    auto tokens = split(sentence, ',');
    if (tokens.size() < 12) return;

    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Parse speed in knots and convert to m/s
    if (!tokens[7].empty()) {
        data_.speed = std::stof(tokens[7]) * 0.514444f;
    }
}

double GPS::parseLatLon(const std::string& value, char direction) {
    if (value.empty()) return 0.0;

    double degrees = std::stod(value.substr(0, 2));
    double minutes = std::stod(value.substr(2));
    double result = degrees + minutes / 60.0;

    if (direction == 'S' || direction == 'W') {
        result = -result;
    }

    return result;
}

} // namespace sensors
} // namespace drone 