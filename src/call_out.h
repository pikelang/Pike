/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef CALL_OUT_H
#define CALL_OUT_H

#include "types.h"

#ifdef HAVE_TIME_H
/* Needed for time_t */
#include <time.h>
#undef HAVE_TIME_H
#endif

struct call_out_s
{
  time_t time;
  struct object *caller;
  struct array *args;
};

typedef struct call_out_s call_out;

extern call_out **pending_calls; /* pointer to first busy pointer */
extern int num_pending_calls; /* no of busy pointers in buffer */

/* Prototypes begin here */
void f_call_out(INT32 args);
void do_call_outs();
void f_find_call_out(INT32 args);
void f_remove_call_out(INT32 args);
struct array *get_all_call_outs();
void f_call_out_info(INT32 args);
void free_all_call_outs();
time_t get_next_call_out();
void verify_all_call_outs();
/* Prototypes end here */

#endif
