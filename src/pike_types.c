/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: pike_types.c,v 1.62 1999/11/20 21:58:02 grubba Exp $");
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
#include "bignum.h"

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
 * objects are coded thus:
 * T_OBJECT <0/1> <program_id>
 *           ^
 *           0 means 'inherits'
 *           1 means 'is'
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

static struct pike_string *a_markers[10],*b_markers[10];

static void clear_markers(void)
{
  unsigned int e;
  for(e=0;e<NELEM(a_markers);e++)
  {
    if(a_markers[e])
    {
      free_string(a_markers[e]);
      a_markers[e]=0;
    }
    if(b_markers[e])
    {
      free_string(b_markers[e]);
      b_markers[e]=0;
    }
  }
}

#ifdef PIKE_DEBUG
void check_type_string(struct pike_string *s)
{
  if(debug_findstring(s) != s)
    fatal("Type string not shared.\n");

  if(type_length(s->str) != s->len)
  {
    stupid_describe_type(s->str,s->len);
    fatal("Length of type is wrong. (should be %d, is %d)\n",type_length(s->str),s->len);
  }
}
#endif

void init_types(void)
{
  string_type_string = CONSTTYPE(tString);
  int_type_string = CONSTTYPE(tInt);
  object_type_string = CONSTTYPE(tObj);
  program_type_string = CONSTTYPE(tPrg);
  float_type_string = CONSTTYPE(tFloat);
  mixed_type_string=CONSTTYPE(tMix);
  array_type_string=CONSTTYPE(tArray);
  multiset_type_string=CONSTTYPE(tMultiset);
  mapping_type_string=CONSTTYPE(tMapping);
  function_type_string=CONSTTYPE(tFunction);
  void_type_string=CONSTTYPE(tVoid);
  any_type_string=CONSTTYPE(tOr(tVoid,tMix));
}

static int type_length(char *t)
{
  char *q=t;
one_more_type:
  switch(EXTRACT_UCHAR(t++))
  {
    default:
      fatal("error in type string %d.\n",EXTRACT_UCHAR(t-1));
      /*NOTREACHED*/
      
      break;
      
    case T_ASSIGN:
      t++;
      goto one_more_type;
      
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
    case T_FLOAT:
    case T_STRING:
    case T_PROGRAM:
    case T_MIXED:
    case T_VOID:
    case T_UNKNOWN:
      break;

    case T_INT:
      t+=sizeof(INT32)*2;
      break;
      
    case T_OBJECT:
      t++;
      t+=sizeof(INT32);
      break;
  }
  return t-q;
}


unsigned char type_stack[PIKE_TYPE_STACK_SIZE];
unsigned char *type_stackp=type_stack;
unsigned char *pike_type_mark_stack[PIKE_TYPE_STACK_SIZE/4];
unsigned char **pike_type_mark_stackp=pike_type_mark_stack;


INT32 pop_stack_mark(void)
{ 
  pike_type_mark_stackp--;
  if(pike_type_mark_stackp<pike_type_mark_stack)
    fatal("Type mark stack underflow\n");

  return type_stackp - *pike_type_mark_stackp;
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
#ifdef PIKE_DEBUG
  if(type_stackp<type_stack)
    fatal("Type stack underflow\n");
#endif
}

void type_stack_reverse(void)
{
  INT32 a;
  a=pop_stack_mark();
  reverse((char *)(type_stackp-a),a,1);
}

void push_type_int(INT32 i)
{
  int e;
  for(e=0;e<(int)sizeof(i);e++)
    push_type( (i>>(e*8)) & 0xff );
}

INT32 extract_type_int(char *p)
{
  int e;
  INT32 ret=0;
  for(e=0;e<(int)sizeof(INT32);e++)
    ret=(ret<<8) | EXTRACT_UCHAR(p+e);
  return ret;
}

void push_unfinished_type(char *s)
{
  int e;
  e=type_length(s);
  for(e--;e>=0;e--) push_type(s[e]);
}

static void push_unfinished_type_with_markers(char *s, struct pike_string **am)
{
  int d,e,c,len=type_length(s);
  type_stack_mark();
  for(e=0;e<len;e++)
  {
    switch(c=EXTRACT_UCHAR(s+e))
    {
#if 1
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	if(am[c-'0'])
	{
	  push_finished_type_backwards(am[c-'0']);
	}else{
	  push_type(T_MIXED);
	}
	break;
#endif

      case T_INT:
	push_type(c);
	for(d=0;d<(int)sizeof(INT32)*2;d++)
	  push_type(EXTRACT_UCHAR(s+ ++e));
	break;
	
      case T_OBJECT:
	push_type(c);
	for(d=0;d<(int)sizeof(INT32)+1;d++) push_type(EXTRACT_UCHAR(s+ ++e));
	break;
	
      default:
	push_type(c);
    }
  }
  type_stack_reverse();
}

void push_finished_type(struct pike_string *type)
{
  int e;
  check_type_string(type);
  for(e=type->len-1;e>=0;e--) push_type(type->str[e]);
}

void push_finished_type_backwards(struct pike_string *type)
{
  int e;
  check_type_string(type);
  MEMCPY(type_stackp, type->str, type->len);
  type_stackp+=type->len;
}

struct pike_string *debug_pop_unfinished_type(void)
{
  int len,e;
  struct pike_string *s;
  len=pop_stack_mark();
  s=begin_shared_string(len);
  type_stackp-=len;
  MEMCPY(s->str, type_stackp, len);
  reverse(s->str, len, 1);
  s=end_shared_string(s);
  check_type_string(s);
  return s;
}

struct pike_string *debug_pop_type(void)
{
  struct pike_string *s;
  s=pop_unfinished_type();
  type_stack_mark();
  return s;
}

struct pike_string *debug_compiler_pop_type(void)
{
  extern int num_parse_error;
  if(num_parse_error)
  {
    /* This could be fixed to check if the type
     * is correct and then return it, I just didn't feel
     * like writing the checking code today. / Hubbe
     */
    type_stack_pop_to_mark();
    type_stack_mark();
    reference_shared_string(mixed_type_string);
    return mixed_type_string;
  }else{
    struct pike_string *s;
    s=pop_unfinished_type();
    type_stack_mark();
    return s;
  }
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
  
  switch(buf[0])
  {
    case 'i':
      if(!strcmp(buf,"int"))
      {
	while(ISSPACE(**s)) ++*s;
	if(**s=='(')
	{
	  INT32 min,max;
	  ++*s;
	  while(ISSPACE(**s)) ++*s;
	  min=STRTOL(*s,(char **)s,0);
	  while(ISSPACE(**s)) ++*s;
	  if(s[0][0]=='.' && s[0][1]=='.')
	    s[0]+=2;
	  else
	    error("Missing .. in integer type.\n");
	  
	  while(ISSPACE(**s)) ++*s;
	  max=STRTOL(*s,(char **)s,0);
	  while(ISSPACE(**s)) ++*s;

	  if(**s != ')') error("Missing ')' in integer range.\n");
	  ++*s;
	  push_type_int(max);
	  push_type_int(min);
	}else{
	  push_type_int(MAX_INT32);
	  push_type_int(MIN_INT32);
	}
	push_type(T_INT);
	break;
      }
      goto bad_type;

    case 'f':
      if(!strcmp(buf,"function"))
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
	break;
      }
      if(!strcmp(buf,"float")) { push_type(T_FLOAT); break; }
      goto bad_type;

    case 'o':
      if(!strcmp(buf,"object"))
      {
	push_type_int(0);
	push_type(0);
	push_type(T_OBJECT);
	break;
      }
      goto bad_type;


    case 'p':
      if(!strcmp(buf,"program")) { push_type(T_PROGRAM); break; }
      goto bad_type;


    case 's':
      if(!strcmp(buf,"string")) { push_type(T_STRING); break; }
      goto bad_type;

    case 'v':
      if(!strcmp(buf,"void")) { push_type(T_VOID); break; }
      goto bad_type;

    case 'm':
      if(!strcmp(buf,"mixed")) { push_type(T_MIXED); break; }
      if(!strcmp(buf,"mapping"))
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
	break;
      }
      if(!strcmp(buf,"multiset"))
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
	break;
      }
      goto bad_type;

    case 'u':
      if(!strcmp(buf,"unknown")) { push_type(T_UNKNOWN); break; }
      goto bad_type;

    case 'a':
      if(!strcmp(buf,"array"))
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
	break;
      }
      goto bad_type;

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
      if(atoi(buf)<10)
      {
	while(ISSPACE(**s)) ++*s;
	if(**s=='=')
	{
	  ++*s;
	  internal_parse_type(_s);
	  push_type(buf[0]);
	  push_type(T_ASSIGN);
	}else{
	  push_type(buf[0]);
	}
	break;
      }

    default:
  bad_type:
      error("Couldn't parse type. (%s)\n",buf);
  }

  while(ISSPACE(**s)) ++*s;
}


static void internal_parse_typeB(char **s)
{
  while(ISSPACE(**((unsigned char **)s))) ++*s;
  switch(**s)
  {
  case '!':
    ++*s;
    internal_parse_typeB(s);
    push_type(T_NOT);
    break;

  case '(':
    ++*s;
    internal_parse_type(s);
    while(ISSPACE(**((unsigned char **)s))) ++*s;
    if(**s != ')') error("Expecting ')'.\n");
    break;
    
  default:

    internal_parse_typeA(s);
  }
}

static void internal_parse_typeCC(char **s)
{
  internal_parse_typeB(s);

  while(ISSPACE(**((unsigned char **)s))) ++*s;
  
  while(**s == '*')
  {
    ++*s;
    while(ISSPACE(**((unsigned char **)s))) ++*s;
    push_type(T_ARRAY);
  }
}

static void internal_parse_typeC(char **s)
{
  type_stack_mark();

  type_stack_mark();
  internal_parse_typeCC(s);
  type_stack_reverse();

  while(ISSPACE(**((unsigned char **)s))) ++*s;
  
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

  while(ISSPACE(**((unsigned char **)s))) ++*s;
  
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
  struct pike_string *ret;
#ifdef PIKE_DEBUG
  unsigned char *ts=type_stackp;
  unsigned char **ptms=pike_type_mark_stackp;
#endif
  type_stack_mark();
  internal_parse_type(&s);

  if( *s )
    fatal("Extra junk at end of type definition.\n");

  ret=pop_unfinished_type();

#ifdef PIKE_DEBUG
  if(ts!=type_stackp || ptms!=pike_type_mark_stackp)
    fatal("Type stack whacked in parse_type.\n");
#endif

  return ret;
}

#ifdef PIKE_DEBUG
void stupid_describe_type(char *a,INT32 len)
{
  INT32 e;
  for(e=0;e<len;e++)
  {
    if(e) printf(" ");
    switch(EXTRACT_UCHAR(a+e))
    {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	printf("%c",EXTRACT_UCHAR(a+e));
	break;
	
      case T_ASSIGN: printf("="); break;
      case T_INT:
	{
	  INT32 min=extract_type_int(a+e+1);
	  INT32 max=extract_type_int(a+e+1+sizeof(INT32));
	  printf("int");
	  if(min!=MIN_INT32 || max!=MAX_INT32)
	    printf("(%ld..%ld)",(long)min,(long)max);
	  e+=sizeof(INT32)*2;
	  break;
	}
      case T_FLOAT: printf("float"); break;
      case T_STRING: printf("string"); break;
      case T_PROGRAM: printf("program"); break;
      case T_OBJECT:
	printf("object(%s %ld)",
	       EXTRACT_UCHAR(a+e+1)?"clone of":"inherits",
	       (long)extract_type_int(a+e+2));
	e+=sizeof(INT32)+1;
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
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      my_putchar(EXTRACT_UCHAR(t-1));
      break;
      
    case T_ASSIGN:
      my_putchar('(');
      my_putchar(EXTRACT_UCHAR(t++));
      my_putchar('=');
      t=low_describe_type(t);
      my_putchar(')');
      break;
      
    case T_VOID: my_strcat("void"); break;
    case T_MIXED: my_strcat("mixed"); break;
    case T_UNKNOWN: my_strcat("unknown"); break;
    case T_INT:
    {
      INT32 min=extract_type_int(t);
      INT32 max=extract_type_int(t+sizeof(INT32));
      my_strcat("int");
      
      if(min!=MIN_INT32 || max!=MAX_INT32)
      {
	char buffer[100];
	sprintf(buffer,"(%ld..%ld)",(long)min,(long)max);
	my_strcat(buffer);
      }
      t+=sizeof(INT32)*2;
      
      break;
    }
    case T_FLOAT: my_strcat("float"); break;
    case T_PROGRAM: my_strcat("program"); break;
    case T_OBJECT:
      if(extract_type_int(t+1))
      {
	char buffer[100];
	sprintf(buffer,"object(%s %ld)",*t?"is":"implements",(long)extract_type_int(t+1));
	my_strcat(buffer);
      }else{
	my_strcat("object");
      }
      
      t+=sizeof(INT32)+1;
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
      my_strcat("array");
      if(EXTRACT_UCHAR(t)==T_MIXED)
      {
	t++;
      }else{
	my_strcat("(");
	t=low_describe_type(t);
	my_strcat(")");
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
  check_type_string(type);
  if(!type) return make_shared_string("mixed");
  init_buf();
  low_describe_type(type->str);
  return free_buf();
}

static int low_is_same_type(char *a, char *b)
{
  if(type_length(a) != type_length(b)) return 0;
  return !MEMCMP(a,b,type_length(a));
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

    default:
      return T_MIXED;

    case T_ARRAY:
    case T_MAPPING:
    case T_MULTISET:

    case T_OBJECT:
    case T_PROGRAM:
    case T_FUNCTION:

    case T_STRING:
    case T_INT:
    case T_FLOAT:
      return EXTRACT_UCHAR(t);
  }
}

TYPE_T compile_type_to_runtime_type(struct pike_string *s)
{
  return low_compile_type_to_runtime_type(s->str);
}


static int low_find_exact_type_match(char *needle, char *haystack)
{
  while(EXTRACT_UCHAR(haystack)==T_OR)
  {
    haystack++;
    if(low_find_exact_type_match(needle, haystack))
      return 1;
    haystack+=type_length(haystack);
  }
  return low_is_same_type(needle, haystack);
}

static void very_low_or_pike_types(char *to_push, char *not_push)
{
  while(EXTRACT_UCHAR(to_push)==T_OR)
  {
    to_push++;
    very_low_or_pike_types(to_push, not_push);
    to_push+=type_length(to_push);
  }
  if(!low_find_exact_type_match(to_push, not_push))
  {
    push_unfinished_type(to_push);
    push_type(T_OR);
  }
}

static void low_or_pike_types(char *t1, char *t2)
{
  if(!t1)
  {
    if(!t2)
      push_type(T_VOID);
    else
      push_unfinished_type(t2);
  }
  else if(!t2)
  {
    push_unfinished_type(t1);
  }
  else if(EXTRACT_UCHAR(t1)==T_MIXED || EXTRACT_UCHAR(t2)==T_MIXED)
  {
    push_type(T_MIXED);
  }
  else if(EXTRACT_UCHAR(t1)==T_INT && EXTRACT_UCHAR(t2)==T_INT)
  {
    INT32 i1,i2;
    i1=extract_type_int(t1+1+sizeof(INT32));
    i2=extract_type_int(t2+1+sizeof(INT32));
    push_type_int(MAXIMUM(i1,i2));

    i1=extract_type_int(t1+1);
    i2=extract_type_int(t2+1);
    push_type_int(MINIMUM(i1,i2));

    push_type(T_INT);
  }
  else
  {
    push_unfinished_type(t1);
    very_low_or_pike_types(t2,t1);
  }
}

static void medium_or_pike_types(struct pike_string *a,
				 struct pike_string *b)
{
  low_or_pike_types( a ? a->str : 0 , b ? b->str : 0 );
}

struct pike_string *or_pike_types(struct pike_string *a,
				  struct pike_string *b)
{
  type_stack_mark();
  medium_or_pike_types(a,b);
  return pop_unfinished_type();
}

static struct pike_string *low_object_lfun_type(char *t, short lfun)
{
  struct program *p;
  int i;
  p=id_to_program(extract_type_int(t+2));
  if(!p) return 0;
  i=FIND_LFUN(p, lfun);
  if(i==-1) return 0;
  return ID_FROM_INT(p, i)->type;
}

#define A_EXACT 1
#define B_EXACT 2
#define NO_MAX_ARGS 4
#define NO_SHORTCUTS 8

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
    if(ret && !(flags & NO_SHORTCUTS)) return ret;
    a+=type_length(a);
    if(ret)
    {
      low_match_types(a,b,flags);
      return ret;
    }else{
      return low_match_types(a,b,flags);
    }

  case T_NOT:
    if(low_match_types(a+1,b,(flags ^ B_EXACT ) | NO_MAX_ARGS))
      return 0;
    return a;

    case T_ASSIGN:
      ret=low_match_types(a+2,b,flags);
      if(ret && EXTRACT_UCHAR(b)!=T_VOID)
      {
	int m=EXTRACT_UCHAR(a+1)-'0';
	type_stack_mark();
	low_or_pike_types(a_markers[m] ? a_markers[m]->str : 0,b);
	if(a_markers[m]) free_string(a_markers[m]);
	a_markers[m]=pop_unfinished_type();
      }
      return ret;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      int m=EXTRACT_UCHAR(a)-'0';
      if(a_markers[m])
	return low_match_types(a_markers[m]->str, b, flags);
      else
	return low_match_types(mixed_type_string->str, b, flags);
    }
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
    if(ret && !(flags & NO_SHORTCUTS)) return ret;
    b+=type_length(b);
    if(ret)
    {
      low_match_types(a,b,flags);
      return ret;
    }else{
      return low_match_types(a,b,flags);
    }

  case T_NOT:
    if(low_match_types(a,b+1, (flags ^ A_EXACT ) | NO_MAX_ARGS))
      return 0;
    return a;

    case T_ASSIGN:
      ret=low_match_types(a,b+2,flags);
      if(ret && EXTRACT_UCHAR(a)!=T_VOID)
      {
	int m=EXTRACT_UCHAR(b+1)-'0';
	type_stack_mark();
	low_or_pike_types(b_markers[m] ? b_markers[m]->str : 0,a);
	if(b_markers[m]) free_string(b_markers[m]);
	b_markers[m]=pop_unfinished_type();
      }
      return ret;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      int m=EXTRACT_UCHAR(b)-'0';
      if(b_markers[m])
	return low_match_types(a, b_markers[m]->str, flags);
      else
	return low_match_types(a, mixed_type_string->str, flags);
    }
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
    struct pike_string *s;
    if((s=low_object_lfun_type(a, LFUN_CALL)))
       return low_match_types(s->str,b,flags);
    return a;
  }

  case TWOT(T_FUNCTION, T_OBJECT):
  {
    struct pike_string *s;
    if((s=low_object_lfun_type(b, LFUN_CALL)))
       return low_match_types(a,s->str,flags);
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

      if(!low_match_types(a_tmp, b_tmp, flags | NO_MAX_ARGS)) return 0;
      if(++correct_args > max_correct_args)
	if(!(flags & NO_MAX_ARGS))
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
      if(!low_match_types(a,b,flags | NO_MAX_ARGS)) return 0;
    }
    if(!(flags & NO_MAX_ARGS))
       max_correct_args=0x7fffffff;
    /* check the returntype */
    if(!low_match_types(a,b,flags)) return 0;
    break;

  case T_MAPPING:
    if(!low_match_types(++a,++b,flags)) return 0;
    if(!low_match_types(a+type_length(a),b+type_length(b),flags)) return 0;
    break;

  case T_OBJECT:
#if 0
    if(extract_type_int(a+2) || extract_type_int(b+2))
    {
      fprintf(stderr,"Type match1: ");
      stupid_describe_type(a,type_length(a));
      fprintf(stderr,"Type match2: ");
      stupid_describe_type(b,type_length(b));
    }
#endif

    /* object(* 0) matches any object */
    if(!extract_type_int(a+2) || !extract_type_int(b+2)) break;

    /* object(x *) =? object(x *) */
    if(EXTRACT_UCHAR(a+1) == EXTRACT_UCHAR(b+1))
    {
      /* x? */
      if(EXTRACT_UCHAR(a+1))
      {
	/* object(1 x) =? object(1 x) */
	if(extract_type_int(a+2) != extract_type_int(b+2)) return 0;
      }else{
	/* object(0 *) =? object(0 *) */
	break;
      }
    }

    {
      struct program *ap,*bp;
      ap=id_to_program(extract_type_int(a+2));
      bp=id_to_program(extract_type_int(b+2));

      if(!ap || !bp) break;

      if(EXTRACT_UCHAR(a+1))
      {
	if(!implements(ap,bp))
	  return 0;
      }else{
	if(!implements(bp,ap))
	  return 0;
      }
    }
    
    break;

  case T_INT:
  {
    INT32 amin=extract_type_int(a+1);
    INT32 amax=extract_type_int(a+1+sizeof(INT32));

    INT32 bmin=extract_type_int(b+1);
    INT32 bmax=extract_type_int(b+1+sizeof(INT32));
    
    if(amin > bmax || bmin > amax) return 0;
    break;
  }
    

  case T_MULTISET:
  case T_ARRAY:
    if(!low_match_types(++a,++b,flags)) return 0;

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

      if(!o1 && !o2) return 0;

      medium_or_pike_types(o1,o2);

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

  a=low_match_types(a,b,NO_SHORTCUTS);
  if(a)
  {
    switch(EXTRACT_UCHAR(a))
    {
    case T_FUNCTION:
      a++;
      while(EXTRACT_UCHAR(a)!=T_MANY) a+=type_length(a);
      a++;
      a+=type_length(a);
      push_unfinished_type_with_markers(a, a_markers );
      return 1;

    case T_PROGRAM:
      push_type_int(0);
      push_type(0);
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
  check_type_string(a);
  check_type_string(b);
  clear_markers();
  return 0!=low_match_types(a->str, b->str,0);
}


#ifdef DEBUG_MALLOC
#define low_index_type(X,Y) ((struct pike_string *)debug_malloc_touch(debug_low_index_type((X),(Y))))
#else
#define low_index_type debug_low_index_type
#endif

/* FIXME, add the index */
static struct pike_string *debug_low_index_type(char *t, node *n)
{
  struct program *p;
  switch(EXTRACT_UCHAR(t++))
  {
  case T_OBJECT:
  {
    p=id_to_program(extract_type_int(t+1));

  comefrom_int_index:
    if(p && n)
    {
      if(n->token == F_ARROW)
      {
	if(FIND_LFUN(p,LFUN_ARROW)!=-1 || FIND_LFUN(p,LFUN_ASSIGN_ARROW)!=-1)
	{
	  reference_shared_string(mixed_type_string);
	  return mixed_type_string;
	}
      }else{
	if(FIND_LFUN(p,LFUN_INDEX) != -1 || FIND_LFUN(p,LFUN_ASSIGN_INDEX) != -1)
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
	  reference_shared_string(mixed_type_string);
	  return mixed_type_string;
	}else{
	  if(EXTRACT_UCHAR(t) ||
	     (p->identifier_references[i].id_flags & ID_NOMASK) ||
	    (ID_FROM_INT(p, i)->identifier_flags & IDENTIFIER_PROTOTYPED))
	  {
	    reference_shared_string(ID_FROM_INT(p, i)->type);
	    return ID_FROM_INT(p, i)->type;
	  }else{
	    reference_shared_string(mixed_type_string);
	    return mixed_type_string;
	  }
	}	   
      }
    }
  }
  default:
    reference_shared_string(mixed_type_string);
    return mixed_type_string;

    case T_INT:
#ifdef AUTO_BIGNUM
      /* Don't force Gmp.mpz to be loaded here since this function
       * is called long before the master object is compiled...
       * /Hubbe
       */
      p=get_auto_bignum_program_or_zero();
      goto comefrom_int_index;
#endif
    case T_VOID:
    case T_FLOAT:
      return 0;

  case T_OR:
  {
    struct pike_string *a,*b;
    a=low_index_type(t,n);
    t+=type_length(t);
    b=low_index_type(t,n);
    if(!b) return a;
    if(!a) return b;
    type_stack_mark();
    medium_or_pike_types(a,b);
    free_string(a);
    free_string(b);
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
    if(n && low_match_types(string_type_string->str,CDR(n)->type->str,0))
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
  clear_markers();
  t=low_index_type(type->str,n);
  if(!t) copy_shared_string(t,mixed_type_string);
  return t;
}


#ifdef DEBUG_MALLOC
#define low_key_type(X,Y) ((struct pike_string *)debug_malloc_touch(debug_low_key_type((X),(Y))))
#else
#define low_key_type debug_low_key_type
#endif

/* FIXME, add the index */
static struct pike_string *debug_low_key_type(char *t, node *n)
{
  switch(EXTRACT_UCHAR(t++))
  {
  case T_OBJECT:
  {
    struct program *p=id_to_program(extract_type_int(t+1));
    if(p && n)
    {
      if(n->token == F_ARROW)
      {
	if(FIND_LFUN(p,LFUN_ARROW)!=-1 || FIND_LFUN(p,LFUN_ASSIGN_ARROW)!=-1)
	{
	  reference_shared_string(mixed_type_string);
	  return mixed_type_string;
	}
      }else{
	if(FIND_LFUN(p,LFUN_INDEX) != -1 || FIND_LFUN(p,LFUN_ASSIGN_INDEX) != -1)
	{
	  reference_shared_string(mixed_type_string);
	  return mixed_type_string;
	}
      }
    }
    reference_shared_string(string_type_string);
    return string_type_string;
  }
  default:
    reference_shared_string(mixed_type_string);
    return mixed_type_string;

    case T_VOID:
    case T_FLOAT:
    case T_INT:
      return 0;

  case T_OR:
  {
    struct pike_string *a,*b;
    a=low_key_type(t,n);
    t+=type_length(t);
    b=low_key_type(t,n);
    if(!b) return a;
    if(!a) return b;
    type_stack_mark();
    medium_or_pike_types(a,b);
    free_string(a);
    free_string(b);
    return pop_unfinished_type();
  }

  case T_AND:
    return low_key_type(t+type_length(t),n);

  case T_ARRAY:
  case T_STRING: /* always int */
    reference_shared_string(int_type_string);
    return int_type_string;

  case T_MAPPING:
  case T_MULTISET:
    return make_shared_binary_string(t, type_length(t));
  }
}

struct pike_string *key_type(struct pike_string *type, node *n)
{
  struct pike_string *t;
  clear_markers();
  t=low_key_type(type->str,n);
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
    struct program *p=id_to_program(extract_type_int(type+1));
    if(p)
    {
      if(n->token == F_ARROW)
      {
	if(FIND_LFUN(p,LFUN_ARROW)!=-1 || FIND_LFUN(p,LFUN_ASSIGN_ARROW)!=-1)
	  return 1;
      }else{
	if(FIND_LFUN(p,LFUN_INDEX)!=-1 || FIND_LFUN(p,LFUN_ASSIGN_INDEX)!=-1)
	  return 1;
      }
      return !!low_match_types(string_type_string->str, index_type,0);
    }else{
      return 1;
    }
  }

  case T_MULTISET:
  case T_MAPPING:
#if 0
    return !!low_match_types(type,index_type,0);
#else
    /* FIXME: Compiler warning here!!!! */
    return 1;
#endif

#ifdef AUTO_BIGNUM
    case T_INT:
#endif
    case T_PROGRAM:
      return !!low_match_types(string_type_string->str, index_type,0);

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
  check_type_string(type);
  check_type_string(index_type);

  return low_check_indexing(type->str, index_type->str, n);
}

static int low_count_arguments(char *q)
{
  int num,num2;

  switch(EXTRACT_UCHAR(q++))
  {
    case T_OR:
      num=low_count_arguments(q);
      num2=low_count_arguments(q+type_length(q));
      if(num<0 && num2>0) return num;
      if(num2<0 && num>0) return num2;
      if(num2<0 && num<0) return ~num>~num2?num:num2;
      return num>num2?num:num2;

    case T_AND:
      num=low_count_arguments(q);
      num2=low_count_arguments(q+type_length(q));
      if(num<0 && num2>0) return num2;
      if(num2<0 && num>0) return num;
      if(num2<0 && num<0) return ~num<~num2?num:num2;
      return num<num2?num:num2;

    default: return 0x7fffffff;

    case T_FUNCTION:
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
}

/* Count the number of arguments for a funciton type.
 * return -1-n if the function can take number of arguments
 * >= n  (varargs)
 */
int count_arguments(struct pike_string *s)
{
  check_type_string(s);

  return low_count_arguments(s->str);
}


static int low_minimum_arguments(char *q)
{
  int num;

  switch(EXTRACT_UCHAR(q++))
  {
    case T_OR:
    case T_AND:
      return MAXIMUM(low_count_arguments(q),
		     low_count_arguments(q+type_length(q)));

    default: return 0;

    case T_FUNCTION:
      num=0;
      while(EXTRACT_UCHAR(q)!=T_MANY)
      {
	if(low_match_types(void_type_string->str, q, B_EXACT))
	  return num;

	num++;
	q+=type_length(q);
      }
      return num;
  }
}

/* Count the minimum number of arguments for a funciton type.
 */
int minimum_arguments(struct pike_string *s)
{
  int ret;
  check_type_string(s);

  ret=low_minimum_arguments(s->str);

#if 0
  fprintf(stderr,"minimum_arguments(");
  simple_describe_type(s);
  fprintf(stderr," ) -> %d\n",ret);
#endif

  return ret;
}

struct pike_string *check_call(struct pike_string *args,
			       struct pike_string *type)
{
  check_type_string(args);
  check_type_string(type);
  clear_markers();
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

INT32 get_max_args(struct pike_string *type)
{
  INT32 ret,tmp=max_correct_args;
  check_type_string(type);
  clear_markers();
  type=check_call(function_type_string, type);
  if(type) free_string(type);
  ret=max_correct_args;
  max_correct_args=tmp;
  return tmp;
}


struct pike_string *zzap_function_return(char *a, INT32 id)
{
  switch(EXTRACT_UCHAR(a))
  {
    case T_OR:
    {
      struct pike_string *ar, *br, *ret=0;
      a++;
      ar=zzap_function_return(a,id);
      br=zzap_function_return(a+type_length(a),id);
      if(ar && br) ret=or_pike_types(ar,br);
      if(ar) free_string(ar);
      if(br) free_string(br);
      return ret;
    }
      
    case T_FUNCTION:
      type_stack_mark();
      push_type_int(id);
      push_type(1);
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
  fatal("zzap_function_return() called with unexpected value: %d\n",
	EXTRACT_UCHAR(a));
  /* NOT_REACHED */
  return NULL;
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
#ifdef AUTO_BIGNUM
      if(is_bignum_object(s->u.object))
      {
	push_type_int(MAX_INT32);
	push_type_int(MIN_INT32);
	push_type(T_INT);
      }
      else
#endif
      {
	push_type_int(s->u.object->prog->id);
	push_type(1);
	push_type(T_OBJECT);
      }
    }else{
      push_type_int(0);
      push_type(0);
      push_type(T_OBJECT);
    }
    return pop_unfinished_type();

  case T_INT:
    if(s->u.integer)
    {
      type_stack_mark();
      /* Fixme, check that the integer is in range of MIN_INT32 .. MAX_INT32!
       */
      push_type_int(s->u.integer);
      push_type_int(s->u.integer);
      push_type(T_INT);
      return pop_unfinished_type();
    }else{
      ret=mixed_type_string;
    }
    reference_shared_string(ret);
    return ret;

  case T_PROGRAM:
  {
    char *a;
    struct pike_string *tmp;
    int id=FIND_LFUN(s->u.program,LFUN_CREATE);
    if(id>=0)
    {
      a=ID_FROM_INT(s->u.program, id)->type->str;
    }else{
      a=function_type_string->str;
    }
    if((tmp=zzap_function_return(a, s->u.program->id)))
      return tmp;
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


int type_may_overload(char *type, int lfun)
{
  switch(EXTRACT_UCHAR(type++))
  {
    case T_ASSIGN:
      return type_may_overload(type+1,lfun);
      
    case T_FUNCTION:
    case T_ARRAY:
      /* might want to check for `() */
      
    default:
      return 0;

    case T_OR:
      return type_may_overload(type,lfun) ||
	type_may_overload(type+type_length(type),lfun);
      
    case T_AND:
      return type_may_overload(type,lfun) &&
	type_may_overload(type+type_length(type),lfun);
      
    case T_NOT:
      return !type_may_overload(type,lfun);

    case T_MIXED:
      return 1;
      
    case T_OBJECT:
    {
      struct program *p=id_to_program(extract_type_int(type+1));
      if(!p) return 1;
      return FIND_LFUN(p, lfun)!=-1;
    }
  }
}
