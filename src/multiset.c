/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "array.h"
#include "multiset.h"
#include "svalue.h"
#include "pike_macros.h"
#include "pike_memory.h"
#include "error.h"
#include "dynamic_buffer.h"
#include "interpret.h"
#include "builtin_functions.h"
#include "gc.h"
#include "security.h"

RCSID("$Id: multiset.c,v 1.21 2000/04/27 02:13:28 hubbe Exp $");

struct multiset *first_multiset;

int multiset_member(struct multiset *l, struct svalue *ind)
{
  return set_lookup(l->ind, ind) >= 0;
}

/*
 * allocate and init a new multiset
 */
struct multiset *allocate_multiset(struct array *ind)
{
  struct multiset *l;
  l=ALLOC_STRUCT(multiset);
  GC_ALLOC(l);
  l->next = first_multiset;
  l->prev = 0;
  l->refs = 1;
  l->ind=ind;
  if(first_multiset) first_multiset->prev = l;
  first_multiset=l;

  INITIALIZE_PROT(l);

  return l;
}

/*
 * free a multiset
 */
void really_free_multiset(struct multiset *l)
{
#ifdef PIKE_DEBUG
  if(l->refs)
    fatal("really free multiset on multiset with nonzero refs.\n");
#endif

  free_array(l->ind);
  FREE_PROT(l);

  if(l->prev)
    l->prev->next = l->next;
  else
    first_multiset = l->next;

  if(l->next)  l->next->prev = l->prev;
    
  free((char *)l);
  GC_FREE(l);
}


void order_multiset(struct multiset *l)
{
  INT32 *order;
  int flags;
  if(l->ind->size < 2) return;
  order = get_set_order(l->ind);
  flags = l->ind->flags;
  l->ind = order_array(l->ind, order);
  l->ind->flags = flags;
  free((char *)order);
}

struct multiset *mkmultiset(struct array *ind)
{
  struct multiset *l;
  l=allocate_multiset(copy_array(ind));
  order_multiset(l);
  return l;
}

void multiset_insert(struct multiset *l,
		 struct svalue *ind)
{
  INT32 i;
  i=set_lookup(l->ind, ind);
  if(i < 0)
  {
    int flags = l->ind->flags;
    l->ind=array_insert(l->ind, ind, ~i);
    l->ind->flags = flags;
  }
}

#if 0
struct array *multiset_indices(struct multiset *l)
{
  return l->ind;
}
#endif

void multiset_delete(struct multiset *l,struct svalue *ind)
{
  INT32 i;
  i=set_lookup(l->ind, ind);

  if(i >= 0)
  {
    int flags = l->ind->flags;
    l->ind=array_remove(l->ind, i);
    l->ind->flags = flags;
  }
}

void check_multiset_for_destruct(struct multiset *l)
{
/* Horrifying worst case!!!!! */
  INT32 i;
  int flags = l->ind->flags;
  while( (i=array_find_destructed_object(l->ind)) >= 0)
    l->ind=array_remove(l->ind, i);
  l->ind->flags = flags;
}

struct multiset *copy_multiset(struct multiset *tmp)
{
  check_multiset_for_destruct(tmp);
  return allocate_multiset(copy_array(tmp->ind));
}

struct multiset *merge_multisets(struct multiset *a,
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

struct multiset *add_multisets(struct svalue *argp,INT32 args)
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
  
  sprintf(buf, l->ind->size == 1 ? "(< /* %ld element */\n" :
	                           "(< /* %ld elements */\n",
	  (long)l->ind->size);
  my_strcat(buf);
  describe_array_low(l->ind,&doing,indent);
  my_putchar('\n');
  for(e=2; e<indent; e++) my_putchar(' ');
  my_strcat(">)");
}

node * make_node_from_multiset(struct multiset *l)
{
  if(check_that_array_is_constant(l->ind))
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

void f_aggregate_multiset(INT32 args)
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
  if(gc_mark(l))
    gc_mark_array_as_referenced(l->ind);
}

#ifdef PIKE_DEBUG
INT32 gc_touch_all_multisets(void)
{
  INT32 n = 0;
  struct multiset *l;
  for(l=first_multiset;l;l=l->next) {
    debug_gc_touch(l);
    n++;
  }
  return n;
}
#endif

void gc_check_all_multisets(void)
{
  struct multiset *l;
  for(l=first_multiset;l;l=l->next)
    gc_check(l->ind);
}

void gc_mark_all_multisets(void)
{
  struct multiset *l;
  for(l=first_multiset;l;l=l->next)
    if(gc_is_referenced(l))
      gc_mark_multiset_as_referenced(l);
}

void gc_free_all_unreferenced_multisets(void)
{
  struct multiset *l,*next;

  for(l=first_multiset;l;l=next)
  {
    if(gc_do_free(l))
    {
      add_ref(l);
      free_svalues(ITEM(l->ind), l->ind->size, l->ind->type_field);
      l->ind->size=0;

      SET_NEXT_AND_FREE(l, free_multiset);
    }else{
#ifdef PIKE_DEBUG
      extern int d_flag;
      if(d_flag) gc_check(l->ind);
#endif
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

