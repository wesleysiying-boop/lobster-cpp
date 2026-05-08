// MatchingEngine — price-time priority matching with limit, market, and IOC.
//
// add() returns a vector of Fill records describing every trade generated
// by the incoming order. If the order is a limit and rests after partial
// crossing, the remainder is added to the book; if IOC or market, any
// remainder is dropped.
//
// The matcher walks the opposite side from best price inward, taking from
// the FIFO at each level until the cross is exhausted or the level is
// empty. This is canonical price-time priority — the same rules used by
// every continuous-cross exchange.
#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include <lobster/arena.hpp>
#include <lobster/book.hpp>
#include <lobster/order.hpp>
#include <lobster/types.hpp>

namespace lobster {

struct Fill {
    OrderId aggressor_id{kInvalidOrderId};
    OrderId resting_id{kInvalidOrderId};
    Price price{0};
    Quantity quantity{0};
    Timestamp timestamp{0};
};

struct OrderRequest {
    OrderId id{kInvalidOrderId};
    Side side{Side::Buy};
    Price price{0};        // ignored for Market
    Quantity quantity{0};
    OrderType type{OrderType::Limit};
    Timestamp timestamp{0};
};

class MatchingEngine {
public:
    explicit MatchingEngine(std::size_t arena_chunk = 4096) : arena_(arena_chunk) {}

    MatchingEngine(const MatchingEngine&) = delete;
    MatchingEngine& operator=(const MatchingEngine&) = delete;

    std::vector<Fill> add(const OrderRequest& req) {
        std::vector<Fill> fills;
        fills.reserve(4);

        Quantity remaining = req.quantity;
        if (remaining == 0) {
            return fills;
        }

        if (req.side == Side::Buy) {
            remaining = match_buy_(req, remaining, fills);
        } else {
            remaining = match_sell_(req, remaining, fills);
        }

        if (remaining > 0 && req.type == OrderType::Limit) {
            Order* o = arena_.allocate();
            o->id = req.id;
            o->side = req.side;
            o->type = req.type;
            o->price = req.price;
            o->quantity = remaining;
            o->initial_quantity = req.quantity;
            o->timestamp = req.timestamp;
            book_.add_resting(o);
        }
        // IOC / Market: any remainder is dropped (cancelled).
        return fills;
    }

    /// Cancel by id. Returns true if the order existed and was removed.
    [[nodiscard]] bool cancel(OrderId id) {
        Order* o = book_.find(id);
        if (o == nullptr) {
            return false;
        }
        book_.remove(o);
        arena_.deallocate(o);
        return true;
    }

    [[nodiscard]] const OrderBook& book() const noexcept { return book_; }
    [[nodiscard]] OrderBook& book() noexcept { return book_; }
    [[nodiscard]] const OrderArena& arena() const noexcept { return arena_; }

private:
    // Aggressive buy crosses against asks ascending until done or limit binds.
    Quantity match_buy_(const OrderRequest& req, Quantity remaining,
                        std::vector<Fill>& fills) {
        auto& asks = book_.asks();
        while (remaining > 0 && !asks.empty()) {
            auto best = asks.begin();
            Price best_price = best->first;
            if (req.type == OrderType::Limit && best_price > req.price) {
                break;  // no longer crossing
            }
            PriceLevel& level = best->second;
            remaining = drain_level_(req, remaining, best_price, level, fills);
            if (level.empty()) {
                asks.erase(best);
            }
        }
        return remaining;
    }

    Quantity match_sell_(const OrderRequest& req, Quantity remaining,
                         std::vector<Fill>& fills) {
        auto& bids = book_.bids();
        while (remaining > 0 && !bids.empty()) {
            auto best = bids.begin();
            Price best_price = best->first;
            if (req.type == OrderType::Limit && best_price < req.price) {
                break;
            }
            PriceLevel& level = best->second;
            remaining = drain_level_(req, remaining, best_price, level, fills);
            if (level.empty()) {
                bids.erase(best);
            }
        }
        return remaining;
    }

    Quantity drain_level_(const OrderRequest& req, Quantity remaining,
                          Price exec_price, PriceLevel& level,
                          std::vector<Fill>& fills) {
        while (remaining > 0 && !level.empty()) {
            Order* head = level.front();
            Quantity take = std::min(remaining, head->quantity);
            fills.push_back(Fill{
                .aggressor_id = req.id,
                .resting_id = head->id,
                .price = exec_price,
                .quantity = take,
                .timestamp = req.timestamp,
            });
            remaining -= take;
            // PriceLevel::on_fill keeps the level's total_quantity_ in sync.
            level.on_fill(head, take);
            if (head->quantity == 0) {
                level.unlink(head);
                book_.erase_id(head->id);
                arena_.deallocate(head);
            }
        }
        return remaining;
    }

    OrderBook book_;
    OrderArena arena_;
};

}  // namespace lobster
