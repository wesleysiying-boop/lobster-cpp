// Basic POD types used throughout the matching engine.
//
// Price is stored as a signed integer tick so we never compare or arithmetic
// floating-point amounts inside the hot path. Quantities are unsigned; an
// order with quantity 0 is a bug, not a state.
#pragma once

#include <cstdint>
#include <limits>

namespace lobster {

using Price = std::int64_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;
using Timestamp = std::uint64_t;  // nanoseconds since epoch

inline constexpr Price kPriceMin = std::numeric_limits<Price>::min();
inline constexpr Price kPriceMax = std::numeric_limits<Price>::max();
inline constexpr OrderId kInvalidOrderId = 0;

enum class Side : std::uint8_t {
    Buy = 0,
    Sell = 1,
};

enum class OrderType : std::uint8_t {
    Limit = 0,
    Market = 1,
    Ioc = 2,  // Immediate-or-cancel: cross now, kill any remainder
};

[[nodiscard]] constexpr Side opposite(Side s) noexcept {
    return s == Side::Buy ? Side::Sell : Side::Buy;
}

}  // namespace lobster
