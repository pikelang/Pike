#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "post_modules/Critbit/bitvector.h"
#define __PIKE__
#ifdef __PIKE__
# include "pike_error.h"
#endif

#define BA_SEGREGATE

struct ba_page {
    char * data;
    uint16_t next, prev;
    uint16_t hchain;
#ifdef BA_SEGREGATE
    uint16_t blocks_used, first;
#else 
    uint16_t blocks_used;
    uint16_t free_mask;
    uintptr_t mask[1];
#endif
};

#ifdef BA_SEGREGATE
struct ba_block_header {
    uint32_t next;
};
#endif

#include "block_allocator.h"

static inline void ba_htable_insert(const struct block_allocator * a,
				    const void * ptr, const uint16_t n);
static inline void ba_remove_page(struct block_allocator * a,
				  uint16_t n);

#define BA_BLOCKN(a, p, n) ((void*)(((p)->data) + (n)*((a)->block_size)))
#define BA_LASTBLOCK(a, p) BA_BLOCKN(a, p, (a)->blocks - 1)
#define BA_NBLOCK(a, p, ptr)	((uintptr_t)((char*)ptr - (p)->data)/(a)->block_size)

#define BA_DIVIDE(a, b)	    ((a) / (b) + !!((a) & ((b)-1)))
#define BA_PAGESIZE(a)	    ((a)->blocks * (a)->block_size) 
#define BA_SPAGE_SIZE(a)    (sizeof(struct ba_page) + (BA_DIVIDE((a)->blocks, sizeof(uintptr_t)*8) - 1)*sizeof(uintptr_t))
#define BA_HASH_MASK(a)  (((a->allocated)) - 1)
#define BA_CHECK_PTR(a, p, ptr)	((char*)ptr >= p->data && BA_LASTBLOCK(a,p) >= ptr)

#ifdef BA_SEGREGATE
# define BA_PAGE(a, n)   ((a)->pages + (n) - 1)
# define BA_BYTES(a)	( (sizeof(struct ba_page) + sizeof(uint16_t)) * ((a)->allocated) )
#else
# define BA_PAGE(a, n)   ((ba_page)((char*)(a)->pages + ((n)-1) * ((a)->ba_page_size)))
# define BA_BYTES(a)	(( (a)->ba_page_size + sizeof(uint16_t)) * ((a)->allocated))
# define BA_LAST_BITS(a)	((a)->blocks & (sizeof(uintptr_t)*8 - 1))
# define BA_LAST_MASK(a)	(BA_LAST_BITS(a) ? TMASK(uintptr_t, BA_LAST_BITS(a)) : ~((uintptr_t)0))
# define BA_MASK_NUM(a)	    (((a)->ba_page_size - sizeof(struct ba_page) + sizeof(uintptr_t))/sizeof(uintptr_t))
#endif


#ifndef __PIKE__
#include <stdio.h>
#include <unistd.h>

void Pike_error(char * msg) {
    fprintf(stderr, "ERROR: %s\n", msg);
    _exit(1);
}
#endif

static inline void ba_realloc(struct block_allocator * a) {
    unsigned int i;
    a->pages = realloc(a->pages, BA_BYTES(a));

    if (!a->pages) {
	Pike_error("no mem");
    }

    //fprintf(stderr, "realloc to size %u\n", a->allocated);
		
#ifndef BA_SEGREGATE
    memset((void*)BA_PAGE(a, a->num_pages+1), 0, BA_BYTES(a) - a->ba_page_size*a->num_pages);
#else
    memset((void*)BA_PAGE(a, a->num_pages+1), 0, BA_BYTES(a) - sizeof(struct ba_page)*a->num_pages);
#endif
    a->htable = (uint16_t*) BA_PAGE(a, (a->allocated)+1);
    for (i = 0; i < a->num_pages; i++) {
	ba_htable_insert(a, BA_LASTBLOCK(a, BA_PAGE(a, i+1)), i+1);
    }
}

void ba_init(struct block_allocator * a,
				 uint32_t block_size, uint32_t blocks) {
    uint32_t page_size = block_size * blocks;

    if (blocks > 0xfffe) {
	Pike_error("number of blocks cannot exceed 2^16-1\n");
    }

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
#ifndef BA_SEGREGATE
    a->ba_page_size = BA_SPAGE_SIZE(a);
#endif
    ba_realloc(a);
}

void ba_free_all(struct block_allocator * a) {
    unsigned int i;

    for (i = 0; i < a->num_pages; i++) {
	free(BA_PAGE(a, i+1)->data);
    }
    a->num_pages = 0;
    a->first = a->last = 0;
}

void ba_count_all(struct block_allocator * a, size_t *num, size_t *size) {
    unsigned int i;
    size_t n = 0;

    fprintf(stderr, "page_size: %u, pages: %u\n", BA_PAGESIZE(a), a->num_pages);
    *size = BA_BYTES(a) + a->num_pages * BA_PAGESIZE(a);
    for (i = 0; i < a->num_pages; i++) {
	n += BA_PAGE(a, i+1)->blocks_used;
    }

    *num = n;
}

void ba_destroy(struct block_allocator * a) {
    ba_free_all(a);
    free(a->pages);
    a->pages = NULL;
}

static inline void ba_grow(struct block_allocator * a) {
    if (a->allocated) {
	a->allocated *= 2;
	ba_realloc(a);
    } else {
	ba_init(a, a->block_size, a->blocks);
    }
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

    return mix(t);
}

static inline unsigned int hash2(const struct block_allocator * a,
			     const void * ptr) {
    uintptr_t t = ((uintptr_t)ptr) >> a->magnitude;

    return mix(t+1); 
}

/*
 * insert the pointer to an allocated page into the
 * hashtable. uses linear probing and open allocation.
 */
static inline void ba_htable_insert(const struct block_allocator * a,
				    const void * ptr, const uint16_t n) {
    unsigned int hval = hash1(a, ptr);
    uint16_t * b = a->htable + (hval & BA_HASH_MASK(a));

    BA_PAGE(a, n)->hchain = *b;
    *b = n;
}

static inline void ba_htable_replace(const struct block_allocator * a, 
				     const void * ptr, const uint16_t n,
				     const uint16_t new) {
    unsigned int hval = hash1(a, ptr);
    uint16_t * b = a->htable + (hval & BA_HASH_MASK(a));

    while (*b) {
	if (*b == n) {
	    *b = new;
	    BA_PAGE(a, new)->hchain = BA_PAGE(a, n)->hchain;
	    return;
	}
	b = &BA_PAGE(a, *b)->hchain;
    }
}

static inline void ba_htable_delete(const struct block_allocator * a,
				    const void * ptr, const uint16_t n) {
    unsigned int hval = hash1(a, ptr);
    uint16_t * b = a->htable + (hval & BA_HASH_MASK(a));

    while (*b) {
	if (*b == n) {
	    *b = BA_PAGE(a, n)->hchain;
	    return;
	}
	b = &BA_PAGE(a, *b)->hchain;
    }
}

void ba_print_htable(struct block_allocator * a) {
    int i;
    ba_page p;

    fprintf(stderr, "allocated: %u\n", a->allocated*2);
    fprintf(stderr, "magnitude: %u\n", a->magnitude);
    fprintf(stderr, "block_size: %u\n", a->block_size);

    for (i = 0; i < a->allocated; i++) {
	uint16_t n = a->htable[i];
	unsigned int hval;
	void * ptr;
	while (n) {
	    p = BA_PAGE(a, n);
	    ptr = BA_LASTBLOCK(a, p);
	    hval = hash1(a, ptr);
	    fprintf(stderr, "%u\t%X\t%p-%p\t%X\n", i, hval, p->data, ptr, ((uintptr_t)ptr >> a->magnitude));
	    n = p->hchain;
	}
    }
}

#define likely(x)   (__builtin_expect((x), 1))
#define unlikely(x)   (__builtin_expect((x), 0))

static inline uint16_t ba_htable_lookup(const struct block_allocator * a,
					const void * ptr) {
    uint16_t n1, n2;
    int c = 0;
    ba_page p;
    n1 = a->htable[hash1(a, ptr) & BA_HASH_MASK(a)];
    n2 = a->htable[hash2(a, ptr) & BA_HASH_MASK(a)];

    while (n1 || n2) {
	//fprintf(stderr, "%4d:\tfound %u, %u\n", ++c, n1, n2);
	if (n1) {
	    p = BA_PAGE(a, n1);
	    if (BA_CHECK_PTR(a, p, ptr)) {
		return n1;
	    }
	    n1 = p->hchain;
	}
	if (n2) {
	    p = BA_PAGE(a, n2);
	    if (BA_CHECK_PTR(a, p, ptr)) {
		return n2;
	    }
	    n2 = p->hchain;
	}
    }
    return 0;
}

void * ba_alloc(struct block_allocator * a) {
    //fprintf(stderr, "ba_alloc(%p)\n", a);
    ba_page p;
#ifndef BA_SEGREGATE
    size_t i, j;
#endif

    if (likely(a->first)) {
#ifndef BA_SEGREGATE
	uintptr_t m;
#else
	void * ptr;
#endif

	p = BA_PAGE(a, a->first);

#ifndef BA_SEGREGATE
	i = p->free_mask;
	if (i >= BA_MASK_NUM(a)) {
	    Pike_error("free mask is out of range!\n");
	}
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
	//fprintf(stderr, "setting bit %u -> %8X (using %u blocks) maks_num: %u\n", i*sizeof(uintptr_t)*8 + j, m, p->blocks_used, BA_MASK_NUM(a));
	p->mask[i] = m;
//	fprintf(stderr, "blk(%p)< n: %u\tmask: %04X\n", p, i, m);

	// TODO: remove this:
	p->blocks_used ++;

	if (m) {
	    /*
	    fprintf(stderr, "3 alloced pointer %p (%u/%u used %u)\n",
		    BA_BLOCKN(a, p, (i*sizeof(uintptr_t)*8 + j)),
		    (i*sizeof(uintptr_t)*8 + j), a->blocks, p->blocks_used);
		    */
	    return BA_BLOCKN(a, p, (i*sizeof(uintptr_t)*8 + j));
	} else {
	    for (m = i+1; m < BA_MASK_NUM(a); m++) {
		if (p->mask[m]) {
		    p->free_mask = m;
		    /*
		    fprintf(stderr, "1 alloced pointer %p (%u/%u used %u)\n",
			    BA_BLOCKN(a, p, (i*sizeof(uintptr_t)*8 + j)),
			    (i*sizeof(uintptr_t)*8 + j), a->blocks,
			    p->blocks_used);
			    */
		    return BA_BLOCKN(a, p, (i*sizeof(uintptr_t)*8 + j));
		}
	    }
	    if (p->blocks_used != a->blocks) {
		fprintf(stderr, "wrong block count detected: %u vs %u\n", p->blocks_used, a->blocks);
		Pike_error("croak\n");
	    }
	    //fprintf(stderr, "page is full now\n");
	    if (a->last == a->first) {
		a->first = a->last = 0;
	    } else {
		a->first = p->next;
	    }
	    p->next = 0;
	    p->free_mask = BA_MASK_NUM(a);
	    return BA_BLOCKN(a, p, (i*sizeof(uintptr_t)*8 + j));
	}
#else
	if (p->blocks_used == a->blocks) Pike_error("baaad!\n");
	p->blocks_used ++;
	ptr = BA_BLOCKN(a, p, p->first-1);
	//fprintf(stderr, "alloced pointer %p (%u/%u used %u)\n",
		//ptr, p->first-1, a->blocks, p->blocks_used);

	if (unlikely(p->blocks_used == a->blocks)) {
	    a->first = p->next;
	    if (!a->first) {
		a->last = 0;
	    }
	    p->next = 0;
	} else {
	    //fprintf(stderr, "next: %u\n", ((struct ba_block_header*)ptr)->next);
	    if (((struct ba_block_header*)ptr)->next)
		p->first = ((struct ba_block_header*)ptr)->next;
	    else p->first++;
	}
	//fprintf(stderr, "first is %u\n", p->first);

	return ptr;
#endif
    }
	
    //fprintf(stderr, "allocating new page. was %p\n", p);
    if (unlikely(a->num_pages == a->allocated))
	ba_grow(a);

    p = BA_PAGE(a, ++a->num_pages);
    p->data = malloc(BA_PAGESIZE(a));
    if (!p->data) {
	Pike_error("no mem");
    }
    p->next = p->prev = 0;
    a->first = a->last = a->num_pages;
    ba_htable_insert(a, BA_LASTBLOCK(a, p), a->num_pages);
#ifndef BA_SEGREGATE
    if (BA_MASK_NUM(a) > 1)
	memset((void*)p->mask, 0xff, (BA_MASK_NUM(a)-1)*sizeof(uintptr_t));
    p->mask[BA_MASK_NUM(a)-1] = BA_LAST_MASK(a);
    p->mask[0] ^= TBITMASK(uintptr_t, 0);

    p->free_mask = 0;
    // TODO: remove this:
    p->blocks_used = 1;
#else
    p->blocks_used = 1;
    p->first = 2;
    memset(p->data, 0x00, BA_PAGESIZE(a));
#endif
    return p->data;
}

void ba_free(struct block_allocator * a, void * ptr) {
    uintptr_t t;
#ifndef BA_SEGREGATE
    unsigned int mask, bit;
#endif
    ba_page p;
    unsigned int n;
    
    n = a->last_free;

    if (n) {
	p = BA_PAGE(a, n);
	if (!BA_CHECK_PTR(a, p, ptr)) n = 0;
    }
    
    if (unlikely(!n)) {
	a->last_free = n = ba_htable_lookup(a, ptr);

	if (unlikely(!n)) {
#define BA_DEBUG
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
	
	p = BA_PAGE(a, n);
    }

    t = (uintptr_t)((char*)ptr - p->data)/a->block_size;
#ifndef BA_SEGREGATE
    mask = t / (sizeof(uintptr_t) * 8);
    bit = t & ((sizeof(uintptr_t)*8) - 1);
    /*
    fprintf(stderr, "freeing pointer %p (%u/%u) used %u\n", ptr, t, a->blocks,
	    p->blocks_used);
	    */

    t = p->mask[mask];

    if (t & TBITMASK(uintptr_t, bit)) {
	Pike_error("double free!");
    }
    if (mask < p->free_mask)
	p->free_mask = mask;
    // TODO: remove this:
    p->blocks_used --;

    if (unlikely(t == 0)) {
	unsigned int i;
	p->free_mask = mask;
	for (i = 0; i < BA_MASK_NUM(a); i++) if (p->mask[i]) {
	    p->mask[mask] = TBITMASK(uintptr_t, bit);
	    return;
	}
	p->mask[mask] = TBITMASK(uintptr_t, bit);
	if (p->blocks_used != a->blocks-1) {
	    fprintf(stderr, "wrong block count detected: %u vs %u\n", p->blocks_used+1, a->blocks);
	    Pike_error("croak\n");
	}
	if (a->first == 0) {
	    a->first = a->last = n;
	    p->next = p->prev = 0;
	} else {
	    p->next = a->first;
	    BA_PAGE(a, a->first)->prev = n;
	    a->first = n;
	}
	return;
    }

    t |= TBITMASK(uintptr_t, bit);
    p->mask[mask] = t;

    if (unlikely(~t == 0)) {
	for (bit = 0; bit < BA_MASK_NUM(a)-1; bit++) if (~p->mask[bit]) return;
	if (~(p->mask[BA_MASK_NUM(a)-1]|~BA_LAST_MASK(a))) return;
	if (p->blocks_used != 0) {
	    fprintf(stderr, "wrong block count detected: %u vs %u\n", p->blocks_used+1, 1);
	    Pike_error("croak\n");
	}
	ba_remove_page(a, n);
	//fprintf(stderr, "could reclaim memory of page %u\n", n);	
	// we can reclaim memory to the system here!
    } else if (p->blocks_used == 0 || p->blocks_used == a->blocks - 1) {
	fprintf(stderr, "wrong number of used blocks: %u\n", p->blocks_used);
	Pike_error("bad croak\n");
    }
#else
    if (p->blocks_used == a->blocks) {
	if (a->first == 0) {
	    a->first = a->last = n;
	    p->next = p->prev = 0;
	} else {
	    p->next = a->first;
	    BA_PAGE(a, a->first)->prev = n;
	    a->first = n;
	}
    } else if (p->blocks_used == 1) {
	ba_remove_page(a, n);
	return;
    }
    p->blocks_used --;
    ((struct ba_block_header*)ptr)->next = p->first;
    //fprintf(stderr, "setting first to %u (%p vs %p) n: %u vs %u\n", t+1, BA_BLOCKN(a, p, t+1), ptr, BA_NBLOCK(a, p, BA_BLOCKN(a, p, t+1)), BA_NBLOCK(a, p, ptr));
    p->first = t+1;
#endif
}

static inline void ba_remove_page(struct block_allocator * a,
				  uint16_t n) {
    ba_page tmp, p;

    p = BA_PAGE(a, n);

    /*
    fprintf(stderr, "removing page %4u\t(%p .. %p) -> %X (%X) (of %u).\n", n,
	    p->data, BA_LASTBLOCK(a, p), hash1(a, BA_LASTBLOCK(a,p)),
	    hash1(a, BA_LASTBLOCK(a,p)) & BA_HASH_MASK(a),
	    a->num_pages);
	    */

    ba_htable_delete(a, BA_LASTBLOCK(a, p), n);

    free(p->data);
    p->data = NULL;

    if (a->num_pages != n) {
	tmp = BA_PAGE(a, a->num_pages);
	ba_htable_replace(a, BA_LASTBLOCK(a, tmp), a->num_pages, n);
	/*
	fprintf(stderr, "replacing with page %u\t(%p .. %p) -> %X (%X)\n",
		a->num_pages,
		tmp->data, BA_LASTBLOCK(a, tmp), hash1(a, BA_LASTBLOCK(a,tmp)),
		hash1(a, BA_LASTBLOCK(a,tmp)) & BA_HASH_MASK(a)
		);
		*/
	*p = *tmp;
    }

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
    } else {
	a->last = p->prev;
    }

    a->num_pages--;
}
