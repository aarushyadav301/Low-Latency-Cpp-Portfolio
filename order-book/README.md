# Limit Order Book

A low-latency limit order book and matching engine written in modern C++. I built this project to model the core data structure at the heart of every financial exchange: a real-time, price-time prioritized record of all resting limit orders that executes trades as new orders arrive.

## Benchmarks

Measured via Google Benchmark 1.9.5 on Apple M4. The Order Book is pre-populated with 100 resting limit orders using normally distributed prices (buy mean $10.00, sell mean $20.00, σ = $4.00). Each iteration resets the book state via `reset()` to ensure consistent measurement conditions.

| Optimization Stage | Mean Latency | CPU Time | Iterations |
|---|---|---|---|
| Baseline (strings + doubles) | 743 ns | 737 ns | 985,527 |
| Enum action/type fields | ~615 ns | ~613 ns | 1,186,823 |
| Integer cent prices | **514 ns** | **511 ns** | 1,344,086 |

**31% total latency reduction** from baseline through targeted hot-path optimizations.

Variance tightened from ±40ns to ±3ns after migrating to integer arithmetic. Predictability matters just as much as speed in low-latency systems. 

---

## Architecture

The engine processes five order types through a unified `processOrder()` interface:

| Order Type | Behavior |
|---|---|
| Buy Market | Walks the ask heap, consuming cheapest sell limits until filled |
| Sell Market | Walks the bid heap, consuming most expensive buy limits until filled |
| Buy Limit | Checks for crossing asks before inserting into max-heap bid side |
| Sell Limit | Checks for crossing bids before inserting into min-heap ask side |
| Cancel | O(1) lookup via heap position map, followed by O(log n) targeted removal |

---

## Data Structures

**Dual Heap Design**

- `sellLimitHeap` -> min-heap sorted by price ascending. Root is always the best ask (cheapest sell).
- `buyLimitHeap` -> max-heap sorted by price descending. Root is always the best bid (most expensive buy).
- Both heaps are pre-allocated at construction to eliminate dynamic memory allocation on the critical path.

**Heap Position Maps**

- `sellHeapMap` and `buyHeapMap` are index arrays mapping each order ID directly to its current heap position.
- Updated on every swap during insertion, removal, and heapify — maintaining O(1) lookup at all times.
- Enables targeted cancellation of any resting order in O(log n) without scanning the heap.

**Order Tracking**

- `buyOrders` -> unordered_set of IDs tracking which resting orders are on the bid side, used by `cancelOrder` to route to the correct heap without storing redundant side information on the struct itself.
- `processedOrders` -> tracks fully filled orders so cancel requests on completed orders fail fast with O(1) detection.

---

## Key Operations

**Crossing Limit Orders**

Incoming limit orders check for immediate execution before resting. A buy limit priced at or above the best ask walks the ask heap consuming resting sell limits — exactly like a market order but stopping when the ask price exceeds the buy limit price. Any unfilled remainder is inserted into the bid heap. Same logic applies symmetrically for sell limits crossing the bid side.

**Book Walking (Market Orders)**

Market orders consume the book greedily from the inside out. On each iteration the engine takes the best available price from the opposite side, accumulates filled shares and cost, and removes the consumed order. If a resting limit is only partially consumed, the remainder is reinserted at the same price. Continues until fully filled or book is exhausted. Market orders follow Immediate-or-Cancelled (IOC) guidelines (unfilled remainder is discarded).

**Targeted Removal (Cancel)**

`removeAsk(heapLoc)` and `removeBid(heapLoc)` accept an arbitrary heap position. The node is replaced with the last heap element, size is decremented, heap maps are updated for both affected orders, and a sift-down restores the heap property. This, combined with the position maps, gives O(log n) cancellation with O(1) lookup.

---

## Complexity

| Operation | Complexity |
|---|---|
| Insert limit order | O(log n) |
| Execute market order (per price level) | O(log n) |
| Get best bid / ask | O(1) |
| Cancel by order ID | O(log n) with O(1) lookup |
| Detect already-filled cancel | O(1) |

---

## Hot Path Optimizations

**Enum action and type fields** -> Replaced all std::string action and type fields with strongly typed enums. This eliminates heap allocation and character-by-character string comparison on every order processed. Result: ~13% latency reduction (experimentally-determined)

**Integer cent prices** -> Prices stored internally as int cents (e.g. $142.50 is stored as 14250). This eliminates floating point arithmetic on the hot path. Integer arithmetic executes in a single cycle versus 3–5 cycles for floating point. The input parser handles dollar-to-cent conversion transparently so the user interface remains in dollars. Result: ~21% additional latency reduction, variance tightened from ±40ns to ±3ns (experimentally-determined). 

**Pre-allocated heap storage** -> Both heaps and position maps are sized at construction. No dynamic memory allocation occurs during order processing.

---

## Build

```bash
cd order-book
make program        # builds the interactive program
make benchmark      # builds the benchmark runner
./program           # run interactively
./benchmark_runner >> benchmarks.txt  # run benchmarks and append output to some txt file (optional)
```

**Input format:**
```
BUY LIMIT 100 $142.50     # buy 100 shares, limit $142.50
SELL LIMIT 200 $143.00    # sell 200 shares, limit $143.00
BUY MARKET 75             # buy 75 shares at market
SELL MARKET 50            # sell 50 shares at market
CANCEL 1                  # cancel order ID 1
q                         # quit
```

Prices are entered in dollars and converted to integer cents internally.

---

## What's In Progress

- Two-thread architecture: dedicated I/O thread feeding a lock-free SPSC queue consumed by the matching engine
- Benchmark suite expansion: 1,000 and 10,000 order book depth to empirically demonstrate O(log n) scaling
- Throughput benchmark: sustained orders/second under continuous load