#ifndef _HEAP_H_
#define _HEAP_H_

#include "types.h"

void heap_init(uint32_t start_addr, size_t size);
void *kmalloc(size_t size);
void *kcalloc(size_t num, size_t size);
void kfree(void *ptr);
size_t heap_get_free(void);
size_t heap_get_used(void);

#endif
