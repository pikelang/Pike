/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "macros.h"
#include "callback.h"

struct callback
{
  struct callback *next;
  callback_func call;
  callback_func free_func;
  void *arg;
};

struct callback *first_callback =0;
struct callback *free_callbacks =0;

#define CALLBACK_CHUNK 128

struct callback_block {
  struct callback_block *next;
  struct callback callbacks[CALLBACK_CHUNK];
};

static struct callback *get_free_callback()
{
  struct callback *tmp;
  if(!(tmp=free_callbacks))
  {
    int e;
    struct callback_block *tmp2;
    tmp2=ALLOC_STRUCT(callback_block);

    for(e=0;e<sizeof(CALLBACK_CHUNK);e++)
    {
      tmp2->callbacks[e].next=tmp;
      tmp=tmp2->callbacks+e;
    }
  }
  free_callbacks=tmp->next;
  return tmp;
}

void call_callback(struct callback **ptr)
{
  struct callback *l;
  while(l=*ptr)
  {
    if(l->call) l->call(l,l->arg);

    if(!l->call)
    {
      *ptr=l->next;
      free((char *)l);
    }else{
      ptr=& l->next;
      l->next=free_callbacks;
      free_callbacks=l;
    }
  }
}

struct callback *add_to_callback(struct callback **ptr,
				 callback_func call,
				 void *arg,
				 callback_func free_func)
{
  struct callback *l;
  l=get_free_callback();
  l->call=call;
  l->arg=arg;

  l->next=*ptr;
  *ptr=l;

  return l;
}

/* It is not actually freed until next time this callback is called
 */
void *remove_callback(struct callback *l)
{
  l->call=0;
  return l->arg;
}

void free_callback(struct callback **ptr)
{
  struct callback *l;
  while(l=*ptr)
  {
    if(l->arg && l->free_func)
      l->free_func(l, l->arg);
    *ptr=l->next;
    l->next=free_callbacks;
    free_callbacks=l;
  }
}
