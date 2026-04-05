# Low Latency C++ Portfolio

A collection of low-latency systems components written in modern C++, targeting the performance constraints of high-frequency trading and other latency-critical applications. Each component is self-contained with its own build system and documentation.

## Components

| Component | Description | Key Technique |
|---|---|---|
| [Memory Pool](./memory-pool/) | O(1) fixed-size block allocator | Union-based intrusive free list, zero auxiliary metadata |
| [Order Book](./order-book/) | Limit order book and matching engine | Dual heap with position tracking, O(log n) cancel via O(1) heap map lookup |

## Design Philosophy

Every component in this portfolio targets the same constraint: **zero dynamic allocation on the critical path**. `malloc` and `new` involve OS-level locking, non-deterministic latency, and heap fragmentation. These are all unacceptable when individual microseconds determine whether an order reaches the exchange first.

The memory pool eliminates heap allocation entirely through pre-allocation at startup. The order book pre-allocates its heaps at construction and reuses order slots in-place. Future components will follow the same principle.

---

## Memory Pool

> `memory-pool/` -> Header-only, fully templated fixed-size allocator

A `MemoryPool<T, N>` pre-allocates N blocks of type T at construction, organized as a LIFO free list using a `union` to overlay the list pointer directly onto unallocated block memory. This ensures zero bytes of auxiliary metadata per block as the free list structure lives inside the memory it manages.

**Highlights:**
- O(1) deterministic `allocate()` and `deallocate()` (single pointer swap, no OS involvement)
- `alignas(T)` enforces correct CPU memory alignment for non-trivially-aligned types, preventing misaligned-access faults
- LIFO ordering maximizes cache locality as recently freed blocks are the first to be reallocated
- Header-only: fully templated so the compiler bakes exact object dimensions into the binary, enabling aggressive inlining

[Full documentation →](./memory-pool/README.md)

---

## Order Book

> `order-book/` -> Limit order book with heap-based matching engine, benchmarked on Apple M4

A price-time priority matching engine supporting buy/sell market and limit orders, crossing limit order execution, partial fills, book walking, and O(log n) targeted order cancellation by ID.

**Benchmarks (Apple M4, Google Benchmark 1.9.5, 100 resting orders):**

| Optimization Stage | Mean Latency | CPU Time |
|---|---|---|
| Baseline (strings + doubles) | 743 ns | 737 ns |
| Enum action/type fields | ~615 ns | ~613 ns |
| Integer cent prices | **514 ns** | **511 ns** |

31% total latency reduction. Variance tightened from ±40ns to ±3ns after integer price migration.

**Highlights:**
- Dual heap design: min-heap for the ask side, max-heap for the bid side (root is always the best available price in O(1))
- Heap position maps (`sellHeapMap`, `buyHeapMap`) track every order's current heap index by ID, enabling O(1) lookup and O(log n) cancellation without scanning through the entire heap naively
- Crossing limit orders execute immediately against resting liquidity before inserting any remainder
- Prices stored as integer cents internally (eliminates floating point arithmetic and IEEE 754 non-determinism)
- Pre-allocated heaps at construction (no dynamic allocation during order processing)

[Full documentation →](./order-book/README.md)

---

## Build

Each component builds independently:

```bash
# Memory Pool (header-only — include directly)
#include "memory-pool/MemoryPool.hpp"

# Order Book
cd order-book
make program        # Interactive program
make benchmark      # Benchmark runner
./program           # Run interactive version
./benchmark_runner  # View benchmarked results
```

---

## Roadmap

- **SPSC Lock-Free Queue**: single-producer single-consumer queue using acquire/release atomics, no mutex on the critical path
- **Two-thread architecture**: dedicated I/O thread feeding the matching engine via SPSC queue, zero blocking on the matching path
- **Order book benchmark expansion**: 1,000 and 10,000 order depth to empirically demonstrate O(log n) scaling
- **Market data feed parser**: ITCH 5.0 binary protocol parser reconstructing a live order book from raw exchange data