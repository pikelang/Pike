#include <stdlib.h>

#include "bitvector.h"
#include "pike_error.h"

struct ba_page {
    char * data;
    uint16_t free;
    uintptr_t mask[1];
};

#include "block_allocator.h"

#define BA_DIVIDE(a, b)	    ((a) / (b) + !!((a) % (b)))
#define BA_PAGESIZE(a)	    ((a)->blocks * (a)->block_size) 
#define BA_MASK_NUM(a)	    BA_DIVIDE((a)->blocks, sizeof(uintptr_t)*8)
#define BA_SPAGE_SIZE(a)    (sizeof(struct ba_page) + (BA_MASK_NUM(a) - 1)*sizeof(uintptr_t))
#define BA_HASH_MASK(a)  ((1 << (a->allocated + 1)) - 1)
#define BA_CHECK_PTR(a, p, ptr)	((char*)ptr >= p->data && ((uintptr_t)(p->data) + BA_PAGESIZE(a) > (uintptr_t)ptr))
#define BA_PAGE(a, n)   ((ba_page)((char*)a->pages + (n) * BA_SPAGE_SIZE(a)))

void ba_init(struct block_allocator * a,
				 uint32_t block_size, uint32_t blocks) {
    uint32_t page_size = block_size * blocks;

    a->free = 0;

    if ((page_size & (page_size - 1)) == 0)
	a->magnitude = (uint16_t)clz32(page_size); 
    else 
	a->magnitude = (uint16_t)clz32(round_up32(page_size));

    a->block_size = block_size;
    a->blocks = blocks;
    a->num_pages = 0;

    a->allocated = 5;
    a->pages = malloc(( BA_SPAGE_SIZE(a) + 2 * sizeof(uint16_t))
				 * (1 << a->allocated));
    if (!a->pages) {
	Pike_error("no mem");
    }
    a->htable = (uint16_t*) BA_PAGE(a, (1 << a->allocated));
    memset(a->pages, 0,
	   ( BA_SPAGE_SIZE(a) + 2 * sizeof(uint16_t)) * (1 << a->allocated));
}

static inline uint16_t mix(uintptr_t t) {
    t ^= (t >> 20) ^ (t >> 12);
    return (uint16_t)(t ^ (t >> 7) ^ (t >> 4));
}

static inline uint16_t hash1(struct block_allocator * a, void * ptr) {
    register uintptr_t t = ((uintptr_t)ptr) >> a->magnitude;

    return mix(t);
}

static inline uint16_t hash2(struct block_allocator * a, void * ptr) {
    register uintptr_t t = ((uintptr_t)ptr) >> a->magnitude;

    return mix(t+1); 
}

/*
 * insert the pointer to an allocated page into the
 * hashtable. uses linear probing and open allocation.
 */
static inline void ba_htable_insert(struct block_allocator * a,
					void * ptr, uint16_t n) {
    uint16_t hval = hash1(a, ptr);

    while (a->htable[hval & BA_HASH_MASK(a)]) hval++;
    a->htable[hval & BA_HASH_MASK(a)] = n;
}

static inline uint16_t ba_htable_lookup(struct block_allocator * a,
					void * ptr) {
    uint16_t hval; 
    uint16_t n;
    ba_page p;

    hval = hash1(a, ptr);

    while ((n = a->htable[hval & BA_HASH_MASK(a)])) {
	p = BA_PAGE(a, n-1);
	if (BA_CHECK_PTR(a, p, ptr)) {
	    return n;
	}
    }

    hval = hash2(a, ptr);

    while ((n = a->htable[hval & BA_HASH_MASK(a)])) {
	p = BA_PAGE(a, n-1);
	if (BA_CHECK_PTR(a, p, ptr)) {
	    return n;
	}
    }

    return 0;
}

void * ba_alloc(struct block_allocator * a) {
    ba_page p;
    size_t i, j;

    if (a->free == 0) {
	p = BA_PAGE(a, a->num_pages++);
	p->data = malloc(BA_PAGESIZE(a));
	if (!p->data) {
	    Pike_error("no mem");
	}
	memset((void*)p->mask, 0xff, BA_MASK_NUM(a)*sizeof(uintptr_t));
	p->free = 0;
	a->free = a->num_pages;
	ba_htable_insert(a, (void*)((char*)p->data + BA_PAGESIZE(a) - 1), a->num_pages);
	p->mask[0] = ~TBITMASK(uintptr_t, 0);
	return p->data;
    } else {
	p = BA_PAGE(a, a->free - 1);

	for (j = -1, i = 0; i < BA_MASK_NUM(a); i++) {
	    uintptr_t m = p->mask[i];
	    if (m) {
		if (sizeof(uintptr_t) == 8)
		    j = ctz64(m);
		else 
		    j = ctz32(m);
		p->mask[i] ^= TBITMASK(uintptr_t, j);
		break;
	    }
	}

	if (j == -1) {
	    Pike_error("This should not happen!\n");
	}

	// now empty.
	if (p->mask[i] == 0 && i == BA_MASK_NUM(a)-1) {
	    a->free = p->free;
	    p->free = 0;
	}

	return (void*)(((char*)p->data) + (i*sizeof(uintptr_t)*8 + j)*a->block_size);
    }
}

void ba_free(struct block_allocator * a, void * ptr) {
    uint16_t n = ba_htable_lookup(a, ptr);
    uintptr_t t;
    unsigned int mask, bit;
    ba_page p;

    if (!n) {
	Pike_error("Unknown pointer: %p\n", ptr);
    }
    
    p = BA_PAGE(a, n-1);
    t = (uintptr_t)((char*)ptr - p->data)/a->block_size;
    mask = t / (sizeof(uintptr_t) * 8);
    bit = t & ((sizeof(uintptr_t)*8) - 1);

    if (p->mask[mask] & TBITMASK(uintptr_t, bit)) {
	Pike_error("double free!");
    }

    if (p->mask[mask] == 0) {
	p->mask[mask] = TBITMASK(uintptr_t, bit);
	if (a->free == 0) {
	    a->free = n;
	    p->free = 0;
	    return;
	}
	if (a->free > n) {
	    p->free = a->free;
	    a->free = n;
	} else {
	    ba_page tmp = BA_PAGE(a, a->free-1);
	    while (tmp->free && tmp->free < n) {
		tmp = BA_PAGE(a, tmp->free - 1);
	    }
	    p->free = tmp->free;
	    tmp->free = n;
	}
	return;
    }

    p->mask[mask] |= ((uintptr_t)1 << bit);

    for (mask = 0; mask < BA_MASK_NUM(a); mask++) {
	if (p->mask[mask] != ~((uintptr_t)0)) return;
    }

    // TODO: reclaim memory to the system
}
