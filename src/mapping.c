#include <stdlib.h>
#include "global.h"
#include "types.h"
#include "mapping.h"
#include "svalue.h"
#include "array.h"
#include "macros.h"
#include "language.h"
#include "error.h"
#include "memory.h"
#include "dynamic_buffer.h"
#include "interpret.h"

struct mapping *first_mapping;

/*
 * This function puts looks up an index in a mapping and assigns
 * the value to dest. It does not free dest first though
 */
void mapping_index_no_free(struct svalue *dest,
			   struct mapping *m,
			   struct svalue *ind)
{
  INT32 i;
  i=set_lookup(m->ind, ind);
  if(i < 0)
  {
    dest->type=T_INT;
    dest->subtype=NUMBER_UNDEFINED;
    dest->u.integer=0;
  }else{
    array_index_no_free(dest, m->val, i);
  }
}

#if 0
/*
 * This funciton does the same thing as the one above, but
 * frees dest first.
 */
void mapping_index(struct svalue *dest,
		   struct mapping *m,
		   struct svalue *ind)
{
  INT32 i;
  m->refs++;
  i=set_lookup(m->ind, ind);
  if(i < 0)
  {
    dest->type=T_INT;
    dest->subtype=NUMBER_UNDEFINED;
    dest->u.integer=0;
  }else{
    array_index(dest, m->val, i);
  }
  free_mapping(m);
}
#endif

/*
 * allocate and init a new mapping
 */
static struct mapping *allocate_mapping(struct array *ind, struct array *val)
{
  struct mapping *m;
  m=ALLOC_STRUCT(mapping);
  m->next = first_mapping;
  m->prev = 0;
  m->refs = 1;
  m->ind=ind;
  m->val=val;
  if(first_mapping) first_mapping->prev = m;
  first_mapping=m;

  return m;
}

/*
 * free a mapping
 */
void really_free_mapping(struct mapping *m)
{
#ifdef DEBUG
  if(m->refs)
    fatal("really free mapping on mapping with nonzero refs.\n");
#endif

  free_array(m->ind);
  free_array(m->val);

  if(m->prev) m->prev->next = m->next;
  if(m->next) m->next->prev = m->prev;
  if(first_mapping == m) first_mapping = 0;

  free((char *)m);
}

static void order_mapping(struct mapping *m)
{
  INT32 *order;
  order = get_set_order(m->ind);
  m->ind = order_array(m->ind, order);
  m->val = order_array(m->val, order);
  free((char *)order);
}

struct mapping *mkmapping(struct array *ind, struct array *val)
{
  struct mapping *m;
  m=allocate_mapping(copy_array(ind), copy_array(val));
  order_mapping(m);
  return m;
}

void mapping_insert(struct mapping *m,
		    struct svalue *ind,
		    struct svalue *from)
{
  INT32 i;
  i=set_lookup(m->ind, ind);

  if(i < 0)
  {
    i = ~i;
    m->ind=array_insert(m->ind, ind, i);
    m->val=array_insert(m->val, from, i);
  }else{
    array_set_index(m->ind, i, ind);
    array_set_index(m->val, i, from);
  }
}

#if 0
struct array *mapping_indices(struct mapping *m)
{
  return m->ind;
}

struct array *mapping_values(struct mapping *m)
{
  return m->val;
}
#endif

void map_delete(struct mapping *m,struct svalue *ind)
{
  INT32 i;
  i=set_lookup(m->ind, ind);

  if(i >= 0)
  {
    m->ind=array_remove(m->ind, i);
    m->val=array_remove(m->val, i);
  }
}

void check_mapping_for_destruct(struct mapping *m)
{
  INT32 i;
  check_array_for_destruct(m->val);
  while( (i=array_find_destructed_object(m->ind)) >= 0)
  {
    m->ind=array_remove(m->ind, i);
    m->val=array_remove(m->val, i);
  }
}

union anything *mapping_get_item_ptr(struct mapping *m,
				     struct svalue *ind,
				     TYPE_T t)
{
  INT32 i;
  i=set_lookup(m->ind, ind);
  if(i < 0)
  {
    struct svalue from;
    from.type=T_INT;
    from.subtype=NUMBER_NUMBER;
    from.u.integer=0;
    i = ~i;
    m->ind=array_insert(m->ind, ind, i);
    m->val=array_insert(m->val, &from, i);
  }
  return low_array_get_item_ptr(m->val, i, t);
}

struct mapping *copy_mapping(struct mapping *tmp)
{
  check_mapping_for_destruct(tmp);
  return allocate_mapping(copy_array(tmp->ind),copy_array(tmp->val));
}

struct mapping *merge_mappings(struct mapping *a,
			       struct mapping *b,
			       INT32 operator)
{
  struct mapping *ret;
  INT32 *zipper;

  check_mapping_for_destruct(a);
  check_mapping_for_destruct(b);

  zipper=merge(a->ind,b->ind,operator);
  ret=allocate_mapping(array_zip(a->ind,b->ind,zipper),
		       array_zip(a->val,b->val,zipper));

  free((char *)zipper);
  return ret;
}

struct mapping *add_mappings(struct svalue *argp,INT32 args)
{
  struct mapping *ret,*a,*b;
  switch(args)
  {
  case 0:
    ret=allocate_mapping(allocate_array_no_init(0,0,T_MIXED),
			 allocate_array_no_init(0,0,T_MIXED));
    break;

  case 1:
    ret=copy_mapping(argp->u.mapping);
    break;

  case 2:
    ret=merge_mappings(argp[0].u.mapping,argp[1].u.mapping,OP_ADD);
    break;

  case 3:
    a=merge_mappings(argp[0].u.mapping,argp[1].u.mapping,OP_ADD);
    ret=merge_mappings(a,argp[2].u.mapping,OP_ADD);
    free_mapping(a);
    break;

  default:
    a=add_mappings(argp,args/2);
    b=add_mappings(argp+args/2,args-args/2);
    ret=merge_mappings(a,b,OP_ADD);
    free_mapping(a);
    free_mapping(b);
    break;
  }

  return ret;
}

int mapping_equal_p(struct mapping *a, struct mapping *b, struct processing *p)
{
  if(a == b) return 1;
  check_mapping_for_destruct(a);
  check_mapping_for_destruct(b);
  
  return array_equal_p(a->ind, b->ind, p)
    && array_equal_p(a->val, b->val, p);
}

void describe_mapping(struct mapping *m,struct processing *p,int indent)
{
  struct processing doing;
  INT32 e,d;
  char buf[40];

  if(! m->ind->size)
  {
    my_strcat("([ ])");
    return;
  }

  doing.next=p;
  doing.pointer_a=(void *)m;
  for(e=0;p;e++,p=p->next)
  {
    if(p->pointer_a == (void *)m)
    {
      sprintf(buf,"@%ld",(long)e);
      my_strcat(buf);
      return;
    }
  }
  
  sprintf(buf,"([ /* %ld elements */\n",(long) m->ind->size);
  my_strcat(buf);
  for(e=0;e<m->ind->size;e++)
  {
    if(e) my_strcat(",\n");
    for(d=0; d<indent; d++) my_putchar(' ');
    describe_index(m->ind, e, &doing, indent+2);
    my_putchar(':');
    describe_index(m->val, e, &doing, indent+2);
  }
  my_putchar('\n');
  for(e=2; e<indent; e++) my_putchar(' ');
  my_strcat("])");
}

node * make_node_from_mapping(struct mapping *m)
{
  if(check_that_array_is_constant(m->ind) &&
     check_that_array_is_constant(m->val))
  {
    struct svalue s;
    s.type=T_MAPPING;
    s.subtype=0;
    s.u.mapping=m;
    return mkconstantsvaluenode(&s);
  }else{
    return mkefuncallnode("mkmapping",mknode(F_ARG_LIST,make_node_from_array(m->ind),make_node_from_array(m->val)));
  }
}

void f_m_delete(INT32 args)
{
  if(args < 2)
    error("Too few arguments to m_delete.\n");
  if(sp[-args].type != T_MAPPING)
    error("Bad argument to to m_delete.\n");

  map_delete(sp[-args].u.mapping,sp+1-args);
  pop_n_elems(args-1);
}

void f_aggregate_mapping(INT32 args)
{
  INT32 e;
  struct array *ind,*val;
  struct svalue *s;
  struct mapping *m;

  if(args & 1)
    error("Uneven number of arguments to aggregage_mapping.\n");

  ind=allocate_array_no_init(args/2,0,T_MIXED);
  val=allocate_array_no_init(args/2,0,T_MIXED);

  s=sp-args;
  for(e=0;e<args/2;e++)
  {
    ITEM(ind)[e]=*(s++);
    ITEM(val)[e]=*(s++);
  }
  sp-=args;
  m=allocate_mapping(ind,val);
  order_mapping(m);
  sp->u.mapping=m;
  sp->type=T_MAPPING;
  sp++;
}

struct mapping *copy_mapping_recursively(struct mapping *m,
					 struct processing *p)
{
  struct processing doing;
  struct mapping *ret;

  doing.next=p;
  doing.pointer_a=(void *)m;
  for(;p;p=p->next)
  {
    if(p->pointer_a == (void *)m)
    {
      ret=(struct mapping *)p->pointer_b;
      ret->refs++;
      return ret;
    }
  }

  ret=allocate_mapping(0,0);
  doing.pointer_b=(void *)ret;

  ret->ind=copy_array_recursively(m->ind,&doing);
  ret->val=copy_array_recursively(m->val,&doing);

  order_mapping(ret);

  return ret;
}

void mapping_search_no_free(struct svalue *to,
			    struct mapping *m,
			    struct svalue *look_for,
			    struct svalue *start)
{
  INT32 start_pos;
  start_pos=0;
  if(start)
  {
    start_pos=set_lookup(m->ind, start);
    if(start_pos < 0)
    {
      to->type=T_INT;
      to->subtype=NUMBER_UNDEFINED;
      to->u.integer=0;
      return;
    }
  }
  start_pos=array_search(m->val,look_for,start_pos);
  if(start_pos < 0)
  {
    to->type=T_INT;
    to->subtype=NUMBER_UNDEFINED;
    to->u.integer=0;
    return;
  }
  array_index_no_free(to,m->ind,start_pos);
}

#ifdef DEBUG

void check_mapping(struct mapping *m,int pass)
{
  if(pass)
  {
    if(checked((void *)m,0) != m->refs)
      fatal("Mapping has wrong number of refs.\n");
    return;
  }

  if(m->refs <=0)
    fatal("Mapping has zero refs.\n");

  if(m->next && m->next->prev != m)
    fatal("Mapping ->next->prev != mapping.\n");

  if(m->prev)
  {
    if(m->prev->next != m)
      fatal("Mapping ->prev->next != mapping.\n");
  }else{
    if(first_mapping != m)
      fatal("Mapping ->prev == 0 but first_mapping != mapping.\n");
  }
  checked((void *)m->ind,1);
  checked((void *)m->val,1);
}

void check_all_mappings(int pass)
{
  struct mapping *m;
  for(m=first_mapping;m;m=m->next)
    check_mapping(m,pass);

  if(!pass)
    checked((void *)first_mapping,1);
}
#endif
