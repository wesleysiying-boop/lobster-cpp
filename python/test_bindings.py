"""End-to-end tests for lobster_py.

These verify that the Python bindings preserve the semantics of the C++
matching engine — same scenarios as in tests/test_engine.cpp, expressed
in pytest. Importantly, they make sure the conversion at the C++/Python
boundary doesn't drop or reorder fills.
"""

from __future__ import annotations

import lobster_py as lob


def _req(
    oid: int,
    side: lob.Side,
    price: int,
    qty: int,
    typ: lob.OrderType = lob.OrderType.Limit,
) -> lob.OrderRequest:
    return lob.OrderRequest(id=oid, side=side, price=price, quantity=qty, type=typ)


def test_resting_orders_do_not_fill() -> None:
    eng = lob.MatchingEngine()
    fills = eng.add(_req(1, lob.Side.Buy, 5000, 100))
    assert fills == []
    assert eng.best_bid() == 5000


def test_full_cross_fills_and_clears_book() -> None:
    eng = lob.MatchingEngine()
    eng.add(_req(1, lob.Side.Sell, 5000, 100))
    fills = eng.add(_req(2, lob.Side.Buy, 5000, 100))
    assert len(fills) == 1
    assert fills[0].aggressor_id == 2
    assert fills[0].resting_id == 1
    assert fills[0].price == 5000
    assert fills[0].quantity == 100
    assert eng.best_bid() is None
    assert eng.best_ask() is None


def test_partial_fill_leaves_remainder() -> None:
    eng = lob.MatchingEngine()
    eng.add(_req(1, lob.Side.Sell, 5000, 30))
    fills = eng.add(_req(2, lob.Side.Buy, 5000, 100))
    assert len(fills) == 1
    assert fills[0].quantity == 30
    assert eng.best_bid() == 5000
    assert eng.best_ask() is None


def test_fifo_priority_within_level() -> None:
    eng = lob.MatchingEngine()
    eng.add(_req(1, lob.Side.Sell, 5000, 50))
    eng.add(_req(2, lob.Side.Sell, 5000, 50))
    fills = eng.add(_req(3, lob.Side.Buy, 5000, 60))
    assert [(f.resting_id, f.quantity) for f in fills] == [(1, 50), (2, 10)]


def test_market_order_sweeps_levels() -> None:
    eng = lob.MatchingEngine()
    eng.add(_req(1, lob.Side.Sell, 5000, 30))
    eng.add(_req(2, lob.Side.Sell, 5005, 30))
    eng.add(_req(3, lob.Side.Sell, 5010, 30))
    fills = eng.add(_req(99, lob.Side.Buy, 0, 80, lob.OrderType.Market))
    assert [f.quantity for f in fills] == [30, 30, 20]
    assert eng.best_ask() == 5010


def test_ioc_drops_remainder() -> None:
    eng = lob.MatchingEngine()
    eng.add(_req(1, lob.Side.Sell, 5000, 20))
    fills = eng.add(_req(2, lob.Side.Buy, 5000, 100, lob.OrderType.Ioc))
    assert len(fills) == 1
    assert fills[0].quantity == 20
    assert eng.best_bid() is None
    assert eng.best_ask() is None


def test_cancel_returns_true_then_false() -> None:
    eng = lob.MatchingEngine()
    eng.add(_req(1, lob.Side.Buy, 5000, 100))
    assert eng.cancel(1) is True
    assert eng.cancel(1) is False
    assert eng.best_bid() is None


def test_book_inspection_returns_top_of_book_in_priority_order() -> None:
    eng = lob.MatchingEngine()
    eng.add(_req(1, lob.Side.Buy, 4990, 5))
    eng.add(_req(2, lob.Side.Buy, 5000, 5))
    eng.add(_req(3, lob.Side.Buy, 4995, 5))
    eng.add(_req(4, lob.Side.Sell, 5005, 5))
    eng.add(_req(5, lob.Side.Sell, 5015, 5))
    eng.add(_req(6, lob.Side.Sell, 5010, 5))

    bids = eng.bid_levels(10)
    asks = eng.ask_levels(10)
    assert [px for px, _ in bids] == [5000, 4995, 4990]   # descending
    assert [px for px, _ in asks] == [5005, 5010, 5015]  # ascending
