/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: builtin_functions.c,v 1.120 1998/07/28 23:01:52 hubbe Exp $");
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
#include "lex.h"
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

void f_aggregate(INT32 args)
{
  struct array *a;
#ifdef DEBUG
  if(args < 0) fatal("Negative args to f_aggregate()\n");
#endif

  a=aggregate_array(args);
  push_array(a); /* beware, macro */
}

void f_trace(INT32 args)
{
  extern int t_flag;
  int old_t_flag=t_flag;
  get_all_args("trace",args,"%i",&t_flag);
  pop_n_elems(args);
  push_int(old_t_flag);
}

void f_hash(INT32 args)
{
  INT32 i;
  if(!args)
    PIKE_ERROR("hash", "Too few arguments.\n", sp, 0);
  if(sp[-args].type != T_STRING)
    PIKE_ERROR("hash", "Bad argument 1.\n", sp, args);
  i=hashstr((unsigned char *)sp[-args].u.string->str,100);
  
  if(args > 1)
  {
    if(sp[1-args].type != T_INT)
      PIKE_ERROR("hash", "Bad argument 2.\n", sp, args);
    
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
    PIKE_ERROR("copy_value", "Too few arguments.\n", sp, 0);

  pop_n_elems(args-1);
  copy_svalues_recursively_no_free(sp,sp-1,1,0);
  free_svalue(sp-1);
  sp[-1]=sp[0];
}

void f_ctime(INT32 args)
{
  time_t i;
  if(!args)
    PIKE_ERROR("ctime", "Too few arguments.\n", sp, args);
  if(sp[-args].type != T_INT)
    PIKE_ERROR("ctime", "Bad argument 1.\n", sp, args);
  i=(time_t)sp[-args].u.integer;
  pop_n_elems(args);
  push_string(make_shared_string(ctime(&i)));
}

void f_lower_case(INT32 args)
{
  INT32 i;
  struct pike_string *ret;
  if(!args)
    PIKE_ERROR("lower_case", "Too few arguments.\n", sp, 0);
  if(sp[-args].type != T_STRING) 
    PIKE_ERROR("lower_case", "Bad argument 1.\n", sp, args);

  ret=begin_shared_string(sp[-args].u.string->len);
  MEMCPY(ret->str, sp[-args].u.string->str,sp[-args].u.string->len);

  for (i = sp[-args].u.string->len-1; i>=0; i--)
    if (isupper(EXTRACT_UCHAR( ret->str + i)))
      ret->str[i] = tolower(EXTRACT_UCHAR(ret->str+i));

  pop_n_elems(args);
  push_string(end_shared_string(ret));
}

void f_upper_case(INT32 args)
{
  INT32 i;
  struct pike_string *ret;
  if(!args)
    PIKE_ERROR("upper_case", "Too few arguments.\n", sp, 0);
  if(sp[-args].type != T_STRING) 
    PIKE_ERROR("upper_case", "Bad argument 1.\n", sp, args);

  ret=begin_shared_string(sp[-args].u.string->len);
  MEMCPY(ret->str, sp[-args].u.string->str,sp[-args].u.string->len);

  for (i = sp[-args].u.string->len-1; i>=0; i--)
    if (islower(EXTRACT_UCHAR(ret->str+i)))
      ret->str[i] = toupper(EXTRACT_UCHAR(ret->str+i));

  pop_n_elems(args);
  push_string(end_shared_string(ret));
}

void f_random(INT32 args)
{
  if(!args)
    PIKE_ERROR("random", "Too few arguments.\n", sp, 0);
  if(sp[-args].type != T_INT) 
    PIKE_ERROR("random", "Bad argument 1.\n", sp, args);

  if(sp[-args].u.integer <= 0)
  {
    sp[-args].u.integer = 0;
  }else{
    sp[-args].u.integer = my_rand() % sp[-args].u.integer;
  }
  pop_n_elems(args-1);
}

void f_random_seed(INT32 args)
{
  if(!args)
    PIKE_ERROR("random_seed", "Too few arguments.\n", sp, 0);
  if(sp[-args].type != T_INT) 
    PIKE_ERROR("random_seed", "Bad argument 1.\n", sp, args);

  my_srand(sp[-args].u.integer);
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
    PIKE_ERROR("search", "Too few arguments.\n", sp, args);

  switch(sp[-args].type)
  {
  case T_STRING:
  {
    char *ptr;
    INT32 len;
    if(sp[1-args].type != T_STRING)
      PIKE_ERROR("search", "Bad argument 2.\n", sp, args);
    
    start=0;
    if(args > 2)
    {
      if(sp[2-args].type!=T_INT)
	PIKE_ERROR("search", "Bad argument 3.\n", sp, args);

      start=sp[2-args].u.integer;
      if(start<0)
	PIKE_ERROR("search", "Start must be greater or equal to zero.\n", sp, args);
    }
    len=sp[-args].u.string->len - start;

    if(len<0)
      PIKE_ERROR("search", "Start must not be greater than the length of the string.\n", sp, args);

    if(len>0 && (ptr=my_memmem(sp[1-args].u.string->str,
			       sp[1-args].u.string->len,
			       sp[-args].u.string->str+start,
			       len)))
    {
      start=ptr-sp[-args].u.string->str;
    }else{
      start=-1;
    }
    pop_n_elems(args);
    push_int(start);
    break;
  }

  case T_ARRAY:
    start=0;
    if(args > 2)
    {
      if(sp[2-args].type!=T_INT)
	PIKE_ERROR("search", "Bad argument 3.\n", sp, args);

      start=sp[2-args].u.integer;
    }
    start=array_search(sp[-args].u.array,sp+1-args,start);
    pop_n_elems(args);
    push_int(start);
    break;

  case T_MAPPING:
    if(args > 2)
      mapping_search_no_free(sp,sp[-args].u.mapping,sp+1-args,sp+2-args);
    else
      mapping_search_no_free(sp,sp[-args].u.mapping,sp+1-args,0);
    free_svalue(sp-args);
    sp[-args]=*sp;
    pop_n_elems(args-1);
    return;

  default:
    PIKE_ERROR("search", "Bad argument 1.\n", sp, args);
  }
}

void f_backtrace(INT32 args)
{
  INT32 frames;
  struct frame *f,*of;
  struct array *a,*i;

  frames=0;
  if(args) pop_n_elems(args);
  for(f=fp;f;f=f->parent_frame) frames++;

  sp->type=T_ARRAY;
  sp->u.array=a=allocate_array_no_init(frames,0);
  sp++;

  /* NOTE: The first frame is ignored, since it is the call to backtrace(). */
  of=0;
  for(f=fp;f;f=(of=f)->parent_frame)
  {
    char *program_name;

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
  if(args<1)
    PIKE_ERROR("add_constant", "Too few arguments.\n", sp, args);

  if(sp[-args].type!=T_STRING)
    PIKE_ERROR("add_constant", "Bad argument 1.\n", sp, args);

  if(args>1)
  {
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
#define IS_ABS(X) ((isalpha((X)[0]) && (X)[1]==':' && IS_SEP((X)[2]))?3:0)
#define IS_ROOT(X) (IS_SEP((X)[0])?1:0)
#endif

static char *combine_path(char *cwd,char *file)
{
  /* cwd is supposed to be combined already */
  char *ret;
  register char *from,*to;
  char *my_cwd;
  char cwdbuf[10];
  my_cwd=0;
  

  if(IS_ABS(file))
  {
    MEMCPY(cwdbuf,file,IS_ABS(file));
    cwdbuf[IS_ABS(file)]=0;
    cwd=cwdbuf;
    file+=IS_ABS(file);
  }

#ifdef IS_ROOT
  else if(IS_ROOT(file))
  {
    if(IS_ABS(cwd))
    {
      MEMCPY(cwdbuf,cwd,IS_ABS(cwd));
      cwdbuf[IS_ABS(cwd)]=0;
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

#ifdef DEBUG    
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
    PIKE_ERROR("combine_path", "Too few arguments.\n", sp, args);

  if(sp[-args].type != T_STRING)
    PIKE_ERROR("combine_path", "Bad argument 1.\n", sp, args);

  path=sp[-args].u.string->str;

  for(e=1;e<args;e++)
  {
    char *newpath;
    if(sp[e-args].type != T_STRING)
    {
      if(dofree) free(path);
      error("Bad argument %d to combine_path.\n",e);
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
    PIKE_ERROR("function_object", "Too few arguments.\n", sp, args);
  if(sp[-args].type != T_FUNCTION)
    PIKE_ERROR("function_object", "Bad argument 1.\n", sp, args);

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
    PIKE_ERROR("function_name", "Too few arguments.\n", sp, args);
  if(sp[-args].type != T_FUNCTION)
    PIKE_ERROR("function_name", "Bad argument 1.\n", sp, args);

  if(sp[-args].subtype == FUNCTION_BUILTIN)
  {
    pop_n_elems(args);
    push_int(0);
  }else{
    if(!sp[-args].u.object->prog)
      PIKE_ERROR("function_name", "Destructed object.\n", sp, args);

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
    PIKE_ERROR("zero_type", "Too few arguments.\n", sp, args);
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
    PIKE_ERROR("allocate", "Too few arguments.\n", sp, args);

  if(sp[-args].type!=T_INT)
    PIKE_ERROR("allocate", "Bad argument 1.\n", sp, args);


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
    sp->u.object=fp->current_object;
    sp->type=T_OBJECT;
    add_ref(fp->current_object);
    sp++;
  }else{
    push_int(0);
  }
}

void f_throw(INT32 args)
{
  if(args < 1)
    PIKE_ERROR("throw", "Too few arguments.\n", sp, args);
  assign_svalue(&throw_value,sp-args);
  pop_n_elems(args);
  throw_severity=0;
  pike_throw();
}

void f_exit(INT32 args)
{
  if(args < 1)
    PIKE_ERROR("exit", "Too few arguments.\n", sp, args);

  if(sp[-args].type != T_INT)
    PIKE_ERROR("exit", "Bad argument 1.\n", sp, args);

  assign_svalue(&throw_value, sp-args);
  throw_severity=THROW_EXIT;
  pike_throw();
}

void f__exit(INT32 args)
{
  if(args < 1)
    PIKE_ERROR("_exit", "Too few arguments.\n", sp, args);

  if(sp[-args].type != T_INT)
    PIKE_ERROR("_exit", "Bad argument 1.\n", sp, args);

  exit(sp[-1].u.integer);
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
    PIKE_ERROR("crypt", "Too few arguments.\n", sp, args);

  if(sp[-args].type != T_STRING)
    PIKE_ERROR("crypt", "Bad argument 1.\n", sp, args);

  
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
      PIKE_ERROR("destruct", "Bad arguments 1.\n", sp, args);
    
    o=sp[-args].u.object;
  }else{
    if(!fp)
      PIKE_ERROR("destruct", "Destruct called without argument from callback function.\n", sp, args);
	   
    o=fp->current_object;
  }
  destruct(o);
  pop_n_elems(args);
}

void f_indices(INT32 args)
{
  INT32 size;
  struct array *a;
  if(args < 1)
    PIKE_ERROR("indices", "Too few arguments.\n", sp, args);

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
    PIKE_ERROR("indices", "Bad argument 1.\n", sp, args);
    return; /* make apcc happy */
  }
  pop_n_elems(args);
  push_array(a);
}

void f_values(INT32 args)
{
  INT32 size;
  struct array *a;
  if(args < 1)
    PIKE_ERROR("values", "Too few arguments.\n", sp, args);

  switch(sp[-args].type)
  {
  case T_STRING:
    size=sp[-args].u.string->len;
    a=allocate_array_no_init(size,0);
    while(--size>=0)
    {
      ITEM(a)[size].type=T_INT;
      ITEM(a)[size].subtype=NUMBER_NUMBER;
      ITEM(a)[size].u.integer=EXTRACT_UCHAR(sp[-args].u.string->str+size);
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
    PIKE_ERROR("values", "Bad argument 1.\n", sp, args);
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
      PIKE_ERROR("next_object", "Bad argument 1.\n", sp, args);
    o=sp[-args].u.object->next;
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
    PIKE_ERROR("object_program", "Too few arguments.\n", sp, args);

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

void f_reverse(INT32 args)
{
  if(args < 1)
    PIKE_ERROR("reverse", "Too few arguments.\n", sp, args);

  switch(sp[-args].type)
  {
  case T_STRING:
  {
    INT32 e;
    struct pike_string *s;
    s=begin_shared_string(sp[-args].u.string->len);
    for(e=0;e<sp[-args].u.string->len;e++)
      s->str[e]=sp[-args].u.string->str[sp[-args].u.string->len-1-e];
    s=end_shared_string(s);
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
    PIKE_ERROR("reverse", "Bad argument 1.\n", sp, args);
    
  }
}

struct tupel
{
  struct pike_string *ind,*val;
};

static int replace_sortfun(void *a,void *b)
{
  return my_quick_strcmp( ((struct tupel *)a)->ind, ((struct tupel *)b)->ind);
}

static struct pike_string * replace_many(struct pike_string *str,
					struct array *from,
					struct array *to)
{
  char *s;
  INT32 length,e;

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

  for(e=0;e<from->size;e++)
  {
    if(ITEM(from)[e].type != T_STRING)
      error("Replace: from array is not array(string)\n");
    v[e].ind=ITEM(from)[e].u.string;
  }

  for(e=0;e<to->size;e++)
  {
    if(ITEM(to)[e].type != T_STRING)
      error("Replace: to array is not array(string)\n");
    v[e].val=ITEM(to)[e].u.string;
  }

  fsort((char *)v,from->size,sizeof(struct tupel),(fsortfun)replace_sortfun);

  for(e=0;e<256;e++)
    set_end[e]=set_start[e]=0;

  for(e=0;e<from->size;e++)
  {
    set_start[EXTRACT_UCHAR(v[from->size-1-e].ind->str)]=from->size-e-1;
    set_end[EXTRACT_UCHAR(v[e].ind->str)]=e+1;
  }

  init_buf();

  length=str->len;
  s=str->str;

  for(;length > 0;)
  {
    INT32 a,b,c;
    if((b=set_end[EXTRACT_UCHAR(s)]))
    {
      a=set_start[EXTRACT_UCHAR(s)];
      while(a<b)
      {
	c=(a+b)/2;

	if(low_quick_binary_strcmp(v[c].ind->str,v[c].ind->len,s,length) <=0)
	{
	  if(a==c) break;
	  a=c;
	}else{
	  b=c;
	}
      }
      if(a<from->size &&
	 length >= v[a].ind->len &&
	 !low_quick_binary_strcmp(v[a].ind->str,v[a].ind->len,
				  s,v[a].ind->len))
      {
	c=v[a].ind->len;
	if(!c) c=1;
	s+=c;
	length-=c;
	my_binary_strcat(v[a].val->str,v[a].val->len);
	continue;
      }
    }
    my_putchar(*s);
    s++;
    length--;
  }

  free((char *)v);
  return free_buf();
}

void f_replace(INT32 args)
{
  if(args < 3)
    PIKE_ERROR("replace", "Too few arguments.\n", sp, args);

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
      PIKE_ERROR("replace", "Bad argument 2.\n", sp, args);
      
    case T_STRING:
      if(sp[2-args].type != T_STRING)
	PIKE_ERROR("replace", "Bad argument 3.\n", sp, args);

      s=string_replace(sp[-args].u.string,
		       sp[1-args].u.string,
		       sp[2-args].u.string);
      break;
      
    case T_ARRAY:
      if(sp[2-args].type != T_ARRAY)
	PIKE_ERROR("replace", "Bad argument 3.\n", sp, args);

      s=replace_many(sp[-args].u.string,
		     sp[1-args].u.array,
		     sp[2-args].u.array);
    
    }
    pop_n_elems(args);
    push_string(s);
    break;
  }

  default:
    PIKE_ERROR("replace", "Bad argument 1.\n", sp, args);
  }
}

void f_compile(INT32 args)
{
  
  struct program *p;
  if(args < 1)
    PIKE_ERROR("compile", "Too few arguments.\n", sp, args);

  if(sp[-args].type != T_STRING)
    PIKE_ERROR("compile", "Bad argument 1.\n", sp, args);

  p=compile(sp[-args].u.string);
  pop_n_elems(args);
  push_program(p);
}

void f_mkmapping(INT32 args)
{
  struct mapping *m;
  struct array *a,*b;
  get_all_args("mkmapping",args,"%a%a",&a,&b);
  if(a->size != b->size)
    PIKE_ERROR("mkmapping", "mkmapping called on arrays of different sizes\n",
	  sp, args);

  m=mkmapping(sp[-args].u.array, sp[1-args].u.array);
  pop_n_elems(args);
  push_mapping(m);
}

void f_objectp(INT32 args)
{
  if(args<1) PIKE_ERROR("objectp", "Too few arguments.\n", sp, args);
  if(sp[-args].type != T_OBJECT || !sp[-args].u.object->prog)
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
  if(args<1) PIKE_ERROR("functionp", "Too few arguments.\n", sp, args);
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
  struct timeval t1,t2,t3;
  INT32 a,b;

  if(!args)
    PIKE_ERROR("sleep", "Too few arguments.\n", sp, args);

  switch(sp[-args].type)
  {
  case T_INT:
    t2.tv_sec=sp[-args].u.integer;
    t2.tv_usec=0;
    break;

  case T_FLOAT:
  {
    FLOAT_TYPE f;
    f=sp[-args].u.float_number;
    t2.tv_sec=floor(f);
    t2.tv_usec=(long)(1000000.0*(f-floor(f)));
    break;
  }

  default:
    PIKE_ERROR("sleep", "Bad argument 1.\n", sp, args);
  }
  pop_n_elems(args);


  if( args >1 && !IS_ZERO(sp-args+1))
  {
    THREADS_ALLOW();
#ifdef __NT__
    Sleep(t2.tv_sec * 1000 + t2.tv_usec / 1000);
#elif defined(HAVE_POLL)
    poll(NULL, 0, t2.tv_sec * 1000 + t2.tv_usec / 1000);
#else
    select(0,0,0,0,&t2);
#endif
    THREADS_DISALLOW();
  }else{
    GETTIMEOFDAY(&t1);
    my_add_timeval(&t1, &t2);
    
    while(1)
    {
      GETTIMEOFDAY(&t2);
      if(my_timercmp(&t1, <= , &t2))
	return;
      
      t3=t1;
      my_subtract_timeval(&t3, &t2);
      
      THREADS_ALLOW();
#ifdef __NT__
      Sleep(t3.tv_sec * 1000 + t3.tv_usec / 1000);
#elif defined(HAVE_POLL)
      poll(NULL, 0, t3.tv_sec * 1000 + t3.tv_usec / 1000);
#else
      select(0,0,0,0,&t3);
#endif
      THREADS_DISALLOW();
      check_signals(0,0,0);
    }
  }
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

#define TYPEP(ID,NAME,TYPE) \
void ID(INT32 args) \
{ \
  int t; \
  if(args<1) PIKE_ERROR(NAME, "Too few arguments.\n", sp, args); \
  t=sp[-args].type == TYPE; \
  pop_n_elems(args); \
  push_int(t); \
}


void f_programp(INT32 args)
{
  if(args<1)
    PIKE_ERROR("programp", "Too few arguments.\n", sp, args);
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

TYPEP(f_intp, "intpp", T_INT)
TYPEP(f_mappingp, "mappingp", T_MAPPING)
TYPEP(f_arrayp, "arrayp", T_ARRAY)
TYPEP(f_multisetp, "multisetp", T_MULTISET)
TYPEP(f_stringp, "stringp", T_STRING)
TYPEP(f_floatp, "floatp", T_FLOAT)

void f_sort(INT32 args)
{
  INT32 e,*order;

  if(args < 1)
    fatal("Too few arguments to sort().\n");

  for(e=0;e<args;e++)
  {
    if(sp[e-args].type != T_ARRAY)
      error("Bad argument %ld to sort().\n",(long)(e+1));

    if(sp[e-args].u.array->size != sp[-args].u.array->size)
      error("Argument %ld to sort() has wrong size.\n",(long)(e+1));
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

  if(args < 2)
    PIKE_ERROR("rows", "Too few arguments.\n", sp, args);

  if(sp[1-args].type!=T_ARRAY)
    PIKE_ERROR("rows", "Bad argument 1.\n", sp, args);

  tmp=sp[1-args].u.array;
  push_array(a=allocate_array(tmp->size));

  for(e=0;e<a->size;e++)
    index_no_free(ITEM(a)+e, sp-args-1, ITEM(tmp)+e);

  add_ref(a);
  pop_n_elems(args+1);
  push_array(a);
}

void f_column(INT32 args)
{
  INT32 e;
  struct array *a,*tmp;
  DECLARE_CYCLIC();

  if(args < 2)
    PIKE_ERROR("column", "Too few arguments.\n", sp, args);

  if(sp[-args].type!=T_ARRAY)
    PIKE_ERROR("column", "Bad argument 1.\n", sp, args);

  tmp=sp[-args].u.array;
  if((a=(struct array *)BEGIN_CYCLIC(tmp,0)))
  {
    add_ref(a);
    pop_n_elems(args);
    push_array(a);
  }else{
    push_array(a=allocate_array(tmp->size));
    SET_CYCLIC_RET(a);

    for(e=0;e<a->size;e++)
      index_no_free(ITEM(a)+e, ITEM(tmp)+e, sp-args);

    END_CYCLIC();
    add_ref(a);
    pop_n_elems(args+1);
    push_array(a);
  }
}

#ifdef DEBUG
void f__verify_internals(INT32 args)
{
  INT32 tmp=d_flag;
  d_flag=0x7fffffff;
  do_debug();
  d_flag=tmp;
  do_gc();
  pop_n_elems(args);
}

void f__debug(INT32 args)
{
  INT32 i=d_flag;
  get_all_args("_debug",args,"%i",&d_flag);
  pop_n_elems(args);
  push_int(i);
}

#ifdef YYDEBUG

void f__compiler_trace(INT32 args)
{
  extern int yydebug;
  INT32 i = yydebug;
  get_all_args("_compiler_trace", args, "%i", &yydebug);
  pop_n_elems(args);
  push_int(i);
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
  time_t t;
  if (args<1 || sp[-1].type!=T_INT)
    PIKE_ERROR("localtime", "Bad argument to localtime", sp, args);

  t=sp[-1].u.integer;
  tm=gmtime(&t);
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
  time_t t;
  if (args<1 || sp[-1].type!=T_INT)
    PIKE_ERROR("localtime", "Bad argument to localtime", sp, args);

  t=sp[-1].u.integer;
  tm=localtime(&t);
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
static void f_mktime (INT32 args)
{
  INT32 sec, min, hour, mday, mon, year, isdst;
  struct tm date;
  struct svalue s;
  struct svalue * r;
  int retval;
  if (args<1)
    PIKE_ERROR("mktime", "Too few arguments.\n", sp, args);

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
  if(sp[6-args].subtype == NUMBER_NUMBER)
  {
    date.tm_isdst=sp[6-args].u.integer;
  }else{
    date.tm_isdst=-1;
  }

#if STRUCT_TM_HAS_GMTOFF
  if(sp[7-args].subtype == NUMBER_NUMBER)
  {
    date.tm_gmtoff=sp[7-args].u.intger;
  }else{
    time_t tmp=0;
    data.tm_gmtoff=localtime(&t).tm_gmtoff;
  }
  retval=mktime(&date);
#else
#ifdef HAVE_EXTERNAL_TIMEZONE
  if(sp[7-args].subtype == NUMBER_NUMBER)
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


/* Check if the string s[0..len[ matches the glob m[0..mlen[ */
static int does_match(char *s, int len, char *m, int mlen)
{
  int i,j;
  for (i=j=0; i<mlen; i++)
  {
    switch (m[i])
    {
     case '?':
       if(j++>=len) return 0;
       break;

     case '*': 
      i++;
      if (i==mlen) return 1;	/* slut */

      for (;j<len;j++)
	if (does_match(s+j,len-j,m+i,mlen-i))
	  return 1;

      return 0;

     default: 
       if(j>=len || m[i]!=s[j]) return 0;
       j++;
    }
  }
  return j==len;
}

void f_glob(INT32 args)
{
  INT32 i,matches;
  struct array *a;
  struct svalue *sval, tmp;
  struct pike_string *glob;

  if(args < 2)
    PIKE_ERROR("glob", "Too few arguments.\n", sp, args);

  if(args > 2) pop_n_elems(args-2);
  args=2;

  if (sp[-args].type!=T_STRING)
    PIKE_ERROR("glob", "Bad argument 2.\n", sp, args);

  glob=sp[-args].u.string;

  switch(sp[1-args].type)
  {
  case T_STRING:
    i=does_match(sp[1-args].u.string->str,
		 sp[1-args].u.string->len,
		 glob->str,
		 glob->len);
    pop_n_elems(2);
    push_int(i);
    break;
    
  case T_ARRAY:
    a=sp[1-args].u.array;
    matches=0;
    for(i=0;i<a->size;i++)
    {
      if(ITEM(a)[i].type != T_STRING)
	PIKE_ERROR("glob", "Bad argument 2.\n", sp, args);

      if(does_match(ITEM(a)[i].u.string->str,
		    ITEM(a)[i].u.string->len,
		    glob->str,
		    glob->len))
      {
	add_ref(ITEM(a)[i].u.string);
	push_string(ITEM(a)[i].u.string);
	matches++;
      }
    }
    f_aggregate(matches);
    tmp=sp[-1];
    sp--;
    pop_n_elems(2);
    sp[0]=tmp;
    sp++;
    break;

  default:
    PIKE_ERROR("glob", "Bad argument 2.\n", sp, args);
  }
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
    PIKE_ERROR("Array.longest_ordered_sequence", "Out of memory", sp, args);
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

   stack = malloc(sizeof(struct diff_magic_link*)*cmptbl->size);

   if (!stack) error("diff_longest_sequence(): Out of memory\n");

   /* NB: marks is used for optimization purposes only */
   marks = calloc(blen,1);

   if (!marks) {
     free(stack);
     error("diff_longest_sequence(): Out of memory\n");
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
#ifdef DEBUG
	 if (x >= blen) {
	   fatal("diff_longest_sequence(): x:%d >= blen:%d\n", x, blen);
	 } else if (x < 0) {
	   fatal("diff_longest_sequence(): x:%d < 0\n", x);
	 }
#endif /* DEBUG */
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
#ifdef DEBUG
	   if (x >= blen) {
	     fatal("diff_longest_sequence(): x:%d >= blen:%d\n", x, blen);
	   } else if (x < 0) {
	     fatal("diff_longest_sequence(): x:%d < 0\n", x);
	   }
#endif /* DEBUG */

	   /* Put x on the stack. */
	   marks[x] = 1;
	   if (pos == top)
	   {
#ifdef DIFF_DEBUG
	     fprintf(stderr, "DIFF:  New top element\n");
#endif /* DIFF_DEBUG */

	     if (! (dml=dml_new(&pools)) )
	     {
	       dml_free_pools(pools);
	       free(stack);
	       error("diff_longest_sequence(): Out of memory\n");
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
	       dml_free_pools(pools);
	       free(stack);
	       error("diff_longest_sequence: Out of memory\n");
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
    error("diff_dyn_longest_sequence(): Out of memory");
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
	  dml_free_pools(dml_pool);
	  free(table);
	  error("diff_dyn_longest_sequence(): Out of memory");
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
    if (dml_pool) {
      dml_free_pools(dml_pool);
    }
    error("diff_dyn_longest_sequence(): Out of memory");
  }

  i = 0;
  while(dml) {
#ifdef DEBUG
    if (i >= sz) {
      fatal("Consistency error in diff_dyn_longest_sequence()\n");
    }
#endif /* DEBUG */
#ifdef DIFF_DEBUG
    fprintf(stderr, "  %02d: %d\n", i, dml->x);
#endif /* DIFF_DEBUG */
    res->item[i].type = T_INT;
    res->item[i].subtype = 0;
    res->item[i].u.integer = dml->x;
    dml = dml->prev;
    i++;
  }
#ifdef DEBUG
  if (i != sz) {
    fatal("Consistency error in diff_dyn_longest_sequence()\n");
  }
#endif /* DEBUG */

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

   if (args<2)
      PIKE_ERROR("diff", "Too few arguments.\n", sp, args);

   if (sp[-args].type!=T_ARRAY ||
       sp[1-args].type!=T_ARRAY)
      PIKE_ERROR("diff", "Bad arguments.\n", sp, args);

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
   struct array *cmptbl;

   if (args<2)
      PIKE_ERROR("diff_compare_table", "Too few arguments.\n", sp, args);

   if (sp[-args].type!=T_ARRAY ||
       sp[1-args].type!=T_ARRAY)
      PIKE_ERROR("diff_compare_table", "Bad arguments.\n", sp, args);

   cmptbl=diff_compare_table(sp[-args].u.array,sp[1-args].u.array,NULL);

   pop_n_elems(args);
   push_array(cmptbl);
}

void f_diff_longest_sequence(INT32 args)
{
   struct array *seq;
   struct array *cmptbl;

   if (args<2)
      PIKE_ERROR("diff_longest_sequence", "Too few arguments.\n", sp, args);

   if (sp[-args].type!=T_ARRAY ||
       sp[1-args].type!=T_ARRAY)
      PIKE_ERROR("diff_longest_sequence", "Bad arguments.\n", sp, args);

   cmptbl = diff_compare_table(sp[-args].u.array,sp[1-args].u.array, NULL);
   push_array(cmptbl);
   /* Note that the stack is one element off here. */
   seq = diff_longest_sequence(cmptbl, sp[1-1-args].u.array->size);

   pop_n_elems(args+1);
   push_array(seq); 
}

void f_diff_dyn_longest_sequence(INT32 args)
{
   struct array *seq;
   struct array *cmptbl;

   if (args<2)
      PIKE_ERROR("diff_dyn_longest_sequence", "Too few arguments.\n",
		 sp, args);

   if (sp[-args].type!=T_ARRAY ||
       sp[1-args].type!=T_ARRAY)
      PIKE_ERROR("diff_dyn_longest_sequence", "Bad arguments.\n", sp, args);

   cmptbl=diff_compare_table(sp[-args].u.array,sp[1-args].u.array, NULL);
   push_array(cmptbl);
   /* Note that the stack is one element off here. */
   seq = diff_dyn_longest_sequence(cmptbl, sp[1-1-args].u.array->size);

   pop_n_elems(args);
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

  call_callback(&memory_usage_callback, (void *)0);

  f_aggregate_mapping(sp-ss);
}

void f__next(INT32 args)
{
  struct svalue tmp;
  if(!args)
    PIKE_ERROR("_next", "Too few arguments.\n", sp, args);
  
  pop_n_elems(args-1);
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
    PIKE_ERROR("_next", "Bad argument 1.\n", sp, args);
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
  if(!args)
    PIKE_ERROR("_prev", "Too few arguments.\n", sp, args);
  
  pop_n_elems(args-1);
  tmp=sp[-1];
  switch(tmp.type)
  {
  case T_OBJECT:  tmp.u.object=tmp.u.object->prev; break;
  case T_ARRAY:   tmp.u.array=tmp.u.array->prev; break;
  case T_MAPPING: tmp.u.mapping=tmp.u.mapping->prev; break;
  case T_MULTISET:tmp.u.multiset=tmp.u.multiset->prev; break;
  case T_PROGRAM: tmp.u.program=tmp.u.program->prev; break;
  default:
    PIKE_ERROR("_prev", "Bad argument 1.\n", sp, args);
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
  if(!args) PIKE_ERROR("_refs", "Too few arguments.\n", sp, args);
  if(sp[-args].type > MAX_REF_TYPE)
    PIKE_ERROR("refs", "Bad argument 1.\n", sp, args);

  i=sp[-args].u.refs[0];
  pop_n_elems(args);
  push_int(i);
}

void f_replace_master(INT32 args)
{
  if(!args)
    PIKE_ERROR("replace_master", "Too few arguments.\n", sp, 0);
  if(sp[-args].type != T_OBJECT)
    PIKE_ERROR("replace_master", "Bad argument 1.\n", sp, args); 
 if(!sp[-args].u.object->prog)
    PIKE_ERROR("replace_master", "Called with destructed object.\n", sp, args);
    
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
  push_int((INT32)(gethrvtime()/1000));
}

void f_gethrtime(INT32 args)
{
  pop_n_elems(args);
  if(args)
    push_int((INT32)(gethrtime())); 
  else
    push_int((INT32)(gethrtime()/1000)); 
}
#else
void f_gethrtime(INT32 args)
{
  struct timeval tv;
  pop_n_elems(args);
  GETTIMEOFDAY(&tv);
  if(args)
    push_int((INT32)((tv.tv_sec *1000000) + tv.tv_usec)*1000);
  else
    push_int((INT32)((tv.tv_sec *1000000) + tv.tv_usec));
}
#endif /* HAVE_GETHRVTIME */

#ifdef PROFILING
static void f_get_prof_info(INT32 args)
{
  struct program *prog = 0;
  int num_functions;
  int i;

  if (!args) {
    PIKE_ERROR("get_profiling_info", "Too few arguments.\n", sp, args);
  }
  prog = program_from_svalue(sp-args);
  if(!prog) PIKE_ERROR("get_profiling_info", "Bad argument 1.\n", sp, args);

  add_ref(prog);

  pop_n_elems(args);

  /* ({ num_clones, ([ "fun_name":({ num_calls }) ]) }) */

  push_int(prog->num_clones);

  for(num_functions=i=0; i<(int)prog->num_identifiers; i++) {
    if (prog->identifiers[i].num_calls)
    {
      num_functions++;
      add_ref(prog->identifiers[i].name);
      push_string(prog->identifiers[i].name);

      push_int(prog->identifiers[i].num_calls);
      push_int(prog->identifiers[i].total_time);
      f_aggregate(2);
    }
  }
  f_aggregate_mapping(num_functions * 2);
  f_aggregate(2);
}
#endif /* PROFILING */

void f_object_variablep(INT32 args)
{
  struct object *o;
  struct pike_string *s;
  int ret;

  get_all_args("variablep",args,"%o%S",&o, &s);

  if(!o->prog)
    PIKE_ERROR("variablep", "Called on destructed object.\n", sp, args);

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

#ifdef DEBUG
  if(args < 0) fatal("Negative args to f_splice()\n");
#endif

  for(i=0;i<args;i++)
    if (sp[i-args].type!=T_ARRAY) 
      error("Illegal argument %d to splice.\n", (i+1));
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
#ifdef DEBUG
  if(args < 0) fatal("Negative args to f_everynth()\n");
#endif

  check_all_args("everynth",args, BIT_ARRAY, BIT_INT | BIT_VOID, BIT_INT | BIT_VOID , 0);

  switch(args)
  {
    default:
    case 3:
     start=sp[2-args].u.integer;
     if(start<0) error("Third argument to everynth is negative.\n");
    case 2:
      n=sp[1-args].u.integer;
      if(n<1) error("Second argument to everynth is negative.\n");
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
#ifdef DEBUG
  if(args < 0) fatal("Negative args to f_transpose()\n");
#endif
  
  if (args<1)
    error("No arguments given to transpose.\n");

  if (sp[-args].type!=T_ARRAY) 
    error("Illegal argument 1 to transpose.\n");

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
    type|=in->item->u.array->type_field;
  
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
  pop_n_elems(args);
  reset_debug_malloc();
}
#endif

#ifdef DEBUG
void f__locate_references(INT32 args)
{
  if(args)
    locate_references(sp[-args].u.refs);
  pop_n_elems(args-1);
}
#endif

void init_builtin_efuns(void)
{
  add_efun("gethrtime", f_gethrtime,"function(int|void:int)", OPT_EXTERNAL_DEPEND);

#ifdef HAVE_GETHRVTIME
  add_efun("gethrvtime",f_gethrvtime,"function(void:int)",OPT_EXTERNAL_DEPEND);
#endif
  
#ifdef PROFILING
  add_efun("get_profiling_info", f_get_prof_info,
	   "function(program:array)", OPT_EXTERNAL_DEPEND);
#endif /* PROFILING */

  add_efun("_refs",f__refs,"function(function|string|array|mapping|multiset|object|program:int)",OPT_EXTERNAL_DEPEND);
  add_efun("replace_master",f_replace_master,"function(object:void)",OPT_SIDE_EFFECT);
  add_efun("master",f_master,"function(:object)",OPT_EXTERNAL_DEPEND);
  add_efun("add_constant",f_add_constant,"function(string,void|mixed:void)",OPT_SIDE_EFFECT);
  add_efun("aggregate",f_aggregate,"function(0=mixed ...:array(0))",OPT_TRY_OPTIMIZE);
  add_efun("aggregate_multiset",f_aggregate_multiset,"function(0=mixed ...:multiset(0))",OPT_TRY_OPTIMIZE);
  add_efun("aggregate_mapping",f_aggregate_mapping,"function(0=mixed ...:mapping(0:0))",OPT_TRY_OPTIMIZE);
  add_efun("all_constants",f_all_constants,"function(:mapping(string:mixed))",OPT_EXTERNAL_DEPEND);
  add_efun("allocate", f_allocate, "function(int,void|0=mixed:array(0))", 0);
  add_efun("arrayp",  f_arrayp,  "function(mixed:int)",0);
  add_efun("backtrace",f_backtrace,"function(:array(array(function|int|string)))",OPT_EXTERNAL_DEPEND);

  add_efun("column",f_column,"function(array,mixed:array)",0);
  add_efun("combine_path",f_combine_path,"function(string...:string)",0);
  add_efun("compile",f_compile,"function(string:program)",OPT_EXTERNAL_DEPEND);
  add_efun("copy_value",f_copy_value,"function(1=mixed:1)",0);
  add_efun("crypt",f_crypt,"function(string:string)|function(string,string:int)",OPT_EXTERNAL_DEPEND);
  add_efun("ctime",f_ctime,"function(int:string)",OPT_TRY_OPTIMIZE);
  add_efun("destruct",f_destruct,"function(object|void:void)",OPT_SIDE_EFFECT);
  add_efun("equal",f_equal,"function(mixed,mixed:int)",OPT_TRY_OPTIMIZE);
  add_function("everynth",f_everynth,"function(array(0=mixed),int|void,int|void:array(0))", 0);
  add_efun("exit",f_exit,"function(int:void)",OPT_SIDE_EFFECT);
  add_efun("_exit",f__exit,"function(int:void)",OPT_SIDE_EFFECT);
  add_efun("floatp",  f_floatp,  "function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("function_name",f_function_name,"function(function:string)",OPT_TRY_OPTIMIZE);
  add_efun("function_object",f_function_object,"function(function:object)",OPT_TRY_OPTIMIZE);
  add_efun("functionp",  f_functionp,  "function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("glob",f_glob,"function(string,string:int)|function(string,string*:array(string))",OPT_TRY_OPTIMIZE);
  add_efun("hash",f_hash,"function(string,int|void:int)",OPT_TRY_OPTIMIZE);
  add_efun("indices",f_indices,"function(string|array:int*)|function(mapping(1=mixed:mixed)|multiset(1=mixed):array(1))|function(object|program:string*)",0);
  add_efun("intp",   f_intp,    "function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("multisetp",   f_multisetp,   "function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("lower_case",f_lower_case,"function(string:string)",OPT_TRY_OPTIMIZE);
  add_efun("m_delete",f_m_delete,"function(0=mapping,mixed:0)",0);
  add_efun("mappingp",f_mappingp,"function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("mkmapping",f_mkmapping,"function(array(1=mixed),array(2=mixed):mapping(1:2))",OPT_TRY_OPTIMIZE);
  add_efun("next_object",f_next_object,"function(void|object:object)",OPT_EXTERNAL_DEPEND);
  add_efun("_next",f__next,"function(string:string)|function(object:object)|function(mapping:mapping)|function(multiset:multiset)|function(program:program)|function(array:array)",OPT_EXTERNAL_DEPEND);
  add_efun("_prev",f__prev,"function(object:object)|function(mapping:mapping)|function(multiset:multiset)|function(program:program)|function(array:array)",OPT_EXTERNAL_DEPEND);
  add_efun("object_program",f_object_program,"function(mixed:program)",0);
  add_efun("objectp", f_objectp, "function(mixed:int)",0);
  add_efun("programp",f_programp,"function(mixed:int)",0);
  add_efun("query_num_arg",f_query_num_arg,"function(:int)",OPT_EXTERNAL_DEPEND);
  add_efun("random",f_random,"function(int:int)",OPT_EXTERNAL_DEPEND);
  add_efun("random_seed",f_random_seed,"function(int:void)",OPT_SIDE_EFFECT);
  add_efun("replace",f_replace,"function(string,string,string:string)|function(string,string*,string*:string)|function(0=array,mixed,mixed:0)|function(1=mapping,mixed,mixed:1)",0);
  add_efun("reverse",f_reverse,"function(int:int)|function(string:string)|function(array:array)",0);
  add_efun("rows",f_rows,"function(mixed,array:array)",0);
  add_efun("rusage", f_rusage, "function(:int *)",OPT_EXTERNAL_DEPEND);
  add_efun("search",f_search,"function(string,string,void|int:int)|function(array,mixed,void|int:int)|function(mapping,mixed:mixed)",0);
  add_efun("sleep", f_sleep, "function(float|int,int|void:void)",OPT_SIDE_EFFECT);
  add_efun("sort",f_sort,"function(array(0=mixed),array(mixed)...:array(0))",OPT_SIDE_EFFECT);
  add_function("splice",f_splice,"function(array(0=mixed)...:array(0))", 0);
  add_efun("stringp", f_stringp, "function(mixed:int)",0);
  add_efun("this_object", f_this_object, "function(:object)",OPT_EXTERNAL_DEPEND);
  add_efun("throw",f_throw,"function(mixed:void)",OPT_SIDE_EFFECT);
  add_efun("time",f_time,"function(void|int:int|float)",OPT_EXTERNAL_DEPEND);
  add_efun("trace",f_trace,"function(int:int)",OPT_SIDE_EFFECT);
  add_function("transpose",f_transpose,"function(array(0=mixed):array(0))", 0);
  add_efun("upper_case",f_upper_case,"function(string:string)",0);
  add_efun("values",f_values,"function(string|multiset:array(int))|function(array(0=mixed)|mapping(mixed:0=mixed)|object|program:array(0))",0);
  add_efun("zero_type",f_zero_type,"function(mixed:int)",0);
  add_efun("array_sscanf",f_sscanf,"function(string,string:array)",0);

#ifdef HAVE_LOCALTIME
  add_efun("localtime",f_localtime,"function(int:mapping(string:int))",OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GMTIME
  add_efun("gmtime",f_gmtime,"function(int:mapping(string:int))",OPT_EXTERNAL_DEPEND);
#endif

#ifdef HAVE_MKTIME
  add_efun("mktime",f_mktime,"function(int,int,int,int,int,int,int,void|int:int)|function(object|mapping:int)",OPT_TRY_OPTIMIZE);
#endif

#ifdef DEBUG
  add_efun("_verify_internals",f__verify_internals,"function(:void)",OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
  add_efun("_debug",f__debug,"function(int:int)",OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
#ifdef YYDEBUG
  add_efun("_compiler_trace",f__compiler_trace,"function(int:int)",OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
#endif /* YYDEBUG */
#endif
  add_efun("_memory_usage",f__memory_usage,"function(:mapping(string:int))",OPT_EXTERNAL_DEPEND);

  add_efun("gc",f_gc,"function(:int)",OPT_SIDE_EFFECT);
  add_efun("version", f_version, "function(:string)", OPT_TRY_OPTIMIZE);

  add_efun("encode_value", f_encode_value, "function(mixed,void|object:string)", OPT_TRY_OPTIMIZE);
  add_efun("decode_value", f_decode_value, "function(string,void|object:mixed)", OPT_TRY_OPTIMIZE);
  add_efun("object_variablep", f_object_variablep, "function(object,string:int)", OPT_EXTERNAL_DEPEND);

  add_function("diff",f_diff,"function(array,array:array(array))",OPT_TRY_OPTIMIZE);
  add_function("diff_longest_sequence",f_diff_longest_sequence,"function(array,array:array(int))",OPT_TRY_OPTIMIZE);
  add_function("diff_dyn_longest_sequence",f_diff_dyn_longest_sequence,"function(array,array:array(int))",OPT_TRY_OPTIMIZE);
  add_function("diff_compare_table",f_diff_compare_table,"function(array,array:array(array))",OPT_TRY_OPTIMIZE);
  add_function("longest_ordered_sequence",f_longest_ordered_sequence,"function(array:array(int))",0);
  add_function("sort",f_sort,"function(array(mixed),array(mixed)...:array(mixed))",OPT_SIDE_EFFECT);
#ifdef DEBUG_MALLOC
  add_efun("_reset_dmalloc",f__reset_dmalloc,"function(void:void)",OPT_SIDE_EFFECT);
#endif
#ifdef DEBUG
  add_efun("_locate_references",f__locate_references,"function(1=mixed:1)",OPT_SIDE_EFFECT);
#endif
}

