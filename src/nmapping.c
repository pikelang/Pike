#include "svalue.h"
#include "gjalloc.h"
#include "pike_macros.h"

// these could probably use the same allocator
static struct block_allocator mapping_allocator = BA_INIT(sizeof(struct mapping), 256),
			      iter_allocator = BA_INIT(sizeof(struct mapping_iterator), 128);

static INLINE int keypair_deleted(const struct keypair * k) {
    return IS_UNDEFINED(&k->key);
}

PMOD_EXPORT int mapping_it_next_eq(struct mapping_iterator * it, struct keypair ** slot) {
    const struct keypair * k = it->u.current;
    const struct mapping * m = it->m;
    unsigned INT32 i = it->n;

    if (k) {
	k = k->next;
    }

    while (!k) {
	if (i == it->hash_mask) break;
	i++;
	k = m->table[i];
    }

    it->n = i;
    *slot = it->u.current = k;
    return !!k;
}

/*
 * the mapping has been grown since iterator creation. it means that it->has_mask is smaller
 * than the mapping hash_mask. this effectively means that instead of increasing n by one and going
 * to the next slot, we have to jump by it->hash_mask until we have handled all slots that belong
 * to our it->n
 */
PMOD_EXPORT int mapping_it_next_grown(struct mapping_iterator * it, struct keypair ** slot) {
    return 1;
}

/* the mapping has shrunk since iterator creation. this means that several original buckets have
 * been merged into the same bucket. we have to manually filter the ones that do not belong to our
 * n. otherwise it works very similar to the trivial case.
 */

PMOD_EXPORT int mapping_it_next_shrunk(struct mapping_iterator * it, struct keypair ** slot) {
    return 1;
}

void mapping_rel_simple(void * _start, void * _stop, ptrdiff_t diff, void * data) {
    struct keypair * start = (struct keypair*)_start,
		   * stop  = (struct keypair*)_stop;
    struct mapping m * = (struct mapping *)data;
    struct mapping_iterator * it;
    unsigned INT32 i;

    for (; start < stop; start++) {
	ba_simple_rel_pointer(&start->next);
	if (keypair_deleted(start)) {
	    ba_simple_rel_pointer(&start->u.next);
	}
    }

    ba_simple_rel_pointer(&m->trash);

    for (it = m->first_iterator; it; it = it->next) {
	ba_simple_rel_pointer(&it->u.current);
    }

    for (i = 0; i <= m->hash_mask; i++) {
	ba_simple_rel_pointer(m->table + i);
    }
}

PMOD_EXPORT void really_free_mapping(struct mapping *m) {
}

ATTRIBUTE((malloc))
PMOD_EXPORT struct mapping *debug_allocate_mapping(int size) {
    struct mapping * m = ba_alloc(&mapping_allocator);
    unsigned INT32 t = (1 << INITIAL_MAGNITUDE);
    m->size = 0;

    while (size > t) t *= 2;

    m->hash_mask = t - 1;
    ba_init_local(&m->allocator, sizeof(struct keypair), t, 256, mapping_rel_simple, m);
    m->table = (struct keypair **)xalloc(sizeof(struct keypair **)*t);
    m->trash = NULL;
    m->first_iterator = NULL;

    return m;
}

static INLINE void free_keypair(void * _start, void * _stop, void * d) {
    struct keypair * start = (struct keypair*)_start,
		   * stop = (struct keypair*)_stop;

    for (;start < stop; start++) {
	if (keypair_deleted(start)) continue;
	free_svalue(start->key);
	free_svalue(start->u.val);
    }
}

PMOD_EXPORT void do_free_mapping(struct mapping *m) {
#ifdef DEBUG
    if (m->first_iterator) Pike_fatal("found iterator in free. refcount seems wrong.\n");
#endif
    free(m->table);
    ba_walk_local(&m->allocator, free_keypair, NULL);
    ba_ldestroy(&m->allocator);
    ba_free(&mapping_allocator, m);
}

static INLINE int reverse_cmpt(unsigned INT32 a, unsigned INT32 b) {
    const unsigned INT32 t = a ^ b;

    if (t) {
	const unsigned INT32 m = (1U << __builtin_ctz(t));

	if (a&m) return 1;
	return -1;
    }

    return 0;
}

/*
 * this is called only when no iterators are present. this also means that
 * code that reenters should not call this. this function also takes care
 * of shrinking and growing the mapping, checking for need for defragmentation
 * and so on.
 */
PMOD_EXPORT void low_mapping_cleanup(struct mapping * m) {
    struct keypair * k = m->trash;
    m->trash = NULL;

    while (k) {
	struct keypair * t = k->u.next;
	ba_lfree(&m->allocator, k);
	k = t;
    }
}

PMOD_EXPORT void mapping_fix_type_field(struct mapping *m) {}
PMOD_EXPORT void mapping_set_flags(struct mapping *m, int flags) {}
PMOD_EXPORT void low_mapping_insert(struct mapping *m,
				    const struct svalue *key,
				    const struct svalue *val,
				    int overwrite) {
    unsigned INT32 hval = hash_svalue(key);
    struct keypair ** t, * k;
    struct mapping_iterator it = { NULL, NULL, NULL, 0, 0 };
    int frozen = 0;

    /* this is just for optimization */
    ba_lreserve(&m->allocator, 1);

    t = m->table + hval & m->hash_mask;

    for (;*t; t = &(*t->next)) {
	int cmp = reverse_cmp(hval, (*t)->hval);
	k = *t;
	if (cmp == 1) continue;
	if (cmp == -1) break;
	if (keypair_deleted(k) || is_nidentical(&k->key, key)) continue;

	if (!is_identical(&k->key, key)) {
	    int eq;
	    if (!frozen) {
		DOUBLELINK(m->first_iterator, (&it));
		frozen = 1;
	    }
	    if (t != m->table + hval)
		it.u.slot = t;

	    eq = is_eq(&k->key, key);
	    if (it.u.slot) {
		t = it.u.slot;
		k = *t;
	    }
	    if (!eq || keypair_deleted(k)) continue;
	}

	if (overwrite) {
	    if (overwrite==2) {
		assign_svalue(&k->key, key);
	    }
	    assign_svalue(&k->u.val, val);
	}

	goto unfreeze;
    }

    k = ba_lalloc(&m->allocator);
    k->next = *t;
    k->hval = hval;
    assign_svalue_no_free(&k->key, key);
    assign_svalue_no_free(&k->u.val, val);
    *t = k;

unfreeze:
    if (frozen)
	mapping_it_exit(&it);
}

PMOD_EXPORT void mapping_insert(struct mapping *m,
				const struct svalue *key,
				const struct svalue *val) {
    if (IS_UNDEFINED(key)) {
	Pike_error("undefined is not a proper key.\n");
    }
    /* TODO: would 2 not be better? */
    low_mapping_insert(m, key, val, 1);
}

static struct keypair * really_low_mapping_lookup(struct mapping *m, const struct svalue * key) {
    unsigned INT32 hval = hash_svalue(key);
    struct keypair ** t, * k;
    struct mapping_iterator it = { NULL, NULL, NULL, 0, 0 };
    int frozen = 0;

    t = m->table + hval & m->hash_mask;

    for (;*t; t = &(*t->next)) {
	int cmp = reverse_cmp(hval, (*t)->hval);
	k = *t;
	if (cmp == 1) continue;
	if (cmp == -1) break;
	if (keypair_deleted(k) || is_nidentical(&k->key, key)) continue;

	if (!is_identical(&k->key, key)) {
	    int eq;
	    if (!frozen) {
		DOUBLELINK(m->first_iterator, (&it));
		frozen = 1;
	    }
	    if (t != m->table + hval)
		it.u.slot = t;

	    eq = is_eq(&k->key, key);
	    if (it.u.slot) {
		t = it.u.slot;
		k = *t;
	    }
	    if (!eq || keypair_deleted(k)) continue;
	}

	goto unfreeze;
    }
    k = NULL;

unfreeze:
    if (frozen)
	mapping_it_exit(&it);

    return k;
}

PMOD_EXPORT union anything *mapping_get_item_ptr(struct mapping *m,
				     const struct svalue *key,
				     TYPE_T t) {
    struct keypair * k = really_low_mapping_lookup(m, key);
    return &k->val.u;
}

PMOD_EXPORT void map_delete_no_free(struct mapping *m,
			const struct svalue *key,
			struct svalue *to) {
    unsigned INT32 hval = hash_svalue(key);
    struct keypair ** t, * k;
    struct mapping_iterator it = { NULL, NULL, NULL, 0, 0 };
    int frozen = 0;

    t = m->table + hval & m->hash_mask;

    for (;*t; t = &(*t->next)) {
	int cmp = reverse_cmp(hval, (*t)->hval);
	k = *t;
	if (cmp == 1) continue;
	if (cmp == -1) break;
	if (keypair_deleted(k) || is_nidentical(&k->key, key)) continue;

	if (!is_identical(&k->key, key)) {
	    int eq;
	    if (!frozen) {
		DOUBLELINK(m->first_iterator, (&it));
		frozen = 1;
	    }
	    if (t != m->table + hval)
		it.u.slot = t;

	    eq = is_eq(&k->key, key);
	    if (it.u.slot) {
		t = it.u.slot;
		k = *t;
	    }
	    if (!eq || keypair_deleted(k)) continue;
	}

	goto unfreeze;
    }
    k = NULL;

unfreeze:
    if (frozen)
	mapping_it_exit(&it);

    if (!k) {
	if (to) {
	    SET_SVAL(*to, T_INT, NUMBER_UNDEFINED, integer, 0);
	}
	return;
    }

    free_svalue(&k->key);

    if (to) {
	move_svalue(to, &k->u.val);
    } else {
	free_svalue(&k->u.key);
    }

    *t = k->next;

    if (m->first_iterator) {
	k->u.next = m->trash;
	m->trash = k;
    } else {
	ba_lfree(&m->allocator, k);
    }
}

PMOD_EXPORT void check_mapping_for_destruct(struct mapping *m);
PMOD_EXPORT struct svalue *low_mapping_lookup(struct mapping *m,
					      const struct svalue *key) {
    struct keypair * k = really_low_mapping_lookup(m, key);
    return &k->val;
}
PMOD_EXPORT struct svalue *low_mapping_string_lookup(struct mapping *m,
                                                     struct pike_string *p);
PMOD_EXPORT void mapping_string_insert(struct mapping *m,
                                       struct pike_string *p,
                                       const struct svalue *val);
PMOD_EXPORT void mapping_string_insert_string(struct mapping *m,
				  struct pike_string *p,
				  struct pike_string *val);
