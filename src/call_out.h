/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef CALL_OUT_H
#define CALL_OUT_H

#include "types.h"

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  if HAVE_TIME_H
#   include <time.h>
#  endif
# endif
#endif

#undef HAVE_SYS_TIME_H
#undef HAVE_TIME_H
#undef TIME_WITH_SYS_TIME

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
