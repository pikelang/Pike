#include "global.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "mapping.h"
#include "list.h"
#include "object.h"
#include "program.h"
#include "add_efun.h"
#include "error.h"
#include "dynamic_buffer.h"

/*
 * This routine frees a short svalue given a pointer to it and
 * its type.
 */
void free_short_svalue(union anything *s,TYPE_T type)
{
  check_type(type);
  check_refs2(s,type);

  if(type <= MAX_REF_TYPE && s->refs)
  {
    if(--*(s->refs) <= 0)
    {
      switch(type)
      {
      case T_ARRAY:
	really_free_array(s->array);
	break;

      case T_MAPPING:
	really_free_mapping(s->mapping);
	break;

      case T_LIST:
	really_free_list(s->list);
	break;

      case T_OBJECT:
	really_free_object(s->object);
	break;

      case T_PROGRAM:
	really_free_program(s->program);
	break;

      case T_STRING:
	really_free_string(s->string);
	break;

#ifdef DEBUG
      default:
	fatal("Bad type in free_short_svalue.\n");
#endif
      }
    }
  }
}

/* Free a normal svalue */
void free_svalue(struct svalue *s)
{
  check_type(s->type);
  check_refs(s);

  if(s->type <= MAX_REF_TYPE)
  {
    if(--*(s->u.refs) <= 0)
    {
      switch(s->type)
      {
      case T_ARRAY:
	really_free_array(s->u.array);
	break;

      case T_MAPPING:
	really_free_mapping(s->u.mapping);
	break;

      case T_LIST:
	really_free_list(s->u.list);
	break;

      case T_FUNCTION:
	if(s->subtype == -1)
	{
	  really_free_callable(s->u.efun);
	  break;
	}
	/* fall through */

      case T_OBJECT:
	really_free_object(s->u.object);
	break;

      case T_PROGRAM:
	really_free_program(s->u.program);
	break;

      case T_STRING:
	really_free_string(s->u.string);
	break;

#ifdef DEBUG
      default:
	fatal("Bad type in free_svalue.\n");
#endif
      }
    }
  }
#ifdef DEBUG
  s->type = 99;
#endif
}

/* Free a bunch of normal svalues.
 * We put this routine here so the compiler can optimize the call
 * inside the loop if it wants to
 */
void free_svalues(struct svalue *s,INT32 num)
{
  while(--num >= 0) free_svalue(s++);
}

void free_short_svalues(union anything *s,INT32 num,TYPE_T type)
{
#ifdef DEBUG
  int e;
  for(e=0;e<num;e++)
    check_refs2(s+e,type);
#endif

  switch(type)
  {
    case T_ARRAY:
      for(;--num >= 0;s++)
        if(s->refs && --*(s->refs) <= 0)
          really_free_array(s->array);
     break;

    case T_MAPPING:
      for(;--num >= 0;s++)
        if(s->refs && --*(s->refs) <= 0)
	  really_free_mapping(s->mapping);
      break;

    case T_LIST:
      for(;--num >= 0;s++)
        if(s->refs && --*(s->refs) <= 0)
          really_free_list(s->list);
      break;

    case T_OBJECT:
      for(;--num >= 0;s++)
        if(s->refs && --*(s->refs) <= 0)
          really_free_object(s->object);
      break;

    case T_PROGRAM:
      for(;--num >= 0;s++)
        if(s->refs && --*(s->refs) <= 0)
          really_free_program(s->program);
      break;

    case T_STRING:
      for(;--num >= 0;s++)
        if(s->refs && --*(s->refs) <= 0)
          really_free_string(s->string);
      break;

    case T_INT:
    case T_FLOAT:
      break;

#ifdef DEBUG
    default:
      fatal("Bad type in free_short_svalues.\n");
#endif
  }
}

void assign_svalue_no_free(struct svalue *to,
			   struct svalue *from)
{
  struct svalue tmp;
  check_type(from->type);
  check_refs(from);

  *to=tmp=*from;
  if(tmp.type <= MAX_REF_TYPE) tmp.u.refs[0]++;
}

void assign_svalues_no_free(struct svalue *to,
			    struct svalue *from,
			    INT32 num)
{
  while(--num >= 0) assign_svalue_no_free(to++,from++);
}


void assign_svalue(struct svalue *to, struct svalue *from)
{
  free_svalue(to);
  assign_svalue_no_free(to,from);
}

void assign_svalues(struct svalue *to, struct svalue *from, INT32 num)
{
  free_svalues(to,num);
  assign_svalues_no_free(to,from,num);
}

void assign_to_short_svalue(union anything *u,
			    TYPE_T type,
			    struct svalue *s)
{
  union anything tmp;
  check_type(s->type);
  check_refs(s);

  if(s->type == type)
  {
    if(type<=MAX_REF_TYPE)
    {
      free_short_svalue(u,type);
      *u = tmp = s->u;
      tmp.refs[0]++;
    }else{
      *u = s->u;
    }
  }else if(IS_ZERO(s)){
    free_short_svalue(u,type);
    u->integer=0;
  }else{
    error("Wrong type in assignment.\n");
  }
}

void assign_to_short_svalue_no_free(union anything *u,
					   TYPE_T type,
					   struct svalue *s)
{
  union anything tmp;
  check_type(s->type);
  check_refs(s);

  if(s->type == type)
  {
    *u = tmp = s->u;
    if(type <= MAX_REF_TYPE) tmp.refs[0]++;
  }else if(IS_ZERO(s)){
    u->integer=0;
  }else{
    error("Wrong type in assignment.\n");
  }
}

void assign_to_short_svalues_no_free(union anything *u,
				     TYPE_T type,
				     struct svalue *s,
				     INT32 num)
{
  while(--num >= 0) assign_to_short_svalue_no_free(u++,type,s++);
}


void assign_from_short_svalue_no_free(struct svalue *s,
					     union anything *u,
					     TYPE_T type)
{
  check_type(type);
  check_refs2(u,type);

  if(type <= MAX_REF_TYPE)
  {
    if((s->u=*u).refs)
    {
      u->refs[0]++;
      s->type=type;
    }else{
      s->type=T_INT;
      s->subtype=NUMBER_NUMBER;
    }
  }else{
    s->type=type;
    s->u=*u;
  }
}

void assign_from_short_svalues_no_free(struct svalue *s,
				       union anything *u,
				       TYPE_T type,
				       INT32 num)
{
  check_type(type);
    
  if(type <= MAX_REF_TYPE)
  {
    while(--num >= 0)
    {
      check_refs2(u,type);

      s->u=*u;
      if(u->refs)
      {
	s->type=type;
	u->refs[0]++;
      }else{
	s->type=T_INT;
	s->subtype=NUMBER_NUMBER;
      }
      s++;
      u++;
    }
  }
  else
  {
    while(--num >= 0)
    {
      s->type=type;
      s->u=*u;
      s++;
      u++;
    }
  }
}

void assign_short_svalue_no_free(union anything *to,
				 union anything *from,
				 TYPE_T type)
{
  union anything tmp;
  check_type(type);
  check_refs2(from,type);

  *to = tmp = *from;
  if(tmp.refs && type <= MAX_REF_TYPE) tmp.refs[0]++;
}

void assign_short_svalue(union anything *to,
			 union anything *from,
			 TYPE_T type)
{
  check_type(type);
  check_refs2(from,type);

  if(type <= MAX_REF_TYPE)
  {
    union anything tmp;
    free_short_svalue(to,type);
    *to = tmp = *from;
    if(tmp.refs) tmp.refs[0]++;
  }else{
    *to = *from;
  }
}

void assign_short_svalues_no_free(union anything *to,
				 union anything *from,
				 TYPE_T type,
				 INT32 num)
{
  union anything tmp;
  check_type(type);

  if(type <= MAX_REF_TYPE)
  {
    while(--num >= 0)
    {
      check_refs2(from,type);

      *(to++) = tmp = *(from++);
      if(tmp.refs) tmp.refs[0]++;
    }
  }else{
    MEMCPY((char *)to, (char *)from, num*sizeof(union anything *));
  }
}

int is_eq(struct svalue *a, struct svalue *b)
{
  check_type(a->type);
  check_type(b->type);
  check_refs(a);
  check_refs(b);

  check_destructed(a);
  check_destructed(b);

  if (a->type != b->type) return 0;
  switch(a->type)
  {
  case T_INT:
  case T_LIST:
  case T_OBJECT:
  case T_PROGRAM:
  case T_ARRAY:
  case T_MAPPING:
    return a->u.integer == b->u.integer;

  case T_STRING:
    return is_same_string(a->u.string,b->u.string);

  case T_FUNCTION:
    return (a->subtype == b->subtype && a->u.object == b->u.object);
      
  case T_FLOAT:
    return a->u.float_number == b->u.float_number;

  default:
    fatal("Unknown type %x\n",a->type);
    return 0; /* make gcc happy */
  }
}


int low_is_equal(struct svalue *a,
		 struct svalue *b,
		 struct processing *p)
{
  check_type(a->type);
  check_type(b->type);
  check_refs(a);
  check_refs(b);

  if(is_eq(a,b)) return 1;

  if(a->type != b->type) return 0;

  switch(a->type)
  {
    case T_INT:
    case T_STRING:
    case T_FLOAT:
    case T_FUNCTION:
      return 0;

    case T_OBJECT:
      return object_equal_p(a->u.object, b->u.object, p);

    case T_ARRAY:
      check_array_for_destruct(a->u.array);
      check_array_for_destruct(b->u.array);
      return array_equal_p(a->u.array, b->u.array, p);

    case T_MAPPING:
      return mapping_equal_p(a->u.mapping, b->u.mapping, p);

    case T_LIST:
      return list_equal_p(a->u.list, b->u.list, p);
      
    default:
      fatal("Unknown type in is_equal.\n");
  }
  return 1; /* survived */
}

int low_short_is_equal(const union anything *a,
		       const union anything *b,
		       TYPE_T type,
		       struct processing *p)
{
  struct svalue sa,sb;

  check_type(type);
  check_refs2(a,type);
  check_refs2(b,type);

  sa.u=*a;
  if(!sa.u.refs)
  {
    if( (sa.type=type) != T_FLOAT)  sa.type=T_INT;
  }else{
    sa.type=type;
  }

  sb.u=*b;
  if(!sb.u.refs)
  {
    if( (sb.type=type) != T_FLOAT)  sb.type=T_INT;
  }else{
    sb.type=type;
  }
  return low_is_equal(&sa,&sb,p);
}

int is_equal(struct svalue *a,struct svalue *b)
{
  return low_is_equal(a,b,0);
}


int is_gt(const struct svalue *a,const struct svalue *b)
{
  check_type(a->type);
  check_type(b->type);
  check_refs(a);
  check_refs(b);

  if (a->type != b->type)
  {
    error("Cannot compare different types.\n");
  }
  switch(a->type)
  {
  default:
    error("Bad argument 1 tp '<'.\n");

  case T_INT:
    return a->u.integer > b->u.integer;

  case T_STRING:
    return my_strcmp(a->u.string, b->u.string) > 0;

  case T_FLOAT:
    return a->u.float_number > b->u.float_number;
  }
}

int is_lt(const struct svalue *a,const struct svalue *b)
{
  check_type(a->type);
  check_type(b->type);
  check_refs(a);
  check_refs(b);

  if (a->type != b->type)
  {
    error("Cannot compare different types.\n");
  }
  switch(a->type)
  {
  default:
    error("Bad arg 1 to '<='.\n");

  case T_INT:
    return a->u.integer < b->u.integer;

  case T_STRING:
    return my_strcmp(a->u.string, b->u.string) < 0;

  case T_FLOAT:
    return a->u.float_number < b->u.float_number;

  }
}

void describe_svalue(struct svalue *s,int indent,struct processing *p)
{
  char buf[40];

  check_type(s->type);
  check_refs(s);

  indent+=2;
  switch(s->type)
  {
    case T_LVALUE:
      my_strcat("lvalue");
      break;

    case T_INT:
      sprintf(buf,"%ld",s->u.integer);
      my_strcat(buf);
      break;

    case T_STRING:
      {
        char *a;
        my_putchar('"');
        for(a=s->u.string->str;*a;a++)
        {
          switch(*a)
          {
            case '"':
            case '\\':
              my_putchar('\\');
            default:
              my_putchar(*a);
          } 
        }
        my_putchar('"');
      }
      break;


    case T_FUNCTION:
      if(s->subtype == -1)
      {
	my_binary_strcat(s->u.efun->name->str,s->u.efun->name->len);
      }else{
	if(s->u.object->prog)
	{
	  struct lpc_string *name;
	  name=ID_FROM_INT(s->u.object->prog,s->subtype)->name;
	  my_binary_strcat(name->str,name->len);
	}else{
	  my_strcat("0");
	}
      }
      break;

    case T_OBJECT:
      my_strcat("object");
      break;

    case T_PROGRAM:
      my_strcat("program");
      break;

    case T_FLOAT:
      sprintf(buf,"%f",(double)s->u.float_number);
      my_strcat(buf);
      break;

    case T_ARRAY:
      describe_array(s->u.array, p, indent);
      break;

    case T_LIST:
      describe_list(s->u.list, p, indent);
      break;

    case T_MAPPING:
      describe_mapping(s->u.mapping, p, indent);
      break;

    default:
      sprintf(buf,"<Unknown %d>",s->type);
      my_strcat(buf);
  }
}

void clear_svalues(struct svalue *s, INT32 num)
{
  struct svalue dum;
  dum.type=T_INT;
  dum.subtype=NUMBER_NUMBER;
  dum.u.integer=0;
  while(--num >= 0) *(s++)=dum;
}

void copy_svalues_recursively_no_free(struct svalue *to,
				      struct svalue *from,
				      INT32 num,
				      struct processing *p)
{
  while(--num >= 0)
  {
    check_type(from->type);
    check_refs(from);

    switch(from->type)
    {
    default:
      if(from->type <= MAX_REF_TYPE)
	(*to=*from).u.refs[0]++;
      else
	*to=*from;
      break;

    case T_ARRAY:
      to->u.array=copy_array_recursively(from->u.array,p);
      to->type=T_ARRAY;
      break;

    case T_MAPPING:
      to->u.mapping=copy_mapping_recursively(from->u.mapping,p);
      to->type=T_MAPPING;
      break;

    case T_LIST:
      to->u.list=copy_list_recursively(from->u.list,p);
      to->type=T_LIST;
      break;
    }
    to++;
    from++;
  }
}


void copy_short_svalues_recursively_no_free(union anything *to,
					    union anything *from,
					    TYPE_T type,
					    INT32 num,
					    struct processing *p)
{
  check_type(type);
  check_refs2(from,type);

  switch(type)
  {
  default:
    assign_short_svalues_no_free(to,from,type,num);
    break;

  case T_ARRAY:
    while(--num >= 0)
    {
      to->array=from->array?copy_array_recursively(from->array,p):0;
      from++;
      to++;
    }
    break;

  case T_MAPPING:
    while(--num >= 0)
    {
      to->mapping=from->mapping?copy_mapping_recursively(from->mapping,p):0;
      from++;
      to++;
    }
    break;

  case T_LIST:
    while(--num >= 0)
    {
      to->list=from->list?copy_list_recursively(from->list,p):0;
      from++;
      to++;
    }
    break;
  }
}

#ifdef DEBUG
void check_short_svalue(union anything *u,TYPE_T type)
{
  check_type(type);
  check_refs2(u,type);
  if(!u->refs) return;

  if(type <= MAX_REF_TYPE)
  {
    checked((void *) u->refs,1);
  }

  switch(type)
  {
  case T_STRING:
    if(!debug_findstring(u->string))
      fatal("Shared string not shared!\n");
    break;
  }
}

void check_svalue(struct svalue *s)
{
  check_refs(s);
  check_short_svalue(& s->u, s->type);
}

#endif
