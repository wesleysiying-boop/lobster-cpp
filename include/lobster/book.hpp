// OrderBook — sorted bid / ask ladders.
//
// Bids and asks are kept in separate std::map<Price, PriceLevel>. We iterate
// asks ascending (cheapest first) and bids descending (most expensive first)
// when matching aggressive orders.
//
// `id_index` is a flat hash map from OrderId to Order* so cancel-by-id is
// O(1) average.
#pragma once

#include <cstddef>
#include <map>
#include <unordered_map>

#include <lobster/order.hpp>
#include <lobster/price_level.hpp>
#include <lobster/types.hpp>

namespace lobster {

class OrderBook {
public:
    using BidLadder = std::map<Price, PriceLevel, std::greater<>>;  // descending
    using AskLadder = std::map<Price, PriceLevel>;                  // ascending

    OrderBook() = default;
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    void add_resting(Order* o) {
        if (o->side == Side::Buy) {
            bids_[o->price].push_back(o);
        } else {
            asks_[o->price].push_back(o);
        }
        id_index_.emplace(o->id, o);
    }

    [[nodiscard]] Order* find(OrderId id) const {
        auto it = id_index_.find(id);
        return it == id_index_.end() ? nullptr : it->second;
    }

    void remove(Order* o) {
        if (o->side == Side::Buy) {
            auto lit = bids_.find(o->price);
            if (lit != bids_.end()) {
                lit->second.unlink(o);
                if (lit->second.empty()) {
                    bids_.erase(lit);
                }
            }
        } else {
            auto lit = asks_.find(o->price);
            if (lit != asks_.end()) {
                lit->second.unlink(o);
                if (lit->second.empty()) {
                    asks_.erase(lit);
                }
            }
        }
        id_index_.erase(o->id);
    }

    void on_full_fill(Order* o) {
        // The level keeps the order in the FIFO until we explicitly unlink
        // it; this matches the engine's match() loop, which pops the front
        // of a level once its head order is fully filled.
        if (o->side == Side::Buy) {
            auto lit = bids_.find(o->price);
            if (lit != bids_.end()) {
                lit->second.unlink(o);
                if (lit->second.empty()) {
                    bids_.erase(lit);
                }
            }
        } else {
            auto lit = asks_.find(o->price);
            if (lit != asks_.end()) {
                lit->second.unlink(o);
                if (lit->second.empty()) {
                    asks_.erase(lit);
                }
            }
        }
        id_index_.erase(o->id);
    }

    void on_partial_fill(Order* o, Quantity filled) {
        if (o->side == Side::Buy) {
            bids_[o->price].on_fill(o, filled);
        } else {
            asks_[o->price].on_fill(o, filled);
        }
    }

    /// Drop an order from the id-index without touching the ladders.
    /// The caller must already have unlinked the order from its PriceLevel.
    void erase_id(OrderId id) noexcept { id_index_.erase(id); }

    [[nodiscard]] BidLadder& bids() noexcept { return bids_; }
    [[nodiscard]] const BidLadder& bids() const noexcept { return bids_; }
    [[nodiscard]] AskLadder& asks() noexcept { return asks_; }
    [[nodiscard]] const AskLadder& asks() const noexcept { return asks_; }

    [[nodiscard]] bool has_best_bid() const noexcept { return !bids_.empty(); }
    [[nodiscard]] bool has_best_ask() const noexcept { return !asks_.empty(); }
    [[nodiscard]] Price best_bid() const noexcept { return bids_.begin()->first; }
    [[nodiscard]] Price best_ask() const noexcept { return asks_.begin()->first; }
    [[nodiscard]] std::size_t open_orders() const noexcept { return id_index_.size(); }

private:
    BidLadder bids_;
    AskLadder asks_;
    std::unordered_map<OrderId, Order*> id_index_;
};

}  // namespace lobster
