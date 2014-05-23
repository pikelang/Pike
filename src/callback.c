/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "pike_macros.h"
#include "callback.h"
#include "pike_error.h"
#include "block_allocator.h"

struct callback_list fork_child_callback;

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

static struct block_allocator callback_allocator
    = BA_INIT(sizeof(struct callback), CALLBACK_CHUNK);

void count_memory_in_callbacks(size_t * num, size_t * size) {
    ba_count_all(&callback_allocator, num, size);
}


#ifdef PIKE_DEBUG
extern int d_flag;

static void check_callback_chain(struct callback_list *lst)
{
  int len=0;
  struct callback *foo;
  if(d_flag>4)
  {
    for(foo=lst->callbacks;foo;foo=foo->next)
    {
      if((len & 1024)==1023)
      {
	int len2=0;
	struct callback *tmp;
	for(tmp=foo->next;tmp && len2<=len;tmp=tmp->next)
	{
#ifdef PIKE_DEBUG
	  if(tmp==foo)
	    Pike_fatal("Callback list is cyclic!!!\n");
#endif
	}
      }
      len++;
    }
  }
}
#else
#define check_callback_chain(X)
#endif

/* Return the first free callback struct, allocate more if needed */


/* Traverse a linked list of callbacks and call all the active callbacks
 * in the list. Deactivated callbacks are freed and placed in the free list.
 */
PMOD_EXPORT void low_call_callback(struct callback_list *lst, void *arg)
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

    if(!l->call)
    {
      if(l->free_func)
	l->free_func(l, l->arg, 0);

      while(*ptr != l)
      {
	ptr=&(ptr[0]->next);
#ifdef PIKE_DEBUG
	if(!*ptr)
	{
	  /* We totally failed to find where we are in the linked list.. */
	  Pike_fatal("Callback linked list breakdown.\n");
	}
#endif
      }

      *ptr=l->next;
      ba_free(&callback_allocator, l);
    }else{
      ptr=& l->next;
    }
    check_callback_chain(lst);
  }
}

/* Add a callback to the linked list pointed to by ptr. */
PMOD_EXPORT struct callback *debug_add_to_callback(struct callback_list *lst,
				       callback_func call,
				       void *arg,
				       callback_func free_func)
{
  struct callback *l;
  l=ba_alloc(&callback_allocator);
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
PMOD_EXPORT void *remove_callback(struct callback *l)
{
  l->call=0;
  l->free_func=0;
  return l->arg;
}

/* Free all the callbacks in a linked list of callbacks */
void free_callback_list(struct callback_list *lst)
{
  struct callback *l,**ptr;
  check_callback_chain(lst);
  ptr=& lst->callbacks;
  while((l=*ptr))
  {
    if(l->free_func)
      l->free_func(l, l->arg, 0);
    *ptr=l->next;
    ba_free(&callback_allocator, l);
  }
}

void cleanup_callbacks(void)
{
  ba_destroy(&callback_allocator);
}
