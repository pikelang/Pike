/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: call_out.c,v 1.25 1999/02/10 21:53:21 hubbe Exp $");
#include "array.h"
#include "dynamic_buffer.h"
#include "object.h"
#include "interpret.h"
#include "error.h"
#include "builtin_functions.h"
#include "pike_memory.h"
#include "main.h"
#include "backend.h"
#include "time_stuff.h"
#include "constants.h"
#include "stralloc.h"
#include "builtin_functions.h"
#include "gc.h"
#include "threads.h"
#include "stuff.h"

#include "callback.h"

#include <math.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

struct call_out_s
{
  INT32 pos;
  struct timeval tv;
  struct call_out_s *next; /* For block alloc */
  struct call_out_s *next_fun;
  struct call_out_s **prev_fun;
  struct call_out_s *next_arr;
  struct call_out_s **prev_arr;
  struct object *caller;
  struct array *args;
};

#include "block_alloc.h"

#ifdef PIKE_DEBUG
#define MESS_UP_BLOCK(X) \
 (X)->next_arr=(struct call_out_s *)-1; \
 (X)->next_fun=(struct call_out_s *)-1; \
 (X)->prev_arr=(struct call_out_s **)-1; \
 (X)->prev_fun=(struct call_out_s **)-1; \
 (X)->caller=(struct object *)-1; \
 (X)->args=(struct array *)-1; \
 (X)->pos=-1;
#else
#define MESS_UP_BLOCK(X)
#endif

#undef EXIT_BLOCK
#define EXIT_BLOCK(X) \
  *(X->prev_arr)=X->next_arr; \
  if(X->next_arr) X->next_arr->prev_arr=X->prev_arr; \
  *(X->prev_fun)=X->next_fun; \
  if(X->next_fun) X->next_fun->prev_fun=X->prev_fun; \
  MESS_UP_BLOCK(X) ;
BLOCK_ALLOC(call_out_s, 1022 )

typedef struct call_out_s call_out;

struct hash_ent
{
  call_out *arr;
  call_out *fun;
};


int num_pending_calls;           /* no of busy pointers in buffer */
static call_out **call_buffer=0;  /* pointer to buffer */
static int call_buffer_size;     /* no of pointers in buffer */

static unsigned int hash_size=0;
static unsigned int hash_order=7;
static struct hash_ent *call_hash=0;

#undef CAR
#undef CDR

#define CAR(X) (((X)<<1)+1)
#define CDR(X) (((X)<<1)+2)
#define PARENT(X) (((X)-1)>>1)
#define CALL(X) (call_buffer[(X)])
#define MOVECALL(X,Y) do { INT32 p_=(X); (CALL(p_)=CALL(Y))->pos=p_; }while(0)
#define CMP(X,Y) my_timercmp(& CALL(X)->tv, <, & CALL(Y)->tv)
#define SWAP(X,Y) do{ call_out *_tmp=CALL(X); (CALL(X)=CALL(Y))->pos=(X); (CALL(Y)=_tmp)->pos=(Y); } while(0)

#ifdef PIKE_DEBUG
static int inside_call_out=0;
#define PROTECT_CALL_OUTS() \
   if(inside_call_out) fatal("Recursive call in call_out module.\n"); \
   inside_call_out=1

#define UNPROTECT_CALL_OUTS() \
   inside_call_out=0
#else
#define PROTECT_CALL_OUTS()
#define UNPROTECT_CALL_OUTS()
#endif

#ifdef PIKE_DEBUG
static void verify_call_outs(void)
{
  struct array *v;
  int e,d;

  if(!d_flag) return;
  if(!call_buffer) return;

  if(num_pending_calls<0 || num_pending_calls>call_buffer_size)
    fatal("Error in call out tables.\n");

  if(d_flag<2) return;

  for(e=0;e<num_pending_calls;e++)
  {
    if(e)
    {
      if(CMP(e, PARENT(e)))
	fatal("Error in call out heap. (@ %d)\n",e);
    }

    if(!(v=CALL(e)->args))
      fatal("No arguments to call\n");

    if(v->refs < 1)
      fatal("Array should have at least one reference.\n");

    if(v->malloced_size<v->size)
      fatal("Impossible array.\n");

    if(!v->size)
      fatal("Call out array of zero size!\n");

    if(CALL(e)->prev_arr[0] != CALL(e))
      fatal("call_out[%d]->prev_arr[0] is wrong!\n",e);

    if(CALL(e)->prev_fun[0] != CALL(e))
      fatal("call_out[%d]->prev_fun[0] is wrong!\n",e);

    if(CALL(e)->pos != e)
      fatal("Call_out->pos is not correct!\n");

    if(d_flag>4)
    {
      for(d=e+1;d<num_pending_calls;d++)
	if(CALL(e)->args == CALL(d)->args)
	  fatal("Duplicate call out in heap.\n");
    }
  }

  for(d=0;d<10 && e<call_buffer_size;d++,e++)
    CALL(e)=(call_out *)-1;

  for(e=0;e<(int)hash_size;e++)
  {
    call_out *c,**prev;
    for(prev=& call_hash[e].arr;(c=*prev);prev=& c->next_arr)
    {
      if(c->prev_arr != prev)
	fatal("c->prev_arr is wrong %p.\n",c);

      if(c->pos<0)
	fatal("Free call_out in call_out hash table %p.\n",c);
    }

    for(prev=& call_hash[e].fun;(c=*prev);prev=& c->next_fun)
    {
      if(c->prev_fun != prev)
	fatal("c->prev_fun is wrong %p.\n",c);

      if(c->pos<0)
	fatal("Free call_out in call_out hash table %p.\n",c);
    }
  }
    
}

static struct callback *verify_call_out_callback=0;
void do_verify_call_outs(struct callback *foo, void *x, void *y)
{
  PROTECT_CALL_OUTS();
  verify_call_outs();
  UNPROTECT_CALL_OUTS();
}
#else
#define verify_call_outs()
#endif


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
#ifdef PIKE_DEBUG
  if(pos <0 || pos>=num_pending_calls)
    fatal("Bad argument to adjust_up(%d)\n",pos);
#endif
  if(!pos) return 0;

  if(CMP(pos, parent))
  {
    SWAP(pos, parent);
    from=pos;
    pos=parent;
    while(pos && CMP(pos, PARENT(pos)))
    {
      parent=PARENT(pos);
      SWAP(pos, parent);
      from=pos;
      pos=parent;
    }
    from+=from&1 ? 1 : -1;
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
  struct array *args;
  unsigned long hval;

  PROTECT_CALL_OUTS();
  if(num_pending_calls==call_buffer_size)
  {
    /* here we need to allocate space for more pointers */
    call_out **new_buffer;

    if(!call_buffer)
    {
      call_buffer_size=128;
      call_buffer=(call_out **)xalloc(sizeof(call_out *)*call_buffer_size);
      if(!call_buffer) return 0;
      num_pending_calls=0;

      hash_size=hashprimes[hash_order];
      call_hash=(struct hash_ent *)xalloc(sizeof(struct hash_ent)*hash_size);
      MEMSET(call_hash, 0, sizeof(struct hash_ent)*hash_size);
    }else{
      struct hash_ent *new_hash;
      int e;

      new_buffer=(call_out **)realloc((char *)call_buffer, sizeof(call_out *)*call_buffer_size*2);
      if(!new_buffer)
	error("Not enough memory for another call_out\n");
      call_buffer_size*=2;
      call_buffer=new_buffer;

      if((new_hash=(struct hash_ent *)malloc(sizeof(struct hash_ent)*
					      hashprimes[hash_order+1])))
      {
	free((char *)call_hash);
	call_hash=new_hash;
	hash_size=hashprimes[++hash_order];
	MEMSET(call_hash, 0, sizeof(struct hash_ent)*hash_size);

	/* Re-hash */
	for(e=0;e<num_pending_calls;e++)
	{
	  call_out *c=CALL(e);
	  
	  hval=(unsigned long)c->args;
	  hval%=hash_size;
	  
	  if((c->next_arr=call_hash[hval].arr))
	    c->next_arr->prev_arr=&c->next_arr;
	  c->prev_arr=&call_hash[hval].arr;
	  call_hash[hval].arr=c;
	  
	  hval=hash_svalue(c->args->item);
	  hval%=hash_size;
	  
	  if((c->next_fun=call_hash[hval].fun))
	    c->next_fun->prev_fun=&c->next_fun;
	  c->prev_fun=&call_hash[hval].fun;
	  call_hash[hval].fun=c;
	}
      }
    }
  }

  /* time to allocate a new call_out struct */
  args=aggregate_array(num_arg-1);

  CALL(num_pending_calls)=new=alloc_call_out_s();
  new->pos=num_pending_calls;

  {
    hval=(unsigned long)args;
    hval%=hash_size;

    if((new->next_arr=call_hash[hval].arr))
      new->next_arr->prev_arr=&new->next_arr;
    new->prev_arr=&call_hash[hval].arr;
    call_hash[hval].arr=new;

    hval=hash_svalue(args->item);
    hval%=hash_size;
    if((new->next_fun=call_hash[hval].fun))
      new->next_fun->prev_fun=&new->next_fun;
    new->prev_fun=&call_hash[hval].fun;
    call_hash[hval].fun=new;
  }

  switch(argp[0].type)
  {
    case T_INT:
      new->tv.tv_sec=argp[0].u.integer;
      new->tv.tv_usec=0;
      break;

    case T_FLOAT:
    {
      FLOAT_TYPE tmp=argp[0].u.float_number;
      new->tv.tv_sec=floor(tmp);
      new->tv.tv_usec=(long)(1000000.0 * (tmp - floor(tmp)));
      break;
    }

    default:
    fatal("Bad timeout to new_call_out!\n");
  }

#ifdef _REENTRANT
  if(num_threads>1)
  {
    struct timeval tmp;
    GETTIMEOFDAY(&tmp);
    my_add_timeval(& new->tv, &tmp);
  }else
#endif
    my_add_timeval(& new->tv, &current_time);

  if(fp && fp->current_object)
  {
    add_ref(new->caller=fp->current_object);
  }else{
    new->caller=0;
  }

  new->args=args;
  sp--;

  

  num_pending_calls++;
  adjust_up(num_pending_calls-1);
  verify_call_outs();

#ifdef _REENTRANT
  wake_up_backend();
#endif

  UNPROTECT_CALL_OUTS();
  return args;
}

static void count_memory_in_call_outs(struct callback *foo,
				      void *bar,
				      void *gazonk)
{
  push_text("num_call_outs");
  push_int(num_pending_calls);
  push_text("call_out_bytes");

  /* FIXME */
  push_int(call_buffer_size * sizeof(call_out **)+
	   num_pending_calls * sizeof(call_out));
}

static struct callback *call_out_backend_callback=0;
static struct callback *mem_callback=0;
void do_call_outs(struct callback *ignored, void *ignored_too, void *arg);

#ifdef PIKE_DEBUG
static void mark_call_outs(struct callback *foo, void *bar, void *gazonk)
{
  int e;
  for(e=0;e<num_pending_calls;e++)
  {
    gc_external_mark(CALL(e)->args);
    if(CALL(e)->caller)
      gc_external_mark(CALL(e)->caller);
  }
}
#endif

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
  ref_push_array(v);

  /* We do not add this callback until we actually have
   * call outs to take care of.
   */
  if(!call_out_backend_callback)
    call_out_backend_callback=add_backend_callback(do_call_outs,0,0);

  if(!mem_callback)
    mem_callback=add_memory_usage_callback(count_memory_in_call_outs,0,0);

#ifdef PIKE_DEBUG
  if(!verify_call_out_callback)
  {
    verify_call_out_callback=add_to_callback(& do_debug_callbacks,
					     do_verify_call_outs, 0, 0);
    add_gc_callback(mark_call_outs, 0, 0);
  }
#endif

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
	  my_timercmp(&CALL(0)->tv,<=,&current_time))
    {
      /* unlink call out */
      call_out c,*cc;

      PROTECT_CALL_OUTS();
      cc=CALL(0);
      if(--num_pending_calls)
      {
	MOVECALL(0,num_pending_calls);
	adjust_down(0);
      }
      UNPROTECT_CALL_OUTS();
      c=*cc;
      really_free_call_out_s(cc);

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
      if(my_timercmp(& CALL(0)->tv, < , &next_timeout))
      {
	next_timeout = CALL(0)->tv;
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
#if 0
  int e;

  if(fun->type == T_ARRAY)
    for(e=0;e<num_pending_calls;e++)
      if(CALL(e)->args == fun->u.array)
	return e;

  for(e=0;e<num_pending_calls;e++)
    if(is_eq(fun, ITEM(CALL(e)->args)))
      return e;

#else
  unsigned long hval;
  call_out *c;

  if(!num_pending_calls) return -1;

  if(fun->type == T_ARRAY)
  {
    hval=(unsigned long)fun->u.array;
    hval%=hash_size;
    for(c=call_hash[hval].arr;c;c=c->next_arr)
    {
      if(c->args == fun->u.array)
      {
#ifdef PIKE_DEBUG
	if(CALL(c->pos) != c)
	  fatal("Call_out->pos not correct!\n");
#endif
	return c->pos;
      }
    }
  }

  hval=hash_svalue(fun);
  hval%=hash_size;
  for(c=call_hash[hval].fun;c;c=c->next_fun)
  {
    if(is_eq(fun, ITEM(c->args)))
    {
#ifdef PIKE_DEBUG
      if(CALL(c->pos) != c)
	fatal("Call_out->pos not correct!\n");
#endif
      return c->pos;
    }
  }
#endif
  return -1;
}


static void f_do_call_outs(INT32 args)
{
  GETTIMEOFDAY(&current_time);
  do_call_outs(0, 0, (void *)1);
  do_call_outs(0, 0, (void *)0);
  pop_n_elems(args);
}

void f_find_call_out(INT32 args)
{
  int e;
  verify_call_outs();

  PROTECT_CALL_OUTS();
  e=find_call_out(sp - args);
  pop_n_elems(args);
  if(e==-1)
  {
    sp->type=T_INT;
    sp->subtype=NUMBER_UNDEFINED;
    sp->u.integer=-1;
    sp++;
  }else{
    push_int(CALL(e)->tv.tv_sec - current_time.tv_sec);
  }
  UNPROTECT_CALL_OUTS();
  verify_call_outs();
}

void f_remove_call_out(INT32 args)
{
  int e;
  PROTECT_CALL_OUTS();
  verify_call_outs();
  e=find_call_out(sp-args);
  verify_call_outs();
  if(e!=-1)
  {
    pop_n_elems(args);
    push_int(CALL(e)->tv.tv_sec - current_time.tv_sec);
    free_array(CALL(e)->args);
    if(CALL(e)->caller)
      free_object(CALL(e)->caller);
    really_free_call_out_s(CALL(e));
    --num_pending_calls;
    if(e!=num_pending_calls)
    {
      MOVECALL(e,num_pending_calls);
      adjust(e);
    }
  }else{
    pop_n_elems(args);
    sp->type=T_INT;
    sp->subtype=NUMBER_UNDEFINED;
    sp->u.integer=-1;
    sp++;
  }
  verify_call_outs();
  UNPROTECT_CALL_OUTS();
}

/* return an array containing info about all call outs:
 * ({  ({ delay, caller, function, args, ... }), ... })
 */
struct array *get_all_call_outs(void)
{
  int e;
  struct array *ret;

  verify_call_outs();
  PROTECT_CALL_OUTS();
  ret=allocate_array_no_init(num_pending_calls,0);
  for(e=0;e<num_pending_calls;e++)
  {
    struct array *v;
    v=allocate_array_no_init(CALL(e)->args->size+2, 0);
    ITEM(v)[0].type=T_INT;
    ITEM(v)[0].subtype=NUMBER_NUMBER;
    ITEM(v)[0].u.integer=CALL(e)->tv.tv_sec - current_time.tv_sec;

    if(CALL(e)->caller)
    {
      ITEM(v)[1].type=T_OBJECT;
      add_ref(ITEM(v)[1].u.object=CALL(e)->caller);
    }else{
      ITEM(v)[1].type=T_INT;
      ITEM(v)[1].subtype=NUMBER_NUMBER;
      ITEM(v)[1].u.integer=0;
    }

    assign_svalues_no_free(ITEM(v)+2,ITEM(CALL(e)->args),CALL(e)->args->size,BIT_MIXED);

    ITEM(ret)[e].type=T_ARRAY;
    ITEM(ret)[e].u.array=v;
  }
  UNPROTECT_CALL_OUTS();
  return ret;
}

void f_call_out_info(INT32 args)
{
  pop_n_elems(args);
  push_array(get_all_call_outs());
}

void free_all_call_outs(void)
{
  int e;
  verify_call_outs();
  for(e=0;e<num_pending_calls;e++)
  {
    free_array(CALL(e)->args);
    if(CALL(e)->caller) free_object(CALL(e)->caller);
    really_free_call_out_s(CALL(e));
  }
  if(call_buffer) free((char*)call_buffer);
  if(call_hash) free((char*)call_hash);
  free_all_call_out_s_blocks();
  num_pending_calls=0;
  call_buffer=NULL;
}

void pike_module_init(void)
{
  
/* function(function,float|int,mixed...:mixed) */
  ADD_EFUN("call_out",f_call_out,tFuncV(tFunction tOr(tFlt,tInt),tMix,tMix),OPT_SIDE_EFFECT);
  
/* function(:array*) */
  ADD_EFUN("call_out_info",f_call_out_info,tFunc(,tArr(tArray)),OPT_EXTERNAL_DEPEND);
  
/* function(void:void) */
  ADD_EFUN("_do_call_outs",f_do_call_outs,tFunc(tVoid,tVoid),
	   OPT_EXTERNAL_DEPEND);
  
/* function(mixed:int) */
  ADD_EFUN("find_call_out",f_find_call_out,tFunc(tMix,tInt),OPT_EXTERNAL_DEPEND);
  
/* function(mixed:int) */
  ADD_EFUN("remove_call_out",f_remove_call_out,tFunc(tMix,tInt),OPT_SIDE_EFFECT);
}

void pike_module_exit(void)
{
  free_all_call_outs();
}
