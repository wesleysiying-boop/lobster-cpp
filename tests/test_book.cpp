#include <catch2/catch_test_macros.hpp>
#include <lobster/arena.hpp>
#include <lobster/book.hpp>

namespace {
lobster::Order* make(lobster::OrderArena& arena, lobster::OrderId id, lobster::Side s,
                    lobster::Price p, lobster::Quantity q) {
    auto* o = arena.allocate();
    o->id = id;
    o->side = s;
    o->type = lobster::OrderType::Limit;
    o->price = p;
    o->quantity = q;
    o->initial_quantity = q;
    return o;
}
}  // namespace

TEST_CASE("OrderBook tracks best bid / best ask", "[book]") {
    lobster::OrderArena arena;
    lobster::OrderBook book;
    book.add_resting(make(arena, 1, lobster::Side::Buy, 100, 10));
    book.add_resting(make(arena, 2, lobster::Side::Buy, 101, 10));
    book.add_resting(make(arena, 3, lobster::Side::Sell, 105, 10));
    book.add_resting(make(arena, 4, lobster::Side::Sell, 104, 10));

    REQUIRE(book.best_bid() == 101);
    REQUIRE(book.best_ask() == 104);
    REQUIRE(book.open_orders() == 4);
}

TEST_CASE("OrderBook find / remove by id", "[book]") {
    lobster::OrderArena arena;
    lobster::OrderBook book;
    auto* o = make(arena, 42, lobster::Side::Buy, 100, 5);
    book.add_resting(o);

    REQUIRE(book.find(42) == o);
    book.remove(o);
    REQUIRE(book.find(42) == nullptr);
    REQUIRE(book.bids().empty());
}

TEST_CASE("OrderBook removes empty levels after last order goes", "[book]") {
    lobster::OrderArena arena;
    lobster::OrderBook book;
    auto* a = make(arena, 1, lobster::Side::Sell, 100, 5);
    auto* b = make(arena, 2, lobster::Side::Sell, 100, 5);
    book.add_resting(a);
    book.add_resting(b);
    REQUIRE(book.asks().size() == 1u);  // one price level

    book.remove(a);
    REQUIRE(book.asks().size() == 1u);  // still one — b remains
    book.remove(b);
    REQUIRE(book.asks().empty());
}
