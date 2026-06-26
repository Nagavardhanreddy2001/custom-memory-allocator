#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "mm.h"

#define WSIZE      4
#define DSIZE      8
#define CHUNKSIZE  (1 << 12)
#define ALIGNMENT  8

#define MAX(x, y)   ((x) > (y) ? (x) : (y))
#define PACK(size, alloc)  ((size) | (alloc))
#define GET(p)             (*(unsigned int *)(p))
#define PUT(p, val)        (*(unsigned int *)(p) = (val))
#define GET_SIZE(p)        (GET(p) & ~0x7)
#define GET_ALLOC(p)       (GET(p) & 0x1)
#define HDRP(bp)           ((char *)(bp) - WSIZE)
#define FTRP(bp)           ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp)      ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)      ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

static char *heap_listp = NULL;

static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size       = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        return bp;
    } else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp),             PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

static void *extend_heap(size_t words) {
    char  *bp;
    size_t size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((bp = sbrk(size)) == (void *)-1) return NULL;
    PUT(HDRP(bp),           PACK(size, 0));
    PUT(FTRP(bp),           PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return coalesce(bp);
}

void mm_init(void) {
    if ((heap_listp = sbrk(4 * WSIZE)) == (void *)-1) return;
    PUT(heap_listp,             0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);
    extend_heap(CHUNKSIZE / WSIZE);
}

static void *find_fit_best(size_t asize) {
    void  *bp, *best = NULL;
    size_t best_size = (size_t)-1;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        size_t sz = GET_SIZE(HDRP(bp));
        if (!GET_ALLOC(HDRP(bp)) && sz >= asize && sz < best_size) {
            best = bp; best_size = sz;
        }
    }
    return best;
}

static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));
    if ((csize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

void *mm_malloc(size_t size) {
    if (size == 0) return NULL;
    size_t asize = (size <= DSIZE) ? 2 * DSIZE
                                   : DSIZE * ((size + DSIZE + DSIZE - 1) / DSIZE);
    void *bp;
    if ((bp = find_fit_best(asize)) != NULL) { place(bp, asize); return bp; }
    size_t extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL) return NULL;
    place(bp, asize);
    return bp;
}

void mm_free(void *ptr) {
    if (!ptr) return;
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

void *mm_realloc(void *ptr, size_t size) {
    if (!ptr)    return mm_malloc(size);
    if (!size)   { mm_free(ptr); return NULL; }
    void  *newptr  = mm_malloc(size);
    if (!newptr) return NULL;
    size_t copysize = GET_SIZE(HDRP(ptr)) - DSIZE;
    if (size < copysize) copysize = size;
    __builtin_memcpy(newptr, ptr, copysize);
    mm_free(ptr);
    return newptr;
}

double mm_utilization(void) {
    size_t payload = 0, total = 0;
    void *bp;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        total += GET_SIZE(HDRP(bp));
        if (GET_ALLOC(HDRP(bp))) payload += GET_SIZE(HDRP(bp));
    }
    return total ? (double)payload / total : 0.0;
}
