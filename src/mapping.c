/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "main.h"
#include "types.h"
#include "object.h"
#include "mapping.h"
#include "svalue.h"
#include "array.h"
#include "macros.h"
#include "language.h"
#include "error.h"
#include "memory.h"
#include "dynamic_buffer.h"
#include "interpret.h"
#include "las.h"
#include "gc.h"

#ifndef OLD_MAPPINGS

#ifndef FLAT_MAPPINGS
#  define AVG_LINK_LENGTH 4
#  define MIN_LINK_LENGTH 1
#  define MAP_SLOTS(X) ((X)+((X)>>4)+8)
#define LOOP(m) for(e=0;e<m->hashsize;e++) for(k=m->hash[e];k;k=k->next)
#else
#  define MAX_FILL 80
#  define AVG_FILL 60
#  define MIN_FILL 30
#  define MAP_SLOTS(X) ((X)*100/AVG_FILL+1)
#define LOOP(m) for(e=0;e<m->hashsize;e++) if((k=m->hash+e)->ind.type <= MAX_TYPE)
#endif

struct keypair
{
#ifndef FLAT_MAPPINGS
  struct keypair *next;
#endif
  struct svalue ind, val;
};

struct mapping *first_mapping;

static void init_mapping(struct mapping *m, INT32 size)
{
#ifndef FLAT_MAPPINGS
  char *tmp;
  INT32 e;
  INT32 hashsize,hashspace;

#ifdef DEBUG
  if(size < 0) fatal("init_mapping with negative value.\n");
#endif

  hashsize=size / AVG_LINK_LENGTH + 1;
  if(!(hashsize & 1)) hashsize++;
  hashspace=hashsize+1;
  e=sizeof(struct keypair)*size+ sizeof(struct keypair *)*hashspace;
  tmp=(char *)xalloc(e);

  m->hash=(struct keypair **) tmp;
  m->hashsize=hashsize;

  tmp+=sizeof(struct keypair *)*hashspace;
  m->free_list=(struct keypair *) tmp;
  
  MEMSET((char *)m->hash, 0, sizeof(struct keypair *) * m->hashsize);

  for(e=1;e<size;e++)
    m->free_list[e-1].next = m->free_list + e;
  m->free_list[e-1].next=0;

#else
  INT32 e;
  INT32 hashsize;
  hashsize=size*100/MAX_FILL+1;
  m->hash=(struct keypair *)xalloc(sizeof(struct keypair)*hashsize);
  m->hashsize=hashsize;
  for(e=0;e<hashsize;e++) m->hash[e].ind.type=T_VOID;
#endif  
  m->ind_types = 0;
  m->val_types = 0;
  m->size = 0;
}


static struct mapping *allocate_mapping(int size)
{
  struct mapping *m;

  GC_ALLOC();

  m=ALLOC_STRUCT(mapping);

  init_mapping(m,size);

  m->next = first_mapping;
  m->prev = 0;
  m->refs = 1;

  if(first_mapping) first_mapping->prev = m;
  first_mapping=m;

  return m;
}


void really_free_mapping(struct mapping *m)
{
  INT32 e;
  struct keypair *k;
#ifdef DEBUG
  if(m->refs)
    fatal("really free mapping on mapping with nonzero refs.\n");
#endif

  LOOP(m)
  {
    free_svalue(& k->val);
    free_svalue(& k->ind);
  }

  if(m->prev)
    m->prev->next = m->next;
  else
    first_mapping = m->next;

  if(m->next) m->next->prev = m->prev;

  free((char *)m->hash);
  free((char *)m);

  GC_FREE();
}

#ifndef FLAT_MAPPINGS
static void mapping_rehash_backwards(struct mapping *m, struct keypair *p)
{
  unsigned INT32 h;
  struct keypair *tmp;

  if(!p) return;
  mapping_rehash_backwards(m,p->next);
  h=hash_svalue(& p->ind) % m->hashsize;
  tmp=m->free_list;
  m->free_list=tmp->next;
  tmp->next=m->hash[h];
  m->hash[h]=tmp;
  tmp->ind=p->ind;
  tmp->val=p->val;
  m->size++;
  m->ind_types |= 1 << p->ind.type;
  m->val_types |= 1 << p->val.type;
}
#endif

static struct mapping *rehash(struct mapping *m, int new_size)
{
#ifdef DEBUG
  INT32 tmp=m->size;
#endif
#ifndef FLAT_MAPPINGS
  INT32 e,hashsize;
  struct keypair *k,**hash;

  hashsize=m->hashsize;
  hash=m->hash;

  init_mapping(m, new_size);

  for(e=0;e<hashsize;e++)
    mapping_rehash_backwards(m, hash[e]);
#else
  struct keypair *hash;
  INT32 e,hashsize;

  hashsize=m->hashsize;
  hash=m->hash;

  init_mapping(m, new_size);

  for(e=0;e<hashsize;e++)
    if(hash[e].ind.type <= MAX_TYPE)
      mapping_insert(m, & hash[e].ind, & hash[e].val);
#endif

#ifdef DEBUG
  if(m->size != tmp)
    fatal("Rehash failed, size not same any more.\n");
#endif

  free((char *)hash);
  return m;
}

void mapping_fix_type_field(struct mapping *m)
{
  INT32 e;
  struct keypair *k;
  TYPE_FIELD ind_types, val_types;

  val_types = ind_types = 0;
#ifndef FLAT_MAPPINGS
  for(e=0;e<m->hashsize;e++)
  {
    for(k=m->hash[e];k;k=k->next)
    {
      val_types |= 1 << k->val.type;
      ind_types |= 1 << k->ind.type;
    }
  }
#else
  for(e=0;e<m->hashsize;e++)
  {
    if(m->hash[e].ind.type <= MAX_TYPE)
    {
      val_types |= 1 << m->hash[e].val.type;
      ind_types |= 1 << m->hash[e].ind.type;
    }
  }
#endif

#ifdef DEBUG
  if(val_types & ~(m->val_types))
    fatal("Mapping value types out of order!\n");

  if(ind_types & ~(m->ind_types))
    fatal("Mapping indices types out of order!\n");
#endif
  m->val_types = val_types;
  m->ind_types = ind_types;
}

#ifdef DEBUG
static void check_mapping_type_fields(struct mapping *m)
{
  INT32 e;
  struct keypair *k,**prev;
  TYPE_FIELD ind_types, val_types;

  ind_types=val_types=0;

#ifndef FLAT_MAPPINGS
  for(e=0;e<m->hashsize;e++)
  {
    for(k=m->hash[e];k;k=k->next)
    {
      val_types |= 1 << k->val.type;
      ind_types |= 1 << k->ind.type;
    }
  }
#else
  for(e=0;e<m->hashsize;e++)
  {
    if(m->hash[e].ind.type <= MAX_TYPE)
    {
      val_types |= 1 << m->hash[e].val.type;
      ind_types |= 1 << m->hash[e].ind.type;
    }
  }
#endif

  if(val_types & ~(m->val_types))
    fatal("Mapping value types out of order!\n");

  if(ind_types & ~(m->ind_types))
    fatal("Mapping indices types out of order!\n");
}
#endif


void mapping_insert(struct mapping *m,
		    struct svalue *key,
		    struct svalue *val)
{
#ifndef FLAT_MAPPINGS
  unsigned INT32 h;
  struct keypair *k, **prev;

  if(val->type==T_INT)
    val->subtype=NUMBER_NUMBER;

  h=hash_svalue(key) % m->hashsize;
  
#ifdef DEBUG
  if(d_flag > 1) check_mapping_type_fields(m);
#endif
  if(m->ind_types & (1 << key->type))
  {
    for(prev= m->hash + h;k=*prev;prev=&k->next)
    {
      if(is_eq(& k->ind, key))
      {
	*prev=k->next;
	k->next=m->hash[h];
	m->hash[h]=k;
	
	m->val_types |= 1 << val->type;
	assign_svalue(& k->val, val);
	return;
      }
    }
  }

  if(!(k=m->free_list))
  {
    rehash(m, m->size * 2 + 1);
    h=hash_svalue(key) % m->hashsize;
    k=m->free_list;
  }

  m->free_list=k->next;
  k->next=m->hash[h];
  m->hash[h]=k;
  m->ind_types |= 1 << key->type;
  m->val_types |= 1 << val->type;
  assign_svalue_no_free(& k->ind, key);
  assign_svalue_no_free(& k->val, val);
  m->size++;
#else
  unsigned INT32 h;
  if(val->type==T_INT)
    val->subtype=NUMBER_NUMBER;

  h=hash_svalue(key) % m->hashsize;

  while(m->hash[h].ind.type != T_VOID)
  {
    if(is_eq(& m->hash[h].ind, key))
    {
      assign_svalue(& m->hash[h].val, val);
      m->val_types |= 1 << val->type;
      return;
    }
    if(++h == m->hashsize) h=0;
  }

  if((m->size+1) * 100 > m->hashsize * MAX_FILL)
  {
    rehash(m, m->size * 2 + 1);
    h=hash_svalue(key) % m->hashsize;

    while(m->hash[h].ind.type != T_VOID)
      if(++h == m->hashsize) h=0;
  }

  m->size++;

  assign_svalue_no_free(& m->hash[h].ind, key);
  assign_svalue_no_free(& m->hash[h].val, val);
  m->ind_types |= 1 << key->type;
  m->val_types |= 1 << val->type;
#endif
}

union anything *mapping_get_item_ptr(struct mapping *m,
				     struct svalue *key,
				     TYPE_T t)
{
#ifndef FLAT_MAPPINGS
  unsigned INT32 h;
  struct keypair *k, **prev;

  h=hash_svalue(key) % m->hashsize;
  
  for(prev= m->hash + h;k=*prev;prev=&k->next)
  {
    if(is_eq(& k->ind, key))
    {
      *prev=k->next;
      k->next=m->hash[h];
      m->hash[h]=k;

      if(k->val.type == t) return & ( k->val.u );
      return 0;
    }
  }

  if(!(k=m->free_list))
  {
    rehash(m, m->size * 2 + 1);
    h=hash_svalue(key) % m->hashsize;
    k=m->free_list;
  }

  m->free_list=k->next;
  k->next=m->hash[h];
  m->hash[h]=k;
  assign_svalue_no_free(& k->ind, key);
  k->val.type=T_INT;
  k->val.subtype=NUMBER_NUMBER;
  k->val.u.integer=0;
  m->ind_types |= 1 << key->type;
  m->val_types |= BIT_INT;
  m->size++;

  if(t != T_INT) return 0;
  return & ( k->val.u );
#else
  unsigned INT32 h;

  h=hash_svalue(key) % m->hashsize;

  while(m->hash[h].ind.type != T_VOID)
  {
    if(is_eq(& m->hash[h].ind, key))
    {
      if(m->hash[h].ind.type == t) return &(m->hash[h].val.u);
      return 0;
    }
    if(++h == m->hashsize) h=0;
  }

  if((m->size+1) * 100 > m->hashsize * MAX_FILL)
  {
    rehash(m, m->size * 2 + 1);
    h=hash_svalue(key) % m->hashsize;
  }

  m->size++;

  assign_svalue_no_free(& m->hash[h].ind, key);
  m->hash[h].val.type=T_INT;
  m->hash[h].val.subtype=NUMBER_NUMBER;
  m->hash[h].val.u.integer=0;
  m->ind_types |= 1 << key->type;
  m->val_types |= BIT_INT;
  return &(m->hash[h].val.u);
#endif
}

void map_delete(struct mapping *m,
		struct svalue *key)
{
#ifndef FLAT_MAPPINGS
  unsigned INT32 h;
  struct keypair *k, **prev;

  h=hash_svalue(key) % m->hashsize;
  
  for(prev= m->hash + h;k=*prev;prev=&k->next)
  {
    if(is_eq(& k->ind, key))
    {
      *prev=k->next;
      free_svalue(& k->ind);
      free_svalue(& k->val);
      k->next=m->free_list;
      m->free_list=k;
      m->size--;

      if(m->size < (m->hashsize + 1) * MIN_LINK_LENGTH)
	rehash(m, MAP_SLOTS(m->size));

      return;
    }
  }
#else
  unsigned INT32 h;

  h=hash_svalue(key) % m->hashsize;

  while(m->hash[h].ind.type != T_VOID)
  {
    if(is_eq(& m->hash[h].ind, key))
    {
      free_svalue(&(m->hash[h].ind));
      free_svalue(&(m->hash[h].val));
      m->hash[h].ind.type=T_DELETED;
      m->size--;

      if(m->size * 100 < m->hashsize * MIN_FILL)
	rehash(m, m->hashsize / 2 + 1);

      return;
    }
    if(++h == m->hashsize) h=0;
  }
#endif
}

void check_mapping_for_destruct(struct mapping *m)
{
#ifndef FLAT_MAPPINGS
  INT32 e;
  struct keypair *k, **prev;
  TYPE_FIELD ind_types, val_types;

#ifdef DEBUG
  if(d_flag > 1) check_mapping_type_fields(m);
#endif

  if((m->ind_types | m->val_types) & BIT_OBJECT)
  {
    val_types = ind_types = 0;
    for(e=0;e<m->hashsize;e++)
    {
      for(prev= m->hash + e;k=*prev;prev=&k->next)
      {
	check_destructed(& k->val);
	
	if(k->ind.type == T_OBJECT && !k->ind.u.object->prog)
	{
	  *prev=k->next;
	  free_svalue(& k->ind);
	  free_svalue(& k->val);
	  k->next=m->free_list;
	  m->free_list=k;
	  m->size--;
	}else{
	  val_types |= 1 << k->val.type;
	  ind_types |= 1 << k->ind.type;
	}
      }
    }
    if(MAP_SLOTS(m->size) < m->hashsize * MIN_LINK_LENGTH)
      rehash(m, MAP_SLOTS(m->size));

    m->val_types = val_types;
    m->ind_types = ind_types;
  }
#else
  INT32 e;
  TYPE_FIELD ind_types, val_types;

#ifdef DEBUG
  if(d_flag > 1) check_mapping_type_fields(m);
#endif

  if((m->ind_types | m->val_types) & BIT_OBJECT)
  {
    val_types = ind_types = 0;
    for(e=0;e<m->hashsize;e++)
    {
      if(m->hash[e].ind.type > MAX_TYPE) continue;

      check_destructed(& m->hash[e].val);

      if(m->hash[e].ind.type == T_OBJECT &&
	 !m->hash[e].ind.u.object->prog)
      {
	free_svalue(& m->hash[e].ind);
	free_svalue(& m->hash[e].val);
	m->hash[e].ind.type=T_VOID;
	m->size--;
      }
    }

    if(m->size * 100 + 1 < m->hashsize * MIN_FILL)
      rehash(m, m->size / 2+1);

    m->val_types = val_types;
    m->ind_types = ind_types;
  }
#endif
}

static struct svalue *low_mapping_lookup(struct mapping *m,
					 struct svalue *key)
{
#ifndef FLAT_MAPPINGS
  unsigned INT32 h;
  struct keypair *k, **prev;

#ifdef DEBUG
  if(d_flag > 1) check_mapping_type_fields(m);
#endif

  if((1 << key->type) & m->ind_types)
  {
    h=hash_svalue(key) % m->hashsize;
  
    for(prev= m->hash + h;k=*prev;prev=&k->next)
    {
      if(is_eq(& k->ind, key))
      {
	*prev=k->next;
	k->next=m->hash[h];
	m->hash[h]=k;
	
	return &k->val;
      }
    }
  }

#else
  unsigned INT32 h;

#ifdef DEBUG
  if(d_flag > 1) check_mapping_type_fields(m);
#endif

  if((1 << key->type) & m->ind_types)
  {
    h=hash_svalue(key) % m->hashsize;
    while(m->hash[h].ind.type != T_VOID)
    {
      if(is_eq(& m->hash[h].ind, key))
	return & m->hash[h].val;

      if(++h == m->hashsize) h=0;
    }
  }
#endif
  return 0;
}

void mapping_index_no_free(struct svalue *dest,
			   struct mapping *m,
			   struct svalue *key)
{
  struct svalue *p;

  if(p=low_mapping_lookup(m,key))
  {
    assign_svalue_no_free(dest, p);
  }else{
    dest->type=T_INT;
    dest->u.integer=0;
    dest->subtype=NUMBER_UNDEFINED;
  }
}

struct array *mapping_indices(struct mapping *m)
{
  INT32 e;
  struct array *a;
  struct svalue *s;
  struct keypair *k;

  a=allocate_array(m->size);
  s=ITEM(a);

  LOOP(m) assign_svalue(s++, & k->ind);

  a->type_field = m->ind_types;
  
  return a;
}

struct array *mapping_values(struct mapping *m)
{
  INT32 e;
  struct keypair *k;
  struct array *a;
  struct svalue *s;

  a=allocate_array(m->size);
  s=ITEM(a);

  LOOP(m)
  {
    check_destructed(& k->val);
    assign_svalue(s++, & k->val);
  }

  a->type_field = m->val_types;
  
  return a;
}

void mapping_replace(struct mapping *m,struct svalue *from, struct svalue *to)
{
  INT32 e;
  struct keypair *k;

  LOOP(m)
    if(is_eq(& k->val, from))
      assign_svalue(& k->val, to);

  m->val_types |= 1 << to->type;
}

struct mapping *mkmapping(struct array *ind, struct array *val)
{
  struct mapping *m;
  struct svalue *i,*v;
  INT32 e;

#ifdef DEBUG
  if(ind->size != val->size)
    fatal("mkmapping on different sized arrays.\n");
#endif

  m=allocate_mapping(MAP_SLOTS(ind->size));
  i=ITEM(ind);
  v=ITEM(val);
  for(e=0;e<ind->size;e++) mapping_insert(m, i++, v++);

  return m;
}

struct mapping *copy_mapping(struct mapping *m)
{
  INT32 e;
  struct mapping *n;
  struct keypair *k;

  n=allocate_mapping(MAP_SLOTS(m->size));

  LOOP(m)
  {
    /* check_destructed(& k->ind); */
    check_destructed(& k->val);

    mapping_insert(n, &k->ind, &k->val);
  }
  
  return m;
}

struct mapping *merge_mappings(struct mapping *a, struct mapping *b, INT32 op)
{
  struct array *ai, *av;
  struct array *bi, *bv;
  struct array *ci, *cv;
  INT32 *zipper;
  struct mapping *m;

  ai=mapping_indices(a);
  av=mapping_values(a);
  zipper=get_set_order(ai);
  order_array(ai, zipper);
  order_array(av, zipper);
  free((char *)zipper);

  bi=mapping_indices(b);
  bv=mapping_values(b);
  zipper=get_set_order(bi);
  order_array(bi, zipper);
  order_array(bv, zipper);
  free((char *)zipper);

  zipper=merge(ai,bi,op);

  ci=array_zip(ai,bi,zipper);
  free_array(ai);
  free_array(bi);

  cv=array_zip(av,bv,zipper);
  free_array(av);
  free_array(bv);
  
  free((char *)zipper);

  m=mkmapping(ci, cv);
  free_array(ci);
  free_array(cv);

  return m;
}

struct mapping *add_mappings(struct svalue *argp, INT32 args)
{
  struct mapping *ret,*a,*b;
  switch(args)
  {
  case 0: return allocate_mapping(0);
  case 1: return copy_mapping(argp->u.mapping);
  case 2: return merge_mappings(argp[0].u.mapping, argp[1].u.mapping, OP_ADD);
  case 3:
    a=merge_mappings(argp[0].u.mapping,argp[1].u.mapping,OP_ADD);
    ret=merge_mappings(a,argp[2].u.mapping,OP_ADD);
    free_mapping(a);
    return ret;
  default:
    a=add_mappings(argp,args/2);
    b=add_mappings(argp+args/2,args-args/2);
    ret=merge_mappings(a,b,OP_ADD);
    free_mapping(a);
    free_mapping(b);
    return ret;
  }
}

int mapping_equal_p(struct mapping *a, struct mapping *b, struct processing *p)
{
  struct processing curr;
  struct keypair *k;
  INT32 e;

  if(a==b) return 1;
  if(a->size != b->size) return 0;

  curr.pointer_a = a;
  curr.pointer_b = b;
  curr.next = p;

  for( ;p ;p=p->next)
    if(p->pointer_a == (void *)a && p->pointer_b == (void *)b)
      return 1;

  check_mapping_for_destruct(a);
  check_mapping_for_destruct(b);
  
  LOOP(a)
  {
    struct svalue *s;
    if(s=low_mapping_lookup(b, & k->ind))
    {
      if(!low_is_equal(s, &k->val, &curr)) return 0;
    }else{
      return 0;
    }
  }
  return 1;
}

void describe_mapping(struct mapping *m,struct processing *p,int indent)
{
  struct processing doing;
  INT32 e,d;
  struct keypair *k;
  char buf[40];

  if(! m->size)
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
  
  sprintf(buf,"([ /* %ld elements */\n",(long) m->size);
  my_strcat(buf);

  d=0;

  LOOP(m)
  {
    if(!d)
    {
      my_strcat(",\n");
      d=1;
    }
    for(d=0; d<indent; d++) my_putchar(' ');
    describe_svalue(& k->ind, indent+2, p);
    my_putchar(':');
    describe_svalue(& k->val, indent+2, p);
  }

  my_putchar('\n');
  for(e=2; e<indent; e++) my_putchar(' ');
  my_strcat("])");
}

node * make_node_from_mapping(struct mapping *m)
{
  struct keypair *k;
  INT32 e;

  mapping_fix_type_field(m);

  if((m->ind_types | m->val_types) & (BIT_FUNCTION | BIT_OBJECT))
  {
    struct array *ind, *val;
    node *n;
    ind=mapping_indices(m);
    val=mapping_indices(m);
    n=mkefuncallnode("mkmapping",mknode(F_ARG_LIST,make_node_from_array(ind),make_node_from_array(val)));
    free_array(ind);
    free_array(val);
    return n;
  }else{
    struct svalue s;
    s.type=T_MAPPING;
    s.subtype=0;
    s.u.mapping=m;
    return mkconstantsvaluenode(&s);
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
  struct keypair *k;
  struct mapping *m;

  if(args & 1)
    error("Uneven number of arguments to aggregage_mapping.\n");

  m=allocate_mapping(MAP_SLOTS(args / 2));

  for(e=-args;e<0;e+=2) mapping_insert(m, sp+e, sp+e+1);
  pop_n_elems(args);
  push_mapping(m);
}

struct mapping *copy_mapping_recursively(struct mapping *m,
					 struct processing *p)
{
  struct processing doing;
  struct mapping *ret;
  INT32 e;
  struct keypair *k;

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

  ret=allocate_mapping(MAP_SLOTS(m->size));
  doing.pointer_b=ret;

  check_stack(2);

  LOOP(m)
  {
    /* check_destructed(& k->ind); */
    check_destructed(& k->val);
    
    copy_svalues_recursively_no_free(sp,&k->ind, 1, p);
    sp++;
    copy_svalues_recursively_no_free(sp,&k->val, 1, p);
    sp++;
    
    mapping_insert(ret, sp-2, sp-1);
    pop_n_elems(2);
  }

  return ret;
}


void mapping_search_no_free(struct svalue *to,
			    struct mapping *m,
			    struct svalue *look_for,
			    struct svalue *start)
{
#ifndef FLAT_MAPPINGS
  unsigned INT32 h;
  struct keypair *k;

  h=0;
  k=m->hash[h];
  if(start)
  {
    h=hash_svalue(start) % m->hashsize;
    for(k=m->hash[h];k;k=k->next)
      if(is_eq(&k->ind, start))
	break;

    if(!k)
    {
      to->type=T_INT;
      to->subtype=NUMBER_UNDEFINED;
      to->u.integer=0;
      return;
    }
    k=k->next;
  }

  while(h < (unsigned INT32)m->hashsize)
  {
    while(k)
    {
      if(is_eq(look_for, &k->val))
      {
	assign_svalue_no_free(to,&k->ind);
	return;
      }
      k=k->next;
    }
    k=m->hash[++h];
  }
#else
  unsigned INT32 h;

  h=0;
  if(start)
  {
    h=hash_svalue(start) % m->hashsize;
    
    while(1)
    {
      if(m->hash[h].ind.type != T_VOID)
      {
	to->type=T_INT;
	to->subtype=NUMBER_UNDEFINED;
	to->u.integer=0;
	return;
      }
      
      if(is_eq(& m->hash[h].ind, start))
	break;
      if(++h == m->hashsize) h=0;
    }
    if(++h == m->hashsize) h=0;
  }

  for(h=0;h<m->hashsize;h++)
  {
    if(m->hash[h].ind.type <= MAX_TYPE)
    {
      if(is_eq(& m->hash[h].val, look_for))
      {
	assign_svalue_no_free(to, & m->hash[h].ind);
	return;
      }
    }
  }
	
#endif

  to->type=T_INT;
  to->subtype=NUMBER_UNDEFINED;
  to->u.integer=0;
}


#ifdef DEBUG

void check_mapping(struct mapping *m)
{
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

  if(m->hashsize < 0)
    fatal("Assert: I don't think he's going to make it Jim.\n");

  if(m->size < 0)
    fatal("Core breach, evacuate ship!\n");

#ifndef FLAT_MAPPINGS
  if(m->size > (m->hashsize + 3) * AVG_LINK_LENGTH)
    fatal("Pretty mean hashtable there buster!.\n");
  
  if(m->size < (m->hashsize - 3) * MIN_LINK_LENGTH)
    fatal("Hashsize is too small for mapping.\n");
#endif
  
  if(m->size > 0 && (!m->ind_types || !m->val_types))
    fatal("Mapping type fields are... wrong.\n");

  if(!m->hash)
    fatal("Hey! where did my hashtable go??\n");
  
}

void check_all_mappings()
{
  struct mapping *m;
  for(m=first_mapping;m;m=m->next)
    check_mapping(m);
}
#endif


#ifdef GC2

void gc_mark_mapping_as_referenced(struct mapping *m)
{
  INT32 e;
  struct keypair *k;

  if(gc_mark(m))
  {
    if((m->ind_types | m->val_types) & BIT_COMPLEX)
    {
      LOOP(m)
      {
	gc_mark_svalues(&k->ind, 1);
	gc_mark_svalues(&k->val, 1);
      }
    }
  }
}

void gc_check_all_mappings()
{
  INT32 e;
  struct keypair *k;
  struct mapping *m;

  for(m=first_mapping;m;m=m->next)
  {
    if((m->ind_types | m->val_types) & BIT_COMPLEX)
    {
      LOOP(m)
      {
	gc_check_svalues(&k->ind, 1);
	gc_check_svalues(&k->val, 1);
      }
    }
  }
}

void gc_mark_all_mappings()
{
  struct mapping *m;
  for(m=first_mapping;m;m=m->next)
    if(gc_is_referenced(m))
      gc_mark_mapping_as_referenced(m);
}

void gc_free_all_unreferenced_mappings()
{
  INT32 e;
  struct keypair *k;
  struct mapping *m,*next;

  for(m=first_mapping;m;m=next)
  {
    if(gc_do_free(m))
    {
      m->refs++;

#ifndef FLAT_MAPPINGS
      for(e=0;e<m->hashsize;e++)
      {
	for(k=m->hash[e];k;k=k->next)
	{
	  free_svalue(&k->ind);
	  free_svalue(&k->val);
	}
	k->next=m->free_list;
	m->hash[e]=0;
      }
#else
      for(e=0;e<m->hashsize;m++)
      {
	if((k=m->hash+e)->ind.type <= MAX_TYPE)
	{
	  free_svalue(&k->ind);
	  free_svalue(&k->val);
	}
	k->ind.type=T_VOID;
      }
#endif
      m->size=0;

      next=m->next;

      free_mapping(m);
    }else{
      next=m->next;
    }
  }
}

#endif /* GC2 */


#else /* OLD_MAPPINGS */
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
  GC_ALLOC();
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

  GC_FREE();
}


void mapping_fix_type_field(struct mapping *m)
{
  array_fix_type_field(m->ind);
  array_fix_type_field(m->val);
}

static void order_mapping(struct mapping *m)
{
  INT32 *order;
  order = get_set_order(m->ind);
  if(!order) return;
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

struct array *mapping_indices(struct mapping *m)
{
  return copy_array(m->ind);
}

struct array *mapping_values(struct mapping *m)
{
  return copy_array(m->val);
}

void mapping_replace(struct mapping *m,struct svalue *from, struct svalue *to)
{
  array_replace(m->val, from, to);
}


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
    ret=allocate_mapping(allocate_array_no_init(0,0),
			 allocate_array_no_init(0,0));
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

  ind=allocate_array_no_init(args/2,0);
  val=allocate_array_no_init(args/2,0);

  s=sp-args;
  for(e=0;e<args/2;e++)
  {
    ITEM(ind)[e]=*(s++);
    ITEM(val)[e]=*(s++);
  }
  sp-=args;
  ind->type_field=BIT_MIXED;
  val->type_field=BIT_MIXED;
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

  ret=allocate_mapping(&empty_array, &empty_array);
  doing.pointer_b=(void *)ret;
  empty_array.refs+=2;

  ret->ind=copy_array_recursively(m->ind,&doing);
  empty_array.refs--;
  ret->val=copy_array_recursively(m->val,&doing);
  empty_array.refs--;

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

void check_mapping(struct mapping *m)
{
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
}

void check_all_mappings()
{
  struct mapping *m;
  for(m=first_mapping;m;m=m->next)
    check_mapping(m);
}
#endif


#ifdef GC2

void gc_mark_mapping_as_referenced(struct mapping *m)
{
  if(gc_mark(m))
  {
    gc_mark_array_as_referenced(m->ind);
    gc_mark_array_as_referenced(m->val);
  }
}

void gc_check_all_mappings()
{
  struct mapping *m;
  for(m=first_mapping;m;m=m->next)
  {
    gc_check(m->ind);
    gc_check(m->val);
  }
}

void gc_mark_all_mappings()
{
  struct mapping *m;
  for(m=first_mapping;m;m=m->next)
    if(gc_is_referenced(m))
      gc_mark_mapping_as_referenced(m);
}

void gc_free_all_unreferenced_mappings()
{
  struct mapping *m,*next;

  for(m=first_mapping;m;m=next)
  {
    if(gc_do_free(m))
    {
      m->refs++;
      free_svalues(ITEM(m->ind), m->ind->size, m->ind->type_field);
      free_svalues(ITEM(m->val), m->val->size, m->val->type_field);
      m->ind->size=0;
      m->val->size=0;
      next=m->next;

      free_mapping(m);
    }else{
      next=m->next;
    }
  }
}

#endif /* GC2 */
#endif /* OLD_MAPPINGS */
