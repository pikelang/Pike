#include "global.h"
#include <ctype.h>
#include "svalue.h"
#include "lpc_types.h"
#include "stralloc.h"
#include "stuff.h"
#include "array.h"
#include "program.h"
#include "add_efun.h"
#include "object.h"
#include "list.h"
#include "mapping.h"
#include "macros.h"
#include "error.h"

/*
 * basic types are represented by just their value in a string
 * basic type are string, int, float, object and program
 * arrays are coded like by the value T_ARRAY followed by the
 * data type, if the type is not known it is T_MIXED, ie:
 * T_ARRAY <data type>
 * mappings are followed by two arguments, the first is the type
 * for the indices, and the second is the type of the data, ie:
 * T_MAPPING <indice type> <data type>
 * list works similarly to arrays.
 * functions are _very_ special:
 * they are coded like this:
 * T_FUNCTION <arg type> <arg type> ... <arg type> T_MANY <arg type> <return type>
 * note that the type after T_MANY can be T_VOID
 * T_TRUE matches anything
 * T_MIXED matches anything except T_VOID
 * T_UNKNOWN only matches T_MIXED and T_UNKNOWN
 */

struct lpc_string *string_type_string;
struct lpc_string *int_type_string;
struct lpc_string *float_type_string;
struct lpc_string *function_type_string;
struct lpc_string *object_type_string;
struct lpc_string *program_type_string;
struct lpc_string *array_type_string;
struct lpc_string *list_type_string;
struct lpc_string *mapping_type_string;
struct lpc_string *mixed_type_string;
struct lpc_string *void_type_string;

void init_types()
{
  string_type_string=parse_type("string");
  int_type_string=parse_type("int");
  object_type_string=parse_type("object");
  program_type_string=parse_type("program");
  float_type_string=parse_type("float");
  mixed_type_string=parse_type("mixed");
  array_type_string=parse_type("array");
  list_type_string=parse_type("list");
  mapping_type_string=parse_type("mapping");
  function_type_string=parse_type("function");
  void_type_string=parse_type("void");
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
    t+=type_length(t);

  case T_ARRAY:
  case T_LIST:
    t+=type_length(t);

  case T_INT:
  case T_FLOAT:
  case T_STRING:
  case T_OBJECT:
  case T_PROGRAM:
  case T_MIXED:
  case T_VOID:
  case T_TRUE:
  case T_UNKNOWN:
    break;
  }
  return t-q;
}

static void internal_parse_type(char **s)
{
  char buf[80];
  unsigned int len;

  while(isspace(**s)) ++*s;

  len=0;
  for(len=0;isidchar(s[0][len]);len++)
  {
    if(len>=sizeof(buf)) error("Buffer overflow in parse_type\n");
    buf[len] = s[0][len];
  }
  buf[len]=0;
  *s += len;
  
  if(!strcmp(buf,"int")) push_type(T_INT);
  else if(!strcmp(buf,"float")) push_type(T_FLOAT);
  else if(!strcmp(buf,"object")) push_type(T_OBJECT);
  else if(!strcmp(buf,"program")) push_type(T_PROGRAM);
  else if(!strcmp(buf,"string")) push_type(T_STRING);
  else if(!strcmp(buf,"void")) push_type(T_VOID);
  else if(!strcmp(buf,"mixed")) push_type(T_MIXED);
  else if(!strcmp(buf,"unknown")) push_type(T_UNKNOWN);
  else if(!strcmp(buf,"function"))
  {
    while(isspace(**s)) ++*s;
    if(**s == '(')
    {
      ++*s;
      while(isspace(**s)) ++*s;
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
	internal_parse_type(s);
	type_stack_reverse();
	if(**s==',')
	{
	  ++*s;
	  while(isspace(**s)) ++*s;
	}
	else if(s[0][0]=='.' && s[0][1]=='.' && s[0][2]=='.')
	{
	  type_stack_reverse();
	  push_type(T_MANY);
	  type_stack_reverse();
	  *s+=3;
	  while(isspace(**s)) ++*s;
	  if(**s != ':') error("Missing ':' after ... in function type.\n");
	  break;
	}
	pop_stack_mark();
	pop_stack_mark();
      }
      ++*s;
      type_stack_mark();
      internal_parse_type(s);  /* return type */
      type_stack_reverse();
      if(**s != ')') error("Missing ')' in function type.\n");
      ++*s;
      type_stack_reverse();
    }else{
      push_type(T_TRUE);
      push_type(T_MIXED);
      push_type(T_MANY);
    }
    push_type(T_FUNCTION);
  }
  else if(!strcmp(buf,"mapping"))
  {
    while(isspace(**s)) ++*s;
    if(**s == '(')
    {
      ++*s;
      internal_parse_type(s);
      if(**s != ':') error("Expecting ':'.\n");
      ++*s;
      internal_parse_type(s);
      if(**s != ')') error("Expecting ')'.\n");
      ++*s;
    }else{
      push_type(T_MIXED);
      push_type(T_MIXED);
    }
    push_type(T_MAPPING);
  }
  else if(!strcmp(buf,"array"))
  {
    while(isspace(**s)) ++*s;
    if(**s == '(')
    {
      ++*s;
      internal_parse_type(s);
      if(**s != ')') error("Expecting ')'.\n");
      ++*s;
    }else{
      push_type(T_MIXED);
    }
    push_type(T_ARRAY);
  }
  else if(!strcmp(buf,"list"))
  {
    while(isspace(**s)) ++*s;
    if(**s == '(')
    {
      ++*s;
      internal_parse_type(s);
      if(**s != ')') error("Expecting ')'.\n");
      ++*s;
    }else{
      push_type(T_MIXED);
    }
    push_type(T_LIST);
  }
  else
    error("Couldn't parse type. (%s)\n",buf);

  while(isspace(**s)) ++*s;
  while(**s == '*')
  {
    ++*s;
    push_type(T_ARRAY);
    while(isspace(**s)) ++*s;
  }

  while(**s == '|')
  {
    ++*s;
    internal_parse_type(s);
    push_type(T_OR);
    while(isspace(**s)) ++*s;
  }
}

/* This function is used when adding simul efuns so that
 * the types for the functions can be easily stored in strings.
 * It takes a string on the exact same format as uLPC and returns a type
 * struct.
 */
struct lpc_string *parse_type(char *s)
{
  internal_parse_type(&s);

  if( *s )
    fatal("Extra junk at end of type definition.\n");

  return pop_type();
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
    case T_OBJECT: printf("object"); break;
    case T_FUNCTION: printf("function"); break;
    case T_ARRAY: printf("array"); break;
    case T_MAPPING: printf("mapping"); break;
    case T_LIST: printf("list"); break;

    case T_UNKNOWN: printf("unknown"); break;
    case T_TRUE: printf("true"); break;
    case T_MANY: printf("many"); break;
    case T_OR: printf("or"); break;
    case T_VOID: printf("void"); break;
    case T_MIXED: printf("mixed"); break;

    default: printf("%d",EXTRACT_UCHAR(a+e)); break;
    }
  }
  printf("\n");
}
#endif

char *low_describe_type(char *t)
{
  switch(EXTRACT_UCHAR(t++))
  {
  case T_VOID:
    my_strcat("void");
    break;

  case T_MIXED:
    my_strcat("mixed");
    break;

  case T_TRUE:
    my_strcat("true");
    break;

  case T_UNKNOWN:
    my_strcat("unknown");
    break;

  case T_INT:
    my_strcat("int");
    break;

  case T_FLOAT:
    my_strcat("float");
    break;

  case T_PROGRAM:
    my_strcat("program");
    break;

  case T_OBJECT:
    my_strcat("object");
    break;

  case T_FUNCTION:
  {
    int s;
    my_strcat("function(");
    s=0;
    while(EXTRACT_UCHAR(t) != T_MANY)
    {
      if(s++) my_strcat(", ");
      low_describe_type(t);
      t+=type_length(t);
    }
    t++;
    if(EXTRACT_UCHAR(t) != T_VOID)
    {
      if(s++) my_strcat(", ");
      low_describe_type(t);
      my_strcat(" ...");
    }
    t+=type_length(t);
    my_strcat(" : ");
    low_describe_type(t);
    my_strcat(")");
    break;
  }

  case T_STRING:
    my_strcat("string");
    break;

  case T_ARRAY:
    t=low_describe_type(t);
    my_strcat("*");
    break;

  case T_LIST:
    my_strcat("list (");
    t=low_describe_type(t);
    my_strcat(")");
    break;

  case T_OR:
    t=low_describe_type(t);
    my_strcat(" | ");
    t=low_describe_type(t);
    break;
    
  case T_MAPPING:
    my_strcat("mapping (");
    t=low_describe_type(t);
    my_strcat(":");
    t=low_describe_type(t);
    my_strcat(")");
    break;
  }
  return t;
}

TYPE_T compile_type_to_runtime_type(struct lpc_string *s)
{
  char *t;
  t=s->str;
  
  switch(EXTRACT_UCHAR(t))
  {
  case T_OR:
  case T_MANY:
  case T_TRUE:
  case T_UNKNOWN:
    return T_MIXED;

  default:
    return EXTRACT_UCHAR(t);
  }
}

/*
 * match two type strings, return zero if they don't match, and return
 * the part of 'a' that _did_ match if it did.
 */
static char *low_match_types(char *a,char *b)
{
  char *ret;
  if(a == b) return a;

  if(EXTRACT_UCHAR(a) == T_OR)
  {
    a++;
    ret=low_match_types(a,b);
    if(ret) return ret;
    a+=type_length(a);
    return low_match_types(a,b);
  }

  if(EXTRACT_UCHAR(b) == T_OR)
  {
    b++;
    ret=low_match_types(a,b);
    if(ret) return ret;
    b+=type_length(b);
    return low_match_types(a,b);
  }

  if(EXTRACT_UCHAR(a)==T_TRUE) return a;
  if(EXTRACT_UCHAR(b)==T_TRUE) return a;

  /* 'mixed' matches anything except 'void' */
  if(EXTRACT_UCHAR(a) == T_MIXED && EXTRACT_UCHAR(b) != T_VOID) return a;
  if(EXTRACT_UCHAR(b) == T_MIXED && EXTRACT_UCHAR(a) != T_VOID) return a;
  if(EXTRACT_UCHAR(a) != EXTRACT_UCHAR(b)) return 0;

  ret=a;
  switch(EXTRACT_UCHAR(a))
  {
  case T_FUNCTION:
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
	b_tmp=a+1;
      }else{
	b_tmp=b;
	b+=type_length(b);
      }

      if(!low_match_types(a_tmp, b_tmp)) return 0;
    }
    /* check the 'many' type */
    a++;
    b++;
    if(EXTRACT_UCHAR(b)==T_VOID)
    {
      b++;
      a+=type_length(a);
    }else{
      if(!low_match_types(a,b)) return 0;
    }
    /* check the returntype */
    if(!low_match_types(a,b)) return 0;
    break;

  case T_MAPPING:
    if(!low_match_types(++a,++b)) return 0;
    if(!low_match_types(a+type_length(a),b+type_length(b))) return 0;
    break;

  case T_LIST:
  case T_ARRAY:
    if(!low_match_types(++a,++b)) return 0;

  case T_INT:
  case T_FLOAT:
  case T_STRING:
  case T_OBJECT:
  case T_PROGRAM:
  case T_VOID:
  case T_MIXED:
  case T_TRUE:
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
  if(EXTRACT_UCHAR(a) == T_OR)
  {
    int tmp;
    a++;
    tmp=low_get_return_type(a,b);
    tmp+=low_get_return_type(a+type_length(a),b);
    if(tmp==2) push_type(T_OR);
    return tmp>0;
  }

  if(EXTRACT_UCHAR(a) == T_ARRAY)
  {
    int tmp;
    a++;
    tmp=low_get_return_type(a,b);
    if(!tmp) return 0;
    push_type(T_ARRAY);
    return 1;
  }

  a=low_match_types(a,b);
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

    default:
      push_type(T_MIXED);
      return 1;
    }
  }
  return 0;
}


int match_types(struct lpc_string *a,struct lpc_string *b)
{
  return 0!=low_match_types(a->str, b->str);
}


#define STACK_SIZE 10000
static unsigned char type_stack[STACK_SIZE];
static unsigned char *type_stackp=type_stack;
static unsigned char *mark_stack[STACK_SIZE/4];
static unsigned char **mark_stackp=mark_stack;

void reset_type_stack()
{
  type_stackp=type_stack;
  mark_stackp=mark_stack;
}

void type_stack_mark()
{
  *mark_stackp=type_stackp;
  mark_stackp++;
  if(mark_stackp > mark_stack + NELEM(mark_stack))
    yyerror("Type mark stack overflow.");
}

void pop_stack_mark()
{ 
  mark_stackp--;
  if(mark_stackp<mark_stack)
    fatal("Type mark stack underflow\n");
}

void pop_type_stack()
{ 
  type_stackp--;
  if(type_stackp<type_stack)
    fatal("Type stack underflow\n");
}

void type_stack_reverse()
{
  unsigned char *a,*b,tmp;
  a=mark_stackp[-1];
  b=type_stackp-1;
  while(b>a) { tmp=*a; *a=*b; *b=tmp; b--; a++; }
  pop_stack_mark();
}

void push_type(unsigned char tmp)
{
  *type_stackp=tmp;
  type_stackp++;
  if(type_stackp > type_stack + sizeof(type_stack))
    yyerror("Type stack overflow.");
}

void push_unfinished_type(char *s)
{
  int e;
  e=type_length(s);
  for(e--;e>=0;e--) push_type(s[e]);
}

void push_finished_type(struct lpc_string *type)
{
  int e;
  for(e=type->len-1;e>=0;e--) push_type(type->str[e]);
}


struct lpc_string *pop_type()
{
  int len,e;
  struct lpc_string *s;
  len=type_stackp - type_stack;
  s=begin_shared_string(len);
  for(e=0;e<len;e++) s->str[e] = *--type_stackp;
  s=end_shared_string(s);
  reset_type_stack();
  return s;
}

/* FIXME, add the index */
static struct lpc_string *low_index_type(char *t)
{
  switch(EXTRACT_UCHAR(t++))
  {
  default:
    reference_shared_string(mixed_type_string);
    return mixed_type_string;

  case T_OR:
  {
    struct lpc_string *a,*b;
    a=low_index_type(t);
    t+=type_length(t);
    b=low_index_type(t);
    if(!b) return a;
    if(!a) return b;
    push_finished_type(b);
    push_finished_type(a);
    push_type(T_OR);
    return pop_type();
  }


  case T_STRING: /* always int */
  case T_LIST: /* always int */
    reference_shared_string(int_type_string);
    return int_type_string;

  case T_MAPPING:
    t+=type_length(t);

  case T_ARRAY:
    return make_shared_binary_string(t, type_length(t));
  }
}

struct lpc_string *index_type(struct lpc_string *type)
{
  struct lpc_string *t;
  t=low_index_type(type->str);
  if(!t) copy_shared_string(t,mixed_type_string);
  return t;
}

static int low_check_indexing(char *type, char *index_type)
{
  switch(EXTRACT_UCHAR(type++))
  {
  case T_OR:
    return low_check_indexing(type,index_type) ||
      low_check_indexing(type+type_length(type),index_type);
    
  case T_STRING:
  case T_ARRAY:
    return !!low_match_types(int_type_string->str, index_type);

  case T_OBJECT:
    return !!low_match_types(string_type_string->str, index_type);

  case T_LIST:
  case T_MAPPING:
    return !!low_match_types(type,index_type);

  case T_MIXED:
    return 1;
    
  default:
    return 0;
  }
}
				 
int check_indexing(struct lpc_string *type,
		   struct lpc_string *index_type)
{
  return low_check_indexing(type->str, index_type->str);
}

/* Count the number of arguments for a funciton type.
 * return -1-n if the function can take number of arguments
 * >= n  (varargs)
 */
int count_arguments(struct lpc_string *s)
{
  int num;
  char *q;

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

struct lpc_string *check_call(struct lpc_string *args,
				 struct lpc_string *type)
{
  reset_type_stack();
  if(low_get_return_type(type->str,args->str))
  {
    return pop_type();
  }else{
    return 0;
  }
}


void check_array_type(struct array *a)
{
  int t;
  t=my_log2(a->array_type);
  if( ( 1 << t ) == a->array_type )
  {
    switch(t)
    {
    case T_ARRAY:
      push_type(T_MIXED);
      push_type(T_ARRAY);
      break;

    case T_LIST:
      push_type(T_MIXED);
      push_type(T_LIST);
      break;

    case T_MAPPING:
      push_type(T_MIXED);
      push_type(T_MIXED);
      push_type(T_MAPPING);
      break;

    default:
      push_type(t);
    }
  }else{
    push_type(T_MIXED);
  }
}

struct lpc_string *get_type_of_svalue(struct svalue *s)
{
  struct lpc_string *ret;
  switch(s->type)
  {
  case T_FUNCTION:
    if(s->subtype == -1)
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
    check_array_type(s->u.array);
    push_type(T_ARRAY);
    return pop_type();

  case T_LIST:
    check_array_type(s->u.list->ind);
    push_type(T_LIST);
    return pop_type();

  case T_MAPPING:
    check_array_type(s->u.mapping->ind);
    check_array_type(s->u.mapping->val);
    push_type(T_MAPPING);
    return pop_type();

  default:
    push_type(s->type);
    return pop_type();
  }
}


char *get_name_of_type(int t)
{
  switch(t)
  {
    case T_LVALUE: return "lvalue";
    case T_INT: return "int";
    case T_STRING: return "string";
    case T_ARRAY: return "array";
    case T_OBJECT: return "object";
    case T_MAPPING: return "mapping";
    case T_LIST: return "list";
    case T_FUNCTION: return "function";
    case T_FLOAT: return "float";
    default: return "unknown";
  }
}

void cleanup_lpc_types()
{
  free_string(string_type_string);
  free_string(int_type_string);
  free_string(float_type_string);
  free_string(function_type_string);
  free_string(object_type_string);
  free_string(program_type_string);
  free_string(array_type_string);
  free_string(list_type_string);
  free_string(mapping_type_string);
  free_string(mixed_type_string);
  free_string(void_type_string);
}
