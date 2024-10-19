/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H

#include "global.h"
#include "pike_error.h"
#include "pike_memory.h"

struct ba_layout {
    unsigned INT32 offset;
    unsigned INT32 block_size;
    unsigned INT32 blocks;
};

#define BA_LAYOUT_INIT(block_size, blocks)  { 0, (block_size), (blocks) }

struct ba_page_header {
    struct ba_block_header * first;
    unsigned INT32 used;
    unsigned INT32 flags;
};

struct ba_page {
    struct ba_page_header h;
};

struct block_allocator {
    struct ba_layout l;
    unsigned char size, last_free, alloc;
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

struct ba_iterator {
    void * cur;
    void * end;
    struct ba_layout l;
};

typedef void (*ba_walk_callback)(struct ba_iterator *,void*);

PMOD_EXPORT
void ba_walk(struct block_allocator * a, ba_walk_callback cb, void * data);

static inline int PIKE_UNUSED_ATTRIBUTE ba_it_step(struct ba_iterator * it) {
    it->cur = (char*)it->cur + it->l.block_size;

    return (char*)it->cur < (char*)it->end;
}

static inline void PIKE_UNUSED_ATTRIBUTE * ba_it_val(struct ba_iterator * it) {
    return it->cur;
}

#define BA_INIT(block_size, blocks) {			    \
    BA_LAYOUT_INIT(block_size, blocks),			    \
    0, 0, 0,						    \
    { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,		    \
      NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,		    \
      NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL }		    \
}

#define BA_INIT_PAGES(block_size, pages)	BA_INIT(block_size, ((pages) * PIKE_MALLOC_PAGE_SIZE)/(block_size))

PMOD_EXPORT void ba_init(struct block_allocator * a, unsigned INT32 block_size, unsigned INT32 blocks);
ATTRIBUTE((malloc)) PMOD_EXPORT void * ba_alloc(struct block_allocator * a);
PMOD_EXPORT void ba_free(struct block_allocator * a, void * ptr);
PMOD_EXPORT void ba_destroy(struct block_allocator * a);
PMOD_EXPORT void ba_free_all(struct block_allocator * a);
PMOD_EXPORT size_t ba_count(const struct block_allocator * a);
PMOD_EXPORT void ba_count_all(const struct block_allocator * a, size_t * num, size_t * size);

#endif
