#include <stdio.h>
#include "global.h"
#include "array.h"
#include "call_out.h"
#include "dynamic_buffer.h"
#include "object.h"
#include "interpret.h"
#include "error.h"
#include "builtin_efuns.h"

call_out **pending_calls=0;      /* pointer to first busy pointer */
int num_pending_calls;           /* no of busy pointers in buffer */
static call_out **call_buffer=0; /* pointer to buffer */
static int call_buffer_size;     /* no of pointers in buffer */

extern time_t current_time;

static void verify_call_outs()
{
#ifdef DEBUG
  struct array *v;
  int e;
  if(call_buffer) return;

  if(num_pending_calls<0 || num_pending_calls>call_buffer_size)
    fatal("Error in call out tables.\n");

  if(pending_calls+num_pending_calls!=call_buffer+call_buffer_size)
    fatal("Error in call out tables.\n");

  for(e=0;e<num_pending_calls;e++)
  {
    if(e)
    {
      if(pending_calls[e-1]>pending_calls[e])
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

  new->time=current_time+argp[0].u.integer;
  if(new->time <= current_time) new->time=current_time+1;

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
  if(new->time>current_time+1) /* do we need to search where ?*/
  {
    e=num_pending_calls;
    while(e>0)
    {
      c=e/2;
      if(new->time>pos[c]->time)
      {
	pos+=c+1;
	e-=c+1;
      }else{
	e=c;
      }
    }
  }
  pos--;
  pending_calls--;
  for(p=pending_calls;p<pos;p++) p[0]=p[1];
  *pos=new;
  num_pending_calls++;

  return 1;
}

void f_call_out(INT32 args)
{
  struct svalue tmp;
  if(args<2)
    error("Too few arguments to call_out.\n");

  /* Swap, for compatibility */
  tmp=sp[-args];
  sp[-args]=sp[1-args];
  sp[1-args]=tmp;

  new_call_out(args,sp-args);
}

void do_call_outs()
{
  call_out *c;
  int args;
  verify_call_outs();
  while(num_pending_calls &&
	pending_calls[0]->time<=current_time &&
	current_time==get_current_time())
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
    push_int(pending_calls[e]->time-current_time);
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
    push_int(pending_calls[e]->time-current_time);
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
  ret=allocate_array_no_init(num_pending_calls,0,T_ARRAY);
  for(e=0;e<num_pending_calls;e++)
  {
    struct array *v;
    v=allocate_array_no_init(pending_calls[e]->args->size+2, 0, T_MIXED);
    ITEM(v)[0].type=T_INT;
    ITEM(v)[0].subtype=NUMBER_NUMBER;
    ITEM(v)[0].u.integer=pending_calls[e]->time-current_time;

    if(pending_calls[e]->caller)
    {
      ITEM(v)[1].type=T_OBJECT;
      (ITEM(v)[1].u.object=pending_calls[e]->caller) ->refs++;
    }else{
      ITEM(v)[1].type=T_INT;
      ITEM(v)[1].subtype=NUMBER_NUMBER;
      ITEM(v)[1].u.integer=0;
    }

    assign_svalues_no_free(ITEM(v)+2,ITEM(pending_calls[e]->args),pending_calls[e]->args->size);

    SHORT_ITEM(ret)[e].array=v;
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

time_t get_next_call_out()
{
  if(num_pending_calls)
  {
    return pending_calls[0]->time;
  }else{
    return 0;
  }
}

#ifdef DEBUG
void verify_all_call_outs()
{
  int e;
  verify_call_outs();
  for(e=0;e<num_pending_calls;e++)
  {
    checked((void *)pending_calls[e]->caller,1);
    checked((void *)pending_calls[e]->args,1);
  }
}
#endif
