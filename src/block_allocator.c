#include "global.h"
#include "pike_error.h"
#include "pike_memory.h"

#include "block_allocator.h"
#include "bitvector.h"

#include <stdlib.h>

#define BA_BLOCKN(l, p, n) ((struct ba_block_header *)((char*)(p) + (l).doffset + (n)*((l).block_size)))
#define BA_LASTBLOCK(l, p) ((struct ba_block_header*)((char*)(p) + (l).doffset + (l).offset))
#define BA_CHECK_PTR(l, p, ptr)	((size_t)((char*)(ptr) - (char*)(p)) <= (l).offset + (l).doffset)

#define BA_ONE	((struct ba_block_header *)1)
static void print_allocator(const struct block_allocator * a);

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

static INLINE void ba_clear_page(struct block_allocator * a, struct ba_page * p, struct ba_layout * l) {
    p->h.used = 0;
    p->h.flags = BA_FLAG_SORTED;
    p->h.first = BA_BLOCKN(*l, p, 0);
    PIKE_MEMPOOL_ALLOC(a, p->h.first, l->block_size);
    p->h.first->next = BA_ONE;
    PIKE_MEMPOOL_FREE(a, p->h.first, l->block_size);
}

static struct ba_page * ba_alloc_page(struct block_allocator * a, int i) {
    struct ba_layout l = ba_get_layout(a, i);
    size_t n = l.offset + l.block_size + l.doffset;
    struct ba_page * p;
    if (l.alignment) {
	p = (struct ba_page*)aligned_alloc(n, l.alignment);
    } else {
#ifdef DEBUG_MALLOC
	/* In debug malloc mode, calling xalloc from the block alloc may result
	 * in a deadlock, since xalloc will call ba_alloc, which in turn may call xalloc.
	 */
	p = (struct ba_page*)system_malloc(n);
	if (!p) {
	    fprintf(stderr, "Fatal: Out of memory.\n");
	    exit(17);
	}
#else
	p = (struct ba_page*)xalloc(n);
#endif
    }
    ba_clear_page(a, p, &a->l);
    PIKE_MEM_NA_RANGE((char*)p + l.doffset, n - l.doffset);
    return p;
}

static void ba_free_empty_pages(struct block_allocator * a) {
    int i = a->size - 1;

    for (i = a->size - 1; i >= 0; i--) {
        struct ba_page * p = a->pages[i];
        if (p->h.used) break;
#ifdef DEBUG_MALLOC
        system_free(p);
#else
        free(p);
#endif
        a->pages[i] = NULL;
    }

    a->size = i+1;
    a->alloc = a->last_free = MAXIMUM(0, i);
}

PMOD_EXPORT void ba_init_aligned(struct block_allocator * a, unsigned INT32 block_size,
				 unsigned INT32 blocks, unsigned INT32 alignment) {
    PIKE_MEMPOOL_CREATE(a);
    block_size = MAXIMUM(block_size, sizeof(struct ba_block_header));
    if (alignment) {
	if (alignment & (alignment - 1))
	    Pike_fatal("Block allocator alignment is not a power of 2.\n");
	if (block_size & (alignment-1))
	    Pike_fatal("Block allocator block size is not aligned.\n");
	a->l.doffset = PIKE_ALIGNTO(sizeof(struct ba_page), alignment);
    } else {
	a->l.doffset = sizeof(struct ba_page);
    }

    blocks = round_up32(blocks);
    a->alloc = a->last_free = 0;
    a->size = 1;
    a->l.block_size = block_size;
    a->l.blocks = blocks;
    a->l.offset = block_size * (blocks-1);
    a->l.alignment = alignment;
    memset(a->pages, 0, sizeof(a->pages));
    a->pages[0] = ba_alloc_page(a, 0);
}

PMOD_EXPORT void ba_destroy(struct block_allocator * a) {
    int i;

    if (!a->l.offset) return;

    for (i = 0; i < a->size; i++) {
	if (a->pages[i]) {
#ifdef DEBUG_MALLOC
	    system_free(a->pages[i]);
#else
	    free(a->pages[i]);
#endif
	    a->pages[i] = NULL;
	}
    }
    a->size = 0;
    a->alloc = 0;
    a->last_free = 0;
    PIKE_MEMPOOL_DESTROY(a);
}

PMOD_EXPORT void ba_free_all(struct block_allocator * a) {
    int i;
    struct ba_layout l;

    if (!a->l.offset) return;
    if (!a->size) return;

    l = ba_get_layout(a, 0);

    for (i = 0; i < a->size; i++) {
        struct ba_page * page = a->pages[i];
        ba_clear_page(a, page, &l);
        ba_double_layout(&l);
    }
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
    size_t n = 0, b = sizeof( struct block_allocator );
    unsigned int i;
    for( i=0; i<a->size; i++ )
    {
        struct ba_layout l = ba_get_layout( a, i );
        b += l.offset + l.block_size + l.doffset;
        n += a->pages[i]->h.used;
    }
    *num = n;
    *size = b;
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
	ba_init_aligned(a, a->l.block_size, a->l.blocks, a->l.alignment);
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
    PIKE_MEMPOOL_ALLOC(a, ptr, a->l.block_size);
    PIKE_MEM_RW_RANGE(ptr, sizeof(struct ba_block_header));

    p->h.used++;

#ifdef PIKE_DEBUG
    {
	struct ba_layout l = ba_get_layout(a, a->alloc);
	if (!BA_CHECK_PTR(l, p, ptr)) {
	    print_allocator(a);
	    Pike_fatal("about to return pointer from hell: %p\n", ptr);
	}
    }
#endif

    if (ptr->next == BA_ONE) {
	struct ba_layout l = ba_get_layout(a, a->alloc);
	p->h.first = (struct ba_block_header*)((char*)ptr + a->l.block_size);
	PIKE_MEMPOOL_ALLOC(a, p->h.first, a->l.block_size);
	p->h.first->next = (struct ba_block_header*)(ptrdiff_t)!(p->h.first == BA_LASTBLOCK(l, p));
	PIKE_MEMPOOL_FREE(a, p->h.first, a->l.block_size);
    } else {
	p->h.first = ptr->next;
    }
    PIKE_MEM_WO_RANGE(ptr, sizeof(struct ba_block_header));

#if PIKE_DEBUG
    if (a->l.alignment && (size_t)ptr & (a->l.alignment - 1)) {
	print_allocator(a);
	Pike_fatal("Returning unaligned pointer.\n");
    }
#endif

    return ptr;
}

PMOD_EXPORT void ba_free(struct block_allocator * a, void * ptr) {
    int i = a->last_free;
    struct ba_page * p = a->pages[i];
    struct ba_layout l = ba_get_layout(a, i);

#if PIKE_DEBUG
    if (a->l.alignment && (size_t)ptr & (a->l.alignment - 1)) {
	print_allocator(a);
	Pike_fatal("Returning unaligned pointer.\n");
    }
#endif

    if (BA_CHECK_PTR(l, p, ptr)) goto found;

#ifdef PIKE_DEBUG
    p = NULL;
#endif

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
	    print_allocator(a);
	    Pike_fatal("freeing from empty page %p\n", p);
	}
#endif
	if (!(--p->h.used)) {
            if (i+1 == a->size) {
                ba_free_empty_pages(a);
            } else {
                ba_clear_page(a, p, &l);
            }
	}
    }
#ifdef PIKE_DEBUG
    } else {
	print_allocator(a);
	Pike_fatal("ptr %p not in any page.\n", ptr);
    }
#endif
    PIKE_MEMPOOL_FREE(a, ptr, a->l.block_size);
}

static void print_allocator(const struct block_allocator * a) {
    int i;
    struct ba_layout l;

    for (i = a->size-1, l = ba_get_layout(a, i); i >= 0; ba_half_layout(&l), i--) {
	struct ba_page * p = a->pages[i];
	fprintf(stderr, "page: %p used: %u/%u last: %p p+offset: %p\n", a->pages[i],
		p->h.used, l.blocks,
		BA_BLOCKN(l, p, l.blocks-1), BA_LASTBLOCK(l, p));
    }
}
