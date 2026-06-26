#ifndef MM_H
#define MM_H

#include <stddef.h>

void  mm_init(void);
void *mm_malloc(size_t size);
void  mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);
double mm_utilization(void);

#endif /* MM_H */