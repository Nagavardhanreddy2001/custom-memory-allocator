#!/bin/bash
set -e
echo "=== Building ==="
make clean && make
echo "=== Stress test ==="
./memtest
echo "=== Valgrind ==="
make valgrind
echo "=== Benchmark ==="
./bench