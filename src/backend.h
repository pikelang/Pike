/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef BACKEND_H
#define BACKEND_H

#include "global.h"

#ifdef HAVE_TIME_H
#include <time.h>
#undef HAVE_TIME_H
#endif

extern time_t current_time;
typedef void (*callback)(int,void *);

/* Prototypes begin here */
struct selectors;
struct callback_list *add_backend_callback(struct array *a);
void init_backend();
void set_read_callback(int fd,callback cb,void *data);
void set_write_callback(int fd,callback cb,void *data);
callback query_read_callback(int fd);
callback query_write_callback(int fd);
void *query_read_callback_data(int fd);
void *query_write_callback_data(int fd);
void do_debug();
void backend();
int write_to_stderr(char *a, INT32 len);
/* Prototypes end here */

#endif
