/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: multiset.h,v 1.16 2000/12/16 05:24:41 marcus Exp $
 */
#ifndef MULTISET_H
#define MULTISET_H

#include "las.h"

struct multiset
{
  INT32 refs;
#ifdef PIKE_SECURITY
  struct object *prot;
#endif
  struct multiset *next,*prev;
  struct array *ind;
};

extern struct multiset *first_multiset;
extern struct multiset *gc_internal_multiset;

#define free_multiset(L) do{ struct multiset *l_=(L); debug_malloc_touch(l_); if(!--l_->refs) really_free_multiset(l_); }while(0)

#define l_sizeof(L) ((L)->ind->size)

/* Prototypes begin here */
PMOD_EXPORT int multiset_member(struct multiset *l, struct svalue *ind);
PMOD_EXPORT struct multiset *allocate_multiset(struct array *ind);
PMOD_EXPORT void really_free_multiset(struct multiset *l);
PMOD_EXPORT void do_free_multiset(struct multiset *l);
PMOD_EXPORT void order_multiset(struct multiset *l);
PMOD_EXPORT struct multiset *mkmultiset(struct array *ind);
PMOD_EXPORT void multiset_insert(struct multiset *l,
		 struct svalue *ind);
struct array *multiset_indices(struct multiset *l);
PMOD_EXPORT void multiset_delete(struct multiset *l,struct svalue *ind);
PMOD_EXPORT void check_multiset_for_destruct(struct multiset *l);
PMOD_EXPORT struct multiset *copy_multiset(struct multiset *tmp);
PMOD_EXPORT struct multiset *merge_multisets(struct multiset *a,
			  struct multiset *b,
			  INT32 operator);
PMOD_EXPORT struct multiset *add_multisets(struct svalue *argp,INT32 args);
int multiset_equal_p(struct multiset *a, struct multiset *b, struct processing *p);
void describe_multiset(struct multiset *l,struct processing *p,int indent);
node * make_node_from_multiset(struct multiset *l);
PMOD_EXPORT void f_aggregate_multiset(INT32 args);
struct multiset *copy_multiset_recursively(struct multiset *l,
				   struct processing *p);
void gc_mark_multiset_as_referenced(struct multiset *l);
unsigned gc_touch_all_multisets(void);
void gc_check_all_multisets(void);
void gc_mark_all_multisets(void);
void real_gc_cycle_check_multiset(struct multiset *l, int weak);
void gc_cycle_check_all_multisets(void);
void gc_free_all_unreferenced_multisets(void);
void count_memory_in_multisets(INT32 *num_, INT32 *size_);
/* Prototypes end here */

#define gc_cycle_check_multiset(X, WEAK) \
  gc_cycle_enqueue((gc_cycle_check_cb *) real_gc_cycle_check_multiset, (X), (WEAK))

#endif /* MULTISET_H */
