#ifndef MAPPING_H
#define MAPPING_H

#include "las.h"

struct mapping
{
  INT32 refs;
  struct mapping *next,*prev;
  struct array *ind;
  struct array *val;
};

#define free_mapping(M) do{ struct mapping *m_=(M); if(!--m_->refs) really_free_mapping(m_); }while(0)

/* Prototypes begin here */
void mapping_index_no_free(struct svalue *dest,
			   struct mapping *m,
			   struct svalue *ind);
void mapping_index(struct svalue *dest,
		   struct mapping *m,
		   struct svalue *ind);
void really_free_mapping(struct mapping *m);
struct mapping *mkmapping(struct array *ind, struct array *val);
void mapping_insert(struct mapping *m,
		    struct svalue *ind,
		    struct svalue *from);
struct array *mapping_indices(struct mapping *m);
struct array *mapping_values(struct mapping *m);
void map_delete(struct mapping *m,struct svalue *ind);
void check_mapping_for_destruct(struct mapping *m);
union anything *mapping_get_item_ptr(struct mapping *m,
				     struct svalue *ind,
				     TYPE_T t);
struct mapping *copy_mapping(struct mapping *tmp);
struct mapping *merge_mappings(struct mapping *a,
			       struct mapping *b,
			       INT32 operator);
struct mapping *add_mappings(struct svalue *argp,INT32 args);
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
/* Prototypes end here */
#endif
