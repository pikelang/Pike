#include "svalue.h"
#include "gjalloc.h"
#include "pike_macros.h"

// these could probably use the same allocator
static struct block_allocator mapping_allocator = BA_INIT(sizeof(struct mapping), 256),
			      iter_allocator = BA_INIT(sizeof(struct mapping_iterator), 128);

static void unlink_iterator(struct mapping_iterator * it) {
    struct mapping * m = it->m;
    DOUBLEUNLINK(m->first_iterator, it);
}


void mapping_rel_simple(void * _start, void * _stop, ptrdiff_t diff, void * data) {
    struct keypair * k = (struct keypair*)_start;
    struct mapping m * = (struct mapping *)data;
    struct mapping_iterator * it;
    unsigned INT32 i;

    do {
	ba_simple_rel_pointer(&k->next);
	if (keypair_deleted(k)) {
	    ba_simple_rel_pointer(&k->u.next);
	}
    } while ((void*)(++k) < _stop);

    ba_simple_rel_pointer(&m->trash);

    for (it = m->first_iterator; it; it = it->next) {
	if ((void*)it->current >= _start && (void*)it->current < _stop)
	    ba_simple_rel_pointer(&it->current);
    }

    for (i = 0; i <= m->hash_mask; i++) {
	ba_simple_rel_pointer(m->table + i);
    }
}

PMOD_EXPORT void really_free_mapping(struct mapping *m) {
}

struct mapping *gc_internal_mapping = 0;
static struct mapping *gc_mark_mapping_pos = 0;
struct mapping * first_mapping = NULL;

ATTRIBUTE((malloc))
PMOD_EXPORT struct mapping *debug_allocate_mapping(int size) {
    struct mapping * m = (struct mapping *)ba_alloc(&mapping_allocator);
    unsigned INT32 t = (1 << INITIAL_MAGNITUDE);
    unsigned INT32 mag = 32 - INITIAL_MAGNITUDE;
    m->size = 0;
    m->reenter = 0;

    ba_init_local(&m->allocator, sizeof(struct keypair), size, 256, mapping_rel_simple, m);

    size /= AVG_CHAIN_LENGTH;

    while (size > t) {
	mag--;
	t *= 2;
    }

    m->hash_mask = t - 1;
    m->magnitude = mag;
    m->table = (struct keypair **)xalloc(sizeof(struct keypair **)*t);
    MEMSET((char*)m->table, 0, sizeof(struct keypair **)*t);
    m->trash = NULL;
    m->first_iterator = NULL;
    /* this allows for faking mapping_data */
    m->data = m;
    m->flags = 0;
    m->key_types = m->val_types = 0;

    DOUBLELINK(first_mapping, m);

    return m;
}

static INLINE void free_keypairs(void * _start, void * _stop, void * d) {
    struct keypair * k = (struct keypair*)_start;

    do {
	if (keypair_deleted(k)) continue;
	free_svalue(k->key);
	free_svalue(k->u.val);
    } while ((void*)(++k) < _stop);
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

static INLINE int reverse_cmp(unsigned INT32 a, unsigned INT32 b) {
    const unsigned INT32 t = a ^ b;

    if (t) {
	/* if there is no machine instruction for ctz, something like
	 *
	 * m = (1 + (t^(t-1))) >> 1
	 *
	 * would work, too
	 */
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

/* The idea here is to byteswap the hash value so that the most significant bits are
 * high in entropy.
 */
static void unmark_and_unlink(struct mapping_iterator * it) {
    mark_leave(&it->m->marker);
    unlink_iterator(it);
}

PMOD_EXPORT void mapping_fix_type_field(struct mapping *m) {}
PMOD_EXPORT void mapping_set_flags(struct mapping *m, int flags) {}
PMOD_EXPORT void low_mapping_insert(struct mapping *m,
				    const struct svalue *key,
				    const struct svalue *val,
				    int overwrite) {
    const unsigned INT32 hval = low_mapping_hash(key);
    struct keypair ** t, * k;
    int frozen = 0;
    ONERROR err;

    mark_enter(&m->marker);

    /* this is just for optimization */
    ba_lreserve(&m->allocator, 1);

    m->key_types |= 1 << TYPEOF(*key);
    m->val_types |= 1 << TYPEOF(*val);

    t = low_get_bucket(m, hval);

    for (;*t; t = &(*t->next)) {
	k = *t;
#if PIKE_DEBUG
	if (keypair_deleted(k)) Pike_error("ran into deleted keypair.");
#endif
	if (hval > (*t)->hval) continue;
	if (hval < (*t)->hval) break;
	if (is_nidentical(&k->key, key)) continue;

	if (!is_identical(&k->key, key)) {
	    int eq;
	    if (!frozen) {
		SET_ONERROR(err, mark_leave, &m->marker);
		frozen = 1;
	    }

	    if (!is_eq(&k->key, key)) continue;
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
    if (t == low_get_bucket(m, hval) && !(k->next)) {
	// TODO: we add into an empty bucket here, so we have to rechain the list, by finding
	// the next and prev item. this would make iteration much easier.
    }
    *t = k;
    m->size ++;

unfreeze:
    if (frozen) {
	UNSET_ONERROR(err);
	mark_leave(&m->marker);
    }
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

static struct keypair ** really_low_mapping_lookup(struct mapping *m, const struct svalue * key) {
    const unsigned INT32 hval = low_mapping_hash(key);
    struct keypair ** t;
    int frozen = 0;

    if (!(m->key_types & (1 << TYPEOF(*key)))) return NULL;

    mark_enter(&m->marker);

    t = low_get_bucket(m, hval);

    for (;*t; t = &(*t->next)) {
	const struct keypair * k = *t;
#if PIKE_DEBUG
	if (keypair_deleted(k)) Pike_error("ran into deleted keypair.");
#endif
	if (hval > (*t)->hval) continue;
	if (hval < (*t)->hval) break;
	if (is_nidentical(&k->key, key)) continue;

	if (!is_identical(&k->key, key)) {
	    int eq;
	    if (!frozen) {
		SET_ONERROR(err, mark_leave, &m->marker);
		frozen = 1;
	    }

	    if (!is_eq(&k->key, key)) continue;
	}

	goto unfreeze;
    }
    t = NULL;

unfreeze:
    if (frozen) {
	UNSET_ONERROR(err);
    }
    mark_leave(&m->marker);

    return t;
}

/* set iterator to continue iteration from k
 */
static INLINE void mapping_it_set(struct mapping_iterator * it, const struct svalue * key) {
    it->current = really_low_mapping_lookup(it->m, key);
}


PMOD_EXPORT union anything *mapping_get_item_ptr(struct mapping *m,
				     const struct svalue *key,
				     TYPE_T t) {
    struct keypair ** t = really_low_mapping_lookup(m, key);
    return *t ? &(*t)->u.val.u : NULL;
}

PMOD_EXPORT void map_delete_no_free(struct mapping *m,
			const struct svalue *key,
			struct svalue *to) {
    const unsigned INT32 hval = low_mapping_hash(key);
    struct keypair * k;
    struct mapping_iterator it;
    int frozen = 0;
    ONERROR err;

    it.m = m;
    if (!(m->key_types & (1 << TYPEOF(*key)))) return NULL;

    mark_enter(&m->marker);

    it.current = low_get_bucket(m, hval);

    for (;k = mapping_it_current(&it); it->current = &(*it->current)->next) {
#if PIKE_DEBUG
	if (keypair_deleted(k)) Pike_error("ran into deleted keypair.");
#endif
	if (hval > k->hval) continue;
	if (hval < k->hval) break;
	if (is_nidentical(&k->key, key)) continue;

	if (!is_identical(&k->key, key)) {
	    int eq;
	    if (!frozen) {
		SET_ONERROR(err, mark_leave, &m->marker);
		frozen = 1;
	    }

	    if (!is_eq(&k->key, key)) continue;
	}

	goto unfreeze;
    }
    k = NULL;

unfreeze:
    if (frozen) {
	UNSET_ONERROR(err);
	mark_leave(&m->marker);
    }

    if (!it->current) {
	if (to) {
	    SET_SVAL(*to, T_INT, NUMBER_UNDEFINED, integer, 0);
	}
	return;
    }

    free_svalue(&k->key);

    if (to) {
	move_svalue(to, &k->u.val);
    } else {
	free_svalue(&k->u.val);
    }

    mapping_it_delete(&it);
}

PMOD_EXPORT void check_mapping_for_destruct(struct mapping *m);
PMOD_EXPORT struct svalue *low_mapping_lookup(struct mapping *m,
					      const struct svalue *key) {
    struct keypair ** t = really_low_mapping_lookup(m, key);
    return *t ? &(*t)->u.val : NULL;
}

static INLINE void fill_indices(void * _start, void * _stop, void * data) {
    struct keypair * k = (struct keypair *)_start;
    struct array * a = (struct array *)data;
    unsigned INT32 size = a->size;

    do {
	if (!keypair_deleted(k)) {
	    assign_svalue_no_free(ITEM(a)+size, &k->key);
	    size++;
	}
    } while ((void*)(++k) < stop);

    a->size = size;
}

PMOD_EXPORT struct array *mapping_indices(struct mapping *m) {
    struct array * a = real_allocate_array(0, m->size);

    ba_walk_local(&m->allocator, fill_indices, a);

    return a;
}

static INLINE void fill_values(void * _start, void * _stop, void * data) {
    struct keypair * k = (struct keypair *)_start;
    struct array * a = (struct array *)data;

    unsigned INT32 size = a->size;

    do {
	if (!keypair_deleted(k)) {
	    assign_svalue_no_free(ITEM(a)+size, &k->u.val);
	    size++;
	}
    } while ((void*)(++k) < stop);

    a->size = size;
}

PMOD_EXPORT struct array *mapping_values(struct mapping *m) {
    struct array * a = real_allocate_array(0, m->size);

    ba_walk_local(&m->allocator, fill_values, a);

    return a;
}

static INLINE void fill_both(void * _start, void * _stop, void * data) {
    struct keypair * k = (struct keypair *)_start;
    struct array * a = (struct array *)data;

    unsigned INT32 size = a->size;

    do {
	if (!keypair_deleted(k)) {
	    struct array *b=allocate_array(2);
	    assign_svalue(b->item+0, & k->key);
	    assign_svalue(b->item+1, & k->u.val);
	    b->type_field = (1 << TYPEOF(k->key)) | (1 << TYPEOF(k->u.val));
	    SET_SVAL(*(ITEM(a)+size), T_ARRAY, 0, array, b);
	    size++;
	}
    } while ((void*)(++k) < stop);

    a->size = size;
}

PMOD_EXPORT struct array *mapping_to_array(struct mapping *m) {
    struct array * a = real_allocate_array(0, m->size);

    ba_walk_local(&m->allocator, fill_both, a);

    return a;
}

struct replace_data {
    struct mapping * m;
    struct svalue * from, * to;
};

static INLINE replace_cb(void * _start, void * _stop, void * data) {
    struct keypair * k = (struct keypair *)_start;
    struct replace_data * r = (struct replace_data*)data;

    do {
	if (keypair_deleted(k)) continue;
	if (!is_nidentical(r->from, &k->u.val)) {
	    if (is_identical(r->from, &k->u.val) || is_eq(r->from, &k->u.val)) {
		assign_svalue(&k->u.val, r->to);
	    }
	}
    } while ((void*)(++k) < stop);

    return BA_CONTINUE;
}

PMOD_EXPORT void mapping_replace(struct mapping *m,struct svalue *from, struct svalue *to) {
    const struct replace_data r = { m, from, to };
    mark_enter(&m->marker);
    ba_walk_local(&m->allocator, replace_cb, &r);
    mark_leave(&m->marker);
}

/* TODO
 * The above functions iterate over the mapping completely. they should update the type fields
 */

PMOD_EXPORT struct mapping *mkmapping(struct array *ind, struct array *val) {
    struct mapping *m;
    struct svalue *i,*v;
    INT32 e;

#ifdef PIKE_DEBUG
    if(ind->size != val->size)
	Pike_fatal("mkmapping on different sized arrays.\n");
#endif

    m=debug_allocate_mapping(ind->size);
    i=ITEM(ind);
    v=ITEM(val);
    for(e=0;e<ind->size;e++) low_mapping_insert(m, i++, v++, 2);

    return m;
}

PMOD_EXPORT struct mapping *copy_mapping(struct mapping *m) {
    struct mapping * n = debug_allocate_mapping(m->size);
    struct mapping_iterator it;
    unsigned INT32 e;
    struct keypair * k;

    mapping_builder_init(&it, n);

    for (e = 0; e <= m->hash_mask; e++) {
	struct keypair * k;
	for (k = m->table[e]; k; k = k->next) {
	    mapping_builder_add(&it, k);
	}
    }

    mapping_builder_exit(&it);

    return n;
}

struct mapping_op_context {
    struct mapping_iterator ita, itb, builder;
}

static void cleanup_mapping_op(struct mapping_op_context * c) {
    /* these iterators are virtual, so we dont need to unlink them */
    mark_leave(&c->ita.m->marker);
    mark_leave(&c->itb.m->marker);
    do_free_mapping(c->builder->m);
}

static struct mapping *or_mappings(const struct mapping *a, const struct mapping *b) {
    // TODO: we want to use restrict here, since the keypairs can never alias
    struct mapping * ret;
    struct keypair * __restrict__ ka, * __restrict__ kb; 
    struct mapping_op_context c;
    struct keypair ** t;
    struct mapping_iterator * itp;
    ONERROR err;

    if (a->size < b->size) {
	struct mapping * t = a;
	a = b;
	b = t;
    }

    if (!b->size) return copy_mapping(b);

    /* TODO: maybe this is too much, but relocation is rather expensive */
    ret = debug_allocate_mapping(a->size + b->size);

    mapping_builder_init(&c.builder, ret);
    ret->first_iterator = NULL;

    c.ita.m = a;
    c.itb.m = b;

    mapping_it_reset(&c.ita);
    mapping_it_reset(&c.itb);

    mark_enter(&a->marker);
    mark_enter(&b->marker);
    SET_ONERROR(err, cleanup_mapping_op, &c);

    while (1) {
	struct keypair * s;

	if (ka->hval > kb->hval) {
	    t = &kb;
	    itp = &c.itb;
	    if (keypair_deleted(t)) goto skip;
	    goto insert;
	}

	t = &ka;
	itp = &c.ita;

	if (keypair_deleted(t)) goto skip;

	if (ka->hval < kb->hval)
	    goto insert;

	s = ka;

	/*
	 * scan through all keypair with identical hash value and
	 * skip those that are already in the other mapping
	 */
	for (s = ka; s && s->hval == ka->hval; s = s->u.next) {
	    if (is_identical(&s->key, &kb->key)) goto skip;
	    if (is_nidentical(&s->key, &kb->key)) continue;
	    if (is_eq(&s->key, &kb->key)) goto skip;
	}
insert:
	mapping_builder_add(&c.builder, *t);
skip:
	if (!mapping_it_next(itp, t)) {
	    goto insert_rest;
	}
    }

insert_rest:
    if (itp == &ita) {
	itp = &c.itb;
	t = &kb;
    } else {
	itp = &c.ita;
	t = &ka;
    }

    do {
	mapping_builder_add(&c.builder, *t);
    } while (mapping_it_next(itp, t));

    UNSET_ONERROR(err);
    mark_leave(&a->marker);
    mark_leave(&b->marker);

    return ret;
}

static struct mapping *and_mappings(const struct mapping *a, const struct mapping *b) {
    // TODO: we want to use restrict here, since the keypairs can never alias
    struct mapping * ret;
    struct keypair * __restrict__ ka, * __restrict__ kb; 
    struct mapping_op_context c;
    struct keypair ** t;
    struct mapping_iterator * itp;
    ONERROR err;

    if (a->size < b->size) {
	struct mapping * t = a;
	a = b;
	b = t;
    }

    if (!b->size) return debug_allocate_mapping(0);

    /* TODO: maybe this is too much, but relocation is rather expensive */
    ret = debug_allocate_mapping(b->size);

    mapping_builder_init(&c.builder, ret);
    ret->first_iterator = NULL;

    c.ita.m = a;
    mapping_it_reset(&c.ita);

    c.itb.m = b;
    mapping_it_reset(&c.itb);

    mark_enter(&a->marker);
    mark_enter(&b->marker);
    SET_ONERROR(err, cleanup_mapping_op, &c);

    while (1) {
	struct keypair * s;

	if (ka->hval > kb->hval) {
	    t = &kb;
	    itp = &c.itb;
	    goto skip;
	}

	t = &ka;
	itp = &c.ita;

	if (keypair_deleted(t)) goto skip;

	if (ka->hval < kb->hval)
	    goto skip;

	s = ka;

	/*
	 * scan through all keypair with identical hash value and
	 * insert those which are also in the other mapping. they
	 * will be skipped when iterated over by the code above
	 */
	for (s = ka; s && s->hval == ka->hval; s = s->u.next) {
	    if (is_identical(&s->key, &kb->key)) goto insert;
	    if (is_nidentical(&s->key, &kb->key)) continue;
	    if (is_eq(&s->key, &kb->key)) goto insert;
	}
insert:
	mapping_builder_add(&c.builder, *t);
skip:
	if (!mapping_it_next(itp, t)) {
	    goto insert_rest;
	}
    }

insert_rest:
    if (itp == &ita) {
	itp = &c.itb;
	t = &kb;
    } else {
	itp = &c.ita;
	t = &ka;
    }

    do {
	mapping_builder_add(&c.builder, *t);
    } while (mapping_it_next(itp, t));

    UNSET_ONERROR(err);
    mark_leave(&a->marker);
    mark_leave(&b->marker);

    return ret;
}

static INLINE void xor_bucket(struct keypair * k1, struct keypair * k2, struct mapping_builder * b) {
    struct keypair * s;

    for (; k1 && k1->hval == k2->hval; k1 = k1->u.next) {
	for (s = k2; s && s->hval == k1->hval; s = s->u.next) {
	    if (is_nidentical(&s->key, &k1->key)) continue;
	    if (is_identical(&s->key, &k1->key) || is_eq(&s->key, &k1->key)) goto try_next;
	}
	mapping_builder_add(b, k1);
try_next:
    }
}

static struct mapping *xor_mappings(const struct mapping *a, const struct mapping *b) {
    // TODO: we want to use restrict here, since the keypairs can never alias
    struct mapping * ret;
    struct keypair * __restrict__ ka, * __restrict__ kb; 
    struct mapping_op_context c;
    struct keypair ** t;
    struct mapping_iterator * itp;
    ONERROR err;

    if (a->size < b->size) {
	struct mapping * t = a;
	a = b;
	b = t;
    }

    if (!b->size) return debug_allocate_mapping(0);

    /* TODO: maybe this is too much, but relocation is rather expensive */
    ret = debug_allocate_mapping(b->size);

    mapping_builder_init(&c.builder, ret);
    ret->first_iterator = NULL;

    c.ita.m = a;
    c.itb.m = b;

    mapping_it_reset(&c.ita);
    mapping_it_reset(&c.itb);

    mark_enter(&a->marker);
    mark_enter(&b->marker);

    SET_ONERROR(err, cleanup_mapping_op, &c);

    while (1) {
	unsigned INT32 hval;
	struct keypair * s;

	if (ka->hval > kb->hval) {
	    t = &kb;
	    itp = &c.itb;
#ifdef PIKE_DEBUG
	    if (keypair_deleted(*t)) Pike_error("ran into deleted keypair.");
#endif

	    goto insert;
	}

	t = &ka;
	itp = &c.ita;

#ifdef PIKE_DEBUG
	if (keypair_deleted(*t)) Pike_error("ran into deleted keypair.");
#endif

	if (ka->hval < kb->hval)
	    goto insert;

	/*
	 * scan through all keypair with identical hash value and
	 * insert those which are not in the other mapping.
	 * do the same for the other mapping and then continue on the next chain
	 */
	xor_bucket(ka, kb, &c.builder);
	xor_bucket(kb, ka, &c.builder);
	hval = kb->hval;

	while (hval == kb->hval && mapping_it_next(&c.itb, &kb));
	while (hval == ka->hval && mapping_it_next(&c.itb, &ka));

	if (!mapping_it_current(&c.ita)) {
	    if (!mapping_it_current(&c.itb)) goto done;
	    goto insert_b;
	}

	if (!mapping_it_current(&c.itb)) goto insert_a;
	continue;
insert:
	mapping_builder_add(&c.builder, *t);
skip:
	if (!mapping_it_next(itp, t)) {
	    goto insert_rest;
	}
    }

insert_rest:
    if (itp == &ita) {
insert_b:
	itp = &c.itb;
	t = &kb;
    } else {
insert_a:
	itp = &c.ita;
	t = &ka;
    }

    do {
	mapping_builder_add(&c.builder, *t);
    } while (mapping_it_next(itp, t));

done:

    UNSET_ONERROR(err);
    mark_leave(&a->marker);
    mark_leave(&b->marker);

    return ret;
}

PMOD_EXPORT struct mapping *merge_mappings(struct mapping *a, struct mapping *b, INT32 op) {
  switch (op) {
  case PIKE_ARRAY_OP_SUB:
    return subtract_mappings(a, b);
  case PIKE_ARRAY_OP_AND:
    return and_mappings(a, b);
  case PIKE_ARRAY_OP_OR:
    return or_mappings(a, b);
  case PIKE_ARRAY_OP_XOR:
    return xor_mappings(a, b);
  }
  Pike_error("unsupported operation\n");
}

PMOD_EXPORT struct mapping *merge_mapping_array_ordered(struct mapping *a, struct array *b, INT32 op) {
    int i;
    struct svalue * key = ITEM(b);

    if (op == PIKE_ARRAY_OP_SUB) {
	a = copy_mapping(a);

	for (i = 0; i < b->size; i++) {
	    map_delete_no_free(a, key++, NULL);
	}

	return a;
    } else if (op == PIKE_ARRAY_OP_AND) {
	struct mapping * ret = debug_allocate_mapping(b->size);
	
	for (i = 0; i < b->size; i++) {
	    struct keypair ** t = really_low_mapping_lookup(a, key);
	    if (*t) low_mapping_insert(ret, &(*t)->key, &(*t)->u.val, 0);
	}
	
	return ret;
    } else Pike_error("unsupported operation");
}

PMOD_EXPORT struct mapping *merge_mapping_array_unordered(struct mapping *a, struct array *b, INT32 op) {
    return merge_mapping_array_unordered(a, b, op);
}

PMOD_EXPORT int mapping_equal_p(struct mapping *a, struct mapping *b, struct processing *p) {
    struct mapping_iterator ita, itb;
    struct keypair * ka, * kb;
    struct processing curr;

    if (a==b) return 1;
    if (m_sizeof(a) != m_sizeof(b)) return 0;
    if (!((a->key_types & b->key_types) | (a->val_types & b->val_types))) return 0;

    curr.pointer_a = a;
    curr.pointer_b = b;
    curr.next = p;

    for( ;p ;p=p->next)
	if(p->pointer_a == (void *)a && p->pointer_b == (void *)b) return 1;

    ita.m = a;
    itb.m = b;

    mapping_it_reset(&ita);
    mapping_it_reset(&itb);

    do {
	if (ka->hval != kb->hval) return 0;
	if (is_identical(&ka->key, &kb->key) && is_identical(&ka->u.val, &kb->u.val)) continue;
	if (!low_is_equal(&ka->key, &kb->key, &curr) 
	    || !low_is_equal(&ka->u.val, &kb->u.val, &curr)) return 0;
    } while (mapping_it_next(&ita, ka) && mapping_it_next(&itb, kb));

    return 1;
}

void describe_mapping(struct mapping *m,struct processing *p,int indent);
node *make_node_from_mapping(struct mapping *m);

PMOD_EXPORT void f_aggregate_mapping(INT32 args) {
    INT32 e;
    struct mapping * m = debug_allocate_mapping(args/2);

    for (e = -args; e < 0; e += 2) {
	STACK_LEVEL_START(-e);
	low_mapping_insert(m, Pike_sp+e, Pike_sp+e+1, 2);
	STACK_LEVEL_DONE(-e);
    }
    pop_n_elems(args);
    push_mapping(m);
}

PMOD_EXPORT struct mapping *copy_mapping_recursively(struct mapping *m,
						     struct mapping *p);
PMOD_EXPORT void mapping_search_no_free(struct svalue *to,
			    struct mapping *m,
			    const struct svalue *look_for,
			    const struct svalue *key );
PMOD_EXPORT INT32 mapping_generation(struct mapping *m);

static void m_foreach(void * _start, void * _stop, void * d) {
    struct mapping * m = (struct mapping *)_start;
    void (*fun)(struct mapping *) = d;

    do {
	fun(m);
    } while ((void*)(++m) < _stop);

    return BA_CONTINUE;
}

#ifdef PIKE_DEBUG
void check_mapping(const struct mapping *m) {}
void check_all_mappings(void) {
    ba_walk(&mapping_allocator, m_foreach, check_mapping);
}
#endif

static int visit_keypairs(void * _start, void * _stop, void * d) {
    int * ref_type = (int*)d;
    struct keypair * k = (struct keypair *)_start;

    do {
	if (keypair_deleted(k)) continue;
	visit_svalue(&k->key, ref_type[0]);
	visit_svalue(&k->u.val, ref_type[1]);
    } while ((void*)(++k) < _stop);

    return BA_CONTINUE;
}

PMOD_EXPORT void visit_mapping (struct mapping *m, int action) {
    int ref_type[2];
    TYPE_FIELD types = m->key_types | m->val_types;

    if (action & VISIT_COMPLEX_ONLY) {
	types &= BIT_COMPLEX;
    } else {
	types &= BIT_REF_TYPES;
    }

    if (types) {
	ref_types[0] = m->flags & MAPPING_WEAK_INDICES ? REF_TYPE_WEAK : REF_TYPE_NORMAL;
	ref_types[1] = m->flags & MAPPING_WEAK_VALUE ? REF_TYPE_WEAK : REF_TYPE_NORMAL;

	ba_walk_local(&m->allocator, visit_keypair, ref_type);
    }
    switch (action) {
    case VISIT_COUNT_BYTES:
	mc_counted_bytes += sizeof(struct mapping);
	mc_counted_bytes += ba_lcapacity(&m->allocator)*sizeof(struct keypair);
	break;
    }
}

void gc_mark_mapping_as_referenced(struct mapping *m);
void real_gc_cycle_check_mapping(struct mapping *m, int weak) {
}

struct keypair_foreach_cb {
    void (*kfun)(struct svalue *);
    void (*vfun)(struct svalue *);
};

static int keypair_check_foreach(void * _start, void * _stop, void * d) {
    struct keypair * k = (struct mapping *)_start;
    const struct keypair_foreach_cb * cb = (struct keypair_foreach_cb *)d;

    do {
	if (keypair_deleted(k)) continue;
	cb.kfun(&k->key, 1, " as mapping index");
	cb.vfun(&k->u.val, 1, " as mapping value");
    } while ((void*)(++k) < _stop);

    return BA_CONTINUE;
}

static void gc_check_mapping(struct mapping *m) {
    if (!((m->key_types | m->val_types) & BIT_COMPLEX)) return;
    GC_ENTER (m, T_MAPPING) {
	struct keypair_foreach_cb c;

	switch (m->flags & MAPPING_WEAK) {
	case MAPPING_WEAK:
	    c.kfun = debug_gc_check_weak_svalues;
	    c.vfun = debug_gc_check_weak_svalues;
	    break;
	case MAPPING_WEAK_INDICES:
	    c.kfun = debug_gc_check_weak_svalues;
	    c.vfun = debug_gc_check_svalues;
	    break;
	case MAPPING_WEAK_VALUES:
	    c.kfun = debug_gc_check_svalues;
	    c.vfun = debug_gc_check_weak_svalues;
	    break;
	case 0:
	    c.kfun = debug_gc_check_svalues;
	    c.vfun = debug_gc_check_svalues;
	    break;
	}

	ba_walk_local(&m->allocator, keypair_check_foreach, &c);
    } GC_LEAVE;
}

unsigned gc_touch_all_mappings(void) {
    ba_walk(&mapping_allocator, m_foreach, debug_gc_touch);
    return ba_count(&mapping_allocator);
}

void gc_check_all_mappings(void) {
    ba_walk(&mapping_allocator, m_foreach, gc_check_mapping);
}

#include "mapping_common.h"

void simple_describe_mapping(struct mapping *m);
void debug_dump_mapping(struct mapping *m);

int keypair_is_constant(void * _start, void * _stop, void * d) {
    int * ret = (int*)d;
    struct keypair * k = (struct keypair*)_start;
    static const TYPE_FIELD all = ~(TYPE_FIELD)0;
    
    do {
	if (keypair_deleted(k)) continue;
	if(!svalues_are_constant(&k->key, 1, all, p) ||
	   !svalues_are_constant(&k->u.val, 1, all, p)) {
	    *ret = 0;
	    return BA_STOP;
	}
    } while((void*)(++k) < _stop);

    return BA_CONTINUE;
}

int mapping_is_constant(struct mapping *m, struct processing *p) {
    int ret = 1;

    if (!m_sizeof(a)) return 1;
    if (!((m->key_types | m->val_types) & ~(BIT_INT|BIT_FLOAT|BIT_STRING))) return 1;
    
    ba_walk_local(&m->allocator, keypair_is_contant, &ret);

    return ret;
}

#include "mapping_common.h"
