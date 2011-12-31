#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "bitvector.h"
#include "pike_memory.h"

#include "block_allocator.h"

#ifndef PMOD_EXPORT
# warning WTF
#endif

static inline void ba_htable_insert(const struct block_allocator * a,
				    const void * ptr, const ba_page_t n);

#define BA_NBLOCK(a, p, ptr)	((uintptr_t)((char*)ptr - (p)->data)/(a)->block_size)

#define BA_DIVIDE(a, b)	    ((a) / (b) + (!!((a) & ((b)-1))))
#define BA_PAGESIZE(a)	    ((a)->blocks * (a)->block_size) 
#define BA_SPAGE_SIZE(a)    (sizeof(struct ba_page) + (BA_DIVIDE((a)->blocks, sizeof(uintptr_t)*8) - 1)*sizeof(uintptr_t))
#define BA_HASH_MASK(a)  (((a->allocated)) - 1)
#define BA_CHECK_PTR(a, p, ptr)	((char*)ptr >= p->data && (void*)BA_LASTBLOCK(a,p) >= ptr)

# define BA_BYTES(a)	( (sizeof(struct ba_page) + sizeof(ba_page_t)) * ((a)->allocated) )


static inline void ba_realloc(struct block_allocator * a) {
    unsigned int i;
    void *old;
    ba_page p;
    a->pages = realloc(old = a->pages, BA_BYTES(a));

    if (unlikely(!a->pages)) {
	a->pages = old;
	fprintf(stderr, "realloc(%lu) failed.\n", BA_BYTES(a));
	Pike_error("no mem");
    }

    //fprintf(stderr, "realloc to size %u\n", a->allocated);
		
#ifdef BA_DEBUG
    memset((void*)BA_PAGE(a, a->num_pages+1), 0, BA_BYTES(a) - sizeof(struct ba_page)*a->num_pages);
#endif
    IF_HASH(
	a->htable = (ba_page_t*) BA_PAGE(a, (a->allocated)+1);
#ifndef BA_DEBUG
	memset(a->htable, 0, a->allocated * sizeof(ba_page_t));
#endif
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

PMOD_EXPORT inline void ba_init(struct block_allocator * a,
			 uint32_t block_size, ba_page_t blocks) {
    uint32_t page_size = block_size * blocks;

    a->first =  0;
    a->last_free = 0;

    if ((page_size & (page_size - 1)) == 0)
	a->magnitude = (uint16_t)ctz32(page_size); 
    else 
	a->magnitude = (uint16_t)ctz32(round_up32(page_size));

    a->block_size = block_size;
    a->blocks = blocks;
    a->num_pages = 0;
    a->empty_pages = 0;
    a->max_empty_pages = 3;

    // we start with management structures for 16 pages
    a->allocated = BA_ALLOC_INITIAL;
    a->pages = NULL;
    ba_realloc(a);
}

PMOD_EXPORT inline void ba_free_all(struct block_allocator * a) {
    unsigned int i;

    for (i = 0; i < a->num_pages; i++) {
	PIKE_MEM_RW_RANGE(BA_PAGE(a, i+1)->data, BA_PAGESIZE(a));
	free(BA_PAGE(a, i+1)->data);
	BA_PAGE(a, i+1)->data = NULL;
    }
    IF_HASH(
	memset(a->htable, 0, a->allocated * sizeof(ba_page_t));
    );
    a->num_pages = 0;
    a->empty_pages = 0;
    a->first = 0;
}

PMOD_EXPORT inline void ba_count_all(struct block_allocator * a, size_t *num, size_t *size) {
    unsigned int i;
    size_t n = 0;

    //fprintf(stderr, "page_size: %u, pages: %u\n", BA_PAGESIZE(a), a->num_pages);
    *size = BA_BYTES(a) + a->num_pages * BA_PAGESIZE(a);
    for (i = 0; i < a->num_pages; i++) {
	n += BA_PAGE(a, i+1)->blocks_used;
    }

    *num = n;
}

PMOD_EXPORT inline void ba_destroy(struct block_allocator * a) {
    ba_free_all(a);
    PIKE_MEM_RW_RANGE(a->pages, BA_BYTES(a));
    free(a->pages);
    a->allocated = 0;
    a->pages = NULL;
}

static inline void ba_grow(struct block_allocator * a) {
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

static inline void ba_shrink(struct block_allocator * a) {
    a->allocated /= 2;
    ba_realloc(a);
}

#define MIX(t)	do {		\
    t ^= (t >> 20) ^ (t >> 12);	\
    t ^= (t >> 7) ^ (t >> 4);	\
} while(0)

static inline ba_page_t hash1(const struct block_allocator * a,
			     const void * ptr) {
    uintptr_t t = ((uintptr_t)ptr) >> a->magnitude;

    MIX(t);

    return (ba_page_t) t;
}

static inline ba_page_t hash2(const struct block_allocator * a,
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

    for (i = 0; i < a->allocated; i++) {
	ba_page_t n = a->htable[i];
	ba_page_t hval;
	void * ptr;
	while (n) {
	    p = BA_PAGE(a, n);
	    ptr = BA_LASTBLOCK(a, p);
	    hval = hash1(a, ptr);
	    fprintf(stderr, "%u\t%X\t%p-%p\t%X (page %d)\n", i, hval, p->data, ptr, (unsigned int)((uintptr_t)ptr >> a->magnitude), n);
	    n = p->hchain;
	}
    }
}
#endif

/*
 * insert the pointer to an allocated page into the
 * hashtable. uses linear probing and open allocation.
 */
static inline void ba_htable_insert(const struct block_allocator * a,
				    const void * ptr, const ba_page_t n) {
    ba_page_t hval = hash1(a, ptr);
    ba_page_t * b = a->htable + (hval & BA_HASH_MASK(a));


#ifdef BA_DEBUG
    while (*b) {
	if (*b == n) {
	    fprintf(stderr, "inserting (%p, %d) twice\n", ptr, n);
	    fprintf(stderr, "is (%p, %d)\n", BA_PAGE(a, n)->data, n);
	    Pike_error("double insert.\n");
	    return;
	}
	b = &BA_PAGE(a, *b)->hchain;
    }
    b = a->htable + (hval & BA_HASH_MASK(a));
#endif

    BA_PAGE(a, n)->hchain = *b;
    *b = n;
}


static inline void ba_htable_replace(const struct block_allocator * a, 
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

static inline void ba_htable_delete(const struct block_allocator * a,
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
    ba_print_htable(a);Pike_error
    fprintf(stderr, "ba_htable_delete(%p, %u)\n", ptr, n);
    Pike_error("did not find index to delete.\n")
#endif
}

static inline ba_page_t ba_htable_lookup(const struct block_allocator * a,
					const void * ptr) {
    int c = 0;
    ba_page_t n1, n2;
    ba_page p;
    n1 = a->htable[hash1(a, ptr) & BA_HASH_MASK(a)];
    while (n1) {
	p = BA_PAGE(a, n1);
	if (BA_CHECK_PTR(a, p, ptr)) {
	    return n1;
	}
	if (c++ > 100) {
	    n2 = a->htable[hash1(a, ptr) & BA_HASH_MASK(a)];
	    n1 = n2;
	    do {
		p = BA_PAGE(a, n1);
		fprintf(stderr, "%d %p -> %d\n", n1, p->data, p->hchain);
		if (n1 == p->hchain) break;
		n1 = p->hchain;
	    } while (n2 != n1);
	    Pike_error("hash chain is infinite\n");
	}
	n1 = p->hchain;
    }
    n2 = a->htable[hash2(a, ptr) & BA_HASH_MASK(a)];
    while (n2) {
	p = BA_PAGE(a, n2);
	if (BA_CHECK_PTR(a, p, ptr)) {
	    return n2;
	}
	if (c++ > 100) {
	    Pike_error("hash chain is infinite\n");
	}
	n2 = p->hchain;
    }

    return 0;
}

#ifdef BA_DEBUG
PMOD_EXPORT inline void ba_check_allocator(struct block_allocator * a,
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
		fprintf(stderr, "block is full but in list. next: %u prev: %u",
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


PMOD_EXPORT inline void * ba_low_alloc(struct block_allocator * a) {
    //fprintf(stderr, "ba_alloc(%p)\n", a);
    ba_page p;
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_alloc top", __FILE__, __LINE__);
#endif
    int i;

    //fprintf(stderr, "allocating new page. was %p\n", p);
    if (unlikely(a->num_pages == a->allocated)) {
	ba_grow(a);
    }
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_alloc after grow", __FILE__, __LINE__);
#endif

    a->num_pages++;
    p = BA_PAGE(a, a->num_pages);
#ifdef BA_DEBUG
    if (p->data) {
	void * new = malloc(BA_PAGESIZE(a));
	fprintf(stderr, "reusing unfreed page %d, data: %p -> %p\n", a->num_pages,
		p->data, new);
	p->data = new;
    } else
#endif
    p->data = malloc(BA_PAGESIZE(a));
    if (!p->data) {
	Pike_error("no mem. alloc returned zero.");
    }
    p->next = p->prev = 0;
    a->first = a->num_pages;
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
	BA_BLOCKN(a, p, i)->magic = BA_MARK_FREE;
#endif
	BA_BLOCKN(a, p, i)->next = BA_BLOCKN(a, p, i+1);
    }
    BA_LASTBLOCK(a, p)->next = NULL;
    //memset(p->data, 0x00, BA_PAGESIZE(a));
#ifdef BA_DEBUG
    BA_LASTBLOCK(a, p)->magic = BA_MARK_FREE;
    BA_BLOCKN(a, p, 0)->magic = BA_MARK_ALLOC;
    ba_check_allocator(a, "ba_alloc after insert", __FILE__, __LINE__);
#endif
    return p->data;
}

PMOD_EXPORT inline ba_page_t ba_find_page(struct block_allocator * a, void * ptr) {
    ba_page_t n = 0;
    ba_page p;

#ifdef BA_HASH_THLD
    if (a->allocated > BA_HASH_THLD) {
#endif
	n = ba_htable_lookup(a, ptr);
#ifdef BA_HASH_THLD
    } else {
	ba_page_t t;
	for (t = 1; t <= a->num_pages; t++) {
	    p = BA_PAGE(a, t);
	    if (BA_CHECK_PTR(a, p, ptr)) {
		a->last_free = n = t;
		break;
	    }
	}
    }
#endif

    if (unlikely(!n)) {
#ifdef BA_DEBUG
	fprintf(stderr, "magnitude: %u\n", a->magnitude);
	fprintf(stderr, "did not find %p (%X[%X] | %X[%X])\n", ptr,
		hash1(a, ptr), hash1(a, ptr) & BA_HASH_MASK(a),
		hash2(a, ptr), hash2(a, ptr) & BA_HASH_MASK(a)
		);
	ba_print_htable(a);
#endif
	Pike_error("Unknown pointer \n");
    }

#ifdef BA_DEBUG
    if (n > a->num_pages) {
	fprintf(stderr, "freeing from unknown page %d (num_pages: %d).\n",
		n, a->num_pages);
    }
#endif
    
    return n;
}


PMOD_EXPORT inline void ba_remove_page(struct block_allocator * a,
				  ba_page_t n) {
    ba_page tmp, p;
#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_remove_page", __FILE__, __LINE__);
    if (a->empty_pages < a->max_empty_pages) {
	Pike_error("strange things happening\n");
    }
#endif

    p = BA_PAGE(a, n);

    /*
    fprintf(stderr, "removing page %4u\t(%p .. %p) -> %X (%X) (of %u).\n", n,
	    p->data, BA_LASTBLOCK(a, p), hash1(a, BA_LASTBLOCK(a,p)),
	    hash1(a, BA_LASTBLOCK(a,p)) & BA_HASH_MASK(a),
	    a->num_pages);
	    */

    IF_HASH(
	ba_htable_delete(a, BA_LASTBLOCK(a, p), n);
    );
    PIKE_MEM_RW_RANGE(*p->data, BA_PAGESIZE(a));
    free(p->data);
    p->data = NULL;

    if (a->last_free == n)
	a->last_free = 0;
    else if (a->last_free == a->num_pages)
	a->last_free = n;

    if (p->prev) {
	tmp = BA_PAGE(a, p->prev);
	tmp->next = p->next;
    } else {
	a->first = p->next;
    }

    if (p->next) {
	tmp = BA_PAGE(a, p->next);
	tmp->prev = p->prev;
    }

    if (a->num_pages != n) {
	tmp = BA_PAGE(a, a->num_pages);
	// page tmp will have index changed to n 
	IF_HASH(
	    ba_htable_delete(a, BA_LASTBLOCK(a, tmp), a->num_pages);
	);
	/*
	fprintf(stderr, "replacing with page %u\t(%p .. %p) -> %X (%X)\n",
		a->num_pages,
		tmp->data, BA_LASTBLOCK(a, tmp), hash1(a, BA_LASTBLOCK(a,tmp)),
		hash1(a, BA_LASTBLOCK(a,tmp)) & BA_HASH_MASK(a)
		);
		*/
	*p = *tmp;
	IF_HASH(
	    // ba_htable_replace(a, BA_LASTBLOCK(a, tmp), a->num_pages, n);
	    ba_htable_insert(a, BA_LASTBLOCK(a, tmp), n);
	);

	if (p->next) BA_PAGE(a, p->next)->prev = n;
	if (p->prev) BA_PAGE(a, p->prev)->next = n;
	if (a->num_pages == a->first)
	    a->first = n;
    }
    //memset(BA_PAGE(a, a->num_pages), 0, sizeof(struct ba_page));

#ifdef BA_DEBUG
    if (a->first == a->num_pages) {
	fprintf(stderr, "a->first will be old removed page %d\n", a->first);
	fprintf(stderr, "page %d was not moved and prev was %d\n", n, p->prev);
    }

#endif

    a->num_pages--;

#ifdef BA_DEBUG
    ba_check_allocator(a, "ba_remove_page", __FILE__, __LINE__);
#endif
    if (a->allocated > BA_ALLOC_INITIAL && a->num_pages < (a->allocated >> 2)) {
	ba_shrink(a);
    }
}
