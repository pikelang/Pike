/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: builtin_functions.c,v 1.262 2000/04/16 22:53:18 hubbe Exp $");
#include "interpret.h"
#include "svalue.h"
#include "pike_macros.h"
#include "object.h"
#include "program.h"
#include "array.h"
#include "error.h"
#include "constants.h"
#include "mapping.h"
#include "stralloc.h"
#include "multiset.h"
#include "pike_types.h"
#include "rusage.h"
#include "operators.h"
#include "fsort.h"
#include "callback.h"
#include "gc.h"
#include "backend.h"
#include "main.h"
#include "pike_memory.h"
#include "threads.h"
#include "time_stuff.h"
#include "version.h"
#include "encode.h"
#include <math.h>
#include <ctype.h>
#include "module_support.h"
#include "module.h"
#include "opcodes.h"
#include "cyclic.h"
#include "signal_handler.h"
#include "security.h"
#include "builtin_functions.h"
#include "bignum.h"
#include "language.h"

#ifdef HAVE_POLL
#ifdef HAVE_POLL_H
#include <poll.h>
#endif /* HAVE_POLL_H */

#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif /* HAVE_SYS_POLL_H */
#endif /* HAVE_POLL */

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

/* #define DIFF_DEBUG */
/* #define ENABLE_DYN_DIFF */

void f_equal(INT32 args)
{
  int i;
  if(args != 2)
    PIKE_ERROR("equal", "Bad number of arguments.\n", sp, args);

  i=is_equal(sp-2,sp-1);
  pop_n_elems(args);
  push_int(i);
}

#ifdef DEBUG_MALLOC
void _f_aggregate(INT32 args)
#else
void f_aggregate(INT32 args)
#endif
{
  struct array *a;
#ifdef PIKE_DEBUG
  if(args < 0) fatal("Negative args to f_aggregate() (%d)\n",args);
#endif

  a=aggregate_array(args);
  push_array(a); /* beware, macro */
}

void f_trace(INT32 args)
{
  extern int t_flag;
  INT_TYPE t;

  get_all_args("trace", args, "%i", &t);
  pop_n_elems(args);
  push_int(t_flag);
  t_flag = t;
}

void f_hash(INT32 args)
{
  INT32 i;
  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("hash",1);
  if(sp[-args].type != T_STRING)
    SIMPLE_BAD_ARG_ERROR("hash", 1, "string");

  switch(sp[-args].u.string->size_shift)
  {
    case 0:
      i=hashstr((unsigned char *)sp[-args].u.string->str,100);
      break;

    case 1:
      i=simple_hashmem((unsigned char *)sp[-args].u.string->str,
		       sp[-args].u.string->len << 1,
		       200);
      break;

    case 2:
      i=simple_hashmem((unsigned char *)sp[-args].u.string->str,
		       sp[-args].u.string->len << 2,
		       400);
      break;

    default:
      fatal("hash(): Bad string shift:%d\n", sp[-args].u.string->size_shift);
  }
  
  if(args > 1)
  {
    if(sp[1-args].type != T_INT)
      SIMPLE_BAD_ARG_ERROR("hash",2,"int");
    
    if(!sp[1-args].u.integer)
      PIKE_ERROR("hash", "Modulo by zero.\n", sp, args);

    i%=(unsigned INT32)sp[1-args].u.integer;
  }
  pop_n_elems(args);
  push_int(i);
}

void f_copy_value(INT32 args)
{
  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("copy_value",1);

  pop_n_elems(args-1);
  copy_svalues_recursively_no_free(sp,sp-1,1,0);
  free_svalue(sp-1);
  sp[-1]=sp[0];
  dmalloc_touch_svalue(sp-1);
}

void f_ctime(INT32 args)
{
  time_t i;
  INT_TYPE x;
  get_all_args("ctime",args,"%i",&x);
  i=(time_t)x;
  pop_n_elems(args);
  push_string(make_shared_string(ctime(&i)));
}

struct case_info {
  int low;	/* low end of range. */
  int mode;
  int data;
};

#define CIM_NONE	0	/* Case-less */
#define CIM_UPPERDELTA	1	/* Upper-case, delta to lower-case in data */
#define CIM_LOWERDELTA	2	/* Lower-case, -delta to upper-case in data */
#define CIM_CASEBIT	3	/* Some case, case mask in data */
#define CIM_CASEBITOFF	4	/* Same as above, but also offset by data */

static struct case_info case_info[] = {
#ifdef IN_TPIKE
#include "dummy_ci.h"
#else /* !IN_TPIKE */
#include "case_info.h"
#endif /* IN_TPIKE */
  { 0x10000, CIM_NONE, 0x0000, },	/* End sentinel. */
};

static struct case_info *find_ci(int c)
{
  static struct case_info *cache = NULL;
  struct case_info *ci = cache;
  int lo = 0;
  int hi = NELEM(case_info);

  if ((c < 0) || (c > 0xffff))
    return NULL;

  if ((ci) && (ci[0].low <= c) && (ci[1].low > c)) {
    return ci;
  }

  while (lo != hi-1) {
    int mid = (lo + hi)/2;
    if (case_info[mid].low < c) {
      lo = mid;
    } else if (case_info[mid].low == c) {
      lo = mid;
      break;
    } else {
      hi = mid;
    }
  }

  return(cache = case_info + lo);
}

#define DO_LOWER_CASE(C) do {\
    int c = C; \
    struct case_info *ci = find_ci(c); \
    if (ci) { \
      switch(ci->mode) { \
      case CIM_NONE: case CIM_LOWERDELTA: break; \
      case CIM_UPPERDELTA: C = c + ci->data; break; \
      case CIM_CASEBIT: C = c | ci->data; break; \
      case CIM_CASEBITOFF: C = ((c - ci->data) | ci->data) + ci->data; break; \
      default: fatal("lower_case(): Unknown case_info mode: %d\n", ci->mode); \
    } \
   } \
  } while(0)

#define DO_UPPER_CASE(C) do {\
    int c = C; \
    struct case_info *ci = find_ci(c); \
    if (ci) { \
      switch(ci->mode) { \
      case CIM_NONE: case CIM_UPPERDELTA: break; \
      case CIM_LOWERDELTA: C = c - ci->data; break; \
      case CIM_CASEBIT: C = c & ~ci->data; break; \
      case CIM_CASEBITOFF: C = ((c - ci->data)& ~ci->data) + ci->data; break; \
      default: fatal("lower_case(): Unknown case_info mode: %d\n", ci->mode); \
    } \
   } \
  } while(0)

void f_lower_case(INT32 args)
{
  INT32 i;
  struct pike_string *orig;
  struct pike_string *ret;
  get_all_args("lower_case", args, "%W", &orig);

  ret = begin_wide_shared_string(orig->len, orig->size_shift);

  MEMCPY(ret->str, orig->str, orig->len << orig->size_shift);

  i = orig->len;

  if (!orig->size_shift) {
    p_wchar0 *str = STR0(ret);

    while(i--) {
      DO_LOWER_CASE(str[i]);
    }
  } else if (orig->size_shift == 1) {
    p_wchar1 *str = STR1(ret);

    while(i--) {
      DO_LOWER_CASE(str[i]);
    }
  } else if (orig->size_shift == 2) {
    p_wchar2 *str = STR2(ret);

    while(i--) {
      DO_LOWER_CASE(str[i]);
    }
  } else {
    fatal("lower_case(): Bad string shift:%d\n", orig->size_shift);
  }

  pop_n_elems(args);
  push_string(end_shared_string(ret));
}

void f_upper_case(INT32 args)
{
  INT32 i;
  struct pike_string *orig;
  struct pike_string *ret;
  int widen = 0;
  get_all_args("upper_case",args,"%W",&orig);

  ret=begin_wide_shared_string(orig->len,orig->size_shift);
  MEMCPY(ret->str, orig->str, orig->len << orig->size_shift);

  i = orig->len;

  if (!orig->size_shift) {
    p_wchar0 *str = STR0(ret);

    while(i--) {
      if (str[i] != 0xff) {
	DO_UPPER_CASE(str[i]);
      } else {
	widen = 1;
      }
    }
  } else if (orig->size_shift == 1) {
    p_wchar1 *str = STR1(ret);

    while(i--) {
      DO_UPPER_CASE(str[i]);
    }
  } else if (orig->size_shift == 2) {
    p_wchar2 *str = STR2(ret);

    while(i--) {
      DO_UPPER_CASE(str[i]);
    }
  } else {
    fatal("lower_case(): Bad string shift:%d\n", orig->size_shift);
  }

  pop_n_elems(args);
  push_string(end_shared_string(ret));

  if (widen) {
    /* Widen the string, and replace any 0xff's with 0x178's. */
    orig = sp[-1].u.string;
    ret = begin_wide_shared_string(orig->len, 1);

    i = orig->len;

    while(i--) {
      if ((STR1(ret)[i] = STR0(orig)[i]) == 0xff) {
	STR1(ret)[i] = 0x178;
      }
    }
    free_string(sp[-1].u.string);
    sp[-1].u.string = end_shared_string(ret);
  }
}

void f_random(INT32 args)
{
  INT_TYPE i;

  if(args && (sp[-args].type == T_OBJECT))
  {
    pop_n_elems(args-1);
    apply(sp[-1].u.object,"_random",0);
    stack_swap();
    pop_stack();
    return;
  }

  get_all_args("random",args,"%i",&i);

  if(i <= 0)
  {
    i = 0;
  }else{
    i = my_rand() % i;
  }
  pop_n_elems(args);
  push_int(i);
}

void f_random_string(INT32 args)
{
  struct pike_string *ret;
  INT32 e,len;
  get_all_args("random_string",args,"%i",&len);
  ret = begin_shared_string(len);
  for(e=0;e<len;e++) ret->str[e]=my_rand();
  pop_n_elems(args);
  push_string(end_shared_string(ret));
}

void f_random_seed(INT32 args)
{
  INT_TYPE i;
#ifdef AUTO_BIGNUM
  check_all_args("random_seed",args,BIT_INT | BIT_OBJECT, 0);
  if(sp[-args].type == T_INT)
  {
    i=sp[-args].u.integer;
  }else{
    i=hash_svalue(sp-args);
  }
#else
  get_all_args("random_seed",args,"%i",&i);
#endif
  my_srand(i);
  pop_n_elems(args);
}

void f_query_num_arg(INT32 args)
{
  pop_n_elems(args);
  push_int(fp ? fp->args : 0);
}

void f_search(INT32 args)
{
  INT32 start;

  if(args < 2)
    SIMPLE_TOO_FEW_ARGS_ERROR("search", 2);

  switch(sp[-args].type)
  {
  case T_STRING:
  {
    char *ptr;
    if(sp[1-args].type != T_STRING)
      SIMPLE_BAD_ARG_ERROR("search", 2, "string");
    
    start=0;
    if(args > 2)
    {
      if(sp[2-args].type!=T_INT)
	SIMPLE_BAD_ARG_ERROR("search", 3, "int");

      start=sp[2-args].u.integer;
      if(start<0) {
	bad_arg_error("search", sp-args, args, 3, "int(0..)", sp+2-args,
		   "Start must be greater or equal to zero.\n");
      }
    }

    if(sp[-args].u.string->len < start)
      bad_arg_error("search", sp-args, args, 1, "int(0..)", sp-args,
		    "Start must not be greater than the "
		    "length of the string.\n");

    start=string_search(sp[-args].u.string,
			sp[1-args].u.string,
			start);
    pop_n_elems(args);
    push_int(start);
    break;
  }

  case T_ARRAY:
    start=0;
    if(args > 2)
    {
      if(sp[2-args].type!=T_INT)
	SIMPLE_BAD_ARG_ERROR("search", 3, "int");

      start=sp[2-args].u.integer;
      if(start<0) {
	bad_arg_error("search", sp-args, args, 3, "int(0..)", sp+2-args,
		   "Start must be greater or equal to zero.\n");
      }
    }
    start=array_search(sp[-args].u.array,sp+1-args,start);
    pop_n_elems(args);
    push_int(start);
    break;

  case T_MAPPING:
    if(args > 2) {
      mapping_search_no_free(sp,sp[-args].u.mapping,sp+1-args,sp+2-args);
    } else {
      mapping_search_no_free(sp,sp[-args].u.mapping,sp+1-args,0);
    }
    free_svalue(sp-args);
    sp[-args]=*sp;
    dmalloc_touch_svalue(sp);
    pop_n_elems(args-1);
    return;

  default:
    SIMPLE_BAD_ARG_ERROR("search", 1, "string|array|mapping");
  }
}

/* int has_prefix(string a, string prefix) */
void f_has_prefix(INT32 args)
{
  struct pike_string *a, *b;

  get_all_args("has_prefix", args, "%W%W", &a, &b);

  /* First handle some common special cases. */
  if ((b->len > a->len) || (b->size_shift > a->size_shift)) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  /* Trivial cases. */
  if ((a == b)||(!b->len)) {
    pop_n_elems(args);
    push_int(1);
    return;
  }

  if (a->size_shift == b->size_shift) {
    int res = !MEMCMP(a->str, b->str, b->len << b->size_shift);
    pop_n_elems(args);
    push_int(res);
    return;
  }

  /* At this point a->size_shift > b->size_shift */
#define TWO_SHIFTS(S1, S2)	((S1)|((S2)<<2))
  switch(TWO_SHIFTS(a->size_shift, b->size_shift)) {
#define CASE_SHIFT(S1, S2) \
  case TWO_SHIFTS(S1, S2): \
    { \
      PIKE_CONCAT(p_wchar,S1) *s1 = PIKE_CONCAT(STR,S1)(a); \
      PIKE_CONCAT(p_wchar,S2) *s2 = PIKE_CONCAT(STR,S2)(b); \
      int len = b->len; \
      while(len-- && (s1[len] == s2[len])) \
	; \
      pop_n_elems(args); \
      push_int(len == -1); \
      return; \
    } \
    break

    CASE_SHIFT(1,0);
    CASE_SHIFT(2,0);
    CASE_SHIFT(2,1);
  default:
    error("has_index(): Unexpected string shift combination: a:%d, b:%d!\n",
	  a->size_shift, b->size_shift);
    break;
  }
#undef CASE_SHIFT
#undef TWO_SHIFTS
}

void f_has_index(INT32 args)
{
  int t = 0;
  
  if(args != 2)
    PIKE_ERROR("has_index", "Bad number of arguments.\n", sp, args);

  switch(sp[-2].type)
  {
    case T_STRING:
      if(sp[-1].type == T_INT)
	t = (0 <= sp[-1].u.integer && sp[-1].u.integer < sp[-2].u.string->len);
  
      pop_n_elems(args);
      push_int(t);
      break;
      
    case T_ARRAY:
      if(sp[-1].type == T_INT)
	t = (0 <= sp[-1].u.integer && sp[-1].u.integer < sp[-2].u.array->size);
      
      pop_n_elems(args);
      push_int(t);
      break;
      
    case T_MULTISET:
    case T_MAPPING:
      f_index(2);
      f_zero_type(1);
      
      if(sp[-1].type == T_INT)
	sp[-1].u.integer = !sp[-1].u.integer;
      else
	PIKE_ERROR("has_index",
		   "Function `zero_type' gave incorrect result.\n", sp, args);
      break;
      
    case T_OBJECT:
      /* FIXME: If the object behaves like an array, it will throw an
	 error for non-valid indices. Therefore it's not a good idea
	 to use the index operator.

	 Maybe we should use object->_has_index(index) provided that
	 the object implements it.
	 
	 /Noring */

      /* Fall-through. */
      
    default:
      stack_swap();
      f_indices(1);
      stack_swap();
      f_search(2);
      
      if(sp[-1].type == T_INT)
	sp[-1].u.integer = (sp[-1].u.integer != -1);
      else
	PIKE_ERROR("has_index",
		   "Function `search' gave incorrect result.\n", sp, args);
  }
}

void f_has_value(INT32 args)
{
  if(args != 2)
    PIKE_ERROR("has_value", "Bad number of arguments.\n", sp, args);

  switch(sp[-2].type)
  {
    case T_MAPPING:
      f_search(2);
      f_zero_type(1);
      
      if(sp[-1].type == T_INT)
	sp[-1].u.integer = !sp[-1].u.integer;
      else
	PIKE_ERROR("has_value",
		   "Function `zero_type' gave incorrect result.\n", sp, args);
      break;
      
    case T_OBJECT:
      /* FIXME: It's very sad that we always have to do linear search
	 with `values' in case of objects. The problem is that we cannot
	 use `search' directly since it's undefined weather it returns
	 -1 (array) or 0 (mapping) during e.g. some data type emulation.
	 
	 Maybe we should use object->_has_value(value) provided that
	 the object implements it.
	 
	 /Noring */

      /* Fall-through. */
      
    default:
      stack_swap();
      f_values(1);
      stack_swap();

    case T_STRING:   /* Strings are odd. /Noring */
    case T_ARRAY:
      f_search(2);

      if(sp[-1].type == T_INT)
	sp[-1].u.integer = (sp[-1].u.integer != -1);
      else
	PIKE_ERROR("has_value", "Search gave incorrect result.\n", sp, args);
  }
}

/* Old backtrace */

void f_backtrace(INT32 args)
{
  INT32 frames;
  struct pike_frame *f,*of;
  struct array *a,*i;

  frames=0;
  if(args) pop_n_elems(args);
  for(f=fp;f;f=f->next) frames++;

  sp->type=T_ARRAY;
  sp->u.array=a=allocate_array_no_init(frames,0);
  sp++;

  /* NOTE: The first pike_frame is ignored, since it is the call to backtrace(). */
  of=0;
  for(f=fp;f;f=(of=f)->next)
  {
    char *program_name;

    debug_malloc_touch(f);
    frames--;

    if(f->current_object && f->context.prog)
    {
      INT32 args;
      args=f->num_args;
      args=MINIMUM(f->num_args, sp - f->locals);
      if(of)
	args=MINIMUM(f->num_args, of->locals - f->locals);
      args=MAXIMUM(args,0);

      ITEM(a)[frames].u.array=i=allocate_array_no_init(3+args,0);
      ITEM(a)[frames].type=T_ARRAY;
      assign_svalues_no_free(ITEM(i)+3, f->locals, args, BIT_MIXED);
      if(f->current_object->prog)
      {
	ITEM(i)[2].type=T_FUNCTION;
	ITEM(i)[2].subtype=f->fun;
	ITEM(i)[2].u.object=f->current_object;
	add_ref(f->current_object);
      }else{
	ITEM(i)[2].type=T_INT;
	ITEM(i)[2].subtype=NUMBER_DESTRUCTED;
	ITEM(i)[2].u.integer=0;
      }

      if(f->pc)
      {
	program_name=get_line(f->pc, f->context.prog, & ITEM(i)[1].u.integer);
	ITEM(i)[1].subtype=NUMBER_NUMBER;
	ITEM(i)[1].type=T_INT;

	ITEM(i)[0].u.string=make_shared_string(program_name);
#ifdef __CHECKER__
	ITEM(i)[0].subtype=0;
#endif
	ITEM(i)[0].type=T_STRING;
      }else{
	ITEM(i)[1].u.integer=0;
	ITEM(i)[1].subtype=NUMBER_NUMBER;
	ITEM(i)[1].type=T_INT;

	ITEM(i)[0].u.integer=0;
	ITEM(i)[0].subtype=NUMBER_NUMBER;
	ITEM(i)[0].type=T_INT;
      }
    }else{
      ITEM(a)[frames].type=T_INT;
      ITEM(a)[frames].u.integer=0;
    }
  }
  a->type_field = BIT_ARRAY | BIT_INT;
}

void f_add_constant(INT32 args)
{
  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY, ("add_constant: permission denied.\n"));
  if(args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("add_constant", 1);

  if(sp[-args].type!=T_STRING)
    SIMPLE_BAD_ARG_ERROR("add_constant", 1, "string");

  if(args>1)
  {
    dmalloc_touch_svalue(sp-args+1);
    low_add_efun(sp[-args].u.string, sp-args+1);
  }else{
    low_add_efun(sp[-args].u.string, 0);
  }
  pop_n_elems(args);
}

#ifndef __NT__
#define IS_SEP(X) ( (X)=='/' )
#define IS_ABS(X) (IS_SEP((X)[0])?1:0)
#else   

#define IS_SEP(X) ( (X) == '/' || (X) == '\\' )

static int find_absolute(char *s)
{
  if(isalpha(s[0]) && s[1]==':' && IS_SEP(s[2]))
    return 3;

  if(IS_SEP(s[0]) && IS_SEP(s[1]))
  {
    int l;
    for(l=2;isalpha(s[l]);l++);
    return l;
  }

  return 0;
}
#define IS_ABS(X) find_absolute((X))

#define IS_ROOT(X) (IS_SEP((X)[0])?1:0)
#endif

static char *combine_path(char *cwd,char *file)
{
  /* cwd is supposed to be combined already */
  char *ret;
  register char *from,*to;
  char *my_cwd;
  char cwdbuf[10];
  int tmp;

  my_cwd=0; 

  if((tmp=IS_ABS(file)))
  {
    MEMCPY(cwdbuf,file,tmp);
    cwdbuf[tmp]=0;
    cwd=cwdbuf;
    file+=tmp;
  }

#ifdef IS_ROOT
  else if(IS_ROOT(file))
  {
    if(tmp=IS_ABS(cwd))
    {
      MEMCPY(cwdbuf,cwd,tmp);
      cwdbuf[tmp]=0;
      cwd=cwdbuf;
      file+=IS_ROOT(file);
    }else{
      MEMCPY(cwdbuf,file,IS_ROOT(file));
      cwdbuf[IS_ROOT(file)]=0;
      cwd=cwdbuf;
      file+=IS_ROOT(file);
    }
  }
#endif

#ifdef PIKE_DEBUG    
  if(!cwd)
    fatal("No cwd in combine_path!\n");
#endif

  if(!*cwd || IS_SEP(cwd[strlen(cwd)-1]))
  {
    ret=(char *)xalloc(strlen(cwd)+strlen(file)+1);
    strcpy(ret,cwd);
    strcat(ret,file);
  }else{
    ret=(char *)xalloc(strlen(cwd)+strlen(file)+2);
    strcpy(ret,cwd);
    strcat(ret,"/");
    strcat(ret,file);
  }

  from=to=ret;


#ifdef __NT__
  if(IS_SEP(from[0]) && IS_SEP(from[1]))
    *(to++)=*(from++);
  else
#endif

  /* Skip all leading "./" */
   while(from[0]=='.' && IS_SEP(from[1])) from+=2;
  
  while(( *to = *from ))
  {
    if(IS_SEP(*from))
    {
      while(to>ret && to[-1]=='/') to--;
      if(from[1] == '.')
      {
	switch(from[2])
	{
	case '.':
	  if(IS_SEP(from[3]) || !from[3])
	  {
	    char *tmp=to;
	    while(--tmp>=ret)
	      if(IS_SEP(*tmp))
		break;
	    tmp++;

	    if(tmp[0]=='.' && tmp[1]=='.' && (IS_SEP(tmp[2]) || !tmp[2]))
	      break;
	    
	    from+=3;
	    to=tmp;
	    continue;
	  }
	  break;

	case 0:
	case '/':
#ifdef __NT__
        case '\\':
#endif
	  from+=2;
	  continue;
	}
      }
    }
    from++;
    to++;
  }

  if(*ret && !IS_SEP(from[-1]) && IS_SEP(to[-1]))
      *--to=0;

  if(!*ret)
  {
    if(IS_SEP(*cwd))
    {
      ret[0]='/';
      ret[1]=0;
    }else{
      ret[0]='.';
      ret[1]=0;
    }
  }

  if(my_cwd) free(my_cwd);
  return ret;
}

void f_combine_path(INT32 args)
{
  char *path=0;
  int e,dofree=0;
  struct pike_string *ret;

  if(args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("combine_path", 1);

  if(sp[-args].type != T_STRING)
    SIMPLE_BAD_ARG_ERROR("combine_path", 1, "string");

  path=sp[-args].u.string->str;

  for(e=1;e<args;e++)
  {
    char *newpath;
    if(sp[e-args].type != T_STRING)
    {
      if(dofree) free(path);
      SIMPLE_BAD_ARG_ERROR("combine_path", e+1, "string");
    }

    newpath=combine_path(path,sp[e-args].u.string->str);
    if(dofree) free(path);
    path=newpath;
    dofree=1;
  }
    
  ret=make_shared_string(path);
  if(dofree) free(path);
  pop_n_elems(args);
  push_string(ret);
}

void f_function_object(INT32 args)
{
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("function_object",1);
  if(sp[-args].type != T_FUNCTION)
    SIMPLE_BAD_ARG_ERROR("function_object",1,"function");

  if(sp[-args].subtype == FUNCTION_BUILTIN)
  {
    pop_n_elems(args);
    push_int(0);
  }else{
    pop_n_elems(args-1);
    sp[-1].type=T_OBJECT;
  }
}

void f_function_name(INT32 args)
{
  struct pike_string *s;
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("function_name", 1);
  if(sp[-args].type != T_FUNCTION)
    SIMPLE_BAD_ARG_ERROR("function_name", 1, "function");

  if(sp[-args].subtype == FUNCTION_BUILTIN)
  {
    pop_n_elems(args);
    push_int(0);
  }else{
    if(!sp[-args].u.object->prog)
      bad_arg_error("function_name", sp-args, args, 1, "function", sp-args,
		    "Destructed object.\n");

    copy_shared_string(s,ID_FROM_INT(sp[-args].u.object->prog,
				     sp[-args].subtype)->name);
    pop_n_elems(args);
  
    sp->type=T_STRING;
    sp->u.string=s;
    sp++;
  }
}

void f_zero_type(INT32 args)
{
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("zero_type",1);

  if(sp[-args].type != T_INT)
  {
    pop_n_elems(args);
    push_int(0);
  }
  else if((sp[-args].type==T_OBJECT || sp[-args].type==T_FUNCTION)
	   && !sp[-args].u.object->prog)
  {
    pop_n_elems(args);
    push_int(NUMBER_DESTRUCTED);
  }
  {
    pop_n_elems(args-1);
    sp[-1].u.integer=sp[-1].subtype;
    sp[-1].subtype=NUMBER_NUMBER;
  }
}

/*
 * Some wide-strings related functions
 */

void f_string_to_unicode(INT32 args)
{
  struct pike_string *in;
  struct pike_string *out = NULL;
  INT32 len;
  int i;

  get_all_args("string_to_unicode", args, "%W", &in);

  switch(in->size_shift) {
  case 0:
    /* Just 8bit characters */
    len = in->len * 2;
    out = begin_shared_string(len);
    MEMSET(out->str, 0, len);	/* Clear the upper (and lower) byte */
    for(i = in->len; i--;) {
      out->str[i * 2 + 1] = in->str[i];
    }
    out = end_shared_string(out);
    break;
  case 1:
    /* 16 bit characters */
    /* FIXME: Should we check for 0xfffe & 0xffff here too? */
    len = in->len * 2;
    out = begin_shared_string(len);
#if (PIKE_BYTEORDER == 4321)
    /* Big endian -- We don't need to do much...
     *
     * FIXME: Future optimization: Check if refcount is == 1,
     * and perform sufficient magic to be able to convert in place.
     */
    MEMCPY(out->str, in->str, len);
#else
    /* Other endianness, may need to do byte-order conversion also. */
    {
      p_wchar1 *str1 = STR1(in);
      for(i = in->len; i--;) {
	unsigned INT32 c = str1[i];
	out->str[i * 2 + 1] = c & 0xff;
	out->str[i * 2] = c >> 8;
      }
    }
#endif
    out = end_shared_string(out);
    break;
  case 2:
    /* 32 bit characters -- Is someone writing in Klingon? */
    {
      p_wchar2 *str2 = STR2(in);
      int j;
      len = in->len * 2;
      /* Check how many extra wide characters there are. */
      for(i = in->len; i--;) {
	if (str2[i] > 0xfffd) {
	  if (str2[i] < 0x10000) {
	    /* 0xfffe: Byte-order detection illegal character.
	     * 0xffff: Illegal character.
	     */
	    error("string_to_unicode(): Illegal character 0x%04x (index %d) "
		  "is not a Unicode character.", str2[i], i);
	  }
	  if (str2[i] > 0x10ffff) {
	    error("string_to_unicode(): Character 0x%08x (index %d) "
		  "is out of range (0x00000000 - 0x0010ffff).", str2[i], i);
	  }
	  /* Extra wide characters take two unicode characters in space.
	   * ie One unicode character extra.
	   */
	  len += 2;
	}
      }
      out = begin_shared_string(len);
      j = len;
      for(i = in->len; i--;) {
	unsigned INT32 c = str2[i];

	j -= 2;

	if (c > 0xffff) {
	  /* Use surrogates */
	  c -= 0x10000;
	  
	  out->str[j + 1] = c & 0xff;
	  out->str[j] = 0xdc | ((c >> 8) & 0x03);
	  j -= 2;
	  c >>= 10;
	  c |= 0xd800;
	}
	out->str[j + 1] = c & 0xff;
	out->str[j] = c >> 8;
      }
#ifdef PIKE_DEBUG
      if (j) {
	fatal("string_to_unicode(): Indexing error: len:%d, j:%d.\n", len, j);
      }
#endif /* PIKE_DEBUG */
      out = end_shared_string(out);
    }
    break;
  default:
    error("string_to_unicode(): Bad string shift: %d!\n", in->size_shift);
    break;
  }
  pop_n_elems(args);
  push_string(out);
}

void f_unicode_to_string(INT32 args)
{
  struct pike_string *in;
  struct pike_string *out = NULL;
  INT32 len;

  get_all_args("unicode_to_string", args, "%S", &in);

  if (in->len & 1) {
    bad_arg_error("unicode_to_string", sp-args, args, 1, "string", sp-args,
		  "String length is odd.\n");
  }

  /* FIXME: In the future add support for decoding of surrogates. */

  len = in->len / 2;

  out = begin_wide_shared_string(len, 1);
#if (PIKE_BYTEORDER == 4321)
  /* Big endian
   *
   * FIXME: Future optimization: Perform sufficient magic
   * to do the conversion in place if the ref-count is == 1.
   */
  MEMCPY(out->str, in->str, in->len);
#else
  /* Little endian */
  {
    int i;
    p_wchar1 *str1 = STR1(out);

    for (i = len; i--;) {
      str1[i] = (((unsigned char *)in->str)[i*2]<<8) +
	((unsigned char *)in->str)[i*2 + 1];
    }
  }
#endif /* PIKE_BYTEORDER == 4321 */
  out = end_shared_string(out);
  pop_n_elems(args);
  push_string(out);
}

void f_string_to_utf8(INT32 args)
{
  int len;
  struct pike_string *in;
  struct pike_string *out;
  int i,j;
  int extended = 0;

  get_all_args("string_to_utf8", args, "%W", &in);

  if (args > 1) {
    if (sp[1-args].type != T_INT) {
      SIMPLE_BAD_ARG_ERROR("string_to_utf8", 2, "int|void");
    }
    extended = sp[1-args].u.integer;
  }

  len = in->len;

  for(i=0; i < in->len; i++) {
    unsigned INT32 c = index_shared_string(in, i);
    if (c & ~0x7f) {
      /* 8bit or more. */
      len++;
      if (c & ~0x7ff) {
	/* 12bit or more. */
	len++;
	if (c & ~0xffff) {
	  /* 17bit or more. */
	  len++;
	  if (c & ~0x1fffff) {
	    /* 22bit or more. */
	    len++;
	    if (c & ~0x3ffffff) {
	      /* 27bit or more. */
	      len++;
	      if (c & ~0x7fffffff) {
		/* 32bit or more. */
		if (!extended) {
		  error("string_to_utf8(): "
			"Value 0x%08x (index %d) is larger than 31 bits.\n",
			c, i);
		}
		len++;
		/* FIXME: Needs fixing when we get 64bit chars... */
	      }
	    }
	  }
	}
      }
    }
  }
  if (len == in->len) {
    /* 7bit string -- already valid utf8. */
    pop_n_elems(args - 1);
    return;
  }
  out = begin_shared_string(len);

  for(i=j=0; i < in->len; i++) {
    unsigned INT32 c = index_shared_string(in, i);
    if (!(c & ~0x7f)) {
      /* 7bit */
      out->str[j++] = c;
    } else if (!(c & ~0x7ff)) {
      /* 11bit */
      out->str[j++] = 0xc0 | (c >> 6);
      out->str[j++] = 0x80 | (c & 0x3f);
    } else if (!(c & ~0xffff)) {
      /* 16bit */
      out->str[j++] = 0xe0 | (c >> 12);
      out->str[j++] = 0x80 | ((c >> 6) & 0x3f);
      out->str[j++] = 0x80 | (c & 0x3f);
    } else if (!(c & ~0x1fffff)) {
      /* 21bit */
      out->str[j++] = 0xf0 | (c >> 18);
      out->str[j++] = 0x80 | ((c >> 12) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 6) & 0x3f);
      out->str[j++] = 0x80 | (c & 0x3f);
    } else if (!(c & ~0x3ffffff)) {
      /* 26bit */
      out->str[j++] = 0xf8 | (c >> 24);
      out->str[j++] = 0x80 | ((c >> 18) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 12) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 6) & 0x3f);
      out->str[j++] = 0x80 | (c & 0x3f);
    } else if (!(c & ~0x7fffffff)) {
      /* 31bit */
      out->str[j++] = 0xfc | (c >> 30);
      out->str[j++] = 0x80 | ((c >> 24) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 18) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 12) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 6) & 0x3f);
      out->str[j++] = 0x80 | (c & 0x3f);
    } else {
      /* This and onwards is extended UTF-8 encoding. */
      /* 32 - 36bit */
      out->str[j++] = 0xfe;
      out->str[j++] = 0x80 | ((c >> 30) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 24) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 18) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 12) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 6) & 0x3f);
      out->str[j++] = 0x80 | (c & 0x3f);
    }
  }
#ifdef PIKE_DEBUG
  if (len != j) {
    fatal("string_to_utf8(): Calculated and actual lengths differ: %d != %d\n",
	  len, j);
  }
#endif /* PIKE_DEBUG */
  out = end_shared_string(out);
  pop_n_elems(args);
  push_string(out);
}

void f_utf8_to_string(INT32 args)
{
  struct pike_string *in;
  struct pike_string *out;
  int len = 0;
  int shift = 0;
  int i,j;
  int extended = 0;

  get_all_args("utf8_to_string", args, "%S", &in);

  if (args > 1) {
    if (sp[1-args].type != T_INT) {
      SIMPLE_BAD_ARG_ERROR("utf8_to_string()", 2, "int|void");
    }
    extended = sp[1-args].u.integer;
  }

  for(i=0; i < in->len; i++) {
    unsigned int c = ((unsigned char *)in->str)[i];
    len++;
    if (c & 0x80) {
      int cont = 0;
      if ((c & 0xc0) == 0x80) {
	error("utf8_to_string(): "
	      "Unexpected continuation block 0x%02x at index %d.\n",
	      c, i);
      }
      if ((c & 0xe0) == 0xc0) {
	/* 11bit */
	cont = 1;
	if (c & 0x1c) {
	  if (shift < 1) {
	    shift = 1;
	  }
	}
      } else if ((c & 0xf0) == 0xe0) {
	/* 16bit */
	cont = 2;
	if (shift < 1) {
	  shift = 1;
	}
      } else {
	shift = 2;
	if ((c & 0xf8) == 0xf0) {
	  /* 21bit */
	  cont = 3;
	} else if ((c & 0xfc) == 0xf8) {
	  /* 26bit */
	  cont = 4;
	} else if ((c & 0xfe) == 0xfc) {
	  /* 31bit */
	  cont = 5;
	} else if (c == 0xfe) {
	  /* 36bit */
	  if (!extended) {
	    error("utf8_to_string(): "
		  "Character 0xfe at index %d when not in extended mode.\n",
		  i);
	  }
	  cont = 6;
	} else {
	  error("utf8_to_string(): "
		"Unexpected character 0xff at index %d.\n",
		i);
	}
      }
      while(cont--) {
	i++;
	if (i >= in->len) {
	  error("utf8_to_string(): Truncated UTF8 sequence.\n");
	}
	c = ((unsigned char *)(in->str))[i];
	if ((c & 0xc0) != 0x80) {
	  error("utf8_to_string(): "
		"Expected continuation character at index %d (got 0x%02x).\n",
		i, c);
	}
      }
    }
  }
  if (len == in->len) {
    /* 7bit in == 7bit out */
    pop_n_elems(args-1);
    return;
  }

  out = begin_wide_shared_string(len, shift);
  
  for(j=i=0; i < in->len; i++) {
    unsigned int c = ((unsigned char *)in->str)[i];

    if (c & 0x80) {
      int cont = 0;

      /* NOTE: The tests aren't as paranoid here, since we've
       * already tested the string above.
       */
      if ((c & 0xe0) == 0xc0) {
	/* 11bit */
	cont = 1;
	c &= 0x1f;
      } else if ((c & 0xf0) == 0xe0) {
	/* 16bit */
	cont = 2;
	c &= 0x0f;
      } else if ((c & 0xf8) == 0xf0) {
	/* 21bit */
	cont = 3;
	c &= 0x07;
      } else if ((c & 0xfc) == 0xf8) {
	/* 26bit */
	cont = 4;
	c &= 0x03;
      } else if ((c & 0xfe) == 0xfc) {
	/* 31bit */
	cont = 5;
	c &= 0x01;
      } else {
	/* 36bit */
	cont = 6;
	c = 0;
      }
      while(cont--) {
	unsigned INT32 c2 = ((unsigned char *)(in->str))[++i] & 0x3f;
	c = (c << 6) | c2;
      }
    }
    low_set_index(out, j++, c);
  }
#ifdef PIKE_DEBUG
  if (j != len) {
    fatal("utf8_to_string(): Calculated and actual lengths differ: %d != %d\n",
	  len, j);
  }
#endif /* PIKE_DEBUG */
  out = end_shared_string(out);
  pop_n_elems(args);
  push_string(out);
}

static void f_parse_pike_type( INT32 args )
{
  struct pike_string *res;
  if( sp[-1].type != T_STRING ||
      sp[-1].u.string->size_shift )
    error( "__parse_type requires a 8bit string as its first argument\n" );
  res = parse_type( (char *)STR0(sp[-1].u.string) );
  pop_stack();
  push_string( res );
}

void f_all_constants(INT32 args)
{
  pop_n_elems(args);
  ref_push_mapping(get_builtin_constants());
}

void f_allocate(INT32 args)
{
  INT32 size;
  struct array *a;

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("allocate",1);

  if(sp[-args].type!=T_INT)
    SIMPLE_BAD_ARG_ERROR("allocate",1,"int");

  size=sp[-args].u.integer;
  if(size < 0)
    PIKE_ERROR("allocate", "Can't allocate array of negative size.\n", sp, args);
  a=allocate_array(size);
  if(args>1)
  {
    INT32 e;
    for(e=0;e<a->size;e++)
      copy_svalues_recursively_no_free(a->item+e, sp-args+1, 1, 0);
  }
  pop_n_elems(args);
  push_array(a);
}

void f_rusage(INT32 args)
{
  INT32 *rus,e;
  struct array *v;
  pop_n_elems(args);
  rus=low_rusage();
  if(!rus)
    PIKE_ERROR("rusage", "System rusage information not available.\n", sp, args);
  v=allocate_array_no_init(29,0);

  for(e=0;e<29;e++)
  {
    ITEM(v)[e].type=T_INT;
    ITEM(v)[e].subtype=NUMBER_NUMBER;
    ITEM(v)[e].u.integer=rus[e];
  }

  sp->u.array=v;
  sp->type=T_ARRAY;
  sp++;
}

void f_this_object(INT32 args)
{
  pop_n_elems(args);
  if(fp)
  {
    ref_push_object(fp->current_object);
  }else{
    push_int(0);
  }
}

node *fix_this_object_type(node *n)
{
  free_string(n->type);
  type_stack_mark();
  push_type_int(new_program->id);
  /*  push_type(1);   We are rather sure that we contain ourselves... */
  push_type(0);		/* But it did not work yet, so... */
  push_type(T_OBJECT);
  n->type = pop_unfinished_type();
  if (n->parent) {
    n->parent->node_info |= OPT_TYPE_NOT_FIXED;
  }
  return NULL;
}

void f_throw(INT32 args)
{
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("throw", 1);
  assign_svalue(&throw_value,sp-args);
  pop_n_elems(args);
  throw_severity=0;
  pike_throw();
}

void f_exit(INT32 args)
{
  static int in_exit=0;
  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY, ("exit: permission denied.\n"));
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("exit", 1);

  if(sp[-args].type != T_INT)
    SIMPLE_BAD_ARG_ERROR("exit", 1, "int");

  if(in_exit) error("exit already called!\n");
  in_exit=1;

  assign_svalue(&throw_value, sp-args);
  throw_severity=THROW_EXIT;
  pike_throw();
}

void f__exit(INT32 args)
{
  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY, ("_exit: permission denied.\n"));
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("_exit", 1);

  if(sp[-args].type != T_INT)
    SIMPLE_BAD_ARG_ERROR("_exit", 1, "int");

  exit(sp[-args].u.integer);
}

void f_time(INT32 args)
{
  if(!args)
  {
    GETTIMEOFDAY(&current_time);
  }else{
    if(sp[-args].type == T_INT && sp[-args].u.integer > 1)
    {
      struct timeval tmp;
      GETTIMEOFDAY(&current_time);
      tmp.tv_sec=sp[-args].u.integer;
      tmp.tv_usec=0;
      my_subtract_timeval(&tmp,&current_time);
      pop_n_elems(args);
      push_float( - (float)tmp.tv_sec - ((float)tmp.tv_usec)/1000000 );
      return;
    }
  }
  pop_n_elems(args);
  push_int(current_time.tv_sec);
}

void f_crypt(INT32 args)
{
  char salt[2];
  char *ret, *saltp;
  char *choise =
    "cbhisjKlm4k65p7qrJfLMNQOPxwzyAaBDFgnoWXYCZ0123tvdHueEGISRTUV89./";

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("crypt", 1);

  if(sp[-args].type != T_STRING)
    SIMPLE_BAD_ARG_ERROR("crypt", 1, "string");

  
  if(args>1)
  {
    if(sp[1-args].type != T_STRING ||
       sp[1-args].u.string->len < 2)
    {
      pop_n_elems(args);
      push_int(0);
      return;
    }
      
    saltp=sp[1-args].u.string->str;
  } else {
    unsigned int foo; /* Sun CC want's this :( */
    foo=my_rand();
    salt[0] = choise[foo % (unsigned int) strlen(choise)];
    foo=my_rand();
    salt[1] = choise[foo % (unsigned int) strlen(choise)];
    saltp=salt;
  }
#ifdef HAVE_CRYPT
  ret = (char *)crypt(sp[-args].u.string->str, saltp);
#else
#ifdef HAVE__CRYPT
  ret = (char *)_crypt(sp[-args].u.string->str, saltp);
#else
  ret = sp[-args].u.string->str;
#endif
#endif
  if(args < 2)
  {
    pop_n_elems(args);
    push_string(make_shared_string(ret));
  }else{
    int i;
    i=!strcmp(ret,sp[1-args].u.string->str);
    pop_n_elems(args);
    push_int(i);
  }
}

void f_destruct(INT32 args)
{
  struct object *o;
  if(args)
  {
    if(sp[-args].type != T_OBJECT)
      SIMPLE_BAD_ARG_ERROR("destruct", 1, "object");

    o=sp[-args].u.object;
  }else{
    if(!fp)
      PIKE_ERROR("destruct", "Destruct called without argument from callback function.\n", sp, args);
	   
    o=fp->current_object;
  }
  if (o->prog && o->prog->flags & PROGRAM_NO_EXPLICIT_DESTRUCT)
    PIKE_ERROR("destruct", "Object can't be destructed explicitly.\n", sp, args);
#ifdef PIKE_SECURITY
  if(!CHECK_DATA_SECURITY(o, SECURITY_BIT_DESTRUCT))
    error("Destruct permission denied.\n");
#endif
  destruct(o);
  pop_n_elems(args);
}

void f_indices(INT32 args)
{
  INT32 size;
  struct array *a;
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("indices", 1);

  switch(sp[-args].type)
  {
  case T_STRING:
    size=sp[-args].u.string->len;
    goto qjump;

  case T_ARRAY:
    size=sp[-args].u.array->size;

  qjump:
    a=allocate_array_no_init(size,0);
    while(--size>=0)
    {
      ITEM(a)[size].type=T_INT;
      ITEM(a)[size].subtype=NUMBER_NUMBER;
      ITEM(a)[size].u.integer=size;
    }
    break;

  case T_MAPPING:
    a=mapping_indices(sp[-args].u.mapping);
    break;

  case T_MULTISET:
    a=copy_array(sp[-args].u.multiset->ind);
    break;

  case T_OBJECT:
    a=object_indices(sp[-args].u.object);
    break;

  case T_PROGRAM:
    a = program_indices(sp[-args].u.program);
    break;

  case T_FUNCTION:
    {
      struct program *p = program_from_svalue(sp-args);
      if (p) {
	a = program_indices(p);
	break;
      }
    }
    /* FALL THROUGH */

  default:
    SIMPLE_BAD_ARG_ERROR("indices", 1,
			 "string|array|mapping|"
			 "multiset|object|program|function");
    return; /* make apcc happy */
  }
  pop_n_elems(args);
  push_array(a);
}

/* this should probably be moved to pike_constants.c or something */
#define FIX_OVERLOADED_TYPE(n, lf, X) fix_overloaded_type(n,lf,X,CONSTANT_STRLEN(X))
static node *fix_overloaded_type(node *n, int lfun, const char *deftype, int deftypelen)
{
  node **first_arg;
  struct pike_string *t,*t2;
  first_arg=my_get_arg(&_CDR(n), 0);
  if(!first_arg) return 0;
  t=first_arg[0]->type;
  if(!t || match_types(t, object_type_string))
  {
    if(t && t->str[0]==T_OBJECT)
    {
      struct program *p=id_to_program(extract_type_int(t->str+2));
      if(p)
      {
	int fun=FIND_LFUN(p, lfun);

	/* FIXME: function type string should really be compiled from
	 * the arguments so that or:ed types are handled correctly
	 */
	if(fun!=-1 &&
	   (t2=check_call(function_type_string , ID_FROM_INT(p, fun)->type,
			  0)))
	{
	  free_string(n->type);
	  n->type=t2;
	  return 0;
	}
      }
    }

    /* If it is an object, it *may* be overloaded, we or with 
     * the deftype....
     */
#if 1
    if(deftype)
    {
      t2=make_shared_binary_string(deftype, deftypelen);
      t=n->type;
      n->type=or_pike_types(t,t2,0);
      free_string(t);
      free_string(t2);
    }
#endif
  }
  
  return 0; /* continue optimization */
}

static node *fix_indices_type(node *n)
{
  return FIX_OVERLOADED_TYPE(n, LFUN__INDICES, tArray);
}

static node *fix_values_type(node *n)
{
  return FIX_OVERLOADED_TYPE(n, LFUN__VALUES, tArray);
}

static node *fix_aggregate_mapping_type(node *n)
{
  struct pike_string *types[2] = { NULL, NULL };
  node *args = CDR(n);
  struct pike_string *new_type = NULL;

#ifdef PIKE_DEBUG
  if (l_flag > 2) {
    fprintf(stderr, "Fixing type for aggregate_mapping():\n");
    print_tree(n);

    fprintf(stderr, "Original type:");
    simple_describe_type(n->type);
  }
#endif /* PIKE_DEBUG */

  if (args) {
    node *arg = args;
    int argno = 0;

    /* Make it easier to find... */
    args->parent = 0;

    while(arg) {
#ifdef PIKE_DEBUG
      if (l_flag > 4) {
	fprintf(stderr, "Searching for arg #%d...\n", argno);
      }
#endif /* PIKE_DEBUG */
      if (arg->token == F_ARG_LIST) {
	if (CAR(arg)) {
	  CAR(arg)->parent = arg;
	  arg = CAR(arg);
	  continue;
	}
	if (CDR(arg)) {
	  CDR(arg)->parent = arg;
	  arg = CDR(arg);
	  continue;
	}
	/* Retrace */
      retrace:
#ifdef PIKE_DEBUG
	if (l_flag > 4) {
	  fprintf(stderr, "Retracing in search for arg %d...\n", argno);
	}
#endif /* PIKE_DEBUG */
	while (arg->parent &&
	       (!CDR(arg->parent) || (CDR(arg->parent) == arg))) {
	  arg = arg->parent;
	}
	if (!arg->parent) {
	  /* No more args. */
	  break;
	}
	arg = arg->parent;
	CDR(arg)->parent = arg;
	arg = CDR(arg);
	continue;
      }
      if (arg->token == F_PUSH_ARRAY) {
	/* FIXME: Should get the type from the pushed array. */
	/* FIXME: Should probably be fixed in las.c:fix_type_field() */
	MAKE_CONSTANT_SHARED_STRING(new_type, tMap(tMixed, tMixed));
	goto set_type;
      }
#ifdef PIKE_DEBUG
      if (l_flag > 4) {
	fprintf(stderr, "Found arg #%d:\n", argno);
	print_tree(arg);
	simple_describe_type(arg->type);
      }
#endif /* PIKE_DEBUG */
      do {
	if (types[argno]) {
	  struct pike_string *t = or_pike_types(types[argno], arg->type, 0);
	  free_string(types[argno]);
	  types[argno] = t;
#ifdef PIKE_DEBUG
	  if (l_flag > 4) {
	    fprintf(stderr, "Resulting type for arg #%d:\n", argno);
	    simple_describe_type(types[argno]);
	  }
#endif /* PIKE_DEBUG */
	} else {
	  copy_shared_string(types[argno], arg->type);
	}
	argno = !argno;
	/* Handle the special case where CAR & CDR are the same.
	 * Only occurrs with SHARED_NODES.
	 */
      } while (argno && arg->parent && CAR(arg->parent) == CDR(arg->parent));
      goto retrace;
    }

    if (argno) {
      yyerror("Odd number of arguments to aggregate_mapping().");
      goto done;
    }

    if (!types[0]) {
      MAKE_CONSTANT_SHARED_STRING(new_type, tMap(tZero, tZero));
      goto set_type;
    }

    type_stack_mark();
    push_unfinished_type(types[1]->str);
    push_unfinished_type(types[0]->str);
    push_type(T_MAPPING);
    new_type = pop_unfinished_type();
  } else {
    MAKE_CONSTANT_SHARED_STRING(new_type, tMap(tZero, tZero));
    goto set_type;
  }
  if (new_type) {
  set_type:
    free_string(n->type);
    n->type = new_type;

#ifdef PIKE_DEBUG
    if (l_flag > 2) {
      fprintf(stderr, "Result type: ");
      simple_describe_type(new_type);
    }
#endif /* PIKE_DEBUG */

    if (n->parent) {
      n->parent->node_info |= OPT_TYPE_NOT_FIXED;
    }    
  }
 done:
  if (args) {
    /* Not really needed, but... */
    args->parent = n;
  }
  if (types[1]) {
    free_string(types[1]);
  }
  if (types[0]) {
    free_string(types[0]);
  }
  return NULL;
}

void f_values(INT32 args)
{
  INT32 size;
  struct array *a;
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("values", 1);

  switch(sp[-args].type)
  {
  case T_STRING:
    size = sp[-args].u.string->len;
    a = allocate_array_no_init(size,0);
    while(--size >= 0)
    {
      ITEM(a)[size].type = T_INT;
      ITEM(a)[size].subtype = NUMBER_NUMBER;
      ITEM(a)[size].u.integer = index_shared_string(sp[-args].u.string, size);
    }
    break;

  case T_ARRAY:
    a=copy_array(sp[-args].u.array);
    break;

  case T_MAPPING:
    a=mapping_values(sp[-args].u.mapping);
    break;

  case T_MULTISET:
    size=sp[-args].u.multiset->ind->size;
    a=allocate_array_no_init(size,0);
    while(--size>=0)
    {
      ITEM(a)[size].type=T_INT;
      ITEM(a)[size].subtype=NUMBER_NUMBER;
      ITEM(a)[size].u.integer=1;
    }
    break;

  case T_OBJECT:
    a=object_values(sp[-args].u.object);
    break;

  case T_PROGRAM:
    a = program_values(sp[-args].u.program);
    break;

  case T_FUNCTION:
    {
      struct program *p = program_from_svalue(sp - args);
      if (p) {
	a = program_values(p);
	break;
      }
    }
    /* FALL THROUGH */

  default:
    SIMPLE_BAD_ARG_ERROR("values", 1,
			 "string|array|mapping|multiset|"
			 "object|program|function");
    return;  /* make apcc happy */
  }
  pop_n_elems(args);
  push_array(a);
}

void f_next_object(INT32 args)
{
  struct object *o;
  if(args < 1)
  {
    o=first_object;
  }else{
    if(sp[-args].type != T_OBJECT)
      SIMPLE_BAD_ARG_ERROR("next_object", 1, "object");
    o=sp[-args].u.object->next;
    while(o && !o->prog) o=o->next;
  }
  pop_n_elems(args);
  if(!o)
  {
    push_int(0);
  }else{
    ref_push_object(o);
  }
}

void f_object_program(INT32 args)
{
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("object_program", 1);

  if(sp[-args].type == T_OBJECT)
  {
    struct object *o=sp[-args].u.object;
    struct program *p;
    if((p=o->prog))
    {
      if(o->parent && o->parent->prog)
      {
	INT32 id=o->parent_identifier;
	o=o->parent;
	add_ref(o);
	pop_n_elems(args);
	push_object(o);
	sp[-1].subtype=id;
	sp[-1].type=T_FUNCTION;
	return;
      }else{
	add_ref(p);
	pop_n_elems(args);
	push_program(p);
	return;
      }
    }
  }

  pop_n_elems(args);
  push_int(0);
}

node *fix_object_program_type(node *n)
{
  /* Fix the type for a common case:
   *
   * object_program(object(is|implements foo))
   */
  node *nn;
  struct pike_string *new_type = NULL;

  if (!n->type) {
    copy_shared_string(n->type, program_type_string);
  }
  if (!(nn = CDR(n))) return NULL;
  if ((nn->token == F_ARG_LIST) && (!(nn = CAR(nn)))) return NULL;
  if (!nn->type) return NULL;

  /* Perform the actual conversion. */
  new_type = object_type_to_program_type(nn->type);
  if (new_type) {
    free_string(n->type);
    n->type = new_type;
  }
  return NULL;
}

void f_reverse(INT32 args)
{
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("reverse", 1);

  switch(sp[-args].type)
  {
  case T_STRING:
  {
    INT32 e;
    struct pike_string *s;
    s=begin_wide_shared_string(sp[-args].u.string->len,
			  sp[-args].u.string->size_shift);
    switch(sp[-args].u.string->size_shift)
    {
      case 0:
	for(e=0;e<sp[-args].u.string->len;e++)
	  STR0(s)[e]=STR0(sp[-args].u.string)[sp[-args].u.string->len-1-e];
	break;

      case 1:
	for(e=0;e<sp[-args].u.string->len;e++)
	  STR1(s)[e]=STR1(sp[-args].u.string)[sp[-args].u.string->len-1-e];
	break;

      case 2:
	for(e=0;e<sp[-args].u.string->len;e++)
	  STR2(s)[e]=STR2(sp[-args].u.string)[sp[-args].u.string->len-1-e];
	break;
    }
    s=low_end_shared_string(s);
    pop_n_elems(args);
    push_string(s);
    break;
  }

  case T_INT:
  {
    INT32 e;
    e=sp[-args].u.integer;
    e=((e & 0x55555555UL)<<1) + ((e & 0xaaaaaaaaUL)>>1);
    e=((e & 0x33333333UL)<<2) + ((e & 0xccccccccUL)>>2);
    e=((e & 0x0f0f0f0fUL)<<4) + ((e & 0xf0f0f0f0UL)>>4);
    e=((e & 0x00ff00ffUL)<<8) + ((e & 0xff00ff00UL)>>8);
    e=((e & 0x0000ffffUL)<<16)+ ((e & 0xffff0000UL)>>16);
    sp[-args].u.integer=e;
    pop_n_elems(args-1);
    break;
  }
    
  case T_ARRAY:
  {
    struct array *a;
    a=reverse_array(sp[-args].u.array);
    pop_n_elems(args);
    push_array(a);
    break;
  }

  default:
    SIMPLE_BAD_ARG_ERROR("reverse", 1, "string|int|array");    
  }
}

struct tupel
{
  int prefix;
  struct pike_string *ind;
  struct pike_string *val;
};

static int replace_sortfun(struct tupel *a,struct tupel *b)
{
  return my_quick_strcmp(a->ind,b->ind);
}

/* Magic, magic and more magic */
static int find_longest_prefix(char *str,
			       INT32 len,
			       int size_shift,
			       struct tupel *v,
			       INT32 a,
			       INT32 b)
{
  INT32 tmp,c,match=-1;
  while(a<b)
  {
    c=(a+b)/2;
    
    tmp=generic_quick_binary_strcmp(v[c].ind->str,
				    v[c].ind->len,
				    v[c].ind->size_shift,
				    str,
				    MINIMUM(len,v[c].ind->len),
				    size_shift);
    if(tmp<0)
    {
      INT32 match2=find_longest_prefix(str,
				       len,
				       size_shift,
				       v,
				       c+1,
				       b);
      if(match2!=-1) return match2;

      while(1)
      {
	if(v[c].prefix==-2)
	{
	  v[c].prefix=find_longest_prefix(v[c].ind->str,
					  v[c].ind->len,
					  v[c].ind->size_shift,
					  v,
					  0 /* can this be optimized? */,
					  c);
	}
	c=v[c].prefix;
	if(c<a || c<match) return match;

	if(!generic_quick_binary_strcmp(v[c].ind->str,
					v[c].ind->len,
					v[c].ind->size_shift,
					str,
					MINIMUM(len,v[c].ind->len),
					size_shift))
	   return c;
      }
    }
    else if(tmp>0)
    {
      b=c;
    }
    else
    {
      a=c+1; /* There might still be a better match... */
      match=c;
    }
  }
  return match;
}
			       

static struct pike_string * replace_many(struct pike_string *str,
					struct array *from,
					struct array *to)
{
  INT32 s,length,e,num;
  struct string_builder ret;

  struct tupel *v;

  int set_start[256];
  int set_end[256];

  if(from->size != to->size)
    error("Replace must have equal-sized from and to arrays.\n");

  if(!from->size)
  {
    reference_shared_string(str);
    return str;
  }

  v=(struct tupel *)xalloc(sizeof(struct tupel)*from->size);

  for(num=e=0;e<from->size;e++)
  {
    if(ITEM(from)[e].type != T_STRING)
    {
      free((char *)v);
      error("Replace: from array is not array(string)\n");
    }

    if(ITEM(to)[e].type != T_STRING)
    {
      free((char *)v);
      error("Replace: to array is not array(string)\n");
    }

    if(ITEM(from)[e].u.string->size_shift > str->size_shift)
      continue;

    v[num].ind=ITEM(from)[e].u.string;
    v[num].val=ITEM(to)[e].u.string;
    v[num].prefix=-2; /* Uninitialized */
    num++;
  }

  fsort((char *)v,num,sizeof(struct tupel),(fsortfun)replace_sortfun);

  for(e=0;e<(INT32)NELEM(set_end);e++)
    set_end[e]=set_start[e]=0;

  for(e=0;e<num;e++)
  {
    INT32 x;
    x=index_shared_string(v[num-1-e].ind,0);
    if(x<(INT32)NELEM(set_start)) set_start[x]=num-e-1;
    x=index_shared_string(v[e].ind,0);
    if(x<(INT32)NELEM(set_end)) set_end[x]=e+1;
  }

  init_string_builder(&ret,str->size_shift);

  length=str->len;

  for(s=0;length > 0;)
  {
    INT32 a,b,ch;

    ch=index_shared_string(str,s);
    if(ch<(INT32)NELEM(set_end)) b=set_end[ch]; else b=num;

    if(b)
    {
      if(ch<(INT32)NELEM(set_start)) a=set_start[ch]; else a=0;

      a=find_longest_prefix(str->str+(s << str->size_shift),
			    length,
			    str->size_shift,
			    v, a, b);

      if(a!=-1)
      {
	ch=v[a].ind->len;
	if(!ch) ch=1;
	s+=ch;
	length-=ch;
	string_builder_shared_strcat(&ret,v[a].val);
	continue;
      }
    }
    string_builder_putchar(&ret, ch);
    s++;
    length--;
  }

  free((char *)v);
  return finish_string_builder(&ret);
}

void f_replace(INT32 args)
{
  if(args < 3)
    SIMPLE_TOO_FEW_ARGS_ERROR("replace", 3);

  switch(sp[-args].type)
  {
  case T_ARRAY:
  {
    array_replace(sp[-args].u.array,sp+1-args,sp+2-args);
    pop_n_elems(args-1);
    break;
  }

  case T_MAPPING:
  {
    mapping_replace(sp[-args].u.mapping,sp+1-args,sp+2-args);
    pop_n_elems(args-1);
    break;
  }

  case T_STRING:
  {
    struct pike_string *s;
    switch(sp[1-args].type)
    {
    default:
      SIMPLE_BAD_ARG_ERROR("replace", 2, "string|array");
      
    case T_STRING:
      if(sp[2-args].type != T_STRING)
	SIMPLE_BAD_ARG_ERROR("replace", 3, "string");

      s=string_replace(sp[-args].u.string,
		       sp[1-args].u.string,
		       sp[2-args].u.string);
      break;
      
    case T_ARRAY:
      if(sp[2-args].type != T_ARRAY)
	SIMPLE_BAD_ARG_ERROR("replace", 3, "array");

      s=replace_many(sp[-args].u.string,
		     sp[1-args].u.array,
		     sp[2-args].u.array);
    
    }
    pop_n_elems(args);
    push_string(s);
    break;
  }

  default:
    SIMPLE_BAD_ARG_ERROR("replace", 1, "array|mapping|string");
  }
}

void f_compile(INT32 args)
{
  struct program *p;

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("compile", 1);

  if(sp[-args].type != T_STRING)
    SIMPLE_BAD_ARG_ERROR("compile", 1, "string");

  if ((args > 1) && (sp[1-args].type != T_OBJECT) &&
      (sp[1-args].type != T_INT)) {
    SIMPLE_BAD_ARG_ERROR("compile", 2, "object");
  }

  if ((args > 1) && (sp[1-args].type == T_OBJECT)) {
    p = compile(sp[-args].u.string, sp[1-args].u.object);
  } else {
    p = compile(sp[-args].u.string, NULL);
  }
#ifdef PIKE_DEBUG
  if(!(p->flags & PROGRAM_FINISHED))
    fatal("Got unfinished program from internal compile().\n");
#endif
  pop_n_elems(args);
  push_program(p);
}

void f_mkmapping(INT32 args)
{
  struct mapping *m;
  struct array *a,*b;
  get_all_args("mkmapping",args,"%a%a",&a,&b);
  if(a->size != b->size)
    bad_arg_error("mkmapping", sp-args, args, 2, "array", sp+1-args,
		  "mkmapping called on arrays of different sizes (%d != %d)\n",
		  a->size, b->size);

  m=mkmapping(sp[-args].u.array, sp[1-args].u.array);
  pop_n_elems(args);
  push_mapping(m);
}

void f_mkmultiset(INT32 args)
{
  struct multiset *m;
  struct array *a;
  get_all_args("mkmultiset",args,"%a",&a);

  m=mkmultiset(sp[-args].u.array);
  pop_n_elems(args);
  push_multiset(m);
}

#define SETFLAG(FLAGS,FLAG,ONOFF) \
  FLAGS = (FLAGS & ~FLAG) | ( ONOFF ? FLAG : 0 )
void f_set_weak_flag(INT32 args)
{
  struct svalue *s;
  INT_TYPE ret;

  get_all_args("set_weak_flag",args,"%*%i",&s,&ret);

  switch(s->type)
  {
    case T_ARRAY:
      SETFLAG(s->u.array->flags,ARRAY_WEAK_FLAG,ret);
      break;
    case T_MAPPING:
      SETFLAG(s->u.mapping->flags,MAPPING_FLAG_WEAK,ret);
      break;
    case T_MULTISET:
      SETFLAG(s->u.multiset->ind->flags,(ARRAY_WEAK_FLAG|ARRAY_WEAK_SHRINK),ret);
      break;
    default:
      SIMPLE_BAD_ARG_ERROR("set_weak_flag",1,"array|mapping|multiset");
  }
  pop_n_elems(args-1);
}



void f_objectp(INT32 args)
{
  if(args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("objectp", 1);
  if(sp[-args].type != T_OBJECT || !sp[-args].u.object->prog
#ifdef AUTO_BIGNUM
     || is_bignum_object(sp[-args].u.object)
#endif
     )
  {
    pop_n_elems(args);
    push_int(0);
  }else{
    pop_n_elems(args);
    push_int(1);
  }
}

void f_functionp(INT32 args)
{
  if(args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("functionp", 1);
  if(sp[-args].type != T_FUNCTION ||
     (sp[-args].subtype != FUNCTION_BUILTIN && !sp[-args].u.object->prog))
  {
    pop_n_elems(args);
    push_int(0);
  }else{
    pop_n_elems(args);
    push_int(1);
  }
}

#ifndef HAVE_AND_USE_POLL
#undef HAVE_POLL
#endif

void f_sleep(INT32 args)
{
#define POLL_SLEEP_LIMIT 0.02

#ifdef HAVE_GETHRTIME
   hrtime_t t0,tv;
#else
   struct timeval t0,tv;
#endif

   double delay=0.0;
   double target;
   int do_microsleep;

#ifdef HAVE_GETHRTIME
   t0=tv=gethrtime();
#define GET_TIME_ELAPSED tv=gethrtime()
#define TIME_ELAPSED (tv-t0)*1e-9
#else
   GETTIMEOFDAY(&t0);
   tv=t0;
#define GET_TIME_ELAPSED GETTIMEOFDAY(&tv)
#define TIME_ELAPSED ((tv.tv_sec-t0.tv_sec) + (tv.tv_usec-t0.tv_usec)*1e-6)
#endif

#define FIX_LEFT() \
       GET_TIME_ELAPSED; \
       left = delay - TIME_ELAPSED; \
       if (do_microsleep) left-=POLL_SLEEP_LIMIT;

   switch(sp[-args].type)
   {
      case T_INT:
	 delay=(double)sp[-args].u.integer;
	 break;

      case T_FLOAT:
	 delay=(double)sp[-args].u.float_number;
	 break;
   }

   /* Special case, sleep(0) means 'yield' */
   if(delay == 0.0)
   {
     check_threads_etc();
     pop_n_elems(args);
     return;
   }

   do_microsleep=delay<10;

   pop_n_elems(args);


   if (delay>POLL_SLEEP_LIMIT)
   {
     while(1)
     {
       double left;
       /* THREADS_ALLOW may take longer time then POLL_SLEEP_LIMIT */
       THREADS_ALLOW();
       do {
	 FIX_LEFT();
	 if(left<=0.0) break;

#ifdef __NT__
	 Sleep((int)(left*1000));
#elif defined(HAVE_POLL)
	 poll(NULL,0,(int)(left*1000));
#else
	 {
	   struct timeval t3;
	   t3.tv_sec=left;
	   t3.tv_usec=(int)((left - (int)left)*1e6);
	   select(0,0,0,0,&t3);
	 }
#endif
       } while(0);
       THREADS_DISALLOW();
       
       FIX_LEFT();
       
       if(left<=0.0)
       {
	 break;
       }else{
	 check_signals(0,0,0);
       }
     }
   }

   if (do_microsleep)
      while (delay>TIME_ELAPSED) 
	 GET_TIME_ELAPSED;
}

void f_gc(INT32 args)
{
  INT32 tmp;
  pop_n_elems(args);
  tmp=num_objects;
  do_gc();
  push_int(tmp - num_objects);
}

#ifdef TYPEP
#undef TYPEP
#endif

#ifdef AUTO_BIGNUM
/* This should probably be here weather AUTO_BIGNUM is defined or not,
 * but it can wait a little. /Hubbe
 */

#define TYPEP(ID,NAME,TYPE,TYPE_NAME)				\
void ID(INT32 args)						\
{								\
  int t;							\
  if(args<1)							\
    SIMPLE_TOO_FEW_ARGS_ERROR(NAME, 1);				\
  if(sp[-args].type == T_OBJECT && sp[-args].u.object->prog)	\
  {								\
    int fun=FIND_LFUN(sp[-args].u.object->prog,LFUN__IS_TYPE);	\
    if(fun != -1)						\
    {								\
      push_constant_text(TYPE_NAME);				\
      apply_low(sp[-args-1].u.object,fun,1);			\
      stack_unlink(args);					\
      return;							\
    }								\
  }								\
  t=sp[-args].type == TYPE;					\
  pop_n_elems(args);						\
  push_int(t);							\
}
#else
#define TYPEP(ID,NAME,TYPE) \
void ID(INT32 args) \
{ \
  int t; \
  if(args<1) SIMPLE_TOO_FEW_ARGS_ERROR(NAME, 1); \
  t=sp[-args].type == TYPE; \
  pop_n_elems(args); \
  push_int(t); \
}
#endif /* AUTO_BIGNUM */


void f_programp(INT32 args)
{
  if(args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("programp", 1);
  switch(sp[-args].type)
  {
  case T_PROGRAM:
    pop_n_elems(args);
    push_int(1);
    return;

  case T_FUNCTION:
    if(program_from_function(sp-args))
    {
      pop_n_elems(args);
      push_int(1);
      return;
    }

  default:
    pop_n_elems(args);
    push_int(0);
  }
}

#ifdef AUTO_BIGNUM
TYPEP(f_intp, "intp", T_INT, "int")
TYPEP(f_mappingp, "mappingp", T_MAPPING, "mapping")
TYPEP(f_arrayp, "arrayp", T_ARRAY, "array")
TYPEP(f_multisetp, "multisetp", T_MULTISET, "multiset")
TYPEP(f_stringp, "stringp", T_STRING, "string")
TYPEP(f_floatp, "floatp", T_FLOAT, "float")
#else
TYPEP(f_intp, "intp", T_INT)
TYPEP(f_mappingp, "mappingp", T_MAPPING)
TYPEP(f_arrayp, "arrayp", T_ARRAY)
TYPEP(f_multisetp, "multisetp", T_MULTISET)
TYPEP(f_stringp, "stringp", T_STRING)
TYPEP(f_floatp, "floatp", T_FLOAT)
#endif /* AUTO_BIGNUM */
     
void f_sort(INT32 args)
{
  INT32 e,*order;

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("sort", 1);

  for(e=0;e<args;e++)
  {
    if(sp[e-args].type != T_ARRAY)
      SIMPLE_BAD_ARG_ERROR("sort", e+1, "array");

    if(sp[e-args].u.array->size != sp[-args].u.array->size)
      bad_arg_error("sort", sp-args, args, e+1, "array", sp+e-args,
		    "Argument %d has wrong size.\n", (e+1));
  }

  if(args > 1)
  {
    order=get_alpha_order(sp[-args].u.array);
    for(e=0;e<args;e++) order_array(sp[e-args].u.array,order);
    free((char *)order);
    pop_n_elems(args-1);
  } else {
    sort_array_destructively(sp[-args].u.array);
  }
}

void f_rows(INT32 args)
{
  INT32 e;
  struct array *a,*tmp;
  struct svalue *val;

  get_all_args("rows", args, "%*%a", &val, &tmp);

  /* Optimization */
  if(tmp->refs == 1)
  {
    struct svalue sval;
    tmp->type_field = BIT_MIXED | BIT_UNFINISHED;
    for(e=0;e<tmp->size;e++)
    {
      index_no_free(&sval, val, ITEM(tmp)+e);
      free_svalue(ITEM(tmp)+e);
      ITEM(tmp)[e]=sval;
    }
    stack_swap();
    pop_stack();
    return;
  }

  push_array(a=allocate_array(tmp->size));
  
  for(e=0;e<a->size;e++)
    index_no_free(ITEM(a)+e, val, ITEM(tmp)+e);
  
  sp--;
  dmalloc_touch_svalue(sp);
  pop_n_elems(args);
  push_array(a);
}

void f_column(INT32 args)
{
  INT32 e;
  struct array *a,*tmp;
  struct svalue *val;

  DECLARE_CYCLIC();

  get_all_args("column", args, "%a%*", &tmp, &val);

  /* Optimization */
  if(tmp->refs == 1)
  {
    /* An array with one ref cannot possibly be cyclic */
    struct svalue sval;
    tmp->type_field = BIT_MIXED | BIT_UNFINISHED;
    for(e=0;e<tmp->size;e++)
    {
      index_no_free(&sval, ITEM(tmp)+e, val);
      free_svalue(ITEM(tmp)+e);
      ITEM(tmp)[e]=sval;
    }
    pop_stack();
    return;
  }

  if((a=(struct array *)BEGIN_CYCLIC(tmp,0)))
  {
    pop_n_elems(args);
    ref_push_array(a);
  }else{
    push_array(a=allocate_array(tmp->size));
    SET_CYCLIC_RET(a);

    for(e=0;e<a->size;e++)
      index_no_free(ITEM(a)+e, ITEM(tmp)+e, val);

    END_CYCLIC();
    sp--;
    dmalloc_touch_svalue(sp);
    pop_n_elems(args);
    push_array(a);
  }
}

#ifdef PIKE_DEBUG
void f__verify_internals(INT32 args)
{
  INT32 tmp=d_flag;
  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY,
			  ("_verify_internals: permission denied.\n"));
  d_flag=0x7fffffff;
  do_debug();
  d_flag=tmp;
  do_gc();
  pop_n_elems(args);
}

void f__debug(INT32 args)
{
  INT_TYPE d;

  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY,
			  ("_debug: permission denied.\n"));

  get_all_args("_debug", args, "%i", &d);
  pop_n_elems(args);
  push_int(d_flag);
  d_flag = d;
}

void f__optimizer_debug(INT32 args)
{
  INT_TYPE l;

  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY,
			  ("_optimizer_debug: permission denied.\n"));

  get_all_args("_optimizer_debug", args, "%i", &l);
  pop_n_elems(args);
  push_int(l_flag);
  l_flag = l;
}

#ifdef YYDEBUG

void f__compiler_trace(INT32 args)
{
  extern int yydebug;
  INT_TYPE yyd;
  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY,
			  ("_compiler_trace: permission denied.\n"));
  get_all_args("_compiler_trace", args, "%i", &yyd);
  pop_n_elems(args);
  push_int(yydebug);
  yydebug = yyd;
}

#endif /* YYDEBUG */
#endif

#if defined(HAVE_LOCALTIME) || defined(HAVE_GMTIME)
static void encode_struct_tm(struct tm *tm)
{
  push_string(make_shared_string("sec"));
  push_int(tm->tm_sec);
  push_string(make_shared_string("min"));
  push_int(tm->tm_min);
  push_string(make_shared_string("hour"));
  push_int(tm->tm_hour);

  push_string(make_shared_string("mday"));
  push_int(tm->tm_mday);
  push_string(make_shared_string("mon"));
  push_int(tm->tm_mon);
  push_string(make_shared_string("year"));
  push_int(tm->tm_year);

  push_string(make_shared_string("wday"));
  push_int(tm->tm_wday);
  push_string(make_shared_string("yday"));
  push_int(tm->tm_yday);
  push_string(make_shared_string("isdst"));
  push_int(tm->tm_isdst);
}
#endif

#ifdef HAVE_GMTIME
void f_gmtime(INT32 args)
{
  struct tm *tm;
  INT_TYPE tt;
  time_t t;

  get_all_args("gmtime", args, "%i", &tt);

  t = tt;
  tm = gmtime(&t);
  pop_n_elems(args);
  encode_struct_tm(tm);

  push_string(make_shared_string("timezone"));
  push_int(0);
  f_aggregate_mapping(20);
}
#endif

#ifdef HAVE_LOCALTIME
void f_localtime(INT32 args)
{
  struct tm *tm;
  INT_TYPE tt;
  time_t t;

  get_all_args("localtime", args, "%i", &tt);

  t = tt;
  tm = localtime(&t);
  pop_n_elems(args);
  encode_struct_tm(tm);

#ifdef HAVE_EXTERNAL_TIMEZONE
  push_string(make_shared_string("timezone"));
  push_int(timezone);
  f_aggregate_mapping(20);
#else
#ifdef STRUCT_TM_HAS_GMTOFF
  push_string(make_shared_string("timezone"));
  push_int(tm->tm_gmtoff);
  f_aggregate_mapping(20);
#else
  f_aggregate_mapping(18);
#endif
#endif
}
#endif

#ifdef HAVE_MKTIME
void f_mktime (INT32 args)
{
  INT_TYPE sec, min, hour, mday, mon, year, isdst;
  struct tm date;
  struct svalue s;
  struct svalue * r;
  int retval;
  if (args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("mktime", 1);

  if(args == 1)
  {
    MEMSET(&date, 0, sizeof(date));

    push_text("sec");
    push_text("min");
    push_text("hour");
    push_text("mday");
    push_text("mon");
    push_text("year");
    push_text("isdst");
    push_text("timezone");
    f_aggregate(8);
    f_rows(2);
    sp--;
    dmalloc_touch_svalue(sp);
    push_array_items(sp->u.array);

    args=8;
  }

  get_all_args("mktime",args, "%i%i%i%i%i%i",
	       &sec, &min, &hour, &mday, &mon, &year);

  MEMSET(&date, 0, sizeof(date));
  date.tm_sec=sec;
  date.tm_min=min;
  date.tm_hour=hour;
  date.tm_mday=mday;
  date.tm_mon=mon;
  date.tm_year=year;

  if ((args > 6) && (sp[6-args].subtype == NUMBER_NUMBER))
  {
    date.tm_isdst = sp[6-args].u.integer;
  } else {
    date.tm_isdst = -1;
  }

#if STRUCT_TM_HAS_GMTOFF
  if((args > 7) && (sp[7-args].subtype == NUMBER_NUMBER))
  {
    date.tm_gmtoff=sp[7-args].u.intger;
  }else{
    time_t tmp = 0;
    data.tm_gmtoff=localtime(&tmp).tm_gmtoff;
  }
  retval=mktime(&date);
#else
#ifdef HAVE_EXTERNAL_TIMEZONE
  if((args > 7) && (sp[7-args].subtype == NUMBER_NUMBER))
  {
    retval=mktime(&date) + sp[7-args].u.integer - timezone;
  }else{
    retval=mktime(&date);
  }
#else
  retval=mktime(&date);
#endif
#endif

  if (retval == -1)
    PIKE_ERROR("mktime", "Cannot convert.\n", sp, args);
  pop_n_elems(args);
  push_int(retval);
}

#endif

/* Parse a sprintf/sscanf-style format string */
static int low_parse_format(p_wchar0 *s, int slen)
{
  int i;
  int offset = 0;
  int num_percent_percent = 0;
  struct svalue *old_sp = sp;

  for (i=offset; i < slen; i++) {
    if (s[i] == '%') {
      int j;
      if (i != offset) {
	push_string(make_shared_binary_string0(s + offset, i));
	if ((sp != old_sp+1) && (sp[-2].type == T_STRING)) {
	  /* Concat. */
	  f_add(2);
	}
      }

      for (j = i+1;j<slen;j++) {
	int c = s[j];

	switch(c) {
	  /* Flags */
	case '!':
	case '#':
	case '$':
	case '-':
	case '/':
	case '0':
	case '=':
	case '>':
	case '@':
	case '^':
	case '_':
	case '|':
	  continue;
	  /* Padding */
	case ' ':
	case '\'':
	case '+':
	case '~':
	  break;
	  /* Attributes */
	case '.':
	case ':':
	case ';':
	  continue;
	  /* Attribute value */
	case '1': case '2': case '3': case '4': case '5':
	case '6': case '7': case '8': case '9':
	  continue;
	  /* Specials */
	case '%':
	  push_constant_text("%");
	  if ((sp != old_sp+1) && (sp[-2].type == T_STRING)) {
	    /* Concat. */
	    f_add(2);
	  }
	  break;
	case '{':
	  i = j + 1 + low_parse_format(s + j + 1, slen - (j+1));
	  f_aggregate(1);
	  if ((i + 2 >= slen) || (s[i] != '%') || (s[i+1] != '}')) {
	    error("parse_format(): Expected %%}.\n");
	  }
	  i += 2;
	  break;
	case '}':
	  f_aggregate(sp - old_sp);
	  return i;
	  /* Set */
	case '[':
	  
	  break;
	  /* Argument */
	default:
	  break;
	}
	break;
      }
      if (j == slen) {
	error("parse_format(): Unterminated %%-expression.\n");
      }
      offset = i = j;
    }
  }

  if (i != offset) {
    push_string(make_shared_binary_string0(s + offset, i));
    if ((sp != old_sp+1) && (sp[-2].type == T_STRING)) {
      /* Concat. */
      f_add(2);
    }
  }

  f_aggregate(sp - old_sp);
  return i;
}

static void f_parse_format(INT32 args)
{
  struct pike_string *s = NULL;
  struct array *a;
  int len;

  get_all_args("parse_format", args, "%W", &s);

  len = low_parse_format(STR0(s), s->len);
  if (len != s->len) {
    error("parse_format(): Unexpected %%} in format string at offset %d\n",
	  len);
  }
#ifdef PIKE_DEBUG
  if (sp[-1].type != T_ARRAY) {
    fatal("parse_format(): Unexpected result from low_parse_format()\n");
  }
#endif /* PIKE_DEBUG */
  a = (--sp)->u.array;
  debug_malloc_touch(a);

  pop_n_elems(args);
  push_array(a);
}


/* Check if the string s[0..len[ matches the glob m[0..mlen[ */
static int does_match(struct pike_string *s,int j,
		      struct pike_string *m,int i)
{
  for (; i<m->len; i++)
  {
    switch (index_shared_string(m,i))
    {
     case '?':
       if(j++>=s->len) return 0;
       break;

     case '*': 
      i++;
      if (i==m->len) return 1;	/* slut */

      for (;j<s->len;j++)
	if (does_match(s,j,m,i))
	  return 1;

      return 0;

     default: 
       if(j>=s->len ||
	  index_shared_string(m,i)!=index_shared_string(s,j)) return 0;
       j++;
    }
  }
  return j==s->len;
}

void f_glob(INT32 args)
{
  INT32 i,matches;
  struct array *a;
  struct svalue *sval, tmp;
  struct pike_string *glob;

  if(args < 2)
    SIMPLE_TOO_FEW_ARGS_ERROR("glob", 2);

  if(args > 2)
    pop_n_elems(args-2);
  args=2;

  if (sp[-args].type!=T_STRING)
    SIMPLE_BAD_ARG_ERROR("glob", 1, "string");

  glob=sp[-args].u.string;

  switch(sp[1-args].type)
  {
  case T_STRING:
    i=does_match(sp[1-args].u.string,0,glob,0);
    pop_n_elems(2);
    push_int(i);
    break;
    
  case T_ARRAY:
    a=sp[1-args].u.array;
    matches=0;
    for(i=0;i<a->size;i++)
    {
      if(ITEM(a)[i].type != T_STRING)
	SIMPLE_BAD_ARG_ERROR("glob", 2, "string|array(string)");

      if(does_match(ITEM(a)[i].u.string,0,glob,0))
      {
	add_ref(ITEM(a)[i].u.string);
	push_string(ITEM(a)[i].u.string);
	matches++;
      }
    }
    f_aggregate(matches);
    tmp=sp[-1];
    sp--;
    dmalloc_touch_svalue(sp);
    pop_n_elems(2);
    sp[0]=tmp;
    sp++;
    break;

  default:
    SIMPLE_BAD_ARG_ERROR("glob", 1, "string|array(string)");
  }
}

/* comb_merge */

/* mixed interleave_array(array(mapping(int:mixed)) tab) */
static void f_interleave_array(INT32 args)
{
  struct array *arr = NULL;
  struct array *min = NULL;
  struct array *order = NULL;
  int max = 0;
  int ok;
  int nelems = 0;
  int i;

  get_all_args("interleave_array", args, "%a", &arr);

  /* We're not interrested in any other arguments. */
  pop_n_elems(args-1);

  if ((ok = arr->type_field & BIT_MAPPING) &&
      (arr->type_field & ~BIT_MAPPING)) {
    /* Might be ok, but do some more checking... */
    for(i = 0; i < arr->size; i++) {
      if (ITEM(arr)[i].type != T_MAPPING) {
	ok = 0;
	break;
      }
    }
  }
  if (!ok) {
    SIMPLE_BAD_ARG_ERROR("interleave_array", 1, "array(mapping(int:mixed))");
  }

  /* The order array */
  ref_push_array(arr);
  f_indices(1);
  order = sp[-1].u.array;

  /* The min array */
  push_array(min = allocate_array(arr->size));

  /* Initialize the min array */
  for (i = 0; i < arr->size; i++) {
    struct mapping *m;
    /* e and k are used by MAPPING_LOOP() */
    INT32 e;
    struct keypair *k;
    INT_TYPE low = 0x7fffffff;
#ifdef PIKE_DEBUG
    if (ITEM(arr)[i].type != T_MAPPING) {
      error("interleave_array(): Element %d is not a mapping!\n", i);
    }
#endif /* PIKE_DEBUG */
    m = ITEM(arr)[i].u.mapping;
    MAPPING_LOOP(m) {
      if (k->ind.type != T_INT) {
	error("interleave_array(): Index not an integer in mapping %d!\n", i);
      }
      if (low > k->ind.u.integer) {
	low = k->ind.u.integer;
	if (low < 0) {
	  error("interleave_array(): Index %d in mapping %d is negative!\n",
		low, i);
	}
      }
      if (max < k->ind.u.integer) {
	max = k->ind.u.integer;
      }
      nelems++;
    }
    /* FIXME: Is this needed? Isn't T_INT default? */
    ITEM(min)[i].u.integer = low;
  }

  ref_push_array(order);
  f_sort(2);	/* Sort the order array on the minimum index */

  /* State on stack now:
   *
   * array(mapping(int:mixed))	arr
   * array(int)			order
   * array(int)			min (now sorted)
   */

  /* Now we can start with the real work... */
  {
    char *tab;
    int size;
    int minfree = 0;

    /* Initialize the lookup table */
    max += 1;
    max *= 2;
    /* max will be the padding at the end. */
    size = (nelems + max) * 8;	/* Initial size */
    if (!(tab = malloc(size + max))) {
      SIMPLE_OUT_OF_MEMORY_ERROR("interleave_array", size+max);
    }
    MEMSET(tab, 0, size + max);

    for (i = 0; i < order->size; i++) {
      int low = ITEM(min)[i].u.integer;
      int j = ITEM(order)[i].u.integer;
      int offset = 0;
      struct mapping *m;
      INT32 e;
      struct keypair *k;

      if (! m_sizeof(m = ITEM(arr)[j].u.mapping)) {
	/* Not available */
	ITEM(min)[i].u.integer = -1;
	continue;
      }

      if (low < minfree) {
	offset = minfree - low;
      } else {
	minfree = offset;
      }

      ok = 0;
      while (!ok) {
	ok = 1;
	MAPPING_LOOP(m) {
	  int ind = k->ind.u.integer;
	  if (tab[offset + ind]) {
	    ok = 0;
	    while (tab[++offset + ind])
	      ;
	  }
	}
      }
      MAPPING_LOOP(m) {
	tab[offset + k->ind.u.integer] = 1;
      }
      while(tab[minfree]) {
	minfree++;
      }
      ITEM(min)[i].u.integer = offset;

      /* Check need for realloc */
      if (offset >= size) {
	char *newtab = realloc(tab, size*2 + max);
	if (!newtab) {
	  free(tab);
	  error("interleave_array(): Couldn't extend table!\n");
	}
	tab = newtab;
	MEMSET(tab + size + max, 0, size);
	size = size * 2;
      }
    }
    free(tab);
  }

  /* We want these two to survive the stackpopping. */
  add_ref(min);
  add_ref(order);

  pop_n_elems(3);

  /* Return value */
  ref_push_array(min);

  /* Restore the order */
  push_array(order);
  push_array(min);
  f_sort(2);
  pop_stack();
}

/* longest_ordered_sequence */

static int find_gt(struct array *a, int i, int *stack, int top)
{
  struct svalue *x = a->item + i;
  int l,h;

  if (!top || !is_lt(x, a->item + stack[top - 1])) return top;

  l = 0;
  h = top;

  while (l < h) {
    int middle = (l + h)/2;
    if (!is_gt(a->item + stack[middle], x)) {
      l = middle+1;
    } else {
      h = middle;
    }
  }
  return l;
}

static struct array *longest_ordered_sequence(struct array *a)
{
  int *stack;
  int *links;
  int i,j,top=0,l=0,ltop=-1;
  struct array *res;
  ONERROR tmp;
  ONERROR tmp2;

  if(!a->size)
    return allocate_array(0);

  stack = malloc(sizeof(int)*a->size);
  links = malloc(sizeof(int)*a->size);

  if (!stack || !links)
  {
    if (stack) free(stack);
    if (links) free(links);
    return 0;
  }

  /* is_gt(), is_lt() and low_allocate_array() can generate errors. */

  SET_ONERROR(tmp, free, stack);
  SET_ONERROR(tmp2, free, links);

  for (i=0; i<a->size; i++) {
    int pos;

    pos = find_gt(a, i, stack, top);

    if (pos == top) {
      top++;
      ltop = i;
    }
    if (pos != 0)
      links[i] = stack[pos-1];
    else
      links[i] = -1;
    stack[pos] = i;
  }

  /* FIXME(?) memory unfreed upon error here */
  res = low_allocate_array(top, 0); 
  while (ltop != -1)
  {
    res->item[--top].u.integer = ltop;
    ltop = links[ltop];
  }

  UNSET_ONERROR(tmp2);
  UNSET_ONERROR(tmp);

  free(stack);
  free(links);
  return res;
}

static void f_longest_ordered_sequence(INT32 args)
{
  struct array *a = NULL;

  get_all_args("Array.longest_ordered_sequence", args, "%a", &a);

  /* THREADS_ALLOW(); */

  a = longest_ordered_sequence(a);

  /* THREADS_DISALLOW(); */

  if (!a) {
    SIMPLE_OUT_OF_MEMORY_ERROR("Array.longest_ordered_sequence",
			       (int)sizeof(int *)*a->size*2);
  }

  pop_n_elems(args);
  push_array(a);
}

/**** diff ************************************************************/

static struct array* diff_compare_table(struct array *a,struct array *b,int *u)
{
   struct array *res;
   struct mapping *map;
   struct svalue *pval;
   int i;

   if (u) {
     *u = 0;	/* Unique rows in array b */
   }

   map=allocate_mapping(256);
   push_mapping(map); /* in case of out of memory */

   for (i=0; i<b->size; i++)
   {
      pval=low_mapping_lookup(map,b->item+i);
      if (!pval)
      {
	 struct svalue val;
	 val.type=T_ARRAY;
	 val.u.array=low_allocate_array(1,1);
	 val.u.array->item[0].type=T_INT;
	 val.u.array->item[0].subtype=NUMBER_NUMBER;
	 val.u.array->item[0].u.integer=i;
	 mapping_insert(map,b->item+i,&val);
	 free_svalue(&val);
	 if (u) {
	   (*u)++;
	 }
      }
      else
      {
	 pval->u.array=resize_array(pval->u.array,pval->u.array->size+1);
	 pval->u.array->item[pval->u.array->size-1].type=T_INT;
	 pval->u.array->item[pval->u.array->size-1].subtype=NUMBER_NUMBER;
	 pval->u.array->item[pval->u.array->size-1].u.integer=i;
      }
   }

   res=low_allocate_array(a->size,0);

   for (i=0; i<a->size; i++)
   {
      pval=low_mapping_lookup(map,a->item+i);
      if (!pval)
      {
	 res->item[i].type=T_ARRAY;
	 add_ref(res->item[i].u.array=&empty_array);
      }
      else
      {
	 assign_svalue(res->item+i,pval);
      }
   }

   pop_stack();
   return res;
}

struct diff_magic_link
{ 
   int x;
   int refs;
   struct diff_magic_link *prev;
};

struct diff_magic_link_pool
{
   struct diff_magic_link *firstfree;
   struct diff_magic_link_pool *next;
   int firstfreenum;
   struct diff_magic_link dml[1];
};

struct diff_magic_link_head
{
  unsigned int depth;
  struct diff_magic_link *link;
};

#define DMLPOOLSIZE 16384

static int dmls=0;

static INLINE struct diff_magic_link_pool*
         dml_new_pool(struct diff_magic_link_pool **pools)
{
   struct diff_magic_link_pool *new;

   new=malloc(sizeof(struct diff_magic_link_pool)+
	      sizeof(struct diff_magic_link)*DMLPOOLSIZE);
   if (!new) return NULL; /* fail */

   new->firstfreenum=0;
   new->firstfree=NULL;
   new->next=*pools;
   *pools=new;
   return *pools;
}

static INLINE struct diff_magic_link* 
       dml_new(struct diff_magic_link_pool **pools)
{
   struct diff_magic_link *new;
   struct diff_magic_link_pool *pool;

   dmls++;

   if ( *pools && (new=(*pools)->firstfree) )
   {
      (*pools)->firstfree=new->prev;
      new->prev=NULL;
      return new;
   }

   pool=*pools;
   while (pool)
   {
      if (pool->firstfreenum<DMLPOOLSIZE)
	 return pool->dml+(pool->firstfreenum++);
      pool=pool->next;
   }

   if ( (pool=dml_new_pool(pools)) )
   {
      pool->firstfreenum=1;
      return pool->dml;
   }
   
   return NULL;
}	

static INLINE void dml_free_pools(struct diff_magic_link_pool *pools)
{
   struct diff_magic_link_pool *pool;

   while (pools)
   {
      pool=pools->next;
      free(pools);
      pools=pool;
   }
}

static INLINE void dml_delete(struct diff_magic_link_pool *pools,
			      struct diff_magic_link *dml)
{
   if (dml->prev && !--dml->prev->refs) dml_delete(pools,dml->prev);
   dmls--;
   dml->prev=pools->firstfree;
   pools->firstfree=dml;
}

static INLINE int diff_ponder_stack(int x,
				    struct diff_magic_link **dml,
				    int top)
{
   int middle,a,b;
   
   a=0; 
   b=top;
   while (b>a)
   {
      middle=(a+b)/2;
      if (dml[middle]->x<x) a=middle+1;
      else if (dml[middle]->x>x) b=middle;
      else return middle;
   }
   if (a<top && dml[a]->x<x) a++;
   return a;
}

static INLINE int diff_ponder_array(int x,
				    struct svalue *arr,
				    int top)
{
   int middle,a,b;
   
   a=0; 
   b=top;
   while (b>a)
   {
      middle=(a+b)/2;
      if (arr[middle].u.integer<x) a=middle+1;
      else if (arr[middle].u.integer>x) b=middle;
      else return middle;
   }
   if (a<top && arr[a].u.integer<x) a++;
   return a;
}

/*
 * The Grubba-Mirar Longest Common Sequence algorithm.
 *
 * This algorithm is O((Na * Nb / K)*lg(Na * Nb / K)), where:
 *
 *  Na == sizeof(a)
 *  Nb == sizeof(b)
 *  K  == sizeof(correlation(a,b))
 *
 * For binary data:
 *  K == 256 => O(Na * Nb * lg(Na * Nb)),
 *  Na ~= Nb ~= N => O(N² * lg(N))
 *
 * For ascii data:
 *  K ~= C * min(Na, Nb), C constant => O(max(Na, Nb)*lg(max(Na,Nb))),
 *  Na ~= Nb ~= N => O(N * lg(N))
 *
 * diff_longest_sequence() takes two arguments:
 *  cmptbl == diff_compare_table(a, b)
 *  blen == sizeof(b) >= max(@(cmptbl*({})))
 */
static struct array *diff_longest_sequence(struct array *cmptbl, int blen)
{
   int i,j,top=0,lsize=0;
   struct array *a;
   struct diff_magic_link_pool *pools=NULL;
   struct diff_magic_link *dml;
   struct diff_magic_link **stack;
   char *marks;

   if(!cmptbl->size)
     return allocate_array(0);

   stack = malloc(sizeof(struct diff_magic_link*)*cmptbl->size);

   if (!stack) {
     int args = 0;
     SIMPLE_OUT_OF_MEMORY_ERROR("diff_longest_sequence",
				(int)sizeof(struct diff_magic_link*) *
				cmptbl->size);
   }

   /* NB: marks is used for optimization purposes only */
   marks = calloc(blen, 1);

   if (!marks && blen) {
     int args = 0;
     free(stack);
     SIMPLE_OUT_OF_MEMORY_ERROR("diff_longest_sequence", blen);
   }

#ifdef DIFF_DEBUG
   fprintf(stderr, "\n\nDIFF: sizeof(cmptbl)=%d, blen=%d\n",
	   cmptbl->size, blen);
#endif /* DIFF_DEBUG */

   for (i = 0; i<cmptbl->size; i++)
   {
      struct svalue *inner=cmptbl->item[i].u.array->item;

#ifdef DIFF_DEBUG
      fprintf(stderr, "DIFF: i=%d\n", i);
#endif /* DIFF_DEBUG */

      for (j = cmptbl->item[i].u.array->size; j--;)
      {
	 int x = inner[j].u.integer;

#ifdef DIFF_DEBUG
	 fprintf(stderr, "DIFF:  j=%d, x=%d\n", j, x);
#endif /* DIFF_DEBUG */
#ifdef PIKE_DEBUG
	 if (x >= blen) {
	   fatal("diff_longest_sequence(): x:%d >= blen:%d\n", x, blen);
	 } else if (x < 0) {
	   fatal("diff_longest_sequence(): x:%d < 0\n", x);
	 }
#endif /* PIKE_DEBUG */
	 if (!marks[x]) {
	   int pos;

	   if (top && x<=stack[top-1]->x) {
	     /* Find the insertion point. */
	     pos = diff_ponder_stack(x, stack, top);
	     if (pos != top) {
	       /* Not on the stack anymore. */
	       marks[stack[pos]->x] = 0;
	     }
	   } else
	     pos=top;

#ifdef DIFF_DEBUG
	   fprintf(stderr, "DIFF:  pos=%d\n", pos);
#endif /* DIFF_DEBUG */

	   /* This part is only optimization (j accelleration). */
	   if (pos && j)
	   {
	     if (!marks[inner[j-1].u.integer])
	     {
	       /* Find the element to insert. */
	       j = diff_ponder_array(stack[pos-1]->x+1, inner, j);
	       x = inner[j].u.integer;
	     }
	   }
	   else
	   {
	     j = 0;
	     x = inner->u.integer;
	   }

#ifdef DIFF_DEBUG
	   fprintf(stderr, "DIFF: New j=%d, x=%d\n", j, x);
#endif /* DIFF_DEBUG */
#ifdef PIKE_DEBUG
	   if (x >= blen) {
	     fatal("diff_longest_sequence(): x:%d >= blen:%d\n", x, blen);
	   } else if (x < 0) {
	     fatal("diff_longest_sequence(): x:%d < 0\n", x);
	   }
#endif /* PIKE_DEBUG */

	   /* Put x on the stack. */
	   marks[x] = 1;
	   if (pos == top)
	   {
#ifdef DIFF_DEBUG
	     fprintf(stderr, "DIFF:  New top element\n");
#endif /* DIFF_DEBUG */

	     if (! (dml=dml_new(&pools)) )
	     {
	       int args = 0;
	       dml_free_pools(pools);
	       free(stack);
	       SIMPLE_OUT_OF_MEMORY_ERROR("diff_longest_sequence",
					  sizeof(struct diff_magic_link_pool) +
					  sizeof(struct diff_magic_link) *
					  DMLPOOLSIZE);
	     }

	     dml->x = x;
	     dml->refs = 1;

	     if (pos)
	       (dml->prev = stack[pos-1])->refs++;
	     else
	       dml->prev = NULL;

	     top++;
	    
	     stack[pos] = dml;
	   } else if (pos && 
		      stack[pos]->refs == 1 &&
		      stack[pos-1] == stack[pos]->prev)
	   {
#ifdef DIFF_DEBUG
	     fprintf(stderr, "DIFF:  Optimized case\n");
#endif /* DIFF_DEBUG */

	     /* Optimization. */
	     stack[pos]->x = x;
	   } else {
#ifdef DIFF_DEBUG
	     fprintf(stderr, "DIFF:  Generic case\n");
#endif /* DIFF_DEBUG */

	     if (! (dml=dml_new(&pools)) )
	     {
	       int args = 0;
	       dml_free_pools(pools);
	       free(stack);
	       SIMPLE_OUT_OF_MEMORY_ERROR("diff_longest_sequence",
					  sizeof(struct diff_magic_link_pool) +
					  sizeof(struct diff_magic_link) *
					  DMLPOOLSIZE);
	     }

	     dml->x = x;
	     dml->refs = 1;

	     if (pos)
	       (dml->prev = stack[pos-1])->refs++;
	     else
	       dml->prev = NULL;

	     if (!--stack[pos]->refs)
	       dml_delete(pools, stack[pos]);
	    
	     stack[pos] = dml;
	   }
#ifdef DIFF_DEBUG
	 } else {
	   fprintf(stderr, "DIFF:  Already marked (%d)!\n", marks[x]);
#endif /* DIFF_DEBUG */
	 }
      }
#ifdef DIFF_DEBUG
      for(j=0; j < top; j++) {
	fprintf(stderr, "DIFF:  stack:%d, mark:%d\n",
		stack[j]->x, marks[stack[j]->x]);
      }
#endif /* DIFF_DEBUG */
   }

   /* No need for marks anymore. */

   free(marks);

   /* FIXME(?) memory unfreed upon error here. */
   a=low_allocate_array(top,0); 
   if (top)
   {
       dml=stack[top-1];
       while (dml)
       {
	  a->item[--top].u.integer=dml->x;
	  dml=dml->prev;
       }
   }

   free(stack);
   dml_free_pools(pools);
   return a;
}

/*
 * The dynamic programming Longest Common Sequence algorithm.
 *
 * This algorithm is O(Na * Nb), where:
 *
 *  Na == sizeof(a)
 *  Nb == sizeof(b)
 *
 * This makes it faster than the G-M algorithm on binary data,
 * but slower on ascii data.
 *
 * NOT true! The G-M algorithm seems to be faster on most data anyway.
 *	/grubba 1998-05-19
 */
static struct array *diff_dyn_longest_sequence(struct array *cmptbl, int blen)
{
  struct array *res = NULL;
  struct diff_magic_link_head *table = NULL;
  struct diff_magic_link_pool *dml_pool = NULL;
  struct diff_magic_link *dml;
  unsigned int sz = (unsigned int)cmptbl->size;
  unsigned int i;
  unsigned int off1 = 0;
  unsigned int off2 = blen + 1;
  unsigned int l1 = 0;
  unsigned int l2 = 0;

  table = calloc(sizeof(struct diff_magic_link_head)*2, off2);
  if (!table) {
    int args = 0;
    SIMPLE_OUT_OF_MEMORY_ERROR("diff_dyn_longest_sequence",
			       sizeof(struct diff_magic_link_head) * 2 * off2);
  }

  /* FIXME: Assumes NULL is represented with all zeroes */
  /* NOTE: Scan strings backwards to get the same result as the G-M
   * algorithm.
   */
  for (i = sz; i--;) {
    struct array *boff = cmptbl->item[i].u.array;

#ifdef DIFF_DEBUG
    fprintf(stderr, "  i:%d\n", i);
#endif /* DIFF_DEBUG */

    if (boff->size) {
      unsigned int bi;
      unsigned int base = blen;
      unsigned int tmp = off1;
      off1 = off2;
      off2 = tmp;

      for (bi = boff->size; bi--;) {
	unsigned int ib = boff->item[bi].u.integer;

#ifdef DIFF_DEBUG
	fprintf(stderr, "    Range [%d - %d] differ\n", base - 1, ib + 1);
#endif /* DIFF_DEBUG */
	while ((--base) > ib) {
	  /* Differ */
	  if (table[off1 + base].link) {
	    if (!--(table[off1 + base].link->refs)) {
	      dml_delete(dml_pool, table[off1 + base].link);
	    }
	  }
	  /* FIXME: Should it be > or >= here to get the same result
	   * as with the G-M algorithm?
	   */
	  if (table[off2 + base].depth > table[off1 + base + 1].depth) {
	    table[off1 + base].depth = table[off2 + base].depth;
	    dml = (table[off1 + base].link = table[off2 + base].link);
	  } else {
	    table[off1 + base].depth = table[off1 + base + 1].depth;
	    dml = (table[off1 + base].link = table[off1 + base + 1].link);
	  }
	  if (dml) {
	    dml->refs++;
	  }
	}
	/* Equal */
#ifdef DIFF_DEBUG
	fprintf(stderr, "    Equal\n");
#endif /* DIFF_DEBUG */

	if (table[off1 + ib].link) {
	  if (!--(table[off1 + ib].link->refs)) {
	    dml_delete(dml_pool, table[off1 + ib].link);
	  }
	}
	table[off1 + ib].depth = table[off2 + ib + 1].depth + 1;
	dml = (table[off1 + ib].link = dml_new(&dml_pool));
	if (!dml) {
	  int args = 0;
	  dml_free_pools(dml_pool);
	  free(table);
	  SIMPLE_OUT_OF_MEMORY_ERROR("diff_dyn_longest_sequence",
				     sizeof(struct diff_magic_link_pool) +
				     sizeof(struct diff_magic_link) *
				     DMLPOOLSIZE);
	}
	dml->refs = 1;
	dml->prev = table[off2 + ib + 1].link;
	if (dml->prev) {
	  dml->prev->refs++;
	}
	dml->x = ib;
      }
#ifdef DIFF_DEBUG
      fprintf(stderr, "    Range [0 - %d] differ\n", base-1);
#endif /* DIFF_DEBUG */
      while (base--) {
	/* Differ */
	if (table[off1 + base].link) {
	  if (!--(table[off1 + base].link->refs)) {
	    dml_delete(dml_pool, table[off1 + base].link);
	  }
	}
	/* FIXME: Should it be > or >= here to get the same result
	 * as with the G-M algorithm?
	 */
	if (table[off2 + base].depth > table[off1 + base + 1].depth) {
	  table[off1 + base].depth = table[off2 + base].depth;
	  dml = (table[off1 + base].link = table[off2 + base].link);
	} else {
	  table[off1 + base].depth = table[off1 + base + 1].depth;
	  dml = (table[off1 + base].link = table[off1 + base + 1].link);
	}
	if (dml) {
	  dml->refs++;
	}
      }
    }
  }

  /* Convert table into res */
  sz = table[off1].depth;
  dml = table[off1].link;
  free(table);
#ifdef DIFF_DEBUG
  fprintf(stderr, "Result array size:%d\n", sz);
#endif /* DIFF_DEBUG */

  res = allocate_array(sz);
  if (!res) {
    int args = 0;
    if (dml_pool) {
      dml_free_pools(dml_pool);
    }
    SIMPLE_OUT_OF_MEMORY_ERROR("diff_dyn_longest_sequence",
			       sizeof(struct array) +
			       sz*sizeof(struct svalue));
  }

  i = 0;
  while(dml) {
#ifdef PIKE_DEBUG
    if (i >= sz) {
      fatal("Consistency error in diff_dyn_longest_sequence()\n");
    }
#endif /* PIKE_DEBUG */
#ifdef DIFF_DEBUG
    fprintf(stderr, "  %02d: %d\n", i, dml->x);
#endif /* DIFF_DEBUG */
    res->item[i].type = T_INT;
    res->item[i].subtype = 0;
    res->item[i].u.integer = dml->x;
    dml = dml->prev;
    i++;
  }
#ifdef PIKE_DEBUG
  if (i != sz) {
    fatal("Consistency error in diff_dyn_longest_sequence()\n");
  }
#endif /* PIKE_DEBUG */

  dml_free_pools(dml_pool);
  return(res);
}

static struct array* diff_build(struct array *a,
				struct array *b,
				struct array *seq)
{
   struct array *ad,*bd;
   int bi,ai,lbi,lai,i,eqstart;

   /* FIXME(?) memory unfreed upon error here (and later) */
   ad=low_allocate_array(0,32);
   bd=low_allocate_array(0,32);
   
   eqstart=0;
   lbi=bi=ai=-1;
   for (i=0; i<seq->size; i++)
   {
      bi=seq->item[i].u.integer;

      if (bi!=lbi+1 || !is_equal(a->item+ai+1,b->item+bi))
      {
	 /* insert the equality */
	 if (lbi>=eqstart)
	 {
	    push_array(friendly_slice_array(b,eqstart,lbi+1));
	    ad=append_array(ad,sp-1);
	    bd=append_array(bd,sp-1);
	    pop_stack();
	 }
	 /* insert the difference */
	 lai=ai;
	 ai=array_search(a,b->item+bi,ai+1)-1;

	 push_array(friendly_slice_array(b,lbi+1,bi));
	 bd=append_array(bd, sp-1);
	 pop_stack();

	 push_array(friendly_slice_array(a,lai+1,ai+1));
	 ad=append_array(ad,sp-1);
	 pop_stack();

	 eqstart=bi;
      }
      ai++;
      lbi=bi;
   }

   if (lbi>=eqstart)
   {
      push_array(friendly_slice_array(b,eqstart,lbi+1));
      ad=append_array(ad,sp-1);
      bd=append_array(bd,sp-1);
      pop_stack();
   }

   if (b->size>bi+1 || a->size>ai+1)
   {
      push_array(friendly_slice_array(b,lbi+1,b->size));
      bd=append_array(bd, sp-1);
      pop_stack();
      
      push_array(friendly_slice_array(a,ai+1,a->size));
      ad=append_array(ad,sp-1);
      pop_stack();
   }

   push_array(ad);
   push_array(bd);
   return aggregate_array(2);
}

void f_diff(INT32 args)
{
   struct array *seq;
   struct array *cmptbl;
   struct array *diff;
   int uniq;

   /* FIXME: Ought to use get_all_args() */

   if (args<2)
      SIMPLE_TOO_FEW_ARGS_ERROR("diff", 2);

   if (sp[-args].type != T_ARRAY)
     SIMPLE_BAD_ARG_ERROR("diff", 1, "array");
   if (sp[1-args].type != T_ARRAY)
     SIMPLE_BAD_ARG_ERROR("diff", 2, "array");

   cmptbl = diff_compare_table(sp[-args].u.array, sp[1-args].u.array, &uniq);

   push_array(cmptbl);
#ifdef ENABLE_DYN_DIFF
   if (uniq * 100 > sp[1-args].u.array->size) {
#endif /* ENABLE_DYN_DIFF */
#ifdef DIFF_DEBUG
     fprintf(stderr, "diff: Using G-M algorithm, u:%d, s:%d\n",
	     uniq, sp[1-args].u.array->size);
#endif /* DIFF_DEBUG */
     seq = diff_longest_sequence(cmptbl, sp[1-1-args].u.array->size);
#ifdef ENABLE_DYN_DIFF
   } else {
#ifdef DIFF_DEBUG
     fprintf(stderr, "diff: Using dyn algorithm, u:%d, s:%d\n",
	     uniq, sp[1-args].u.array->size);
#endif /* DIFF_DEBUG */
     seq = diff_dyn_longest_sequence(cmptbl, sp[1-1-args].u.array->size);
   }     
#endif /* ENABLE_DYN_DIFF */
   push_array(seq);
   
   diff=diff_build(sp[-2-args].u.array,sp[1-2-args].u.array,seq);

   pop_n_elems(2+args);
   push_array(diff);
}

void f_diff_compare_table(INT32 args)
{
  struct array *a;
  struct array *b;
  struct array *cmptbl;

  get_all_args("diff_compare_table", args, "%a%a", &a, &b);

  cmptbl = diff_compare_table(a, b, NULL);

  pop_n_elems(args);
  push_array(cmptbl);
}

void f_diff_longest_sequence(INT32 args)
{
  struct array *a;
  struct array *b;
  struct array *seq;
  struct array *cmptbl;

  get_all_args("diff_longest_sequence", args, "%a%a", &a, &b);

  cmptbl = diff_compare_table(a, b, NULL);

  push_array(cmptbl);

  seq = diff_longest_sequence(cmptbl, b->size);

  pop_n_elems(args+1);
  push_array(seq); 
}

void f_diff_dyn_longest_sequence(INT32 args)
{
  struct array *a;
  struct array *b;
  struct array *seq;
  struct array *cmptbl;

  get_all_args("diff_dyn_longest_sequence", args, "%a%a", &a, &b);

  cmptbl=diff_compare_table(a, b, NULL);

  push_array(cmptbl);

  seq = diff_dyn_longest_sequence(cmptbl, b->size);

  pop_n_elems(args+1);
  push_array(seq); 
}

/**********************************************************************/

static struct callback_list memory_usage_callback;

struct callback *add_memory_usage_callback(callback_func call,
					  void *arg,
					  callback_func free_func)
{
  return add_to_callback(&memory_usage_callback, call, arg, free_func);
}


void f__memory_usage(INT32 args)
{
  INT32 num,size;
  struct svalue *ss;
  pop_n_elems(args);
  ss=sp;

  count_memory_in_mappings(&num, &size);
  push_text("num_mappings");
  push_int(num);
  push_text("mapping_bytes");
  push_int(size);

  count_memory_in_strings(&num, &size);
  push_text("num_strings");
  push_int(num);
  push_text("string_bytes");
  push_int(size);

  count_memory_in_arrays(&num, &size);
  push_text("num_arrays");
  push_int(num);
  push_text("array_bytes");
  push_int(size);

  count_memory_in_programs(&num,&size);
  push_text("num_programs");
  push_int(num);
  push_text("program_bytes");
  push_int(size);

  count_memory_in_multisets(&num, &size);
  push_text("num_multisets");
  push_int(num);
  push_text("multiset_bytes");
  push_int(size);

  count_memory_in_objects(&num, &size);
  push_text("num_objects");
  push_int(num);
  push_text("object_bytes");
  push_int(size);

  count_memory_in_callbacks(&num, &size);
  push_text("num_callbacks");
  push_int(num);
  push_text("callback_bytes");
  push_int(size);

  count_memory_in_callables(&num, &size);
  push_text("num_callables");
  push_int(num);
  push_text("callable_bytes");
  push_int(size);

  count_memory_in_pike_frames(&num, &size);
  push_text("num_frames");
  push_int(num);
  push_text("frame_bytes");
  push_int(size);

  call_callback(&memory_usage_callback, (void *)0);

  f_aggregate_mapping(sp-ss);
}

void f__next(INT32 args)
{
  struct svalue tmp;

  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY, ("_next: permission denied.\n"));

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("_next", 1);
  
  pop_n_elems(args-1);
  args = 1;
  tmp=sp[-1];
  switch(tmp.type)
  {
  case T_OBJECT:  tmp.u.object=tmp.u.object->next; break;
  case T_ARRAY:   tmp.u.array=tmp.u.array->next; break;
  case T_MAPPING: tmp.u.mapping=tmp.u.mapping->next; break;
  case T_MULTISET:tmp.u.multiset=tmp.u.multiset->next; break;
  case T_PROGRAM: tmp.u.program=tmp.u.program->next; break;
  case T_STRING:  tmp.u.string=tmp.u.string->next; break;
  default:
    SIMPLE_BAD_ARG_ERROR("_next", 1,
			 "object|array|mapping|multiset|program|string");
  }
  if(tmp.u.refs)
  {
    assign_svalue(sp-1,&tmp);
  }else{
    pop_stack();
    push_int(0);
  }
}

void f__prev(INT32 args)
{
  struct svalue tmp;

  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY, ("_prev: permission denied.\n"));

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("_prev", 1);
  
  pop_n_elems(args-1);
  args = 1;
  tmp=sp[-1];
  switch(tmp.type)
  {
  case T_OBJECT:  tmp.u.object=tmp.u.object->prev; break;
  case T_ARRAY:   tmp.u.array=tmp.u.array->prev; break;
  case T_MAPPING: tmp.u.mapping=tmp.u.mapping->prev; break;
  case T_MULTISET:tmp.u.multiset=tmp.u.multiset->prev; break;
  case T_PROGRAM: tmp.u.program=tmp.u.program->prev; break;
  default:
    SIMPLE_BAD_ARG_ERROR("_prev", 1, "object|array|mapping|multiset|program");
  }
  if(tmp.u.refs)
  {
    assign_svalue(sp-1,&tmp);
  }else{
    pop_stack();
    push_int(0);
  }
}

void f__refs(INT32 args)
{
  INT32 i;

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("_refs", 1);

  if(sp[-args].type > MAX_REF_TYPE)
    SIMPLE_BAD_ARG_ERROR("refs", 1,
			 "array|mapping|multiset|object|"
			 "function|program|string");

  i=sp[-args].u.refs[0];
  pop_n_elems(args);
  push_int(i);
}


/* This function is for debugging *ONLY*
 * do not document please. /Hubbe
 */
void f__leak(INT32 args)
{
  INT32 i;

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("_leak", 1);

  if(sp[-args].type > MAX_REF_TYPE)
    SIMPLE_BAD_ARG_ERROR("_leak", 1,
			 "array|mapping|multiset|object|"
			 "function|program|string");

  add_ref(sp[-args].u.array);
  i=sp[-args].u.refs[0];
  pop_n_elems(args);
  push_int(i);
}

void f__typeof(INT32 args)
{
  INT32 i;
  struct pike_string *s,*t;
  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("_typeof", 1);

  low_init_threads_disable();
  s=get_type_of_svalue(sp-args);
  t=describe_type(s);
  exit_threads_disable(NULL);

  free_string(s);
  pop_n_elems(args);
  push_string(t);
}

void f_replace_master(INT32 args)
{
  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY,
			  ("replace_master: permission denied.\n"));

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("replace_master", 1);
  if(sp[-args].type != T_OBJECT)
    SIMPLE_BAD_ARG_ERROR("replace_master", 1, "object");
 if(!sp[-args].u.object->prog)
    bad_arg_error("replace_master", sp-args, args, 1, "object", sp-args,
		  "Called with destructed object.\n");
    
  free_object(master_object);
  master_object=sp[-args].u.object;
  add_ref(master_object);

  free_program(master_program);
  master_program=master_object->prog;
  add_ref(master_program);

  pop_n_elems(args);
}

void f_master(INT32 args)
{
  pop_n_elems(args);
  ref_push_object(master());
}

#ifdef HAVE_GETHRVTIME
#include <sys/time.h>

void f_gethrvtime(INT32 args)
{
  pop_n_elems(args);
  push_int64(gethrvtime()/1000);
}

void f_gethrtime(INT32 args)
{
  pop_n_elems(args);
  if(args)
    push_int64(gethrtime()); 
  else
    push_int64(gethrtime()/1000);
}
#else
void f_gethrtime(INT32 args)
{
  struct timeval tv;
  pop_n_elems(args);
  GETTIMEOFDAY(&tv);
#ifdef INT64
  if(args)
    push_int64((((INT64)tv.tv_sec * 1000000) + tv.tv_usec)*1000);
  else
    push_int64(((INT64)tv.tv_sec * 1000000) + tv.tv_usec);
#else /* !INT64 */
  if(args)
    push_int64(((tv.tv_sec * 1000000) + tv.tv_usec)*1000);
  else
    push_int64((tv.tv_sec * 1000000) + tv.tv_usec);
#endif /* INT64 */
}
#endif /* HAVE_GETHRVTIME */

#ifdef PROFILING
static void f_get_prof_info(INT32 args)
{
  struct program *prog = 0;
  int num_functions;
  int i;

  if (!args) {
    SIMPLE_TOO_FEW_ARGS_ERROR("get_profiling_info", 1);
  }
  prog = program_from_svalue(sp-args);
  if(!prog)
    SIMPLE_BAD_ARG_ERROR("get_profiling_info", 1, "program|function|object");

  /* ({ num_clones, ([ "fun_name":({ num_calls, total_time, self_time }) ]) })
   */

  pop_n_elems(args-1);
  args = 1;

  push_int(prog->num_clones);

  for(num_functions=i=0; i<(int)prog->num_identifiers; i++) {
    if (prog->identifiers[i].num_calls)
    {
      num_functions++;
      ref_push_string(prog->identifiers[i].name);

      push_int(prog->identifiers[i].num_calls);
      push_int(prog->identifiers[i].total_time);
      push_int(prog->identifiers[i].self_time);
      f_aggregate(3);
    }
  }
  f_aggregate_mapping(num_functions * 2);
  f_aggregate(2);

  stack_swap();
  pop_stack();
}
#endif /* PROFILING */

void f_object_variablep(INT32 args)
{
  struct object *o;
  struct pike_string *s;
  int ret;

  get_all_args("variablep",args,"%o%S",&o, &s);

  if(!o->prog)
    bad_arg_error("variablep", sp-args, args, 1, "object", sp-args,
		  "Called on destructed object.\n");

  ret=find_shared_string_identifier(s,o->prog);
  if(ret!=-1)
  {
    ret=IDENTIFIER_IS_VARIABLE(ID_FROM_INT(o->prog, ret)->identifier_flags);
  }else{
    ret=0;
  }
  pop_n_elems(args);
  push_int(!!ret);
}



void f_splice(INT32 args)
{
  struct array *out;
  INT32 size=0x7fffffff;
  INT32 i,j,k;

#ifdef PIKE_DEBUG
  if(args < 0) fatal("Negative args to f_splice()\n");
#endif

  for(i=0;i<args;i++)
    if (sp[i-args].type!=T_ARRAY) 
      SIMPLE_BAD_ARG_ERROR("splice", i+1, "array");
    else
      if (sp[i-args].u.array->size < size)
	size=sp[i-args].u.array->size;

  out=allocate_array(args * size);
  if (!args)
  {
    push_array(out);
    return;
  }

  out->type_field=0;
  for(i=-args; i<0; i++) out->type_field|=sp[i].u.array->type_field;

  for(k=j=0; j<size; j++)
    for(i=-args; i<0; i++)
      assign_svalue_no_free(out->item+(k++), sp[i].u.array->item+j);

  pop_n_elems(args);
  push_array(out);
  return;
}

void f_everynth(INT32 args)
{
  INT32 k,n=2;
  INT32 start=0;
  struct array *a;
  struct array *ina;
  INT32 size=0;
#ifdef PIKE_DEBUG
  if(args < 0) fatal("Negative args to f_everynth()\n");
#endif

  check_all_args("everynth", args,
		 BIT_ARRAY, BIT_INT | BIT_VOID, BIT_INT | BIT_VOID , 0);

  switch(args)
  {
    default:
    case 3:
     start=sp[2-args].u.integer;
     if(start<0)
       bad_arg_error("everynth", sp-args, args, 3, "int", sp+2-args,
		     "Argument negative.\n");
    case 2:
      n=sp[1-args].u.integer;
      if(n<1)
	bad_arg_error("everynth", sp-args, args, 2, "int", sp+1-args,
		      "Argument negative.\n");
    case 1:
      ina=sp[-args].u.array;
  }

  a=allocate_array(((size=ina->size)-start+n-1)/n);
  for(k=0; start<size; start+=n)
    assign_svalue_no_free(a->item+(k++), ina->item+start);

  a->type_field=ina->type_field;
  pop_n_elems(args);
  push_array(a);
  return;
}


void f_transpose(INT32 args)
{
  struct array *out;
  struct array *in;
  struct array *outinner;
  struct array *ininner;
  INT32 sizeininner=0,sizein=0;
  INT32 inner=0;
  INT32 j,i;
  TYPE_FIELD type=0;
#ifdef PIKE_DEBUG
  if(args < 0) fatal("Negative args to f_transpose()\n");
#endif
  
  if (args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("transpose", 1);

  if (sp[-args].type!=T_ARRAY) 
    SIMPLE_BAD_ARG_ERROR("transpose", 1, "array(array)");

  in=sp[-args].u.array;
  sizein=in->size;

  if(!sizein)
  {
    pop_n_elems(args);
    out=allocate_array(0);
    push_array(out);
    return; 
  }

  if(in->type_field != BIT_ARRAY)
  {
    array_fix_type_field(in);
    if(!in->type_field || in->type_field & ~BIT_ARRAY)
      error("The array given as argument 1 to transpose must contain arrays only.\n");
  }

  sizeininner=in->item->u.array->size;

  for(i=1 ; i<sizein; i++)
    if (sizeininner!=(in->item+i)->u.array->size)
      error("The array given as argument 1 to transpose must contain arrays of the same size.\n");

  out=allocate_array(sizeininner);

  for(i=0; i<sizein; i++)
    type|=in->item[i].u.array->type_field;
  
  for(j=0; j<sizeininner; j++)
  {
    struct svalue * ett;
    struct svalue * tva;

    outinner=allocate_array(sizein);
    ett=outinner->item;
    tva=in->item;
    for(i=0; i<sizein; i++)
      assign_svalue_no_free(ett+i, tva[i].u.array->item+j);

    outinner->type_field=type;
    out->item[j].u.array=outinner;
    out->item[j].type=T_ARRAY;
  }

  out->type_field=BIT_ARRAY;
  pop_n_elems(args);
  push_array(out);
  return;
}

#ifdef DEBUG_MALLOC
void f__reset_dmalloc(INT32 args)
{
  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY,
			  ("_reset_dmalloc: permission denied.\n"));
  pop_n_elems(args);
  reset_debug_malloc();
}

void f__dmalloc_set_name(INT32 args)
{
  char *s;
  INT_TYPE i;
  extern char * dynamic_location(const char *file, int line);
  extern char * dmalloc_default_location;

  if(args)
  {
    get_all_args("_dmalloc_set_name", args, "%s%i", &s, &i);
    dmalloc_default_location = dynamic_location(s, i);
  }else{
    dmalloc_default_location=0;
  }
  pop_n_elems(args);
}

void f__list_open_fds(INT32 args)
{
  extern void list_open_fds(void);
  list_open_fds();
}
#endif

#ifdef PIKE_DEBUG
void f__locate_references(INT32 args)
{
  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY,
			  ("_locate_references: permission denied.\n"));
  if(args)
    locate_references(sp[-args].u.refs);
  pop_n_elems(args-1);
}

void f__describe(INT32 args)
{
  struct svalue *s;

  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY,
			  ("_optimizer_debug: permission denied.\n"));
  get_all_args("_describe", args, "%*", &s);
  debug_describe_svalue(debug_malloc_pass(s));
  pop_n_elems(args-1);
}

#endif

void f_map_array(INT32 args)
{
  ONERROR tmp;
  INT32 e;
  struct svalue *fun;
  struct array *ret,*foo;

  if (args < 2)
    SIMPLE_TOO_FEW_ARGS_ERROR("map_array", 2);

  if(sp[-args].type != T_ARRAY)
    SIMPLE_BAD_ARG_ERROR("map_array", 1, "array");
  
  foo=sp[-args].u.array;
  fun=sp-args+1;

  ret=allocate_array(foo->size);
  SET_ONERROR(tmp, do_free_array, ret);
  for(e=0;e<foo->size;e++)
  {
    push_svalue(foo->item+e);
    assign_svalues_no_free(sp,fun+1,args-2,-1);
    sp+=args-2;
    apply_svalue(fun,args-1);
    ret->item[e]=*(--sp);
    dmalloc_touch_svalue(sp);
  }
  pop_n_elems(args);
  UNSET_ONERROR(tmp);
  push_array(ret);
}

void f_map(INT32 args)
{
   /*
     map(arr,fun,extra) => ret

     arr	fun		goes
     array	function |	array ret; ret[i]=fun(arr[i],@extra);
       		program |	
           	object | 
                array

     array	multiset |	ret = rows(fun,arr)
     		mapping 

     array	string  	array ret; ret[i]=arr[i][fun](@extra);

     array      void|int(0)     array ret; ret=arr(@extra)

     mapping |	*               mapping ret = 
     program |                     mkmapping(indices(arr),
     function			             map(values(arr),fun,@extra));

     multiset	*               multiset ret = 
                                   (multiset)(map(indices(arr),fun,@extra));

     string     *               string ret = 
                                   (string)map((array)arr,fun,@extra);

     object	*               if arr->cast :              
                                   try map((array)arr,fun,@extra);
                                   try map((mapping)arr,fun,@extra);
				   try map((multiset)arr,fun,@extra);
                                if arr->_sizeof && arr->`[] 
                                   array ret; ret[i]=arr[i];
				   ret=map(ret,fun,@extra);
                                error

     *          *               error (ie arr or fun = float or bad int )

    */

   struct svalue *mysp;
   struct array *a,*d,*f;
   int splice,i,n;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("map", 1);
   else if (args<2)
      { push_int(0); args++; }

   switch (sp[-args].type)
   {
      case T_ARRAY:
	 break;

      case T_MAPPING:
      case T_PROGRAM:
      case T_FUNCTION:
	 /* mapping ret =                             
	       mkmapping(indices(arr),                
	                 map(values(arr),fun,@extra)); */
	 f_aggregate(args-2);
	 mysp=sp;
	 splice=mysp[-1].u.array->size;

	 push_svalue(mysp-3); /* arr */
	 f_values(1);
	 push_svalue(mysp-2); /* fun */
	 *sp=mysp[-1];        /* extra */
	 mysp[-1].type=T_INT;
	 push_array_items(sp->u.array);
	 f_map(splice+2);     /* ... arr fun extra -> ... retval */
	 stack_pop_n_elems_keep_top(2); /* arr fun extra ret -> arr retval */
	 stack_swap();        /* retval arr */
	 f_indices(1);        /* retval retind */
	 stack_swap();        /* retind retval */
	 f_mkmapping(2);      /* ret :-) */
	 return;

      case T_MULTISET:
	 /* multiset ret =                             
	       (multiset)(map(indices(arr),fun,@extra)); */
	 push_svalue(sp-args);      /* take indices from arr */
	 free_svalue(sp-args-1);    /* move it to top of stack */
	 sp[-args-1].type=T_INT;    
	 f_indices(1);              /* call f_indices */
	 sp--;
	 dmalloc_touch_svalue(sp);
	 sp[-args]=sp[0];           /* move it back */
	 f_map(args);               
	 sp--;                      /* allocate_multiset is destructive */
	 dmalloc_touch_svalue(sp);
	 push_multiset(allocate_multiset(sp->u.array));
	 return;

      case T_STRING:
	 /* multiset ret =                             
	       (string)(map((array)arr,fun,@extra)); */
	 push_svalue(sp-args);      /* take indices from arr */
	 free_svalue(sp-args-1);    /* move it to top of stack */
	 sp[-args-1].type=T_INT;    
	 o_cast(NULL,T_ARRAY);      /* cast the string to an array */
	 sp--;                       
	 dmalloc_touch_svalue(sp);
	 sp[-args]=sp[0];           /* move it back */
	 f_map(args);               
	 o_cast(NULL,T_STRING);     /* cast the array to a string */
	 return;

      case T_OBJECT:
	 /* if arr->cast :              
               try map((array)arr,fun,@extra);
               try map((mapping)arr,fun,@extra);
               try map((multiset)arr,fun,@extra); */

	 mysp=sp+3-args;

	 push_svalue(mysp-3);
	 push_constant_text("cast");
	 f_arrow(2);
	 if (!IS_ZERO(sp-1))
	 {
	    pop_stack();

	    push_constant_text("array");
	    safe_apply(mysp[-3].u.object,"cast",1);
	    if (sp[-1].type==T_ARRAY)
	    {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--sp);
	       dmalloc_touch_svalue(sp);
	       f_map(args);
	       return;
	    }
	    pop_stack();

	    push_constant_text("mapping");
	    safe_apply(mysp[-3].u.object,"cast",1);
	    if (sp[-1].type==T_MAPPING)
	    {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--sp);
	       dmalloc_touch_svalue(sp);
	       f_map(args);
	       return;
	    }
	    pop_stack();

	    push_constant_text("multiset");
	    safe_apply(mysp[-3].u.object,"cast",1);
	    if (sp[-1].type==T_MULTISET)
	    {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--sp);
	       dmalloc_touch_svalue(sp);
	       f_map(args);
	       return;
	    }
	    pop_stack();
	 }
	 pop_stack();

         /* if arr->_sizeof && arr->`[] 
               array ret; ret[i]=arr[i];
               ret=map(ret,fun,@extra); */

	 /* class myarray { int a0=1,a1=2; int `[](int what) { return ::`[]("a"+what); } int _sizeof() { return 2; } } 
	    map(myarray(),lambda(int in){ werror("in=%d\n",in); }); */

	 push_svalue(mysp-3);
	 push_constant_text("`[]");
	 f_arrow(2);
	 push_svalue(mysp-3);
	 push_constant_text("_sizeof");
	 f_arrow(2);
	 if (!IS_ZERO(sp-2)&&!IS_ZERO(sp-1))
	 {
	    f_call_function(1);
	    if (sp[-1].type!=T_INT)
	       SIMPLE_BAD_ARG_ERROR("map", 1, 
				    "object sizeof() returning integer");
	    n=sp[-1].u.integer;
	    pop_stack();
	    push_array(d=allocate_array(n));
	    stack_swap();
	    for (i=0; i<n; i++)
	    {
	       stack_dup(); /* `[] */
	       push_int(i);
	       f_call_function(2);
	       d->item[i]=*(--sp);
	       dmalloc_touch_svalue(sp);
	    }
	    pop_stack();
	    free_svalue(mysp-3);
	    mysp[-3]=*(--sp);
	    dmalloc_touch_svalue(sp);
	    f_map(args);
	    return;
	 }
	 pop_stack();
	 pop_stack();

	 SIMPLE_BAD_ARG_ERROR("map",1,
			      "object that works in map");

      default:
	 SIMPLE_BAD_ARG_ERROR("map",1,
			      "array|mapping|program|function|"
			      "multiset|string|object");
   }

   f_aggregate(args-2);
   mysp=sp;
   splice=mysp[-1].u.array->size;

   a=mysp[-3].u.array;
   n=a->size;

   switch (mysp[-2].type)
   {
      case T_FUNCTION:
      case T_PROGRAM:
      case T_OBJECT:
      case T_ARRAY:
	 /* ret[i]=fun(arr[i],@extra); */
         push_array(d=allocate_array(n));
	 d=sp[-1].u.array;

	 if(mysp[-2].type == T_FUNCTION &&
	    mysp[-2].subtype == FUNCTION_BUILTIN)
	 {
	   c_fun fun=mysp[-2].u.efun->function;
	   struct svalue *spbase=sp;

	   if(splice)
	   {
	     for (i=0; i<n; i++)
	     {
	       push_svalue(a->item+i);
	       add_ref_svalue(mysp-1);
	       push_array_items(mysp[-1].u.array);
	       (* fun)(1+splice);
	       if(sp>spbase)
	       {
		 dmalloc_touch_svalue(sp-1);
		 d->item[i]=*--sp;
		 pop_n_elems(sp-spbase);
	       }
	     }
	   }else{
	     for (i=0; i<n; i++)
	     {
	       push_svalue(a->item+i);
	       (* fun)(1);
	       if(sp>spbase)
	       {
		 dmalloc_touch_svalue(sp-1);
		 d->item[i]=*--sp;
		 pop_n_elems(sp-spbase);
	       }
	     }
	   }
	 }else{
	   for (i=0; i<n; i++)
	   {
	     push_svalue(a->item+i);
	     if (splice) 
	     {
	       add_ref_svalue(mysp-1);
	       push_array_items(mysp[-1].u.array);
	       apply_svalue(mysp-2,1+splice);
	     }
	     else
	     {
	       apply_svalue(mysp-2,1);
	     }
	     dmalloc_touch_svalue(sp-1);
	     d->item[i]=*--sp;
	   }
	 }
	 stack_pop_n_elems_keep_top(3); /* fun arr extra d -> d */
	 return;

      case T_MAPPING:
      case T_MULTISET:
	 /* ret[i]=fun[arr[i]]; */
	 pop_stack();
	 stack_swap();
	 f_rows(2);
	 return; 

      case T_STRING:
	 /* ret[i]=arr[i][fun](@extra); */
         push_array(d=allocate_array(n));
	 d=sp[-1].u.array;
	 for (i=0; i<n; i++)
	 {
	    push_svalue(a->item+i);
	    push_svalue(mysp-2);
	    f_arrow(2);
	    if(IS_ZERO(sp-1))
	    {
	      pop_stack();
	      continue;
	    }
	    add_ref_svalue(mysp-1);
	    push_array_items(mysp[-1].u.array);
	    f_call_function(splice+1);
	    d->item[i]=*--sp;
	    dmalloc_touch_svalue(sp);
	 }
	 stack_pop_n_elems_keep_top(3); /* fun arr extra d -> d */
	 return;

      case T_INT:
	 if (mysp[-2].u.integer==0)
	 {
	    /* ret=arr(@extra); */
	    stack_swap(); /* arr fun extra -> arr extra fun */
	    pop_stack();  /* arr extra */
	    sp--;
	    dmalloc_touch_svalue(sp);
	    push_array_items(sp->u.array);
	    f_call_function(1+splice);
	    return;
	 }	    
	 /* no break here */
      default:
	 SIMPLE_BAD_ARG_ERROR("map",2,
			      "function|program|object|"
			      "string|int(0)|multiset");
   }      
}

void f_filter(INT32 args)
{
   /* filter(mixed arr,mixed fun,mixed ... extra) ->

        _arr__________goes________

        array		if fun is an array:
			   for (i=0; i<sizeof(arr); i++) 
				   if (fun[i]) res+=({arr[i]});
			otherwise:
			   keep=map(arr,fun,@extra);
			   for (i=0; i<sizeof(arr); i++) 
				   if (keep[i]) res+=({arr[i]});
        
        multiset	(multiset)filter((array)arr,fun,@extra);
        
        mapping |	ind=indices(arr),val=values(arr)
        program |       keep=map(val,fun,@extra);
        function     	for (i=0; i<sizeof(keep); i++) 
				if (keep[i]) res[ind[i]]=val[i];
        
        string		(string)filter( (array)arr,fun,@extra );
   
        object	        if arr->cast :              
                           try filter((array)arr,fun,@extra);
                           try filter((mapping)arr,fun,@extra);
			   try filter((multiset)arr,fun,@extra);
   */

   int n,i,m,k;
   struct array *a,*y,*f;
   struct svalue *mysp;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("filter", 1);
   
   switch (sp[-args].type)
   {
      case T_ARRAY:
	 if (args >= 2 && sp[1-args].type == T_ARRAY) {
	   if (sp[1-args].u.array->size != sp[-args].u.array->size)
	     SIMPLE_BAD_ARG_ERROR("filter", 2, "array of same size as the first");
	   pop_n_elems(args-2);
	 }
	 else {
	   MEMMOVE(sp-args+1,sp-args,args*sizeof(*sp));
	   dmalloc_touch_svalue(sp);
	   sp++;
	   add_ref_svalue(sp-args);
	   f_map(args);
	 }

	 f=sp[-1].u.array;
	 a=sp[-2].u.array;
	 n=a->size;
	 for (k=m=i=0; i<n; i++)
	    if (!IS_ZERO(f->item+i))
	    {
	       push_svalue(a->item+i);
	       if (m++>32) 
	       {
		  f_aggregate(m);
		  m=0;
		  if (++k>32) {
		    f_add(k);
		    k=1;
		  }
	       }
	    }
	 if (m || !k) {
	   f_aggregate(m);
	   k++;
	 }
	 if (k > 1) f_add(k);
	 stack_pop_n_elems_keep_top(2);
	 return;

      case T_MAPPING:
      case T_PROGRAM:
      case T_FUNCTION:
	 /* mapping ret =                             
	       mkmapping(indices(arr),                
	                 map(values(arr),fun,@extra)); */
	 MEMMOVE(sp-args+2,sp-args,args*sizeof(*sp));
	 sp+=2;
	 sp[-args-2].type=T_INT;
	 sp[-args-1].type=T_INT;

	 push_svalue(sp-args);
	 f_indices(1);
	 sp--;
	 sp[-args-2]=*sp;
	 dmalloc_touch_svalue(sp);
	 push_svalue(sp-args);
	 f_values(1);
	 sp--;
	 sp[-args-1]=*sp;
	 dmalloc_touch_svalue(sp);

	 assign_svalue(sp-args,sp-args-1); /* loop values only */
	 f_map(args);

	 y=sp[-3].u.array;
	 a=sp[-2].u.array;
	 f=sp[-1].u.array;
	 n=a->size;

	 for (m=i=0; i<n; i++)
	    if (!IS_ZERO(f->item+i)) m++;

	 push_mapping(allocate_mapping(MAXIMUM(m,4)));

	 for (i=0; i<n; i++)
	    if (!IS_ZERO(f->item+i))
	       mapping_insert(sp[-1].u.mapping,y->item+i,a->item+i);

	 stack_pop_n_elems_keep_top(3);
	 return;

      case T_MULTISET:
	 push_svalue(sp-args);      /* take indices from arr */
	 free_svalue(sp-args-1);    /* move it to top of stack */
	 sp[-args-1].type=T_INT;    
	 f_indices(1);              /* call f_indices */
	 sp--;                       
	 dmalloc_touch_svalue(sp);
	 sp[-args]=sp[0];           /* move it back */
	 f_filter(args);               
	 sp--;                      /* allocate_multiset is destructive */
	 dmalloc_touch_svalue(sp);
	 push_multiset(allocate_multiset(sp->u.array));
	 return;

      case T_STRING:
	 push_svalue(sp-args);      /* take indices from arr */
	 free_svalue(sp-args-1);    /* move it to top of stack */
	 sp[-args-1].type=T_INT;    
	 o_cast(NULL,T_ARRAY);      /* cast the string to an array */
	 sp--;                       
	 dmalloc_touch_svalue(sp);
	 sp[-args]=sp[0];           /* move it back */
	 f_filter(args);               
	 o_cast(NULL,T_STRING);     /* cast the array to a string */
	 return;

      case T_OBJECT:
	 mysp=sp+3-args;

	 push_svalue(mysp-3);
	 push_constant_text("cast");
	 f_arrow(2);
	 if (!IS_ZERO(sp-1))
	 {
	    pop_stack();

	    push_constant_text("array");
	    safe_apply(mysp[-3].u.object,"cast",1);
	    if (sp[-1].type==T_ARRAY)
	    {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--sp);
	       dmalloc_touch_svalue(sp);
	       f_filter(args);
	       return;
	    }
	    pop_stack();

	    push_constant_text("mapping");
	    safe_apply(mysp[-3].u.object,"cast",1);
	    if (sp[-1].type==T_MAPPING)
	    {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--sp);
	       dmalloc_touch_svalue(sp);
	       f_filter(args);
	       return;
	    }
	    pop_stack();

	    push_constant_text("multiset");
	    safe_apply(mysp[-3].u.object,"cast",1);
	    if (sp[-1].type==T_MULTISET)
	    {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--sp);
	       dmalloc_touch_svalue(sp);
	       f_filter(args);
	       return;
	    }
	    pop_stack();
	 }
	 pop_stack();

	 SIMPLE_BAD_ARG_ERROR("filter",1,
			      "...|object that can be cast to array, multiset or mapping");

      default:
	 SIMPLE_BAD_ARG_ERROR("filter",1,
			      "array|mapping|program|function|"
			      "multiset|string|object");
   }
}

/* map(), map_array() and filter() inherit sideeffects from their
 * second argument.
 */
static node *fix_map_node_info(node *n)
{
  int argno;
  node **cb_;
  int node_info = OPT_SIDE_EFFECT;	/* Assume worst case. */

  /* Note: argument 2 has argno 1. */
  for (argno = 1; (cb_ = my_get_arg(&_CDR(n), argno)); argno++) {
    node *cb = *cb_;

    if ((cb->token == F_CONSTANT) &&
	(cb->u.sval.type == T_FUNCTION) &&
	(cb->u.sval.subtype == FUNCTION_BUILTIN)) {
      if (cb->u.sval.u.efun->optimize == fix_map_node_info) {
	/* map(), map_array() or filter(). */
	continue;
      }
      node_info = cb->u.sval.u.efun->flags & OPT_SIDE_EFFECT;
    }
    /* FIXME: Type-checking? */
    break;
  }

  if (!cb_) {
    yyerror("Too few arguments to map() or filter()!\n");
  }

  n->node_info |= node_info;
  n->tree_info |= node_info;

  return 0;	/* continue optimization */
}

void f_enumerate(INT32 args)
{
   struct array *d;
   int i;
   INT_TYPE n;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("enumarate", 1);
   if (args<2) 
   {
      push_int(1);
      args++;
   }
   if (args<3)
   {
      push_int(0);
      args++;
   }

   if (args<=3 &&
       (sp[1-args].type==T_INT &&
	sp[2-args].type==T_INT))
   {
      INT_TYPE step,start;

      get_all_args("enumerate", args, "%i%i%i", &n, &step, &start);
      if (n<0) 
	 SIMPLE_BAD_ARG_ERROR("enumerate",1,"int(0..)");

      pop_n_elems(args);
      push_array(d=allocate_array(n));
      for (i=0; i<n; i++)
      {
	 d->item[i].u.integer=start;
	 d->item[i].type=T_INT;
	 d->item[i].subtype=NUMBER_NUMBER;
	 start+=step;
      }
   }
   else if (args<=3 &&
	    ((sp[1-args].type==T_INT ||
	      sp[1-args].type==T_FLOAT) &&
	     (sp[2-args].type==T_INT ||
	      sp[2-args].type==T_FLOAT) ) )
   {
      FLOAT_TYPE step, start;

      get_all_args("enumerate", args, "%i%F%F", &n, &step, &start);
      if (n<0) 
	 SIMPLE_BAD_ARG_ERROR("enumerate",1,"int(0..)");

      pop_n_elems(args);

      push_array(d=allocate_array(n));
      for (i=0; i<n; i++)
      {
	 d->item[i].u.float_number=start;
	 d->item[i].type=T_FLOAT;
	 start+=step;
      }
   }
   else
   {
      get_all_args("enumerate", args, "%i", &n);
      if (n<0) SIMPLE_BAD_ARG_ERROR("enumerate",1,"int(0..)");
      if (args>4) pop_n_elems(args-4);
      if (args<4)
      {
	 push_array(d=allocate_array(n));
	 push_svalue(sp-2); /* start */
	 for (i=0; i<n; i++)
	 {
	    assign_svalue_no_free(d->item+i,sp-1);
	    if (i<n-1)
	    {
	       push_svalue(sp-4); /* step */
	       f_add(2);
	    }
	 }
      }
      else
      {
	 push_array(d=allocate_array(n));
	 push_svalue(sp-3); /* start */
	 for (i=0; i<n; i++)
	 {
	    assign_svalue_no_free(d->item+i,sp-1);
	    if (i<n-1)
	    {
	       push_svalue(sp-3); /* function */
	       stack_swap();
	       push_svalue(sp-6); /* step */
	       f_call_function(3);
	    }
	 }
      }
      pop_stack();
      stack_pop_n_elems_keep_top(4);
   }
}

void f_string_count(INT32 args)
{
   struct pike_string * haystack=NULL;
   struct pike_string * needle=NULL;
   int c=0;
   int i,j;

   get_all_args("String.count",args,"%W%W",&haystack,&needle);

   switch (needle->len)
   {
      case 0:
	 switch (haystack->len)
	 {
	    case 0: c=1; break; /* "" appears one time in "" */
	    case 1: c=0; break; /* "" doesn't appear in "x" */
	    default: c=haystack->len-1; /* one time between each character */
	 }
	 break;
      case 1:
	 /* maybe optimize? */
      default:
	 for (i=0; i<haystack->len; i++)
	 {
	    j=string_search(haystack,needle,i);
	    if (j==-1) break;
	    i=j+needle->len-1;
	    c++;
	 }
	 break;
   }
   pop_n_elems(args);
   push_int(c);
}

void f_inherit_list(INT32 args)
{
  struct program *p;
  struct svalue *arg;
  struct object *par;
  int parid,e,q=0;

  get_all_args("inherit_list",args,"%*",&arg);
  if(sp[-args].type == T_OBJECT)
    f_object_program(1);
  
  p=program_from_svalue(arg);
  if(!p) 
    SIMPLE_BAD_ARG_ERROR("inherit_list", 1, "program");

  if(arg->type == T_FUNCTION)
  {
    par=arg->u.object;
    parid=arg->subtype;
  }else{
    par=0;
    parid=-1;
  }

  check_stack(p->num_inherits);
  for(e=0;e<p->num_inherits;e++)
  {
    struct inherit *in=p->inherits+e;
    if(in->inherit_level==1)
    {
      if(in->parent_offset)
      {
	struct inherit *inherit=in;
	int accumulator;
	struct object *o=par;
	int i;

	o=par;
	i=in->parent_identifier;
	accumulator=inherit->parent_offset - 1;
	while(accumulator--)
	{
	  struct program *p;
	  if(inherit->parent_offset)
	  {
	    i=o->parent_identifier;
	    o=o->parent;
	    accumulator+=inherit->parent_offset-1;
	  }else{
	    i=inherit->parent_identifier;
	    o=inherit->parent;
	  }
	  if(!o || !o->prog || i<0) break;
	  inherit=INHERIT_FROM_INT(o->prog, i);
	}
	if(o && o->prog && i>=0)
	{
	  ref_push_object(o);
	  sp[-1].subtype=i;
	  sp[-1].type=T_FUNCTION;
#ifdef PIKE_DEBUG
	  if(program_from_svalue(sp-1) != in->prog)
	    fatal("Programming error in inherit_list!\n");
#endif
	  q++;
	  continue;
	}
      }

      if(in->parent && in->parent->prog)
      {
	ref_push_object(in->parent);
	sp[-1].subtype=in->parent_identifier;
	sp[-1].type=T_FUNCTION;
#ifdef PIKE_DEBUG
	if(program_from_svalue(sp-1) != in->prog)
	  fatal("Programming error in inherit_list!\n");
#endif
      }else{
	ref_push_program(in->prog);
      }
      q++;
    }
  }
  f_aggregate(q);
}


void f_program_implements(INT32 args)
{
  struct program *p,*p2;
  int ret;
  get_all_args("Program.implements",args,"%p%p",&p,&p2);

  ret=implements(p,p2);
  pop_n_elems(args);
  push_int(ret);
}

void f_program_inherits(INT32 args)
{
  struct program *p,*p2;
  struct svalue *arg,*arg2;
  int ret;
  get_all_args("Program.inherits",args,"%p%p",&p,&p2);

  ret=!!low_get_storage(p2,p);
  pop_n_elems(args);
  push_int(ret);
}

void f_program_defined(INT32 args)
{
  struct program *p;
  get_all_args("Program.defined",args,"%p",&p);

  if(p && p->num_linenumbers)
  {
    char *tmp;
    INT32 line;
    if((tmp=get_line(p->program, p, &line)))
    {
      struct pike_string *tmp2;
      tmp2=make_shared_string(tmp);
      pop_n_elems(args);

      push_string(tmp2);
      if(line > 1)
      {
	push_constant_text(":");
	push_int(line);
	f_add(3);
      }
      return;
    }
  }

  pop_n_elems(args);
  push_int(0);
}

void f_function_defined(INT32 args)
{
  check_all_args("Function.defined",args,BIT_FUNCTION, 0);

  if(sp[-args].subtype != FUNCTION_BUILTIN &&
     sp[-args].u.object->prog)
  {
    char *tmp;
    INT32 line;
    struct identifier *id=ID_FROM_INT(sp[-args].u.object->prog,
				      sp[-args].subtype);
    if(IDENTIFIER_IS_PIKE_FUNCTION( id->identifier_flags ) &&
      id->func.offset != -1)
    {
      if((tmp=get_line(sp[-args].u.object->prog->program + id->func.offset,
		       sp[-args].u.object->prog,
		       &line)))
      {
	struct pike_string *tmp2;
	tmp2=make_shared_string(tmp);
	pop_n_elems(args);
	
	push_string(tmp2);
	push_constant_text(":");
	push_int(line);
	f_add(3);
	return;
      }
    }
  }

  pop_n_elems(args);
  push_int(0);
  
}

void f_string_width(INT32 args)
{
  struct pike_string *s;
  int ret;
  get_all_args("String.width",args,"%W",&s);
  ret=s->size_shift;
  pop_n_elems(args);
  push_int(8 * (1<<ret));
}

void init_builtin_efuns(void)
{
  struct program *pike___master_program;

  ADD_EFUN("gethrtime", f_gethrtime,
	   tFunc(tOr(tInt,tVoid),tInt), OPT_EXTERNAL_DEPEND);

#ifdef HAVE_GETHRVTIME
  ADD_EFUN("gethrvtime",f_gethrvtime,
	   tFunc(tNone,tInt),OPT_EXTERNAL_DEPEND);
#endif
  
#ifdef PROFILING
  ADD_EFUN("get_profiling_info", f_get_prof_info,
	   tFunc(tPrg,tArray), OPT_EXTERNAL_DEPEND);
#endif /* PROFILING */

  ADD_EFUN("_refs",f__refs,tFunc(tRef,tInt),OPT_EXTERNAL_DEPEND);
  ADD_EFUN("_leak",f__leak,tFunc(tRef,tInt),OPT_EXTERNAL_DEPEND);
  ADD_EFUN("_typeof",f__typeof,tFunc(tMix,tStr),0);

  /* class __master
   * Used to prototype the master object.
   */
  start_new_program();
  ADD_PROTOTYPE("_main", tFunc(tArr(tStr) tArr(tStr),tVoid), 0);

  ADD_PROTOTYPE("cast_to_object", tFunc(tString tString, tObj), 0);
  ADD_PROTOTYPE("cast_to_program", tFunc(tStr tStr tOr(tVoid, tObj), tPrg), 0);
  ADD_PROTOTYPE("compile_error", tFunc(tStr tInt tStr, tVoid), 0);
  ADD_PROTOTYPE("compile_warning", tFunc(tStr tInt tStr, tVoid), 0);
  ADD_PROTOTYPE("decode_charset", tFunc(tStr tStr, tStr), 0);
  ADD_PROTOTYPE("describe_backtrace", tFunc(tOr(tObj, tArr(tMix)), tStr), 0);
  ADD_PROTOTYPE("handle_error", tFunc(tOr(tArr(tMix),tObj), tVoid), 0);
  ADD_PROTOTYPE("handle_import",
		tFunc(tStr tOr(tStr, tVoid) tOr(tObj, tVoid), tMix), 0);
  ADD_PROTOTYPE("handle_include", tFunc(tStr tStr tInt, tStr), 0);
  ADD_PROTOTYPE("handle_inherit", tFunc(tStr tStr tOr(tObj, tVoid), tPrg), 0);
  
  /* FIXME: Are these three actually supposed to be used?
   * They are called by encode.c:rec_restore_value
   *	/grubba 2000-03-13
   */

#if 0 /* they are not required - Hubbe */
  ADD_PROTOTYPE("functionof", tFunc(tStr, tFunction), ID_OPTIONAL);
  ADD_PROTOTYPE("objectof", tFunc(tStr, tObj), ID_OPTIONAL);
  ADD_PROTOTYPE("programof", tFunc(tStr, tPrg), ID_OPTIONAL);
#endif

  ADD_PROTOTYPE("read_include", tFunc(tStr, tStr), 0);
  ADD_PROTOTYPE("resolv", tFunc(tStr tOr(tStr, tVoid), tMix), 0);

  /* These two aren't called from C-code, but are popular from other code. */
  ADD_PROTOTYPE("getenv",
		tOr(tFunc(tStr,tStr), tFunc(tNone, tMap(tStr, tStr))),
		ID_OPTIONAL);
  ADD_PROTOTYPE("putenv", tFunc(tStr tStr, tVoid), ID_OPTIONAL);
  pike___master_program = end_program();
  add_program_constant("__master", pike___master_program, 0);

  ADD_EFUN_DTYPE("replace_master", f_replace_master,
		 dtFunc(dtObjImpl(pike___master_program), dtVoid),
		 OPT_SIDE_EFFECT);

/* function(:object) */
  ADD_EFUN_DTYPE("master", f_master,
		 dtFunc(dtNone, dtObjImpl(pike___master_program)),
		 OPT_EXTERNAL_DEPEND);
  
  /* __master still contains a reference */
  free_program(pike___master_program);
  
/* function(string,void|mixed:void) */
  ADD_EFUN("add_constant", f_add_constant,
	   tFunc(tStr tOr(tVoid,tMix),tVoid),OPT_SIDE_EFFECT);

/* function(0=mixed ...:array(0)) */
#ifdef DEBUG_MALLOC
  ADD_EFUN("aggregate",_f_aggregate,
	   tFuncV(tNone,tSetvar(0,tMix),tArr(tVar(0))),OPT_TRY_OPTIMIZE);
#else
  ADD_EFUN("aggregate",f_aggregate,
	   tFuncV(tNone,tSetvar(0,tMix),tArr(tVar(0))),OPT_TRY_OPTIMIZE);
#endif
  
/* function(0=mixed ...:multiset(0)) */
  ADD_EFUN("aggregate_multiset",f_aggregate_multiset,
	   tFuncV(tNone,tSetvar(0,tMix),tSet(tVar(0))),OPT_TRY_OPTIMIZE);
  
/* function(0=mixed ...:mapping(0:0)) */
  ADD_EFUN2("aggregate_mapping",f_aggregate_mapping,
	    tFuncV(tNone,tSetvar(0,tMix),tMap(tVar(0),tVar(0))),
	    OPT_TRY_OPTIMIZE, fix_aggregate_mapping_type, 0);
  
/* function(:mapping(string:mixed)) */
  ADD_EFUN("all_constants",f_all_constants,
	   tFunc(tNone,tMap(tStr,tMix)),OPT_EXTERNAL_DEPEND);
  
/* function(int,void|0=mixed:array(0)) */
  ADD_EFUN("allocate", f_allocate,
	   tFunc(tInt tOr(tVoid,tSetvar(0,tMix)),tArr(tVar(0))), 0);
  
/* function(mixed:int) */
  ADD_EFUN("arrayp", f_arrayp,tFunc(tMix,tInt),0);
  
/* function(:array(array)) */
  ADD_EFUN("backtrace",f_backtrace,
	   tFunc(tNone,tArr(tArray)),OPT_EXTERNAL_DEPEND);

  
/* function(array,mixed:array) */
  ADD_EFUN("column",f_column,tFunc(tArray tMix,tArray),0);
  
/* function(string...:string) */
  ADD_EFUN("combine_path",f_combine_path,tFuncV(tNone,tStr,tStr),0);
  
/* function(string,object|void,mixed...:program) */
  ADD_EFUN("compile", f_compile, tFuncV(tStr tOr(tObj, tVoid),tMix,tPrg),
	   OPT_EXTERNAL_DEPEND);
  
/* function(1=mixed:1) */
  ADD_EFUN("copy_value",f_copy_value,tFunc(tSetvar(1,tMix),tVar(1)),0);
  
/* function(string:string)|function(string,string:int) */
  ADD_EFUN("crypt",f_crypt,
	   tOr(tFunc(tStr,tStr),tFunc(tStr tStr,tInt)),OPT_EXTERNAL_DEPEND);
  
/* function(int:string) */
  ADD_EFUN("ctime",f_ctime,tFunc(tInt,tStr),OPT_TRY_OPTIMIZE);
  
/* function(object|void:void) */
  ADD_EFUN("destruct",f_destruct,tFunc(tOr(tObj,tVoid),tVoid),OPT_SIDE_EFFECT);
  
/* function(mixed,mixed:int) */
  ADD_EFUN("equal",f_equal,tFunc(tMix tMix,tInt),OPT_TRY_OPTIMIZE);

  /* function(array(0=mixed),int|void,int|void:array(0)) */
  ADD_FUNCTION("everynth",f_everynth,
	       tFunc(tArr(tSetvar(0,tMix)) tOr(tInt,tVoid) tOr(tInt,tVoid),
		     tArr(tVar(0))), 0);
  
/* function(int:void) */
  ADD_EFUN("exit",f_exit,tFunc(tInt,tVoid),OPT_SIDE_EFFECT);
  
/* function(int:void) */
  ADD_EFUN("_exit",f__exit,tFunc(tInt,tVoid),OPT_SIDE_EFFECT);
  
/* function(mixed:int) */
  ADD_EFUN("floatp",  f_floatp,tFunc(tMix,tInt),OPT_TRY_OPTIMIZE);
  
/* function(function:string) */
  ADD_EFUN("function_name",f_function_name,
	   tFunc(tFunction,tStr),OPT_TRY_OPTIMIZE);
  
/* function(function:object) */
  ADD_EFUN("function_object",f_function_object,
	   tFunc(tFunction,tObj),OPT_TRY_OPTIMIZE);
  
/* function(mixed:int) */
  ADD_EFUN("functionp",  f_functionp,tFunc(tMix,tInt),OPT_TRY_OPTIMIZE);
  
/* function(string,string:int)|function(string,string*:array(string)) */
  ADD_EFUN("glob",f_glob,
	   tOr(tFunc(tStr tStr,tInt),tFunc(tStr tArr(tStr),tArr(tStr))),
	   OPT_TRY_OPTIMIZE);
  
/* function(string,int|void:int) */
  ADD_EFUN("hash",f_hash,tFunc(tStr tOr(tInt,tVoid),tInt),OPT_TRY_OPTIMIZE);
  
/* function(string|array:int*)|function(mapping(1=mixed:mixed)|multiset(1=mixed):array(1))|function(object|program:string*) */
  ADD_EFUN2("indices",f_indices,
	   tOr3(tFunc(tOr(tStr,tArray),tArr(tInt)),
		tFunc(tOr(tMap(tSetvar(1,tMix),tMix),tSet(tSetvar(1,tMix))),
		      tArr(tVar(1))),
		tFunc(tOr(tObj,tPrg),tArr(tStr))),
	    OPT_TRY_OPTIMIZE,fix_indices_type,0);
  
/* function(mixed:int) */
  ADD_EFUN("intp", f_intp,tFunc(tMix,tInt),OPT_TRY_OPTIMIZE);
  
/* function(mixed:int) */
  ADD_EFUN("multisetp", f_multisetp,tFunc(tMix,tInt),OPT_TRY_OPTIMIZE);
  
/* function(string:string) */
  ADD_EFUN("lower_case",f_lower_case,tFunc(tStr,tStr),OPT_TRY_OPTIMIZE);
  
/* function(0=mapping,mixed:0) */
  ADD_EFUN("m_delete",f_m_delete,tFunc(tSetvar(0,tMapping) tMix,tVar(0)),
	   OPT_SIDE_EFFECT);
  
/* function(mixed:int) */
  ADD_EFUN("mappingp",f_mappingp,tFunc(tMix,tInt),OPT_TRY_OPTIMIZE);
  
/* function(array(1=mixed),array(2=mixed):mapping(1:2)) */
  ADD_EFUN("mkmapping",f_mkmapping,
	   tFunc(tArr(tSetvar(1,tMix)) tArr(tSetvar(2,tMix)),
		 tMap(tVar(1),tVar(2))),OPT_TRY_OPTIMIZE);

  ADD_EFUN("mkmultiset",f_mkmultiset,
	   tFunc(tArr(tSetvar(1,tMix)), tSet(tVar(1))),
	   OPT_TRY_OPTIMIZE);
  
/* function(1=mixed,int:1) */
  ADD_EFUN("set_weak_flag",f_set_weak_flag,
	   tFunc(tSetvar(1,tMix) tInt,tVar(1)),OPT_SIDE_EFFECT);
  
/* function(void|object:object) */
  ADD_EFUN("next_object",f_next_object,
	   tFunc(tOr(tVoid,tObj),tObj),OPT_EXTERNAL_DEPEND);
  
/* function(string:string)|function(object:object)|function(mapping:mapping)|function(multiset:multiset)|function(program:program)|function(array:array) */
  ADD_EFUN("_next",f__next,
	   tOr6(tFunc(tStr,tStr),
		tFunc(tObj,tObj),
		tFunc(tMapping,tMapping),
		tFunc(tMultiset,tMultiset),
		tFunc(tPrg,tPrg),
		tFunc(tArray,tArray)),OPT_EXTERNAL_DEPEND);
  
/* function(object:object)|function(mapping:mapping)|function(multiset:multiset)|function(program:program)|function(array:array) */
  ADD_EFUN("_prev",f__prev,
	   tOr5(tFunc(tObj,tObj),
		tFunc(tMapping,tMapping),
		tFunc(tMultiset,tMultiset),
		tFunc(tPrg,tPrg),
		tFunc(tArray,tArray)),OPT_EXTERNAL_DEPEND);
  
/* function(mixed:program) */
  ADD_EFUN2("object_program", f_object_program,tFunc(tMix, tPrg),
	    OPT_TRY_OPTIMIZE, fix_object_program_type, 0);
  
/* function(mixed:int) */
  ADD_EFUN("objectp", f_objectp,tFunc(tMix,tInt),0);
  
/* function(mixed:int) */
  ADD_EFUN("programp",f_programp,tFunc(tMix,tInt),0);
  
/* function(:int) */
  ADD_EFUN("query_num_arg",f_query_num_arg,
	   tFunc(tNone,tInt),OPT_EXTERNAL_DEPEND);
  
/* function(int:int) */
  ADD_EFUN("random",f_random,
	   tFunc(tInt,tInt),OPT_EXTERNAL_DEPEND);
  
/* function(int:void) */
  ADD_EFUN("random_seed",f_random_seed,
	   tFunc(tInt,tVoid),OPT_SIDE_EFFECT);

  ADD_EFUN("random_string",f_random_string,
	   tFunc(tInt,tString),0);
  
/* function(string,string,string:string)|function(string,string*,string*:string)|function(0=array,mixed,mixed:0)|function(1=mapping,mixed,mixed:1) */
  ADD_EFUN("replace",f_replace,
	   tOr4(tFunc(tStr tStr tStr,tStr),
		tFunc(tStr tArr(tStr) tArr(tStr),tStr),
		tFunc(tSetvar(0,tArray) tMix tMix,tVar(0)),
		tFunc(tSetvar(1,tMapping) tMix tMix,tVar(1))),0);
  
/* function(int:int)|function(string:string)|function(0=array:0) */
  ADD_EFUN("reverse",f_reverse,
	   tOr3(tFunc(tInt,tInt),
		tFunc(tStr,tStr),
		tFunc(tSetvar(0, tArray),tVar(0))),0);
  
/* function(mixed,array:array) */
  ADD_EFUN("rows",f_rows,
	   tOr6(tFunc(tMap(tSetvar(0,tMix),tSetvar(1,tMix)) tArr(tVar(0)),
		      tArr(tVar(1))),
		tFunc(tSet(tSetvar(0,tMix)) tArr(tVar(0)), tArr(tInt01)),
		tFunc(tString tArr(tInt), tArr(tInt)),
		tFunc(tArr(tSetvar(0,tMix)) tArr(tInt), tArr(tVar(1))),
		tFunc(tArray tArr(tNot(tInt)), tArray),
		tFunc(tOr4(tObj,tFunction,tProgram,tInt) tArray, tArray)), 0);

/* function(:int *) */
  ADD_EFUN("rusage", f_rusage,tFunc(tNone,tArr(tInt)),OPT_EXTERNAL_DEPEND);

  /* FIXME: Is the third arg a good idea when the first is a mapping? */
  ADD_EFUN("search",f_search,
	   tOr4(tFunc(tStr tStr tOr(tVoid,tInt),
		      tInt),
		tFunc(tArr(tSetvar(0,tMix)) tVar(0) tOr(tVoid,tInt),
		      tInt),
		tFunc(tMap(tSetvar(1,tMix),tSetvar(2,tMix)) tVar(2) tOr(tVoid,tVar(1)),
		      tVar(1)),

		tIfnot(
		  tFunc(tArr(tSetvar(0,tMix)) tVar(0) tOr(tVoid,tInt),
			tInt),
		  tIfnot(
		    tFunc(tMap(tSetvar(1,tMix),tSetvar(2,tMix)) tVar(2) tOr(tVoid,tVar(1)),
			  tVar(1)),
		    tFunc( tOr(tMapping,tArray) tMix tOr(tVoid,tMix), tZero)))),
	   0);
  
  ADD_EFUN2("has_prefix", f_has_prefix, tFunc(tStr tStr,tInt01),
	    OPT_TRY_OPTIMIZE, 0, 0);

  ADD_EFUN("has_index",f_has_index,
	   tOr5(tFunc(tStr tIntPos, tInt),
		tFunc(tArray tIntPos, tInt),
		tFunc(tSet(tSetvar(0,tMix)) tVar(0), tInt),
		tFunc(tMap(tSetvar(1,tMix),tMix) tVar(1), tInt),
		tFunc(tObj tMix, tInt)),
	   0);

  ADD_EFUN("has_value",f_has_value,
	   tOr5(tFunc(tStr tStr, tInt),
		tFunc(tArr(tSetvar(0,tMix)) tVar(0), tInt),
		tFunc(tMultiset tInt, tInt),
		tFunc(tMap(tMix,tSetvar(1,tMix)) tVar(1), tInt),
		tFunc(tObj tMix, tInt)),
	   0);

/* function(float|int,int|void:void) */
  ADD_EFUN("sleep", f_sleep,
	   tFunc(tOr(tFlt,tInt) tOr(tInt,tVoid),tVoid),OPT_SIDE_EFFECT);
  
/* function(array(0=mixed),array(mixed)...:array(0)) */
  ADD_EFUN("sort",f_sort,
	   tFuncV(tArr(tSetvar(0,tMix)),tArr(tMix),tArr(tVar(0))),
	   OPT_SIDE_EFFECT);
  /* function(array(0=mixed)...:array(0)) */
  ADD_FUNCTION("splice",f_splice,
	       tFuncV(tNone,tArr(tSetvar(0,tMix)),tArr(tVar(0))), 0);
  
/* function(mixed:int) */
  ADD_EFUN("stringp", f_stringp,tFunc(tMix,tInt),0);
  
/* function(:object) */
  ADD_EFUN2("this_object", f_this_object,tFunc(tNone,tObj),
	    OPT_EXTERNAL_DEPEND, fix_this_object_type, 0);
  
/* function(mixed:void) */
  ADD_EFUN("throw",f_throw,tFunc(tMix,tVoid),OPT_SIDE_EFFECT);
  
/* function(void|int:int|float) */
  ADD_EFUN("time",f_time,
	   tFunc(tOr(tVoid,tInt),tOr(tInt,tFlt)),OPT_EXTERNAL_DEPEND);
  
/* function(int:int) */
  ADD_EFUN("trace",f_trace,tFunc(tInt,tInt),OPT_SIDE_EFFECT);
  /* function(array(0=mixed):array(0)) */
  ADD_FUNCTION("transpose",f_transpose,
	       tFunc(tArr(tSetvar(0,tMix)),tArr(tVar(0))), 0);
  
/* function(string:string) */
  ADD_EFUN("upper_case",f_upper_case,tFunc(tStr,tStr),0);
  
/* function(string|multiset:array(int))|function(array(0=mixed)|mapping(mixed:0=mixed)|object|program:array(0)) */
  ADD_EFUN2("values",f_values,
	   tOr(tFunc(tOr(tStr,tMultiset),tArr(tInt)),
	       tFunc(tOr4(tArr(tSetvar(0,tMix)),
			  tMap(tMix,tSetvar(0,tMix)),
			  tObj,tPrg),
		     tArr(tVar(0)))),0,fix_values_type,0);
  
/* function(mixed:int) */
  ADD_EFUN("zero_type",f_zero_type,tFunc(tMix,tInt01),0);
  
/* function(string,string:array) */
  ADD_EFUN("array_sscanf",f_sscanf,tFunc(tStr tStr,tArray),0);

  /* Some Wide-string stuff */
  
/* function(string:string) */
  ADD_EFUN("string_to_unicode", f_string_to_unicode,
	   tFunc(tStr,tStr), OPT_TRY_OPTIMIZE);
  
/* function(string:string) */
  ADD_EFUN("unicode_to_string", f_unicode_to_string,
	   tFunc(tStr,tStr), OPT_TRY_OPTIMIZE);
  
/* function(string,int|void:string) */
  ADD_EFUN("string_to_utf8", f_string_to_utf8,
	   tFunc(tStr tOr(tInt,tVoid),tStr), OPT_TRY_OPTIMIZE);
  
/* function(string,int|void:string) */
  ADD_EFUN("utf8_to_string", f_utf8_to_string,
	   tFunc(tStr tOr(tInt,tVoid),tStr), OPT_TRY_OPTIMIZE);


  ADD_EFUN("__parse_pike_type", f_parse_pike_type,
	   tFunc(tStr,tStr),OPT_TRY_OPTIMIZE);

#ifdef HAVE_LOCALTIME
  
/* function(int:mapping(string:int)) */
  ADD_EFUN("localtime",f_localtime,
	   tFunc(tInt,tMap(tStr,tInt)),OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GMTIME
  
/* function(int:mapping(string:int)) */
  ADD_EFUN("gmtime",f_gmtime,tFunc(tInt,tMap(tStr,tInt)),OPT_EXTERNAL_DEPEND);
#endif

#ifdef HAVE_MKTIME
  
/* function(int,int,int,int,int,int,int,void|int:int)|function(object|mapping:int) */
  ADD_EFUN("mktime",f_mktime,
	   tOr(tFunc(tInt tInt tInt tInt tInt tInt tInt tOr(tVoid,tInt),tInt),
	       tFunc(tOr(tObj,tMapping),tInt)),OPT_TRY_OPTIMIZE);
#endif

#ifdef PIKE_DEBUG
  
/* function(:void) */
  ADD_EFUN("_verify_internals",f__verify_internals,
	   tFunc(tNone,tVoid),OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
  
/* function(int:int) */
  ADD_EFUN("_debug",f__debug,
	   tFunc(tInt,tInt),OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);

/* function(int:int) */
  ADD_EFUN("_optimizer_debug",f__optimizer_debug,
	   tFunc(tInt,tInt),OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);

#ifdef YYDEBUG
  
/* function(int:int) */
  ADD_EFUN("_compiler_trace",f__compiler_trace,
	   tFunc(tInt,tInt),OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
#endif /* YYDEBUG */
#endif
  
/* function(:mapping(string:int)) */
  ADD_EFUN("_memory_usage",f__memory_usage,
	   tFunc(tNone,tMap(tStr,tInt)),OPT_EXTERNAL_DEPEND);

  
/* function(:int) */
  ADD_EFUN("gc",f_gc,tFunc(tNone,tInt),OPT_SIDE_EFFECT);
  
/* function(:string) */
  ADD_EFUN("version", f_version,tFunc(tNone,tStr), OPT_TRY_OPTIMIZE);

  
/* function(mixed,void|object:string) */
  ADD_EFUN("encode_value", f_encode_value,
	   tFunc(tMix tOr(tVoid,tObj),tStr), OPT_TRY_OPTIMIZE);

  /* function(mixed,void|object:string) */
  ADD_EFUN("encode_value_canonic", f_encode_value_canonic,
	   tFunc(tMix tOr(tVoid,tObj),tStr), OPT_TRY_OPTIMIZE);

/* function(string,void|object:mixed) */
  ADD_EFUN("decode_value", f_decode_value,
	   tFunc(tStr tOr(tVoid,tObj),tMix), OPT_TRY_OPTIMIZE);
  
/* function(object,string:int) */
  ADD_EFUN("object_variablep", f_object_variablep,
	   tFunc(tObj tStr,tInt), OPT_EXTERNAL_DEPEND);

  /* function(array(mapping(int:mixed)):array(int)) */
  ADD_FUNCTION("interleave_array",f_interleave_array,
	       tFunc(tArr(tMap(tInt,tMix)),tArr(tInt)),OPT_TRY_OPTIMIZE);
  /* function(array(0=mixed),array(1=mixed):array(array(array(0)|array(1))) */
  ADD_FUNCTION("diff",f_diff,
	       tFunc(tArr(tSetvar(0,tMix)) tArr(tSetvar(1,tMix)),
		     tArr(tArr(tOr(tArr(tVar(0)),tArr(tVar(1)))))),
	       OPT_TRY_OPTIMIZE);
  /* function(array,array:array(int)) */
  ADD_FUNCTION("diff_longest_sequence",f_diff_longest_sequence,
	       tFunc(tArray tArray,tArr(tInt)),OPT_TRY_OPTIMIZE);
  /* function(array,array:array(int)) */
  ADD_FUNCTION("diff_dyn_longest_sequence",f_diff_dyn_longest_sequence,
	       tFunc(tArray tArray,tArr(tInt)),OPT_TRY_OPTIMIZE);
  /* function(array,array:array(array)) */
  ADD_FUNCTION("diff_compare_table",f_diff_compare_table,
	       tFunc(tArray tArray,tArr(tArr(tInt))),OPT_TRY_OPTIMIZE);
  /* function(array:array(int)) */
  ADD_FUNCTION("longest_ordered_sequence",f_longest_ordered_sequence,
	       tFunc(tArray,tArr(tInt)),0);
  /* function(array(0=mixed),array(mixed)...:array(0)) */
  ADD_FUNCTION("sort",f_sort,
	       tFuncV(tArr(tSetvar(0, tMix)),tArr(tMix),tArr(tVar(0))),
	       OPT_SIDE_EFFECT);
  ADD_FUNCTION("string_count",f_string_count,
	       tFunc(tString tString,tInt),OPT_TRY_OPTIMIZE);

#define tMapStuff(IN,SUB,OUTFUN,OUTSET,OUTPROG,OUTMIX,OUTARR,OUTMAP) \
  tOr7( tFuncV(IN tFuncV(SUB,tMix,tSetvar(2,tAny)),tMix,OUTFUN), \
        tIfnot(tFuncV(IN tFunction,tMix,tMix), \
	       tOr(tFuncV(IN tProgram, tMix, OUTPROG), \
		   tFuncV(IN tObj, tMix, OUTMIX))), \
	tFuncV(IN tSet(tMix),tMix,OUTSET), \
	tFuncV(IN tMap(tMix, tSetvar(2,tMix)), tMix, OUTMAP), \
        tFuncV(IN tArray, tMix, OUTARR), \
        tFuncV(IN tInt0, tMix, OUTMIX), \
	tFuncV(IN, tVoid, OUTMIX) )

  ADD_EFUN2("map", f_map,
	    tOr7(tMapStuff(tArr(tSetvar(1,tMix)),tVar(1),
			   tArr(tVar(2)),
			   tArr(tInt01),
			   tArr(tObj),
			   tArr(tMix),
			   tArr(tArr(tMix)),
			   tArr(tOr(tInt0,tVar(2)))),

		 tMapStuff(tMap(tSetvar(3,tMix),tSetvar(1,tMix)),tVar(1),
			   tMap(tVar(3),tVar(2)),
			   tMap(tVar(3),tInt01),
			   tMap(tVar(3),tObj),
			   tMap(tVar(3),tMix),
			   tMap(tVar(3),tArr(tMix)),
			   tMap(tVar(3),tOr(tInt0,tVar(2)))),
		
 		 tMapStuff(tSet(tSetvar(1,tMix)),tVar(1),
			   tSet(tVar(2)),
			   tSet(tInt01),
			   tSet(tObj),
			   tSet(tMix),
			   tSet(tArr(tMix)),
			   tSet(tOr(tInt0,tVar(2)))),

		 tMapStuff(tOr(tProgram,tFunction),tMix,
			   tMap(tStr,tVar(2)),
			   tMap(tStr,tInt01),
			   tMap(tStr,tObj),
			   tMap(tStr,tMix),
			   tMap(tStr,tArr(tMix)),
			   tMap(tStr,tOr(tInt0,tVar(2)))),

		 tOr4( tFuncV(tString tFuncV(tInt,tMix,tInt),tMix,tString), 
		       tFuncV(tString tFuncV(tInt,tMix,tInt),tMix,tString),
		       tFuncV(tString tSet(tMix),tMix,tString),
		       tFuncV(tString tMap(tMix,tInt), tMix, tString) ),
		 
		 tFuncV(tArr(tStringIndicable) tString,tMix,tMix),

		 tFuncV(tObj,tMix,tMix) ),
	    OPT_TRY_OPTIMIZE, fix_map_node_info, 0);
  
  ADD_EFUN2("filter", f_filter,
	    tOr3(tFuncV(tSetvar(1,tOr4(tArray,tMapping,tMultiset,tString)),
			tMixed,tVar(1)),
		 tFuncV(tOr(tProgram,tFunction),tMixed,tMap(tString,tMix)),
		 tFuncV(tObj,tMix,tMix) ) ,
	    OPT_TRY_OPTIMIZE, fix_map_node_info, 0);

  ADD_EFUN("enumerate",f_enumerate,
	   tOr8(tFunc(tIntPos,tArr(tInt)),
		tFunc(tIntPos tInt,tArr(tInt)),
		tFunc(tIntPos tInt tOr(tVoid,tInt),tArr(tInt)),
		tFunc(tIntPos tFloat tOr3(tVoid,tInt,tFloat),tArr(tFloat)),
		tFunc(tIntPos tOr(tInt,tFloat) tFloat,tArr(tFloat)),
		tFunc(tIntPos tMix tObj,tArr(tVar(1))),
		tFunc(tIntPos tObj tOr(tVoid,tMix),tArr(tVar(1))),
		tFunc(tIntPos tMix tMix 
		      tFuncV(tNone,tMix,tSetvar(1,tMix)),tArr(tVar(1)))),
	   OPT_TRY_OPTIMIZE);
		
  ADD_FUNCTION("inherit_list",f_inherit_list,tFunc(tProgram,tArr(tProgram)),0);
  ADD_FUNCTION("program_implements",f_program_implements,
	       tFunc(tProgram tProgram,tInt),0);
  ADD_FUNCTION("program_inherits",f_program_inherits,
	       tFunc(tProgram tProgram,tInt),0);
  ADD_FUNCTION("program_defined",f_program_defined,
	       tFunc(tProgram,tString),0);
  ADD_FUNCTION("function_defined",f_function_defined,
	       tFunc(tFunction,tString),0);
  ADD_FUNCTION("string_width",f_string_width,tFunc(tString,tInt),0);

#ifdef DEBUG_MALLOC
  
/* function(void:void) */
  ADD_EFUN("_reset_dmalloc",f__reset_dmalloc,
	   tFunc(tVoid,tVoid),OPT_SIDE_EFFECT);
  ADD_EFUN("_dmalloc_set_name",f__dmalloc_set_name,
	   tOr(tFunc(tStr tInt,tVoid), tFunc(tVoid,tVoid)),OPT_SIDE_EFFECT);
  ADD_EFUN("_list_open_fds",f__list_open_fds,
	   tFunc(tVoid,tVoid),OPT_SIDE_EFFECT);
#endif
#ifdef PIKE_DEBUG
  
/* function(1=mixed:1) */
  ADD_EFUN("_locate_references",f__locate_references,
	   tFunc(tSetvar(1,tMix),tVar(1)),OPT_SIDE_EFFECT);
  ADD_EFUN("_describe",f__describe,
	   tFunc(tSetvar(1,tMix),tVar(1)),OPT_SIDE_EFFECT);
#endif

  ADD_EFUN("_gc_status",f__gc_status,
	   tFunc(tNone,tMap(tString,tOr(tInt,tFloat))),
	   OPT_EXTERNAL_DEPEND);
}

