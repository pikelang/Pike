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

static INLINE void mapping_it_init(struct mapping_iterator * it, struct mapping * m) {
    DOUBLELINK(m->first_iterator, it);
    it->u.current = NULL;
    it->m = m;
    it->hash_mask = m->hash_mask;
    it->n = (unsigned INT32)-1;
}

static INLINE void mapping_it_exit(struct mapping_iterator * it) {
    struct mapping * m = it->m;

    DOUBLEUNLINK(m->first_iterator, it);

    if (!m->first_iterator)
	low_mapping_cleanup(m);
}

static INLINE int mapping_it_next(struct mapping_iterator * it,
				  struct keypair ** slot) {
    const struct keypair * k = it->u.current;
    const struct mapping * m = it->m;
    unsigned INT32 i = it->n;

    if (it->hash_mask == m->hash_mask) {
	return mapping_it_next_eq(it, slot);
    } else if (it->hash_mask < m->hash_mask) {
	/* mapping has grown in the meantime */
	return mapping_it_next_grown(it, slot);
    } else {
	/* mapping has been shrunk in the meantime */
	return mapping_it_next_shrunk(it, slot);
    }
}

/* set iterator to continue iteration from k
 */
static INLINE void mapping_it_set(struct mapping_iterator * it, const struct keypair * k) {
    const struct mapping * m = it->m;
    it->u.current = k;
    it->hash_mask = m->hash_mask;
    it->n = m->hash_mask & k->hval;
}
