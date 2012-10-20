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
    unsigned INT32 flags;
    TYPE_FIELD key_types, val_types;
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
    struct ht_keypair ** current;
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

static INLINE int keypair_deleted(const struct keypair * k) {
    return TYPEOF(k->key) == PIKE_T_FREE;
}

static INLINE unsigned INT32 low_mapping_hash(const struct svalue * key) {
    return __builtin_bswap32(hash_svalue(key));
}

static INLINE struct keypair ** low_get_bucket(const struct mapping * m, unsigned INT32 hval) {
    return m->table + (hval >> m->magnitude);
}


static INLINE void mapping_it_reset(struct mapping_iterator * it) {
    unsigned INT32 i = 0;
    const struct mapping * m = it->m;
    struct keypair ** slot = NULL;
    for (; i <= m->hash_mask; i++) if (*(slot = m->table + i)) break;
    it->current = slot;
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

static INLINE void mapping_it_current(struct mapping_iterator * it) {
    return it->current ? *it->currrent : NULL;
}

static INLINE void mapping_it_step(struct mapping_iterator * it) {
    struct keypair ** slot = it->current;
    const struct mapping * m = it->m;

    do {
	const struct keypair * k = *slot;
	if (k->next) {
	    slot = &k->next;
	} else {
	    i = (k->hval >> m->magnitude) + 1;

	    for (; i <= m->hash_mask; i++) if (*(slot = m->table + i)) break;
	}
    } while (*slot && keypair_deleted(*slot));

    it->current = slot;
}

static INLINE int mapping_it_next(struct mapping_iterator * it,
				  struct keypair ** t) {
    if (!it->current) return 0;
    mapping_it_step(it);
    if (!it->current) return 0;
    *t = *it->current;
    return 1;
}

static INLINE void mapping_it_delete(struct mapping_iterator * it) {
    struct mapping * m = it->m;
    struct keypair ** slot = it->current;
    struct keypair * k;
    if (!slot) return;

    k = *slot;

    mark_free_svalue(&k->key);
    mark_free_svalue(&k->u.val);
    mapping_it_step(it);

    /* this seems paradoxical. but we dont require iterators to be
     * registered with the mapping */
    if (m->first_iterator) {
	k->u.next = m->trash;
	m->track = k;
    } else {
	ba_lfree(&m->allocator, k);
    }
    m->size --;
}

static INLINE void mapping_builder_init(struct mapping_iterator * it,
					struct mapping * m) {
    mapping_it_init(it, m);
}

static INLINE void mapping_builder_add(struct mapping_iterator * it, struct keypair * k) {
    struct keypair * n;
    struct mapping * m = it->m;

    if (it->current) {
	const struct keypair * k = *it->current;
	unsigned INT32 hval = k->hval;
	if ((hval ^ k->hval) >> m->magnitude)
	    goto new_slot;
    } else {
new_slot:
	it->current = low_get_bucket(m, k->hval);
    }
    n = ba_lalloc(&m->allocator);
    n->next = NULL;
    n->hval = k->hval;
    assign_svalue_no_free(&n->key, &k->key);
    assign_svalue_no_free(&n->u.val, &k->u.val);
    *it->current = n;
    it->current = &n->next;
    m->key_types |= 1 << TYPEOF(k->key);
    m->val_types |= 1 << TYPEOF(k->u.val);
}

static INLINE void mapping_builder_finish(struct mapping_iterator * it) {
    mapping_it_exit(it);
}

extern struct mapping * first_mapping;


#define mapping_data	mapping
#define MAPPING_LOOP(m)	for (e = 0; e <= m->hash_mask; e++) for (k = m->table[e]; k; k = k->next)
#define NEW_MAPPING_LOOP(m)	MAPPING_LOOP(m)
