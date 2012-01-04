#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "block_allocator.h"

#include "bitvector.h"

#if defined(PIKE_CORE) || defined(DYNAMIC_MODULE) || defined(STATIC_MODULE)
#include "pike_memory.h"
#endif

#ifndef PMOD_EXPORT
# warning WTF
#endif

#if defined(BA_DEBUG) || (!defined(PIKE_CORE) && !defined(DYNAMIC_MODULE) \
			  && !defined(STATIC_MODULES))
#endif
PMOD_EXPORT char errbuf[8192];

static INLINE void ba_htable_insert(const struct block_allocator * a,
				    const void * ptr, const ba_page_t n);

#define BA_NBLOCK(a, p, ptr)	((uintptr_t)((char*)ptr - (char)(p+1))/(a)->block_size)

#define BA_DIVIDE(a, b)	    ((a) / (b) + (!!((a) & ((b)-1))))
#define BA_PAGESIZE(a)	    ((a)->blocks * (a)->block_size) 
#define BA_SPAGE_SIZE(a)    (sizeof(struct ba_page) + (BA_DIVIDE((a)->blocks, sizeof(uintptr_t)*8) - 1)*sizeof(uintptr_t))
#define BA_HASH_MASK(a)  (((a->allocated)) - 1)

#ifdef BA_HASH_THLD
# define BA_BYTES(a)	( (sizeof(ba_page) + ((a->allocated > BA_HASH_THLD) ? sizeof(ba_page_t) : 0)) * ((a)->allocated) )
#else
# define BA_BYTES(a)	( (sizeof(ba_page) + sizeof(ba_page_t)) * ((a)->allocated) )
#endif

#ifndef PIKE_MEM_RW_RANGE
# define PIKE_MEM_RW_RANGE(x, y)
#endif

static INLINE void ba_realloc(struct block_allocator * a) {
    unsigned int i;
    ba_page * old;
    ba_page p;
    a->pages = (ba_page*)realloc(old = a->pages, BA_BYTES(a));
    
    if (unlikely(!a->pages)) {
	//a->pages = old;
	BA_ERROR("alloc of %lu bytes failed.", BA_BYTES(a));
    }

    //fprintf(stderr, "realloc to size %u\n", a->allocated);
		
#ifdef BA_DEBUG
    memset((void*)BA_PAGE(a, a->num_pages+1), 0, BA_BYTES(a) - sizeof(struct ba_page)*a->num_pages);
#endif
    a->htable = NULL;
    IF_HASH(
	a->htable = (ba_page_t*) (a->pages + a->allocated);
	memset(a->htable, 0, a->allocated * sizeof(ba_page_t));
	for (i = 0; i < a->num_pages; i++) {
	    p = BA_PAGE(a, i+1);
	    p->hchain = 0;
	    ba_htable_insert(a, BA_LASTBLOCK(a, p), i+1);
	}
    );
#ifdef BA_DEBUG
    ba_check_allocator(a, "realloc", __FILE__, __LINE__);
#endif
}

PMOD_EXPORT void ba_show_pages(struct block_allocator * a) {
    unsigned int i = 0;

    fprintf(stderr, "first: %p last_free: %p\n", a->first, a->last_free);
    for (i = 1; i <= a->num_pages; i++) {
	ba_page p = BA_PAGE(a, i);
	fprintf(stderr, "%d\t%f\t(%u %d) --> (prev: %p | next: %p)\n",
		i, p->blocks_used/(double)a->blocks * 100,
		p->blocks_used,
		p->blocks_used,
		p->prev, p->next);

    }
}

PMOD_EXPORT INLINE void ba_init(struct block_allocator * a,
			 uint32_t block_size, ba_page_t blocks) {
    uint32_t page_size = block_size * blocks;

    a->first = NULL;
    a->last_free = NULL;
    a->last_free_num = 0;

    if ((page_size & (page_size - 1)) == 0)
	a->magnitude = (uint16_t)ctz32(page_size); 
    else 
	a->magnitude = (uint16_t)ctz32(round_up32(page_size));

    a->block_size = block_size;
    a->blocks = (1 << a->magnitude) / block_size;
    a->num_pages = 0;
    a->empty_pages = 0;
    a->max_empty_pages = BA_MAX_EMPTY;

    // we start with management structures for 16 pages
    a->allocated = BA_ALLOC_INITIAL;
    a->pages = NULL;
    ba_realloc(a);
}

PMOD_EXPORT INLINE void ba_free_all(struct block_allocator * a) {
    unsigned int i;

    if (!a->allocated) return;

    for (i = 0; i < a->num_pages; i++) {
	ba_page p = BA_PAGE(a, i+1);
	PIKE_MEM_RW_RANGE(BA_BLOCKN(a, 0), BA_PAGESIZE(a));
	free(p);
	BA_PAGE(a, i+1) = NULL;
    }
    a->num_pages = 0;
    a->empty_pages = 0;
    a->first = NULL;
    a->last_free = NULL;
    a->allocated = BA_ALLOC_INITIAL;
    ba_realloc(a);
    IF_HASH(
	memset(a->htable, 0, sizeof(*a->htable)*a->allocated);
    );
}

PMOD_EXPORT INLINE void ba_count_all(struct block_allocator * a, size_t *num, size_t *size) {
    unsigned int i;
    size_t n = 0;

    //fprintf(stderr, "page_size: %u, pages: %u\n", BA_PAGESIZE(a), a->num_pages);
    *size = BA_BYTES(a) + a->num_pages * BA_PAGESIZE(a);
    for (i = 0; i < a->num_pages; i++) {
	n += BA_PAGE(a, i+1)->blocks_used;
    }

    *num = n;
}

PMOD_EXPORT INLINE void ba_destroy(struct block_allocator * a) {
    ba_free_all(a);
    PIKE_MEM_RW_RANGE(a->pages, BA_BYTES(a));
    free(a->pages);
    a->allocated = 0;
    a->htable = NULL;
    a->pages = NULL;
}

static INLINE void ba_grow(struct block_allocator * a) {
    if (a->allocated) {
	// try to detect 32bit overrun?
	if (a->allocated >= ((ba_page_t)1 << (sizeof(ba_page_t)*8-1))) {
	    Pike_error("too many pages.\n");
	}
	a->allocated *= 2;
	ba_realloc(a);
    } else {
	ba_init(a, a->block_size, a->blocks);
    }
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_grow", __FILE__, __LINE__);
#endif
}

static INLINE void ba_shrink(struct block_allocator * a) {
    a->allocated /= 2;
    ba_realloc(a);
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

#ifdef BA_DEBUG
PMOD_EXPORT void ba_print_htable(const struct block_allocator * a) {
    unsigned int i;
    ba_page p;

    fprintf(stderr, "allocated: %u\n", a->allocated);
    fprintf(stderr, "num_pages: %u\n", a->num_pages);
    fprintf(stderr, "max_empty_pages: %u\n", a->max_empty_pages);
    fprintf(stderr, "empty_pages: %u\n", a->empty_pages);
    fprintf(stderr, "magnitude: %u\n", a->magnitude);
    fprintf(stderr, "block_size: %u\n", a->block_size);

    IF_HASH(
	for (i = 0; i < a->allocated; i++) {
	    ba_page_t n = a->htable[i];
	    ba_page_t hval;
	    void * ptr;
	    while (n) {
		p = BA_PAGE(a, n);
		ptr = BA_LASTBLOCK(a, p);
		hval = hash1(a, ptr);
		fprintf(stderr, "%u\t%X[%d]\t%p-%p\t%X (page %d)\n", i, hval, hval & BA_HASH_MASK(a), p+1, ptr, (unsigned int)((uintptr_t)ptr >> a->magnitude), n);
		n = p->hchain;
	    }
	}
    );
}
#endif

/*
 * insert the pointer to an allocated page into the
 * hashtable. uses linear probing and open allocation.
 */
static INLINE void ba_htable_insert(const struct block_allocator * a,
				    const void * ptr, const ba_page_t n) {
    ba_page_t hval = hash1(a, ptr);
    ba_page_t * b = a->htable + (hval & BA_HASH_MASK(a));


#ifdef BA_DEBUG
    while (*b) {
	if (*b == n) {
	    fprintf(stderr, "inserting (%p, %d) twice\n", ptr, n);
	    fprintf(stderr, "is (%p, %d)\n", BA_PAGE(a, n)+1, n);
	    Pike_error("double insert.\n");
	    return;
	}
	b = &BA_PAGE(a, *b)->hchain;
    }
    b = a->htable + (hval & BA_HASH_MASK(a));
#endif

#ifdef BA_DEBUG
    fprintf(stderr, "replacing bucket %u with page %u by %u\n",
	    hval & BA_HASH_MASK(a), *b, n);
#endif

    BA_PAGE(a, n)->hchain = *b;
    *b = n;
}


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
    Pike_error("did not find index to replace.\n")
#endif
}

static INLINE void ba_htable_delete(const struct block_allocator * a,
				    const void * ptr, const ba_page_t n) {
    ba_page_t hval = hash1(a, ptr);
    ba_page_t * b = a->htable + (hval & BA_HASH_MASK(a));

    while (*b) {
	if (*b == n) {
	    *b = BA_PAGE(a, n)->hchain;
	    BA_PAGE(a, n)->hchain = 0;
	    return;
	}
	b = &BA_PAGE(a, *b)->hchain;
    }
#ifdef DEBUG
    ba_print_htable(a);
    fprintf(stderr, "ba_htable_delete(%p, %u)\n", ptr, n);
    Pike_error("did not find index to delete.\n")
#endif
}

static INLINE ba_page_t ba_htable_lookup(const struct block_allocator * a,
					const void * ptr) {
#ifdef COUNT
    count_name = "hash";
#endif
#ifdef BA_DEBUG
    int c = 0;
#endif
    ba_page_t n;
    ba_page p;
    n = a->htable[hash1(a, ptr) & BA_HASH_MASK(a)];
    while (n) {
	p = BA_PAGE(a, n);
	if (BA_CHECK_PTR(a, p, ptr)) {
	    INC(good);
	    return n;
	}
	n = p->hchain;
    }
    n = a->htable[hash2(a, ptr) & BA_HASH_MASK(a)];
    while (n) {
	p = BA_PAGE(a, n);
	if (BA_CHECK_PTR(a, p, ptr)) {
	    if (a->htable[hash1(a, ptr) & BA_HASH_MASK(a)])
		INC(ugly);
	    else INC(bad);
	    return n;
	}
#ifdef BA_DEBUG
	if (c++ > a->allocated) {
	    Pike_error("hash chain is infinite\n");
	}
#endif
	n = p->hchain;
    }

    return 0;
}

#ifdef BA_DEBUG
PMOD_EXPORT INLINE void ba_check_allocator(struct block_allocator * a,
				    char *fun, char *file, int line) {
    unsigned int i = 0;
    int bad = 0;
    ba_page p;

    if (a->empty_pages > a->num_pages) {
	fprintf(stderr, "too many empty pages.\n");
	bad = 1;
    }

    for (i = 1; i <= a->num_pages; i++) {
	ba_page_t hval;
	int found = 0;
	ba_page_t n;
	p = BA_PAGE(a, i);

	if (p->blocks_used == a->blocks) {
	    if (p->prev || p->next) {
		BA_ERROR("block is full but in list. next: %p prev: %p\n",
			p->next, p->prev);
		bad = 1;
	    }
	}

	IF_HASH(
	    hval = hash1(a, BA_LASTBLOCK(a, p));
	    n = a->htable[hval & BA_HASH_MASK(a)];
	    while (n) {
		if (n == i) {
		    found = 1;
		    break;
		}
		n = BA_PAGE(a, n)->hchain;
	    }

	    if (!found) {
		fprintf(stderr, "did not find page %d, %p (%u) in htable.\n",
			i, BA_LASTBLOCK(a, BA_PAGE(a, i)), hval);
		fprintf(stderr, "looking in bucket %u mask: %d\n", hval & BA_HASH_MASK(a), BA_HASH_MASK(a));
		bad = 1;
	    }

	    for (i = 0; i < a->allocated; i++) {
		n = a->htable[i];

		while (n) {
		    p = BA_PAGE(a, n);
		    hval = hash1(a, BA_LASTBLOCK(a, p));
		    if (i != (hval & BA_HASH_MASK(a))) {
			fprintf(stderr, "page %u found in wrong bucket %d\n",
				n, i);
			bad = 1;
		    }
		    n = p->hchain;
		}
	    }
	);
    }

    if (bad) {
	ba_print_htable(a);
	fprintf(stderr, "\nCalled from %s:%d:%s\n", fun, line, file);
	fprintf(stderr, "pages: %u\n", a->num_pages);
	Pike_error("bad");
    }
}
#endif


PMOD_EXPORT INLINE void * ba_low_alloc(struct block_allocator * a) {
    //fprintf(stderr, "ba_alloc(%p)\n", a);
    ba_page p;
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_alloc top", __FILE__, __LINE__);
#endif
    unsigned int i;

    //fprintf(stderr, "allocating new page. was %p\n", p);
    if (unlikely(a->num_pages == a->allocated)) {
	ba_grow(a);
    }
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_alloc after grow", __FILE__, __LINE__);
#endif

    a->num_pages++;
#ifdef BA_DEBUG
    if (BA_PAGE(a, a->num_pages)) {
	void * new = malloc(BA_PAGESIZE(a));
	fprintf(stderr, "reusing unfreed page %d, data: %p -> %p\n", a->num_pages,
		p+1, new);
	a->num_pages--;
	ba_show_pages(a);
	a->num_pages++;
	BA_PAGE(a, a->num_pages) = p = new;
    } else
#endif
    BA_PAGE(a, a->num_pages) = p = (ba_page)malloc(sizeof(struct ba_page) + BA_PAGESIZE(a));
    if (!p) {
	Pike_error("no mem. alloc returned zero.");
    }
    p->next = p->prev = NULL;
    a->first = p;
    IF_HASH(
	ba_htable_insert(a, BA_LASTBLOCK(a, p), a->num_pages);
#ifdef BA_DEBUG
	ba_check_allocator(a, "ba_alloc after insert", __FILE__, __LINE__);
#endif
    );
    p->blocks_used = 1;
    p->first = BA_BLOCKN(a, p, 1);

    for (i = 1; i+1 < a->blocks; i++) {
#ifdef BA_DEBUG
	PIKE_MEM_RW(BA_BLOCKN(a, p, i)->magic);
	BA_BLOCKN(a, p, i)->magic = BA_MARK_FREE;
#endif
	BA_BLOCKN(a, p, i)->next = BA_BLOCKN(a, p, i+1);
    }
    BA_LASTBLOCK(a, p)->next = NULL;
    //memset(p->data, 0x00, BA_PAGESIZE(a));
#ifdef BA_DEBUG
    PIKE_MEM_RW(BA_LASTBLOCK(a, p)->magic);
    BA_LASTBLOCK(a, p)->magic = BA_MARK_FREE;
    PIKE_MEM_RW(BA_BLOCKN(a, p, 0)->magic);
    BA_BLOCKN(a, p, 0)->magic = BA_MARK_ALLOC;
    ba_check_allocator(a, "ba_alloc after insert", __FILE__, __LINE__);
#endif
    return BA_BLOCKN(a, p, 0);
}

PMOD_EXPORT INLINE void ba_low_free(struct block_allocator * a,
				    void * ptr, ba_page p, ba_page_t n) {
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_low_free", __FILE__, __LINE__);
#endif
    if (!p) {
#ifdef BA_HASH_THLD
	if (a->allocated <= BA_HASH_THLD)
	    BA_ERROR("ba_low_free(.., %p) should never be called: %d, %d\n",
		     ptr, a->allocated, BA_HASH_THLD);
#endif
	n = ba_htable_lookup(a, ptr);
	if (n) {
	    a->last_free = p = BA_PAGE(a, n);
	    a->last_free_num = n;
	} else {
#ifdef BA_DEBUG
	    fprintf(stderr, "magnitude: %u\n", a->magnitude);
	    fprintf(stderr, "allocated: %u\n", a->allocated);
	    fprintf(stderr, "did not find %p (%X[%X] | %X[%X])\n", ptr,
		    hash1(a, ptr), hash1(a, ptr) & BA_HASH_MASK(a),
		    hash2(a, ptr), hash2(a, ptr) & BA_HASH_MASK(a)
		    );
	    ba_print_htable(a);
#endif
	    BA_ERROR("Unknown pointer (not found in hash) %p\n", ptr);
	}
    }

    if (p->blocks_used == a->blocks) {
	if (a->first) {
	    a->first->prev = p;
	}
	p->next = a->first;
	p->prev = NULL;
	a->first = p;
	p->first = NULL;
    } else if (p->blocks_used == 1 && a->empty_pages > a->max_empty_pages) {
	a->empty_pages--;
	ba_remove_page(a, n);
	return;
    }

#ifdef BA_DEBUG
    if ((int)p->blocks_used <= 0) {
	fprintf(stderr, "MEGAFAIL in page %u at %p: %d %u\n",
					  1 + (a->pages - p),
					  p,
					  p->blocks_used,
					  p->blocks_used);
	ba_show_pages(a);
    }
#endif
    p->blocks_used --;
#ifdef BA_DEBUG
    if (((ba_block_header)ptr)->magic == BA_MARK_FREE) {
	fprintf(stderr, "double freed somethign\n");
    }
    //memset(ptr, 0x75, a->block_size);
    ((ba_block_header)ptr)->magic = BA_MARK_FREE;
#endif
    ((ba_block_header)ptr)->next = p->first;
    //fprintf(stderr, "setting first to %u (%p vs %p) n: %u vs %u\n", t+1, BA_BLOCKN(a, p, t+1), ptr, BA_NBLOCK(a, p, BA_BLOCKN(a, p, t+1)), BA_NBLOCK(a, p, ptr));
    p->first = ptr;
}


PMOD_EXPORT INLINE void ba_remove_page(struct block_allocator * a,
				       ba_page_t n) {
    ba_page tmp, p;
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_remove_page", __FILE__, __LINE__);
    if (a->empty_pages < a->max_empty_pages) {
	Pike_error("strange things happening\n");
    }
#endif

    p = BA_PAGE(a, n);

#ifdef BA_DEBUG
    fprintf(stderr, "> ===============\n");
    ba_show_pages(a);
    ba_print_htable(a);
    fprintf(stderr, "removing page %4u[%p]\t(%p .. %p) -> %X (%X) (of %u).\n", n, p,
	    p+1, BA_LASTBLOCK(a, p), hash1(a, BA_LASTBLOCK(a,p)),
	    hash1(a, BA_LASTBLOCK(a,p)) & BA_HASH_MASK(a),
	    a->num_pages);
#endif

    IF_HASH(
	ba_htable_delete(a, BA_LASTBLOCK(a, p), n);
    );
    PIKE_MEM_RW_RANGE(*p, BA_PAGESIZE(a));

    if (a->last_free == p)
	a->last_free = NULL;

    if (p->prev) {
	p->prev->next = p->next;
    } else {
	if (p->next) {
	    a->first = p->next;
	    a->first->prev = NULL;
	} else a->first = NULL;
    }

    if (p->next) {
	p->next->prev = p->prev;
    }

    if (a->num_pages != n) {
	tmp = BA_PAGE(a, a->num_pages);
	// page tmp will have index changed to n 
#ifdef BA_DEBUG
	fprintf(stderr, "removing last from htable\n");
#endif
	IF_HASH(
	    ba_htable_delete(a, BA_LASTBLOCK(a, tmp), a->num_pages);
	);
#ifdef BA_DEBUG
	ba_print_htable(a);
	fprintf(stderr, "fooo\n");
	fprintf(stderr, "replacing with page %4u[%p]\t(%p .. %p) -> %X (%X) (of %u).\n", a->num_pages, tmp,
		tmp->data, BA_LASTBLOCK(a, tmp), hash1(a, BA_LASTBLOCK(a,tmp)),
		hash1(a, BA_LASTBLOCK(a,tmp)) & BA_HASH_MASK(a),
		a->num_pages);
	/*
	fprintf(stderr, "replacing with page %u\t(%p .. %p) -> %X (%X)\n",
		a->num_pages,
		tmp->data, BA_LASTBLOCK(a, tmp), hash1(a, BA_LASTBLOCK(a,tmp)),
		hash1(a, BA_LASTBLOCK(a,tmp)) & BA_HASH_MASK(a)
		);
		*/
#endif
	BA_PAGE(a, n) = BA_PAGE(a, a->num_pages);
	if (tmp == a->last_free) {
	    a->last_free_num = n;
	}
	IF_HASH(
	    // ba_htable_replace(a, BA_LASTBLOCK(a, tmp), a->num_pages, n);
	    ba_htable_insert(a, BA_LASTBLOCK(a, tmp), n);
	);
#ifdef BA_DEBUG
	ba_print_htable(a);
	fprintf(stderr, "fooo\n");
#endif
    }

#ifdef BA_DEBUG
    memset(BA_PAGE(a, a->num_pages), 0, sizeof(struct ba_page));
    if (a->first == BA_PAGE(a, a->num_pages)) {
	fprintf(stderr, "a->first will be old removed page %d\n", a->first);
	fprintf(stderr, "page %d was not moved and prev was %p\n", n, p->prev);
    }

#endif

    a->num_pages--;
    free(p);

#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_remove_page", __FILE__, __LINE__);
#endif
    if (a->allocated > BA_ALLOC_INITIAL && a->num_pages <= (a->allocated >> 2)) {
	ba_shrink(a);
    }
#ifdef BA_DEBUG
    ba_show_pages(a);
    ba_print_htable(a);
    fprintf(stderr, "< =================\n");
#endif
}
