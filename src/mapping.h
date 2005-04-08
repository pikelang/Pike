/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: mapping.h,v 1.60 2005/04/08 16:55:53 grubba Exp $
*/

#ifndef MAPPING_H
#define MAPPING_H

#include "svalue.h"
#include "dmalloc.h"
#include "block_alloc_h.h"

/* Compatible with PIKE_WEAK_INDICES and PIKE_WEAK_VALUES. */
#define MAPPING_WEAK_INDICES	2
#define MAPPING_WEAK_VALUES	4
#define MAPPING_WEAK		6
#define MAPPING_FLAG_WEAK	6 /* Compat. */

struct keypair
{
  struct keypair *next;
  unsigned INT32 hval;
  struct svalue ind, val;
};

struct mapping_data
{
  PIKE_MEMORY_OBJECT_MEMBERS;
  INT32 valrefs; /* lock values too */
  INT32 hardlinks;
  INT32 size, hashsize;
  INT32 num_keypairs;
  TYPE_FIELD ind_types, val_types;
  INT16 flags;
  struct keypair *free_list;
  struct keypair *hash[1 /* hashsize */ ];
  /* struct keypair data_block[ hashsize * AVG_LINK_LENGTH ] */
};

#undef MAPPING_SIZE_DEBUG
/* This debug doesn't work with weak mappings in the gc. */

struct mapping
{
  PIKE_MEMORY_OBJECT_MEMBERS;
#ifdef MAPPING_SIZE_DEBUG
  INT32 debug_size;
#endif
  struct mapping_data *data;
  struct mapping *next, *prev;
};


extern struct mapping *first_mapping;
extern struct mapping *gc_internal_mapping;

#define map_delete(m,key) map_delete_no_free(m, key, 0)
#define m_sizeof(m) ((m)->data->size)
#define m_ind_types(m) ((m)->data->ind_types)
#define m_val_types(m) ((m)->data->val_types)
#define mapping_get_flags(m) ((m)->data->flags)
#define mapping_data_is_shared(m) ((m)->data->refs > 1)

#define MD_KEYPAIRS(MD, HSIZE) \
   ( (struct keypair *)							\
     DO_ALIGN( PTR_TO_INT(((struct mapping_data *)(MD))->hash + HSIZE),	\
	       ALIGNOF(struct keypair)) )

#ifndef PIKE_MAPPING_KEYPAIR_LOOP
#define NEW_MAPPING_LOOP(md) \
  for((e=0) DO_IF_DMALLOC( ?0:(debug_malloc_touch(md)) ) ;e<(md)->hashsize;e++) for(k=(md)->hash[e];k;k=k->next)

/* WARNING: this should not be used */
#define MAPPING_LOOP(m) \
  for((e=0) DO_IF_DMALLOC( ?0:(debug_malloc_touch(m),debug_malloc_touch((m)->data))) ;e<(m)->data->hashsize;e++) for(k=(m)->data->hash[e];k;k=k->next)

#else /* PIKE_MAPPING_KEYPAIR_LOOP */

#define NEW_MAPPING_LOOP(md) \
  for(((k = MD_KEYPAIRS(md, (md)->hashsize)), e=0) DO_IF_DMALLOC( ?0:(debug_malloc_touch(md)) ) ; e<(md)->size; e++,k++)

/* WARNING: this should not be used */
#define MAPPING_LOOP(m) \
  for(((k = MD_KEYPAIRS((m)->data, (m)->data->hashsize)), e=0) DO_IF_DMALLOC( ?0:(debug_malloc_touch(m),debug_malloc_touch((m)->data)) ) ; e<(m)->data->size; e++,k++)

#endif /* PIKE_MAPPING_KEYPAIR_LOOP */

#define free_mapping(M) do{						\
    struct mapping *m_=(M);						\
    debug_malloc_touch(m_);						\
    DO_IF_DEBUG (							\
      DO_IF_PIKE_CLEANUP (						\
	if (gc_external_refs_zapped)					\
	  gc_check_zapped (m_, PIKE_T_MAPPING, __FILE__, __LINE__)));	\
    if(!sub_ref(m_))							\
      really_free_mapping(m_);						\
  }while(0)

#define free_mapping_data(M) do{ \
 struct mapping_data *md_=(M); \
 debug_malloc_touch(md_); \
 if(!sub_ref(md_)) really_free_mapping_data(md_); \
 /* FIXME: What about valrefs & hardlinks? */ \
}while(0)

PMOD_PROTO void really_free_mapping(struct mapping *md);

/* Prototypes begin here */
BLOCK_ALLOC_FILL_PAGES(mapping, 2);






PMOD_EXPORT struct mapping *debug_allocate_mapping(int size);
PMOD_EXPORT void really_free_mapping_data(struct mapping_data *md);
PMOD_EXPORT void do_free_mapping(struct mapping *m);
struct mapping_data *copy_mapping_data(struct mapping_data *md);
PMOD_EXPORT void mapping_fix_type_field(struct mapping *m);
PMOD_EXPORT void mapping_set_flags(struct mapping *m, int flags);
PMOD_EXPORT void low_mapping_insert(struct mapping *m,
				    const struct svalue *key,
				    const struct svalue *val,
				    int overwrite);
PMOD_EXPORT void mapping_insert(struct mapping *m,
				const struct svalue *key,
				const struct svalue *val);
PMOD_EXPORT union anything *mapping_get_item_ptr(struct mapping *m,
				     struct svalue *key,
				     TYPE_T t);
PMOD_EXPORT void map_delete_no_free(struct mapping *m,
			struct svalue *key,
			struct svalue *to);
PMOD_EXPORT void check_mapping_for_destruct(struct mapping *m);
PMOD_EXPORT struct svalue *low_mapping_lookup(struct mapping *m,
					      const struct svalue *key);
PMOD_EXPORT struct svalue *low_mapping_string_lookup(struct mapping *m,
					 struct pike_string *p);
PMOD_EXPORT void mapping_string_insert(struct mapping *m,
			   struct pike_string *p,
			   struct svalue *val);
PMOD_EXPORT void mapping_string_insert_string(struct mapping *m,
				  struct pike_string *p,
				  struct pike_string *val);
PMOD_EXPORT struct svalue *simple_mapping_string_lookup(struct mapping *m,
							const char *p);
PMOD_EXPORT struct svalue *mapping_mapping_lookup(struct mapping *m,
				      struct svalue *key1,
				      struct svalue *key2,
				      int create);
PMOD_EXPORT struct svalue *mapping_mapping_string_lookup(struct mapping *m,
				      struct pike_string *key1,
				      struct pike_string *key2,
				      int create);
PMOD_EXPORT void mapping_index_no_free(struct svalue *dest,
			   struct mapping *m,
			   struct svalue *key);
PMOD_EXPORT struct array *mapping_indices(struct mapping *m);
PMOD_EXPORT struct array *mapping_values(struct mapping *m);
PMOD_EXPORT struct array *mapping_to_array(struct mapping *m);
PMOD_EXPORT void mapping_replace(struct mapping *m,struct svalue *from, struct svalue *to);
PMOD_EXPORT struct mapping *mkmapping(struct array *ind, struct array *val);
PMOD_EXPORT struct mapping *copy_mapping(struct mapping *m);
PMOD_EXPORT struct mapping *copy_mapping(struct mapping *m);
PMOD_EXPORT struct mapping *merge_mappings(struct mapping *a, struct mapping *b, INT32 op);
PMOD_EXPORT struct mapping *merge_mapping_array_ordered(struct mapping *a, 
					    struct array *b, INT32 op);
PMOD_EXPORT struct mapping *merge_mapping_array_unordered(struct mapping *a, 
					      struct array *b, INT32 op);
PMOD_EXPORT struct mapping *add_mappings(struct svalue *argp, INT32 args);
PMOD_EXPORT int mapping_equal_p(struct mapping *a, struct mapping *b, struct processing *p);
void describe_mapping(struct mapping *m,struct processing *p,int indent);
node *make_node_from_mapping(struct mapping *m);
PMOD_EXPORT void f_aggregate_mapping(INT32 args);
PMOD_EXPORT struct mapping *copy_mapping_recursively(struct mapping *m,
						     struct mapping *p);
PMOD_EXPORT void mapping_search_no_free(struct svalue *to,
			    struct mapping *m,
			    struct svalue *look_for,
			    struct svalue *key );
void check_mapping(struct mapping *m);
void check_all_mappings(void);
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
/* Prototypes end here */

#define allocate_mapping(X) dmalloc_touch(struct mapping *,debug_allocate_mapping(X))

#define gc_cycle_check_mapping(X, WEAK) \
  gc_cycle_enqueue((gc_cycle_check_cb *) real_gc_cycle_check_mapping, (X), (WEAK))

#endif /* MAPPING_H */
