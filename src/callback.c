/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "pike_macros.h"
#include "callback.h"
#include "error.h"

/*
 * This file is used to simplify the management of callbacks when certain
 * events occur. The callbacks are managed as linked lists, allocated in
 * chunks.
 */

/* FIXME: free all chunks of memory at exit */

struct callback
{
  struct callback *next;
  callback_func call;
  callback_func free_func;
  void *arg;
};

#define CALLBACK_CHUNK 128

struct callback_block {
  struct callback_block *next;
  struct callback callbacks[CALLBACK_CHUNK];
};

static struct callback_block *callback_chunks=0;
static struct callback *first_callback =0;
static struct callback *free_callbacks =0;

#ifdef DEBUG
extern int d_flag;

static int is_in_free_list(struct callback * c)
{
  struct callback *foo;
  int e;
  for(foo=free_callbacks;foo;foo=foo->next)
    if(foo==c)
      return 1;

  return 0;
}

static void check_callback_chain(struct callback_list *lst)
{
  int e,len=0;
  struct callback_block *tmp;
  struct callback *foo;
  if(d_flag>1)
  {
    for(foo=lst->callbacks;foo;foo=foo->next)
    {
      if((len & 1024)==1023)
      {
	int len2=0;
	struct callback *tmp;
	for(tmp=foo->next;tmp && len2<=len;tmp=tmp->next)
	{
	  if(tmp==foo)
	    fatal("Callback list is cyclic!!!\n");
	}
      }
      len++;
    }
    
    for(tmp=callback_chunks;tmp;tmp=tmp->next)
    {
      for(e=0;e<CALLBACK_CHUNK;e++)
      {
	int d;
	struct callback_block *tmp2;
	
	if(tmp->callbacks[e].free_func == (callback_func)remove_callback)
	{
	  if(!is_in_free_list(tmp->callbacks+e))
	    fatal("Lost track of a struct callback!\n");

	  if(tmp->callbacks[e].next &&
	     !is_in_free_list(tmp->callbacks[e].next))
	    fatal("Free callback has next in Z'ha'dum!\n");

	}else{
	  if(is_in_free_list(tmp->callbacks[e].next))
	    fatal("Non-free callback has next in free list!\n");
	}
	
	if(tmp->callbacks[e].next)
	{
	  d=CALLBACK_CHUNK;
	  for(tmp2=callback_chunks;tmp2;tmp2=tmp2->next)
	  {
	    for(d=0;d<CALLBACK_CHUNK;d++)
	    {
	      if(tmp2->callbacks+d == tmp->callbacks[e].next)
		break;
	      
	      if(d < CALLBACK_CHUNK) break;
	    }
	  }
	  
	  if(d == CALLBACK_CHUNK)
	    fatal("Callback next pointer pointing to Z'ha'dum\n");
	}
      }
    }
  }
}
#else
#define check_callback_chain(X)
#endif

/* Return the first free callback struct, allocate more if needed */
static struct callback *get_free_callback(void)
{
  struct callback *tmp;
  if(!free_callbacks)
  {
    int e;
    struct callback_block *tmp2;
    tmp2=ALLOC_STRUCT(callback_block);
    tmp2->next=callback_chunks;
    callback_chunks=tmp2;

    for(e=0;e<CALLBACK_CHUNK;e++)
    {
      tmp2->callbacks[e].next=free_callbacks;
      tmp2->callbacks[e].free_func=(callback_func)remove_callback;
      free_callbacks=tmp2->callbacks+e;
    }
  }
  tmp=free_callbacks;
  free_callbacks=tmp->next;
  return tmp;
}

/* Traverse a linked list of callbacks and call all the active callbacks
 * in the list. Deactivated callbacks are freed and placed in the free list.
 */
void call_callback(struct callback_list *lst, void *arg)
{
  int this_call;
  struct callback *l,**ptr;

  lst->num_calls++;
  this_call=lst->num_calls;

  check_callback_chain(lst);
  ptr=&lst->callbacks;
  while((l=*ptr))
  {
    if(l->call)
    {
      l->call(l,l->arg, arg);
      if(lst->num_calls != this_call) return;
    }
    check_callback_chain(lst);

    if(!l->call)
    {
      if(l->free_func)
	l->free_func(l, l->arg, 0);

      while(*ptr != l)
      {
	ptr=&(ptr[0]->next);
	if(!*ptr)
	{
	  /* We totally failed to find where we are in the linked list.. */
	  fatal("Callback linked list breakdown.\n");
	}
      }

      *ptr=l->next;
      l->next=free_callbacks;
      free_callbacks=l;
#ifdef DEBUG
      l->free_func=(callback_func)remove_callback;
#endif
    }else{
      ptr=& l->next;
    }
    check_callback_chain(lst);
  }
}

/* Add a callback to the linked list pointed to by ptr. */
struct callback *add_to_callback(struct callback_list *lst,
				 callback_func call,
				 void *arg,
				 callback_func free_func)
{
  struct callback *l;
  l=get_free_callback();
  l->call=call;
  l->arg=arg;
  l->free_func=free_func;

  l->next=lst->callbacks;
  lst->callbacks=l;

  check_callback_chain(lst);

  return l;
}

/* This function deactivates a callback.
 * It is not actually freed until next time this callback is "called"
 */
void *remove_callback(struct callback *l)
{
  l->call=0;
  l->free_func=0;
  return l->arg;
}

/* Free all the callbacks in a linked list of callbacks */
void free_callback(struct callback_list *lst)
{
  struct callback *l,**ptr;
  check_callback_chain(lst);
  ptr=& lst->callbacks;
  while((l=*ptr))
  {
    if(l->free_func)
      l->free_func(l, l->arg, 0);
    *ptr=l->next;
    l->next=free_callbacks;
    free_callbacks=l;
#ifdef DEBUG
    l->free_func=(callback_func)remove_callback;
#endif
  }
}

void cleanup_callbacks(void)
{
  while(callback_chunks)
  {
    struct callback_block *tmp=callback_chunks;
    callback_chunks=tmp->next;
    free((char *)tmp);
  }
  free_callbacks=0;
}


void count_memory_in_callbacks(INT32 *num_, INT32 *size_)
{
  INT32 num=0, size=0;
  struct callback_block *tmp;
  struct callback *tmp2;
  for(tmp=callback_chunks;tmp;tmp=tmp->next)
  {
    num+=CALLBACK_CHUNK;
    size+=sizeof(struct callback_block);
  }
  for(tmp2=free_callbacks;tmp2;tmp2=tmp2->next) num--;
  *num_=num;
  *size_=size;
}
