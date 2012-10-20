#include "pike_error.h"
#include "bitvector.h"
#include "svalue.h"
#include "pike_macros.h"
#include "svalue.h"
#include "global.h"
#define EXPORT PMOD_EXPORT
#define ba_error Pike_error
#define round_up32_ ba_round_up32_
#define round_up32 ba_round_up32
#include "gjalloc/gjalloc.h"
#undef EXPORT
#undef round_up32_
#undef round_up32
#undef ba_error
#include <stdint.h>

#ifndef INITIAL_MAGNITUDE
# define INITIAL_MAGNITUDE	4
#endif

#ifndef AVG_CHAIN_LENGTH
# define AVG_CHAIN_LENGTH	4
#endif

#define MAPPING_WEAK_INDICES	2
#define MAPPING_WEAK_VALUES	4
#define MAPPING_WEAK		6
#define MAPPING_FLAG_WEAK	6 /* Compat. */

// TODO this is maybe not layouted in the best
// way possible?
struct keypair {
    struct keypair * next;
    struct svalue key;
    union {
	struct svalue val;
	struct keypair * next;
    } u;
    unsigned INT32 hval;
};

struct reentrance_marker {
    int marker;
};

struct mapping {
    PIKE_MEMORY_OBJECT_MEMBERS;
    unsigned INT32 hash_mask;
    unsigned INT32 magnitude;
    unsigned INT32 size;
    unsigned INT32 flags;
    TYPE_FIELD key_types, val_types;
    struct keypair ** table; 
    struct mapping_iterator * first_iterator;
    struct keypair * trash;
    struct ba_local allocator;
    struct mapping * next, * prev;
    struct mapping * data;
    struct reentrance_marker marker;
};

struct mapping_iterator {
    struct mapping_iterator * next, * prev;
    struct keypair ** current;
    struct mapping * m;
};

static void clear_marker(struct reentrance_marker * m) {
    m->marker = 0;
}

/* definitions */
PMOD_EXPORT void low_mapping_cleanup(struct mapping * m);
PMOD_EXPORT void mapping_fix_type_field(struct mapping *m);
PMOD_EXPORT void mapping_set_flags(struct mapping *m, int flags);
PMOD_EXPORT void low_mapping_insert(struct mapping *m,
				    const struct svalue *key,
				    const struct svalue *val,
				    int overwrite);
PMOD_EXPORT void really_free_mapping(struct mapping *m);
PMOD_EXPORT struct mapping *debug_allocate_mapping(int size);
PMOD_EXPORT void do_free_mapping(struct mapping *m);
PMOD_EXPORT void mapping_insert(struct mapping *m,
				const struct svalue *key,
				const struct svalue *val);
PMOD_EXPORT union anything *mapping_get_item_ptr(struct mapping *m,
				     const struct svalue *key,
				     TYPE_T t);

PMOD_EXPORT void map_delete_no_free(struct mapping *m,
			const struct svalue *key,
			struct svalue *to);
PMOD_EXPORT void check_mapping_for_destruct(struct mapping *m);
PMOD_EXPORT struct svalue *low_mapping_lookup(struct mapping *m,
					      const struct svalue *key);
PMOD_EXPORT struct keypair * mapping_lookup_random(const struct mapping * m);

PMOD_EXPORT struct array *mapping_indices(struct mapping *m);

PMOD_EXPORT struct array *mapping_values(struct mapping *m);
PMOD_EXPORT struct array *mapping_to_array(struct mapping *m);
PMOD_EXPORT void mapping_replace(struct mapping *m,struct svalue *from, struct svalue *to);

PMOD_EXPORT struct mapping *mkmapping(struct array *ind, struct array *val);

PMOD_EXPORT struct mapping *copy_mapping(struct mapping *m);

PMOD_EXPORT struct mapping *merge_mappings(struct mapping *a, struct mapping *b, INT32 op);
PMOD_EXPORT struct mapping *merge_mapping_array_ordered(struct mapping *a, struct array *b, INT32 op);
PMOD_EXPORT struct mapping *merge_mapping_array_unordered(struct mapping *a, struct array *b, INT32 op);
PMOD_EXPORT struct mapping *add_mappings(struct svalue *argp, INT32 args);
PMOD_EXPORT int mapping_equal_p(struct mapping *a, struct mapping *b, struct processing *p);
PMOD_EXPORT void f_aggregate_mapping(INT32 args);
PMOD_EXPORT struct mapping *copy_mapping_recursively(struct mapping *m,
						     struct mapping *p);

PMOD_EXPORT void mapping_search_no_free(struct svalue *to,
			    struct mapping *m,
			    const struct svalue *look_for,
			    const struct svalue *key );
PMOD_EXPORT INT32 mapping_generation(struct mapping *m);

PMOD_EXPORT struct svalue *low_mapping_string_lookup(struct mapping *m,
                                                     struct pike_string *p);
PMOD_EXPORT void mapping_string_insert(struct mapping *m,
                                       struct pike_string *p,
                                       const struct svalue *val);
PMOD_EXPORT void mapping_string_insert_string(struct mapping *m,
				  struct pike_string *p,
				  struct pike_string *val);
PMOD_EXPORT struct svalue *simple_mapping_string_lookup(struct mapping *m,
							const char *p);
PMOD_EXPORT struct svalue *mapping_mapping_lookup(struct mapping *m,
						  const struct svalue *key1,
						  const struct svalue *key2,
						  int create);
PMOD_EXPORT struct svalue *mapping_mapping_string_lookup(struct mapping *m,
				      struct pike_string *key1,
				      struct pike_string *key2,
				      int create);
PMOD_EXPORT void mapping_index_no_free(struct svalue *dest,
			   struct mapping *m,
			   const struct svalue *key);
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
int mapping_is_constant(struct mapping *m,
			struct processing *p);

#define allocate_mapping(X) dmalloc_touch(struct mapping *,debug_allocate_mapping(X))
#define visit_mapping_ref(M, REF_TYPE)				\
  visit_ref (pass_mapping (M), (REF_TYPE),			\
	     (visit_thing_fn *) &visit_mapping, NULL)
#define gc_cycle_check_mapping(X, WEAK) \
  gc_cycle_enqueue((gc_cycle_check_cb *) real_gc_cycle_check_mapping, (X), (WEAK))

static INLINE unsigned INT32 m_sizeof(const struct mapping * m) {
    return m->size;
}

static INLINE TYPE_FIELD m_ind_types(const struct mapping * m) {
    return m->key_types;
}

static INLINE TYPE_FIELD m_val_types(const struct mapping * m) {
    return m->val_types;
}

static INLINE INT16 mapping_get_flags(const struct mapping * m) {
    return (INT16)m->flags;
}

static INLINE void mark_enter(struct reentrance_marker * m) {
    if (m->marker) Pike_error("Side effect free method neentered\n");
    m->marker = 1; 
}

static INLINE void mark_leave(struct reentrance_marker * m) {
    if (!m->marker) {
	m->marker = 0;
	Pike_error("marker is zero\n");
    } else 
	m->marker = 0;
}

#define mapping_data_is_shared(m) ((m)->refs > 1)
#define map_delete(m,key) map_delete_no_free(m, key, 0)
#define keypair_ind(k)	((k)->key)
#define keypair_val(k)	((k)->u.val)

static INLINE void free_mapping(struct mapping * m) {
    if (!sub_ref(m)) {
	really_free_mapping(m);
    }
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

static INLINE void mapping_it_init(struct mapping_iterator * it, struct mapping * m) {
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

static INLINE struct keypair * mapping_it_current(struct mapping_iterator * it) {
    return it->current ? *it->current : NULL;
}

static INLINE void mapping_it_step(struct mapping_iterator * it) {
    struct keypair ** slot = it->current;
    const struct mapping * m = it->m;

    do {
	struct keypair * k = *slot;
	if (k->next) {
	    slot = &k->next;
	} else {
	    unsigned INT32 i = (k->hval >> m->magnitude) + 1;

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
	m->trash = k;
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
extern struct mapping *gc_internal_mapping;

#define mapping_data	mapping
#define MAPPING_LOOP(m)	for (e = 0; (unsigned INT32)e <= m->hash_mask; e++) for (k = m->table[e]; k; k = k->next)
#define NEW_MAPPING_LOOP(m)	MAPPING_LOOP(m)
