// Slab arena allocator for Order nodes.
//
// Steady-state operation should never call malloc. We grow by appending new
// chunks (each a contiguous std::vector<Order>) when the free list is empty
// and there's room in the current chunk, but in a typical run the free list
// dominates: a fill releases the order back to the free list, the next add
// pops from it.
//
// Note: pointers handed out remain stable for the arena's lifetime; we never
// move existing chunks. The book stores Order* directly.
#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

#include <lobster/order.hpp>

namespace lobster {

class OrderArena {
public:
    explicit OrderArena(std::size_t chunk_size = 4096) : chunk_size_(chunk_size) {
        chunks_.emplace_back(std::make_unique<Chunk>(chunk_size_));
    }

    OrderArena(const OrderArena&) = delete;
    OrderArena& operator=(const OrderArena&) = delete;
    OrderArena(OrderArena&&) noexcept = default;
    OrderArena& operator=(OrderArena&&) noexcept = default;
    ~OrderArena() = default;

    [[nodiscard]] Order* allocate() {
        if (free_head_ != nullptr) {
            Order* node = free_head_;
            free_head_ = free_head_->next;
            *node = Order{};  // reset
            return node;
        }
        Chunk& cur = *chunks_.back();
        if (cur.used == cur.storage.size()) {
            chunks_.emplace_back(std::make_unique<Chunk>(chunk_size_));
        }
        Chunk& target = *chunks_.back();
        Order* node = &target.storage[target.used++];
        *node = Order{};
        return node;
    }

    void deallocate(Order* node) noexcept {
        assert(node != nullptr);
        node->prev = nullptr;
        node->next = free_head_;
        free_head_ = node;
    }

    [[nodiscard]] std::size_t allocated_chunks() const noexcept { return chunks_.size(); }
    [[nodiscard]] std::size_t chunk_size() const noexcept { return chunk_size_; }

private:
    struct Chunk {
        explicit Chunk(std::size_t n) : storage(n), used{0} {}
        std::vector<Order> storage;
        std::size_t used;
    };

    std::size_t chunk_size_;
    std::vector<std::unique_ptr<Chunk>> chunks_;
    Order* free_head_{nullptr};
};

}  // namespace lobster
