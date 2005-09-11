/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: mapping.h,v 1.62 2005/09/11 00:40:10 grendel Exp $
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

/** Free a previously allocated mapping. The preferred method of freeing
  * a mapping is by calling the @ref do_free_mapping function.
  *
  * @param M The mapping to be freed
  * @see do_free_mapping
  * @see free_mapping_data
  */
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

/** Free only the mapping data leaving the mapping structure itself intact.
  *
  * @param M The mapping structure 'data' member of the mapping whose data is to be removed
  * @see free_mapping
  * @see really_free_mapping_data
  * @see mapping_data
  * @see mapping
  */
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

/** Function that actually frees the mapping data, called by the wrapper
  * macro free_mapping_data.
  *
  * @param M The mapping structure data member of the mapping whose data is to be removed
  * @see free_mapping
  * @see really_free_mapping_data
  * @see mapping_data
  * @see mapping
  */
PMOD_EXPORT void really_free_mapping_data(struct mapping_data *md);

/** A wrapper function for the free_mapping macro. Should be used instead of
  * the macro as it checks whether the passed mapping is NULL or not.
  *
  * @param m The mapping to be freed
  * @see free_mapping
  */
PMOD_EXPORT void do_free_mapping(struct mapping *m);

/** Makes a copy of the passed mapping data and returns it to the caller.
  *
  * @param md The mapping structure data member to be copied
  * @return Copy of the passed data
  * @see mapping
  */
struct mapping_data *copy_mapping_data(struct mapping_data *md);
PMOD_EXPORT void mapping_fix_type_field(struct mapping *m);
PMOD_EXPORT void mapping_set_flags(struct mapping *m, int flags);

/** This function inserts key:val into the mapping m.
 * Same as doing m[key]=val; in pike.
 *
 * @param overwrite how to deal with existing values@n
 *   @b 0: Do not replace the value if the entry exists.@n
 *   @b 1: Replace the value if the entry exists.@n
 *   @b 2: Replace both the index and the value if the entry exists.
 */
PMOD_EXPORT void low_mapping_insert(struct mapping *m,
				    const struct svalue *key,
				    const struct svalue *val,
				    int overwrite);

/** Inserts the specified key and value into the indicated mapping. If
  * the key already exists in the mapping, its value is replaced with the
  * new one. For other modes of dealing with existing keys you need to
  * use the @ref low_mapping_insert function.
  *
  * @param m mapping the key/value are to be inserted to
  * @param key the new entry key
  * @param value the new entry value
  * @see low_mapping_insert
  */
PMOD_EXPORT void mapping_insert(struct mapping *m,
				const struct svalue *key,
				const struct svalue *val);
PMOD_EXPORT union anything *mapping_get_item_ptr(struct mapping *m,
				     struct svalue *key,
				     TYPE_T t);

/** Deletes the specified entry from the indicated mapping and frees
  * the memory associated with it unless the @b to parameter is not
  * NULL, in which case the deleted item's value is assigned to the
  * @b to @ref svalue.
  *
  * @param m mapping containing the entry to delete
  * @param key the key which indexes the entry to delete
  * @param to if not NULL will contain the deleted item's value
  */
PMOD_EXPORT void map_delete_no_free(struct mapping *m,
			struct svalue *key,
			struct svalue *to);
PMOD_EXPORT void check_mapping_for_destruct(struct mapping *m);

/** Look up the specified key in the indicated mapping and return
  * the corresponding @ref svalue representing the value of the
  * entry.
  *
  * @param m mapping to check for the key presence
  * @param key key to look up in the mapping
  * @return an svalue representing the looked up entry's value or
  * NULL if the key was not found in the mapping.
  * @see low_mapping_string_lookup
  * @see simple_mapping_string_lookup
  */
PMOD_EXPORT struct svalue *low_mapping_lookup(struct mapping *m,
					      const struct svalue *key);

/** Look up an entry in the indicated mapping indexed by the passed @b Pike
  * string. It is a shortcut for the common case when the mapping is indexed
  * on Pike strings instead of using an @ref svalue and the @ref low_mapping_lookup
  * function.
  *
  * @param m mapping to check for the key presence
  * @param p a Pike string to use as the lookup key
  * @return an svalue representing the looked up entry's value or NULL of no such
  * item was found.
  * @see low_mapping_lookup
  * @see simple_mapping_string_lookup
  */
PMOD_EXPORT struct svalue *low_mapping_string_lookup(struct mapping *m,
					 struct pike_string *p);

/** A shortcut function for inserting an entry into a mapping for cases 
  * where the key is a Pike string.
  *
  * @param m mapping to insert the new entry into
  * @param p a Pike string to be used as the new entry key
  * @param val an svalue representing the new entry's value
  * @see mapping_insert
  * @see low_mapping_insert
  * @see mapping_string_insert_string
  */
PMOD_EXPORT void mapping_string_insert(struct mapping *m,
			   struct pike_string *p,
			   struct svalue *val);

/** A shortcut function for inserting an entry into a mapping for cases
  * where both the key and the value are Pike strings.
  *
  * @param m mapping to insert the new entry into
  * @param p a Pike string to be used as the new entry key
  * @param val a Pike string to be used as the new entry's value
  * @see mapping_string_insert
  * @see low_mapping_insert
  * @see mapping_string_insert
  */
PMOD_EXPORT void mapping_string_insert_string(struct mapping *m,
				  struct pike_string *p,
				  struct pike_string *val);

/** A shortcut function to look up an entry in a mapping using a C string
  * as the key. The function converts the C string into a Pike string and
  * then calls @ref low_mapping_string_lookup to do the work. If your code
  * looks up an item in mappings very often and you are using a constant C
  * string for that, it will be more beneficial to allocate a Pike string
  * yourself in the initialization section of your module and then use it
  * with @ref low_mapping_string_lookup instead of calling simple_mapping_string_lookup.
  * The advantage is not very big since this function allocates the Pike
  * string only once, the first time it is used (or when it gets garbage-
  * collected), but it is a small optimization nevertheless.
  *
  * @param m mapping to look up the entry in
  * @param p string to be used as the lookup key
  * @return an svalue representing the looked up entry's value or NULL if no
  * such entry was found
  * @see low_mapping_string_lookup
  * @see simple_mapping_string_lookup
  * @see low_mapping_lookup
  */
PMOD_EXPORT struct svalue *simple_mapping_string_lookup(struct mapping *m,
							const char *p);

/** First look up @b key1 in the passed mapping and check whether the returned
  * value, if any, is a mapping itself. If it is a mapping, look it up using 
  * @b key2 and return the retrieved entry's value, if any. If key1 lookup in
  * the @b m mapping doesn't return a mapping and @b create is not 0 allocate
  * a new mapping, insert it in @b m and use for lookup with @b key2. If @b key2
  * lookup doesn't yield a value in neither of the above cases and @b create is not
  * 0, allocate a new svalue representing an undefined number, insert it into the
  * mapping retrieved (or allocated) during the @b key1 lookup and then perform a
  * @b key2 lookup on the mapping retrieved (or allocated) during the @b key1 and
  * return the result.
  *
  * @param m primary mapping to perform a @b key1 lookup on
  * @param key1 key used to lookup an entry in the primary mapping @b m
  * @param key2 key used to lookup an entry in the secondary mapping retrieved
  * (or allocated) as the result of the @b key1 lookup
  * @param create 0 to not insert an entry into neither mapping if it cannot be
  * found in the mapping, not 0 to insert such entry.
  * @return the result of the @b key2 lookup or 0 if either @b key1 or @b key2
  * returned no value and @b create was 0
  *
  * @see low_mapping_lookup
  */
PMOD_EXPORT struct svalue *mapping_mapping_lookup(struct mapping *m,
				      struct svalue *key1,
				      struct svalue *key2,
				      int create);

/** A shortcut for @ref mapping_mapping_lookup when both keys are Pike strings.
  * For the (complicated) description of what the function does, see the documentation
  * of the @ref mapping_mapping_lookup.
  *
  * @param m primary mapping to perform a @b key1 lookup on
  * @param key1 Pike string used to lookup an entry in the primary mapping @b m
  * @param key2 Pike string used to lookup an entry in the secondary mapping retrieved
  * (or allocated) as the result of the @b key1 lookup
  * @param create 0 to not insert an entry into neither mapping if it cannot be
  * found in the mapping, not 0 to insert such entry.
  * @return the result of the @b key2 lookup or 0 if either @b key1 or @b key2
  * returned no value and @b create was 0
  *
  * @see low_mapping_lookup
  * @see mapping_mapping_lookup
  */
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
