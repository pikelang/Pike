/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "stralloc.h"
#include "global.h"
#include "types.h"
#include "macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "multiset.h"
#include "builtin_functions.h"
#include "dynamic_buffer.h"
#include "error.h"

#ifdef _AIX
#include <net/nh.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include <math.h>

struct encode_data
{
  struct svalue counter;
  struct mapping *encoded;
  dynamic_buffer buf;
};

#define addstr(s, l) low_my_binary_strcat((s), (l), &(data->buf))
#define addchar(t)   low_my_putchar((t),&(data->buf))

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

static void encode_value2(struct svalue *val, struct encode_data *data)
{
  INT32 i;
  struct svalue *tmp;
  if(tmp=low_mapping_lookup(data->encoded, val))
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
      INT32 x,y;
      y=(int)ceil(log(val->u.float_number)/log(2.0))-30;
      x=(int)((val->u.float_number)*pow(2.0,(float)-y));
      while(x && y && !(x&1))
      {
	x>>=1;
	y++;
      }
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
    val->u.mapping->refs++;
    push_mapping(val->u.mapping);
    f_indices(1);
    
    val->u.mapping->refs++;
    push_mapping(val->u.mapping);
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
  case T_FUNCTION:
  case T_PROGRAM:
    check_stack(1);
    push_svalue(val);
    APPLY_MASTER("nameof", 1);
    if(sp[-1].type == val[-1].type)
      error("Error in master()->nameof(), same type returned.\n");
    code_entry(val->type, 0,data);
    encode_value2(sp-1, data);
    pop_stack();
    break;
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

  initialize_buf(&data->buf);
  data->encoded=allocate_mapping(128);
  data->counter.type=T_INT;
  data->counter.u.integer=COUNTER_START;

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
};

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

static void decode_value2(struct decode_data *data)
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
    if(tmp2=low_mapping_lookup(data->decoded, &tmp))
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
    push_string(make_shared_binary_string(data->data + data->ptr, num));
    data->ptr+=num;
    break;

  case T_FLOAT:
  {
    INT32 num2=num;

    tmp=data->counter;
    data->counter.u.integer++;

    DECODE();
    push_float(num2 * pow(2.0, (double) num));
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
    a->refs++;
    push_array(a);
    return;
  }

  case T_MAPPING:
  {
    struct mapping *m=allocate_mapping(num);
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
    m->refs++;
    push_mapping(m);
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
    m->refs++;
    push_multiset(m);
    return;
  }

  case T_OBJECT:
  case T_FUNCTION:
  case T_PROGRAM:
    tmp=data->counter;
    data->counter.u.integer++;
    decode_value2(data);
    switch(what & T_MASK)
    {
    case T_OBJECT: APPLY_MASTER("objectof", 1); break;
    case T_FUNCTION: APPLY_MASTER("functionof", 1); break;
    case T_PROGRAM: APPLY_MASTER("programof", 1); break;
    }
    if(sp[-1].type != (what & T_MASK))
      error("Failed to restore string. (master() failed)\n");
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

static INT32 my_decode(struct pike_string *tmp)
{
  ONERROR err;
  struct decode_data d, *data;
  data=&d;
  data->counter.type=T_INT;
  data->counter.u.integer=COUNTER_START;
  data->data=tmp->str;
  data->len=tmp->len;
  data->ptr=0;

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
    if(*l < t) error("Format error, string to short\n");
    push_string(make_shared_binary_string(*v, t));
    (*l)-= t; (*v)+= t;
    return;
    
  case T_ARRAY:
    check_stack(t);
    for(i=0;i<t;i++) rec_restore_value(v,l);
    f_aggregate(t);
    return;

  case T_MULTISET:
    check_stack(t);
    for(i=0;i<t;i++) rec_restore_value(v,l);
    f_aggregate_multiset(t);
    return;
    
  case T_MAPPING:
    check_stack(t*2);
    for(i=0;i<t;i++)
    {
      rec_restore_value(v,l);
      rec_restore_value(v,l);
    }
    f_aggregate_mapping(t*2);
    return;

  case T_OBJECT:
    if(*l < t) error("Format error, string to short\n");
    push_string(make_shared_binary_string(*v, t));
    (*l) -= t; (*v) += t;
    APPLY_MASTER("objectof", 1);
    return;
    
  case T_FUNCTION:
    if(*l < t) error("Format error, string to short\n");
    push_string(make_shared_binary_string(*v, t));
    (*l) -= t; (*v) += t;
    APPLY_MASTER("functionof", 1);
    return;
     
  case T_PROGRAM:
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

  if(args != 1 || (sp[-1].type != T_STRING))
    error("Illegal argument to restore_value(STRING)\n");

  s = sp[-1].u.string;
  if(!my_decode(s))
  {
    char *v=s->str;
    INT32 l=s->len;
    rec_restore_value(&v, &l);
  }
  assign_svalue(sp-args-1, sp-1);
  pop_n_elems(args);
}

