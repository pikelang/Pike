/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: builtin_functions.c,v 1.56 1997/11/08 01:34:36 hubbe Exp $");
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

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif


void f_equal(INT32 args)
{
  int i;
  if(args < 2)
    error("Too few arguments to equal.\n");

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
    error("Too few arguments to hash()\n");
  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to hash()\n");
  i=hashstr((unsigned char *)sp[-args].u.string->str,100);

  if(args > 1)
  {
    if(sp[1-args].type != T_INT)
      error("Bad argument 2 to hash()\n");
    
    if(!sp[1-args].u.integer)
      error("Modulo by zero in hash()\n");

    i%=(unsigned INT32)sp[1-args].u.integer;
  }
  pop_n_elems(args);
  push_int(i);
}

void f_copy_value(INT32 args)
{
  if(!args)
    error("Too few arguments to copy_value()\n");

  pop_n_elems(args-1);
  copy_svalues_recursively_no_free(sp,sp-1,1,0);
  free_svalue(sp-1);
  sp[-1]=sp[0];
}

void f_ctime(INT32 args)
{
  INT32 i;
  if(!args)
    error("Too few arguments to ctime()\n");
  if(sp[-args].type != T_INT)
    error("Bad argument 1 to ctime()\n");
  i=sp[-args].u.integer;
  pop_n_elems(args);
  push_string(make_shared_string(ctime((time_t *)&i)));
}

void f_lower_case(INT32 args)
{
  INT32 i;
  struct pike_string *ret;
  if(!args)
    error("Too few arguments to lower_case()\n");
  if(sp[-args].type != T_STRING) 
    error("Bad argument 1 to lower_case()\n");

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
    error("Too few arguments to upper_case()\n");
  if(sp[-args].type != T_STRING) 
    error("Bad argument 1 to upper_case()\n");

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
    error("Too few arguments to random()\n");
  if(sp[-args].type != T_INT) 
    error("Bad argument 1 to random()\n");

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
    error("Too few arguments to random_seed()\n");
  if(sp[-args].type != T_INT) 
    error("Bad argument 1 to random_seed()\n");

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
    error("Too few arguments to search().\n");

  switch(sp[-args].type)
  {
  case T_STRING:
  {
    char *ptr;
    INT32 len;
    if(sp[1-args].type != T_STRING)
      error("Bad argument 2 to search()\n");
    
    start=0;
    if(args > 2)
    {
      if(sp[2-args].type!=T_INT)
	error("Bad argument 3 to search()\n");

      start=sp[2-args].u.integer;
    }
    len=sp[-args].u.string->len - start;

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
	error("Bad argument 3 to search()\n");

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
    error("Bad argument 2 to search()\n");
  }
}

void f_call_function(INT32 args)
{
  INT32 expected_stack=sp-args+2-evaluator_stack;

  strict_apply_svalue(sp-args, args - 1);
  if(sp < expected_stack + evaluator_stack)
  {
#ifdef DEBUG
    if(sp+1 != expected_stack + evaluator_stack)
      fatal("Stack underflow!\n");
#endif

    pop_stack();
    push_int(0);
  }else{
    free_svalue(sp-2);
    sp[-2]=sp[-1];
    sp--;
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
	f->current_object->refs++;
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
    error("Too few arguments to add_efun.\n");

  if(sp[-args].type!=T_STRING)
    error("Bad argument 1 to add_efun.\n");

  if(args>1)
  {
    low_add_efun(sp[-args].u.string, sp-args+1);
  }else{
    low_add_efun(sp[-args].u.string, 0);
  }
  pop_n_elems(args);
}

void f_compile_file(INT32 args)
{
  struct program *p;
  if(args<1)
    error("Too few arguments to compile_file.\n");

  if(sp[-args].type!=T_STRING)
    error("Bad argument 1 to compile_file.\n");

  p=compile_file(sp[-args].u.string);
  pop_n_elems(args);
  push_program(p);
}

static char *combine_path(char *cwd,char *file)
{
  /* cwd is supposed to be combined already */
  char *ret;
  register char *from,*to;
  char *my_cwd;
  my_cwd=0;
  

  if(file[0]=='/')
  {
    cwd="/";
    file++;
  }
#ifdef DEBUG    
  if(!cwd)
    fatal("No cwd in combine_path!\n");
#endif

  if(!*cwd || cwd[strlen(cwd)-1]=='/')
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
  while(from[0]=='.' && from[1]=='/') from+=2;
  
  while(( *to = *from ))
  {
    if(*from == '/')
    {
      while(from[1] == '/') from++;
      if(from[1] == '.')
      {
	switch(from[2])
	{
	case '.':
	  if(from[3] == '/' || from[3] == 0)
	  {
	    char *tmp=to;
	    while(--tmp>=ret)
	      if(*tmp == '/')
		break;

	    if(tmp[1]=='.' && tmp[2]=='.' && (tmp[3]=='/' || !tmp[3]))
	      break;
	    
	    from+=3;
	    to=tmp;
	    if(to<ret)
	    {
	      to++;
	      if(*from) from++;
	    }
	    continue;
	  }
	  break;

	case 0:
	case '/':
	  from+=2;
	  continue;
	}
      }
    }
    from++;
    to++;
  }
  if(!*ret)
  {
    if(*cwd=='/')
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
  char *path;
  if(args<2)
    error("Too few arguments to combine_path.\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to combine_path.\n");

  if(sp[1-args].type != T_STRING)
    error("Bad argument 2 to combine_path.\n");

  path=combine_path(sp[-args].u.string->str,sp[1-args].u.string->str);
  pop_n_elems(args);

  sp->u.string=make_shared_string(path);
  sp->type=T_STRING;
  sp++;
  free(path);
}

void f_function_object(INT32 args)
{
  if(args < 1)
    error("Too few arguments to function_object()\n");
  if(sp[-args].type != T_FUNCTION)
    error("Bad argument 1 to function_object.\n");

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
    error("Too few arguments to function_object()\n");
  if(sp[-args].type != T_FUNCTION)
    error("Bad argument 1 to function_object.\n");

  if(sp[-args].subtype == FUNCTION_BUILTIN)
  {
    pop_n_elems(args);
    push_int(0);
  }else{
    if(!sp[-args].u.object->prog)
      error("function_name on destructed object.\n");

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
    error("Too few arguments to zero_type()\n");
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
  push_mapping(get_builtin_constants());
  sp[-1].u.mapping->refs++;
}

void f_allocate(INT32 args)
{
  INT32 size;

  if(args < 1)
    error("Too few arguments to allocate.\n");

  if(sp[-args].type!=T_INT)
    error("Bad argument 1 to allocate.\n");


  size=sp[-args].u.integer;
  if(size < 0)
    error("Allocate on negative number.\n");
  pop_n_elems(args);
  push_array( allocate_array(size) );
}

void f_rusage(INT32 args)
{
  INT32 *rus,e;
  struct array *v;
  pop_n_elems(args);
  rus=low_rusage();
  if(!rus)
    error("System rusage information not available.\n");
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
    fp->current_object->refs++;
    sp++;
  }else{
    push_int(0);
  }
}

void f_throw(INT32 args)
{
  if(args < 1)
    error("Too few arguments to throw()\n");
  pop_n_elems(args-1);
  throw_value=sp[-1];
  sp--;
  throw();
}

static struct callback_list exit_callbacks;

struct callback *add_exit_callback(callback_func call,
				   void *arg,
				   callback_func free_func)
{
  return add_to_callback(&exit_callbacks, call, arg, free_func);
}

void f_exit(INT32 args)
{
  ONERROR tmp;
  int i;
  if(args < 1)
    error("Too few arguments to exit.\n");

  if(sp[-args].type != T_INT)
    error("Bad argument 1 to exit.\n");

  i=sp[-args].u.integer;

#ifdef _REENTRANT
  if(num_threads>1) exit(i);
#endif

  SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");

  call_callback(&exit_callbacks, (void *)0);
  free_callback(&exit_callbacks);

  exit_modules();

  UNSET_ONERROR(tmp);
  exit(i);
}

void f_time(INT32 args)
{
  pop_n_elems(args);
  if(args)
    push_int(current_time.tv_sec);
  else
    push_int((INT32)TIME(0));
}

void f_crypt(INT32 args)
{
  char salt[2];
  char *ret, *saltp;
  char *choise =
    "cbhisjKlm4k65p7qrJfLMNQOPxwzyAaBDFgnoWXYCZ0123tvdHueEGISRTUV89./";

  if(args < 1)
    error("Too few arguments to crypt()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to crypt()\n");

  
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
      error("Bad arguments 1 to destruct()\n");
    
    o=sp[-args].u.object;
  }else{
    if(!fp)
      error("Destruct called without argument from callback function.\n");
	   
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
    error("Too few arguments to indices()\n");

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

  default:
    error("Bad argument 1 to indices()\n");
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
    error("Too few arguments to values()\n");

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

  default:
    error("Bad argument 1 to values()\n");
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
      error("Bad argument 1 to next_object()\n");
    o=sp[-args].u.object->next;
  }
  pop_n_elems(args);
  if(!o)
  {
    push_int(0);
  }else{
    o->refs++;
    push_object(o);
  }
}

void f_object_program(INT32 args)
{
  struct program *p;
  if(args < 1)
    error("Too few argumenets to object_program()\n");

  if(sp[-args].type == T_OBJECT)
    p=sp[-args].u.object->prog;
  else
    p=0;

  pop_n_elems(args);

  if(!p)
  {
    push_int(0);
  }else{
    p->refs++;
    push_program(p);
  }
}

void f_reverse(INT32 args)
{
  if(args < 1)
    error("Too few arguments to reverse()\n");

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
    error("Bad argument 1 to reverse()\n");
    
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
      error("Replace: from array not string *\n");
    v[e].ind=ITEM(from)[e].u.string;
  }

  for(e=0;e<to->size;e++)
  {
    if(ITEM(to)[e].type != T_STRING)
      error("Replace: to array not string *\n");
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
    error("Too few arguments to replace()\n");

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
      error("Bad argument 2 to replace()\n");
      
    case T_STRING:
      if(sp[2-args].type != T_STRING)
	error("Bad argument 3 to replace()\n");

      s=string_replace(sp[-args].u.string,
		       sp[1-args].u.string,
		       sp[2-args].u.string);
      break;
      
    case T_ARRAY:
      if(sp[2-args].type != T_ARRAY)
	error("Bad argument 3 to replace()\n");

      s=replace_many(sp[-args].u.string,
		     sp[1-args].u.array,
		     sp[2-args].u.array);
    
    }
    pop_n_elems(args);
    push_string(s);
    break;
  }

  default:
    error("Bad argument 1 to replace().\n");
  }
}

void f_compile_string(INT32 args)
{
  
  struct program *p;
  if(args < 1)
    error("Too few arguments to compile_string()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to compile_string()\n");

  if(args < 2)
  {
    push_string(make_shared_string("-"));
    args++;
  }

  if(sp[1-args].type != T_STRING)
    error("Bad argument 2 to compile_string()\n");

  p=compile_string(sp[-args].u.string,sp[1-args].u.string);
  pop_n_elems(args);
  push_program(p);
}

void f_mkmapping(INT32 args)
{
  struct mapping *m;
  struct array *a,*b;
  get_all_args("mkmapping",args,"%a%a",&a,&b);
  if(a->size != b->size)
    error("mkmapping called on arrays of different sizes\n");

  m=mkmapping(sp[-args].u.array, sp[1-args].u.array);
  pop_n_elems(args);
  push_mapping(m);
}

void f_objectp(INT32 args)
{
  if(args<1) error("Too few arguments to objectp.\n");
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
  if(args<1) error("Too few arguments to functionp.\n");
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

void f_sleep(INT32 args)
{
  struct timeval t1,t2,t3;
  INT32 a,b;

  if(!args)
    error("Too few arguments to sleep.\n");

  GETTIMEOFDAY(&t1);

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
    error("Bad argument 1 to sleep.\n");
  }

  my_add_timeval(&t1, &t2);
  
  pop_n_elems(args);
  while(1)
  {
    GETTIMEOFDAY(&t2);
    if(my_timercmp(&t1, <= , &t2))
      break;

    t3=t1;
    my_subtract_timeval(&t3, &t2);

    THREADS_ALLOW();
    select(0,0,0,0,&t3);
    THREADS_DISALLOW();
    check_threads_etc();
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
  if(args<1) error("Too few arguments to %s.\n",NAME); \
  t=sp[-args].type == TYPE; \
  pop_n_elems(args); \
  push_int(t); \
}

TYPEP(f_programp, "programp", T_PROGRAM)
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
    error("Too few arguments to rows().\n");

  if(sp[1-args].type!=T_ARRAY)
    error("Bad argument 1 to rows().\n");

  tmp=sp[1-args].u.array;
  push_array(a=allocate_array(tmp->size));

  for(e=0;e<a->size;e++)
    index_no_free(ITEM(a)+e, sp-args-1, ITEM(tmp)+e);

  a->refs++;
  pop_n_elems(args+1);
  push_array(a);
}

void f_column(INT32 args)
{
  INT32 e;
  struct array *a,*tmp;
  DECLARE_CYCLIC();

  if(args < 2)
    error("Too few arguments to column().\n");

  if(sp[-args].type!=T_ARRAY)
    error("Bad argument 1 to column().\n");

  tmp=sp[-args].u.array;
  if((a=(struct array *)BEGIN_CYCLIC(tmp,0)))
  {
    a->refs++;
    pop_n_elems(args);
    push_array(a);
  }else{
    push_array(a=allocate_array(tmp->size));
    SET_CYCLIC_RET(a);

    for(e=0;e<a->size;e++)
      index_no_free(ITEM(a)+e, ITEM(tmp)+e, sp-args);

    END_CYCLIC();
    a->refs++;
    pop_n_elems(args+1);
    push_array(a);
  }
}

#ifdef DEBUG
void f__verify_internals(INT32 args)
{
  INT32 tmp;
  tmp=d_flag;
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

#endif

#ifdef HAVE_LOCALTIME
void f_localtime(INT32 args)
{
  struct tm *tm;
  time_t t;
  if (args<1 || sp[-1].type!=T_INT)
    error("Illegal argument to localtime");

  t=sp[-1].u.integer;
  tm=localtime(&t);
  pop_n_elems(args);

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
  INT32 sec, min, hour, mday, mon, year, isdst, tz;
  struct tm date;
  struct svalue s;
  struct svalue * r;
  int retval;
  if (args<1)
    error ("Too few arguments to mktime().\n");

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

  get_all_args("mktime",args, "%i%i%i%i%i%i%i",
	       &sec, &min, &hour, &mday, &mon, &year, &isdst, &tz);

  
  date.tm_sec=sec;
  date.tm_min=min;
  date.tm_hour=hour;
  date.tm_mday=mday;
  date.tm_mon=mon;
  date.tm_year=year;
  date.tm_isdst=isdst;

#if STRUCT_TM_HAS_GMTOFF
  date.tm_gmtoff=tz;
  retval=mktime(&date);
#else
#ifdef HAVE_EXTERNAL_TIMEZONE
  if(sp[8-args].subtype == NUMBER_NUMBER)
  {
    int save_timezone=timezone;
    timezone=tz;
    retval=mktime(&date);
    timezone=save_timezone;
  }else{
    retval=mktime(&date);
  }
#else
  retval=mktime(&date);
#endif
#endif

  if (retval == -1)
    error ("mktime: Cannot convert.\n");
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
    error("Too few arguments to glob().\n");

  if(args > 2) pop_n_elems(args-2);
  args=2;

  if (sp[-args].type!=T_STRING)
    error("Bad argument 2 to glob().\n");

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
	error("Bad argument 2 to glob()\n");

      if(does_match(ITEM(a)[i].u.string->str,
		    ITEM(a)[i].u.string->len,
		    glob->str,
		    glob->len))
      {
	ITEM(a)[i].u.string->refs++;
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
    error("Bad argument 2 to glob().\n");
  }
}

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
    error("Too few arguments to _next()\n");
  
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
    error("Bad argument 1 to _next()\n");
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
    error("Too few arguments to _next()\n");
  
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
    error("Bad argument 1 to _prev()\n");
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
  if(!args) error("Too few arguments to _refs()\n");
  if(sp[-args].type > MAX_REF_TYPE)
    error("Bad argument 1 to _refs()\n");

  i=sp[-args].u.refs[0];
  pop_n_elems(args);
  push_int(i);
}

void f_replace_master(INT32 args)
{
  if(!args)
    error("Too few arguments to replace_master()\n");
  if(sp[-args].type != T_OBJECT)
    error("Bad argument 1 to replace_master()\n"); 
 if(!sp[-args].u.object->prog)
    error("replace_master() called with destructed object.\n");
    
  free_object(master_object);
  master_object=sp[-args].u.object;
  master_object->refs++;

  free_program(master_program);
  master_program=master_object->prog;
  master_program->refs++;

  pop_n_elems(args);
}

void f_master(INT32 args)
{
  pop_n_elems(args);
  master_object->refs++;
  push_object(master_object);
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
  push_int((INT32)(gethrtime()/1000)); 
}
#endif /* HAVE_GETHRVTIME */

#ifdef PROFILING
static void f_get_prof_info(INT32 args)
{
  struct program *prog;
  int num_functions;
  int i;

  if (!args) {
    error("get_profiling_info(): Too few arguments\n");
  }
  if (sp[-args].type != T_PROGRAM) {
    error("get_profiling_info(): Bad argument 1\n");
  }

  prog = sp[-args].u.program;
  prog->refs++;

  pop_n_elems(args);

  /* ({ num_clones, ([ "fun_name":({ num_calls }) ]) }) */

  push_int(prog->num_clones);

  for(num_functions=i=0; i<(int)prog->num_identifiers; i++) {
    if (IDENTIFIER_IS_FUNCTION(prog->identifiers[i].identifier_flags)) {
      num_functions++;
      prog->identifiers[i].name->refs++;
      push_string(prog->identifiers[i].name);

      push_int(prog->identifiers[i].num_calls);
      f_aggregate(1);
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
    error("variablep() called on destructed object.\n");

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

void init_builtin_efuns(void)
{
  init_operators();

#ifdef HAVE_GETHRVTIME
  add_efun("gethrvtime",f_gethrvtime,"function(void:int)",OPT_EXTERNAL_DEPEND);
  add_efun("gethrtime", f_gethrtime,"function(void:int)", OPT_EXTERNAL_DEPEND);
#endif
  
#ifdef PROFILING
  add_efun("get_profiling_info", f_get_prof_info,
	   "function(program:array)", OPT_EXTERNAL_DEPEND);
#endif /* PROFILING */

  add_efun("_refs",f__refs,"function(function|string|array|mapping|multiset|object|program:int)",OPT_EXTERNAL_DEPEND);
  add_efun("replace_master",f_replace_master,"function(object:void)",OPT_SIDE_EFFECT);
  add_efun("master",f_master,"function(:object)",OPT_EXTERNAL_DEPEND);
  add_efun("add_constant",f_add_constant,"function(string,void|mixed:void)",OPT_SIDE_EFFECT);
  add_efun("aggregate",f_aggregate,"function(mixed ...:mixed *)",OPT_TRY_OPTIMIZE);
  add_efun("aggregate_multiset",f_aggregate_multiset,"function(mixed ...:multiset)",OPT_TRY_OPTIMIZE);
  add_efun("aggregate_mapping",f_aggregate_mapping,"function(mixed ...:mapping)",OPT_TRY_OPTIMIZE);
  add_efun("all_constants",f_all_constants,"function(:mapping(string:mixed))",OPT_EXTERNAL_DEPEND);
  add_efun("allocate", f_allocate, "function(int, string|void:mixed *)", 0);
  add_efun("arrayp",  f_arrayp,  "function(mixed:int)",0);
  add_efun("backtrace",f_backtrace,"function(:array(array(function|int|string)))",OPT_EXTERNAL_DEPEND);
  add_efun("call_function",f_call_function,"function(mixed,mixed ...:mixed)",OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);

  add_efun("column",f_column,"function(array,mixed:array)",0);
  add_efun("combine_path",f_combine_path,"function(string,string:string)",0);
  add_efun("compile_file",f_compile_file,"function(string:program)",OPT_EXTERNAL_DEPEND);
  add_efun("compile_string",f_compile_string,"function(string,string|void:program)",OPT_EXTERNAL_DEPEND);
  add_efun("copy_value",f_copy_value,"function(mixed:mixed)",0);
  add_efun("crypt",f_crypt,"function(string:string)|function(string,string:int)",OPT_EXTERNAL_DEPEND);
  add_efun("ctime",f_ctime,"function(int:string)",OPT_TRY_OPTIMIZE);
  add_efun("destruct",f_destruct,"function(object|void:void)",OPT_SIDE_EFFECT);
  add_efun("equal",f_equal,"function(mixed,mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("exit",f_exit,"function(int:void)",OPT_SIDE_EFFECT);
  add_efun("floatp",  f_floatp,  "function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("function_name",f_function_name,"function(function:string)",OPT_TRY_OPTIMIZE);
  add_efun("function_object",f_function_object,"function(function:object)",OPT_TRY_OPTIMIZE);
  add_efun("functionp",  f_functionp,  "function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("glob",f_glob,"function(string,string:int)|function(string,string*:array(string))",OPT_TRY_OPTIMIZE);
  add_efun("hash",f_hash,"function(string,int|void:int)",OPT_TRY_OPTIMIZE);
  add_efun("indices",f_indices,"function(string|array:int*)|function(mapping|multiset:mixed*)|function(object:string*)",0);
  add_efun("intp",   f_intp,    "function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("multisetp",   f_multisetp,   "function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("lower_case",f_lower_case,"function(string:string)",OPT_TRY_OPTIMIZE);
  add_efun("m_delete",f_m_delete,"function(mapping,mixed:mapping)",0);
  add_efun("mappingp",f_mappingp,"function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("mkmapping",f_mkmapping,"function(mixed *,mixed *:mapping)",OPT_TRY_OPTIMIZE);
  add_efun("next_object",f_next_object,"function(void|object:object)",OPT_EXTERNAL_DEPEND);
  add_efun("_next",f__next,"function(string:string)|function(object:object)|function(mapping:mapping)|function(multiset:multiset)|function(program:program)|function(array:array)",OPT_EXTERNAL_DEPEND);
  add_efun("_prev",f__prev,"function(object:object)|function(mapping:mapping)|function(multiset:multiset)|function(program:program)|function(array:array)",OPT_EXTERNAL_DEPEND);
  add_efun("object_program",f_object_program,"function(mixed:program)",0);
  add_efun("objectp", f_objectp, "function(mixed:int)",0);
  add_efun("programp",f_programp,"function(mixed:int)",0);
  add_efun("query_num_arg",f_query_num_arg,"function(:int)",OPT_EXTERNAL_DEPEND);
  add_efun("random",f_random,"function(int:int)",OPT_EXTERNAL_DEPEND);
  add_efun("random_seed",f_random_seed,"function(int:void)",OPT_SIDE_EFFECT);
  add_efun("replace",f_replace,"function(string,string,string:string)|function(string,string*,string*:string)|function(array,mixed,mixed:array)|function(mapping,mixed,mixed:array)",0);
  add_efun("reverse",f_reverse,"function(int:int)|function(string:string)|function(array:array)",0);
  add_efun("rows",f_rows,"function(mixed,array:array)",0);
  add_efun("rusage", f_rusage, "function(:int *)",OPT_EXTERNAL_DEPEND);
  add_efun("search",f_search,"function(string,string,void|int:int)|function(array,mixed,void|int:int)|function(mapping,mixed:mixed)",0);
  add_efun("sleep", f_sleep, "function(float|int:void)",OPT_SIDE_EFFECT);
  add_efun("sort",f_sort,"function(array(mixed),array(mixed)...:array(mixed))",OPT_SIDE_EFFECT);
  add_efun("stringp", f_stringp, "function(mixed:int)",0);
  add_efun("this_object", f_this_object, "function(:object)",OPT_EXTERNAL_DEPEND);
  add_efun("throw",f_throw,"function(mixed:void)",OPT_SIDE_EFFECT);
  add_efun("time",f_time,"function(void|int:int)",OPT_EXTERNAL_DEPEND);
  add_efun("trace",f_trace,"function(int:int)",OPT_SIDE_EFFECT);
  add_efun("upper_case",f_upper_case,"function(string:string)",0);
  add_efun("values",f_values,"function(string|multiset:int*)|function(array|mapping|object:mixed*)",0);
  add_efun("zero_type",f_zero_type,"function(mixed:int)",0);

#ifdef HAVE_LOCALTIME
  add_efun("localtime",f_localtime,"function(int:mapping(string:int))",OPT_EXTERNAL_DEPEND);
#endif

#ifdef HAVE_MKTIME
  add_efun("mktime",f_mktime,"function(int,int,int,int,int,int,int,int:int)|function(object|mapping:int)",OPT_TRY_OPTIMIZE);
#endif

#ifdef DEBUG
  add_efun("_verify_internals",f__verify_internals,"function(:void)",OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
  add_efun("_debug",f__debug,"function(int:int)",OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
#endif
  add_efun("_memory_usage",f__memory_usage,"function(:mapping(string:int))",OPT_EXTERNAL_DEPEND);

  add_efun("gc",f_gc,"function(:int)",OPT_SIDE_EFFECT);
  add_efun("version", f_version, "function(:string)", OPT_TRY_OPTIMIZE);

  add_efun("encode_value", f_encode_value, "function(mixed:string)", OPT_TRY_OPTIMIZE);
  add_efun("decode_value", f_decode_value, "function(string:mixed)", OPT_TRY_OPTIMIZE);
  add_efun("object_variablep", f_object_variablep, "function(object,string:int)", OPT_EXTERNAL_DEPEND);
}

