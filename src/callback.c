/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "macros.h"
#include "callback.h"

struct callback_list
{
  struct callback_list *next;
  struct callback call;
};

struct callback *first_callback =0;

void unlink_callback(struct callback *c)
{
  if(c->next) c->next->prev=c->prev;
  if(c->prev) c->prev->next=c->next;
}

void link_callback(struct callback *c)
{
  if(first_callback) first_callback->prev=c;
  first_callback=c;
  c->next=first_callback;
  c->prev=0;
}

void do_callback(struct callback *c)
{
  c->args->refs++;
  push_array_items(c->args);
  f_call_function(c->args->size);
}

void unlink_and_call_callback(struct callback *c)
{
  int size;
  size=c->args->size;
  push_array_items(c->args);
  c->args=0;
  f_call_function(size);
  unlink_callback(c);
}

/*** ***/

void call_callback_list(struct callback_list **ptr)
{
  struct callback_list *l;
  while(l=*ptr)
  {
    if(l->call.args)
    {
      do_callback(& l->call);
      ptr=& l->next;
    }else{
      unlink_callback(& l->call);
      *ptr=l->next;
      free((char *)l);
    }
  }
}

/* NOTICE, eats one reference off array! */
struct callback_list *add_to_callback_list(struct callback_list **ptr, struct array *a)
{
  struct callback_list *l;
  l=ALLOC_STRUCT(callback_list);
  link_callback(& l->call);
  l->call.args=a;
  l->next=*ptr;
  *ptr=l;
  return l;
}

void remove_callback(struct callback_list *l)
{
  free_array(l->call.args);
  l->call.args=0;
}

void free_callback_list(struct callback_list **ptr)
{
  struct callback_list *l;
  while(l=*ptr)
  {
    if(l->call.args)
      free_array(l->call.args);
      
    unlink_callback(& l->call);
    *ptr=l->next;
    free((char *)l);
  }
}

void call_and_free_callback_list(struct callback_list **ptr)
{
  struct callback_list *l;
  while(l=*ptr)
  {
    if(l->call.args)
      unlink_and_call_callback(& l->call);
    else
      unlink_callback(& l->call);
      
    *ptr=l->next;
    free((char *)l);
  }
}

