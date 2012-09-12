#include "bitvector.h"
#include "svalue.h"
#include <stdint.h>

#ifndef INITIAL_MAGNITUDE
# define INITIAL_MAGNITUDE	4
#endif

struct keypair {
    struct ht_keypair * next;
    unsigned INT32 hval;
    struct svalue key;
    union {
	struct svalue val;
	struct ht_keypair * next;
    } u;
}

struct mapping {
    PIKE_MEMORY_OBJECT_MEMBERS;
    unsigned INT32 hash_mask;
    unsigned INT32 size;
    struct keypair ** table; 
    struct hash_iterator * first_iterator;
    struct keypair * trash;
    struct ba_local allocator;
}

struct mapping_iterator {
    struct mapping_iterator * next, * prev;
    union {
	struct ht_keypair * current;
	struct ht_keypair ** slot;
    } u;
    struct mapping * m;
    unsigned INT32 hash_mask;
    unsigned INT32 n;
}
