/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "array.h"
#include "dynamic_buffer.h"
#include "object.h"
#include "interpret.h"
#include "error.h"
#include "builtin_efuns.h"
#include "memory.h"
#include "main.h"
#include "backend.h"
#include "time_stuff.h"
#include "add_efun.h"

#include "callback.h"

#include <math.h>

struct call_out_s
{
  struct timeval tv;
  struct object *caller;
  struct array *args;
};

typedef struct call_out_s call_out;

call_out **pending_calls=0;      /* pointer to first busy pointer */
int num_pending_calls;           /* no of busy pointers in buffer */
static call_out **call_buffer=0; /* pointer to buffer */
static int call_buffer_size;     /* no of pointers in buffer */

static void verify_call_outs()
{
#ifdef DEBUG
  struct array *v;
  int e;

  if(!d_flag) return;
  if(!call_buffer) return;

  if(num_pending_calls<0 || num_pending_calls>call_buffer_size)
    fatal("Error in call out tables.\n");

  if(pending_calls+num_pending_calls!=call_buffer+call_buffer_size)
    fatal("Error in call out tables.\n");

  for(e=0;e<num_pending_calls;e++)
  {
    if(e)
    {
      if(my_timercmp(&pending_calls[e-1]->tv,>,&pending_calls[e]->tv))
	fatal("Error in call out order.\n");
    }
    
    if(!(v=pending_calls[e]->args))
      fatal("No arguments to call\n");

    if(v->refs!=1)
      fatal("Array should exactly have one reference.\n");

    if(v->malloced_size<v->size)
      fatal("Impossible array.\n");
  }
#endif
}


/* start a new call out, return 1 for success */
static int new_call_out(int num_arg,struct svalue *argp)
{
  int e,c;
  call_out *new,**p,**pos;

  if(!call_buffer)
  {
    call_buffer_size=20;
    call_buffer=(call_out **)xalloc(sizeof(call_out *)*call_buffer_size);
    if(!call_buffer) return 0;
    pending_calls=call_buffer+call_buffer_size;
    num_pending_calls=0;
  }

  if(num_pending_calls==call_buffer_size)
  {
    /* here we need to allocate space for more pointers */
    call_out **new_buffer;

    new_buffer=(call_out **)xalloc(sizeof(call_out *)*call_buffer_size*2);
    if(!new_buffer)
      return 0;

    MEMCPY((char *)(new_buffer+call_buffer_size),
	   (char *)call_buffer,
	   sizeof(call_out *)*call_buffer_size);
    free((char *)call_buffer);
    call_buffer=new_buffer;
    pending_calls=call_buffer+call_buffer_size;
    call_buffer_size*=2;
  }

  /* time to allocate a new call_out struct */
  f_aggregate(num_arg-1);

  new=(call_out *)xalloc(sizeof(call_out));

  if(!new) return 0;
  
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

  /* time to link it into the buffer using binsearch */
  pos=pending_calls;

  e=num_pending_calls;
  while(e>0)
  {
    c=e/2;
    if(my_timercmp(& new->tv,>,& pos[c]->tv))
    {
      pos+=c+1;
      e-=c+1;
    }else{
      e=c;
    }
  }
  pos--;
  pending_calls--;
  for(p=pending_calls;p<pos;p++) p[0]=p[1];
  *pos=new;
  num_pending_calls++;

  verify_call_outs();
  return 1;
}

static struct callback *call_out_backend_callback=0;
void do_call_outs(struct callback *ignored, void *ignored_too, void *arg);

void f_call_out(INT32 args)
{
  struct svalue tmp;
  if(args<2)
    error("Too few arguments to call_out.\n");

  if(sp[1-args].type != T_INT && sp[1-args].type!=T_FLOAT)
    error("Bad argument 2 to call_out.\n");

  /* Swap, for compatibility */
  tmp=sp[-args];
  sp[-args]=sp[1-args];
  sp[1-args]=tmp;

  new_call_out(args,sp-args);

  /* We do not add this callback until we actually have
   * call outs to take care of.
   */
  if(!call_out_backend_callback)
    call_out_backend_callback=add_backend_callback(do_call_outs,0,0);
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
	  my_timercmp(&pending_calls[0]->tv,<=,&current_time))
    {
      /* unlink call out */
      c=pending_calls[0];
      pending_calls++;
      num_pending_calls--;

      if(c->caller) free_object(c->caller);

      args=c->args->size;
      push_array_items(c->args);
      free((char *)c);
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
      if(my_timercmp(& pending_calls[0]->tv, < , &next_timeout))
      {
	next_timeout = pending_calls[0]->tv;
      }
    }else{
      if(call_out_backend_callback)
      {
	/* There are no call outs queued, let's remove
	 * the taxing backend callback...
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
  for(e=0;e<num_pending_calls;e++)
  {
    if(is_eq(fun, ITEM(pending_calls[e]->args)))
      return e;
  }
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
    push_int(pending_calls[e]->tv.tv_sec - current_time.tv_sec);
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
    push_int(pending_calls[e]->tv.tv_sec - current_time.tv_sec);
    free_array(pending_calls[e]->args);
    if(pending_calls[e]->caller)
      free_object(pending_calls[e]->caller);
    free((char*)(pending_calls[e]));
    for(;e>0;e--)
      pending_calls[e]=pending_calls[e-1];
    pending_calls++;
    num_pending_calls--;
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
    v=allocate_array_no_init(pending_calls[e]->args->size+2, 0);
    ITEM(v)[0].type=T_INT;
    ITEM(v)[0].subtype=NUMBER_NUMBER;
    ITEM(v)[0].u.integer=pending_calls[e]->tv.tv_sec - current_time.tv_sec;

    if(pending_calls[e]->caller)
    {
      ITEM(v)[1].type=T_OBJECT;
      (ITEM(v)[1].u.object=pending_calls[e]->caller) ->refs++;
    }else{
      ITEM(v)[1].type=T_INT;
      ITEM(v)[1].subtype=NUMBER_NUMBER;
      ITEM(v)[1].u.integer=0;
    }

    assign_svalues_no_free(ITEM(v)+2,ITEM(pending_calls[e]->args),pending_calls[e]->args->size,BIT_MIXED);

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
    free_array(pending_calls[e]->args);
    if(pending_calls[e]->caller) free_object(pending_calls[e]->caller);
    free((char*)(pending_calls[e]));
  }
  if(call_buffer) free((char*)call_buffer);
  num_pending_calls=0;
  call_buffer=NULL;
  pending_calls=NULL;
}

#ifdef DEBUG
void verify_all_call_outs()
{
  verify_call_outs();
}
#endif

void init_call_out_efuns(void)
{
  add_efun("call_out",f_call_out,"function(function,float|int,mixed...:void)",OPT_SIDE_EFFECT);
  add_efun("call_out_info",f_call_out_info,"function(:array*)",OPT_EXTERNAL_DEPEND);
  add_efun("find_call_out",f_find_call_out,"function(function:int)",OPT_EXTERNAL_DEPEND);
  add_efun("remove_call_out",f_remove_call_out,"function(function:int)",OPT_SIDE_EFFECT);
}

void init_call_out_programs(void) {}

void exit_call_out(void)
{
  free_all_call_outs();
}
