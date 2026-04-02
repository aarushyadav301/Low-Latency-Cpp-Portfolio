# Limit Order Book

A low-latency limit order book and matching engine written in modern C++. This project models the core data structure at the heart of every financial exchange: a real-time, price-time prioritized record of all resting limit orders and executing trades as new orders arrive.

## Architecture

The engine processes four order types through a unified `processOrder()` interface:

| Order Type | Behavior |
|---|---|
| Buy Market | Walks the ask heap, consuming the cheapest available sell limits until filled |
| Sell Market | Walks the bid heap, consuming the most expensive available buy limits until filled |
| Buy Limit | Inserts into the max-heap bid side at the correct price level |
| Sell Limit | Inserts into the min-heap ask side at the correct price level |
| Cancel | O(1) lookup via heap position map, followed by targeted heap removal |

## Data Structures

**Dual Heap Design**

- `sellLimitHeap` — min-heap sorted by price ascending. The root is always the best ask (cheapest sell).
- `buyLimitHeap` — max-heap sorted by price descending. The root is always the best bid (most expensive buy).
- Both heaps are pre-allocated at 200 slots to eliminate dynamic memory allocation on the critical path.

**Heap Position Maps**

- `sellHeapMap` and `buyHeapMap` are index arrays mapping each order's ID directly to its current position in the heap.
- These are updated on every swap during insertion and removal, enabling O(1) lookup of any order's heap location by ID.
- This is what makes targeted cancellation efficient — without the map, finding an arbitrary order in a heap is O(n).

**Order Tracking**

- `buyLimits` — an unordered_set of IDs tracking which resting orders are on the bid side, used by `cancelOrder` to route to the correct heap without storing redundant side information on the order itself.
- `processedOrders` — tracks fully filled orders so cancel requests on completed orders fail fast.

## Key Operations

**Book Walking (Market Orders)**

Market orders consume the book greedily from the inside out. On each iteration the engine takes the minimum available from the opposite side, accumulates filled shares and cost, and removes the consumed order. If a resting limit order is only partially consumed, the remainder is reinserted at the same price. This continues until the market order is fully filled or the book is exhausted.

**Targeted Removal (Cancel / Partial Fill)**

`removeAsk(heapLoc)` and `removeBid(heapLoc)` accept an arbitrary heap position rather than always removing the root. The node at `heapLoc` is replaced with the last element in the heap, the size is decremented, and a sift-down restores the heap property. Combined with the heap position maps, this gives O(log n) cancellation with O(1) lookup.

## Complexity

| Operation | Complexity |
|---|---|
| Insert limit order | O(log n) |
| Execute market order (single level) | O(log n) |
| Get best bid / ask | O(1) |
| Cancel by order ID | O(log n) with O(1) lookup |

## Build

```bash
cd order-book
make program
./program
```

**Input format:**
```
BUY LIMIT 100 $142.50
SELL LIMIT 200 $143.00
BUY MARKET 75 $0
CANCEL 1
q  # Quit the program when you're done
```

## What's In Progress

- Crossing limit order matching — incoming limit orders that cross the spread should execute immediately against resting orders before inserting any remainder
- Price representation — migrating from `double` to `uint64` integer cents to eliminate floating point non-determinism
- Last traded price tracking
- Two-thread architecture: dedicated I/O thread feeding a lock-free SPSC queue consumed by the matching engine