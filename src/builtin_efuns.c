/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "interpret.h"
#include "svalue.h"
#include "macros.h"
#include "object.h"
#include "program.h"
#include "array.h"
#include "error.h"
#include "add_efun.h"
#include "mapping.h"
#include "stralloc.h"
#include "lex.h"
#include "list.h"
#include "lpc_types.h"
#include "rusage.h"
#include "operators.h"
#include "fsort.h"
#include "call_out.h"
#include "callback.h"
#include <sys/time.h>
#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
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

  a=aggregate_array(args,T_MIXED);
  push_array(a); /* beware, macro */
}

void f_trace(INT32 args)
{
  extern int t_flag;
  int old_t_flag;

  if(args < 0)
    error("Too few arguments to trace()\n");
  
  if(sp[-args].type != T_INT)
    error("Bad argument 1 to trace()\n");

  old_t_flag=t_flag;
  t_flag=sp[-args].u.integer;
  sp[-args].u.integer=old_t_flag;
  pop_n_elems(args-1);
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
  struct lpc_string *ret;
  if(!args)
    error("Too few arguments to lower_case()\n");
  if(sp[-args].type != T_STRING) 
    error("Bad argument 1 to lower_case()\n");

  ret=begin_shared_string(sp[-args].u.string->len);
  MEMCPY(ret->str, sp[-args].u.string->str,sp[-args].u.string->len);

  for (i = sp[-args].u.string->len-1; i>=0; i--)
    if (isupper(ret->str[i]))
      ret->str[i] = tolower(ret->str[i]);

  pop_n_elems(args);
  push_string(end_shared_string(ret));
}

void f_upper_case(INT32 args)
{
  INT32 i;
  struct lpc_string *ret;
  if(!args)
    error("Too few arguments to upper_case()\n");
  if(sp[-args].type != T_STRING) 
    error("Bad argument 1 to upper_case()\n");

  ret=begin_shared_string(sp[-args].u.string->len);
  MEMCPY(ret->str, sp[-args].u.string->str,sp[-args].u.string->len);

  for (i = sp[-args].u.string->len-1; i>=0; i--)
    if (islower(ret->str[i]))
      ret->str[i] = toupper(ret->str[i]);

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
  pop_n_elems(args-1);
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

    if(len>0 && (ptr=MEMMEM(sp[1-args].u.string->str,
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

void f_clone(INT32 args)
{
  struct object *o;

  if(args<1)
    error("Too few arguments to clone.\n");

  if(sp[-args].type != T_PROGRAM)
    error("Bad argument 1 to clone.\n");

  o=clone(sp[-args].u.program,args-1);
  pop_stack();
  push_object(o);
}

void f_call_function(INT32 args)
{
  strict_apply_svalue(sp-args, args - 1);
  free_svalue(sp-2);
  sp[-2]=sp[-1];
  sp--;
}

void f_backtrace(INT32 args)
{
  INT32 frames;
  struct frame *f;
  struct array *a,*i;

  frames=0;
  if(args) pop_n_elems(args);
  for(f=fp;f;f=f->parent_frame) frames++;

  sp->type=T_ARRAY;
  sp->u.array=a=allocate_array_no_init(frames,0,T_ARRAY);
  sp++;

  for(f=fp;f;f=f->parent_frame)
  {
    char *program_name;

    frames--;

    if(f->current_object && f->current_object->prog)
    {
      SHORT_ITEM(a)[frames].array=i=allocate_array_no_init(3,0,T_MIXED);
      ITEM(i)[2].type=T_FUNCTION;
      ITEM(i)[2].subtype=f->fun;
      ITEM(i)[2].u.object=f->current_object;
      f->current_object->refs++;

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
      SHORT_ITEM(a)[frames].refs=0;
    }
  }
}

void f_add_efun(INT32 args)
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
  }else{
    
    if(!cwd)
    {
#ifdef HAVE_GETWD

#ifndef MAXPATHLEN
#define MAXPATHLEN 1000
#endif

      cwd=(char *)getwd(my_cwd=(char *)xalloc(MAXPATHLEN+1));
      if(!cwd)
	fatal("Couldn't fetch current path.\n");
#else
#ifdef HAVE_GETCWD
      my_cwd=cwd=(char *)getcwd(0,1000); 
#else
      /* maybe autoconf was wrong.... if not, insert your method here */
      my_cwd=cwd=(char *)getcwd(0,1000); 
#endif
#endif
    }
  }

  if(cwd[strlen(cwd)-1]=='/')
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
	    from+=3;
	    if(to != ret)
	    {
	      for(--to;*to!='/' && to>ret;to--);
	    }
	    if(!*from && to==ret && *to=='/') to++;
	    continue;
	  }
	  break;

	case '/':
	case 0:
	  from+=2;
	  continue;
	}
      }
    }
    from++;
    to++;
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

void f_implode(INT32 args)
{
  struct lpc_string *ret;

  if(args < 1)
    error("Too few arguments to implode.\n");

  if(sp[-args].type != T_ARRAY)
    error("Bad argument 1 to implode.\n");

  if(args<2)
  {
    push_string(make_shared_string(""));
  }else{
    pop_n_elems(args-2);
    if(sp[-1].type != T_STRING)
      error("Bad argument 2 to implode.\n");
  }

  ret=implode(sp[-2].u.array, sp[-1].u.string);
  pop_n_elems(2);
  push_string(ret);
}

void f_explode(INT32 args)
{
  struct array *ret;
  if(args < 2)
    error("Too few arguments to explode.\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to explode.\n");

  if(sp[1-args].type != T_STRING)
    error("Bad argument 2 to explode.\n");

  ret=explode(sp[-args].u.string, sp[1-args].u.string);
  pop_n_elems(args);
  push_array(ret);
}

void f_function_object(INT32 args)
{
  if(args < 1)
    error("Too few arguments to function_object()\n");
  if(sp[-args].type != T_FUNCTION)
    error("Bad argument 1 to function_object.\n");

  if(sp[-args].subtype == -1)
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
  struct lpc_string *s;
  if(args < 1)
    error("Too few arguments to function_object()\n");
  if(sp[-args].type != T_FUNCTION)
    error("Bad argument 1 to function_object.\n");

  if(sp[-args].subtype == -1)
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
    error("Bad argument 1 to zero_type.\n");

  pop_n_elems(args-1);
  sp[-1].u.integer=sp[-1].subtype;
  sp[-1].subtype=NUMBER_NUMBER;
}

void f_all_efuns(INT32 args)
{
  struct svalue *save_sp;
  pop_n_elems(args);
  save_sp=sp;
  push_all_efuns_on_stack();
  f_aggregate_mapping(sp-save_sp);
}

void f_allocate(INT32 args)
{
  TYPE_T t;
  INT32 size;

  if(args < 1)
    error("Too few arguments to allocate.\n");

  if(sp[-args].type!=T_INT)
    error("Bad argument 1 to allocate.\n");

  if(args > 1)
  {
    struct lpc_string *s;
    if(sp[1-args].type != T_STRING)
      error("Bad argument 2 to allocate.\n");

    s=sp[1-args].u.string;
    s=parse_type(s->str);
    t=compile_type_to_runtime_type(s);
    free_string(s);
  }else{
    t=T_MIXED;
  }

  size=sp[-args].u.integer;
  if(size < 0)
    error("Allocate on negative number.\n");
  pop_n_elems(args);
  push_array( allocate_array(size, t) );
}

void f_sizeof(INT32 args)
{
  INT32 tmp;
  if(args<1)
    error("Too few arguments to sizeof()\n");

  pop_n_elems(args-1);
  switch(sp[-1].type)
  {
  case T_STRING:
    tmp=sp[-1].u.string->len;
    free_string(sp[-1].u.string);
    break;

  case T_ARRAY:
    tmp=sp[-1].u.array->size;
    free_array(sp[-1].u.array);
    break;

  case T_MAPPING:
    tmp=sp[-1].u.mapping->ind->size;
    free_mapping(sp[-1].u.mapping);
    break;

  case T_LIST:
    tmp=sp[-1].u.list->ind->size;
    free_list(sp[-1].u.list);
    break;
    
  default:
    error("Bad argument 1 to sizeof().\n");
    return; /* make apcc happy */
  }
  sp[-1].type=T_INT;
  sp[-1].subtype=NUMBER_NUMBER;
  sp[-1].u.integer=tmp;
}

void f_rusage(INT32 args)
{
  INT32 *rus;
  struct array *v;
  pop_n_elems(args);
  rus=low_rusage();
  if(!rus)
    error("System rusage information not available.\n");
  v=allocate_array_no_init(29,0,T_INT);
  MEMCPY((char *)SHORT_ITEM(v),(char *)rus,sizeof(INT32)*29);
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

static struct callback_list *exit_callbacks=0;

struct callback_list *add_exit_callback(struct array *a)
{
  return add_to_callback_list(&exit_callbacks, a);

}

void f_exit(INT32 args)
{
  int i;
  if(args < 1)
    error("Too few arguments to exit.\n");

  if(sp[-args].type != T_INT)
    error("Bad argument 1 to exit.\n");

  call_and_free_callback_list(& exit_callbacks);

  i=sp[-args].u.integer;
#define DEALLOCATE_MEMORY
#ifdef DEALLOCATE_MEMORY
  exit_modules();
#endif

  exit(i);
}

void f_query_host_name(INT32 args)
{
  char hostname[1000];
  pop_n_elems(args);
  gethostname(hostname,1000);
  push_string(make_shared_string(hostname));
}

void f_time(INT32 args)
{
  extern time_t current_time;
  pop_n_elems(args);
  if(args)
    push_int(current_time);
  else
    push_int(get_current_time());
}

void f_crypt(INT32 args)
{
  char salt[2];
  char *ret;
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
      error("Bad argument 2 to crypt()\n");
      
    salt[0] = sp[1-args].u.string->str[0];
    salt[1] = sp[1-args].u.string->str[1];
  } else {
    salt[0] = choise[my_rand()%strlen(choise)];
    salt[1] = choise[my_rand()%strlen(choise)];
  }
#ifdef HAVE_CRYPT
  ret = (char *)crypt(sp[-args].u.string->str, salt);
#else
#ifdef HAVE__CRYPT
  ret = (char *)_crypt(sp[-args].u.string->str, salt);
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
  push_int(0);
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
    a=allocate_array_no_init(size,0,T_INT);
    while(size>0)
      SHORT_ITEM(a)[--size].integer=size;
    break;

  case T_MAPPING:
    a=copy_array(sp[-args].u.mapping->ind);
    break;

  case T_LIST:
    a=copy_array(sp[-args].u.list->ind);
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
    error("Too few arguments to indices()\n");

  switch(sp[-args].type)
  {
  case T_STRING:
    size=sp[-args].u.string->len;
    a=allocate_array_no_init(size,0,T_INT);
    while(size>0)
      SHORT_ITEM(a)[--size].integer=EXTRACT_UCHAR(sp[-args].u.string->str+size);
    break;

  case T_ARRAY:
    a=copy_array(sp[-args].u.array);
    break;

  case T_MAPPING:
    a=copy_array(sp[-args].u.mapping->val);
    break;

  case T_LIST:
    size=sp[-args].u.list->ind->size;
    a=allocate_array_no_init(size,0,T_INT);
    while(size>0)
      SHORT_ITEM(a)[--size].integer=1;
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

  if(sp[-args].type != T_OBJECT)
    error("Bad argument 1 to object_program()\n");

  p=sp[-args].u.object->prog;
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
    struct lpc_string *s;
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
    e=((e & 0x55555555)<<1) + ((e & 0xaaaaaaaa)>>1);
    e=((e & 0x33333333)<<2) + ((e & 0xcccccccc)>>2);
    e=((e & 0x0f0f0f0f)<<4) + ((e & 0xf0f0f0f0)>>4);
    e=((e & 0x00ff00ff)<<8) + ((e & 0xff00ff00)>>8);
    e=((e & 0x0000ffff)<<16)+ ((e & 0xffff0000)>>16);
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
  struct lpc_string *ind,*val;
};

static int replace_sortfun(void *a,void *b)
{
  return my_quick_strcmp( ((struct tupel *)a)->ind, ((struct tupel *)b)->ind);
}

struct lpc_string * replace_many(struct lpc_string *str,
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

  if(from->array_type == T_MIXED)
  {
    for(e=0;e<from->size;e++)
    {
      if(ITEM(from)[e].type != T_STRING)
	error("Replace: from array not string *\n");
      v[e].ind=ITEM(from)[e].u.string;
    }
  }
  else if(from->array_type == T_STRING)
  {
    for(e=0;e<from->size;e++)
    {
      if(!SHORT_ITEM(from)[e].string)
	error("Replace: from array not string *\n");
      v[e].ind=SHORT_ITEM(from)[e].string;
    }
  }
  else
  {
    error("Replace: from array is not string *\n");
  }

  if(to->array_type == T_MIXED)
  {
    for(e=0;e<to->size;e++)
    {
      if(ITEM(to)[e].type != T_STRING)
	error("Replace: to array not string *\n");
      v[e].val=ITEM(to)[e].u.string;
    }
  }
  else if(to->array_type == T_STRING)
  {
    for(e=0;e<to->size;e++)
    {
      if(!SHORT_ITEM(to)[e].string)
	error("Replace: to array not string *\n");
      v[e].val=SHORT_ITEM(to)[e].string;
    }
  }
  else
  {
    error("Replace: to array is not string *\n");
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
      if(a<from->size && !MEMCMP(v[a].ind->str,s,v[a].ind->len))
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
    array_replace(sp[-args].u.mapping->val,sp+1-args,sp+2-args);
    pop_n_elems(args-1);
    break;
  }

  case T_STRING:
  {
    struct lpc_string *s;
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
  if(args<2)
    error("Too few arguments to mkmapping.\n");
  if(sp[-args].type!=T_ARRAY)
    error("Bad argument 1 to mkmapping.\n");
  if(sp[1-args].type!=T_ARRAY)
    error("Bad argument 2 to mkmapping.\n");

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
     (sp[-args].subtype != -1 && !sp[-args].u.object->prog))
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
  struct timeval timeout;
  INT32 a,b;

  if(!args) error("Too few arguments to sleep.\n");
  if(sp[-args].type!=T_INT)
    error("Bad argument 1 to sleep.\n");
  a=get_current_time()+sp[-args].u.integer;
  
  pop_n_elems(args);
  while(1)
  {
    timeout.tv_usec=0;
    b=a-get_current_time();
    if(b<0) break;

    timeout.tv_sec=b;
    select(0,0,0,0,&timeout);
    check_signals();
  }
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
TYPEP(f_listp, "listp", T_LIST)
TYPEP(f_stringp, "stringp", T_STRING)
TYPEP(f_floatp, "floatp", T_FLOAT)

void init_builtin_efuns()
{
  add_efun("add_efun",f_add_efun,"function(string,void|mixed:void)",OPT_SIDE_EFFECT);
  add_efun("aggregate",f_aggregate,"function(mixed ...:mixed *)",OPT_TRY_OPTIMIZE);
  add_efun("aggregate_list",f_aggregate_list,"function(mixed ...:list)",OPT_TRY_OPTIMIZE);
  add_efun("aggregate_mapping",f_aggregate_mapping,"function(mixed ...:mapping)",OPT_TRY_OPTIMIZE);
  add_efun("all_efuns",f_all_efuns,"function(:mapping(string:mixed))",OPT_EXTERNAL_DEPEND);
  add_efun("allocate", f_allocate, "function(int, string|void:mixed *)", 0);
  add_efun("arrayp",  f_arrayp,  "function(mixed:int)",0);
  add_efun("backtrace",f_backtrace,"function(:array(array(function|int)))",OPT_EXTERNAL_DEPEND);
  add_efun("call_function",f_call_function,"function(mixed,mixed ...:mixed)",OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);
  add_efun("call_out",f_call_out,"function(function,int,mixed...:void)",OPT_SIDE_EFFECT);
  add_efun("call_out_info",f_call_out_info,"function(:array*)",OPT_EXTERNAL_DEPEND);
  add_efun("clone",f_clone,"function(program,mixed...:object)",OPT_EXTERNAL_DEPEND);
  add_efun("combine_path",f_combine_path,"function(string,string:string)",0);
  add_efun("compile_file",f_compile_file,"function(string:program)",OPT_EXTERNAL_DEPEND);
  add_efun("compile_string",f_compile_string,"function(string,string|void:program)",OPT_EXTERNAL_DEPEND);
  add_efun("copy_value",f_copy_value,"function(mixed:mixed)",0);
  add_efun("crypt",f_crypt,"function(string:string)|function(string,string:int)",OPT_EXTERNAL_DEPEND);
  add_efun("ctime",f_ctime,"function(int:string)",OPT_TRY_OPTIMIZE);
  add_efun("destruct",f_destruct,"function(object|void:void)",OPT_SIDE_EFFECT);
  add_efun("equal",f_equal,"function(mixed,mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("exit",f_exit,"function(int:void)",OPT_SIDE_EFFECT);
  add_efun("explode",f_explode,"function(string,string:array(string))",OPT_TRY_OPTIMIZE);
  add_efun("find_call_out",f_find_call_out,"function(function:int)",OPT_EXTERNAL_DEPEND);
  add_efun("floatp",  f_floatp,  "function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("function_name",f_function_name,"function(function:string)",OPT_TRY_OPTIMIZE);
  add_efun("function_object",f_function_object,"function(function:object)",OPT_TRY_OPTIMIZE);
  add_efun("functionp",  f_functionp,  "function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("hash",f_hash,"function(string,int|void:int)",OPT_TRY_OPTIMIZE);
  add_efun("implode",f_implode,"function(array,string|void:string)",OPT_TRY_OPTIMIZE);
  add_efun("indices",f_indices,"function(string|array:int*)|function(mapping|list:mixed*)",0);
  add_efun("intp",   f_intp,    "function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("listp",   f_listp,   "function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("lower_case",f_lower_case,"function(string:string)",OPT_TRY_OPTIMIZE);
  add_efun("mappingp",f_mappingp,"function(mixed:int)",OPT_TRY_OPTIMIZE);
  add_efun("m_delete",f_m_delete,"function(mapping,mixed:mapping)",0);
  add_efun("mkmapping",f_mkmapping,"function(mixed *,mixed *:mapping)",OPT_TRY_OPTIMIZE);
  add_efun("next_object",f_next_object,"function(void|object:object)",OPT_EXTERNAL_DEPEND);
  add_efun("object_program",f_object_program,"function(object:program)",0);
  add_efun("objectp", f_objectp, "function(mixed:int)",0);
  add_efun("programp",f_programp,"function(mixed:int)",0);
  add_efun("query_host_name",f_query_host_name,"function(:string)",0);
  add_efun("query_num_arg",f_query_num_arg,"function(:int)",OPT_EXTERNAL_DEPEND);
  add_efun("random",f_random,"function(int:int)",OPT_EXTERNAL_DEPEND);
  add_efun("random_seed",f_random_seed,"function(int:void)",OPT_SIDE_EFFECT);
  add_efun("remove_call_out",f_remove_call_out,"function(function:int)",OPT_SIDE_EFFECT);
  add_efun("replace",f_replace,"function(string,string,string:string)|function(string,string*,string*:string)|function(array,mixed,mixed:array)|function(mapping,mixed,mixed:array)",0);
  add_efun("reverse",f_reverse,"function(int:int)|function(string:string)|function(array:array)",0);
  add_efun("rusage", f_rusage, "function(:int *)",OPT_EXTERNAL_DEPEND);
  add_efun("search",f_search,"function(string,string,void|int:int)|function(array,mixed,void|int:int)|function(mapping,mixed:mixed)",0);
  add_efun("sizeof", f_sizeof, "function(string|list|array|mapping:int)",0);
  add_efun("sleep", f_sleep, "function(int:void)",OPT_SIDE_EFFECT);
  add_efun("stringp", f_stringp, "function(mixed:int)",0);
  add_efun("sum",f_sum,"function(int ...:int)|function(float ...:float)|function(string,string|int|float ...:string)|function(string,string|int|float ...:string)|function(int|float,string,string|int|float:string)|function(array ...:array)|function(mapping ...:mapping)|function(list...:list)",0);
  add_efun("this_object", f_this_object, "function(:object)",OPT_EXTERNAL_DEPEND);
  add_efun("throw",f_throw,"function(mixed:void)",0);
  add_efun("time",f_time,"function(void|int:int)",OPT_EXTERNAL_DEPEND);
  add_efun("trace",f_trace,"function(int:int)",OPT_SIDE_EFFECT);
  add_efun("upper_case",f_upper_case,"function(string:string)",0);
  add_efun("values",f_values,"function(string|list:int*)|function(array|mapping:mixed*)",0);
  add_efun("zero_type",f_zero_type,"function(int:int)",0);
}


