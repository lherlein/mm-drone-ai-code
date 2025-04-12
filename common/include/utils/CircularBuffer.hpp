#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>

namespace drone {
namespace utils {

template<typename T, size_t Size>
class CircularBuffer {
public:
    CircularBuffer() : head_(0), tail_(0), full_(false) {}

    bool push(const T& item) {
        if (full_) {
            return false;
        }

        buffer_[tail_] = item;
        tail_ = (tail_ + 1) % Size;
        full_ = tail_ == head_;
        return true;
    }

    bool pop(T& item) {
        if (empty()) {
            return false;
        }

        item = buffer_[head_];
        head_ = (head_ + 1) % Size;
        full_ = false;
        return true;
    }

    bool peek(T& item) const {
        if (empty()) {
            return false;
        }

        item = buffer_[head_];
        return true;
    }

    void clear() {
        head_ = tail_ = 0;
        full_ = false;
    }

    bool empty() const {
        return (!full_ && (head_ == tail_));
    }

    bool full() const {
        return full_;
    }

    size_t size() const {
        if (full_) {
            return Size;
        }
        if (tail_ >= head_) {
            return tail_ - head_;
        }
        return Size - (head_ - tail_);
    }

    size_t capacity() const {
        return Size;
    }

private:
    std::array<T, Size> buffer_;
    size_t head_;
    size_t tail_;
    bool full_;
};

} // namespace utils
} // namespace drone 