#include "bitvector.h"
#include "svalue.h"
#include <stdint.h>

#ifndef INITIAL_MAGNITUDE
# define INITIAL_MAGNITUDE	4
#endif

#ifndef AVG_CHAIN_LENGTH
# define AVG_CHAIN_LENGTH	4
#endif

// TODO this is maybe not layouted in the best
// way possible?
struct keypair {
    struct ht_keypair * next;
    struct svalue key;
    union {
	struct svalue val;
	struct ht_keypair * next;
    } u;
    unsigned INT32 hval;
}

struct reentrance_marker {
    int marker;
}

struct mapping {
    PIKE_MEMORY_OBJECT_MEMBERS;
    unsigned INT32 hash_mask;
    unsigned INT32 magnitude;
    unsigned INT32 size;
    struct keypair ** table; 
    struct hash_iterator * first_iterator;
    struct keypair * trash;
    struct ba_local allocator;
    struct mapping * next, * prev;
    struct mapping * data;
    struct reentrance_marker marker;
}

struct mapping_iterator {
    struct mapping_iterator * next, * prev;
    union {
	struct ht_keypair * current;
	struct ht_keypair ** slot;
    } u;
    struct mapping * m;
}

static void clear_marker(struct reentrance_marker * m) {
    m->marker = 0;
}

static INLINE void mark_enter(struct reentrance_marker * m) {
    if (m->marker) Pike_error("Side effect free method reentered\n");
    m->marker = 1; 
}

static INLINE void mark_leave(struct reentrance_marker * m) {
    if (!m->marker) {
	m->marker = 0;
	Pike_error("marker is zero\n");
    } else 
	m->marker = 0;
}

static INLINE void mapping_it_reset(struct mapping_iterator * it) {
    unsigned INT32 i = 0;
    const struct mapping * m = it->m;
    it->u.current = NULL;
    for (; i <= m->hash_mask; i++) if (m->table[i]) {
	it->u.current = m->table[i];
	break;
    }
}

static INLINE void mapping_it_init(struct mapping_iterator * it, const struct mapping * m) {
    DOUBLELINK(m->first_iterator, it);
    it->m = m;
    mapping_it_reset(it);
}


static INLINE void mapping_it_exit(struct mapping_iterator * it) {
    struct mapping * m = it->m;

    DOUBLEUNLINK(m->first_iterator, it);

    if (!m->first_iterator)
	low_mapping_cleanup(m);
}

static INLINE int mapping_it_next(struct mapping_iterator * it,
				  struct keypair ** slot) {
    struct keypair * k = it->u.current;
    const struct mapping * m = it->m;

    if (!k) return 0;

    do {
	if (k->next) {
	    k = k->next;
	} else {
	    i = (k->hval >> m->magnitude) + 1;
	    k = NULL;

	    for (; i <= m->hash_mask; i++) if (k = m->table[i]) break;
	    if (!k) break;
	}
    } while (keypair_deleted(k));

    *slot = it->u.current = k;
    return !!k;
}

/* set iterator to continue iteration from k
 */
static INLINE void mapping_it_set(struct mapping_iterator * it, const struct keypair * k) {
    const struct mapping * m = it->m;
    it->u.current = k;
}

static INLINE void mapping_builder_init(struct mapping_iterator * it,
					struct mapping * m) {
    mapping_it_init(it, m);
}

static INLINE void mapping_builder_add(struct mapping_iterator * it, struct keypair * k) {
    struct keypair ** slot, * n;
    const struct mapping * m = it->m;

    if (it->u.current) {
	unsigned INT32 hval = it->u.current->hval;
	if ((hval ^ k->hval) >> m->magnitude)
	    goto new_slot;
	slot = &k->next;    
    } else {
new_slot:
	slot = low_get_bucket(m, k->hval);
    }
    n = ba_lalloc(&m->allocator);
    n->next = NULL;
    n->hval = k->hval;
    assign_svalue_no_free(&n->key, &k->key);
    assign_svalue_no_free(&n->u.val, &k->u.val);
    *slot = n;
    it->u.current = n
}

static INLINE void mapping_builder_finish(struct mapping_iterator * it) {
    mapping_it_exit(it);
}

extern struct mapping * first_mapping;


#define mapping_data	mapping
#define MAPPING_LOOP(m)	for (e = 0; e <= m->hash_mask; e++) for (k = m->table[e]; k; k = k->next)
#define NEW_MAPPING_LOOP(m)	MAPPING_LOOP(m)
