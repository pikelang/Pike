/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: mapping.h,v 1.20 2000/02/01 23:51:48 hubbe Exp $
 */
#ifndef MAPPING_H
#define MAPPING_H

#include "las.h"
#include "block_alloc_h.h"

#define MAPPING_FLAG_WEAK 1

struct keypair
{
  struct keypair *next;
  unsigned INT32 hval;
  struct svalue ind, val;
};

struct mapping_data
{
  INT32 refs;
  INT32 valrefs; /* lock values too */
  INT32 hardlinks;
  INT32 size, hashsize;
  INT32 num_keypairs;
  TYPE_FIELD ind_types, val_types;
  struct keypair *free_list;
  struct keypair *hash[1 /* hashsize */ ];
  /* struct keypair data_block[ hashsize * AVG_LINK_LENGTH ] */
};

struct mapping
{
  INT32 refs;
#ifdef PIKE_SECURITY
  struct object *prot;
#endif
  INT16 flags;
  struct mapping_data *data;
  struct mapping *next, *prev;
};


extern struct mapping *first_mapping;

#define map_delete(m,key) map_delete_no_free(m, key, 0)
#define m_sizeof(m) ((m)->data->size)
#define m_ind_types(m) ((m)->data->ind_types)
#define m_val_types(m) ((m)->data->val_types)


#define NEW_MAPPING_LOOP(md) \
  for((e=0) DO_IF_DMALLOC( ?0:(debug_malloc_touch(md)) ) ;e<md->hashsize;e++) for(k=md->hash[e];k;k=k->next)

/* WARNING: this should not be used */
#define MAPPING_LOOP(m) \
  for((e=0) DO_IF_DMALLOC( ?0:(debug_malloc_touch(m),debug_malloc_touch(m->data))) ;e<m->data->hashsize;e++) for(k=m->data->hash[e];k;k=k->next)

#define free_mapping(M) do{ struct mapping *m_=(M); debug_malloc_touch(m_); if(!--m_->refs) really_free_mapping(m_); }while(0)

#define free_mapping_data(M) do{ \
 struct mapping_data *md_=(M); \
 debug_malloc_touch(md_); \
 if(!--md_->refs) really_free_mapping_data(md_); \
}while(0)

/* Prototypes begin here */
BLOCK_ALLOC(mapping, 511)

static void check_mapping_type_fields(struct mapping *m);
struct mapping *debug_allocate_mapping(int size);
void really_free_mapping_data(struct mapping_data *md);
struct mapping_data *copy_mapping_data(struct mapping_data *md);
void mapping_fix_type_field(struct mapping *m);
void low_mapping_insert(struct mapping *m,
			struct svalue *key,
			struct svalue *val,
			int overwrite);
void mapping_insert(struct mapping *m,
		    struct svalue *key,
		    struct svalue *val);
union anything *mapping_get_item_ptr(struct mapping *m,
				     struct svalue *key,
				     TYPE_T t);
void map_delete_no_free(struct mapping *m,
			struct svalue *key,
			struct svalue *to);
void check_mapping_for_destruct(struct mapping *m);
struct svalue *low_mapping_lookup(struct mapping *m,
				  struct svalue *key);
struct svalue *low_mapping_string_lookup(struct mapping *m,
					 struct pike_string *p);
void mapping_string_insert(struct mapping *m,
			   struct pike_string *p,
			   struct svalue *val);
void mapping_string_insert_string(struct mapping *m,
				  struct pike_string *p,
				  struct pike_string *val);
struct svalue *simple_mapping_string_lookup(struct mapping *m,
					    char *p);
struct svalue *mapping_mapping_lookup(struct mapping *m,
				      struct svalue *key1,
				      struct svalue *key2,
				      int create);
struct svalue *mapping_mapping_string_lookup(struct mapping *m,
				      struct pike_string *key1,
				      struct pike_string *key2,
				      int create);
void mapping_index_no_free(struct svalue *dest,
			   struct mapping *m,
			   struct svalue *key);
struct array *mapping_indices(struct mapping *m);
struct array *mapping_values(struct mapping *m);
struct array *mapping_to_array(struct mapping *m);
void mapping_replace(struct mapping *m,struct svalue *from, struct svalue *to);
struct mapping *mkmapping(struct array *ind, struct array *val);
struct mapping *copy_mapping(struct mapping *m);
struct mapping *copy_mapping(struct mapping *m);
struct mapping *merge_mappings(struct mapping *a, struct mapping *b, INT32 op);
struct mapping *merge_mapping_array_ordered(struct mapping *a, 
					    struct array *b, INT32 op);
struct mapping *merge_mapping_array_unordered(struct mapping *a, 
					      struct array *b, INT32 op);
struct mapping *add_mappings(struct svalue *argp, INT32 args);
int mapping_equal_p(struct mapping *a, struct mapping *b, struct processing *p);
void describe_mapping(struct mapping *m,struct processing *p,int indent);
node *make_node_from_mapping(struct mapping *m);
void f_m_delete(INT32 args);
void f_aggregate_mapping(INT32 args);
struct mapping *copy_mapping_recursively(struct mapping *m,
					 struct processing *p);
void mapping_search_no_free(struct svalue *to,
			    struct mapping *m,
			    struct svalue *look_for,
			    struct svalue *key );
void check_mapping(struct mapping *m);
void check_all_mappings(void);
void gc_mark_mapping_as_referenced(struct mapping *m);
void gc_check_all_mappings(void);
void gc_mark_all_mappings(void);
void gc_free_all_unreferenced_mappings(void);
void simple_describe_mapping(struct mapping *m);
void debug_dump_mapping(struct mapping *m);
void zap_all_mappings(void);
/* Prototypes end here */
#endif

#define allocate_mapping(X) dmalloc_touch(struct mapping *,debug_allocate_mapping(X))

