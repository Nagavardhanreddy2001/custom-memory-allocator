# Custom Memory Allocator

A dynamic memory allocator built from scratch in C, replicating
malloc, free, and realloc using sbrk() system calls — no standard
library heap functions used anywhere.

## Three Strategies Implemented

| Strategy            | Mechanism                        | Strength        |
|---------------------|----------------------------------|-----------------|
| First-fit           | Linear scan, first free block    | Simple baseline |
| Best-fit            | Linear scan, smallest fit        | Best utilization|
| Segregated free lists | 12 size-class doubly-linked lists | Best speed     |

## Benchmark Results (5,000 ops, 500 live slots)

### Speed comparison
| Workload        | First-fit | Best-fit | Seg-lists | Speedup vs first-fit |
|-----------------|-----------|----------|-----------|----------------------|
| Small (1-64B)   |  3.14ms   |  5.80ms  |  0.39ms   | 8x faster            |
| Medium (64-512B)|  6.07ms   | 13.14ms  |  0.86ms   | 7x faster            |
| Large (512-4KB) | 11.10ms   | 18.89ms  |  2.70ms   | 4x faster            |
| Mixed (1-4KB)   | 10.30ms   | 20.35ms  |  2.71ms   | 4x faster            |

### Utilization comparison
| Workload        | First-fit | Best-fit | Delta     |
|-----------------|-----------|----------|-----------|
| Medium (64-512B)|   83.3%   |   87.5%  | +4.2%     |
| Large (512-4KB) |   83.7%   |   87.5%  | +3.7%     |
| Mixed (1-4KB)   |   84.9%   |   89.0%  | +4.1%     |

### Key insight
Segregated free lists are 4-8x faster than both scan-based strategies
because find() searches only one size class instead of the full heap.
Best-fit wins on utilization (+4%) by always choosing the tightest fit,
at the cost of 2x slower allocation time.
This is the same tradeoff made by production allocators like jemalloc.

## Bugs Fixed

1. mm_free dropped the coalesce return value
2. mm_utilization counted header and footer bytes as payload
3. mm_init did not reset heap on repeated calls (broke benchmarking)

## Validation

- Stress test: 30,000 ops (alloc + free + realloc), PASSED
- Valgrind: 0 errors, 0 leaks
- AddressSanitizer: clean

## Build and Run

```bash
make              # build everything
./memtest         # stress test (30,000 ops)
make valgrind     # Valgrind leak check
./bench           # three-way strategy benchmark
make asan         # AddressSanitizer check
make clean        # remove binaries
```

## Implementation Details

- Heap grown with sbrk() only — zero use of malloc/calloc
- Block layout: header (4B) + payload + footer (4B), 8-byte aligned
- Minimum block: 24 bytes (holds fwd+bwd pointers while free)
- Coalescing: immediate on every free(), all 4 adjacency cases
- Splitting: only when remainder >= minimum block size
- Seg-list classes: 8B, 16B, 32B, 64B ... up to 8KB, then 8KB+
