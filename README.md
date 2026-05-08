# lobster-cpp

> A small, fast limit-order-book matching engine in C++20. No virtuals on the hot path, no malloc in steady state, intrusive lists for O(1) cancel. Header-only.

[![CI](https://github.com/wesleysiying-boop/lobster-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/wesleysiying-boop/lobster-cpp/actions/workflows/ci.yml)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

`lobster-cpp` is a self-contained limit-order-book (LOB) matching engine
focused on the hot path: an `add` or `cancel` should take roughly the same
time whether the book has 10 orders or 10,000. It's the matching kernel
you'd drop into a backtester or a paper-trading exchange — small enough
to read, fast enough to study cache and branch behavior on.

The project is intended to pair with [`quantforge`](https://github.com/wesleysiying-boop/quantforge):
that project's Python event-driven backtester can plug `lobster-cpp` in
as its execution venue when you want a higher-fidelity simulation than
"market orders fill at the next bar's open."

---

## Design

```
                   ┌──────────────────────────────────┐
                   │           OrderBook              │
                   │                                  │
                   │  bids: std::map<Price, Level>    │  iterate desc
                   │  asks: std::map<Price, Level>    │  iterate asc
                   │                                  │
                   │  Each Level holds an intrusive   │
                   │  doubly-linked FIFO of orders.   │
                   │                                  │
                   │  id_index: hashmap<id, Order*>   │  O(1) cancel
                   └─────────────────┬────────────────┘
                                     │
                  ┌──────────────────┴──────────────────┐
                  │           MatchingEngine            │
                  │  add()   — limit / market / IOC     │
                  │  cancel()— O(1) via id_index        │
                  │  match() — walk best price levels   │
                  │            until cross is exhausted │
                  └─────────────────────────────────────┘
```

Why these choices:

* **`std::map<Price, Level>`** for price levels. `O(log n)` insertion of a
  brand-new price level is fine; the common case is hitting an existing
  level, which is `O(log n)` for the lookup but `O(1)` for the append. A
  flat array indexed by tick is faster for narrow markets but pathological
  for wide ones; `std::map` is the more honest default.
* **Intrusive doubly-linked list** within each level — a cancel removes
  exactly one node by stitching its neighbors. No traversal, no
  reallocations, no shift-down.
* **Arena allocator** for `Order` nodes — one big upfront allocation, then
  free-list reuse. Steady-state operation does zero `malloc`/`free`.
* **`uint64_t` order IDs**, **`int64_t` price ticks**, **`uint32_t`
  quantities** — POD types, no virtual functions on the hot path, trivial
  to memcpy.
* **`SpscQueue`** for inbound order events, designed for one writer thread
  and one consumer (the engine) — uses `std::atomic` with `memory_order_*`
  carefully chosen, no locks.

---

## Quick start

```bash
git clone https://github.com/wesleysiying-boop/lobster-cpp.git
cd lobster-cpp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
build/bench/lobster_bench       # latency benchmarks
build/examples/simple           # toy demo
```

Minimal usage:

```cpp
#include <lobster/engine.hpp>

int main() {
    lobster::MatchingEngine engine;

    // Resting bid: 100 shares @ $50.00 (price in ticks of $0.01 → 5000)
    engine.add({.id = 1, .side = lobster::Side::Buy,
                .price = 5000, .quantity = 100, .type = lobster::OrderType::Limit});

    // Aggressive sell: 30 shares @ $50.00 — crosses, 30 fill, 70 remaining at bid
    auto fills = engine.add({.id = 2, .side = lobster::Side::Sell,
                             .price = 5000, .quantity = 30, .type = lobster::OrderType::Limit});

    // fills[0]: aggressor=2, resting=1, price=5000, quantity=30
}
```

---

## Status

Work in progress, built incrementally. Roadmap reflects reality.

### Roadmap

- [x] CMake build, CI on Linux + macOS with both clang and gcc
- [x] Core types: Order, Side, OrderType, intrusive list hooks
- [x] PriceLevel with FIFO semantics
- [x] OrderBook with sorted bids/asks and id-index
- [x] MatchingEngine: limit, market, IOC, cancel
- [x] Arena allocator for Order nodes
- [x] SPSC lock-free queue
- [x] Catch2 unit tests covering match priority, partial fills, cancel races
- [x] Google Benchmark suite — `add_aggressive_match`, `cancel`, `add_resting`
- [ ] FOK (fill-or-kill) and STP (self-trade prevention) order types
- [ ] Pegged orders (peg to best bid / ask)
- [ ] pybind11 Python bindings → use as a venue inside `quantforge`
- [ ] L2 snapshot + delta replay for offline analysis
- [ ] Lock-free MPSC queue variant for multi-feed input

### Companion projects

- [`quantforge`](https://github.com/wesleysiying-boop/quantforge) — Python event-driven backtester. `lobster-cpp` is intended to slot in as a higher-fidelity broker.

---

## Benchmarks

Reproducible via `cmake --build build && build/bench/lobster_bench`.
Numbers below from Apple M-series, `Release` (`-O3`), no thermal throttle:

| Op                                     | Median   |  Rate     |
| -------------------------------------- | -------: | --------: |
| Add resting limit (no cross)           |  125 ns  |  8.1 M/s  |
| Cancel by id                           |  116 ns  |  8.6 M/s  |
| Aggressive limit, single-level cross\* |  828 ns  |  1.2 M/s  |

\* The cross-bench is conservative because each iteration also re-posts
the resting order (so the level isn't permanently empty), and because
`std::vector<Fill>` allocation is included. The matching code itself is
similar to add/cancel; the `vector` and pause/resume scaffolding dominate.

---

## License

[MIT](LICENSE) © 2026 Wesley Si Ying
