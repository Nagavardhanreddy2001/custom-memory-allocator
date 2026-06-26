#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../src/mm.h"

#ifndef N
#define N 10000
#endif
#ifndef MAX_SIZE
#define MAX_SIZE 4096
#endif

int main(void) {
    mm_init();
    void  *ptrs[N];
    size_t sizes[N];
    srand(42);

    clock_t t0 = clock();

    for (int i = 0; i < N; i++) {
        sizes[i] = (rand() % MAX_SIZE) + 1;
        ptrs[i]  = mm_malloc(sizes[i]);
        if (!ptrs[i]) { fprintf(stderr, "malloc failed at %d\n", i); return 1; }
        __builtin_memset(ptrs[i], 0xAB, sizes[i]);
    }

    printf("Utilization after alloc:    %.1f%%\n", mm_utilization() * 100);

    for (int i = 0; i < N; i += 2) { mm_free(ptrs[i]); ptrs[i] = NULL; }

    printf("Utilization after 50%% free: %.1f%%\n", mm_utilization() * 100);

    for (int i = 1; i < N; i += 2) {
        size_t ns = (rand() % MAX_SIZE) + 1;
        ptrs[i] = mm_realloc(ptrs[i], ns);
        if (!ptrs[i]) { fprintf(stderr, "realloc failed at %d\n", i); return 1; }
    }
    for (int i = 0; i < N; i++) if (ptrs[i]) mm_free(ptrs[i]);

    double elapsed = (double)(clock() - t0) / CLOCKS_PER_SEC;
    printf("Stress test PASSED — %d ops in %.3fs\n", N * 3, elapsed);
    return 0;
}
