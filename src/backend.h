/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: backend.h,v 1.7 1999/05/02 08:11:29 hubbe Exp $
 */
#ifndef BACKEND_H
#define BACKEND_H

#include "global.h"
#include "time_stuff.h"
#include "callback.h"

extern struct timeval current_time;
extern struct timeval next_timeout;
typedef void (*file_callback)(int,void *);
extern struct callback_list do_debug_callbacks;

/* Prototypes begin here */
struct selectors;
struct callback *debug_add_backend_callback(callback_func call,
				      void *arg,
				      callback_func free_func);
void wake_up_backend(void);
void init_backend(void);
void set_read_callback(int fd,file_callback cb,void *data);
void set_write_callback(int fd,file_callback cb,void *data);
#ifdef WITH_OOB
void set_read_oob_callback(int fd,file_callback cb,void *data);
void set_write_oob_callback(int fd,file_callback cb,void *data);
#endif /* WITH_OOB */
file_callback query_read_callback(int fd);
file_callback query_write_callback(int fd);
#ifdef WITH_OOB
file_callback query_read_oob_callback(int fd);
file_callback query_write_oob_callback(int fd);
#endif /* WITH_OOB */
void *query_read_callback_data(int fd);
void *query_write_callback_data(int fd);
#ifdef WITH_OOB
void *query_read_oob_callback_data(int fd);
void *query_write_oob_callback_data(int fd);
#endif /* WITH_OOB */
void do_debug(void);
void backend(void);
int write_to_stderr(char *a, INT32 len);
/* Prototypes end here */

#define add_backend_callback(X,Y,Z) \
  dmalloc_touch(struct callback *,debug_add_backend_callback((X),(Y),(Z)))

#endif
