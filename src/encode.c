/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "multiset.h"
#include "dynamic_buffer.h"
#include "error.h"
#include "operators.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "fsort.h"
#include "threads.h"
#include "stuff.h"

RCSID("$Id: encode.c,v 1.22 1998/05/01 16:20:27 grubba Exp $");

#ifdef _AIX
#include <net/nh.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include <math.h>

#ifdef HAVE_FREXP
#define FREXP frexp
#else
double FREXP(double x, int *exp)
{
  double ret;
  *exp=(int)ceil(log(fabs(x))/log(2.0));
  ret=(x*pow(2.0,(float)-*exp));
  return ret;
}
#endif

#if HAVE_LDEXP
#define LDEXP ldexp
#else
double LDEXP(double x, int exp)
{
  return x * pow(2.0,(double)exp);
}
#endif

#ifdef DEBUG
#define encode_value2 encode_value2_
#define decode_value2 decode_value2_
#endif



struct encode_data
{
  struct object *codec;
  struct svalue counter;
  struct mapping *encoded;
  dynamic_buffer buf;
};

static void encode_value2(struct svalue *val, struct encode_data *data);

#define addstr(s, l) low_my_binary_strcat((s), (l), &(data->buf))
#define addchar(t)   low_my_putchar((t),&(data->buf))
#define adddata(S) do { \
  code_entry(T_STRING, (S)->len, data); \
  addstr((char *)((S)->str),(S)->len); \
}while(0)

#define adddata2(s,l) addstr((char *)(s),(l) * sizeof(s[0]));

/* Current encoding: ¶ik0 */
#define T_AGAIN 15
#define T_MASK 15
#define T_NEG 16
#define T_SMALL 32
#define SIZE_SHIFT 6
#define MAX_SMALL (1<<(8-SIZE_SHIFT))
#define COUNTER_START -MAX_SMALL

/* Let's cram those bits... */
static void code_entry(int type, INT32 num, struct encode_data *data)
{
  int t;
  if(num<0)
  {
    type|=T_NEG;
    num=~num;
  }

  if(num < MAX_SMALL)
  {
    type|=T_SMALL | (num << SIZE_SHIFT);
    addchar(type);
    return;
  }else{
    num-=MAX_SMALL;
  }

  for(t=0;t<3;t++)
  {
    if(num >= (256 << (t<<3)))
      num-=(256 << (t<<3));
    else
      break;
  }

  type|=t << SIZE_SHIFT;
  addchar(type);

  switch(t)
  {
  case 3: addchar(num >> 24);
  case 2: addchar(num >> 16);
  case 1: addchar(num >> 8);
  case 0: addchar(num);
  }
}


static void code_number(INT32 num, struct encode_data *data)
{
  code_entry(num & 15, num >> 4, data);
}

#ifdef _REENTRANT
static void do_enable_threads(void)
{
  if(!--threads_disabled)
    co_broadcast(&threads_disabled_change);
}
#endif

static int encode_type(char *t, struct encode_data *data)
{
  char *q=t;
one_more_type:
  addchar(EXTRACT_UCHAR(t));
  switch(EXTRACT_UCHAR(t++))
  {
    default:
      fatal("error in type string.\n");
      /*NOTREACHED*/
      
      break;
      
    case T_ASSIGN:
      addchar(EXTRACT_UCHAR(t++));
      goto one_more_type;
      
    case T_FUNCTION:
      while(EXTRACT_UCHAR(t)!=T_MANY)
	t+=encode_type(t, data);
      addchar(EXTRACT_UCHAR(t++));
      
    case T_MAPPING:
    case T_OR:
    case T_AND:
      t+=encode_type(t, data);
      
    case T_ARRAY:
    case T_MULTISET:
    case T_NOT:
      goto one_more_type;
      
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case T_INT:
    case T_FLOAT:
    case T_STRING:
    case T_PROGRAM:
    case T_MIXED:
    case T_VOID:
    case T_UNKNOWN:
      break;
      
    case T_OBJECT:
    {
      INT32 x;
      addchar(EXTRACT_UCHAR(t++));
      x=EXTRACT_INT(t);
      t+=sizeof(INT32);
      if(x)
      {
	struct program *p=id_to_program(x);
	if(p)
	{
	  ref_push_program(p);
	}else{
	  push_int(0);
	}
      }else{
	push_int(0);
      }
      encode_value2(sp-1, data);
      pop_stack();
      break;
    }
  }
  return t-q;
}


static void encode_value2(struct svalue *val, struct encode_data *data)

#ifdef DEBUG
#undef encode_value2
#define encode_value2(X,Y) do { struct svalue *_=sp; encode_value2_(X,Y); if(sp!=_) fatal("encode_value2 failed!\n"); } while(0)
#endif

{
  static struct svalue dested = { T_INT, NUMBER_DESTRUCTED };
  INT32 i;
  struct svalue *tmp;
  
  if((val->type == T_OBJECT || (val->type==T_FUNCTION && \
				val->subtype!=FUNCTION_BUILTIN)) && !val->u.object->prog)
    val=&dested;
  
  if((tmp=low_mapping_lookup(data->encoded, val)))
  {
    code_entry(T_AGAIN, tmp->u.integer, data);
    return;
  }else{
    mapping_insert(data->encoded, val, &data->counter);
    data->counter.u.integer++;
  }
  
  
  switch(val->type)
  {
    case T_INT:
      code_entry(T_INT, val->u.integer,data);
      break;
      
    case T_STRING:
      code_entry(T_STRING, val->u.string->len,data);
      addstr(val->u.string->str, val->u.string->len);
      break;
      
    case T_FLOAT:
    {
      if(val->u.float_number==0.0)
      {
	code_entry(T_FLOAT,0,data);
	code_entry(T_FLOAT,0,data);
      }else{
	INT32 x;
	int y;
	double tmp;
	
	tmp=FREXP((double)val->u.float_number, &y);
	x=(INT32)((1<<30)*tmp);
	y-=30;
#if 0
	while(x && y && !(x&1))
	{
	  x>>=1;
	  y++;
	}
#endif
	code_entry(T_FLOAT,x,data);
	code_entry(T_FLOAT,y,data);
      }
      break;
    }
    
    case T_ARRAY:
      code_entry(T_ARRAY, val->u.array->size, data);
      for(i=0; i<val->u.array->size; i++)
	encode_value2(ITEM(val->u.array)+i, data);
      break;
      
    case T_MAPPING:
      check_stack(2);
      ref_push_mapping(val->u.mapping);
      f_indices(1);
      
      ref_push_mapping(val->u.mapping);
      f_values(1);
      
      code_entry(T_MAPPING, sp[-2].u.array->size,data);
      for(i=0; i<sp[-2].u.array->size; i++)
      {
	encode_value2(ITEM(sp[-2].u.array)+i, data); /* indices */
	encode_value2(ITEM(sp[-1].u.array)+i, data); /* values */
      }
      pop_n_elems(2);
      break;
      
    case T_MULTISET:
      code_entry(T_MULTISET, val->u.multiset->ind->size,data);
      for(i=0; i<val->u.multiset->ind->size; i++)
	encode_value2(ITEM(val->u.multiset->ind)+i, data);
      break;
      
    case T_OBJECT:
      check_stack(1);
      push_svalue(val);
      apply(data->codec, "nameof", 1);
      switch(sp[-1].type)
      {
	case T_INT:
	  if(sp[-1].subtype == NUMBER_UNDEFINED) 
	  {
	    pop_stack();
	    push_svalue(val);
	    f_object_program(1);
	    
	    code_entry(val->type, 1,data);
	    encode_value2(sp-1, data);
	    pop_stack();
	    
	    push_svalue(val);
	    apply(data->codec,"encode_object",1);
	    break;
	  }
	  /* FALL THROUGH */
	
	default:
	  code_entry(val->type, 0,data);
	  break;
	  
      }
      encode_value2(sp-1, data);
      pop_stack();
      break;
      
    case T_FUNCTION:
      check_stack(1);
      push_svalue(val);
      apply(data->codec,"nameof", 1);
      if(sp[-1].type == T_INT && sp[-1].subtype==NUMBER_UNDEFINED)
      {
	if(val->subtype != FUNCTION_BUILTIN)
	{
	  int eq;
	  code_entry(val->type, 1, data);
	  push_svalue(val);
	  sp[-1].type=T_OBJECT;
	  ref_push_string(ID_FROM_INT(val->u.object->prog, val->subtype)->name);
	  f_arrow(2);
	  eq=is_eq(sp-1, val);
	  pop_stack();
	  if(eq)
	  {
	    /* We have to remove ourself from the cache for now */
	    struct svalue tmp=data->counter;
	    tmp.u.integer--;
	    map_delete(data->encoded, val);
	    
	    push_svalue(val);
	    sp[-1].type=T_OBJECT;
	    encode_value2(sp-1, data);
	    ref_push_string(ID_FROM_INT(val->u.object->prog, val->subtype)->name);
	    encode_value2(sp-1, data);
	    pop_n_elems(3);
	    
	    /* Put value back in cache */
	    mapping_insert(data->encoded, val, &tmp);
	    return;
	  }
	}
      }

      code_entry(val->type, 0,data);
      encode_value2(sp-1, data);
      pop_stack();
      break;
      
      
    case T_PROGRAM:
    {
      int d;
      check_stack(1);
      push_svalue(val);
      apply(data->codec,"nameof", 1);
      if(sp[-1].type == val->type)
	error("Error in master()->nameof(), same type returned.\n");
      if(sp[-1].type == T_INT && sp[-1].subtype == NUMBER_UNDEFINED)
      {
	INT32 e;
	struct program *p=val->u.program;
	if(p->init || p->exit || p->gc_marked || p->gc_check ||
	   (p->flags & PROGRAM_HAS_C_METHODS))
	  error("Cannot encode C programs.\n");
	code_entry(val->type, 1,data);
	code_number(p->flags,data);
	code_number(p->storage_needed,data);
	code_number(p->timestamp.tv_sec,data);
	code_number(p->timestamp.tv_usec,data);

#define FOO(X,Y,Z) \
	code_number( p->num_##Z, data);
#include "program_areas.h"

	adddata2(p->program, p->num_program);
	adddata2(p->linenumbers, p->num_linenumbers);

	for(d=0;d<p->num_identifier_index;d++)
	  code_number(p->identifier_index[d],data);

	for(d=0;d<p->num_variable_index;d++)
	  code_number(p->variable_index[d],data);

	for(d=0;d<p->num_identifier_references;d++)
	{
	  code_number(p->identifier_references[d].inherit_offset,data);
	  code_number(p->identifier_references[d].identifier_offset,data);
	  code_number(p->identifier_references[d].id_flags,data);
	}
	  
	for(d=0;d<p->num_strings;d++) adddata(p->strings[d]);

	for(d=1;d<p->num_inherits;d++)
	{
	  code_number(p->inherits[d].inherit_level,data);
	  code_number(p->inherits[d].identifier_level,data);
	  code_number(p->inherits[d].parent_offset,data);
	  code_number(p->inherits[d].storage_offset,data);

	  if(p->inherits[d].parent)
	  {
	    ref_push_object(p->inherits[d].parent);
	    sp[-1].subtype=p->inherits[d].parent_identifier;
	    sp[-1].type=T_FUNCTION;
	  }else if(p->inherits[d].prog){
	    ref_push_program(p->inherits[d].prog);
	  }else{
	    push_int(0);
	  }
	  encode_value2(sp-1,data);
	  pop_stack();

	  adddata(p->inherits[d].name);
	}

	for(d=0;d<p->num_identifiers;d++)
	{
	  adddata(p->identifiers[d].name);
	  encode_type(p->identifiers[d].type->str, data);
	  code_number(p->identifiers[d].identifier_flags,data);
	  code_number(p->identifiers[d].run_time_type,data);
	  code_number(p->identifiers[d].func.offset,data);
	}

	for(d=0;d<p->num_constants;d++)
	  encode_value2(p->constants+d, data);

	for(d=0;d<NUM_LFUNS;d++)
	  code_number(p->lfuns[d], data);
      }else{
	code_entry(val->type, 0,data);
	encode_value2(sp-1, data);
      }
      pop_stack();
      break;
      }
  }
}

static void free_encode_data(struct encode_data *data)
{
  toss_buffer(& data->buf);
  free_mapping(data->encoded);
}

void f_encode_value(INT32 args)
{
  ONERROR tmp;
  struct encode_data d, *data;
  data=&d;
  
  check_all_args("encode_value", args, BIT_MIXED, BIT_VOID | BIT_OBJECT, 0);

  initialize_buf(&data->buf);
  data->encoded=allocate_mapping(128);
  data->counter.type=T_INT;
  data->counter.u.integer=COUNTER_START;
  if(args > 1)
  {
    data->codec=sp[1-args].u.object;
  }else{
    data->codec=get_master();
  }

  SET_ONERROR(tmp, free_encode_data, data);
  addstr("\266ke0", 4);
  encode_value2(sp-args, data);
  UNSET_ONERROR(tmp);

  free_mapping(data->encoded);

  pop_n_elems(args);
  push_string(low_free_buf(&data->buf));
}

struct decode_data
{
  unsigned char *data;
  INT32 len;
  INT32 ptr;
  struct mapping *decoded;
  struct svalue counter;
  struct object *codec;
  int pickyness;
};

static void decode_value2(struct decode_data *data);

static int my_extract_char(struct decode_data *data)
{
  if(data->ptr >= data->len)
    error("Format error, not enough data in string.\n");
  return data->data [ data->ptr++ ];
}

#define GETC() my_extract_char(data)

#define DECODE() \
  what=GETC(); \
  e=what>>SIZE_SHIFT; \
  if(what & T_SMALL)  { \
     num=e; \
  } else { \
     num=0; \
     while(e-->=0) num=(num<<8) + (GETC()+1); \
     num+=MAX_SMALL - 1; \
  } \
  if(what & T_NEG) num=~num


#define decode_entry(X,Y,Z)					\
  do {								\
    INT32 what, e, num;                                         \
    DECODE();							\
    if((what & T_MASK) != (X)) error("Failed to decode, wrong bits (%d).\n", what & T_MASK);\
    (Y)=num;							\
  } while(0);

#define getdata2(S,L) do {						\
      if(data->ptr + (long)(sizeof(S[0])*(L)) > data->len)		\
	error("Failed to decode string. (string range error)\n");	\
      MEMCPY((S),(data->data + data->ptr), sizeof(S[0])*(L));		\
      data->ptr+=sizeof(S[0])*(L);					\
  }while(0)

#define getdata(X) do {							     \
   long length;								     \
      decode_entry(T_STRING, length,data);				     \
      if(data->ptr + length > data->len || length <0)			     \
	error("Failed to decode string. (string range error)\n");	     \
      X=make_shared_binary_string((char *)(data->data + data->ptr), length); \
      data->ptr+=length;						     \
  }while(0)

#define decode_number(X,data) do {	\
   int what, e, num;				\
   DECODE();					\
   X=(what & T_MASK) | (num<<4);		\
  }while(0)					\


static void low_decode_type(struct decode_data *data)
{
  int tmp;
one_more_type:
  push_type(tmp=GETC());
  switch(tmp)
  {
    default:
      fatal("error in type string.\n");
      /*NOTREACHED*/
      break;
      
    case T_ASSIGN:
      push_type(GETC());
      goto one_more_type;
      
    case T_FUNCTION:
      while(GETC()!=T_MANY)
      {
	data->ptr--;
	low_decode_type(data);
      }
      push_type(T_MANY);
      
    case T_MAPPING:
    case T_OR:
    case T_AND:
      low_decode_type(data);

    case T_ARRAY:
    case T_MULTISET:
    case T_NOT:
      goto one_more_type;
      
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case T_INT:
    case T_FLOAT:
    case T_STRING:
    case T_PROGRAM:
    case T_MIXED:
    case T_VOID:
    case T_UNKNOWN:
      break;
      
    case T_OBJECT:
    {
      INT32 x;
      push_type(GETC());
      decode_value2(data);
      type_stack_mark();
      switch(sp[-1].type)
      {
	case T_INT:
	  push_type_int(0);
	  break;

	case T_PROGRAM:
	  push_type_int(sp[-1].u.program->id);
	  break;
	  
	default:
	  error("Failed to decode type.\n");
      }
      pop_stack();
      type_stack_reverse();
    }
  }
}

/* This really needs to disable threads.... */
#define decode_type(X,data)  do {		\
  type_stack_mark();				\
  type_stack_mark();				\
  low_decode_type(data);			\
  type_stack_reverse();				\
  (X)=pop_unfinished_type();			\
} while(0)

static void decode_value2(struct decode_data *data)

#ifdef DEBUG
#undef decode_value2
#define decode_value2(X) do { struct svalue *_=sp; decode_value2_(X); if(sp!=_+1) fatal("decode_value2 failed!\n"); } while(0)
#endif


{
  INT32 what, e, num;
  struct svalue tmp, *tmp2;
  
  DECODE();
  
  check_stack(1);
  
  switch(what & T_MASK)
  {
    case T_AGAIN:
      tmp.type=T_INT;
      tmp.subtype=0;
      tmp.u.integer=num;
      if((tmp2=low_mapping_lookup(data->decoded, &tmp)))
      {
	push_svalue(tmp2);
      }else{
	error("Failed to decode string. (invalid T_AGAIN)\n");
      }
      return;
      
    case T_INT:
      tmp=data->counter;
      data->counter.u.integer++;
      push_int(num);
      break;
      
    case T_STRING:
      tmp=data->counter;
      data->counter.u.integer++;
      if(data->ptr + num > data->len)
	error("Failed to decode string. (string range error)\n");
      push_string(make_shared_binary_string((char *)(data->data + data->ptr), num));
      data->ptr+=num;
      break;
      
    case T_FLOAT:
    {
      INT32 num2=num;
      
      tmp=data->counter;
      data->counter.u.integer++;
      
      DECODE();
      push_float(LDEXP((double)num2, num));
      break;
    }
    
    case T_ARRAY:
    {
      struct array *a=allocate_array(num);
      tmp.type=T_ARRAY;
      tmp.u.array=a;
      mapping_insert(data->decoded, & data->counter, &tmp);
      data->counter.u.integer++;
      
      /* Since a reference to the array is stored in the mapping, we can
       * safely decrease this reference here. Thus it will be automatically
       * freed if something goes wrong.
       */
      a->refs--;
      
      for(e=0;e<num;e++)
      {
	decode_value2(data);
	ITEM(a)[e]=sp[-1];
	sp--;
      }
      ref_push_array(a);
      return;
    }
    
    case T_MAPPING:
    {
      struct mapping *m;
      if(num<0)
	error("Failed to decode string. (mapping size is negative)\n");

      m=allocate_mapping(num);
      tmp.type=T_MAPPING;
      tmp.u.mapping=m;
      mapping_insert(data->decoded, & data->counter, &tmp);
      data->counter.u.integer++;
      m->refs--;
      
      for(e=0;e<num;e++)
      {
	decode_value2(data);
	decode_value2(data);
	mapping_insert(m, sp-2, sp-1);
	pop_n_elems(2);
      }
      ref_push_mapping(m);
      return;
    }
    
    case T_MULTISET:
    {
      struct multiset *m=mkmultiset(low_allocate_array(0, num));
      tmp.type=T_MULTISET;
      tmp.u.multiset=m;
      mapping_insert(data->decoded, & data->counter, &tmp);
      data->counter.u.integer++;
      m->refs--;
      
      for(e=0;e<num;e++)
      {
	decode_value2(data);
	multiset_insert(m, sp-1);
	pop_stack();
      }
      ref_push_multiset(m);
      return;
    }
    
    
    case T_OBJECT:
      tmp=data->counter;
      data->counter.u.integer++;
      decode_value2(data);
      
      switch(num)
      {
	case 0:
	  if(data->codec)
	  {
	    apply(data->codec,"objectof", 1);
	  }else{
	    ref_push_mapping(get_builtin_constants());
	    stack_swap();
	    f_index(2);
	  }
	  break;
	  
	case 1:
	  if(IS_ZERO(sp-1))
	  {
	    mapping_insert(data->decoded, &tmp, sp-1);
	    decode_value2(data);
	    pop_stack();
	  }else{
	    f_call_function(1);
	    mapping_insert(data->decoded, &tmp, sp-1);
	    push_svalue(sp-1);
	    decode_value2(data);
	    if(!data->codec)
	      error("Failed to decode (no codec)\n");
	    apply(data->codec,"decode_object",2);
	    pop_stack();
	  }
	  if(data->pickyness && sp[-1].type != T_OBJECT)
	    error("Failed to decode object.\n");
	  return;
	  
	default:
	  error("Object coding not compatible.\n");
	  break;
      }
      if(data->pickyness && sp[-1].type != T_OBJECT)
	error("Failed to decode.\n");
      break;
      
    case T_FUNCTION:
      tmp=data->counter;
      data->counter.u.integer++;
      decode_value2(data);
      
      switch(num)
      {
	case 0:
	  if(data->codec)
	  {
	    apply(data->codec,"functionof", 1);
	  }else{
	    ref_push_mapping(get_builtin_constants());
	    stack_swap();
	    f_index(2);
	  }
	  break;
	  
	case 1:
	  decode_value2(data);
	  f_arrow(2);
	  break;
	  
	default:
	  error("Function coding not compatible.\n");
	  break;
      }
      if(data->pickyness && sp[-1].type != T_FUNCTION)
	error("Failed to decode function.\n");
      break;
      
      
    case T_PROGRAM:
      switch(num)
      {
	case 0:
	  tmp=data->counter;
	  data->counter.u.integer++;
	  decode_value2(data);
	  if(data->codec)
	  {
	    apply(data->codec,"programof", 1);
	  }else{
	    ref_push_mapping(get_builtin_constants());
	    stack_swap();
	    f_index(2);
	  }
	  if(data->pickyness && sp[-1].type != T_PROGRAM)
	    error("Failed to decode program.\n");
	  break;

	case 1:
	{
	  int d;
	  SIZE_T size=0;
	  char *dat;
	  struct program *p;

#ifdef _REENTRANT
	  ONERROR err;
	  threads_disabled++;
	  SET_ONERROR(err, do_enable_threads, 0);
#endif

	  p=low_allocate_program();
	  debug_malloc_touch(p);
	  tmp.type=T_PROGRAM;
	  tmp.u.program=p;
	  mapping_insert(data->decoded, & data->counter, &tmp);
	  data->counter.u.integer++;
	  p->refs--;
	  
	  decode_number(p->flags,data);
	  p->flags &= ~(PROGRAM_FINISHED | PROGRAM_OPTIMIZED);
	  decode_number(p->storage_needed,data);
	  decode_number(p->timestamp.tv_sec,data);
	  decode_number(p->timestamp.tv_usec,data);

#define FOO(X,Y,Z) \
	  decode_number( p->num_##Z, data);
#include "program_areas.h"
	  
#define FOO(NUMTYPE,TYPE,NAME) \
          size=DO_ALIGN(size, ALIGNOF(TYPE)); \
          size+=p->PIKE_CONCAT(num_,NAME)*sizeof(p->NAME[0]);
#include "program_areas.h"

	  dat=xalloc(size);
	  debug_malloc_touch(dat);
	  MEMSET(dat,0,size);
	  size=0;
#define FOO(NUMTYPE,TYPE,NAME) \
	  size=DO_ALIGN(size, ALIGNOF(TYPE)); \
          p->NAME=(TYPE *)(dat+size); \
          size+=p->PIKE_CONCAT(num_,NAME)*sizeof(p->NAME[0]);
#include "program_areas.h"

	  for(e=0;e<p->num_constants;e++)
	    p->constants[e].type=T_INT;

	  debug_malloc_touch(dat);

	  p->total_size=size + sizeof(struct program);

	  p->flags |= PROGRAM_OPTIMIZED;

	  getdata2(p->program, p->num_program);
	  getdata2(p->linenumbers, p->num_linenumbers);

	  for(d=0;d<p->num_identifier_index;d++)
	  {
	    decode_number(p->identifier_index[d],data);
	    if(p->identifier_index[d] > p->num_identifier_references)
	    {
	      p->identifier_index[d]=0;
	      error("Malformed program in decode.\n");
	    }
	  }
	  
	  for(d=0;d<p->num_variable_index;d++)
	  {
	    decode_number(p->variable_index[d],data);
	    if(p->variable_index[d] > p->num_identifiers)
	    {
	      p->variable_index[d]=0;
	      error("Malformed program in decode.\n");
	    }
	  }
	  
	  for(d=0;d<p->num_identifier_references;d++)
	  {
	    decode_number(p->identifier_references[d].inherit_offset,data);
	    if(p->identifier_references[d].inherit_offset > p->num_inherits)
	    {
	      p->identifier_references[d].inherit_offset=0;
	      error("Malformed program in decode.\n");
	    }
	    decode_number(p->identifier_references[d].identifier_offset,data);
	    decode_number(p->identifier_references[d].id_flags,data);
	  }
	  
	  for(d=0;d<p->num_strings;d++)
	    getdata(p->strings[d]);

	  debug_malloc_touch(dat);

	  data->pickyness++;
	  p->inherits[0].prog=p;
	  p->inherits[0].parent_offset=1;

	  for(d=1;d<p->num_inherits;d++)
	  {
	    decode_number(p->inherits[d].inherit_level,data);
	    decode_number(p->inherits[d].identifier_level,data);
	    decode_number(p->inherits[d].parent_offset,data);
	    decode_number(p->inherits[d].storage_offset,data);
	    
	    decode_value2(data);
	    switch(sp[-1].type)
	    {
	      case T_FUNCTION:
		if(sp[-1].subtype == FUNCTION_BUILTIN)
		  error("Failed to decode parent.\n");
		
		p->inherits[d].parent_identifier=sp[-1].subtype;
		p->inherits[d].prog=program_from_svalue(sp-1);
		if(!p->inherits[d].prog)
		  error("Failed to decode parent.\n");
		add_ref(p->inherits[d].prog);
		p->inherits[d].parent=sp[-1].u.object;
		sp--;
		break;

	      case T_PROGRAM:
		p->inherits[d].parent_identifier=0;
		p->inherits[d].prog=sp[-1].u.program;
		sp--;
		break;
	      default:
		error("Failed to decode inheritance.\n");
	    }
	    
	    getdata(p->inherits[d].name);
	  }
	  
	  debug_malloc_touch(dat);

	  for(d=0;d<p->num_identifiers;d++)
	  {
	    getdata(p->identifiers[d].name);
	    decode_type(p->identifiers[d].type,data);
	    decode_number(p->identifiers[d].identifier_flags,data);
	    decode_number(p->identifiers[d].run_time_type,data);
	    decode_number(p->identifiers[d].func.offset,data);
	  }

	  debug_malloc_touch(dat);
	  
	  for(d=0;d<p->num_constants;d++)
	  {
	    decode_value2(data);
	    p->constants[d]=*--sp;
	  }
	  data->pickyness--;

	  debug_malloc_touch(dat);

	  for(d=0;d<NUM_LFUNS;d++)
	    decode_number(p->lfuns[d],data);

	  debug_malloc_touch(dat);
	  
	  {
	    struct program *new_program_save=new_program;
	    new_program=p;
	    fsort((void *)p->identifier_index,
		  p->num_identifier_index,
		  sizeof(unsigned short),(fsortfun)program_function_index_compare);
	    new_program=new_program_save;
	  }
	  p->flags |= PROGRAM_FINISHED;
	  ref_push_program(p);

#ifdef _REENTRANT
	  UNSET_ONERROR(err);
	  if(!--threads_disabled)
	    co_broadcast(&threads_disabled_change);
#endif
	  return;
	}

	default:
	  error("Cannot decode program encoding type %d\n",num);
      }
      break;

  default:
    error("Failed to restore string. (Illegal type)\n");
  }

  mapping_insert(data->decoded, & tmp, sp-1);
}


static void free_decode_data(struct decode_data *data)
{
  free_mapping(data->decoded);
}

static INT32 my_decode(struct pike_string *tmp,
		       struct object *codec)
{
  ONERROR err;
  struct decode_data d, *data;
  data=&d;
  data->counter.type=T_INT;
  data->counter.u.integer=COUNTER_START;
  data->data=(unsigned char *)tmp->str;
  data->len=tmp->len;
  data->ptr=0;
  data->codec=codec;
  data->pickyness=0;

  if(data->len < 5) return 0;
  if(GETC() != 182 ||
     GETC() != 'k' ||
     GETC() != 'e' ||
     GETC() != '0')
    return 0;

  data->decoded=allocate_mapping(128);

  SET_ONERROR(err, free_decode_data, data);
  decode_value2(data);
  UNSET_ONERROR(err);
  free_mapping(data->decoded);
  return 1;
}

/* Compatibilidy decoder */

static unsigned char extract_char(char **v, INT32 *l)
{
  if(!*l) error("Format error, not enough place for char.\n");
  else (*l)--;
  (*v)++;
  return ((unsigned char *)(*v))[-1];
}

static INT32 extract_int(char **v, INT32 *l)
{
  INT32 j,i;

  j=extract_char(v,l);
  if(j & 0x80) return (j & 0x7f);

  if((j & ~8) > 4)
    error("Format Error: Error in format string, invalid integer.\n");
  i=0;
  while(j & 7) { i=(i<<8) | extract_char(v,l); j--; }
  if(j & 8) return -i;
  return i;
}

static void rec_restore_value(char **v, INT32 *l)
{
  INT32 t,i;

  i=extract_int(v,l);
  t=extract_int(v,l);
  switch(i)
  {
  case T_INT: push_int(t); return;
    
  case T_FLOAT:
    if(sizeof(INT32) < sizeof(float))  /* FIXME FIXME FIXME FIXME */
      error("Float architecture not supported.\n"); 
    push_int(t); /* WARNING! */
    sp[-1].type = T_FLOAT;
    return;
    
  case T_STRING:
    if(t<0) error("Format error, length of string is negative.\n");
    if(*l < t) error("Format error, string to short\n");
    push_string(make_shared_binary_string(*v, t));
    (*l)-= t; (*v)+= t;
    return;
    
  case T_ARRAY:
    if(t<0) error("Format error, length of array is negative.\n");
    check_stack(t);
    for(i=0;i<t;i++) rec_restore_value(v,l);
    f_aggregate(t);
    return;

  case T_MULTISET:
    if(t<0) error("Format error, length of multiset is negative.\n");
    check_stack(t);
    for(i=0;i<t;i++) rec_restore_value(v,l);
    f_aggregate_multiset(t);
    return;
    
  case T_MAPPING:
    if(t<0) error("Format error, length of mapping is negative.\n");
    check_stack(t*2);
    for(i=0;i<t;i++)
    {
      rec_restore_value(v,l);
      rec_restore_value(v,l);
    }
    f_aggregate_mapping(t*2);
    return;

  case T_OBJECT:
    if(t<0) error("Format error, length of object is negative.\n");
    if(*l < t) error("Format error, string to short\n");
    push_string(make_shared_binary_string(*v, t));
    (*l) -= t; (*v) += t;
    APPLY_MASTER("objectof", 1);
    return;
    
  case T_FUNCTION:
    if(t<0) error("Format error, length of function is negative.\n");
    if(*l < t) error("Format error, string to short\n");
    push_string(make_shared_binary_string(*v, t));
    (*l) -= t; (*v) += t;
    APPLY_MASTER("functionof", 1);
    return;
     
  case T_PROGRAM:
    if(t<0) error("Format error, length of program is negative.\n");
    if(*l < t) error("Format error, string to short\n");
    push_string(make_shared_binary_string(*v, t));
    (*l) -= t; (*v) += t;
    APPLY_MASTER("programof", 1);
    return;
    
  default:
    error("Format error. Unknown type\n");
  }
}

void f_decode_value(INT32 args)
{
  struct pike_string *s;
  struct object *codec;

  check_all_args("decode_value",args, BIT_STRING, BIT_VOID | BIT_OBJECT | BIT_INT, 0);

  s = sp[-args].u.string;
  if(args<2)
  {
    codec=get_master();
  }
  else if(sp[1-args].type == T_OBJECT)
  {
    codec=sp[1-args].u.object;
  }
  else
  {
    codec=0;
  }

  if(!my_decode(s, codec))
  {
    char *v=s->str;
    INT32 l=s->len;
    rec_restore_value(&v, &l);
  }
  assign_svalue(sp-args-1, sp-1);
  pop_n_elems(args);
}


