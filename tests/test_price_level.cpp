#include <catch2/catch_test_macros.hpp>
#include <lobster/order.hpp>
#include <lobster/price_level.hpp>

namespace {
lobster::Order make_order(lobster::OrderId id, lobster::Quantity qty) {
    return lobster::Order{
        .id = id, .side = lobster::Side::Buy, .type = lobster::OrderType::Limit,
        .price = 100, .quantity = qty, .initial_quantity = qty, .timestamp = 0,
    };
}
}  // namespace

TEST_CASE("PriceLevel maintains FIFO order", "[price_level]") {
    lobster::PriceLevel level;
    auto o1 = make_order(1, 10);
    auto o2 = make_order(2, 20);
    auto o3 = make_order(3, 30);
    level.push_back(&o1);
    level.push_back(&o2);
    level.push_back(&o3);

    REQUIRE(level.size() == 3);
    REQUIRE(level.total_quantity() == 60u);
    REQUIRE(level.front() == &o1);
}

TEST_CASE("PriceLevel unlinks middle node correctly", "[price_level]") {
    lobster::PriceLevel level;
    auto o1 = make_order(1, 10);
    auto o2 = make_order(2, 20);
    auto o3 = make_order(3, 30);
    level.push_back(&o1);
    level.push_back(&o2);
    level.push_back(&o3);

    level.unlink(&o2);

    REQUIRE(level.size() == 2);
    REQUIRE(level.total_quantity() == 40u);
    REQUIRE(level.front() == &o1);
    REQUIRE(o1.next == &o3);
    REQUIRE(o3.prev == &o1);
}

TEST_CASE("PriceLevel unlinks head and tail correctly", "[price_level]") {
    lobster::PriceLevel level;
    auto o1 = make_order(1, 10);
    auto o2 = make_order(2, 20);
    level.push_back(&o1);
    level.push_back(&o2);

    level.unlink(&o1);
    REQUIRE(level.front() == &o2);
    REQUIRE(o2.prev == nullptr);

    level.unlink(&o2);
    REQUIRE(level.empty());
}

TEST_CASE("PriceLevel on_fill decrements quantity correctly", "[price_level]") {
    lobster::PriceLevel level;
    auto o1 = make_order(1, 100);
    level.push_back(&o1);
    level.on_fill(&o1, 30);
    REQUIRE(o1.quantity == 70u);
    REQUIRE(level.total_quantity() == 70u);
}
