#pragma once

#include <chrono>

namespace drone {
namespace utils {

class Timer {
public:
    Timer() : start_(std::chrono::steady_clock::now()) {}

    void reset() {
        start_ = std::chrono::steady_clock::now();
    }

    template<typename Duration>
    bool hasElapsed(Duration duration) const {
        auto now = std::chrono::steady_clock::now();
        return (now - start_) >= duration;
    }

    template<typename Duration>
    Duration elapsed() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<Duration>(now - start_);
    }

private:
    std::chrono::steady_clock::time_point start_;
};

} // namespace utils
} // namespace drone 