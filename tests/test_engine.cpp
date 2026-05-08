#include <catch2/catch_test_macros.hpp>
#include <lobster/engine.hpp>

using lobster::MatchingEngine;
using lobster::Order;
using lobster::OrderRequest;
using lobster::OrderType;
using lobster::Side;

namespace {
OrderRequest req(lobster::OrderId id, Side s, lobster::Price p, lobster::Quantity q,
                 OrderType t = OrderType::Limit) {
    return OrderRequest{.id = id, .side = s, .price = p, .quantity = q, .type = t, .timestamp = 0};
}
}  // namespace

TEST_CASE("Resting limit orders do not produce fills", "[engine]") {
    MatchingEngine eng;
    auto fills = eng.add(req(1, Side::Buy, 5000, 100));
    REQUIRE(fills.empty());
    REQUIRE(eng.book().best_bid() == 5000);
}

TEST_CASE("Aggressive cross fills fully against single resting", "[engine]") {
    MatchingEngine eng;
    eng.add(req(1, Side::Sell, 5000, 100));
    auto fills = eng.add(req(2, Side::Buy, 5000, 100));
    REQUIRE(fills.size() == 1);
    REQUIRE(fills[0].aggressor_id == 2);
    REQUIRE(fills[0].resting_id == 1);
    REQUIRE(fills[0].price == 5000);
    REQUIRE(fills[0].quantity == 100u);
    REQUIRE(eng.book().asks().empty());
    REQUIRE(eng.book().bids().empty());
}

TEST_CASE("Partial cross leaves remainder resting on aggressor side", "[engine]") {
    MatchingEngine eng;
    eng.add(req(1, Side::Sell, 5000, 30));
    auto fills = eng.add(req(2, Side::Buy, 5000, 100));
    REQUIRE(fills.size() == 1);
    REQUIRE(fills[0].quantity == 30u);
    REQUIRE(eng.book().best_bid() == 5000);   // remainder rested
    REQUIRE(eng.book().asks().empty());
}

TEST_CASE("FIFO time priority within a price level", "[engine]") {
    MatchingEngine eng;
    eng.add(req(1, Side::Sell, 5000, 50));   // arrived first
    eng.add(req(2, Side::Sell, 5000, 50));   // arrived second
    auto fills = eng.add(req(3, Side::Buy, 5000, 60));

    REQUIRE(fills.size() == 2);
    REQUIRE(fills[0].resting_id == 1);
    REQUIRE(fills[0].quantity == 50u);
    REQUIRE(fills[1].resting_id == 2);
    REQUIRE(fills[1].quantity == 10u);
}

TEST_CASE("Best-price priority across levels", "[engine]") {
    MatchingEngine eng;
    eng.add(req(1, Side::Sell, 5005, 100));
    eng.add(req(2, Side::Sell, 5000, 100));   // better (cheaper) ask
    auto fills = eng.add(req(3, Side::Buy, 5005, 50));
    REQUIRE(fills.size() == 1);
    REQUIRE(fills[0].resting_id == 2);        // hit the better-priced one
    REQUIRE(fills[0].price == 5000);
}

TEST_CASE("Limit price binds — no fill above the limit on a buy", "[engine]") {
    MatchingEngine eng;
    eng.add(req(1, Side::Sell, 5005, 100));
    auto fills = eng.add(req(2, Side::Buy, 5000, 50));
    REQUIRE(fills.empty());
    REQUIRE(eng.book().best_bid() == 5000);
}

TEST_CASE("Market order crosses through all levels", "[engine]") {
    MatchingEngine eng;
    eng.add(req(1, Side::Sell, 5000, 30));
    eng.add(req(2, Side::Sell, 5005, 30));
    eng.add(req(3, Side::Sell, 5010, 30));
    auto fills = eng.add(req(99, Side::Buy, 0, 80, OrderType::Market));
    REQUIRE(fills.size() == 3);
    REQUIRE(fills[0].quantity == 30u);
    REQUIRE(fills[1].quantity == 30u);
    REQUIRE(fills[2].quantity == 20u);
    REQUIRE(eng.book().best_ask() == 5010);
    REQUIRE(eng.book().asks().begin()->second.total_quantity() == 10u);
}

TEST_CASE("IOC drops unfilled remainder", "[engine]") {
    MatchingEngine eng;
    eng.add(req(1, Side::Sell, 5000, 20));
    auto fills = eng.add(req(2, Side::Buy, 5000, 100, OrderType::Ioc));
    REQUIRE(fills.size() == 1);
    REQUIRE(fills[0].quantity == 20u);
    REQUIRE(eng.book().bids().empty());        // no remainder rested
    REQUIRE(eng.book().asks().empty());
}

TEST_CASE("Cancel removes by id, returns false on miss", "[engine]") {
    MatchingEngine eng;
    eng.add(req(1, Side::Buy, 5000, 100));
    REQUIRE(eng.cancel(1));
    REQUIRE_FALSE(eng.cancel(1));   // already gone
    REQUIRE(eng.book().bids().empty());
}

TEST_CASE("After many non-crossing add/cancel cycles, book conserves count", "[engine]") {
    MatchingEngine eng;
    // Bids well below asks so nothing crosses — every order rests.
    for (lobster::OrderId i = 1; i <= 100; ++i) {
        eng.add(req(i, Side::Buy, static_cast<lobster::Price>(4900 + (i % 10)), 10));
    }
    for (lobster::OrderId i = 101; i <= 200; ++i) {
        eng.add(req(i, Side::Sell, static_cast<lobster::Price>(5100 + (i % 10)), 10));
    }
    REQUIRE(eng.book().open_orders() == 200);
    for (lobster::OrderId i = 1; i <= 200; i += 2) {
        REQUIRE(eng.cancel(i));
    }
    REQUIRE(eng.book().open_orders() == 100);
}
