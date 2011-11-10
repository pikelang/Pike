#include <stdint.h>

typedef struct ba_page * ba_page;

struct block_allocator {
    uint32_t block_size;
    uint32_t blocks;
    uint32_t num_pages;
    uint16_t first, last; 
    uint32_t magnitude;
    uint32_t allocated;
    void * pages;
    uint16_t * htable;
    uint16_t last_free;
};

void ba_init(struct block_allocator * a, uint32_t block_size, uint32_t blocks);
void * ba_alloc(struct block_allocator * a);
void ba_free(struct block_allocator * a, void * ptr);
void ba_print_htable(struct block_allocator * a);
