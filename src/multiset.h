/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef MULTISET_H
#define MULTISET_H

#include "las.h"

struct multiset
{
  INT32 refs;
  struct multiset *next,*prev;
  struct array *ind;
};

#define free_multiset(L) do{ struct multiset *l_=(L); if(!--l_->refs) really_free_multiset(l_); }while(0)

#define l_sizeof(L) ((L)->ind->size)

/* Prototypes begin here */
int multiset_member(struct multiset *l, struct svalue *ind);
void really_free_multiset(struct multiset *l);
struct multiset *mkmultiset(struct array *ind);
void multiset_insert(struct multiset *l,
		 struct svalue *ind);
struct array *multiset_indices(struct multiset *l);
void multiset_delete(struct multiset *l,struct svalue *ind);
void check_multiset_for_destruct(struct multiset *l);
struct multiset *copy_multiset(struct multiset *tmp);
struct multiset *merge_multisets(struct multiset *a,
			  struct multiset *b,
			  INT32 operator);
struct multiset *add_multisets(struct svalue *argp,INT32 args);
int multiset_equal_p(struct multiset *a, struct multiset *b, struct processing *p);
void describe_multiset(struct multiset *l,struct processing *p,int indent);
node * make_node_from_multiset(struct multiset *l);
void f_aggregate_multiset(INT32 args);
struct multiset *copy_multiset_recursively(struct multiset *l,
				   struct processing *p);
void gc_mark_multiset_as_referenced(struct multiset *l);
void gc_check_all_multisets(void);
void gc_mark_all_multisets(void);
void gc_free_all_unreferenced_multisets(void);
void count_memory_in_multisets(INT32 *num_, INT32 *size_);
/* Prototypes end here */

#endif
