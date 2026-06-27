/*
 * mm_seg.c — segregated free list allocator
 *
 * Replaces the implicit free-list scan with an array of explicit
 * doubly-linked free lists, one per size class:
 *
 *   class 0 :   1 –   8 bytes
 *   class 1 :   9 –  16 bytes
 *   class 2 :  17 –  32 bytes
 *   class 3 :  33 –  64 bytes
 *   class 4 :  65 – 128 bytes
 *   class 5 : 129 – 256 bytes
 *   class 6 : 257 – 512 bytes
 *   class 7 : 513 –  1K bytes
 *   class 8 :  1K –  2K bytes
 *   class 9 :  2K –  4K bytes
 *   class10 :  4K –  8K bytes
 *   class11 :  8K+  bytes
 *
 * Each free block stores fwd/bwd pointers in its payload area
 * (safe because payload is unused while block is free).
 *
 * insert/remove: O(1)
 * find:          O(k) where k = blocks in size class  (much smaller than n)
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "mm_seg.h"

/* ── block layout macros (same as mm.c) ─────── */
#define WSIZE      4
#define DSIZE      8
#define CHUNKSIZE  (1 << 12)
#define ALIGNMENT  8

#define MAX(x,y)          ((x)>(y)?(x):(y))
#define PACK(size,alloc)  ((size)|(alloc))
#define GET(p)            (*(unsigned int *)(p))
#define PUT(p,val)        (*(unsigned int *)(p)=(val))
#define GET_SIZE(p)       (GET(p) & ~0x7)
#define GET_ALLOC(p)      (GET(p) &  0x1)
#define HDRP(bp)          ((char *)(bp) - WSIZE)
#define FTRP(bp)          ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp)     ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)     ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

/* ── explicit list pointers live in payload ──── */
#define FWD(bp)   (*(void **)(bp))
#define BWD(bp)   (*((void **)(bp) + 1))

/* minimum block: hdr + fwd + bwd + ftr = 4+8+8+4 = 24 bytes */
#define MIN_BLOCK  (2 * DSIZE + 2 * WSIZE)

/* ── segregated list array ───────────────────── */
#define NUM_CLASSES 12
static void *seg_list[NUM_CLASSES];
static char *heap_listp = NULL;
static char *heap_start = NULL;

/* map a block size to its size class index */
static int get_class(size_t size) {
    int i = 0;
    size_t bound = 8;
    while (i < NUM_CLASSES - 1 && size > bound) {
        bound <<= 1;
        i++;
    }
    return i;
}

/* ── list operations ─────────────────────────── */

static void seg_insert(void *bp) {
    int cls = get_class(GET_SIZE(HDRP(bp)));
    void *head = seg_list[cls];

    FWD(bp) = head;
    BWD(bp) = NULL;
    if (head) BWD(head) = bp;
    seg_list[cls] = bp;
}

static void seg_remove(void *bp) {
    int   cls  = get_class(GET_SIZE(HDRP(bp)));
    void *fwd  = FWD(bp);
    void *bwd  = BWD(bp);

    if (bwd) FWD(bwd) = fwd;
    else     seg_list[cls] = fwd;   /* bp was the head */

    if (fwd) BWD(fwd) = bwd;
}

/* first-fit within each class, then try larger classes */
static void *seg_find(size_t asize) {
    for (int cls = get_class(asize); cls < NUM_CLASSES; cls++) {
        for (void *bp = seg_list[cls]; bp; bp = FWD(bp)) {
            if (GET_SIZE(HDRP(bp)) >= asize)
                return bp;
        }
    }
    return NULL;
}

/* ── coalesce (updates seg list) ─────────────── */
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size       = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        seg_insert(bp);
        return bp;
    }
    if (prev_alloc && !next_alloc) {
        seg_remove(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        seg_remove(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp),             PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {
        seg_remove(NEXT_BLKP(bp));
        seg_remove(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)))
              + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    seg_insert(bp);
    return bp;
}

static void *extend_heap(size_t words) {
    size_t size = (words % 2) ? (words+1)*WSIZE : words*WSIZE;
    char *bp = sbrk(size);
    if (bp == (void *)-1) return NULL;
    PUT(HDRP(bp),            PACK(size, 0));
    PUT(FTRP(bp),            PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return coalesce(bp);
}

/* ── public API ──────────────────────────────── */

void seg_mm_init(void) {
    if (heap_start && brk(heap_start) == -1) return;
    for (int i = 0; i < NUM_CLASSES; i++) seg_list[i] = NULL;

    char *init = sbrk(4 * WSIZE);
    if (init == (void *)-1) return;
    if (!heap_start) heap_start = init;

    PUT(init,             0);
    PUT(init + WSIZE,     PACK(DSIZE, 1));
    PUT(init + 2*WSIZE,   PACK(DSIZE, 1));
    PUT(init + 3*WSIZE,   PACK(0, 1));
    heap_listp = init + 2*WSIZE;
    extend_heap(CHUNKSIZE / WSIZE);
}

void *seg_mm_malloc(size_t size) {
    if (!size) return NULL;
    /* minimum block must hold fwd+bwd pointers (2 pointers = 16B on 64-bit) */
    size_t asize = MAX(MIN_BLOCK,
                       DSIZE * ((size + DSIZE + DSIZE - 1) / DSIZE));
    void *bp = seg_find(asize);
    if (bp) {
        seg_remove(bp);
        /* split if remainder fits a minimum block */
        size_t csize = GET_SIZE(HDRP(bp));
        if (csize - asize >= MIN_BLOCK) {
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            void *next = NEXT_BLKP(bp);
            PUT(HDRP(next), PACK(csize - asize, 0));
            PUT(FTRP(next), PACK(csize - asize, 0));
            seg_insert(next);
        } else {
            PUT(HDRP(bp), PACK(csize, 1));
            PUT(FTRP(bp), PACK(csize, 1));
        }
        return bp;
    }
    size_t extsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extsize / WSIZE)) == NULL) return NULL;
    seg_remove(bp);
    size_t csize = GET_SIZE(HDRP(bp));
    if (csize - asize >= MIN_BLOCK) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        void *next = NEXT_BLKP(bp);
        PUT(HDRP(next), PACK(csize - asize, 0));
        PUT(FTRP(next), PACK(csize - asize, 0));
        seg_insert(next);
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
    return bp;
}

void seg_mm_free(void *ptr) {
    if (!ptr) return;
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

void *seg_mm_realloc(void *ptr, size_t size) {
    if (!ptr)  return seg_mm_malloc(size);
    if (!size) { seg_mm_free(ptr); return NULL; }
    void  *newptr   = seg_mm_malloc(size);
    if (!newptr) return NULL;
    size_t copysize = GET_SIZE(HDRP(ptr)) - DSIZE;
    if (size < copysize) copysize = size;
    __builtin_memcpy(newptr, ptr, copysize);
    seg_mm_free(ptr);
    return newptr;
}

double seg_mm_utilization(void) {
    size_t payload = 0, total = 0;
    void *bp;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        size_t sz = GET_SIZE(HDRP(bp));
        total += sz;
        if (GET_ALLOC(HDRP(bp))) payload += sz - 2 * WSIZE;
    }
    return total ? (double)payload / total : 0.0;
}
