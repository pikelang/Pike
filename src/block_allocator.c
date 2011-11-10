#include <stdlib.h>
#include <string.h>

#include "bitvector.h"
//#include "pike_error.h"

struct ba_page {
    char * data;
    uint16_t next, prev;
    uint16_t free_mask;
    uintptr_t mask[1];
};

#include "block_allocator.h"

static inline void ba_htable_insert(const struct block_allocator * a,
				    const void * ptr, const uint16_t n);

#define BA_DIVIDE(a, b)	    ((a) / (b) + !!((a) & ((b)-1)))
#define BA_PAGESIZE(a)	    ((a)->blocks * (a)->block_size) 
#define BA_MASK_NUM(a)	    BA_DIVIDE((a)->blocks, sizeof(uintptr_t)*8)
#define BA_SPAGE_SIZE(a)    (sizeof(struct ba_page) + (BA_MASK_NUM(a) - 1)*sizeof(uintptr_t))
#define BA_HASH_MASK(a)  (((a->allocated * 2)) - 1)
#define BA_CHECK_PTR(a, p, ptr)	((char*)ptr >= p->data && ((uintptr_t)(p->data) + BA_PAGESIZE(a) > (uintptr_t)ptr))
#define BA_PAGE(a, n)   ((ba_page)((char*)a->pages + (n) * BA_SPAGE_SIZE(a)))
#define BA_LAST_BITS(a)	((a)->blocks & (sizeof(uintptr_t)*8 - 1))
#define BA_LAST_MASK(a)	(BA_LAST_BITS(a) ? TMASK(uintptr_t, BA_LAST_BITS(a)) : ~((uintptr_t)0))
#define BA_BYTES(a)	(( BA_SPAGE_SIZE(a) + 2 * sizeof(uint16_t)) * (a->allocated))
#define BA_BLOCKN(a, p, n) ((void*)(((char*)(p)->data) + (n)*((a)->block_size)))

#ifndef PIKE_ERROR_H
#include <stdio.h>
#include <unistd.h>

void Pike_error(char * msg) {
    fprintf(stderr, "ERROR: %s\n", msg);
    _exit(1);
}
#endif

static inline void ba_realloc(struct block_allocator * a) {
    int i;
    a->pages = realloc(a->pages, BA_BYTES(a));

    if (!a->pages) {
	Pike_error("no mem");
    }

    //fprintf(stderr, "realloc to size %u\n", a->allocated);
		
    memset((void*)BA_PAGE(a, a->num_pages), 0, BA_BYTES(a) - BA_SPAGE_SIZE(a)*a->num_pages);
    a->htable = (uint16_t*) BA_PAGE(a, (a->allocated));
    for (i = 0; i < a->num_pages; i++) {
	ba_htable_insert(a, (void*)((char*)(BA_PAGE(a, i)->data) + BA_PAGESIZE(a) - 1),
			 i+1);
    }
}

void ba_init(struct block_allocator * a,
				 uint32_t block_size, uint32_t blocks) {
    uint32_t page_size = block_size * blocks;

    a->first = a->last = 0;
    a->last_free = 0;

    if ((page_size & (page_size - 1)) == 0)
	a->magnitude = (uint16_t)ctz32(page_size); 
    else 
	a->magnitude = (uint16_t)ctz32(round_up32(page_size));
    //fprintf(stderr, "page_size: %u, magnitude: %u\n", page_size, a->magnitude);

    a->block_size = block_size;
    a->blocks = blocks;
    a->num_pages = 0;

    // we start with management structures for 16 pages
    a->allocated = 16;
    a->pages = NULL;
    ba_realloc(a);
}

void ba_free_all(struct block_allocator * a) {
    int i;

    for (i = 0; i < a->num_pages; i++) {
	free(BA_PAGE(a, i)->data);
    }
    a->num_pages = 0;
    a->first = a->last = 0;
}

void ba_destroy(struct block_allocator * a) {
    ba_free_all(a);
    free(a->pages);
    a->pages = NULL;
}

static inline void ba_grow(struct block_allocator * a) {
    a->allocated *= 2;
    ba_realloc(a);
}

static inline void ba_shrink(struct block_allocator * a) {
    a->allocated /= 2;
    ba_realloc(a);
}

static inline unsigned int mix(uintptr_t t) {
    t ^= (t >> 20) ^ (t >> 12);
    return (unsigned int)(t ^ (t >> 7) ^ (t >> 4));
}

static inline unsigned int hash1(const struct block_allocator * a,
			     const void * ptr) {
    uintptr_t t = ((uintptr_t)ptr) >> a->magnitude;

    return mix(t)*2;
}

static inline unsigned int hash2(const struct block_allocator * a,
			     const void * ptr) {
    uintptr_t t = ((uintptr_t)ptr) >> a->magnitude;

    return mix(t+1)*2; 
}

/*
 * insert the pointer to an allocated page into the
 * hashtable. uses linear probing and open allocation.
 */
static inline void ba_htable_insert(const struct block_allocator * a,
				    const void * ptr, const uint16_t n) {
    register unsigned int hval = hash1(a, ptr);

    /*
    fprintf(stderr, "MASK: %04X\t\tmagnitude: %u\t",
	    BA_HASH_MASK(a), a->magnitude);
    fprintf(stderr, "ba_htable_insert(%p, %u)\thval: %04X\tbucket: %u\n",
	    ptr, n, hval, hval & BA_HASH_MASK(a));
    */

    while (a->htable[hval & BA_HASH_MASK(a)]) hval++;
    /*
    fprintf(stderr, "ended up in bucket: %u\n",
	    hval & BA_HASH_MASK(a));
	    */
    a->htable[hval & BA_HASH_MASK(a)] = n;
}

void ba_print_htable(struct block_allocator * a) {
    int i;
    ba_page p;

    fprintf(stderr, "allocated: %u\n", a->allocated*2);
    fprintf(stderr, "magnitude: %u\n", a->magnitude);

    for (i = 0; i < a->allocated * 2; i++) {
	uint16_t n = a->htable[i];
	unsigned int hval;
	void * ptr;
	if (n) {
	    p = BA_PAGE(a, n-1);
	    ptr = (void*)((char*)p->data + BA_PAGESIZE(a) - 1);
	    hval = hash1(a, ptr);
	    fprintf(stdout, "%u\t%u\t%u\t%u\n", i, hval, (uintptr_t)ptr, ((uintptr_t)ptr >> a->magnitude));
	}
    }
}

#define likely(x)   (__builtin_expect((x), 1))
#define unlikely(x)   (__builtin_expect((x), 0))

static inline uint16_t ba_htable_lookup(const struct block_allocator * a,
					const void * ptr) {
    unsigned int hval1, hval2; 
    unsigned int n1, n2;
    unsigned int mask = BA_HASH_MASK(a);
    ba_page p;

    hval1 = hash1(a, ptr);
    n1 = a->htable[hval1 & mask];
    if (n1) {
	p = BA_PAGE(a, n1-1);
	if (BA_CHECK_PTR(a, p, ptr)) {
	    return n1;
	}
	hval1++;
    }

    hval2 = hash2(a, ptr);
    n2 = a->htable[hval2 & mask];
    if (n2) {
	p = BA_PAGE(a, n2-1);
	if (BA_CHECK_PTR(a, p, ptr)) {
	    return n2;
	}
	hval2++;
    }

    while (n1 || n2) {
	if (n1) {
	    n1 = a->htable[hval1 & mask];
	    if (n1) {
		p = BA_PAGE(a, n1-1);
		if (BA_CHECK_PTR(a, p, ptr)) {
		    return n1;
		}
		hval1++;
	    }
	}

	if (n2) {
	    n2 = a->htable[hval2 & mask];
	    if (n2) {
		p = BA_PAGE(a, n2-1);
		if (BA_CHECK_PTR(a, p, ptr)) {
		    return n2;
		}
		hval2++;
	    }
	}

    }

    return 0;
}

#define BA_MASK_COLLAPSE(a, p, t)   do {				\
    unsigned int _i, _n = BA_MASK_NUM(a);				\
    t = p->mask[0];							\
    switch (_n) {							\
    default:								\
    for (_i = 1; _i < _n - 1; _i++)					\
	if (t == 0) t |= p->mask[_i];					\
	else if (~t == 0) t &= p->mask[_i];				\
	else break;							\
    case 2:								\
	if (t == 0) t |= p->mask[_n-1] & BA_LAST_MASK(a);		\
	else if (~t == 0) t &= p->mask[_n-1] | ~BA_LAST_MASK(a);	\
	break;								\
    case 1:								\
	t &= BA_LAST_MASK(a);						\
	if (t == BA_LAST_MASK(a)) t = ~((uintptr_t)0);			\
    }									\
} while(0)

void * ba_alloc(struct block_allocator * a) {
    ba_page p;
    size_t i, j;

    if (unlikely(a->first == 0)) {
	if (unlikely(a->num_pages == a->allocated)) {
	    ba_grow(a);
	}
	p = BA_PAGE(a, a->num_pages++);
	p->data = malloc(BA_PAGESIZE(a));
	if (!p->data) {
	    Pike_error("no mem");
	}
	memset((void*)p->mask, 0xff, BA_MASK_NUM(a)*sizeof(uintptr_t));
	p->next = p->prev = 0;
	a->first = a->last = a->num_pages;
	ba_htable_insert(a, (void*)((char*)p->data + BA_PAGESIZE(a) - 1), a->num_pages);
	p->free_mask = 0;
	p->mask[0] = ~TBITMASK(uintptr_t, 0);
	return p->data;
    } else {
	uintptr_t m;

	p = BA_PAGE(a, a->first - 1);

	i = p->free_mask;
	m = p->mask[i];
	if (!m) {
//	    fprintf(stderr, "blk(%p)# n: %u\tmask: %04X\n", p, i, m);
	    Pike_error("This should not happen!\n");
	}
//	fprintf(stderr, "blk(%p)> n: %u\tmask: %04X\n", p, i, m);
#if SIZEOF_CHAR_P == 8
	j = ctz64(m);
#else
	j = ctz32(m);
#endif
	m ^= TBITMASK(uintptr_t, j);
	p->mask[i] = m;
//	fprintf(stderr, "blk(%p)< n: %u\tmask: %04X\n", p, i, m);

	if (unlikely(m == 0)) {
	    for (m = i+1; m < BA_MASK_NUM(a); m++) {
		if (p->mask[m]) {
		    p->free_mask = m;
		    return BA_BLOCKN(a, p, (i*sizeof(uintptr_t)*8 + j));
		}
	    }
	    if (a->last == a->first) {
		a->first = a->last = 0;
	    } else {
		a->first = p->next;
	    }
	    p->next = 0;
	    p->free_mask = BA_MASK_NUM(a);
	}

	return BA_BLOCKN(a, p, (i*sizeof(uintptr_t)*8 + j));
    }
}

void ba_free(struct block_allocator * a, void * ptr) {
    uintptr_t t;
    unsigned int mask, bit;
    ba_page p;
    unsigned int n;
    
    n = a->last_free;

    if (n) {
	p = BA_PAGE(a, n-1);
	if (!BA_CHECK_PTR(a, p, ptr)) n = 0;
    }
    
    if (!n) {
	a->last_free = n = ba_htable_lookup(a, ptr);

	if (!n) {
	    Pike_error("Unknown pointer \n");
	}
	
	p = BA_PAGE(a, n-1);
    }

    t = (uintptr_t)((char*)ptr - p->data)/a->block_size;
    mask = t / (sizeof(uintptr_t) * 8);
    bit = t & ((sizeof(uintptr_t)*8) - 1);

    t = p->mask[mask];

    if (t & TBITMASK(uintptr_t, bit)) {
	Pike_error("double free!");
    }
    if (mask < p->free_mask)
	p->free_mask = mask;

    if (t == 0) {
	p->free_mask = mask;
	goto check_special;
    }

    t |= TBITMASK(uintptr_t, bit);
    p->mask[mask] = t;

    if (~t != 0) {
	return;
    }

check_special:

    BA_MASK_COLLAPSE(a, p, t);
    p->mask[mask] |= TBITMASK(uintptr_t, bit);
    //fprintf(stderr, "collapsed mask is: %X\n", t);

    if (!t) {
	if (a->first == 0) {
	    a->first = a->last = n;
	    p->next = p->prev = 0;
	} else {
	    p->next = a->first;
	    BA_PAGE(a, a->first)->prev = n;
	    a->first = n;
	}
    } else if (~t == 0) {
	//fprintf(stderr, "could reclaim memory of page %u\n", n);	
	// we can reclaim memory to the system here!
    }
}
