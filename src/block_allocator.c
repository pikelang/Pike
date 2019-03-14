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
#define BA_FLAG_SORTED 1u

#ifdef PIKE_DEBUG
static void print_allocator(const struct block_allocator * a);
#endif

#ifdef PIKE_DEBUG
static void ba_check_ptr(struct block_allocator * a, int page, void * ptr, struct ba_block_header *loc,
                         int ln);
#endif

static INLINE unsigned INT32 ba_block_number(const struct ba_layout * l, const struct ba_page * p,
                                             const void * ptr) {
    return ((char*)ptr - (char*)BA_BLOCKN(*l, p, 0)) / l->block_size;
}

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
	p = aligned_alloc(n, l.alignment);
    } else {
#ifdef DEBUG_MALLOC
	/* In debug malloc mode, calling xalloc from the block alloc may result
	 * in a deadlock, since xalloc will call ba_alloc, which in turn may call xalloc.
	 */
	p = system_malloc(n);
	if (!p) {
	    fprintf(stderr, "Fatal: Out of memory.\n");
	    exit(17);
	}
#else
	p = xalloc(n);
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

PMOD_EXPORT void ba_low_init_aligned(struct block_allocator * a) {
    unsigned INT32 block_size = MAXIMUM(a->l.block_size, sizeof(struct ba_block_header));

    PIKE_MEMPOOL_CREATE(a);

    if (a->l.alignment) {
	if (a->l.alignment & (a->l.alignment - 1))
	    Pike_fatal("Block allocator a->l.alignment is not a power of 2.\n");
	if (block_size & (a->l.alignment-1))
	    Pike_fatal("Block allocator block size is not aligned.\n");
	a->l.doffset = PIKE_ALIGNTO(sizeof(struct ba_page), a->l.alignment);
    } else {
	a->l.doffset = sizeof(struct ba_page);
    }

    a->l.blocks = round_up32(a->l.blocks);
    a->l.block_size = block_size;
    a->l.offset = block_size * (a->l.blocks-1);
}

PMOD_EXPORT void ba_init_aligned(struct block_allocator * a, unsigned INT32 block_size,
				 unsigned INT32 blocks, unsigned INT32 alignment) {
    a->l.blocks = blocks;
    a->l.block_size = block_size;
    a->l.alignment = alignment;
    ba_low_init_aligned(a);
    a->alloc = a->last_free = a->size = 0;
    memset(a->pages, 0, sizeof(a->pages));
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
    a->alloc = a->last_free = a->size = 0;
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
    int i;

    if (!a->l.offset) {
        ba_low_init_aligned(a);
    }

    /*
     * The biggest page is full, lets try to find some space in the previous ones
     */
    for (i = a->size - 1; i >= 0; i--) {
        struct ba_page * p = a->pages[i];

        if (p->h.first) {
            a->alloc = i;
            return;
        }
    }

    if (a->size == (sizeof(a->pages)/sizeof(a->pages[0]))) {
        Pike_error("Out of memory.");
    }
    a->pages[a->size] = ba_alloc_page(a, a->size);
    a->alloc = a->size;
    a->size++;
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
    ba_check_ptr(a, a->alloc, ptr, NULL, __LINE__);
#endif

    if (ptr->next == BA_ONE) {
	struct ba_layout l = ba_get_layout(a, a->alloc);
	p->h.first = (struct ba_block_header*)((char*)ptr + a->l.block_size);
	PIKE_MEMPOOL_ALLOC(a, p->h.first, a->l.block_size);
	p->h.first->next = (struct ba_block_header*)(ptrdiff_t)!(p->h.first == BA_LASTBLOCK(l, p));
	PIKE_MEMPOOL_FREE(a, p->h.first, a->l.block_size);
    } else {
#ifdef PIKE_DEBUG
        if (ptr->next)
            ba_check_ptr(a, a->alloc, ptr->next, ptr, __LINE__);
#endif
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

    if (p) {
	struct ba_block_header * b = (struct ba_block_header*)ptr;
#ifdef PIKE_DEBUG
	if (!p->h.used) {
	    print_allocator(a);
	    Pike_fatal("freeing from empty page %p\n", p);
	}
        ba_check_ptr(a, a->last_free, ptr, NULL, __LINE__);
#endif
        b->next = p->h.first;
        p->h.first = b;
        p->h.flags = 0;
	if (!(--p->h.used)) {
            if (i+1 == a->size) {
                ba_free_empty_pages(a);
            } else {
                ba_clear_page(a, p, &l);
            }
	}
    } else {
#ifdef PIKE_DEBUG
	print_allocator(a);
#endif
	Pike_fatal("ptr %p not in any page.\n", ptr);
    }
    PIKE_MEMPOOL_FREE(a, ptr, a->l.block_size);
}

#ifdef PIKE_DEBUG
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

#define Pike_nfatal(n) \
    (fprintf(stderr,msg_fatal_error,__FILE__,(long)(n)),debug_fatal)

static void ba_check_ptr(struct block_allocator * a, int page, void * ptr, struct ba_block_header * loc,
                         int ln) {
    struct ba_layout l = ba_get_layout(a, page);
    struct ba_page * p = a->pages[page];

    if (BA_BLOCKN(l, p, ba_block_number(&l, p, ptr)) != ptr) {
        char * block = (char*)BA_BLOCKN(l, p, ba_block_number(&l, p, ptr));
        print_allocator(a);
        if (loc) fprintf(stderr, "In block %p:\n", loc);
        Pike_nfatal(ln)("Free-List corruption. List pointer %p is inside block [%p , %p)\n",
                        ptr, block, block + l.block_size);
    }

    if (!BA_CHECK_PTR(l, p, ptr)) {
        print_allocator(a);
        if (loc) fprintf(stderr, "In block %p:\n", loc);
        Pike_nfatal(ln)("Free-List corruption. Block %p does not belong to page %p\n", ptr, p);
    }
}
#endif

#if SIZEOF_LONG == 8 || SIZEOF_LONG_LONG == 8
#define BV_LENGTH   64
#define BV_ONE	    ((unsigned INT64)1)
#define BV_NIL	    ((unsigned INT64)0)
#define BV_CLZ	    clz64
#define BV_CTZ	    ctz64
typedef unsigned INT64 bv_int_t;
#else
#define BV_LENGTH   32
#define BV_ONE	    ((unsigned INT32)1)
#define BV_NIL	    ((unsigned INT32)0)
#define BV_CLZ	    clz32
#define BV_CTZ	    ctz32
typedef unsigned INT32 bv_int_t;
#endif

#define BV_WIDTH    (BV_LENGTH/8)

struct bitvector {
    size_t length;
    bv_int_t * v;
};

static INLINE void bv_set_vector(struct bitvector * bv, void * p) {
    bv->v = (bv_int_t*)p;
}

static INLINE size_t bv_byte_length(struct bitvector * bv) {
    size_t bytes = ((bv->length + 7) / 8);

    return PIKE_ALIGNTO(bytes, BV_WIDTH);
}

static INLINE void bv_set(struct bitvector * bv, size_t n, int value) {
    size_t bit = n % BV_LENGTH;
    size_t c = n / BV_LENGTH;
    bv_int_t * _v = bv->v + c;
    if (value) *_v |= BV_ONE << bit;
    else *_v &= ~(BV_ONE << bit);
}

static INLINE int bv_get(struct bitvector * bv, size_t n) {
    size_t bit = n % BV_LENGTH;
    size_t c = n / BV_LENGTH;
    return !!(bv->v[c] & (BV_ONE << bit));
}

static INLINE size_t bv_ctz(struct bitvector * bv, size_t n) {
    size_t bit = n % BV_LENGTH;
    size_t c = n / BV_LENGTH;
    bv_int_t * _v = bv->v + c;
    bv_int_t V = *_v & (~BV_NIL << bit);

    bit = c * BV_LENGTH;
    while (!(V)) {
	if (bit >= bv->length) {
	    bit = (size_t)-1;
	    goto RET;
	}
	V = *(++_v);
	bit += (BV_WIDTH*8);
    }
    bit += BV_CTZ(V);
    if (bit >= bv->length) bit = (size_t)-1;

RET:
    return bit;
}

#ifdef BA_DEBUG
static INLINE void bv_print(struct bitvector * bv) {
    size_t i;
    for (i = 0; i < bv->length; i++) {
	fprintf(stderr, "%d", bv_get(bv, i));
    }
    fprintf(stderr, "\n");
}
#endif

struct ba_block_header * ba_sort_list(const struct ba_page * p,
				      struct ba_block_header * b,
				      const struct ba_layout * l) {
    struct bitvector v;
    size_t i, j;
    struct ba_block_header ** t = &b;

    v.length = l->blocks;
    i = bv_byte_length(&v);
    /* we should probably reuse an area for this.
     */
    bv_set_vector(&v, alloca(i));
    memset(v.v, 0, i);

    /*
     * store the position of all blocks in a bitmask
     */
    while (b) {
	unsigned INT32 n = ba_block_number(l, p, b);
	bv_set(&v, n, 1);
	if (b->next == BA_ONE) {
	    v.length = n+1;
	    break;
	} else b = b->next;
    }

    /*
     * Handle consecutive free blocks in the end, those
     * we dont need anyway.
     */
    if (v.length) {
	i = v.length-1;
	while (i && bv_get(&v, i)) { i--; }
	v.length = i+1;
    }

    j = 0;

    /*
     * We now rechain all blocks.
     */
    while ((i = bv_ctz(&v, j)) != (size_t)-1) {
	*t = BA_BLOCKN(*l, p, i);
	t = &((*t)->next);
	j = i+1;
    }

    /*
     * The last one
     */

    if (v.length < l->blocks) {
	*t = BA_BLOCKN(*l, p, v.length);
	(*t)->next = BA_ONE;
    } else *t = NULL;

    return b;
}

static INLINE void ba_list_defined(struct block_allocator * a, struct ba_block_header * b) {
#ifdef USE_VALGRIND
    while (b && b != BA_ONE) {
        PIKE_MEMPOOL_ALLOC(a, b, a->l.block_size);
        PIKE_MEM_RW_RANGE(b, sizeof(struct ba_block_header));
        b = b->next;
    }
#endif
}

static INLINE void ba_list_undefined(struct block_allocator * a, struct ba_block_header * b) {
#ifdef USE_VALGRIND
    while (b && b != BA_ONE) {
        struct ba_block_header * next = b->next;
        PIKE_MEMPOOL_FREE(a, b, a->l.block_size);
        b = next;
    }
#endif
}

/*
 * This function allows iteration over all allocated blocks. Some things are not allowed:
 *  - throwing from within the callback
 *  - allocating blocks during iteration
 *  - nested iteration
 *
 *  - freeing is OK, however some nodes will _still_ beiterated over, when they are freed during the
 *    iteration.
 *
 *  TODO: if needed, allocation can be fixed. For that to work, the free list of the currently
 *  iterated page has to be removed and restored after iteration. that would guarantee allocation
 *  from a different page
 *
 *  NOTE
 *    the callback will be called multiple times. for a usage example, see las.c
 */
PMOD_EXPORT
void ba_walk(struct block_allocator * a, ba_walk_callback cb, void * data) {
    struct ba_iterator it;
    unsigned INT32 i;

    it.l = ba_get_layout(a, 0);

    if (!a->size) return;

    for (i = 0; i < a->size; i++) {
        struct ba_page * p = a->pages[i];
        if (p && p->h.used) {
            struct ba_block_header * free_list, * free_block;

            ba_list_defined(a, p->h.first);

            if (!(p->h.flags & BA_FLAG_SORTED)) {
                p->h.first = ba_sort_list(p, p->h.first, &it.l);
                p->h.flags |= BA_FLAG_SORTED;
            }
            /* we fake an allocation to prevent the page from being freed during iteration */
            p->h.used ++;

            free_list = p->h.first;
            free_block = free_list;

            it.cur = BA_BLOCKN(it.l, p, 0);

            while(1) {
                if (free_block == NULL) {
                    it.end = ((char*)BA_LASTBLOCK(it.l, p) + it.l.block_size);
#ifdef PIKE_DEBUG
                    if ((char*)it.end < (char*)it.cur)
                        Pike_fatal("Free list not sorted in ba_walk.\n");
#endif
                    if ((char*)it.end != (char*)it.cur) {
                        cb(&it, data);
                    }
                    break;
                } else if (free_block == BA_ONE) {
                    /* all consecutive blocks are free, so we are dont */
                    break;
                }

                it.end = free_block;
                free_block = free_block->next;

#ifdef PIKE_DEBUG
                if ((char*)it.end < (char*)it.cur)
                    Pike_fatal("Free list not sorted in ba_walk.\n");
#endif
                if ((char*)it.end != (char*)it.cur)
                    cb(&it, data);

                it.cur = (char*)it.end + it.l.block_size;
            }

            /* if the callback throws, this will never happen */
            ba_list_undefined(a, free_list);
            p->h.used--;
        }
        ba_double_layout(&it.l);
    }

    /* during the iteration blocks might have been freed. The pages will still be there, so we might have
     * to do some cleanup. */
    if (!a->pages[a->size-1]->h.used)
        ba_free_empty_pages(a);
}
