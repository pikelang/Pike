#include "global.h"

struct stack_allocator {
    struct chunk * cur;
};

struct chunk {
    struct chunk * prev;
    void * data, * top;
    size_t size;
};

void stack_alloc_init(struct stack_allocator * a, size_t initial);
void stack_alloc_enlarge(struct stack_allocator * a, size_t size);
void stack_alloc_destroy(struct stack_allocator * a);

static INLINE size_t left(struct chunk * c) {
    return (size_t)(((char*)c->data + c->size) - (char*)c->top);
}

static INLINE void sa_alloc_enlarge(struct stack_allocator * a, size_t size) {
    struct chunk * c = a->cur;

    if (left(c) < size)
        stack_alloc_enlarge(a, size);
}

static INLINE void * sa_alloc_fast(struct stack_allocator * a, size_t size) {
    struct chunk * c = a->cur;
    void * ret = c->top;
    c->top = (char*)c->top + size;
    return ret;
}

static INLINE void * sa_alloc(struct stack_allocator * a, size_t size) {
    sa_alloc_enlarge(a, size);
    return sa_alloc_fast(a, size);
}
