#include <stdlib.h>
#include "global.h"
#include "array.h"
#include "types.h"
#include "list.h"
#include "svalue.h"
#include "macros.h"
#include "memory.h"
#include "error.h"
#include "dynamic_buffer.h"
#include "interpret.h"
#include "builtin_efuns.h"

struct list *first_list;

int list_member(struct list *l, struct svalue *ind)
{
  return set_lookup(l->ind, ind) >= 0;
}

/*
 * allocate and init a new list
 */
static struct list *allocate_list(struct array *ind)
{
  struct list *l;
  l=ALLOC_STRUCT(list);
  l->next = first_list;
  l->prev = 0;
  l->refs = 1;
  l->ind=ind;
  if(first_list) first_list->prev = l;
  first_list=l;

  return l;
}

/*
 * free a list
 */
void really_free_list(struct list *l)
{
#ifdef DEBUG
  if(l->refs)
    fatal("really free list on list with nonzero refs.\n");
#endif

  free_array(l->ind);

  if(l->prev) l->prev->next = l->next;
  if(l->next) l->next->prev = l->prev;
  if(first_list == l) first_list = 0;

  free((char *)l);
}

static void order_list(struct list *l)
{
  INT32 *order;
  order = get_set_order(l->ind);
  l->ind = order_array(l->ind, order);
  free((char *)order);
}

struct list *mklist(struct array *ind)
{
  struct list *l;
  l=allocate_list(copy_array(ind));
  order_list(l);
  return l;
}

void list_insert(struct list *l,
		 struct svalue *ind)
{
  INT32 i;
  i=set_lookup(l->ind, ind);
  if(i < 0)
  {
    l->ind=array_insert(l->ind, ind, ~i);
  }
}

#if 0
struct array *list_indices(struct list *l)
{
  return l->ind;
}
#endif

void list_delete(struct list *l,struct svalue *ind)
{
  INT32 i;
  i=set_lookup(l->ind, ind);

  if(i >= 0) l->ind=array_remove(l->ind, i);
}

void check_list_for_destruct(struct list *l)
{
/* Horrifiying worst case!!!!! */
  INT32 i;
  while( (i=array_find_destructed_object(l->ind)) >= 0)
    l->ind=array_remove(l->ind, i);
}

struct list *copy_list(struct list *tmp)
{
  check_list_for_destruct(tmp);
  return allocate_list(copy_array(tmp->ind));
}

struct list *merge_lists(struct list *a,
			  struct list *b,
			  INT32 operator)
{
  struct list *ret;
  INT32 *zipper;

  check_list_for_destruct(a);
  check_list_for_destruct(b);

  zipper=merge(a->ind,b->ind,operator);
  ret=allocate_list(array_zip(a->ind,b->ind,zipper));
  free((char *)zipper);
  return ret;
}

struct list *add_lists(struct svalue *argp,INT32 args)
{
  struct list *ret,*a,*b;
  switch(args)
  {
  case 0:
    ret=allocate_list(allocate_array_no_init(0,0,T_MIXED));
    break;

  case 1:
    ret=copy_list(argp->u.list);
    break;

  case 2:
    ret=merge_lists(argp[0].u.list,argp[1].u.list,OP_ADD);
    break;

  case 3:
    a=merge_lists(argp[0].u.list, argp[1].u.list, OP_ADD);
    ret=merge_lists(a, argp[2].u.list, OP_ADD);
    free_list(a);
    break;

  default:
    a=add_lists(argp,args/2);
    b=add_lists(argp+args/2,args-args/2);
    ret=merge_lists(a,b,OP_ADD);
    free_list(a);
    free_list(b);
    break;
  }

  return ret;
}

int list_equal_p(struct list *a, struct list *b, struct processing *p)
{
  if(a == b) return 1;
  check_list_for_destruct(a);
  
  return array_equal_p(a->ind, b->ind, p);
}

void describe_list(struct list *l,struct processing *p,int indent)
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
  
  sprintf(buf,"(< /* %ld elements */\n",(long)l->ind->size);
  my_strcat(buf);
  describe_array_low(l->ind,&doing,indent);
  my_putchar('\n');
  for(e=2; e<indent; e++) my_putchar(' ');
  my_strcat(">)");
}

node * make_node_from_list(struct list *l)
{
  if(check_that_array_is_constant(l->ind))
  {
    struct svalue s;
    s.type=T_LIST;
    s.subtype=0;
    s.u.list=l;
    return mkconstantsvaluenode(&s);
  }else{
    return mkefuncallnode("mklist",make_node_from_array(l->ind));
  }
}

void f_aggregate_list(INT32 args)
{
  struct list *l;
  f_aggregate(args);
  l=allocate_list(sp[-1].u.array);
  order_list(l);
  sp[-1].type=T_LIST;
  sp[-1].u.list=l;
}


struct list *copy_list_recursively(struct list *l,
				   struct processing *p)
{
  struct processing doing;
  struct list *ret;

  doing.next=p;
  doing.pointer_a=(void *)l;
  for(;p;p=p->next)
  {
    if(p->pointer_a == (void *)l)
    {
      ret=(struct list *)p->pointer_b;
      ret->refs++;
      return ret;
    }
  }

  ret=allocate_list(0);
  doing.pointer_b=(void *)ret;

  ret->ind=copy_array_recursively(l->ind,&doing);

  order_list(ret);

  return ret;
}
