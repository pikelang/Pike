#include "stack_allocator.h"

MALLOC_FUNCTION
static struct chunk * alloc_chunk(size_t size) {
    struct chunk * c = xalloc(sizeof(struct chunk) + size);
    c->data = c->top = c + 1;
    c->size = size;
    c->prev = NULL;
    return c;
}

void stack_alloc_enlarge(struct stack_allocator * a, size_t len) {
    struct chunk * c = a->cur;
    size_t size;

    size = c ? c->size * 2 : a->initial;

    if (len <= size) {
        len = size;
    } else if (len & (len-1)) {
        len |= len >> 1;
        len |= len >> 2;
        len |= len >> 4;
        len |= len >> 8;
        len |= len >> 16;
        len |= len >> 32;
        len ++;
    }
    a->cur = alloc_chunk(len);
    a->cur->prev = c;
}

void stack_alloc_destroy(struct stack_allocator * a) {
    struct chunk * c = a->cur;

    while (c) {
        struct chunk * prev = c->prev;
        free(c);
        c = prev;
    }

    a->cur = NULL;
}
