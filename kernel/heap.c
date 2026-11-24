#include "heap.h"

typedef struct block_header {
    size_t size;
    bool is_free;
    struct block_header *next;
} block_header_t;

static size_t   heap_total_size    = 0;
static block_header_t *free_list   = NULL;

void heap_init(uint32_t start_addr, size_t size) {
    (void)start_addr;
    heap_total_size    = size;

    free_list = (block_header_t *)start_addr;
    free_list->size    = size - sizeof(block_header_t);
    free_list->is_free = true;
    free_list->next    = NULL;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    /* Align to 8 bytes */
    size = (size + 7) & ~7;

    block_header_t *curr = free_list;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            /* Can we split this block? */
            if (curr->size >= size + sizeof(block_header_t) + 16) {
                block_header_t *next_block = (block_header_t *)((uint8_t *)curr + sizeof(block_header_t) + size);
                next_block->size    = curr->size - size - sizeof(block_header_t);
                next_block->is_free = true;
                next_block->next    = curr->next;

                curr->size = size;
                curr->next = next_block;
            }
            curr->is_free = false;
            return (void *)((uint8_t *)curr + sizeof(block_header_t));
        }
        curr = curr->next;
    }
    return NULL; /* Out of memory */
}

void *kcalloc(size_t num, size_t size) {
    size_t total = num * size;
    void *ptr = kmalloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void kfree(void *ptr) {
    if (!ptr) return;

    block_header_t *header = (block_header_t *)((uint8_t *)ptr - sizeof(block_header_t));
    header->is_free = true;

    /* Coalesce adjacent free blocks */
    block_header_t *curr = free_list;
    while (curr) {
        if (curr->is_free && curr->next && curr->next->is_free) {
            curr->size += sizeof(block_header_t) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

size_t heap_get_free(void) {
    size_t free_mem = 0;
    block_header_t *curr = free_list;
    while (curr) {
        if (curr->is_free) free_mem += curr->size;
        curr = curr->next;
    }
    return free_mem;
}

size_t heap_get_used(void) {
    return heap_total_size - heap_get_free();
}
