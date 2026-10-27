#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <utility>

namespace Hardware {

template <typename T, size_t N>
class SPSCQueue {
    std::array<T, N> buffer;
    std::atomic<size_t> head{0};
    std::atomic<size_t> tail{0};

   public:
    SPSCQueue() = default;

    bool push(T&& item) {
        auto current_tail = tail.load(std::memory_order_relaxed);
        auto next_tail = (current_tail + 1) % N;
        if (next_tail == head.load(std::memory_order_acquire)) {
            return false;
        }
        buffer[current_tail] = std::move(item);
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    bool push(const T& item) {
        auto current_tail = tail.load(std::memory_order_relaxed);
        auto next_tail = (current_tail + 1) % N;
        if (next_tail == head.load(std::memory_order_acquire)) {
            return false;
        }
        buffer[current_tail] = item;
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        auto current_head = head.load(std::memory_order_relaxed);
        if (current_head == tail.load(std::memory_order_acquire)) {
            return false;
        }
        item = std::move(buffer[current_head]);
        head.store((current_head + 1) % N, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool empty() const {
        return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
    }

    [[nodiscard]] size_t size() const {
        auto h = head.load(std::memory_order_acquire);
        auto t = tail.load(std::memory_order_acquire);
        if (t >= h) {
            return t - h;
        }
        return N - (h - t);
    }
};

}  // namespace Hardware

namespace Core {
using Hardware::SPSCQueue;
}
