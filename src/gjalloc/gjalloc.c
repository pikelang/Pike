#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* vim indenting workaround */
#endif
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>
#ifdef BA_STATS
# include <unistd.h>
# include <mcheck.h>
#endif

#include "gjalloc.h"

#ifdef BA_NEED_ERRBUF
EXPORT char errbuf[128];
#endif

static INLINE void ba_htable_insert(const struct block_allocator * a,
				    struct ba_page * ptr);
static INLINE void ba_htable_grow(struct block_allocator * a);
static void ba_update_slot(struct block_allocator * a,
			   struct ba_page * p,
			   struct ba_page_header * h);

#define BA_NBLOCK(l, p, ptr)	((uintptr_t)((char*)(ptr) - (char*)((p)+1))/(uintptr_t)((l).block_size))

#define BA_PAGESIZE(l)	    ((l).offset + (l).block_size)
#define BA_HASH_MASK(a)  (((a->allocated)) - 1)

#define BA_BYTES(a)	( (sizeof(void*) * ((a)->allocated) ) )

#define PRINT_NODE(a, name) do {		\
    fprintf(stderr, #name": %p\n", a->name);	\
} while (0)

#define PRINT_LIST(a, name) do {		\
    uint32_t c = 0;				\
    struct ba_page * _p = (a->name);		\
    fprintf(stderr, #name": ");			\
    while (_p) {				\
	fprintf(stderr, "%p -> ", _p);		\
	if (c++ > a->num_pages) break;		\
	_p = _p->next;				\
    }						\
    if (_p) fprintf(stderr, "(cycle)\n");	\
    else fprintf(stderr, "(nil)\n");		\
} while (0)


EXPORT void ba_show_pages(const struct block_allocator * a) {
    fprintf(stderr, "allocated: %u\n", a->allocated);
    fprintf(stderr, "num_pages: %u\n", a->num_pages);
    fprintf(stderr, "max_empty_pages: %u\n", a->max_empty_pages);
    fprintf(stderr, "empty_pages: %u\n", a->empty_pages);
    fprintf(stderr, "magnitude: %u\n", a->magnitude);
    PRINT_NODE(a, alloc);
    PRINT_LIST(a, pages[0]);
    PRINT_LIST(a, pages[1]);
    PRINT_LIST(a, pages[2]);
    PRINT_NODE(a, last_free);

    /*
     * TODO: we should consider updating alloc/last_free here
     */

    PAGE_LOOP(a, {
	uint32_t blocks_used;
	blocks_used = p->h.used;
	fprintf(stderr, "(%p)\t%f\t(%u %d) --> (prev: %p | next: %p)\n",
		p, blocks_used/(double)a->l.blocks * 100,
		blocks_used,
		blocks_used,
		p->prev, p->next);
    });
}

#ifdef BA_STATS
# define PRCOUNT(X)	fprintf(stat_file, #X " %lg %% ", (a->stats.X/all)*100);
static FILE * stat_file = NULL;
ATTRIBUTE((constructor))
static void ba_stats_init() {
    char * fname = getenv("BA_STAT_TRACE");
    if (fname) {
	stat_file = fopen(fname, "w");
    } else stat_file = stderr;
}
ATTRIBUTE((destructor))
static void ba_stats_deinit() {
    if (stat_file != stderr) {
	fclose(stat_file);
	stat_file = stderr;
    }
}
EXPORT void ba_print_stats(struct block_allocator * a) {
    double all;
    struct ba_stats * s = &a->stats;
    size_t used = s->st_max * a->l.block_size;
    size_t malloced = s->st_max_pages * (a->l.block_size * a->l.blocks);
    size_t overhead = s->st_max_pages * sizeof(struct ba_page);
    int mall = s->st_mallinfo.uordblks;
    int moverhead = s->st_mallinfo.fordblks;
    const char * fmt = "%s:\n max used %.1lf kb in %.1lf kb (#%lld) pages"
		     " (overhead %.2lf kb)"
		     " mall: %.1lf kb overhead: %.1lf kb "
		     " page_size: %d block_size: %d\n";
    overhead += s->st_max_allocated * 2 * sizeof(void*);
    if (s->st_max == 0) return;
    fprintf(stat_file, fmt,
	   s->st_name,
	   used / 1024.0, malloced / 1024.0,
	   s->st_max_pages, overhead / 1024.0,
	   mall / 1024.0,
	   moverhead / 1024.0,
	   a->l.block_size * a->l.blocks,
	   a->l.block_size
	   );
    all = ((a->stats.free_fast1 + a->stats.free_fast2 + a->stats.find_linear + a->stats.find_hash + a->stats.free_empty + a->stats.free_full));
    if (all == 0.0) return;
    fprintf(stat_file, "free COUNTS: ");
    PRCOUNT(free_fast1);
    PRCOUNT(free_fast2);
    PRCOUNT(find_linear);
    PRCOUNT(find_hash);
    PRCOUNT(free_empty);
    PRCOUNT(free_full);
    fprintf(stat_file, " all %lu\n", (long unsigned int)all);
}
#endif


#ifndef BA_CMEMSET
static INLINE void cmemset(char * dst, const char * src, size_t s,
			   size_t n) {
    memcpy(dst, src, s);

    for (--n,n *= s; n >= s; n -= s,s <<= 1)
	memcpy(dst + s, dst, s);

    if (n) memcpy(dst + s, dst, n);
}
#define BA_CMEMSET(dst, src, s, n)	cmemset(dst, src, s, n)
#endif

#ifndef BA_XALLOC
ATTRIBUTE((malloc))
static INLINE void * ba_xalloc(const size_t size) {
    void * p = malloc(size);
    if (!p) ba_error("no mem");
    return p;
}
#define BA_XALLOC(size)	ba_xalloc(size)
#endif

#ifndef BA_XREALLOC
ATTRIBUTE((malloc))
static INLINE void * ba_xrealloc(void * p, const size_t size) {
    p = realloc(p, size);
    if (!p) ba_error("no mem");
    return p;
}
#define BA_XREALLOC(ptr, size)	ba_xrealloc(ptr, size)
#endif

EXPORT void ba_init(struct block_allocator * a, uint32_t block_size,
		    uint32_t blocks) {

    if (!block_size) BA_ERROR(a, "ba_init called with zero block_size\n");

    if (block_size < sizeof(struct ba_block_header)) {
	block_size = sizeof(struct ba_block_header);
    }

    if (!blocks) blocks = 512;

    ba_init_layout(&a->l, block_size, blocks);
    ba_align_layout(&a->l);

    a->pages[0] = a->pages[1] = a->pages[2] = NULL;
    a->last_free = NULL;
    a->alloc = NULL;
    a->h.first = NULL;

#ifdef BA_DEBUG
    fprintf(stderr, "blocks: %u block_size: %u page_size: %lu\n",
	    blocks, block_size, (unsigned long)BA_PAGESIZE(a->l));
#endif


#ifndef ctz32
# define ctz32 __builtin_ctz
#endif
    a->magnitude = ctz32(round_up32(BA_PAGESIZE(a->l)));
    a->num_pages = 0;
    a->empty_pages = 0;
    a->max_empty_pages = BA_MAX_EMPTY;

    /* we start with management structures for BA_ALLOC_INITIAL pages */
    a->allocated = BA_ALLOC_INITIAL;
    a->htable = (struct ba_page **)BA_XALLOC(BA_ALLOC_INITIAL * sizeof(void*));
    memset(a->htable, 0, BA_ALLOC_INITIAL * sizeof(void*));
    a->blueprint = NULL;
#ifdef BA_STATS
    memset(&a->stats, 0, sizeof(struct ba_stats));
#endif
#ifdef BA_USE_VALGRIND
    VALGRIND_CREATE_MEMPOOL(a, 0, 0);
#endif

#ifdef BA_DEBUG
    fprintf(stderr, " -> blocks: %u block_size: %u page_size: %lu mallocing size: %lu, next pages: %u magnitude: %u\n",
	    a->l.blocks, block_size, (unsigned long)(BA_PAGESIZE(a->l)-sizeof(struct ba_page)),
	    (unsigned long)BA_PAGESIZE(a->l),
	    round_up32(BA_PAGESIZE(a->l)),
	    a->magnitude);
#endif
}

static INLINE void ba_sort(struct ba_page * p, struct ba_page_header * h,
			   const struct ba_layout * l) {
    if (!(h->flags & BA_FLAG_SORTED)) {
	h->first = ba_sort_list(p, h->first, l);
	h->flags |= BA_FLAG_SORTED;
    }
}

#define BA_WALK_CHUNKS3(p, h, l, label, C...)  do {			\
    struct ba_page_header * _h = (h);					\
    const struct ba_layout * _l = (l);					\
    char * _start, * _stop;						\
    ba_sort(p, _h, _l);							\
    ba_list_defined(p, _h->first, _l);					\
    _start = (char*)BA_BLOCKN(*_l, (p), 0);				\
    if (!_h->first) {							\
	_stop = (char*)BA_LASTBLOCK(*_l, p) + _l->block_size;		\
    } else _stop = (char*)_h->first;					\
									\
    do {								\
	struct ba_block_header * _b;					\
	if (_start < _stop) C						\
	if (!BA_CHECK_PTR(*_l, (p), _stop)) break;			\
	_b = ((struct ba_block_header*)_stop)->next;		    	\
	_start = _stop + _l->block_size;				\
	if (_b == BA_ONE) break;					\
	if (!_b) _stop = (char*)BA_LASTBLOCK(*_l, p) + _l->block_size;	\
	else _stop = (char*)_b;						\
    } while (1);							\
    ba_list_noaccess(p, _h->first, _l);					\
} while (0)
#define BA_WALK_CHUNKS2(p, h, l, label, C...)  BA_WALK_CHUNKS3(p, h, l, label, C)
#define BA_WALK_CHUNKS(p, h, l, C...) BA_WALK_CHUNKS2(p, h, l, XCONCAT(chunk_loop_label, __LINE__), C)

static INLINE void ba_list_defined(const struct ba_page * p,
				   const struct ba_block_header * b,
				   const struct ba_layout * l) {
#ifdef BA_USE_VALGRIND
    struct ba_page_header h;
    h.first = (struct ba_block_header *)b;
    while (h.first) {
	VALGRIND_MAKE_MEM_DEFINED(h.first, sizeof(void*));
	ba_shift(&h, p, l);
    }
#endif
}

static INLINE void ba_list_noaccess(const struct ba_page * p,
				    const struct ba_block_header * b,
				    const struct ba_layout * l) {
#ifdef BA_USE_VALGRIND
    struct ba_page_header h;
    h.first = (struct ba_block_header *)b;
    while (h.first) {
	b = h.first;
	ba_shift(&h, p, l);
	VALGRIND_MAKE_MEM_NOACCESS(b, sizeof(void*));
    }
#endif
}

static INLINE void ba_free_page(const struct ba_layout * l,
				struct ba_page * p,
				const void * blueprint) {
    const char * bp = (char*)blueprint;
    p->h.first = BA_BLOCKN(*l, p, 0);
    p->h.used = 0;
    p->h.flags = BA_FLAG_SORTED;

    if (bp)
	BA_CMEMSET((char*)(p+1), bp, l->block_size, l->blocks);

#ifdef BA_USE_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED(p->h.first, sizeof(void*));
#endif
    p->h.first->next = BA_ONE;
#ifdef BA_USE_VALGRIND
    VALGRIND_MAKE_MEM_NOACCESS(p+1, l->block_size * l->blocks);
#endif
}

EXPORT INLINE void ba_free_all(struct block_allocator * a) {
    if (!a->allocated) return;
#ifdef BA_USE_VALGRIND
    VALGRIND_DESTROY_MEMPOOL(a);
#endif

    PAGE_LOOP(a, {
	free(p);
    });

    a->h.first = NULL;
    a->alloc = NULL;
    a->num_pages = 0;
    a->empty_pages = 0;
    a->pages[0] = a->pages[1] = a->pages[2] = NULL;
    a->last_free = NULL;
    memset(a->htable, 0, a->allocated * sizeof(void*));
#ifdef BA_USE_VALGRIND
    VALGRIND_CREATE_MEMPOOL(a, 0, 0);
#endif
}

EXPORT void ba_count_all(struct block_allocator * a, size_t *num,
			 size_t *size) {
    /* fprintf(stderr, "page_size: %u, pages: %u\n", BA_PAGESIZE(a->l), a->num_pages); */
    *size = BA_BYTES(a) + a->num_pages * BA_PAGESIZE(a->l);
    *num = ba_count(a);
}

EXPORT size_t ba_count(struct block_allocator * a) {
    size_t n = 0;

    if (a->alloc) a->alloc->h = a->h;
    if (a->last_free) {
	ba_update_slot(a, a->last_free, &a->hf);
	a->last_free = NULL;
    }

    PAGE_LOOP(a, {
	n += p->h.used;
    });
    return n;
}

EXPORT void ba_destroy(struct block_allocator * a) {
    if (!a->allocated) return;
#ifdef BA_USE_VALGRIND
    VALGRIND_DESTROY_MEMPOOL(a);
#endif
    PAGE_LOOP(a, {
	free(p);
    });

    a->h.first = NULL;
    a->alloc = NULL;
    free(a->htable);
    a->last_free = NULL;
    a->allocated = 0;
    a->htable = NULL;
    a->pages[0] = a->pages[1] = a->pages[2] = NULL;
    a->empty_pages = 0;
    a->num_pages = 0;
#ifdef BA_STATS
    a->stats.st_max = a->stats.st_used = 0;
#endif
#ifdef BA_USE_VALGRIND
    VALGRIND_CREATE_MEMPOOL(a, 0, 0);
#endif
}

static INLINE void ba_grow(struct block_allocator * a) {
    if (a->allocated) {
	/* try to detect 32bit overrun? */
	if (a->allocated >= ((uint32_t)1 << (sizeof(uint32_t)*8-1))) {
	    BA_ERROR(a, "too many pages.\n");
	}
	ba_htable_grow(a);
    } else {
	ba_init(a, a->l.block_size, a->l.blocks);
    }
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_grow", __FILE__, __LINE__);
#endif
}

#define MIX(t)	do {		\
    t ^= (t >> 20) ^ (t >> 12);	\
    t ^= (t >> 7) ^ (t >> 4);	\
} while(0)

static INLINE uint32_t hash1(const struct block_allocator * a,
			     const void * ptr) {
    uintptr_t t = ((uintptr_t)ptr) >> a->magnitude;

    MIX(t);

    return (uint32_t) t;
}

static INLINE uint32_t hash2(const struct block_allocator * a,
			     const void * ptr) {
    uintptr_t t = ((uintptr_t)ptr) >> a->magnitude;

    t++;
    MIX(t);

    return (uint32_t) t;
}

static INLINE void ba_htable_grow(struct block_allocator * a) {
    uint32_t n, old;
    old = a->allocated;
    a->allocated *= 2;
    a->htable = (struct ba_page **)BA_XREALLOC(a->htable,
					       a->allocated * sizeof(void*));
    memset(a->htable+old, 0, old * sizeof(void*));
    for (n = 0; n < old; n++) {
	struct ba_page ** b = a->htable + n;

	while (*b) {
	    uint32_t h = hash1(a, BA_LASTBLOCK(a->l, *b)) & BA_HASH_MASK(a);
	    if (h != n) {
		struct ba_page * p = *b;
		*b = p->hchain;
		p->hchain = a->htable[h];
		a->htable[h] = p;
		continue;
	    }
	    b = &(*b)->hchain;
	}
    }
}

static INLINE void ba_htable_shrink(struct block_allocator * a) {
    uint32_t n;

    a->allocated /= 2;

    for (n = 0; n < a->allocated; n++) {
	struct ba_page * p = a->htable[a->allocated + n];

	if (p) {
	    struct ba_page * t = p;
	    if (a->htable[n]) {
		while (t->hchain) t = t->hchain;
		t->hchain = a->htable[n];
	    }
	    a->htable[n] = p;
	}
    }

    a->htable = (struct ba_page **)BA_XREALLOC(a->htable,
					       a->allocated * sizeof(void*));
}

#ifdef BA_DEBUG
EXPORT void ba_print_htable(const struct block_allocator * a) {
    unsigned int i;

    for (i = 0; i < a->allocated; i++) {
	struct ba_page * p = a->htable[i];
	uint32_t hval;
	if (!p) {
	    fprintf(stderr, "empty %u\n", i);
	}
	while (p) {
	    void * ptr = BA_LASTBLOCK(a->l, p);
	    hval = hash1(a, ptr);
	    fprintf(stderr, "%u\t%X[%u]\t%p-%p\t%X\n", i, hval, hval & BA_HASH_MASK(a), p+1, ptr, (unsigned int)((uintptr_t)ptr >> a->magnitude));
	    p = p->hchain;
	}
    }
}
#endif

EXPORT void ba_print_hashstats(const struct block_allocator * a) {
    unsigned int i;

    for (i = 0; i < a->allocated; i++) {
	unsigned int j = 0;
	struct ba_page * p = a->htable[i];
	while (p) {
	    j++;
	    p = p->hchain;
	}
	fprintf(stdout, "%u\t%u\n", i, j);
    }
}

/*
 * insert the pointer to an allocated page into the
 * hashtable.
 */
static INLINE void ba_htable_insert(const struct block_allocator * a,
				    struct ba_page * p) {
    uint32_t hval = hash1(a, BA_LASTBLOCK(a->l, p));
    struct ba_page ** b = a->htable + (hval & BA_HASH_MASK(a));


#ifdef BA_DEBUG
    while (*b) {
	if (*b == p) {
	    fprintf(stderr, "inserting (%p) twice\n", p);
	    fprintf(stderr, "is (%p)\n", *b);
	    BA_ERROR(a, "double insert.\n");
	    return;
	}
	b = &(*b)->hchain;
    }
    b = a->htable + (hval & BA_HASH_MASK(a));
#endif

#if 0/*def BA_DEBUG */
    fprintf(stderr, "replacing bucket %u with page %u by %u\n",
	    hval & BA_HASH_MASK(a), *b, n);
#endif

    p->hchain = *b;
    *b = p;
}


static INLINE void ba_htable_delete(const struct block_allocator * a,
				    struct ba_page * p) {
    uint32_t hval = hash1(a, BA_LASTBLOCK(a->l, p));
    struct ba_page ** b = a->htable + (hval & BA_HASH_MASK(a));

    while (*b) {
	if (*b == p) {
	    *b = p->hchain;
	    p->hchain = 0;
	    return;
	}
	b = &(*b)->hchain;
    }
#ifdef DEBUG
    ba_print_htable(a);
    fprintf(stderr, "ba_htable_delete(%p, %u)\n", p, n);
    BA_ERROR(a, "did not find index to delete.\n")
#endif
}

static INLINE
struct ba_page * ba_htable_lookup(const struct block_allocator * a,
				  const void * ptr) {
    struct ba_page * p;
    uint32_t h[2];
    int b;
    h[0] = hash1(a, ptr);
    h[1] = hash2(a, ptr);

    b = (h[0] == h[1]);

LOOKUP:
    p = a->htable[h[b] & BA_HASH_MASK(a)];
    while (p) {
	if (BA_CHECK_PTR(a->l, p, ptr)) {
	    return p;
	}
	p = p->hchain;
    }
    if (!(b++)) goto LOOKUP;
    return NULL;
}

#ifdef BA_DEBUG
EXPORT INLINE void ba_check_allocator(const struct block_allocator * a,
				      const char *fun, const char *file,
				      int line) {
    uint32_t n;
    int bad = 0;
    struct ba_page * p;

    if (a->alloc)
	a->alloc->h = a->h;

    if (a->empty_pages > a->num_pages) {
	fprintf(stderr, "too many empty pages.\n");
	bad = 1;
    }

    for (n = 0; n < a->allocated; n++) {
	uint32_t hval;
	p = a->htable[n];

	while (p) {
	    /* TODO: this check is broken for local allocators. Its not
	     * necessarily triggered right now...
	     */
	    if (p != a->alloc) {
		if (!p->h.first) {
		    if (p->prev || p->next) {
			fprintf(stderr, "page %p is full but in list."
				" next: %p prev: %p\n",
				p, p->next,
				p->prev);
			bad = 1;
		    }
		    if (p->h.used != a->l.blocks) {
			fprintf(stderr, "page %p has no first, but "
				"used != blocks (%u != %u)\n",
				p, p->h.used, a->l.blocks);
			bad = 1;
		    }
		}
	    }

	    hval = hash1(a, BA_LASTBLOCK(a->l, p)) & BA_HASH_MASK(a);

	    if (hval != n) {
		fprintf(stderr, "page with hash %u found in bucket %u\n",
			hval, n);
		bad = 1;
	    }
	    p = p->hchain;
	}

    }
    if (bad)
	ba_print_htable(a);

    if (bad) {
	ba_show_pages(a);
	fprintf(stderr, "\nCalled from %s:%d:%s\n", fun, line, file);
	fprintf(stderr, "pages: %u\n", a->num_pages);
	BA_ERROR(a, "bad");
    }
}
#endif

ATTRIBUTE((malloc))
EXPORT struct ba_page * ba_low_alloc_page(const struct ba_layout * l) {
    struct ba_page * p;
    p = (struct ba_page *)BA_XALLOC(BA_PAGESIZE(*l));
    p->h.first = BA_BLOCKN(*l, p, 0);
    p->h.first->next = BA_ONE;
    p->h.used = 0;
    p->h.flags = BA_FLAG_SORTED;
    p->prev = p->next = NULL;
    return p;
}


static INLINE struct ba_page * ba_alloc_page(struct block_allocator * a) {
    struct ba_page * p;

    if (unlikely(a->num_pages >= 2*a->allocated)) {
	ba_grow(a);
    }

    a->num_pages++;
    p = (struct ba_page *)BA_XALLOC(BA_PAGESIZE(a->l));
    ba_free_page(&a->l, p, a->blueprint);
    p->next = p->prev = NULL;
    ba_htable_insert(a, p);
#ifdef BA_DEBUG
    fprintf(stderr, "allocated page %p[%p-%p].\n",
	    p, BA_BLOCKN(a->l, p, 0), BA_LASTBLOCK(a->l, p));
    ba_check_allocator(a, "ba_alloc after insert.", __FILE__, __LINE__);
#endif

    return p;
}

/*
 * Get a new page from the allocator to allocate blocks from. p is the old
 * one which is full now.
 */
static INLINE struct ba_page * ba_get_page(struct block_allocator * a,
					   struct ba_page * p) {
    struct ba_page ** b = a->pages;

    if (p) {
	p->h.first = NULL;
	p->h.used = a->l.blocks;
	p->h.flags = BA_FLAG_SORTED;
    }

    /*
     * we try a->pages[0] first, to keep defragmentation
     * low. this might hit performance, by allocating from almost
     * full pages first.
     */
    b += !*b;

    if (*b) {
	p = *b;
	DOUBLE_SHIFT(*b);
    } else if (*(++b)) {
	p = *b;
	DOUBLE_SHIFT(*b);
	a->empty_pages--;
    } else {
	p = ba_alloc_page(a);
    }
    p->next = NULL;

    return p;
}

static void ba_update_slot(struct block_allocator * a,
			   struct ba_page * p,
			   struct ba_page_header * h) {
    struct ba_page ** oslot, ** nslot;
    oslot = ba_get_slot(a, &p->h);
    nslot = ba_get_slot(a, h);

    if (oslot == nslot) return;

    if (oslot) DOUBLE_UNLINK(*oslot, p);
    if (nslot) DOUBLE_LINK(*nslot, p);

    /*
     * page is empty, so we have to treat it specially
     */
    if (!h->used) {
	INC(free_empty);
	a->empty_pages ++;

	/* reset the internal list. this avoids fragmentation which would
	 * otherwise happen when reusing pages. Since that is cheap here,
	 * we do it.
	 */
	h->first = BA_BLOCKN(a->l, p, 0);
#ifdef BA_USE_VALGRIND
	VALGRIND_MAKE_MEM_DEFINED(h->first, sizeof(void*));
#endif
	h->first->next = BA_ONE;
#ifdef BA_USE_VALGRIND
	VALGRIND_MAKE_MEM_NOACCESS(h->first, sizeof(void*));
#endif
	p->h = * h;
	/*
	 * This comparison looks strange, however the first page never gets
	 * freed, so this is actually ok.
	 */
	if (a->empty_pages >= a->max_empty_pages) {
	    ba_remove_page(a);
	}
    } else p->h = * h;
}

EXPORT void ba_global_get_page(struct block_allocator * a) {
    a->alloc = ba_get_page(a, a->alloc);
    if (a->alloc == a->last_free) {
	ba_update_slot(a, a->last_free, &a->hf);
	a->last_free = NULL;
    }
    a->h = a->alloc->h;
}

static INLINE void ba_low_get_free_page(struct block_allocator * a,
					struct ba_page_header * h,
					struct ba_page ** pp,
					const struct ba_block_header * ptr) {
    if (*pp) {
	struct ba_page * p = *pp;

	ba_update_slot(a, p, h);
    }

    *pp = ba_find_page(a, ptr);
    *h = (*pp)->h;
}

EXPORT void ba_get_free_page(struct block_allocator * a, const void * ptr) {
    ba_low_get_free_page(a, &a->hf, &a->last_free,
			 (struct ba_block_header *)ptr);
}

EXPORT void ba_local_get_free_page(struct ba_local * a, const void * ptr) {
    ba_low_get_free_page(a->a, &a->hf, &a->last_free,
			   (struct ba_block_header *)ptr);
}

static INLINE
struct ba_page * ba_htable_linear_lookup(const struct block_allocator * a,
					 const void * ptr) {
    PAGE_LOOP(a, {
	if (BA_CHECK_PTR(a->l, p, ptr)) {
	    return p;
	}
    });
    return NULL;
}

EXPORT struct ba_page * ba_find_page(const struct block_allocator * a,
				     const void * ptr) {
    struct ba_page * p;
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_find_page", __FILE__, __LINE__);
#endif
#ifdef BA_HASH_THLD
    if (a->num_pages <= BA_HASH_THLD) {
	INC(find_linear);
	p = ba_htable_linear_lookup(a, ptr);
    } else {
#endif
	INC(find_hash);
	p = ba_htable_lookup(a, ptr);
#ifdef BA_HASH_THLD
    }
#endif
    if (p) return p;
    
#ifdef BA_DEBUG
    fprintf(stderr, "magnitude: %u\n", a->magnitude);
    fprintf(stderr, "allocated: %u\n", a->allocated);
    fprintf(stderr, "did not find %p (%X[%X] | %X[%X])\n", ptr,
	    hash1(a, ptr), hash1(a, ptr) & BA_HASH_MASK(a),
	    hash2(a, ptr), hash2(a, ptr) & BA_HASH_MASK(a)
	    );
    ba_print_htable(a);
#endif
    BA_ERROR(a, "Unknown pointer (not found in hash) %lx\n", (long int)ptr);
}

EXPORT void ba_remove_page(struct block_allocator * a) {
    struct ba_page * t, * p;

    p = t = a->pages[2];

    /* I am not sure if this really helps. It should, by removing the
     * page which is highest in memory, potentially help malloc lower
     * the break.
     */
    do {
	if (t > p) p = t;

	t = t->next;
    } while (t);

    DOUBLE_UNLINK(a->pages[2], p);

#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_remove_page", __FILE__, __LINE__);
    if (a->empty_pages < a->max_empty_pages) {
	BA_ERROR(a, "strange things happening\n");
    }
    if (p == a->pages[0] || p == a->pages[1]) {
	BA_ERROR(a, "removing first page. this is not supposed to happen\n");
    }
#endif

#if 0/*def BA_DEBUG*/
    fprintf(stderr, "> ===============\n");
    ba_show_pages(a);
    ba_print_htable(a);
    fprintf(stderr, "removing page %4u[%p]\t(%p .. %p) -> %X (%X) (of %u).\n", 
	    n, p, p+1, BA_LASTBLOCK(a->l, p), hash1(a, BA_LASTBLOCK(a->l,p)),
	    hash1(a, BA_LASTBLOCK(a->l,p)) & BA_HASH_MASK(a),
	    a->num_pages);
#endif

    ba_htable_delete(a, p);

#ifdef BA_DEBUG
    memset(p, 0, sizeof(struct ba_page));
#endif

    a->empty_pages--;
    a->num_pages--;
    free(p);

#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_remove_page", __FILE__, __LINE__);
#endif
    if (a->allocated > BA_ALLOC_INITIAL
	&& a->num_pages <= (a->allocated >> 2)) {
	ba_htable_shrink(a);
    }
}

/*
 * Here come local allocator definitions
 */

EXPORT size_t ba_lcount(struct ba_local * a) {
    if (a->a) {
	if (a->page) a->page->h = a->h;
	if (a->last_free) a->last_free->h = a->hf;
	return ba_count(a->a);
    } else {
	return (size_t)(a->page ? a->h.used : 0);
    }
}

EXPORT void ba_init_local(struct ba_local * a, uint32_t block_size,
			  uint32_t blocks, uint32_t max_blocks,
			  ba_simple simple,
			  void * data) {

    if (block_size < sizeof(struct ba_block_header)) {
	block_size = sizeof(struct ba_block_header);
    }

    if (!blocks) blocks = 16;
    if (!max_blocks) max_blocks = 512;

    if (blocks > max_blocks) {
	ba_error("foo");
    }

    if (!simple) {
	ba_error("simple relocation mechanism needs to be implemented");
    }

    ba_init_layout(&a->l, block_size, blocks);
    a->h.first = NULL;
    a->h.used = 0;
    a->h.flags = 0;
    a->page = NULL;
    a->max_blocks = max_blocks;
    a->a = NULL;
    a->rel.simple = simple;
    a->rel.data = data;
    a->last_free = NULL;
#ifdef BA_USE_VALGRIND
    VALGRIND_CREATE_MEMPOOL(a, 0, 0);
#endif
}

static struct ba_page * ba_local_grow_page(struct ba_local * a,
					   struct ba_page * p,
					   const struct ba_layout * l) {
    struct ba_page_header * h = &a->h;
    const struct ba_layout * ol = &a->l;
    const struct ba_relocation * rel = &a->rel;

    struct ba_block_header ** t;
    ptrdiff_t diff;
    struct ba_page * n;

    n = (struct ba_page*)BA_XREALLOC(p, BA_PAGESIZE(*l));

    if (!p) {
	ba_free_page(l, n, NULL);
	*h = n->h;
	return n;
    }

    diff = (char*)n - (char*)p;

    t = &h->first;

    if (diff) {
	/*
	 * relocate all free list pointers and append new blocks at the end
	 */
	while (*t > BA_ONE) {
	    ba_simple_rel_pointer((char*)t, diff);
	    t = &((*t)->next);
#ifdef BA_USE_VALGRIND
	    VALGRIND_MAKE_MEM_DEFINED(*t, sizeof(void*));
#endif
	}
	if (*t == NULL) {
	    *t = (struct ba_block_header*)((char*)n + BA_PAGESIZE(*ol));
	    (*t)->next = BA_ONE;
	}

	BA_WALK_CHUNKS(n, h, l, {
#ifdef BA_USE_VALGRIND
	    char * temp = _start;
	    /* protect free list during relocation.  */
	    ba_list_noaccess(n, h->first, l);
#endif
	    rel->simple(_start, _stop, diff, rel->data);
#ifdef BA_USE_VALGRIND
	    ba_list_defined(n, h->first, l);
#endif

#ifdef BA_USE_VALGRIND
	    /* ol is in principle different from the allocator, however
	     * as long as we keep it as the first item in the struct,
	     * its fine to use it */
	    while (temp < _stop) {
		VALGRIND_MEMPOOL_CHANGE(ol, temp-diff, temp, l->block_size);
		temp += l->block_size;
	    }
#endif
	});
    } else {
#ifdef BA_USE_VALGRIND
	ba_list_defined(n, h->first, l);
#endif
	while (*t > BA_ONE) {
	    t = &((*t)->next);
	}
	if (*t == NULL) {
	    *t = (struct ba_block_header*)((char*)n + BA_PAGESIZE(*ol));
	    (*t)->next = BA_ONE;
	}
#ifdef BA_USE_VALGRIND
	ba_list_noaccess(n, h->first, l);
#endif
    }

    return n;
}

EXPORT INLINE void ba_local_grow(struct ba_local * a,
				 const uint32_t blocks) {
    struct ba_layout l;
    int transform = 0;

    if (blocks >= a->max_blocks) {
	ba_init_layout(&l, a->l.block_size, a->max_blocks);
	ba_align_layout(&l);
	transform = 1;
    } else {
	ba_init_layout(&l, a->l.block_size, blocks);
    }

    a->page = ba_local_grow_page(a, a->page, &l);
    a->l = l;

    if (transform) {
#ifdef BA_DEBUG
	fprintf(stderr, "transforming into allocator\n");
#endif
	a->a = (struct block_allocator *)
		BA_XALLOC(sizeof(struct block_allocator));
	ba_init(a->a, l.block_size, l.blocks);
	/* transform here! */
	a->page->next = a->page->prev = NULL;
	a->a->num_pages ++;
	//a->a->first = a->page;
	ba_htable_insert(a->a, a->page);
    }
}


EXPORT void ba_local_get_page(struct ba_local * a) {
    if (a->a) {
	/* get page from allocator */
	a->page = ba_get_page(a->a, a->page);
	if (a->page == a->last_free) {
	    ba_update_slot(a->a, a->last_free, &a->hf);
	    a->last_free = NULL;
	}
	a->h = a->page->h;
    } else {
	if (a->page) {
	    /* double the size */
	    ba_local_grow(a, a->l.blocks*2);
	} else {
	    ba_init_layout(&a->l, a->l.block_size, a->l.blocks);
	    a->page = (struct ba_page *)BA_XALLOC(BA_PAGESIZE(a->l));
	    ba_free_page(&a->l, a->page, NULL);
	    a->h = a->page->h;
	}
    }
}

EXPORT void ba_ldestroy(struct ba_local * a) {
    if (a->a) {
	ba_destroy(a->a);
	free(a->a);
	a->a = NULL;
    } else {
	free(a->page);
    }
    a->page = NULL;
    a->h.first = NULL;
#ifdef BA_USE_VALGRIND
    VALGRIND_DESTROY_MEMPOOL(a);
#endif
}

EXPORT void ba_lfree_all(struct ba_local * a) {
    if (a->a) {
	ba_free_all(a->a);
    } else {
	/* could be only in a->h */
	ba_free_page(&a->l, a->page, NULL);
	a->h = a->page->h;
    }
}

static INLINE int ba_walk_page(const struct ba_layout * l,
				struct ba_page * p, ba_walk_callback callback,
				void * data) {
    int ret = BA_CONTINUE;

#define BA_CALLBACK(a, b, c)	do {	\
    ba_list_noaccess(p, p->h.first, l);	\
    ret = callback(a, b, c);		\
    ba_list_defined(p, p->h.first, l);	\
} while (0)

    BA_WALK_CHUNKS(p, &p->h, l, {
	BA_CALLBACK(_start, _stop, data);
	if (ret == BA_STOP) break;
    });
#undef BA_CALLBACK
    return ret;
}

EXPORT void ba_walk(struct block_allocator * a, ba_walk_callback callback,
		    void * data) {
    if (a->alloc)
	a->alloc->h = a->h;

    if (a->last_free) ba_update_slot(a, a->last_free, &a->hf);

    PAGE_LOOP(a, {
	if (p->h.used) {
	    if (BA_STOP == ba_walk_page(&a->l, p, callback, data)) return;
	}
    });

    if (a->last_free)
	a->hf = a->last_free->h;
    if (a->alloc)
	a->h = a->alloc->h;
}

EXPORT void ba_walk_local(struct ba_local * a, ba_walk_callback callback,
			  void * data) {
    if (a->page)
	a->page->h = a->h;
    if (!a->a) {
	if (a->page) {
	    ba_walk_page(&a->l, a->page, callback, data);
	}
    } else {
	if (a->last_free) ba_update_slot(a->a, a->last_free, &a->hf);
	ba_walk(a->a, callback, data);
	if (a->last_free) a->hf = a->last_free->h;
    }
    if (a->page)
	a->h = a->page->h;
}


struct ba_block_header * ba_sort_list(const struct ba_page * p,
				      struct ba_block_header * b,
				      const struct ba_layout * l) {
    struct bitvector v;
    size_t i, j;
    struct ba_block_header ** t = &b;

    ba_list_defined(p, b, l);

#ifdef BA_DEBUG
    fprintf(stderr, "sorting max %llu blocks\n",
	    (unsigned long long)l->blocks);
#endif
    v.length = l->blocks;
    i = bv_byte_length(&v);
    /* we should probably reuse an area for this.
     */
    bv_set_vector(&v, BA_XALLOC(i));
    memset(v.v, 0, i);

    /*
     * store the position of all blocks in a bitmask
     */
    while (b) {
	uintptr_t n = BA_NBLOCK(*l, p, b);
#ifdef BA_DEBUG
	//fprintf(stderr, "block %llu is free\n", (long long unsigned)n);
#endif
	bv_set(&v, n, 1);
	if (b->next == BA_ONE) {
	    v.length = n+1;
	    break;
	} else b = b->next;
    }

#ifdef BA_DEBUG
    //bv_print(&v);
#endif

    /*
     * Handle consecutive free blocks in the end, those
     * we dont need anyway.
     */
    if (v.length) {
	i = v.length-1;
	while (i && bv_get(&v, i)) { i--; }
	v.length = i+1;
    }

#ifdef BA_DEBUG
    //bv_print(&v);
#endif

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

    free(v.v);

    ba_list_noaccess(p, b, l);

    return b;
}

EXPORT void ba_defragment_page(const struct ba_layout *l,
			       struct ba_page_header * h,
			       struct ba_page * p,
			       ba_relocation_callback relocate) {

    ba_sort(p, h, l);
    /* TODO */
}

/*
 * merges page p2 into page p1.
 */
EXPORT void ba_merge_pages(const struct ba_layout *l,
			   struct ba_page * p1, struct ba_page * p2,
			   ba_relocation_callback relocate, void * data) {
    struct ba_page_header *h1 = &p1->h;
    struct ba_page_header *h2 = &p2->h;
    struct ba_block_header * free_block;

    ba_sort(p1, h1, l);

    free_block = h1->first;

    ba_list_defined(p1, free_block, l);

    if (h1->used + h2->used > l->blocks) {
	ba_error("Cannot merge %p and %p\n", p1, p2);
    }
#ifdef BA_USE_VALGRIND
# define BA_RELOCATE_CHUNK(dst, src, bytes) do {		    \
    char * tsrc = (char*)(src);					    \
    char * tdst = (char*)(dst);					    \
    ptrdiff_t offset;						    \
    for (offset = 0; offset < bytes; offset += l->block_size) {	    \
	VALGRIND_MEMPOOL_ALLOC(l, tdst+offset, l->block_size);	    \
    }								    \
    relocate(dst, src, bytes, data);				    \
    for (offset = 0; offset < bytes; offset += l->block_size) {	    \
	VALGRIND_MAKE_MEM_NOACCESS(tsrc+offset, l->block_size);	    \
	VALGRIND_MEMPOOL_FREE(l, tsrc+offset);			    \
    }								    \
} while(0)
#else
# define BA_RELOCATE_CHUNK(dst, src, bytes) do {		    \
    relocate(dst, src, bytes, data);				    \
} while(0)
#endif

    BA_WALK_CHUNKS(p2, h2, l, {
	char * start = (char*)_start;
	char * stop = (char*)_stop;
	ptrdiff_t bytes = stop - start;

	if (free_block->next == BA_ONE) {
in_row:
	    BA_RELOCATE_CHUNK(free_block, start, bytes);

	    free_block = (struct ba_block_header*)((char*)free_block + bytes);
	    free_block->next = BA_ONE;
	} else do {
	    struct ba_block_header * tmp = free_block;
	    size_t free_bytes = l->block_size;

	    while ((char*)tmp->next == (char*)tmp + l->block_size) {
		tmp = tmp->next;
		free_bytes += l->block_size;
		if (free_bytes == bytes) break;
	    }

	    if (tmp->next == BA_ONE) goto in_row;

	    tmp = tmp->next;
	    BA_RELOCATE_CHUNK(free_block, start, free_bytes);
	    start += free_bytes;
	    bytes -= free_bytes;
	    free_block = tmp;
	} while(bytes);
    });

    h1->used += h2->used;

    if (h1->used == l->blocks) {
	h1->first = NULL;
    } else {
	h1->first = free_block;
    }
    ba_list_noaccess(p1, free_block, l);
    ba_free_page(l, p2, NULL);
}

/*
 * the only thing we can do here is to clear empty pages
 */
EXPORT void ba_shrink(struct block_allocator * a) {
    struct ba_page * p = a->pages[2];
    if (p) {
	a->pages[2] = NULL;
	do {
	    struct ba_page * t = p->next;
	    ba_htable_delete(a, p);
	    free(p);
	    p = t;
	} while (p);

	a->num_pages -= a->empty_pages;
	a->empty_pages = 0;

	if (a->allocated > BA_ALLOC_INITIAL
	    && a->num_pages <= (a->allocated >> 2)) {
	    ba_htable_shrink(a);
	}
    }
}

EXPORT void ba_defragment(struct block_allocator * a, size_t capacity,
			  ba_relocation_callback relocate, void * data) {
    struct ba_page *p1, *p2;

    ba_shrink(a);

    if (ba_capacity(a) <= capacity) return;

    /*
     * we discard alloc, in order to use it for possible defragmentation,
     * too
     */
    if (a->alloc && ba_is_low(&a->l, &a->h)) {
	a->alloc->h = a->h;
	DOUBLE_LINK(a->pages[BA_SLOT_LOW], a->alloc);
	a->alloc = NULL;
	a->h.first = NULL;
    }

    if (a->last_free) {
	ba_update_slot(a, a->last_free, &a->hf);
	a->last_free = NULL;
    }

    /*
     * this merges all pages that are less than 50% full
     */
    while ((p1 = a->pages[BA_SLOT_LOW]) && (p2 = p1->next)) {
	ba_merge_pages(&a->l, p1, p2, relocate, data);

	/* p2 is empty now */
	DOUBLE_UNLINK(a->pages[BA_SLOT_LOW], p2);
	ba_htable_delete(a, p2);
	a->num_pages--;
	free(p2);

	/* move p1 to other slot */
	if (ba_is_high(&a->l, &p1->h)) {
	    DOUBLE_SHIFT(a->pages[BA_SLOT_LOW]);
	    DOUBLE_LINK(a->pages[BA_SLOT_HIGH], p1);
	}

	if (ba_capacity(a) <= capacity) return;
    }
}

EXPORT void ba_ldefragment(struct ba_local * a, size_t capacity,
			   ba_relocation_callback relocate, void *data) {
    if (a->a) {
	if (a->last_free) {
	    ba_update_slot(a->a, a->last_free, &a->hf);
	    a->last_free = NULL;
	}

	ba_defragment(a->a, capacity, relocate, data);
    } else {
	/* TODO: defragment the page and possibly shrink it */
    }
}

#ifdef __cplusplus
}
#endif
