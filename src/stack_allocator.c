#include "stack_allocator.h"

static struct chunk * alloc_chunk(size_t size) {
    struct chunk * c = (struct chunk*)xalloc(sizeof(struct chunk) + size);
    c->data = c->top = c + 1;
    c->size = size;
    c->prev = NULL;
    return c;
}

void stack_alloc_init(struct stack_allocator * a, size_t initial) {
    a->cur = alloc_chunk(initial);
}

void stack_alloc_enlarge(struct stack_allocator * a, size_t len) {
    struct chunk * c = a->cur;
    if (len <= c->size * 2) {
        len = c->size * 2;
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
