/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: callback.h,v 1.19 2005/04/08 16:54:19 grubba Exp $
*/

#ifndef CALLBACK_H
#define CALLBACK_H

#include "array.h"

struct callback;

struct callback_list
{
  struct callback *callbacks;
  int num_calls;
};

extern struct callback_list fork_child_callback;

typedef void (*callback_func)(struct callback *, void *,void *);

#include "block_alloc_h.h"
/* Prototypes begin here */
struct callback;
BLOCK_ALLOC(callback, CALLBACK_CHUNK);
PMOD_EXPORT void low_call_callback(struct callback_list *lst, void *arg);
PMOD_EXPORT struct callback *debug_add_to_callback(struct callback_list *lst,
						   callback_func call,
						   void *arg,
						   callback_func free_func);
PMOD_EXPORT void *remove_callback(struct callback *l);
void free_callback_list(struct callback_list *lst);
void cleanup_callbacks(void);
/* Prototypes end here */

#define add_to_callback(LST,CALL,ARG,FF) \
  dmalloc_touch(struct callback *,debug_add_to_callback((LST),(CALL),(ARG),(FF)))

#if 1
#define call_callback(LST, ARG) do {			\
  struct callback_list *lst_=(LST);			\
  void *arg_=(ARG);					\
  if(lst_->callbacks) low_call_callback(lst_, arg_);	\
}while(0)
#else
#define call_callback(LST, ARG) low_call_callback((LST), (ARG))
#endif

#endif
