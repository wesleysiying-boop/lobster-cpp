// PriceLevel — FIFO of orders at one price.
//
// Resting orders are linked in arrival order. The matching engine consumes
// the front; cancellations remove an arbitrary middle node by unhooking it.
#pragma once

#include <cassert>
#include <cstddef>

#include <lobster/order.hpp>

namespace lobster {

class PriceLevel {
public:
    PriceLevel() noexcept = default;
    PriceLevel(const PriceLevel&) = delete;
    PriceLevel& operator=(const PriceLevel&) = delete;
    PriceLevel(PriceLevel&& other) noexcept
        : head_{other.head_}, tail_{other.tail_},
          count_{other.count_}, total_quantity_{other.total_quantity_} {
        other.head_ = other.tail_ = nullptr;
        other.count_ = 0;
        other.total_quantity_ = 0;
    }
    PriceLevel& operator=(PriceLevel&& other) noexcept {
        if (this != &other) {
            head_ = other.head_;
            tail_ = other.tail_;
            count_ = other.count_;
            total_quantity_ = other.total_quantity_;
            other.head_ = other.tail_ = nullptr;
            other.count_ = 0;
            other.total_quantity_ = 0;
        }
        return *this;
    }
    ~PriceLevel() = default;

    void push_back(Order* o) noexcept {
        assert(o != nullptr);
        o->prev = tail_;
        o->next = nullptr;
        if (tail_ != nullptr) {
            tail_->next = o;
        } else {
            head_ = o;
        }
        tail_ = o;
        ++count_;
        total_quantity_ += o->quantity;
    }

    void unlink(Order* o) noexcept {
        assert(o != nullptr);
        assert(count_ > 0);
        if (o->prev != nullptr) {
            o->prev->next = o->next;
        } else {
            head_ = o->next;
        }
        if (o->next != nullptr) {
            o->next->prev = o->prev;
        } else {
            tail_ = o->prev;
        }
        o->prev = o->next = nullptr;
        --count_;
        total_quantity_ -= o->quantity;
    }

    void on_fill(Order* o, Quantity filled) noexcept {
        assert(o != nullptr);
        assert(o->quantity >= filled);
        o->quantity -= filled;
        total_quantity_ -= filled;
    }

    [[nodiscard]] Order* front() const noexcept { return head_; }
    [[nodiscard]] bool empty() const noexcept { return head_ == nullptr; }
    [[nodiscard]] std::size_t size() const noexcept { return count_; }
    [[nodiscard]] Quantity total_quantity() const noexcept { return total_quantity_; }

private:
    Order* head_{nullptr};
    Order* tail_{nullptr};
    std::size_t count_{0};
    Quantity total_quantity_{0};
};

}  // namespace lobster
