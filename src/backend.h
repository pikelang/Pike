/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef BACKEND_H
#define BACKEND_H

#include "global.h"
#include "time_stuff.h"
#include "callback.h"

extern struct timeval current_time;
extern struct timeval next_timeout;
typedef void (*file_callback)(int,void *);

/* Prototypes begin here */
struct selectors;
struct callback *add_backend_callback(callback_func call,
				      void *arg,
				      callback_func free_func);
void init_backend();
void set_read_callback(int fd,file_callback cb,void *data);
void set_write_callback(int fd,file_callback cb,void *data);
file_callback query_read_callback(int fd);
file_callback query_write_callback(int fd);
void *query_read_callback_data(int fd);
void *query_write_callback_data(int fd);
void do_debug();
void backend();
int write_to_stderr(char *a, INT32 len);
/* Prototypes end here */

#endif
