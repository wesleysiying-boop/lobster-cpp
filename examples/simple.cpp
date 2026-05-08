// A minimal walk-through of the matching engine.
//
// Build:  cmake -S . -B build && cmake --build build
// Run:    ./build/examples/simple
#include <iomanip>
#include <iostream>

#include <lobster/engine.hpp>

using lobster::MatchingEngine;
using lobster::OrderRequest;
using lobster::OrderType;
using lobster::Side;

namespace {
void print_top_of_book(const lobster::OrderBook& book) {
    std::cout << "  TOB: ";
    if (book.has_best_bid()) {
        std::cout << "bid " << book.best_bid()
                  << " x " << book.bids().begin()->second.total_quantity();
    } else {
        std::cout << "bid -";
    }
    std::cout << "  |  ";
    if (book.has_best_ask()) {
        std::cout << "ask " << book.best_ask()
                  << " x " << book.asks().begin()->second.total_quantity();
    } else {
        std::cout << "ask -";
    }
    std::cout << "\n";
}
}  // namespace

int main() {
    MatchingEngine eng;

    std::cout << "1. Two resting bids and one resting ask:\n";
    eng.add(OrderRequest{
        .id = 1, .side = Side::Buy, .price = 4995, .quantity = 100, .type = OrderType::Limit});
    eng.add(OrderRequest{
        .id = 2, .side = Side::Buy, .price = 5000, .quantity = 50, .type = OrderType::Limit});
    eng.add(OrderRequest{
        .id = 3, .side = Side::Sell, .price = 5005, .quantity = 75, .type = OrderType::Limit});
    print_top_of_book(eng.book());

    std::cout << "\n2. Aggressive sell @ 5000 for 30 — should cross into id=2:\n";
    auto fills = eng.add(OrderRequest{.id = 4, .side = Side::Sell, .price = 5000,
                                      .quantity = 30, .type = OrderType::Limit});
    for (const auto& f : fills) {
        std::cout << "  fill: aggressor=" << f.aggressor_id
                  << "  resting=" << f.resting_id
                  << "  price=" << f.price
                  << "  qty=" << f.quantity << "\n";
    }
    print_top_of_book(eng.book());

    std::cout << "\n3. Cancel id=1, then market buy 200 — sweeps the ask side:\n";
    [[maybe_unused]] bool ok = eng.cancel(1);
    auto sweep = eng.add(OrderRequest{.id = 5, .side = Side::Buy, .price = 0,
                                      .quantity = 200, .type = OrderType::Market});
    for (const auto& f : sweep) {
        std::cout << "  fill: aggressor=" << f.aggressor_id
                  << "  resting=" << f.resting_id
                  << "  price=" << f.price
                  << "  qty=" << f.quantity << "\n";
    }
    print_top_of_book(eng.book());

    std::cout << "\nOpen orders remaining: " << eng.book().open_orders() << "\n";
    return 0;
}
