# Custom Memory Allocator

A dynamic memory allocator built from scratch in C, replicating
malloc, free, and realloc using sbrk() system calls — no standard
library heap functions used anywhere.

## Three Strategies Implemented

| Strategy             | Mechanism                         | Strength         |
|----------------------|-----------------------------------|------------------|
| First-fit            | Linear scan, first free block     | Simple baseline  |
| Best-fit             | Linear scan, smallest fit         | Best utilization |
| Segregated free lists| 12 size-class doubly-linked lists | Best speed       |

## Benchmark Results (5,000 ops, 500 live slots)

### Speed
| Workload         | First-fit | Best-fit | Seg-lists | Speedup        |
|------------------|-----------|----------|-----------|----------------|
| Small (1-64B)    |  3.14ms   |  5.80ms  |  0.39ms   | 8x vs first-fit|
| Medium (64-512B) |  6.07ms   | 13.14ms  |  0.86ms   | 7x vs first-fit|
| Large (512-4KB)  | 11.10ms   | 18.89ms  |  2.70ms   | 4x vs first-fit|
| Mixed (1-4KB)    | 10.30ms   | 20.35ms  |  2.71ms   | 4x vs first-fit|

### Utilization
| Workload         | First-fit | Best-fit | Delta |
|------------------|-----------|----------|-------|
| Medium (64-512B) |   83.3%   |  87.5%   | +4.2% |
| Large (512-4KB)  |   83.7%   |  87.5%   | +3.7% |
| Mixed (1-4KB)    |   84.9%   |  89.0%   | +4.1% |

### Key insight
Segregated free lists are 4-8x faster because find() searches only
one size class instead of scanning the full heap. Best-fit wins on
utilization (+4%) by always choosing the tightest fit, at the cost
of 2x slower allocation. This is the same tradeoff in jemalloc/tcmalloc.

## Project Structure
custom-memory-allocator/

├── src/

│   ├── mm.c        implicit list — first-fit and best-fit

│   ├── mm.h

│   ├── mm_seg.c    segregated free lists (12 size classes)

│   ├── mm_seg.h

│   ├── utils.c

│   └── utils.h

├── tests/

│   ├── memtest.c   stress test — 30,000 randomized ops

│   └── bench.c     three-way strategy benchmark

├── scripts/

│   ├── run_all.sh

│   └── benchmark.sh

├── .vscode/

└── Makefile
## Design Decisions

- fwd/bwd pointers stored as void* (8B on x86-64) in free block payload
- Minimum block size: 24 bytes (2*DSIZE + 2*WSIZE = header + fwd + bwd + footer)
- Utilization metric excludes header+footer overhead (2*WSIZE per block)
- mm_init and seg_mm_init reset all free lists on each call for clean benchmark runs
- Coalescing is immediate on every free() — all 4 adjacency cases handled
- Block splitting only when remainder >= MIN_BLOCK (avoids unusable fragments)
- 12 size classes: 8B, 16B, 32B, 64B, 128B, 256B, 512B, 1K, 2K, 4K, 8K, 8K+

## Bugs Fixed

1. mm_free dropped the coalesce return value
2. mm_utilization counted header and footer bytes as payload (fixed: subtracts 2*WSIZE per block)
3. mm_init did not reset heap on repeated calls (broke clean benchmark comparisons)

## Validation

- Stress test: 30,000 ops (alloc + free + realloc) — PASSED
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
