// Lock-free single-producer / single-consumer ring buffer.
//
// One thread calls `try_push` and only it ever advances `head_`.
// One thread calls `try_pop`  and only it ever advances `tail_`.
// Memory ordering chosen so the producer's writes to the buffer slot are
// visible to the consumer when it observes the new `head_`, and vice
// versa for the consumer's reads of `tail_`.
//
// Capacity is rounded up to a power of two so the modulo wrap is a mask.
#pragma once

#include <atomic>
#include <bit>
#include <cstddef>
#include <optional>
#include <vector>

namespace lobster {

template <typename T>
class SpscQueue {
public:
    explicit SpscQueue(std::size_t capacity)
        : mask_(round_up_pow2_(capacity) - 1),
          buffer_(round_up_pow2_(capacity)) {}

    SpscQueue(const SpscQueue&) = delete;
    SpscQueue& operator=(const SpscQueue&) = delete;

    [[nodiscard]] bool try_push(const T& value) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t next = (head + 1) & mask_;
        if (next == tail_.load(std::memory_order_acquire)) {
            return false;  // full
        }
        buffer_[head] = value;
        head_.store(next, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool try_push(T&& value) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t next = (head + 1) & mask_;
        if (next == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        buffer_[head] = std::move(value);
        head_.store(next, std::memory_order_release);
        return true;
    }

    [[nodiscard]] std::optional<T> try_pop() {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt;  // empty
        }
        T value = std::move(buffer_[tail]);
        tail_.store((tail + 1) & mask_, std::memory_order_release);
        return value;
    }

    [[nodiscard]] bool empty() const noexcept {
        return tail_.load(std::memory_order_acquire) ==
               head_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t capacity() const noexcept { return mask_ + 1; }

private:
    static std::size_t round_up_pow2_(std::size_t n) noexcept {
        if (n < 2) {
            return 2;
        }
        return std::bit_ceil(n);
    }

    const std::size_t mask_;
    std::vector<T> buffer_;
    // 64-byte alignment to keep producer and consumer state on separate
    // cache lines — no false sharing between push / pop hot paths.
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
};

}  // namespace lobster
