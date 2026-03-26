# Low-Latency O(1) Memory Pool

A high-performance, template-based memory pool allocator written in modern C++. Designed for high-frequency trading (HFT) and latency-critical systems where heap allocation jitter (`std::malloc` / `new`) is unacceptable.

## Technical Overview
- **Zero Dynamic Allocation:** All memory is pre-allocated up front. Once the critical path begins, `allocate()` and `deallocate()` never ask the OS for memory.
- **O(1) Free List:** Maintains a strict LIFO (Last-In, First-Out) free list, ensuring that finding the next available block requires only a single pointer swap. LIFO ordering also maximizes CPU cache-locality (recently freed blocks are re-allocated first).
- **Zero-Overhead Bookkeeping:** Utilizes a C++ `union` to overlay the free-list pointer directly onto the unallocated user memory space. This means the pool requires 0 bytes of extra memory overhead per block while in use.
- **Header-Only:** Fully templated `template <typename T, size_t blockNum>` to allow the compiler to bake the exact object dimensions into the binary, enabling aggressive function inlining.