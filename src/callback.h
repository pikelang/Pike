#ifndef CALLBACK_H
#define CALLBACK_H

#include "array.h"

/* Prototypes begin here */
struct callback
{
  struct callback *next, *prev;
  struct array *args;
};
struct callback_list;
void unlink_callback(struct callback *c);
void link_callback(struct callback *c);
void do_callback(struct callback *c);
void unlink_and_call_callback(struct callback *c);
void call_callback_list(struct callback_list **ptr);
struct callback_list *add_to_callback_list(struct callback_list **ptr, struct array *a);
void remove_from_callback_list(struct callback_list *l);
/* Prototypes end here */

#endif
