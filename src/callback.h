/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: callback.h,v 1.12 2000/12/19 00:42:02 hubbe Exp $
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
BLOCK_ALLOC(callback, CALLBACK_CHUNK)
PMOD_EXPORT void call_callback(struct callback_list *lst, void *arg);
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

#endif
