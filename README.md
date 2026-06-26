# Custom Memory Allocator

A dynamic memory allocator built from scratch in C, replicating `malloc`, `free`, and `realloc` using `sbrk()` system calls without using any standard library heap functions.

## Features

- Three placement strategies: first-fit, best-fit, and segregated free lists
- Boundary tag coalescing: O(1) merging of adjacent free blocks in all 4 cases
- Block splitting to avoid internal fragmentation
- Stress tested: 30,000 ops with zero errors
- Valgrind verified: 0 memory errors, 0 leaks

## Benchmark Results

| Workload         | Time     | Peak Utilization |
|------------------|----------|------------------|
| Small (1-64B)    |  4.49ms  | 80.8%            |
| Medium (64-512B) | 11.57ms  | 89.9%            |
| Large (512-4KB)  | 16.29ms  | 87.8%            |
| Mixed (1-4KB)    | 14.98ms  | 89.3%            |

## Test Results

- Stress test PASSED: 30,000 ops in 1.216s
- Utilization after alloc: 100.0%
- Utilization after 50% free: 50.3%
- Valgrind: 0 errors, 0 leaks

## Build and Run

```bash
make          # build everything
./memtest     # stress test
make valgrind # leak check
./bench       # benchmark
make clean    # remove binaries
```

## Key Implementation Details

- sbrk() used to grow heap, no malloc/calloc anywhere
- Minimum block size: 16 bytes
- Alignment: 8-byte aligned payloads
- Coalescing: immediate on every free() call
- Splitting: only when remainder is at least 16 bytes
