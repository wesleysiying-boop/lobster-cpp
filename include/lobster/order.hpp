// Order — POD with intrusive doubly-linked list hooks.
//
// The hooks let a PriceLevel maintain a FIFO of resting orders without ever
// allocating list nodes — the order *is* its own list node. Cancel becomes
// "stitch neighbors", which is O(1) and touches no other memory than the
// adjacent two cache lines.
#pragma once

#include <lobster/types.hpp>

namespace lobster {

struct Order {
    OrderId id{kInvalidOrderId};
    Side side{Side::Buy};
    OrderType type{OrderType::Limit};
    Price price{0};
    Quantity quantity{0};      // remaining; decremented on partial fill
    Quantity initial_quantity{0};
    Timestamp timestamp{0};

    // Intrusive list hooks owned by the PriceLevel that contains this order.
    Order* prev{nullptr};
    Order* next{nullptr};

    [[nodiscard]] constexpr bool is_filled() const noexcept { return quantity == 0; }
};

}  // namespace lobster
