#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>
#ifdef BA_STATS
# include <unistd.h>
# include <mcheck.h>
#endif

#include "bitvector.h"

#if defined(PIKE_CORE) || defined(DYNAMIC_MODULE) || defined(STATIC_MODULE)
#include "pike_memory.h"
#endif
#include "block_allocator.h"


#ifndef PMOD_EXPORT
# warning WTF
#endif

#if (!defined(PIKE_CORE) && !defined(DYNAMIC_MODULE) \
			  && !defined(STATIC_MODULES))
PMOD_EXPORT char errbuf[8192];
#endif

#ifndef BA_USE_MEMALIGN
static INLINE void ba_htable_insert(const struct block_allocator * a,
				    ba_page ptr);
static INLINE void ba_htable_grow(struct block_allocator * a);
#endif

#define BA_NBLOCK(a, p, ptr)	((uintptr_t)((char*)ptr - (char)(p+1))/(a)->block_size)

#define BA_DIVIDE(a, b)	    ((a) / (b) + (!!((a) & ((b)-1))))
#define BA_PAGESIZE(a)	    (sizeof(struct ba_page) + (a)->blocks * (a)->block_size) 
#define BA_HASH_MASK(a)  (((a->allocated)) - 1)

#ifdef BA_HASH_THLD
# define BA_BYTES(a)	( (sizeof(ba_page) + ((a->allocated > BA_HASH_THLD) ? sizeof(ba_page_t) : 0)) * ((a)->allocated) )
#else
# define BA_BYTES(a)	( (sizeof(ba_page) * ((a)->allocated) )
#endif

#define PRINT_NODE(a, name) do {\
    fprintf(stderr, #name": %p\n", a->name);\
} while (0)


PMOD_EXPORT void ba_show_pages(const struct block_allocator * a) {
    unsigned int i = 0;

#ifndef BA_USE_MEMALIGN
    fprintf(stderr, "allocated: %u\n", a->allocated);
#endif
    fprintf(stderr, "num_pages: %u\n", a->num_pages);
    fprintf(stderr, "max_empty_pages: %u\n", a->max_empty_pages);
    fprintf(stderr, "empty_pages: %u\n", a->empty_pages);
    fprintf(stderr, "magnitude: %u\n", a->magnitude);
    PRINT_NODE(a, empty);
    PRINT_NODE(a, first);
#ifdef BA_USE_MEMALIGN
    PRINT_NODE(a, full);
#else
    PRINT_NODE(a, last_free);
#endif

    PAGE_LOOP(a, {
	ba_block_t blocks_used;
	blocks_used = p->used;
	fprintf(stderr, "(%p)\t%f\t(%u %d) --> (prev: %p | next: %p)\n",
		p, blocks_used/(double)a->blocks * 100,
		blocks_used,
		blocks_used,
		p->prev, p->next);
    });
}

#ifdef BA_STATS
# define PRCOUNT(X)	fprintf(stat_file, #X " %5.1lf %% ", (a->stats.X/all)*100);
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
PMOD_EXPORT void ba_print_stats(struct block_allocator * a) {
    FILE *f;
    int i;
    double all;
    struct block_alloc_stats * s = &a->stats;
    size_t used = s->st_max * a->block_size;
    size_t malloced = s->st_max_pages * (a->block_size * a->blocks);
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
	   a->block_size * a->blocks,
	   a->block_size
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

//#define BA_ALIGNMENT	8

PMOD_EXPORT INLINE void ba_init(struct block_allocator * a,
			 uint32_t block_size, ba_page_t blocks) {
    uint32_t page_size;

#ifdef BA_ALIGNMENT
    if (block_size & (BA_ALIGNMENT - 1))
	block_size += (BA_ALIGNMENT - (block_size & (BA_ALIGNMENT - 1)));
#endif

    page_size = block_size * blocks + BLOCK_HEADER_SIZE;

    a->empty = a->first = NULL;
#ifdef BA_USE_MEMALIGN
    a->full = NULL;
#else
    a->last_free = NULL;
#endif

#ifdef BA_DEBUG
    fprintf(stderr, "blocks: %u block_size: %u page_size: %u\n",
	    blocks, block_size, page_size);
#endif

    // is not multiple of memory page size
    if ((page_size & (page_size - 1))) {
	page_size = round_up32(page_size);
    }
    a->blocks = (page_size - BLOCK_HEADER_SIZE)/block_size;
    a->offset = sizeof(struct ba_page) + (a->blocks - 1) * block_size;
#ifdef BA_DEBUG
    fprintf(stderr, "blocks: %u block_size: %u page_size: %u mallocing size: %lu, next pages: %u\n",
	    a->blocks, block_size, blocks * block_size,
	    BA_PAGESIZE(a)+sizeof(struct ba_page),
	    round_up32(BA_PAGESIZE(a)+sizeof(struct ba_page)));
#endif


    a->magnitude = (uint16_t)ctz32(page_size); 
    a->block_size = block_size;
    a->num_pages = 0;
    a->empty_pages = 0;
    a->max_empty_pages = BA_MAX_EMPTY;

    // we start with management structures for 16 pages
#ifndef BA_USE_MEMALIGN
    a->allocated = BA_ALLOC_INITIAL;
    a->pages = (ba_page*)malloc(BA_ALLOC_INITIAL * sizeof(ba_page));
    if (!a->pages) Pike_error("nomem");
    memset(a->pages, 0, BA_ALLOC_INITIAL * sizeof(ba_page));
#endif
}

static INLINE void ba_free_page(struct block_allocator * a, ba_page p) {
    p->first = BA_BLOCKN(a, p, 0);
    p->used = 0;
    if (a->blueprint) {
	//xmemset(p+1, a->blueprint, a->block_size, a->blocks);
	size_t len = (a->blocks - 1) * a->block_size, clen = a->block_size;
	p++;
	memcpy(p, a->blueprint, a->block_size);

	while (len > clen) {
	    memcpy(((char*)(p)) + clen, p, clen);
	    len -= clen;
	    clen <<= 1;
	}

	if (len) memcpy(((char*)(p)) + clen, p, len);
	p--;
    }

#ifdef BA_CHAIN_PAGE
    do {
	char * ptr = (char*)(p+1);
	
	while (ptr < BA_LASTBLOCK(a, p)) {
#ifdef BA_DEBUG
	    PIKE_MEM_RW(((ba_block_header)ptr)->magic);
	    ((ba_block_header)ptr)->magic = BA_MARK_FREE;
#endif
	    ((ba_block_header)ptr)->next = (ba_block_header)(ptr+a->block_size);
	    ptr+=a->block_size;
	}
	BA_LASTBLOCK(a, p)->next = NULL;
    } while (0);
#else
    p->first->next = BA_ONE;
#endif
}

PMOD_EXPORT INLINE void ba_free_all(struct block_allocator * a) {
    unsigned int i;

#ifdef BA_USE_MEMALIGN
    if (!a->num_pages) return;
#else
    if (!a->allocated) return;
#endif

    PAGE_LOOP(a, {
	PIKE_MEM_RW_RANGE(p, BA_PAGESIZE(a));
	free(p);
    });

    a->num_pages = 0;
    a->empty_pages = 0;
    a->empty = a->first = NULL;
#ifdef BA_USE_MEMALIGN
    a->full = NULL;
#else
    a->last_free = NULL;
    a->allocated = BA_ALLOC_INITIAL;
    memset(a->pages, 0, a->allocated * sizeof(ba_page));
#endif
}

PMOD_EXPORT INLINE void ba_count_all(struct block_allocator * a, size_t *num, size_t *size) {
    unsigned int i;
    size_t n = 0;

    //fprintf(stderr, "page_size: %u, pages: %u\n", BA_PAGESIZE(a), a->num_pages);
#ifdef BA_USE_MEMALIGN
    *size = a->num_pages * BA_PAGESIZE(a);
#else
    *size = BA_BYTES(a) + a->num_pages * BA_PAGESIZE(a);
#endif
    PAGE_LOOP(a, {
	n += p->used;
    });

    *num = n;
}

PMOD_EXPORT INLINE void ba_destroy(struct block_allocator * a) {
    unsigned int i;

    PAGE_LOOP(a, {
	PIKE_MEM_RW_RANGE(p, BA_PAGESIZE(a));
	free(p);
    });

#ifdef BA_USE_MEMALIGN
    a->full = NULL;
#else
    PIKE_MEM_RW_RANGE(a->pages, BA_BYTES(a));
    free(a->pages);
    a->last_free = NULL;
    a->allocated = 0;
    a->pages = NULL;
#endif
    a->empty = a->first = NULL;
    a->empty_pages = 0;
    a->num_pages = 0;
#ifdef BA_STATS
    a->stats.st_max = a->stats.st_used = 0;
#endif
}

#ifndef BA_USE_MEMALIGN
static INLINE void ba_grow(struct block_allocator * a) {
    if (a->allocated) {
	// try to detect 32bit overrun?
	if (a->allocated >= ((ba_page_t)1 << (sizeof(ba_page_t)*8-1))) {
	    BA_ERROR("too many pages.\n");
	}
	ba_htable_grow(a);
    } else {
	ba_init(a, a->block_size, a->blocks);
    }
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_grow", __FILE__, __LINE__);
#endif
}

#define MIX(t)	do {		\
    t ^= (t >> 20) ^ (t >> 12);	\
    t ^= (t >> 7) ^ (t >> 4);	\
} while(0)

static INLINE ba_page_t hash1(const struct block_allocator * a,
			     const void * ptr) {
    uintptr_t t = ((uintptr_t)ptr) >> a->magnitude;

    MIX(t);

    return (ba_page_t) t;
}

static INLINE ba_page_t hash2(const struct block_allocator * a,
			     const void * ptr) {
    uintptr_t t = ((uintptr_t)ptr) >> a->magnitude;

    t++;
    MIX(t);

    return (ba_page_t) t;
}

static INLINE void ba_htable_grow(struct block_allocator * a) {
    ba_page_t n, old;
    old = a->allocated;
    a->allocated *= 2;
    a->pages = (ba_page*)realloc(a->pages, a->allocated * sizeof(ba_page));
    if (!a->pages) Pike_error("nomem");
    memset(a->pages+old, 0, old * sizeof(ba_page));
    for (n = 0; n < old; n++) {
	ba_page * b = a->pages + n;

	while (*b) {
	    ba_page_t h = hash1(a, BA_LASTBLOCK(a, *b)) & BA_HASH_MASK(a);
	    if (h != n) {
		ba_page p = *b;
		*b = p->hchain;
		p->hchain = a->pages[h];
		a->pages[h] = p;
		continue;
	    }
	    b = &(*b)->hchain;
	}
    }
}

static INLINE void ba_htable_shrink(struct block_allocator * a) {
    ba_page_t n;

    a->allocated /= 2;

    for (n = 0; n < a->allocated; n++) {
	ba_page p = a->pages[a->allocated + n];

	if (p) {
	    ba_page t = p;
	    if (a->pages[n]) {
		while (t->hchain) t = t->hchain;
		t->hchain = a->pages[n];
	    }
	    a->pages[n] = p;
	}
    }

    a->pages = (ba_page*)realloc(a->pages, a->allocated * sizeof(ba_page));
    if (!a->pages) Pike_error("nomem");
}

#ifdef BA_DEBUG
PMOD_EXPORT void ba_print_htable(const struct block_allocator * a) {
    unsigned int i;

    for (i = 0; i < a->allocated; i++) {
	ba_page p = a->pages[i];
	ba_page_t hval;
	if (!p) {
	    fprintf(stderr, "empty %u\n", i);
	}
	while (p) {
	    void * ptr = BA_LASTBLOCK(a, p);
	    hval = hash1(a, ptr);
	    fprintf(stderr, "%u\t%X[%u]\t%p-%p\t%X\n", i, hval, hval & BA_HASH_MASK(a), p+1, ptr, (unsigned int)((uintptr_t)ptr >> a->magnitude));
	    p = p->hchain;
	}
    }
}
#endif

/*
 * insert the pointer to an allocated page into the
 * hashtable. uses linear probing and open allocation.
 */
static INLINE void ba_htable_insert(const struct block_allocator * a,
				    ba_page p) {
    ba_page_t hval = hash1(a, BA_LASTBLOCK(a, p));
    ba_page * b = a->pages + (hval & BA_HASH_MASK(a));


#ifdef BA_DEBUG
    while (*b) {
	if (*b == p) {
	    fprintf(stderr, "inserting (%p) twice\n", p);
	    fprintf(stderr, "is (%p)\n", *b);
	    BA_ERROR("double insert.\n");
	    return;
	}
	b = &(*b)->hchain;
    }
    b = a->pages + (hval & BA_HASH_MASK(a));
#endif

#if 0//def BA_DEBUG
    fprintf(stderr, "replacing bucket %u with page %u by %u\n",
	    hval & BA_HASH_MASK(a), *b, n);
#endif

    p->hchain = *b;
    *b = p;
}


#if 0
static INLINE void ba_htable_replace(const struct block_allocator * a, 
				     const void * ptr, const ba_page_t n,
				     const ba_page_t new) {
    ba_page_t hval = hash1(a, ptr);
    ba_page_t * b = a->htable + (hval & BA_HASH_MASK(a));

    while (*b) {
	if (*b == n) {
	    *b = new;
	    BA_PAGE(a, new)->hchain = BA_PAGE(a, n)->hchain;
	    BA_PAGE(a, n)->hchain = 0;
	    return;
	}
	b = &BA_PAGE(a, *b)->hchain;
    }
#ifdef DEBUG

    fprintf(stderr, "ba_htable_replace(%p, %u, %u)\n", ptr, n, new);
    fprintf(stderr, "hval: %u, %u, %u\n", hval, hval & BA_HASH_MASK(a), BA_HASH_MASK(a));
    ba_print_htable(a);
    BA_ERROR("did not find index to replace.\n")
#endif
}
#endif

static INLINE void ba_htable_delete(const struct block_allocator * a,
				    ba_page p) {
    ba_page_t hval = hash1(a, BA_LASTBLOCK(a, p));
    ba_page * b = a->pages + (hval & BA_HASH_MASK(a));

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
    BA_ERROR("did not find index to delete.\n")
#endif
}

static INLINE void ba_htable_lookup(struct block_allocator * a,
				    const void * ptr) {
    ba_page p;
    ba_page_t h[2];
    unsigned char c, b = 0;
    h[0] = hash1(a, ptr);
    h[1] = hash2(a, ptr);

    c = ((uintptr_t)ptr >> (a->magnitude - 1)) & 1;

LOOKUP:
    p = a->pages[h[c] & BA_HASH_MASK(a)];
    while (p) {
	if (BA_CHECK_PTR(a, p, ptr)) {
	    a->last_free = p;
	    return;
	}
	p = p->hchain;
    }
    c = !c;
    if (!(b++)) goto LOOKUP;
}
#endif

#ifdef BA_DEBUG
PMOD_EXPORT INLINE void ba_check_allocator(struct block_allocator * a,
				    char *fun, char *file, int line) {
    ba_page_t n;
    int bad = 0;
    ba_page p;

    if (a->empty_pages > a->num_pages) {
	fprintf(stderr, "too many empty pages.\n");
	bad = 1;
    }

#ifndef BA_USE_MEMALIGN
    for (n = 0; n < a->allocated; n++) {
	ba_page_t hval;
	int found = 0;
	p = a->pages[n];

	while (p) {
	    if (!p->first && p != a->first) {
		if (p->prev || p->next) {
		    fprintf(stderr, "page %p is full but in list. next: %p"
			     "prev: %p\n",
			    p, p->next,
			    p->prev);
		    bad = 1;
		}
	    }

	    hval = hash1(a, BA_LASTBLOCK(a, p)) & BA_HASH_MASK(a);

	    if (hval != n) {
		fprintf(stderr, "page with hash %u found in bucket %u\n",
			hval, n);
		bad = 1;
	    }
	    p = p->hchain;
	}

	if (bad)
	    ba_print_htable(a);
    }
#endif

    if (bad) {
	ba_show_pages(a);
	fprintf(stderr, "\nCalled from %s:%d:%s\n", fun, line, file);
	fprintf(stderr, "pages: %u\n", a->num_pages);
	BA_ERROR("bad");
    }
}
#endif

static INLINE void ba_alloc_page(struct block_allocator * a) {
    ba_page p;
    unsigned int i;

#ifdef BA_USE_MEMALIGN
    if (a->num_pages == 0) ba_init(a, a->block_size, a->blocks);
#else
    if (unlikely(a->num_pages == a->allocated)) {
	ba_grow(a);
    }
#endif

    a->num_pages++;
#ifndef BA_USE_MEMALIGN
    p = (ba_page)malloc(BA_PAGESIZE(a));
#else
    p = (ba_page)memalign((size_t)(1 << a->magnitude), BA_PAGESIZE(a));
#endif
    if (!p) BA_ERROR("no mem. alloc returned zero.");
    ba_free_page(a, p);
    p->next = p->prev = NULL;
    a->first = p;
#ifndef BA_USE_MEMALIGN
    ba_htable_insert(a, p);
#endif
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_alloc after insert", __FILE__, __LINE__);
#endif
#ifdef BA_DEBUG
    PIKE_MEM_RW(BA_LASTBLOCK(a, p)->magic);
    BA_LASTBLOCK(a, p)->magic = BA_MARK_FREE;
    PIKE_MEM_RW(BA_BLOCKN(a, p, 0)->magic);
    BA_BLOCKN(a, p, 0)->magic = BA_MARK_ALLOC;
    ba_check_allocator(a, "ba_alloc after insert", __FILE__, __LINE__);
#endif
}

PMOD_EXPORT void ba_low_alloc(struct block_allocator * a) {
    //fprintf(stderr, "ba_alloc(%p)\n", a);
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_alloc top", __FILE__, __LINE__);
#endif

    //fprintf(stderr, "allocating new page. was %p\n", p);
    if (a->first) {
	ba_page p = a->first;
	DOUBLE_SHIFT(a->first);
#ifdef BA_USE_MEMALIGN
	DOUBLE_LINK(a->full, p);
#else
	p->next = NULL;
	p->first = NULL;
#endif
    }

    if (a->first) return;

    if (a->empty_pages) {
	a->first = a->empty;
	SINGLE_SHIFT(a->empty);
	a->empty_pages--;
	a->first->next = NULL;
    } else ba_alloc_page(a);

#ifdef BA_DEBUG
    if (!a->first->first) {
	ba_show_pages(a);
	BA_ERROR("a->first has no first block!\n");
    }
#endif

#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_alloc after grow", __FILE__, __LINE__);
#endif
}

PMOD_EXPORT void ba_low_free(struct block_allocator * a, ba_page p,
			     ba_block_header ptr) {
    // page was full
    if (unlikely(!ptr->next)) {
	INC(free_full);
#ifdef BA_USE_MEMALIGN
	DOUBLE_UNLINK(a->full, p);
#endif
	DOUBLE_LINK(a->first, p);
    } else if (!p->used) {
	INC(free_empty);
	if (a->empty_pages == a->max_empty_pages) {
	    ba_remove_page(a, p);
	    return;
	}
	DOUBLE_UNLINK(a->first, p);
	SINGLE_LINK(a->empty, p);
	a->empty_pages ++;
    }
}

#ifndef BA_USE_MEMALIGN
static INLINE void ba_htable_linear_lookup(struct block_allocator * a,
				    const void * ptr) {
    PAGE_LOOP(a, {
	if (BA_CHECK_PTR(a, p, ptr)) {
	    a->last_free = p;
	    return;
	}
    });
}

PMOD_EXPORT INLINE void ba_find_page(struct block_allocator * a,
				     const void * ptr) {
    a->last_free = NULL;
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_low_free", __FILE__, __LINE__);
#endif
#ifdef BA_HASH_THLD
    if (a->num_pages <= BA_HASH_THLD) {
	INC(find_linear);
	ba_htable_linear_lookup(a, ptr);
    } else {
#endif
	INC(find_hash);
	ba_htable_lookup(a, ptr);
#ifdef BA_HASH_THLD
    }

    if (a->last_free) return;
    
#endif
#ifdef BA_DEBUG
    fprintf(stderr, "magnitude: %u\n", a->magnitude);
    fprintf(stderr, "allocated: %u\n", a->allocated);
    fprintf(stderr, "did not find %p (%X[%X] | %X[%X])\n", ptr,
	    hash1(a, ptr), hash1(a, ptr) & BA_HASH_MASK(a),
	    hash2(a, ptr), hash2(a, ptr) & BA_HASH_MASK(a)
	    );
    ba_print_htable(a);
#endif
    BA_ERROR("Unknown pointer (not found in hash) %lx\n", (long int)ptr);
}
#endif

PMOD_EXPORT void ba_remove_page(struct block_allocator * a,
				       ba_page p) {
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_remove_page", __FILE__, __LINE__);
    if (a->empty_pages < a->max_empty_pages) {
	BA_ERROR("strange things happening\n");
    }
    if (p == a->first) {
	BA_ERROR("removing first page. this is not supposed to happen\n");
    }
#endif

#if 0//def BA_DEBUG
    fprintf(stderr, "> ===============\n");
    ba_show_pages(a);
    ba_print_htable(a);
    fprintf(stderr, "removing page %4u[%p]\t(%p .. %p) -> %X (%X) (of %u).\n", 
	    n, p, p+1, BA_LASTBLOCK(a, p), hash1(a, BA_LASTBLOCK(a,p)),
	    hash1(a, BA_LASTBLOCK(a,p)) & BA_HASH_MASK(a),
	    a->num_pages);
#endif

#ifndef BA_USE_MEMALIGN
    ba_htable_delete(a, p);
    PIKE_MEM_RW_RANGE(*p, BA_PAGESIZE(a));

    // we know that p == a->last_free
    a->last_free = NULL;
#endif

    DOUBLE_UNLINK(a->first, p);

#ifdef BA_DEBUG
    memset(p, 0, sizeof(struct ba_page));
    if (a->first == p) {
	fprintf(stderr, "a->first will be old removed page %p\n", a->first);
    }
#endif

    a->num_pages--;
    free(p);

#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_remove_page", __FILE__, __LINE__);
#endif
#ifndef BA_USE_MEMALIGN
    if (a->allocated > BA_ALLOC_INITIAL && a->num_pages <= (a->allocated >> 2)) {
	ba_htable_shrink(a);
    }
#endif
}
