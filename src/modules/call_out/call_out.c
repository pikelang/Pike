/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: call_out.c,v 1.7 1997/02/11 08:39:34 hubbe Exp $");
#include "array.h"
#include "dynamic_buffer.h"
#include "object.h"
#include "interpret.h"
#include "error.h"
#include "builtin_functions.h"
#include "memory.h"
#include "main.h"
#include "backend.h"
#include "time_stuff.h"
#include "constants.h"
#include "stralloc.h"
#include "builtin_functions.h"

#include "callback.h"

#include <math.h>

struct call_out_s
{
  struct timeval tv;
  struct object *caller;
  struct array *args;
};

typedef struct call_out_s call_out;

int num_pending_calls;           /* no of busy pointers in buffer */
static call_out *call_buffer=0;  /* pointer to buffer */
static int call_buffer_size;     /* no of pointers in buffer */

#undef CAR
#undef CDR

#define CAR(X) (((X)<<1)+1)
#define CDR(X) (((X)<<1)+2)
#define PARENT(X) (((X)-1)>>1)
#define CALL(X) call_buffer[(X)]
#define CMP(X,Y) my_timercmp(& CALL(X).tv, <, & CALL(Y).tv)
#define SWAP(X,Y) do{ call_out _tmp=CALL(X); CALL(X)=CALL(Y); CALL(Y)=_tmp; } while(0)

static void verify_call_outs()
{
#ifdef DEBUG
  struct array *v;
  int e;

  if(!d_flag) return;
  if(!call_buffer) return;

  if(num_pending_calls<0 || num_pending_calls>call_buffer_size)
    fatal("Error in call out tables.\n");

  for(e=0;e<num_pending_calls;e++)
  {
    if(e)
    {
      if(CMP(e, PARENT(e)))
	fatal("Error in call out heap.\n");
    }
    
    if(!(v=CALL(e).args))
      fatal("No arguments to call\n");

    if(v->refs < 1)
      fatal("Array should have at least one reference.\n");

    if(v->malloced_size<v->size)
      fatal("Impossible array.\n");
  }
#endif
}

static void adjust_down(int pos)
{
  while(1)
  {
    int a=CAR(pos), b=CDR(pos);
    if(a >= num_pending_calls) break;
    if(b < num_pending_calls)
      if(CMP(b, a))
	a=b;

    if(CMP(pos, a)) break;
    SWAP(pos, a);
    pos=a;
  }
}

static int adjust_up(int pos)
{
  int parent=PARENT(pos);
  int from;
  if(!pos) return 0;

  if(CMP(pos, parent))
  {
    SWAP(pos, parent);
    from=pos;
    pos=parent;
    while(pos && CMP(pos, parent=PARENT(pos)))
    {
      SWAP(pos, parent);
      from=pos;
      pos=parent;
    }
    from^=1;
    if(from < num_pending_calls && CMP(from, pos))
    {
      SWAP(from, pos);
      adjust_down(from);
    }
    return 1;
  }
  return 0;
}

static void adjust(int pos)
{
  if(!adjust_up(pos)) adjust_down(pos);
}

/* start a new call out, return 1 for success */
static struct array * new_call_out(int num_arg,struct svalue *argp)
{
  int e,c;
  call_out *new,**p,**pos;

  if(num_pending_calls==call_buffer_size)
  {
    /* here we need to allocate space for more pointers */
    call_out *new_buffer;

    if(!call_buffer)
    {
      call_buffer_size=128;
      call_buffer=(call_out *)xalloc(sizeof(call_out)*call_buffer_size);
      if(!call_buffer) return 0;
      num_pending_calls=0;
    }else{
      new_buffer=(call_out *)realloc((char *)call_buffer, sizeof(call_out)*call_buffer_size*2);
      if(!new_buffer)
	error("Not enough memorry for another call_out\n");
      call_buffer_size*=2;
      call_buffer=new_buffer;
    }
  }

  /* time to allocate a new call_out struct */
  f_aggregate(num_arg-1);

  new=&CALL(num_pending_calls);

  if(argp[0].type==T_INT)
  {
    new->tv.tv_sec=argp[0].u.integer;
    new->tv.tv_usec=0;
  }
  else if(argp[0].type == T_FLOAT)
  {
    FLOAT_TYPE tmp=argp[0].u.float_number;
    new->tv.tv_sec=floor(tmp);
    new->tv.tv_usec=(long)(1000000.0 * (tmp - floor(tmp)));
  }
  else
  {
    fatal("Bad timeout to new_call_out!\n");
  }

  my_add_timeval(& new->tv, &current_time);

  if(fp && fp->current_object)
  {
    new->caller=fp->current_object;
    new->caller->refs++;
  }else{
    new->caller=0;
  }

  new->args=sp[-1].u.array;
  sp -= 2;

  num_pending_calls++;
  adjust_up(num_pending_calls-1);
  verify_call_outs();
  return new->args;
}

static void count_memory_in_call_outs(struct callback *foo,
				      void *bar,
				      void *gazonk)
{
  push_text("num_call_outs");
  push_int(num_pending_calls);
  push_text("call_out_bytes");
  push_int(call_buffer_size * sizeof(call_out **)+
	   num_pending_calls * sizeof(call_out));
}

static struct callback *call_out_backend_callback=0;
static struct callback *mem_callback=0;
void do_call_outs(struct callback *ignored, void *ignored_too, void *arg);

void f_call_out(INT32 args)
{
  struct svalue tmp;
  struct array *v;
  if(args<2)
    error("Too few arguments to call_out.\n");

  if(sp[1-args].type != T_INT && sp[1-args].type!=T_FLOAT)
    error("Bad argument 2 to call_out.\n");

  /* Swap, for compatibility */
  tmp=sp[-args];
  sp[-args]=sp[1-args];
  sp[1-args]=tmp;

  v=new_call_out(args,sp-args);
  v->refs++;
  push_array(v);

  /* We do not add this callback until we actually have
   * call outs to take care of.
   */
  if(!call_out_backend_callback)
    call_out_backend_callback=add_backend_callback(do_call_outs,0,0);

  if(!mem_callback)
    mem_callback=add_memory_usage_callback(count_memory_in_call_outs,0,0);
}

void do_call_outs(struct callback *ignored, void *ignored_too, void *arg)
{
  call_out *c;
  int args;
  time_t tmp;
  verify_call_outs();

  if(arg)
  {
    tmp=(time_t)TIME(0);
    while(num_pending_calls &&
	  my_timercmp(&CALL(0).tv,<=,&current_time))
    {
      /* unlink call out */
      call_out c;
      c=CALL(0);
      CALL(0)=CALL(--num_pending_calls);
      adjust_down(0);

      if(c.caller) free_object(c.caller);

      args=c.args->size;
      push_array_items(c.args);
      check_destructed(sp-args);
      if(sp[-args].type!=T_INT)
      {
	f_call_function(args);
	pop_stack();
      }else{
	pop_n_elems(args);
      }
      verify_call_outs();

      if(tmp != (time_t) TIME(0)) break;
    }
  }
  else /* if(arg) */
  {
    if(num_pending_calls)
    {
      if(my_timercmp(& CALL(0).tv, < , &next_timeout))
      {
	next_timeout = CALL(0).tv;
      }
    }else{
      if(call_out_backend_callback)
      {
	/* There are no call outs queued, let's remove
	 * the "taxing" backend callback...
	 */
	remove_callback(call_out_backend_callback);
	call_out_backend_callback=0;
      }
    }
  }
}

static int find_call_out(struct svalue *fun)
{
  int e;

  if(fun->type == T_ARRAY)
    for(e=0;e<num_pending_calls;e++)
      if(CALL(e).args == fun->u.array)
	return e;

  for(e=0;e<num_pending_calls;e++)
    if(is_eq(fun, ITEM(CALL(e).args)))
      return e;

  return -1;
}

void f_find_call_out(INT32 args)
{
  int e;
  verify_call_outs();
  e=find_call_out(sp - args);
  pop_n_elems(args);
  if(e==-1)
  {
    sp->type=T_INT;
    sp->subtype=NUMBER_UNDEFINED;
    sp->u.integer=-1;
    sp++;
  }else{
    push_int(CALL(e).tv.tv_sec - current_time.tv_sec);
  }
  verify_call_outs();
}

void f_remove_call_out(INT32 args)
{
  int e;
  verify_call_outs();
  e=find_call_out(sp-args);
  if(e!=-1)
  {
    pop_n_elems(args);
    push_int(CALL(e).tv.tv_sec - current_time.tv_sec);
    free_array(CALL(e).args);
    if(CALL(e).caller)
      free_object(CALL(e).caller);
    CALL(e)=CALL(--num_pending_calls);
    adjust(e);
  }else{
    pop_n_elems(args);
    sp->type=T_INT;
    sp->subtype=NUMBER_UNDEFINED;
    sp->u.integer=-1;
    sp++;
  }
  verify_call_outs();
}

/* return an array containing info about all call outs:
 * ({  ({ delay, caller, function, args, ... }), ... })
 */
struct array *get_all_call_outs()
{
  int e;
  struct array *ret;

  verify_call_outs();
  ret=allocate_array_no_init(num_pending_calls,0);
  for(e=0;e<num_pending_calls;e++)
  {
    struct array *v;
    v=allocate_array_no_init(CALL(e).args->size+2, 0);
    ITEM(v)[0].type=T_INT;
    ITEM(v)[0].subtype=NUMBER_NUMBER;
    ITEM(v)[0].u.integer=CALL(e).tv.tv_sec - current_time.tv_sec;

    if(CALL(e).caller)
    {
      ITEM(v)[1].type=T_OBJECT;
      ITEM(v)[1].u.object=CALL(e).caller;
      CALL(e).caller->refs++;
    }else{
      ITEM(v)[1].type=T_INT;
      ITEM(v)[1].subtype=NUMBER_NUMBER;
      ITEM(v)[1].u.integer=0;
    }

    assign_svalues_no_free(ITEM(v)+2,ITEM(CALL(e).args),CALL(e).args->size,BIT_MIXED);

    ITEM(ret)[e].type=T_ARRAY;
    ITEM(ret)[e].u.array=v;
  }
  return ret;
}

void f_call_out_info(INT32 args)
{
  pop_n_elems(args);
  push_array(get_all_call_outs());
}

void free_all_call_outs()
{
  int e;
  verify_call_outs();
  for(e=0;e<num_pending_calls;e++)
  {
    free_array(CALL(e).args);
    if(CALL(e).caller) free_object(CALL(e).caller);
  }
  if(call_buffer) free((char*)call_buffer);
  num_pending_calls=0;
  call_buffer=NULL;
}

#ifdef DEBUG
void verify_all_call_outs()
{
  verify_call_outs();
}
#endif

void pike_module_init(void)
{
  add_efun("call_out",f_call_out,"function(function,float|int,mixed...:mixed)",OPT_SIDE_EFFECT);
  add_efun("call_out_info",f_call_out_info,"function(:array*)",OPT_EXTERNAL_DEPEND);
  add_efun("find_call_out",f_find_call_out,"function(mixed:int)",OPT_EXTERNAL_DEPEND);
  add_efun("remove_call_out",f_remove_call_out,"function(mixed:int)",OPT_SIDE_EFFECT);
}

void pike_module_exit(void)
{
  free_all_call_outs();
}
