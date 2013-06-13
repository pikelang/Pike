#include "global.h"
#include "pike_error.h"
#include "pike_memory.h"

#include "block_allocator.h"
#include "bitvector.h"

#define BA_BLOCKN(l, p, n) ((struct ba_block_header *)(((char*)((p)+1)) + (n)*((l).block_size)))
#define BA_LASTBLOCK(l, p) ((struct ba_block_header*)((char*)((p)+1) + (l).offset))
#define BA_CHECK_PTR(l, p, ptr)	((size_t)((char*)(ptr) - (char*)((p)+1)) <= (l).offset)

#define BA_ONE	((struct ba_block_header *)1)

static INLINE void ba_dec_layout(struct ba_layout * l, int i) {
    l->blocks >>= i;
    l->offset += l->block_size;
    l->offset >>= i;
    l->offset -= l->block_size;
}

static INLINE void ba_inc_layout(struct ba_layout * l, int i) {
    l->blocks <<= i;
    l->offset += l->block_size;
    l->offset <<= i;
    l->offset -= l->block_size;
}

static INLINE void ba_double_layout(struct ba_layout * l) {
    ba_inc_layout(l, 1);
}

static INLINE void ba_half_layout(struct ba_layout * l) {
    ba_dec_layout(l, 1);
}

static INLINE struct ba_layout ba_get_layout(const struct block_allocator * a, int i) {
    struct ba_layout l = a->l;
    ba_inc_layout(&l, i);
    return l;
}

struct ba_block_header {
    struct ba_block_header * next;
};

static struct ba_page * ba_alloc_page(struct block_allocator * a, int i) {
    struct ba_layout l = ba_get_layout(a, i);
    size_t n = l.offset + l.block_size + sizeof(struct ba_page);
    struct ba_page * p = (struct ba_page*)xalloc(n);
    p->h.first = BA_BLOCKN(a->l, p, 0);
    p->h.first->next = BA_ONE;
    p->h.used = 0;
    return p;
}

PMOD_EXPORT void ba_init(struct block_allocator * a, unsigned INT32 block_size, unsigned INT32 blocks) {
    block_size = MAXIMUM(block_size, sizeof(struct ba_block_header));
    blocks = round_up32(blocks);
    a->alloc = a->last_free = 0;
    a->size = 1;
    a->l.block_size = block_size;
    a->l.blocks = blocks;
    a->l.offset = block_size * (blocks-1);
    memset(a->pages, 0, sizeof(a->pages));
    a->pages[0] = ba_alloc_page(a, 0);
}

PMOD_EXPORT void ba_destroy(struct block_allocator * a) {
    int i;
    for (i = 0; i < a->size; i++) {
	if (a->pages[i]) {
	    free(a->pages[i]);
	    a->pages[i] = NULL;
	}
    }
    a->size = 0;
    a->alloc = 0;
    a->last_free = 0;
}

PMOD_EXPORT size_t ba_count(const struct block_allocator * a) {
    size_t c = 0;
    unsigned int i;
    for (i = 0; i < a->size; i++) {
	c += a->pages[i]->h.used;
    }

    return c;
}

PMOD_EXPORT void ba_count_all(const struct block_allocator * a, size_t * num, size_t * size) {
    if (a->size) {
	size_t n = (a->l.blocks << (a->size-1)) - a->l.blocks;
	*num = n;
	*size = a->l.block_size * n;
    } else {
	*num = *size = 0;
    }
}

static void ba_low_alloc(struct block_allocator * a) {
    if (a->l.offset) {
	unsigned int i;

	for (i = 1; i <= a->size; i++) {
	    struct ba_page * p = a->pages[a->size - i];

	    if (p->h.first) {
		a->alloc = a->size - i;
		return;
	    }
	}
	if (a->size == (sizeof(a->pages)/sizeof(a->pages[0]))) {
	    Pike_error("Out of memory.");
	}
	a->pages[a->size] = ba_alloc_page(a, a->size);
	a->alloc = a->size;
	a->size++;
    } else {
	ba_init(a, a->l.block_size, a->l.blocks);
    }
}

ATTRIBUTE((malloc))
PMOD_EXPORT void * ba_alloc(struct block_allocator * a) {
    struct ba_page * p = a->pages[a->alloc];
    struct ba_block_header * ptr;

    if (!p || !p->h.first) {
	ba_low_alloc(a);
	p = a->pages[a->alloc];
    }

    ptr = p->h.first;

    p->h.used++;

    if (ptr->next == BA_ONE) {
	struct ba_layout l = ba_get_layout(a, a->alloc);
	p->h.first = (struct ba_block_header*)((char*)ptr + a->l.block_size);
	p->h.first->next = (struct ba_block_header*)(ptrdiff_t)!(p->h.first == BA_LASTBLOCK(l, p));
    } else {
	p->h.first = ptr->next;
    }

    return ptr;
}

PMOD_EXPORT void ba_free(struct block_allocator * a, void * ptr) {
    int i = a->last_free;
    struct ba_page * p = a->pages[i];
    struct ba_layout l = ba_get_layout(a, i);

    if (BA_CHECK_PTR(l, p, ptr)) goto found;

    p = NULL;

    for (i = a->size-1, l = ba_get_layout(a, i); i >= 0; i--, ba_half_layout(&l)) {
	if (BA_CHECK_PTR(l, a->pages[i], ptr)) {
	    a->last_free = i;
	    p = a->pages[i];
	    break;
	}
    }
found:

#ifdef PIKE_DEBUG
    if (p) {
#endif
    {
	struct ba_block_header * b = (struct ba_block_header*)ptr;
	b->next = p->h.first;
	p->h.first = b;
#ifdef PIKE_DEBUG
	if (!p->h.used) {
	    fprintf(stderr, "freeing from empty page %p\n", p);
	    goto ERR;
	}
#endif
	if (!(--p->h.used) && i+1 == a->size) {
	    while (i >= 0 && !(p->h.used)) {
		free(p);
		a->pages[i] = NULL;

		p = a->pages[--i];
	    }
	    a->size = i+1;
	    a->alloc = a->last_free = MAXIMUM(0, i);
	}
    }
#ifdef PIKE_DEBUG
    } else {
	int i;
ERR:
	for (i = a->size-1, l = ba_get_layout(a, i); i >= 0; ba_half_layout(&l), i--) {
	    p = a->pages[i];
	    fprintf(stderr, "page: %p used: %u/%u last: %p p+offset: %p\n", a->pages[i],
		    p->h.used, l.blocks,
		    BA_BLOCKN(l, p, l.blocks-1), BA_LASTBLOCK(l, p));
	}
	Pike_fatal("ptr %p not in any page.\n", ptr);
    }
#endif
}
