/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
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

/* Prototypes begin here */
struct callback;
struct callback_block;
void call_callback(struct callback_list *lst, void *arg);
struct callback *add_to_callback(struct callback_list *lst,
				 callback_func call,
				 void *arg,
				 callback_func free_func);
void *remove_callback(struct callback *l);
void free_callback_list(struct callback_list *lst);
void cleanup_callbacks(void);
void count_memory_in_callbacks(INT32 *num_, INT32 *size_);
/* Prototypes end here */

#endif
