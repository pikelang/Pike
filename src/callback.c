/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "macros.h"
#include "callback.h"

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
static void check_callback_chain(struct callback_list *lst)
{
  int len=0;
  struct callback *foo;
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
}
#else
#define check_callback_chain(X)
#endif

/* Return the first free callback struct, allocate more if needed */
static struct callback *get_free_callback()
{
  struct callback *tmp;
  if(!(tmp=free_callbacks))
  {
    int e;
    struct callback_block *tmp2;
    tmp2=ALLOC_STRUCT(callback_block);
    tmp2->next=callback_chunks;
    callback_chunks=tmp2;

    for(e=0;e<(int)sizeof(CALLBACK_CHUNK);e++)
    {
      tmp2->callbacks[e].next=tmp;
      tmp=tmp2->callbacks+e;
    }
  }
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
  while(l=*ptr)
  {
    if(l->call)
    {
      l->call(l,l->arg, arg);
      if(lst->num_calls != this_call) return;
    }
    check_callback_chain(lst);

    if(!l->call)
    {
      *ptr=l->next;
      l->next=free_callbacks;
      free_callbacks=l;
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
  return l->arg;
}

/* Free all the callbacks in a linked list of callbacks */
void free_callback(struct callback_list *lst)
{
  struct callback *l,**ptr;
  check_callback_chain(lst);
  ptr=& lst->callbacks;
  while(l=*ptr)
  {
    if(l->arg && l->free_func)
      l->free_func(l, l->arg, 0);
    *ptr=l->next;
    l->next=free_callbacks;
    free_callbacks=l;
  }
}

void cleanup_callbacks()
{
  while(callback_chunks)
  {
    struct callback_block *tmp=callback_chunks;
    callback_chunks=tmp->next;
    free((char *)tmp);
  }
  free_callbacks=0;
}
