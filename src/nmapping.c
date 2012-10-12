#include "svalue.h"
#include "gjalloc.h"
#include "pike_macros.h"

// these could probably use the same allocator
static struct block_allocator mapping_allocator = BA_INIT(sizeof(struct mapping), 256),
			      iter_allocator = BA_INIT(sizeof(struct mapping_iterator), 128);

static INLINE int keypair_deleted(const struct keypair * k) {
    return IS_UNDEFINED(&k->key);
}

static void unlink_iterator(struct mapping_iterator * it) {
    struct mapping * m = it->m;
    DOUBLEUNLINK(m->first_iterator, it);
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

PMOD_EXPORT struct mapping * first_mapping = NULL;

ATTRIBUTE((malloc))
PMOD_EXPORT struct mapping *debug_allocate_mapping(int size) {
    struct mapping * m = (struct mapping *)ba_alloc(&mapping_allocator);
    unsigned INT32 t = (1 << INITIAL_MAGNITUDE);
    unsigned INT32 mag = 32 - INITIAL_MAGNITUDE;
    m->size = 0;
    m->reenter = 0;

    size /= AVG_CHAIN_LENGTH;

    while (size > t) {
	mag--;
	t *= 2;
    }

    ba_init_local(&m->allocator, sizeof(struct keypair), t, 256, mapping_rel_simple, m);
    m->hash_mask = t - 1;
    m->magnitude = mag;
    m->table = (struct keypair **)xalloc(sizeof(struct keypair **)*t);
    MEMSET((char*)m->table, 0, sizeof(struct keypair **)*t);
    m->trash = NULL;
    m->first_iterator = NULL;
    /* this allows for faking mapping_data */
    m->data = m;

    DOUBLELINK(first_mapping, m);

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
static INLINE unsigned INT32 low_mapping_hash(const struct svalue * key) {
    return __builtin_bswap32(hash_svalue(key));
}

static INLINE struct keypair ** low_get_bucket(const struct mapping * m, unsigned INT32 hval) {
    return m->table + (hval >> m->magnitude);
}

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

    t = low_get_bucket(m, hval);

    for (;*t; t = &(*t->next)) {
	k = *t;
	if (hval > (*t)->hval) continue;
	if (hval < (*t)->hval) break;
	if (keypair_deleted(k) || is_nidentical(&k->key, key)) continue;

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

static struct keypair * really_low_mapping_lookup(struct mapping *m, const struct svalue * key) {
    const unsigned INT32 hval = low_mapping_hash(key);
    struct keypair ** t, * k;
    int frozen = 0;

    mark_enter(&m->marker);

    t = low_get_bucket(m, hval);

    for (;*t; t = &(*t->next)) {
	k = *t;
	if (hval > (*t)->hval) continue;
	if (hval < (*t)->hval) break;
	if (keypair_deleted(k) || is_nidentical(&k->key, key)) continue;

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

    return k;
}

PMOD_EXPORT union anything *mapping_get_item_ptr(struct mapping *m,
				     const struct svalue *key,
				     TYPE_T t) {
    struct keypair * k = really_low_mapping_lookup(m, key);
    return &k->u.val.u;
}

PMOD_EXPORT void map_delete_no_free(struct mapping *m,
			const struct svalue *key,
			struct svalue *to) {
    const unsigned INT32 hval = low_mapping_hash(key);
    struct keypair ** t, * k;
    int frozen = 0;
    ONERROR err;

    mark_enter(&m->marker);

    t = low_get_bucket(m, hval);

    for (;*t; t = &(*t->next)) {
	k = *t;
	if (hval > (*t)->hval) continue;
	if (hval < (*t)->hval) break;
	if (keypair_deleted(k) || is_nidentical(&k->key, key)) continue;

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

    m->size --;

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
    return &k->u.val;
}
PMOD_EXPORT struct svalue *low_mapping_string_lookup(struct mapping *m,
                                                     struct pike_string *p);
PMOD_EXPORT void mapping_string_insert(struct mapping *m,
                                       struct pike_string *p,
                                       const struct svalue *val);
PMOD_EXPORT void mapping_string_insert_string(struct mapping *m,
				  struct pike_string *p,
				  struct pike_string *val);
PMOD_EXPORT struct svalue *mapping_mapping_string_lookup(struct mapping *m,
				      struct pike_string *key1,
				      struct pike_string *key2,
				      int create);
PMOD_EXPORT void mapping_index_no_free(struct svalue *dest,
			   struct mapping *m,
			   const struct svalue *key);

static INLINE void fill_indices(void * _start, void * _stop, void * data) {
    struct keypair * start = (struct keypair *)_start,
		   * stop  = (struct keypair *)_stop;
    struct array * a = (struct array *)data;

    unsigned INT32 size = a->size;

    while (start < stop) {
	if (!keypair_deleted(start)) {
	    assign_svalue_no_free(ITEM(a)+size, &start->key);
	    size++;
	}
    }

    a->size = size;
}

PMOD_EXPORT struct array *mapping_indices(struct mapping *m) {
    struct array * a = real_allocate_array(0, m->size);

    ba_walk_local(&m->allocator, fill_indices, a);

    return a;
}

static INLINE void fill_values(void * _start, void * _stop, void * data) {
    struct keypair * start = (struct keypair *)_start,
		   * stop  = (struct keypair *)_stop;
    struct array * a = (struct array *)data;

    unsigned INT32 size = a->size;

    while (start < stop) {
	if (!keypair_deleted(start)) {
	    assign_svalue_no_free(ITEM(a)+size, &start->u.val);
	    size++;
	}
    }

    a->size = size;
}

PMOD_EXPORT struct array *mapping_values(struct mapping *m) {
    struct array * a = real_allocate_array(0, m->size);

    ba_walk_local(&m->allocator, fill_values, a);

    return a;
}

PMOD_EXPORT struct array *mapping_to_array(struct mapping *m);
PMOD_EXPORT void mapping_replace(struct mapping *m,struct svalue *from, struct svalue *to);
PMOD_EXPORT struct mapping *mkmapping(struct array *ind, struct array *val);

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
    c.ita.u.current = NULL;
    mapping_it_next(&c.ita, &ka);

    c.itb.m = b;
    c.itb.u.current = NULL;
    mapping_it_next(&c.itb, &kb);

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
    c.ita.u.current = NULL;
    mapping_it_next(&c.ita, &ka);

    c.itb.m = b;
    c.itb.u.current = NULL;
    mapping_it_next(&c.itb, &kb);

    mark_enter(&a->marker);
    mark_enter(&b->marker);
    SET_ONERROR(err, cleanup_mapping_op, &c);

    while (1) {
	unsigned INT32 hval;
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

	if (!c.ita.u.current) {
	    if (!c.itb.u.current) goto done;
	    goto insert_b;
	}

	if (!c.itb.u.current) goto insert_a;
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
	struct keypair * k;
	
	for (i = 0; i < b->size; i++) {
	    k = really_low_mapping_lookup(a, key);
	    if (k) low_mapping_insert(ret, &k->key, &k->u.val, 0);
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

  curr.pointer_a = a;
  curr.pointer_b = b;
  curr.next = p;

  for( ;p ;p=p->next)
    if(p->pointer_a == (void *)a && p->pointer_b == (void *)b)
      return 1;

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
PMOD_EXPORT void f_aggregate_mapping(INT32 args);
PMOD_EXPORT struct mapping *copy_mapping_recursively(struct mapping *m,
						     struct mapping *p);
PMOD_EXPORT void mapping_search_no_free(struct svalue *to,
			    struct mapping *m,
			    const struct svalue *look_for,
			    const struct svalue *key );
PMOD_EXPORT INT32 mapping_generation(struct mapping *m);
#ifdef PIKE_DEBUG
void check_mapping(const struct mapping *m);
void check_all_mappings(void);
#endif
PMOD_EXPORT void visit_mapping (struct mapping *m, int action);
void gc_mark_mapping_as_referenced(struct mapping *m);
void real_gc_cycle_check_mapping(struct mapping *m, int weak);
unsigned gc_touch_all_mappings(void);
void gc_check_all_mappings(void);
void gc_mark_all_mappings(void);
void gc_cycle_check_all_mappings(void);
void gc_zap_ext_weak_refs_in_mappings(void);
size_t gc_free_all_unreferenced_mappings(void);
void simple_describe_mapping(struct mapping *m);
void debug_dump_mapping(struct mapping *m);

int mapping_is_constant(struct mapping *m, struct processing *p) {
    unsigned INT32 e;
    struct keypair * k;
    static const TYPE_FIELD all = ~(TYPE_FIELD)0;

    if (!m_sizeof(a)) return 1;
    
    /* TODO: this could walk keypairs in memory order */
    MAPPING_LOOP(a) {
	if(!svalues_are_constant(&k->key, all, md->ind_types, p) ||
	   !svalues_are_constant(&k->u.val, all, md->val_types, p)) return 0;
    }

    return 1;
}
