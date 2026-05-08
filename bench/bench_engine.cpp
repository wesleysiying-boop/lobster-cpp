// Latency benchmarks for the matching engine.
//
// These measure the steady-state cost of the three operations a real engine
// spends 99% of its cycles on: posting a resting order, an aggressive cross
// against an existing best level, and a cancel by id.
//
// We pre-populate the book and recycle order ids so the arena's free list
// dominates over fresh chunk allocations — this is the regime production
// engines actually run in.

#include <benchmark/benchmark.h>
#include <lobster/engine.hpp>

#include <random>

namespace {

using lobster::MatchingEngine;
using lobster::OrderRequest;
using lobster::OrderType;
using lobster::Price;
using lobster::Side;

OrderRequest req(lobster::OrderId id, Side s, Price p, lobster::Quantity q,
                 OrderType t = OrderType::Limit) {
    return OrderRequest{.id = id, .side = s, .price = p, .quantity = q, .type = t, .timestamp = 0};
}

void BM_AddResting(benchmark::State& state) {
    MatchingEngine eng;
    lobster::OrderId id = 1;
    // Pre-fill far-from-cross levels so subsequent adds also rest.
    for (int i = 0; i < 100; ++i) {
        eng.add(req(id++, Side::Buy, 4000 + i, 10));
    }
    std::mt19937_64 rng{42};
    std::uniform_int_distribution<Price> px{4000, 4500};
    for (auto _ : state) {
        auto fills = eng.add(req(id++, Side::Buy, px(rng), 10));
        benchmark::DoNotOptimize(fills);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AddResting);

void BM_AggressiveCross_OneLevel(benchmark::State& state) {
    MatchingEngine eng;
    lobster::OrderId id = 1;
    for (auto _ : state) {
        state.PauseTiming();
        eng.add(req(id++, Side::Sell, 5000, 10));  // resting ask
        state.ResumeTiming();
        auto fills = eng.add(req(id++, Side::Buy, 5000, 10));  // aggressive
        benchmark::DoNotOptimize(fills);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AggressiveCross_OneLevel);

void BM_Cancel(benchmark::State& state) {
    MatchingEngine eng;
    lobster::OrderId id = 1;
    // Maintain a steady rolling pool of resting orders and cancel from it.
    constexpr int pool_size = 4096;
    std::vector<lobster::OrderId> ids;
    ids.reserve(pool_size);
    for (int i = 0; i < pool_size; ++i) {
        eng.add(req(id, Side::Buy, 5000 + (i % 50), 10));
        ids.push_back(id++);
    }
    std::size_t cursor = 0;
    for (auto _ : state) {
        bool ok = eng.cancel(ids[cursor]);
        benchmark::DoNotOptimize(ok);
        // Refill the slot so the pool stays at a steady size.
        eng.add(req(id, Side::Buy, 5000 + static_cast<Price>(cursor % 50), 10));
        ids[cursor] = id++;
        cursor = (cursor + 1) % ids.size();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Cancel);

}  // namespace

BENCHMARK_MAIN();
