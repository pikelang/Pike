/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef CALLBACK_H
#define CALLBACK_H

#include "array.h"

struct callback;

typedef void (*callback_func)(struct callback *, void *);

/* Prototypes begin here */
struct callback;
struct callback_block;
void call_callback(struct callback **ptr);
struct callback *add_to_callback(struct callback **ptr,
				 callback_func call,
				 void *arg,
				 callback_func free_func);
void *remove_callback(struct callback *l);
void free_callback(struct callback **ptr);
/* Prototypes end here */

#endif
