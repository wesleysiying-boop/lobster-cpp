// pybind11 bindings for the lobster matching engine.
//
// Exposes the public API that a Python user actually needs: enums,
// OrderRequest, Fill, and MatchingEngine.add / cancel / a small set of
// inspection methods on the book. Internal types like Order, PriceLevel,
// and OrderBook itself are deliberately NOT exposed — they hold raw
// pointers into the engine's arena and would be unsafe to hand to Python.

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <lobster/engine.hpp>

namespace py = pybind11;

namespace {

std::vector<std::pair<lobster::Price, lobster::Quantity>>
bid_levels(const lobster::MatchingEngine& eng, std::size_t max_levels) {
    std::vector<std::pair<lobster::Price, lobster::Quantity>> out;
    out.reserve(std::min<std::size_t>(max_levels, eng.book().bids().size()));
    for (const auto& [px, level] : eng.book().bids()) {
        if (out.size() >= max_levels) break;
        out.emplace_back(px, level.total_quantity());
    }
    return out;
}

std::vector<std::pair<lobster::Price, lobster::Quantity>>
ask_levels(const lobster::MatchingEngine& eng, std::size_t max_levels) {
    std::vector<std::pair<lobster::Price, lobster::Quantity>> out;
    out.reserve(std::min<std::size_t>(max_levels, eng.book().asks().size()));
    for (const auto& [px, level] : eng.book().asks()) {
        if (out.size() >= max_levels) break;
        out.emplace_back(px, level.total_quantity());
    }
    return out;
}

}  // namespace

PYBIND11_MODULE(lobster_py, m) {
    m.doc() = "Python bindings for the lobster matching engine.";

    py::enum_<lobster::Side>(m, "Side")
        .value("Buy", lobster::Side::Buy)
        .value("Sell", lobster::Side::Sell);

    py::enum_<lobster::OrderType>(m, "OrderType")
        .value("Limit", lobster::OrderType::Limit)
        .value("Market", lobster::OrderType::Market)
        .value("Ioc", lobster::OrderType::Ioc);

    py::class_<lobster::OrderRequest>(m, "OrderRequest")
        .def(py::init([](lobster::OrderId id, lobster::Side side, lobster::Price price,
                         lobster::Quantity quantity, lobster::OrderType type,
                         lobster::Timestamp timestamp) {
                 return lobster::OrderRequest{
                     .id = id, .side = side, .price = price, .quantity = quantity,
                     .type = type, .timestamp = timestamp,
                 };
             }),
             py::arg("id"), py::arg("side"), py::arg("price"),
             py::arg("quantity"), py::arg("type") = lobster::OrderType::Limit,
             py::arg("timestamp") = static_cast<lobster::Timestamp>(0))
        .def_readwrite("id", &lobster::OrderRequest::id)
        .def_readwrite("side", &lobster::OrderRequest::side)
        .def_readwrite("price", &lobster::OrderRequest::price)
        .def_readwrite("quantity", &lobster::OrderRequest::quantity)
        .def_readwrite("type", &lobster::OrderRequest::type)
        .def_readwrite("timestamp", &lobster::OrderRequest::timestamp)
        .def("__repr__", [](const lobster::OrderRequest& r) {
            return "OrderRequest(id=" + std::to_string(r.id) +
                   ", side=" + std::string(r.side == lobster::Side::Buy ? "Buy" : "Sell") +
                   ", price=" + std::to_string(r.price) +
                   ", quantity=" + std::to_string(r.quantity) + ")";
        });

    py::class_<lobster::Fill>(m, "Fill")
        .def_readonly("aggressor_id", &lobster::Fill::aggressor_id)
        .def_readonly("resting_id", &lobster::Fill::resting_id)
        .def_readonly("price", &lobster::Fill::price)
        .def_readonly("quantity", &lobster::Fill::quantity)
        .def_readonly("timestamp", &lobster::Fill::timestamp)
        .def("__repr__", [](const lobster::Fill& f) {
            return "Fill(aggressor=" + std::to_string(f.aggressor_id) +
                   ", resting=" + std::to_string(f.resting_id) +
                   ", price=" + std::to_string(f.price) +
                   ", qty=" + std::to_string(f.quantity) + ")";
        });

    py::class_<lobster::MatchingEngine>(m, "MatchingEngine")
        .def(py::init<std::size_t>(), py::arg("arena_chunk") = 4096)
        .def("add", &lobster::MatchingEngine::add, py::arg("request"),
             "Submit an order request. Returns the list of fills it produced.")
        .def("cancel", &lobster::MatchingEngine::cancel, py::arg("order_id"),
             "Cancel by id. Returns True if the order existed and was removed.")
        .def("best_bid",
             [](const lobster::MatchingEngine& eng) -> std::optional<lobster::Price> {
                 if (!eng.book().has_best_bid()) return std::nullopt;
                 return eng.book().best_bid();
             })
        .def("best_ask",
             [](const lobster::MatchingEngine& eng) -> std::optional<lobster::Price> {
                 if (!eng.book().has_best_ask()) return std::nullopt;
                 return eng.book().best_ask();
             })
        .def("bid_levels", &bid_levels, py::arg("max_levels") = 10,
             "Top-of-book bids as a list of (price, total_quantity), best first.")
        .def("ask_levels", &ask_levels, py::arg("max_levels") = 10,
             "Top-of-book asks as a list of (price, total_quantity), best first.")
        .def("open_orders", [](const lobster::MatchingEngine& eng) {
            return eng.book().open_orders();
        });

    m.attr("__version__") = "0.1.0";
}
