#include <catch2/catch_test_macros.hpp>
#include <lobster/arena.hpp>

#include <unordered_set>

TEST_CASE("OrderArena hands out distinct, stable pointers", "[arena]") {
    lobster::OrderArena arena{16};
    std::unordered_set<lobster::Order*> seen;
    for (int i = 0; i < 100; ++i) {
        lobster::Order* o = arena.allocate();
        REQUIRE(o != nullptr);
        REQUIRE(seen.insert(o).second);  // unique
    }
    REQUIRE(arena.allocated_chunks() > 1);  // grew past first chunk
}

TEST_CASE("OrderArena reuses freed nodes via free list", "[arena]") {
    lobster::OrderArena arena{4};
    lobster::Order* a = arena.allocate();
    lobster::Order* b = arena.allocate();
    arena.deallocate(a);
    lobster::Order* c = arena.allocate();
    REQUIRE(c == a);  // last freed comes back first (LIFO)
    arena.deallocate(b);
    arena.deallocate(c);
}

TEST_CASE("OrderArena resets allocated nodes", "[arena]") {
    lobster::OrderArena arena{4};
    lobster::Order* a = arena.allocate();
    a->id = 12345;
    a->quantity = 99;
    arena.deallocate(a);
    lobster::Order* b = arena.allocate();
    REQUIRE(b == a);
    REQUIRE(b->id == lobster::kInvalidOrderId);
    REQUIRE(b->quantity == 0u);
}
