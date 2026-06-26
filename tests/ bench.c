/* bench.c — benchmark 3 strategies + measure utilization */
#include <stdio.h>
#include <time.h>
#include "../src/mm.h"
void run_bench(const char *name) {
    mm_init();
    clock_t t0 = clock();
    /* alloc + free loop */
    for (int i = 0; i < 50000; i++) {
        void *p = mm_malloc(64);
        mm_free(p);
    }
    double ms = (double)(clock()-t0)/CLOCKS_PER_SEC*1000;
    printf("%s: %.2fms  util=%.1f%%\n",
        name, ms, mm_utilization()*100);
}