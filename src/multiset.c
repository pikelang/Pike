/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
#include "array.h"
#include "multiset.h"
#include "svalue.h"
#include "pike_macros.h"
#include "pike_memory.h"
#include "pike_error.h"
#include "dynamic_buffer.h"
#include "interpret.h"
#include "builtin_functions.h"
#include "gc.h"
#include "security.h"

RCSID("$Id: multiset.c,v 1.34 2003/01/29 15:55:26 mast Exp $");

struct multiset *first_multiset;

struct multiset *gc_internal_multiset = 0;
static struct multiset *gc_mark_multiset_pos = 0;

PMOD_EXPORT int multiset_member(struct multiset *l, struct svalue *ind)
{
  int i;
  add_ref(l->ind);
  i=set_lookup(l->ind, ind) >= 0;
  free_array(l->ind);
  return i;
}

/*
 * allocate and init a new multiset
 */
PMOD_EXPORT struct multiset *allocate_multiset(struct array *ind)
{
  struct multiset *l;
  l=ALLOC_STRUCT(multiset);
  GC_ALLOC(l);
  l->refs = 1;
  l->ind=ind;
  DOUBLELINK(first_multiset, l);

  INITIALIZE_PROT(l);

  return l;
}

/*
 * free a multiset
 */
PMOD_EXPORT void really_free_multiset(struct multiset *l)
{
#ifdef PIKE_DEBUG
  if(l->refs)
    fatal("really free multiset on multiset with nonzero refs.\n");
#endif

  free_array(l->ind);
  FREE_PROT(l);

  DOUBLEUNLINK(first_multiset, l);

  free((char *)l);
  GC_FREE(l);
}

PMOD_EXPORT void do_free_multiset(struct multiset *l)
{
  if (l)
    free_multiset(l);
}

#define BEGIN() do{				\
  struct array *ind=l->ind;			\
  INT32 is_weak = ind->flags &			\
    (ARRAY_WEAK_FLAG|ARRAY_WEAK_SHRINK);	\
  if(ind->refs > 1)				\
  {						\
    ind=copy_array(l->ind);			\
    free_array(l->ind);				\
    l->ind=ind;					\
  }						\
  add_ref(ind)

#define END()					\
  if(l->ind != ind)				\
  {						\
    free_array(l->ind);				\
    l->ind = array_set_flags(ind, is_weak);	\
  }else{					\
    free_array(ind);				\
  }						\
}while(0)


PMOD_EXPORT void order_multiset(struct multiset *l)
{
  INT32 *order;
  int flags;
  if(l->ind->size < 2) return;

  BEGIN();

#if 0

  order = get_set_order(ind);
  flags = ind->flags;
  ind = order_array(ind, order);
  ind->flags = flags;
  free((char *)order);
#else
  {
    extern void set_sort_svalues(struct svalue *, struct svalue *);
    set_sort_svalues(ITEM(ind),ITEM(ind)+ind->size-1);
  }
#endif

  END();
}

PMOD_EXPORT struct multiset *mkmultiset(struct array *ind)
{
  struct multiset *l;
  l=allocate_multiset(copy_array(ind));
  order_multiset(l);
  return l;
}

PMOD_EXPORT void multiset_insert(struct multiset *l,
		 struct svalue *v)
{
  INT32 i;
  BEGIN();
  i=set_lookup(ind, v);
  if(i < 0)
  {
    int flags = ind->flags;
    ind=array_insert(ind, v, ~i);
    ind->flags = flags;
  }
  END();
}

#if 0
struct array *multiset_indices(struct multiset *l)
{
  return l->ind;
}
#endif

PMOD_EXPORT void multiset_delete(struct multiset *l,struct svalue *v)
{
  INT32 i;
  BEGIN();
  i=set_lookup(ind, v);

  if(i >= 0)
  {
    int flags = ind->flags;
    ind=array_remove(ind, i);
    ind->flags = flags;
  }
  END();
}

PMOD_EXPORT void check_multiset_for_destruct(struct multiset *l)
{
/* Horrifying worst case!!!!! */
  INT32 i;
  BEGIN();
  {
    int flags = ind->flags;
    while( (i=array_find_destructed_object(ind)) >= 0)
      ind=array_remove(ind, i);
    ind->flags = flags;
  }
  END();
}

PMOD_EXPORT struct multiset *copy_multiset(struct multiset *tmp)
{
  check_multiset_for_destruct(tmp);
  return allocate_multiset(copy_array(tmp->ind));
}

PMOD_EXPORT struct multiset *merge_multisets(struct multiset *a,
			  struct multiset *b,
			  INT32 operator)
{
  struct multiset *ret;
  INT32 *zipper;

  check_multiset_for_destruct(a);
  check_multiset_for_destruct(b);

  zipper=merge(a->ind,b->ind,operator);
  ret=allocate_multiset(array_zip(a->ind,b->ind,zipper));
  free((char *)zipper);
  return ret;
}

PMOD_EXPORT struct multiset *add_multisets(struct svalue *argp,INT32 args)
{
  struct multiset *ret,*a,*b;
  switch(args)
  {
  case 0:
    ret=allocate_multiset(allocate_array_no_init(0,0));
    break;

  case 1:
    ret=copy_multiset(argp->u.multiset);
    break;

  case 2:
    ret=merge_multisets(argp[0].u.multiset,argp[1].u.multiset,PIKE_ARRAY_OP_ADD);
    break;

  case 3:
    a=merge_multisets(argp[0].u.multiset, argp[1].u.multiset, PIKE_ARRAY_OP_ADD);
    ret=merge_multisets(a, argp[2].u.multiset, PIKE_ARRAY_OP_ADD);
    free_multiset(a);
    break;

  default:
    a=add_multisets(argp,args/2);
    b=add_multisets(argp+args/2,args-args/2);
    ret=merge_multisets(a,b,PIKE_ARRAY_OP_ADD);
    free_multiset(a);
    free_multiset(b);
    break;
  }

  return ret;
}

int multiset_equal_p(struct multiset *a, struct multiset *b, struct processing *p)
{
  if(a == b) return 1;
  check_multiset_for_destruct(a);
  
  return array_equal_p(a->ind, b->ind, p);
}

void describe_multiset(struct multiset *l,struct processing *p,int indent)
{
  struct processing doing;
  int e;
  char buf[40];
  if(!l->ind->size)
  {
    my_strcat("(< >)");
    return;
  }

  doing.next=p;
  doing.pointer_a=(void *)l;
  for(e=0;p;e++,p=p->next)
  {
    if(p->pointer_a == (void *)l)
    {
      sprintf(buf,"@%d",e);
      my_strcat(buf);
      return;
    }
  }

  if (l->ind->size == 1) {
    my_strcat("(< /* 1 element */\n");
  } else {
    sprintf(buf, "(< /* %ld elements */\n", (long)l->ind->size);
    my_strcat(buf);
  }
  describe_array_low(l->ind,&doing,indent);
  my_putchar('\n');
  for(e=2; e<indent; e++) my_putchar(' ');
  my_strcat(">)");
}

node * make_node_from_multiset(struct multiset *l)
{
  if(multiset_is_constant(l,0))
  {
    struct svalue s;
    s.type=T_MULTISET;
    s.subtype=0;
    s.u.multiset=l;
    return mkconstantsvaluenode(&s);
  }else{
    return mkefuncallnode("mkmultiset",make_node_from_array(l->ind));
  }
}

PMOD_EXPORT void f_aggregate_multiset(INT32 args)
{
  struct multiset *l;
  f_aggregate(args);
  l=allocate_multiset(sp[-1].u.array);
  order_multiset(l);
  sp[-1].type=T_MULTISET;
  sp[-1].u.multiset=l;
}


struct multiset *copy_multiset_recursively(struct multiset *l,
				   struct processing *p)
{
  struct processing doing;
  struct multiset *ret;

  doing.next=p;
  doing.pointer_a=(void *)l;
  for(;p;p=p->next)
  {
    if(p->pointer_a == (void *)l)
    {
      ret=(struct multiset *)p->pointer_b;
      add_ref(ret);
      return ret;
    }
  }

  ret=allocate_multiset( & empty_array );
  doing.pointer_b=(void *)ret;

  ret->ind=copy_array_recursively(l->ind,&doing);

  order_multiset(ret);

  return ret;
}


void gc_mark_multiset_as_referenced(struct multiset *l)
{
  if(gc_mark(l)) {
    if (l == gc_mark_multiset_pos)
      gc_mark_multiset_pos = l->next;
    if (l == gc_internal_multiset)
      gc_internal_multiset = l->next;
    else {
      DOUBLEUNLINK(first_multiset, l);
      DOUBLELINK(first_multiset, l); /* Linked in first. */
    }
    gc_mark_array_as_referenced(l->ind);
  }
}

void real_gc_cycle_check_multiset(struct multiset *l, int weak)
{
  GC_CYCLE_ENTER(l, weak) {
    gc_cycle_check_array(l->ind, 0);
  } GC_CYCLE_LEAVE;
}

unsigned gc_touch_all_multisets(void)
{
  unsigned n = 0;
  struct multiset *l;
  if (first_multiset && first_multiset->prev)
    fatal("Error in multiset link list.\n");
  for(l=first_multiset;l;l=l->next) {
    debug_gc_touch(l);
    n++;
    if (l->next && l->next->prev != l)
      fatal("Error in multiset link list.\n");
  }
  return n;
}

void gc_check_all_multisets(void)
{
  struct multiset *l;
  for(l=first_multiset;l;l=l->next)
    debug_gc_check(l->ind, T_MULTISET, l);
}

void gc_mark_all_multisets(void)
{
  gc_mark_multiset_pos = gc_internal_multiset;
  while (gc_mark_multiset_pos) {
    struct multiset *l = gc_mark_multiset_pos;
    gc_mark_multiset_pos = l->next;
    if(gc_is_referenced(l))
      gc_mark_multiset_as_referenced(l);
  }
}

void gc_cycle_check_all_multisets(void)
{
  struct multiset *l;
  for (l = gc_internal_multiset; l; l = l->next) {
    real_gc_cycle_check_multiset(l, 0);
    gc_cycle_run_queue();
  }
}

void gc_free_all_unreferenced_multisets(void)
{
  struct multiset *l,*next;

  for(l=gc_internal_multiset;l;l=next)
  {
    if(gc_do_free(l))
    {
      /* Got an extra ref from gc_cycle_pop(). */
      free_svalues(ITEM(l->ind), l->ind->size, l->ind->type_field);
      l->ind->size=0;

      gc_free_extra_ref(l);
      SET_NEXT_AND_FREE(l, free_multiset);
    }else{
      next=l->next;
    }
  }
}

void count_memory_in_multisets(INT32 *num_, INT32 *size_)
{
  struct multiset *m;
  INT32 size=0, num=0;
  for(m=first_multiset;m;m=m->next)
  {
    num++;
    size+=sizeof(struct multiset);
  }
  *num_=num;
  *size_=size;
}


int multiset_is_constant(struct multiset *m,
			 struct processing *p)
{
  return array_is_constant(m->ind, p);
}
