/* mm_seg.c — segregated free list allocator */
#define NUM_CLASSES 12
static void *seg_list[NUM_CLASSES];
/* size class buckets: 8, 16, 32, 64 ... 4096+ */
static int get_class(size_t size) {
    int i = 0; size_t sz = 8;
    while (i < NUM_CLASSES - 1 && size > sz)
        { sz <<= 1; i++; }
    return i;
}
#define SEG_PREV(bp) (*(void **)(bp))
#define SEG_NEXT(bp) (*((void **)(bp) + 1))
static void seg_insert(void *bp) {
    int cls = get_class(GET_SIZE(HDRP(bp)));   
    SEG_NEXT(bp) = seg_list[cls];
    seg_list[cls] = bp;
}