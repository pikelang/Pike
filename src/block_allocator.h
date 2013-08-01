#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H

#include "global.h"
#include "pike_error.h"
#include "pike_memory.h"
#include "pike_int_types.h"

struct ba_layout {
    unsigned INT32 offset;
    unsigned INT32 block_size;
    unsigned INT32 blocks;
    unsigned INT32 alignment;
    unsigned INT32 doffset;
};

#define BA_LAYOUT_INIT(block_size, blocks, alignment)  { 0, (block_size), (blocks), (alignment), 0 }

struct ba_page_header {
    struct ba_block_header * first;
    unsigned INT32 used;
};

struct ba_page {
    struct ba_page_header h;
};

struct block_allocator {
    struct ba_layout l;
    unsigned char size;
    unsigned char last_free;
    unsigned char alloc;
    unsigned char __align;
    /*
     * This places an upper limit on the number of blocks
     * and should be adjusted as needed.
     *
     * the formula is as follows:
     *	(initial_size << 24) - initial_size
     * for example for short pike strings this means that at most
     * 192 GB of short pike strings with shift width 0 can be allocated.
     */
    struct ba_page * pages[24];
};

#define BA_INIT_ALIGNED(block_size, blocks, alignment) {    \
    BA_LAYOUT_INIT(block_size, blocks, alignment),	    \
    0, 0, 0, 0,						    \
    { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,		    \
      NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,		    \
      NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL }		    \
}

#define BA_INIT(block_size, blocks) BA_INIT_ALIGNED(block_size, blocks, 0)

#define BA_INIT_PAGES(block_size, pages)	BA_INIT(block_size, ((pages) * PIKE_MALLOC_PAGE_SIZE)/(block_size))

PMOD_EXPORT void ba_init_aligned(struct block_allocator * a, unsigned INT32 block_size, unsigned INT32 blocks,
				 unsigned INT32 alignment);
ATTRIBUTE((malloc)) PMOD_EXPORT void * ba_alloc(struct block_allocator * a);
PMOD_EXPORT void ba_free(struct block_allocator * a, void * ptr);
PMOD_EXPORT void ba_destroy(struct block_allocator * a);
PMOD_EXPORT size_t ba_count(const struct block_allocator * a);
PMOD_EXPORT void ba_count_all(const struct block_allocator * a, size_t * num, size_t * size);

static INLINE void ba_init(struct block_allocator * a, unsigned INT32 block_size, unsigned INT32 blocks) {
    ba_init_aligned(a, block_size, blocks, 0);
}
#endif
