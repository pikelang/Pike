#include "stralloc.h"
#include "global.h"
#include "types.h"
#include "macros.h"
#include "object.h"
#include "add_efun.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "list.h"
#include "builtin_efuns.h"
#include "dynamic_buffer.h"

#include "spider.h"

#define strcat(buff, s, l) low_my_binary_strcat((s), (l), (buff))
#define addchar(buff, t)   low_my_putchar((t),(buff))


/*
          byte 0          byte 1          byte 2          byte 3          byte 4     
int x     8 7 6 5 4 3 2 1 8 7 6 5 4 3 2 1 8 7 6 5 4 3 2 1 8 7 6 5 4 3 2 1 8 7 6 5 4 3 2 1
-----------------------------------------------------------------------------------------
< 1<<7    1 x x x x x x x
< 1<<8    0 0 0 0 0 0 0 1 x x x x x x x x
< 1<<16   0 0 0 0 0 0 1 0 x x x x x x x x x x x x x x x x
< 1<<24   0 0 0 0 0 0 1 1 x x x x x x x x x x x x x x x x x x x x x x x x
< 1<<32   0 0 0 0 0 1 0 0 x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x


< 0       0 0 0 0 1 as above
*/

INLINE static void add_int_to_buffer(dynamic_buffer *buff, INT32 t)
{
  int negbit = 0;

  if((unsigned INT32)t < 0x80)
  {
    addchar(buff, (unsigned char)((((unsigned INT32)t)|0x80)&0xff));
    return;
  }

  if(t<0)
  {
    negbit = 8;
    t = -t;
  }
  
  if((unsigned INT32)t < 0x100) {
    addchar(buff, 1|negbit);
    addchar(buff, (t&255));
  } else if((unsigned INT32)t < 0x10000) {
    addchar(buff, 2|negbit);
    addchar(buff, (t>>8)&255);
    addchar(buff, t&255);
  } else if((unsigned INT32)t < 0x1000000) {
    addchar(buff, 3|negbit);
    addchar(buff, (t>>16)&255);
    addchar(buff, (t>>8)&255);
    addchar(buff, t&255);
  } else {
    addchar(buff, 4|negbit);
    t = htonl(t);
    strcat(buff, (char *)(&t), 4);
  }
}

static void rec_save_value(struct svalue *val, dynamic_buffer *buff,
			   struct processing *p)
{
  struct processing doing;
  INT32 i;
  
  if((1<<val->type)&BIT_COMPLEX)
  {
    doing.next=p;
    doing.pointer_a=(void *)val->u.refs;

    for(;p;p=p->next)
      if(p->pointer_a == (void *)val->u.refs)
	error("Cannot save cyclic structures (yet)\n");
    p = &doing;
  }


  add_int_to_buffer(buff, val->type);

  switch(val->type)
  {
   case T_ARRAY:
     add_int_to_buffer(buff, val->u.array->size);
    for(i=0; i<val->u.array->size; i++)
      rec_save_value(ITEM(val->u.array)+i, buff, p);
    break;

   case T_MAPPING:
    val->u.mapping->refs+=2;
    push_mapping(val->u.mapping);
    f_indices(1);
    push_mapping(val->u.mapping);
    f_values(1);
    
    add_int_to_buffer(buff, sp[-2].u.array->size);
    for(i=0; i<sp[-2].u.array->size; i++)
    {
      rec_save_value(ITEM(sp[-2].u.array)+i, buff, p); /* indices */
      rec_save_value(ITEM(sp[-1].u.array)+i, buff, p); /* values */
    }
    pop_n_elems(2);
    break;

   case T_LIST:
    add_int_to_buffer(buff, val->u.list->ind->size);
    for(i=0; i<val->u.list->ind->size; i++)
      rec_save_value(ITEM(val->u.list->ind)+i, buff, p);
    break;

   case T_OBJECT:
    val->u.object->refs++;
    push_object(val->u.object);
    APPLY_MASTER("nameof", 1);
    if(sp[-1].type == T_STRING)
    {
      add_int_to_buffer(buff, sp[-1].u.string->len);
      strcat(buff, sp[-1].u.string->str, sp[-1].u.string->len);
    } else {
      add_int_to_buffer(buff, 0);
    }
    pop_stack();
    break;

   case T_FUNCTION:
    val->u.object->refs++;
    sp->u.object = val->u.object;
    sp->type = val->type;
    sp->subtype = val->subtype;
    sp++;
    APPLY_MASTER("nameof", 1);
    if(sp[-1].type == T_STRING)
    {
      add_int_to_buffer(buff, sp[-1].u.string->len);
      strcat(buff, sp[-1].u.string->str, sp[-1].u.string->len);
    } else {
      add_int_to_buffer(buff, 0);
    }
    pop_stack();
    break;

   case T_PROGRAM:
    val->u.program->refs++;
    push_program(val->u.program);
    APPLY_MASTER("nameof", 1);
    if(sp[-1].type == T_STRING)
    {
      add_int_to_buffer(buff, sp[-1].u.string->len);
      strcat(buff, sp[-1].u.string->str, sp[-1].u.string->len);
    } else {
      add_int_to_buffer(buff, 0);
    }
    pop_stack();
    break;

   case T_STRING:
    add_int_to_buffer(buff, val->u.string->len);
    strcat(buff, val->u.string->str, val->u.string->len);
    break;

   case T_INT:
    add_int_to_buffer(buff, val->u.integer);    
    break;
    
   case T_FLOAT:
    /* How to encode a float _correctly_? */
    if(sizeof(INT32) < sizeof(float))  /* FIXME FIXME FIXME FIXME */
      error("Float architecture not supported.\n"); 
    add_int_to_buffer(buff, val->u.integer);
    break;
  }
}

void f_encode_value(INT32 args)
{
  dynamic_buffer buff;

  buff.s.str = NULL;
  buff.s.len = 0;
  buff.bufsize = 0;

  low_init_buf(&buff);

  rec_save_value(sp-args, &buff, NULL);

  pop_n_elems(args);
  push_string(low_free_buf(&buff));
}

static unsigned char extract_char(char **v, unsigned INT32 *l)
{
  if(!*l) error("Format error, not enough place for char.\n");
  else (*l)--;
  (*v)++;
  return ((unsigned char *)(*v))[-1];
}


static INT32 extract_int(char **v, unsigned INT32 *l)
{
  unsigned INT32 j;
  INT32 t=0;
  char *c = (char *)(&t);

  if(!*l)
    error("Format error: not enough place for short integer.\n");

  if((**v) & 0x80) /* (very) Short int. */
    return extract_char(v,l) & 0x7f;

  switch(extract_char(v,l))
  {
  case 1: /* char */
    return extract_char(v,l);
    
  case 2: /* word */
    return (extract_char(v,l)<<8) | (extract_char(v,l));
    
  case 3: /* 24 bit thingie */
    return (extract_char(v,l)<<16) | (extract_char(v,l)<<8) | extract_char(v,l);

  case 4: /* 32 bit number */
    return (extract_char(v,l)<<24) |
           (extract_char(v,l)<<16) | (extract_char(v,l)<<8) |
            extract_char(v,l);
    
  case 9:  /* -char */
    return -extract_char(v,l);
  case 10: /* -word */
    return -((extract_char(v,l)<<8) | (extract_char(v,l)));
  case 11: /* -24 bit */
    return -((extract_char(v,l)<<16) | (extract_char(v,l)<<8) | extract_char(v,l));
  case 12: /* -32 bit thingie */
    return -((extract_char(v,l)<<24) |
	     (extract_char(v,l)<<16) | (extract_char(v,l)<<8) |
	     extract_char(v,l));
    
  }
  error("Format Error: Error in format string, invalid integer.\n");
  return t;
}

static void rec_restore_value(char **v, INT32 *l)
{
  INT32 t,i;

  switch(extract_int(v,l))
  {
   case T_INT:
    push_int(extract_int(v,l));
    return;
    
   case T_FLOAT:
    /* How to encode a float _correctly_? */
    if(sizeof(INT32) < sizeof(float))  /* FIXME FIXME FIXME FIXME */
      error("Float architecture not supported.\n"); 
    push_int(extract_int(v,l)); /* WARNING! */
    sp[-1].type = T_FLOAT;
    return;

   case T_STRING:
    t = extract_int(v,l);
    if(*l < t) error("Format error, string to short\n");
    push_string(make_shared_binary_string(*v, t));
    (*l)-= t; (*v)+= t;
    return;
    
   case T_ARRAY:
    t = extract_int(v,l);
    for(i=0;i<t;i++)
      rec_restore_value(v,l);
    f_aggregate(t);
    return;

   case T_LIST:
    t = extract_int(v,l);
    for(i=0;i<t;i++)
      rec_restore_value(v,l);
    f_aggregate_list(t);
    return;
    
   case T_MAPPING:
    t = extract_int(v,l);
    for(i=0;i<t;i++)
    {
      rec_restore_value(v,l);
      rec_restore_value(v,l);
    }
    f_aggregate_mapping(t*2);
    return;

   case T_OBJECT:
    t = extract_int(v,l);
    push_string(make_shared_binary_string(*v, t));
    (*l) -= t; (*v) += t;
    APPLY_MASTER("objectof", 1);
    return;
    
   case T_FUNCTION:
    t = extract_int(v,l);
    push_string(make_shared_binary_string(*v, t));
    (*l) -= t; (*v) += t;
    APPLY_MASTER("functionof", 1);
    return;
     
   case T_PROGRAM:
    t = extract_int(v,l);
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
  struct lpc_string *s;
  char *v;
  INT32 l;

  if(args != 1 || (sp[-1].type != T_STRING))
    error("Illegal argument to restore_value(STRING)\n");
  
  s = sp[-1].u.string;
  s->refs++; v=s->str; l=s->len;

  pop_n_elems(args);
  rec_restore_value(&v, &l);
  free_string(s);
}

