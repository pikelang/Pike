/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: pike_types.c,v 1.26 1997/09/29 00:57:55 hubbe Exp $");
#include <ctype.h>
#include "svalue.h"
#include "pike_types.h"
#include "stralloc.h"
#include "stuff.h"
#include "array.h"
#include "program.h"
#include "constants.h"
#include "object.h"
#include "multiset.h"
#include "mapping.h"
#include "pike_macros.h"
#include "error.h"
#include "las.h"
#include "language.h"
#include "pike_memory.h"

int max_correct_args;

static void internal_parse_type(char **s);
static int type_length(char *t);

#define TWOT(X,Y) (((X) << 8)+(Y))
#define EXTRACT_TWOT(X,Y) TWOT(EXTRACT_UCHAR(X), EXTRACT_UCHAR(Y))

/*
 * basic types are represented by just their value in a string
 * basic type are string, int, float, object and program
 * arrays are coded like by the value T_ARRAY followed by the
 * data type, if the type is not known it is T_MIXED, ie:
 * T_ARRAY <data type>
 * mappings are followed by two arguments, the first is the type
 * for the indices, and the second is the type of the data, ie:
 * T_MAPPING <indice type> <data type>
 * multiset works similarly to arrays.
 * functions are _very_ special:
 * they are coded like this:
 * T_FUNCTION <arg type> <arg type> ... <arg type> T_MANY <arg type> <return type>
 * note that the type after T_MANY can be T_VOID
 * T_MIXED matches anything except T_VOID
 * T_UNKNOWN only matches T_MIXED and T_UNKNOWN
 */

struct pike_string *string_type_string;
struct pike_string *int_type_string;
struct pike_string *float_type_string;
struct pike_string *function_type_string;
struct pike_string *object_type_string;
struct pike_string *program_type_string;
struct pike_string *array_type_string;
struct pike_string *multiset_type_string;
struct pike_string *mapping_type_string;
struct pike_string *mixed_type_string;
struct pike_string *void_type_string;
struct pike_string *any_type_string;

#ifdef DEBUG
static void CHECK_TYPE(struct pike_string *s)
{
  if(debug_findstring(s) != s)
    fatal("Type string not shared.\n");

  if(type_length(s->str) != s->len)
    fatal("Length of type is wrong.\n");
}
#else
#define CHECK_TYPE(X)
#endif

void init_types(void)
{
  string_type_string=parse_type("string");
  int_type_string=parse_type("int");
  object_type_string=parse_type("object");
  program_type_string=parse_type("program");
  float_type_string=parse_type("float");
  mixed_type_string=parse_type("mixed");
  array_type_string=parse_type("array");
  multiset_type_string=parse_type("multiset");
  mapping_type_string=parse_type("mapping");
  function_type_string=parse_type("function");
  void_type_string=parse_type("void");
  any_type_string=parse_type("void|mixed");
}

static int type_length(char *t)
{
  char *q=t;

  switch(EXTRACT_UCHAR(t++))
  {
  default:
    fatal("error in type string.\n");
    /*NOTREACHED*/

  case T_FUNCTION:
    while(EXTRACT_UCHAR(t)!=T_MANY) t+=type_length(t); /* skip arguments */
    t++;

  case T_MAPPING:
  case T_OR:
  case T_AND:
    t+=type_length(t);

  case T_ARRAY:
  case T_MULTISET:
  case T_NOT:
    t+=type_length(t);

  case T_INT:
  case T_FLOAT:
  case T_STRING:
  case T_PROGRAM:
  case T_MIXED:
  case T_VOID:
  case T_UNKNOWN:
    break;

  case T_OBJECT:
    t+=sizeof(INT32);
    break;
  }
  return t-q;
}


#define STACK_SIZE 100000
static unsigned char type_stack[STACK_SIZE];
static unsigned char *type_stackp=type_stack;
static unsigned char *mark_stack[STACK_SIZE/4];
static unsigned char **mark_stackp=mark_stack;

void push_type(unsigned char tmp)
{
  *type_stackp=tmp;
  type_stackp++;
  if(type_stackp > type_stack + sizeof(type_stack))
    yyerror("Type stack overflow.");
}

void type_stack_mark(void)
{
  *mark_stackp=type_stackp;
  mark_stackp++;
  if(mark_stackp > mark_stack + NELEM(mark_stack))
    yyerror("Type mark stack overflow.");
}

INT32 pop_stack_mark(void)
{ 
  mark_stackp--;
  if(mark_stackp<mark_stack)
    fatal("Type mark stack underflow\n");

  return type_stackp - *mark_stackp;
}

void pop_type_stack(void)
{ 
  type_stackp--;
  if(type_stackp<type_stack)
    fatal("Type stack underflow\n");
}

void type_stack_pop_to_mark(void)
{
  type_stackp-=pop_stack_mark();
#ifdef DEBUG
  if(type_stackp<type_stack)
    fatal("Type stack underflow\n");
#endif
}

void reset_type_stack(void)
{
  type_stack_pop_to_mark();
  type_stack_mark();
}

void type_stack_reverse(void)
{
  INT32 a;
  a=pop_stack_mark();
  reverse((char *)(type_stackp-a),a,1);
}

void push_type_int(unsigned INT32 i)
{
  int e;
  for(e=sizeof(i)-1;e>=0;e--)
    push_type(((unsigned char *)&i)[e]);
}

void push_unfinished_type(char *s)
{
  int e;
  e=type_length(s);
  for(e--;e>=0;e--) push_type(s[e]);
}

void push_finished_type(struct pike_string *type)
{
  int e;
  CHECK_TYPE(type);
  for(e=type->len-1;e>=0;e--) push_type(type->str[e]);
}

struct pike_string *pop_unfinished_type(void)
{
  int len,e;
  struct pike_string *s;
  len=pop_stack_mark();
  s=begin_shared_string(len);
  type_stackp-=len;
  MEMCPY(s->str, type_stackp, len);
  reverse(s->str, len, 1);
  s=end_shared_string(s);
  CHECK_TYPE(s);
  return s;
}

struct pike_string *pop_type(void)
{
  struct pike_string *s;
  s=pop_unfinished_type();
  type_stack_mark();
  return s;
}

static void internal_parse_typeA(char **_s)
{
  char buf[80];
  unsigned int len;
  unsigned char **s = (unsigned char **)_s;
  
  while(ISSPACE(**s)) ++*s;

  len=0;
  for(len=0;isidchar(EXTRACT_UCHAR(s[0]+len));len++)
  {
    if(len>=sizeof(buf)) error("Buffer overflow in parse_type\n");
    buf[len] = s[0][len];
  }
  buf[len]=0;
  *s += len;
  
  if(!strcmp(buf,"int")) push_type(T_INT);
  else if(!strcmp(buf,"float")) push_type(T_FLOAT);
  else if(!strcmp(buf,"object"))
  {
    push_type_int(0);
    push_type(T_OBJECT);
  }
  else if(!strcmp(buf,"program")) push_type(T_PROGRAM);
  else if(!strcmp(buf,"string")) push_type(T_STRING);
  else if(!strcmp(buf,"void")) push_type(T_VOID);
  else if(!strcmp(buf,"mixed")) push_type(T_MIXED);
  else if(!strcmp(buf,"unknown")) push_type(T_UNKNOWN);
  else if(!strcmp(buf,"function"))
  {
    while(ISSPACE(**s)) ++*s;
    if(**s == '(')
    {
      ++*s;
      while(ISSPACE(**s)) ++*s;
      type_stack_mark();
      while(1)
      {
	if(**s == ':')
	{
	  push_type(T_MANY);
	  push_type(T_VOID);
	  break;
	}

	type_stack_mark();
	type_stack_mark();
	type_stack_mark();
	internal_parse_type(_s);
	type_stack_reverse();
	if(**s==',')
	{
	  ++*s;
	  while(ISSPACE(**s)) ++*s;
	}
	else if(s[0][0]=='.' && s[0][1]=='.' && s[0][2]=='.')
	{
	  type_stack_reverse();
	  push_type(T_MANY);
	  type_stack_reverse();
	  *s+=3;
	  while(ISSPACE(**s)) ++*s;
	  if(**s != ':') error("Missing ':' after ... in function type.\n");
	  break;
	}
	pop_stack_mark();
	pop_stack_mark();
      }
      ++*s;
      type_stack_mark();
      internal_parse_type(_s);  /* return type */
      type_stack_reverse();
      if(**s != ')') error("Missing ')' in function type.\n");
      ++*s;
      type_stack_reverse(); 
   }else{
      push_type(T_MIXED);
      push_type(T_MIXED);
      push_type(T_MANY);
    }
    push_type(T_FUNCTION);
  }
  else if(!strcmp(buf,"mapping"))
  {
    while(ISSPACE(**s)) ++*s;
    if(**s == '(')
    {
      type_stack_mark();
      ++*s;
      type_stack_mark();
      internal_parse_type(_s);
      type_stack_reverse();
      if(**s != ':') error("Expecting ':'.\n");
      ++*s;
      type_stack_mark();
      internal_parse_type(_s);
      type_stack_reverse();
      if(**s != ')') error("Expecting ')'.\n");
      ++*s;
      type_stack_reverse();
    }else{
      push_type(T_MIXED);
      push_type(T_MIXED);
    }
    push_type(T_MAPPING);
  }
  else if(!strcmp(buf,"array"))
  {
    while(ISSPACE(**s)) ++*s;
    if(**s == '(')
    {
      ++*s;
      internal_parse_type(_s);
      if(**s != ')') error("Expecting ')'.\n");
      ++*s;
    }else{
      push_type(T_MIXED);
    }
    push_type(T_ARRAY);
  }
  else if(!strcmp(buf,"multiset"))
  {
    while(ISSPACE(**s)) ++*s;
    if(**s == '(')
    {
      ++*s;
      internal_parse_type(_s);
      if(**s != ')') error("Expecting ')'.\n");
      ++*s;
    }else{
      push_type(T_MIXED);
    }
    push_type(T_MULTISET);
  }
  else
    error("Couldn't parse type. (%s)\n",buf);

  while(ISSPACE(**s)) ++*s;
}


static void internal_parse_typeB(char **s)
{
  while(ISSPACE(**s)) ++*s;
  switch(**s)
  {
  case '!':
    ++*s;
    internal_parse_typeB(s);
    push_type(T_NOT);
    break;

  case '(':
    ++*s;
    internal_parse_typeB(s);
    while(ISSPACE(**s)) ++*s;
    if(**s != ')') error("Expecting ')'.\n");
    break;
    
  default:

    internal_parse_typeA(s);
  }
}

static void internal_parse_typeCC(char **s)
{
  internal_parse_typeB(s);

  while(ISSPACE(**s)) ++*s;
  
  while(**s == '*')
  {
    ++*s;
    while(ISSPACE(**s)) ++*s;
    push_type(T_ARRAY);
  }
}

static void internal_parse_typeC(char **s)
{
  type_stack_mark();

  type_stack_mark();
  internal_parse_typeCC(s);
  type_stack_reverse();

  while(ISSPACE(**s)) ++*s;
  
  if(**s == '&')
  {
    ++*s;
    type_stack_mark();
    internal_parse_typeC(s);
    type_stack_reverse();
    type_stack_reverse();
    push_type(T_AND);
  }else{
    type_stack_reverse();
  }
}

static void internal_parse_type(char **s)
{
  internal_parse_typeC(s);

  while(ISSPACE(**s)) ++*s;
  
  while(**s == '|')
  {
    ++*s;
    internal_parse_typeC(s);
    push_type(T_OR);
  }
}

/* This function is used when adding simul efuns so that
 * the types for the functions can be easily stored in strings.
 * It takes a string on the exact same format as Pike and returns a type
 * struct.
 */
struct pike_string *parse_type(char *s)
{
  type_stack_mark();
  internal_parse_type(&s);

  if( *s )
    fatal("Extra junk at end of type definition.\n");

  return pop_unfinished_type();
}

#ifdef DEBUG
void stupid_describe_type(char *a,INT32 len)
{
  INT32 e;
  for(e=0;e<len;e++)
  {
    if(e) printf(" ");
    switch(EXTRACT_UCHAR(a+e))
    {
    case T_INT: printf("int"); break;
    case T_FLOAT: printf("float"); break;
    case T_STRING: printf("string"); break;
    case T_PROGRAM: printf("program"); break;
    case T_OBJECT:
      printf("object(%ld)",(long)EXTRACT_INT(a+e+1));
      e+=sizeof(INT32);
      break;
    case T_FUNCTION: printf("function"); break;
    case T_ARRAY: printf("array"); break;
    case T_MAPPING: printf("mapping"); break;
    case T_MULTISET: printf("multiset"); break;

    case T_UNKNOWN: printf("unknown"); break;
    case T_MANY: printf("many"); break;
    case T_OR: printf("or"); break;
    case T_AND: printf("and"); break;
    case T_NOT: printf("not"); break;
    case T_VOID: printf("void"); break;
    case T_MIXED: printf("mixed"); break;

    default: printf("%d",EXTRACT_UCHAR(a+e)); break;
    }
  }
  printf("\n");
}

void simple_describe_type(struct pike_string *s)
{
  stupid_describe_type(s->str,s->len);
}
#endif

char *low_describe_type(char *t)
{
  switch(EXTRACT_UCHAR(t++))
  {
  case T_VOID: my_strcat("void"); break;
  case T_MIXED: my_strcat("mixed"); break;
  case T_UNKNOWN: my_strcat("unknown"); break;
  case T_INT: my_strcat("int"); break;
  case T_FLOAT: my_strcat("float"); break;
  case T_PROGRAM: my_strcat("program"); break;
  case T_OBJECT:
    my_strcat("object");
    t+=4;
    /* Prog id */
    break;
  case T_STRING: my_strcat("string"); break;

  case T_FUNCTION:
  {
    int s;
    my_strcat("function(");
    s=0;
    while(EXTRACT_UCHAR(t) != T_MANY)
    {
      if(s++) my_strcat(", ");
      t=low_describe_type(t);
    }
    t++;
    if(EXTRACT_UCHAR(t) == T_VOID)
    {
      t++;
    }else{
      if(s++) my_strcat(", ");
      t=low_describe_type(t);
      my_strcat(" ...");
    }
    my_strcat(" : ");
    t=low_describe_type(t);
    my_strcat(")");
    break;
  }

  case T_ARRAY:
    if(EXTRACT_UCHAR(t)==T_MIXED)
    {
      my_strcat("array");
      t++;
    }else{
      t=low_describe_type(t);
      my_strcat("*");
    }
    break;

  case T_MULTISET:
    my_strcat("multiset");
    if(EXTRACT_UCHAR(t)!=T_MIXED)
    {
      my_strcat("(");
      t=low_describe_type(t);
      my_strcat(")");
    }else{
      t++;
    }
    break;

  case T_NOT:
    my_strcat("!");
    t=low_describe_type(t);
    break;

  case T_OR:
    t=low_describe_type(t);
    my_strcat(" | ");
    t=low_describe_type(t);
    break;

  case T_AND:
    t=low_describe_type(t);
    my_strcat(" & ");
    t=low_describe_type(t);
    break;
    
  case T_MAPPING:
    my_strcat("mapping");
    if(EXTRACT_UCHAR(t)==T_MIXED && EXTRACT_UCHAR(t+1)==T_MIXED)
    {
      t+=2;
    }else{
      my_strcat("(");
      t=low_describe_type(t);
      my_strcat(":");
      t=low_describe_type(t);
      my_strcat(")");
    }
    break;
  }
  return t;
}

struct pike_string *describe_type(struct pike_string *type)
{
  if(!type) return make_shared_string("mixed");
  init_buf();
  low_describe_type(type->str);
  return free_buf();
}

static TYPE_T low_compile_type_to_runtime_type(char *t)
{
  TYPE_T tmp;
  switch(EXTRACT_UCHAR(t))
  {
  case T_OR:
    t++;
    tmp=low_compile_type_to_runtime_type(t);
    if(tmp == low_compile_type_to_runtime_type(t+type_length(t)))
      return tmp;

  case T_MANY:
  case T_UNKNOWN:
  case T_AND:
  case T_NOT:
    return T_MIXED;

  default:
    return EXTRACT_UCHAR(t);
  }
}

TYPE_T compile_type_to_runtime_type(struct pike_string *s)
{
  return low_compile_type_to_runtime_type(s->str);
}

#define A_EXACT 1
#define B_EXACT 2

/*
 * match two type strings, return zero if they don't match, and return
 * the part of 'a' that _did_ match if it did.
 */
static char *low_match_types(char *a,char *b, int flags)
{
  int correct_args;
  char *ret;
  if(a == b) return a;

  switch(EXTRACT_UCHAR(a))
  {
  case T_AND:
    a++;
    ret=low_match_types(a,b,flags);
    if(!ret) return 0;
    a+=type_length(a);
    return low_match_types(a,b,flags);

  case T_OR:
    a++;
    ret=low_match_types(a,b,flags);
    if(ret) return ret;
    a+=type_length(a);
    return low_match_types(a,b,flags);

  case T_NOT:
    if(low_match_types(a+1,b,flags | B_EXACT))
      return 0;
    return a;
  }

  switch(EXTRACT_UCHAR(b))
  {
  case T_AND:
    b++;
    ret=low_match_types(a,b,flags);
    if(!ret) return 0;
    b+=type_length(b);
    return low_match_types(a,b,flags);

  case T_OR:
    b++;
    ret=low_match_types(a,b,flags);
    if(ret) return ret;
    b+=type_length(b);
    return low_match_types(a,b,flags);

  case T_NOT:
    if(low_match_types(a,b+1, flags | A_EXACT))
      return 0;
    return a;
  }

  /* 'mixed' matches anything */
  if(EXTRACT_UCHAR(a) == T_MIXED && !(flags & A_EXACT)) return a;
  if(EXTRACT_UCHAR(b) == T_MIXED && !(flags & B_EXACT)) return a;

  /* Special cases (tm) */
  switch(EXTRACT_TWOT(a,b))
  {
  case TWOT(T_PROGRAM, T_FUNCTION):
  case TWOT(T_FUNCTION, T_PROGRAM):
    return a;
  case TWOT(T_OBJECT, T_FUNCTION):
  {
    struct program *p;
    if((p=id_to_program(EXTRACT_INT(a+1))))
    {
      int i=p->lfuns[LFUN_CALL];
      if(i == -1) return 0;
      return low_match_types(ID_FROM_INT(p, i)->type->str, b, flags);
    }
    return a;
  }

  case TWOT(T_FUNCTION, T_OBJECT):
  {
    struct program *p;
    if((p=id_to_program(EXTRACT_INT(b+1))))
    {
      int i=p->lfuns[LFUN_CALL];
      if(i == -1) return 0;
      return low_match_types(a, ID_FROM_INT(p, i)->type->str, flags);
    }
    return a;
  }
  }

  if(EXTRACT_UCHAR(a) != EXTRACT_UCHAR(b)) return 0;

  ret=a;
  switch(EXTRACT_UCHAR(a))
  {
  case T_FUNCTION:
    correct_args=0;
    a++;
    b++;
    while(EXTRACT_UCHAR(a)!=T_MANY || EXTRACT_UCHAR(b)!=T_MANY)
    {
      char *a_tmp,*b_tmp;
      if(EXTRACT_UCHAR(a)==T_MANY)
      {
	a_tmp=a+1;
      }else{
	a_tmp=a;
	a+=type_length(a);
      }

      if(EXTRACT_UCHAR(b)==T_MANY)
      {
	b_tmp=b+1;
      }else{
	b_tmp=b;
	b+=type_length(b);
      }

      if(!low_match_types(a_tmp, b_tmp, flags)) return 0;
      if(++correct_args > max_correct_args)
	max_correct_args=correct_args;
    }
    /* check the 'many' type */
    a++;
    b++;
    if(EXTRACT_UCHAR(b)==T_VOID || EXTRACT_UCHAR(a)==T_VOID)
    {
      a+=type_length(a);
      b+=type_length(b);
    }else{
      if(!low_match_types(a,b,flags)) return 0;
    }
    /* check the returntype */
    if(!low_match_types(a,b,flags)) return 0;
    break;

  case T_MAPPING:
    if(!low_match_types(++a,++b,flags)) return 0;
    if(!low_match_types(a+type_length(a),b+type_length(b),flags)) return 0;
    break;

  case T_OBJECT:
    a++;
    b++;
    if(!EXTRACT_INT(a) || !EXTRACT_INT(b)) break;
    if(EXTRACT_INT(a) != EXTRACT_INT(b)) return 0;
    break;

  case T_MULTISET:
  case T_ARRAY:
    if(!low_match_types(++a,++b,flags)) return 0;

  case T_INT:
  case T_FLOAT:
  case T_STRING:
  case T_PROGRAM:
  case T_VOID:
  case T_MIXED:
    break;

  default:
    fatal("error in type string.\n");
  }
  return ret;
}

/*
 * Return the return type from a function call.
 */
static int low_get_return_type(char *a,char *b)
{
  int tmp;
  switch(EXTRACT_UCHAR(a))
  {
  case T_OR:
    {
      struct pike_string *o1,*o2;
      a++;
      o1=o2=0;

      type_stack_mark();
      if(low_get_return_type(a,b)) 
      {
	o1=pop_unfinished_type();
	type_stack_mark();
      }

      if(low_get_return_type(a+type_length(a),b))
	o2=pop_unfinished_type();
      else
	pop_stack_mark();

      if(o1 == o2)
      {
	if(!o1)
	{
	  return 0;
	}else{
	  push_finished_type(o1);
	}
      }
      else if(o1 == mixed_type_string || o2 == mixed_type_string)
      {
	push_type(T_MIXED);
      }
      else
      {
	if(o1) push_finished_type(o1);
	if(o2) push_finished_type(o2);
	if(o1 && o2) push_type(T_OR);
      }

      if(o1) free_string(o1);
      if(o2) free_string(o2);

      return 1;
    }

  case T_AND:
    a++;
    type_stack_mark();
    tmp=low_get_return_type(a,b);
    type_stack_pop_to_mark();
    if(!tmp) return 0;
    return low_get_return_type(a+type_length(a),b);

  case T_ARRAY:
    a++;
    tmp=low_get_return_type(a,b);
    if(!tmp) return 0;
    push_type(T_ARRAY);
    return 1;
  }

  a=low_match_types(a,b,0);
  if(a)
  {
    switch(EXTRACT_UCHAR(a))
    {
    case T_FUNCTION:
      a++;
      while(EXTRACT_UCHAR(a)!=T_MANY) a+=type_length(a);
      a++;
      a+=type_length(a);
      push_unfinished_type(a);
      return 1;

    case T_PROGRAM:
      push_type_int(0);
      push_type(T_OBJECT);
      return 1;

    default:
      push_type(T_MIXED);
      return 1;
    }
  }
  return 0;
}


int match_types(struct pike_string *a,struct pike_string *b)
{
  CHECK_TYPE(a);
  CHECK_TYPE(b);
  return 0!=low_match_types(a->str, b->str,0);
}



/* FIXME, add the index */
static struct pike_string *low_index_type(char *t, node *n)
{
  switch(EXTRACT_UCHAR(t++))
  {
  case T_OBJECT:
  {
    struct program *p=id_to_program(EXTRACT_INT(t));
    if(p)
    {
      if(n->token == F_ARROW)
      {
	if(p->lfuns[LFUN_ARROW] != -1 || p->lfuns[LFUN_ASSIGN_ARROW] != -1)
	{
	  reference_shared_string(mixed_type_string);
	  return mixed_type_string;
	}
      }else{
	if(p->lfuns[LFUN_INDEX] != -1 || p->lfuns[LFUN_ASSIGN_INDEX] != -1)
	{
	  reference_shared_string(mixed_type_string);
	  return mixed_type_string;
	}
      }
      if(CDR(n)->token == F_CONSTANT && CDR(n)->u.sval.type==T_STRING)
      {
	INT32 i;
	i=find_shared_string_identifier(CDR(n)->u.sval.u.string, p);
	if(i==-1)
	{
	  reference_shared_string(int_type_string);
	  return int_type_string;
	}else{
	  reference_shared_string(ID_FROM_INT(p, i)->type);
	  return ID_FROM_INT(p, i)->type;
	}	   
      }
    }
  }
  default:
    reference_shared_string(mixed_type_string);
    return mixed_type_string;

  case T_OR:
  {
    struct pike_string *a,*b;
    type_stack_mark();
    a=low_index_type(t,n);
    t+=type_length(t);
    b=low_index_type(t,n);
    if(!b) return a;
    if(!a) return b;
    push_finished_type(b);
    push_finished_type(a);
    push_type(T_OR);
    return pop_unfinished_type();
  }

  case T_AND:
    return low_index_type(t+type_length(t),n);

  case T_STRING: /* always int */
  case T_MULTISET: /* always int */
    reference_shared_string(int_type_string);
    return int_type_string;

  case T_MAPPING:
    t+=type_length(t);
    return make_shared_binary_string(t, type_length(t));

  case T_ARRAY:
    if(low_match_types(string_type_string->str,CDR(n)->type->str,0))
    {
      struct pike_string *a=low_index_type(t,n);
      if(!a)
	return make_shared_binary_string(t, type_length(t));
	
      type_stack_mark();
      push_finished_type(a);
      free_string(a);
      push_type(T_ARRAY);
      if(low_match_types(int_type_string->str,CDR(n)->type->str,0))
      {
	push_unfinished_type(t);
	  push_type(T_OR);
      }
      return pop_unfinished_type();
    }else{
      return make_shared_binary_string(t, type_length(t));
    }
  }
}

struct pike_string *index_type(struct pike_string *type, node *n)
{
  struct pike_string *t;
  t=low_index_type(type->str,n);
  if(!t) copy_shared_string(t,mixed_type_string);
  return t;
}

static int low_check_indexing(char *type, char *index_type, node *n)
{
  switch(EXTRACT_UCHAR(type++))
  {
  case T_OR:
    return low_check_indexing(type,index_type,n) ||
      low_check_indexing(type+type_length(type),index_type,n);

  case T_AND:
    return low_check_indexing(type,index_type,n) &&
      low_check_indexing(type+type_length(type),index_type,n);

  case T_NOT:
    return !low_check_indexing(type,index_type,n);

  case T_ARRAY:
    if(low_match_types(string_type_string->str, index_type,0) &&
       low_check_indexing(type, index_type,n))
      return 1;

  case T_STRING:
    return !!low_match_types(int_type_string->str, index_type,0);

  case T_OBJECT:
  {
    struct program *p=id_to_program(EXTRACT_INT(type));
    if(p)
    {
      if(n->token == F_ARROW)
      {
	if(p->lfuns[LFUN_ARROW] != -1 || p->lfuns[LFUN_ASSIGN_ARROW] != -1)
	return 1;
      }else{
	if(p->lfuns[LFUN_INDEX] != -1 || p->lfuns[LFUN_ASSIGN_INDEX] != -1)
	return 1;
      }
      return !!low_match_types(string_type_string->str, index_type,0);
    }else{
      return 1;
    }
  }

  case T_MULTISET:
  case T_MAPPING:
    return !!low_match_types(type,index_type,0);

  case T_MIXED:
    return 1;
    
  default:
    return 0;
  }
}
				 
int check_indexing(struct pike_string *type,
		   struct pike_string *index_type,
		   node *n)
{
  CHECK_TYPE(type);
  CHECK_TYPE(index_type);

  return low_check_indexing(type->str, index_type->str, n);
}

/* Count the number of arguments for a funciton type.
 * return -1-n if the function can take number of arguments
 * >= n  (varargs)
 */
int count_arguments(struct pike_string *s)
{
  int num;
  char *q;

  CHECK_TYPE(s);

  q=s->str;
  if(EXTRACT_UCHAR(q) != T_FUNCTION) return MAX_LOCAL;
  q++;
  num=0;
  while(EXTRACT_UCHAR(q)!=T_MANY)
  {
    num++;
    q+=type_length(q);
  }
  q++;
  if(EXTRACT_UCHAR(q)!=T_VOID) return ~num;
  return num;
}

struct pike_string *check_call(struct pike_string *args,
			       struct pike_string *type)
{
  CHECK_TYPE(args);
  CHECK_TYPE(type);
  type_stack_mark();
  max_correct_args=0;
  
  if(low_get_return_type(type->str,args->str))
  {
    return pop_unfinished_type();
  }else{
    pop_stack_mark();
    return 0;
  }
}

struct pike_string *get_type_of_svalue(struct svalue *s)
{
  struct pike_string *ret;
  switch(s->type)
  {
  case T_FUNCTION:
    if(s->subtype == FUNCTION_BUILTIN)
    {
      ret=s->u.efun->type;
    }else{
      struct program *p;

      p=s->u.object->prog;
      if(!p)
      {
	ret=int_type_string;
      }else{
	ret=ID_FROM_INT(p,s->subtype)->type;
      }
    }
    reference_shared_string(ret);
    return ret;
       
  case T_ARRAY:
    type_stack_mark();
    push_type(T_MIXED);
    push_type(T_ARRAY);
    return pop_unfinished_type();

  case T_MULTISET:
    type_stack_mark();
    push_type(T_MIXED);
    push_type(T_MULTISET);
    return pop_unfinished_type();

  case T_MAPPING:
    type_stack_mark();
    push_type(T_MIXED);
    push_type(T_MIXED);
    push_type(T_MAPPING);
    return pop_unfinished_type();

  case T_OBJECT:
    type_stack_mark();
    if(s->u.object->prog)
    {
      push_type_int(s->u.object->prog->id);
    }else{
      push_type_int(0);
    }
    push_type(T_OBJECT);
    return pop_unfinished_type();

  case T_INT:
    if(s->u.integer)
    {
      ret=int_type_string;
    }else{
      ret=mixed_type_string;
    }
    reference_shared_string(ret);
    return ret;

  case T_PROGRAM:
  {
    char *a;
    int id=s->u.program->lfuns[LFUN_CREATE];
    if(id>=0)
    {
      a=ID_FROM_INT(s->u.program, id)->type->str;
    }else{
      a=function_type_string->str;
    }
    if(EXTRACT_UCHAR(a)==T_FUNCTION)
    {
      type_stack_mark();
      push_type_int(s->u.program->id);
      push_type(T_OBJECT);
      
      type_stack_mark();
      a++;
      while(EXTRACT_UCHAR(a)!=T_MANY)
      {
	type_stack_mark();
	push_unfinished_type(a);
	type_stack_reverse();
	a+=type_length(a);
      }
      a++;
      push_type(T_MANY);
      type_stack_mark();
      push_unfinished_type(a);
      type_stack_reverse();
      type_stack_reverse();
      push_type(T_FUNCTION);
      return pop_unfinished_type();
    }
  }

  default:
    type_stack_mark();
    push_type(s->type);
    return pop_unfinished_type();
  }
}


char *get_name_of_type(int t)
{
  switch(t)
  {
    case T_ARRAY: return "array";
    case T_FLOAT: return "float";
    case T_FUNCTION: return "function";
    case T_INT: return "int";
    case T_LVALUE: return "lvalue";
    case T_MAPPING: return "mapping";
    case T_MULTISET: return "multiset";
    case T_OBJECT: return "object";
    case T_PROGRAM: return "program";
    case T_STRING: return "string";
    case T_VOID: return "void";
    default: return "unknown";
  }
}

void cleanup_pike_types(void)
{
  free_string(string_type_string);
  free_string(int_type_string);
  free_string(float_type_string);
  free_string(function_type_string);
  free_string(object_type_string);
  free_string(program_type_string);
  free_string(array_type_string);
  free_string(multiset_type_string);
  free_string(mapping_type_string);
  free_string(mixed_type_string);
  free_string(void_type_string);
  free_string(any_type_string);
}
