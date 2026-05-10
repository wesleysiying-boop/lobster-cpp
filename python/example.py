"""lobster_py demo — same scenario as examples/simple.cpp, in Python.

Build & install:
    pip install -e .

Run:
    python python/example.py
"""

from __future__ import annotations

import lobster_py as lob


def print_tob(eng: lob.MatchingEngine) -> None:
    bid = eng.best_bid()
    ask = eng.best_ask()
    bid_lvl = eng.bid_levels(1)
    ask_lvl = eng.ask_levels(1)
    bid_qty = bid_lvl[0][1] if bid_lvl else 0
    ask_qty = ask_lvl[0][1] if ask_lvl else 0
    bid_str = f"bid {bid} x {bid_qty}" if bid is not None else "bid -"
    ask_str = f"ask {ask} x {ask_qty}" if ask is not None else "ask -"
    print(f"  TOB: {bid_str}  |  {ask_str}")


def main() -> None:
    eng = lob.MatchingEngine()

    print("1. Two resting bids and one resting ask:")
    eng.add(lob.OrderRequest(id=1, side=lob.Side.Buy, price=4995, quantity=100))
    eng.add(lob.OrderRequest(id=2, side=lob.Side.Buy, price=5000, quantity=50))
    eng.add(lob.OrderRequest(id=3, side=lob.Side.Sell, price=5005, quantity=75))
    print_tob(eng)

    print("\n2. Aggressive sell @ 5000 for 30 — should cross into id=2:")
    fills = eng.add(lob.OrderRequest(id=4, side=lob.Side.Sell, price=5000, quantity=30))
    for f in fills:
        print(f"  {f}")
    print_tob(eng)

    print("\n3. Cancel id=1, then market buy 200 — sweeps the ask side:")
    eng.cancel(1)
    sweep = eng.add(
        lob.OrderRequest(id=5, side=lob.Side.Buy, price=0, quantity=200, type=lob.OrderType.Market)
    )
    for f in sweep:
        print(f"  {f}")
    print_tob(eng)

    print(f"\nOpen orders remaining: {eng.open_orders()}")


if __name__ == "__main__":
    main()
