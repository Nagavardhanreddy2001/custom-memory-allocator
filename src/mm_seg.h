#ifndef MM_SEG_H
#define MM_SEG_H

#include <stddef.h>

void   seg_mm_init(void);
void  *seg_mm_malloc(size_t size);
void   seg_mm_free(void *ptr);
void  *seg_mm_realloc(void *ptr, size_t size);
double seg_mm_utilization(void);

#endif
