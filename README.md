# Low Latency C++ Portfolio

A collection of low-latency systems components written in modern C++, targeting the performance constraints of high-frequency trading and other latency-critical applications. Each component is self-contained with its own build system and documentation.

## Components

| Component | Description | Key Technique |
|---|---|---|
| [Memory Pool](./memory-pool/) | O(1) fixed-size block allocator | Union-based intrusive free list, zero auxiliary metadata |
| [Order Book](./order-book/) | Limit order book and matching engine | Dual heap with position tracking, O(1) best bid/ask |

## Design Philosophy

Every component in this portfolio targets the same constraint: **zero dynamic allocation on the critical path**. `malloc` and `new` involve OS-level locking, non-deterministic latency, and heap fragmentation. These are all unacceptable when individual microseconds determine whether an order reaches the exchange first.

The memory pool eliminates heap allocation entirely through pre-allocation at startup. The order book pre-allocates its heaps at construction and reuses order slots in-place. Future components will follow the same principle.

---

## Memory Pool

> `memory-pool/`: Header-only, fully templated fixed-size allocator

A `MemoryPool<T, N>` pre-allocates N blocks of type T at construction, organized as a LIFO free list using a `union` to overlay the list pointer directly onto unallocated block memory. This means zero bytes of auxiliary metadata per block (the free list structure lives inside the memory it manages).

**Highlights:**
- O(1) deterministic `allocate()` and `deallocate()`: single pointer swap, no OS involvement
- `alignas(T)` enforces correct CPU memory alignment for non-trivially-aligned types, preventing misaligned-access faults
- LIFO ordering maximizes cache locality (recently freed blocks are the first to be reallocated)
- Header-only: fully templated so the compiler bakes exact object dimensions into the binary, enabling aggressive inlining

[Full documentation →](./memory-pool/README.md)

---

## Order Book

> `order-book/`: Limit order book with heap-based matching engine

A price-time priority matching engine supporting buy/sell market and limit orders, partial fills, book walking, and O(1) targeted order cancellation by ID.

**Highlights:**
- Dual heap design: min-heap for the ask side, max-heap for the bid side (root is always the best available price)
- Heap position maps (`sellHeapMap`, `buyHeapMap`) track every order's current heap index by ID, enabling O(1) lookup for cancellation without scanning
- Market orders walk the book greedily, consuming price levels and reinserting partial remainders
- Pre-allocated heaps at construction (no dynamic allocation during order processing)

**In progress:** crossing limit order matching, integer cent price representation, SPSC queue + dedicated I/O thread

[Full documentation →](./order-book/README.md)

---

## Build

Each component builds independently:

```bash
# Memory Pool (header-only, no build required — include directly)
#include "memory-pool/MemoryPool.hpp"

# Order Book
cd order-book
make
./program
```

## Future Work

- **SPSC Lock-Free Queue**: single-producer single-consumer queue using acquire/release atomics, no mutex on the critical path
- **Two-thread architecture**: dedicated I/O thread feeding the matching engine via SPSC queue, zero blocking on the matching path
- **Benchmark suite**: latency measurements for allocation, order insertion, and market order execution using `std::chrono`
- **Market data feed parser**: ITCH 5.0 binary protocol parser reconstructing a live order book from raw exchange data
