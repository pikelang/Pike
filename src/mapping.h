/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef MAPPING_H
#define MAPPING_H

#include "las.h"

struct mapping
{
  INT32 refs, size, hashsize;
  TYPE_FIELD ind_types, val_types;
  struct mapping *next, *prev;
  struct keypair **hash;
  struct keypair *free_list;
};

#define m_sizeof(m) ((m)->size)
#define m_ind_types(m) ((m)->ind_types)
#define m_val_types(m) ((m)->val_types)

#define free_mapping(M) do{ struct mapping *m_=(M); if(!--m_->refs) really_free_mapping(m_); }while(0)

/* Prototypes begin here */
struct keypair;
struct mapping *allocate_mapping(int size);
void really_free_mapping(struct mapping *m);
void mapping_fix_type_field(struct mapping *m);
void mapping_insert(struct mapping *m,
		    struct svalue *key,
		    struct svalue *val);
union anything *mapping_get_item_ptr(struct mapping *m,
				     struct svalue *key,
				     TYPE_T t);
void map_delete(struct mapping *m,
		struct svalue *key);
void check_mapping_for_destruct(struct mapping *m);
struct svalue *low_mapping_lookup(struct mapping *m,
				  struct svalue *key);
struct svalue *low_mapping_string_lookup(struct mapping *m,
					 struct pike_string *p);
void mapping_index_no_free(struct svalue *dest,
			   struct mapping *m,
			   struct svalue *key);
struct array *mapping_indices(struct mapping *m);
struct array *mapping_values(struct mapping *m);
void mapping_replace(struct mapping *m,struct svalue *from, struct svalue *to);
struct mapping *mkmapping(struct array *ind, struct array *val);
struct mapping *copy_mapping(struct mapping *m);
struct mapping *merge_mappings(struct mapping *a, struct mapping *b, INT32 op);
struct mapping *add_mappings(struct svalue *argp, INT32 args);
int mapping_equal_p(struct mapping *a, struct mapping *b, struct processing *p);
void describe_mapping(struct mapping *m,struct processing *p,int indent);
node * make_node_from_mapping(struct mapping *m);
void f_m_delete(INT32 args);
void f_aggregate_mapping(INT32 args);
struct mapping *copy_mapping_recursively(struct mapping *m,
					 struct processing *p);
void mapping_search_no_free(struct svalue *to,
			    struct mapping *m,
			    struct svalue *look_for,
			    struct svalue *start);
void check_mapping(struct mapping *m);
void check_all_mappings();
void gc_mark_mapping_as_referenced(struct mapping *m);
void gc_check_all_mappings();
void gc_mark_all_mappings();
void gc_free_all_unreferenced_mappings();
void zap_all_mappings();
void count_memory_in_mappings(INT32 *num_, INT32 *size_);
/* Prototypes end here */
#endif
