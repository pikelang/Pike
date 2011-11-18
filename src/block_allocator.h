#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H
#include <stdint.h>

typedef struct ba_page * ba_page;

struct block_allocator {
    uint32_t block_size;
    uint32_t blocks;
    uint32_t num_pages;
    uint16_t first, last; 
    uint32_t magnitude;
#ifdef BA_SEGREGATE
    ba_page pages;
#else
    void * pages;
#endif
    uint16_t * htable;
    uint16_t allocated;
#ifdef BA_SEGREGATE
    uint16_t last_free;
#else
    uint16_t last_free, ba_page_size;
#endif
};

#ifdef BA_SEGREGATE
#define BA_INIT(block_size, blocks) { block_size, blocks, 0, NULL, NULL, \
				      0, NULL, NULL, 0, 0 }
#else
#define BA_INIT(block_size, blocks) { block_size, blocks, 0, NULL, NULL, \
				      0, NULL, NULL, 0, 0, 0 }
#endif

void ba_init(struct block_allocator * a, uint32_t block_size, uint32_t blocks);
void * ba_alloc(struct block_allocator * a);
void ba_free(struct block_allocator * a, void * ptr);
void ba_print_htable(struct block_allocator * a);

#endif /* BLOCK_ALLOCATOR_H */
