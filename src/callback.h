/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef CALLBACK_H
#define CALLBACK_H

#include "array.h"

/* Prototypes begin here */
struct callback_list;
struct callback_block;
void call_callback_list(struct callback_list **ptr);
struct callback_list *add_to_callback_list(struct callback_list **ptr,
					   callback call,
					   void *arg);
void *remove_callback(struct callback_list *l);
void free_callback_list(struct callback_list **ptr);
void call_and_free_callback_list(struct callback_list **ptr);
/* Prototypes end here */

#endif
