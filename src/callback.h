/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef CALLBACK_H
#define CALLBACK_H

#include "array.h"

struct callback;

typedef void (*callback_func)(struct callback *, void *,void *);

/* Prototypes begin here */
struct callback;
struct callback_block;
void call_callback(struct callback **ptr, void *arg);
struct callback *add_to_callback(struct callback **ptr,
				 callback_func call,
				 void *arg,
				 callback_func free_func);
void *remove_callback(struct callback *l);
void free_callback(struct callback **ptr);
void cleanup_callbacks();
/* Prototypes end here */

#endif
