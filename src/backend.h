/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: backend.h,v 1.10 2000/12/16 04:58:17 marcus Exp $
 */
#ifndef BACKEND_H
#define BACKEND_H

#include "global.h"
#include "time_stuff.h"
#include "callback.h"

PMOD_EXPORT extern struct timeval current_time;
PMOD_EXPORT extern struct timeval next_timeout;
typedef void (*file_callback)(int,void *);
extern struct callback_list do_debug_callbacks;
PMOD_EXPORT extern int fds_size;

/* Prototypes begin here */
struct selectors;
PMOD_EXPORT struct callback *debug_add_backend_callback(callback_func call,
				      void *arg,
				      callback_func free_func);
PMOD_EXPORT void wake_up_backend(void);
void init_backend(void);
void set_read_callback(int fd,file_callback cb,void *data);
void set_write_callback(int fd,file_callback cb,void *data);
#ifdef WITH_OOB
void set_read_oob_callback(int fd,file_callback cb,void *data);
void set_write_oob_callback(int fd,file_callback cb,void *data);
#endif /* WITH_OOB */
PMOD_EXPORT file_callback query_read_callback(int fd);
PMOD_EXPORT file_callback query_write_callback(int fd);
#ifdef WITH_OOB
PMOD_EXPORT file_callback query_read_oob_callback(int fd);
PMOD_EXPORT file_callback query_write_oob_callback(int fd);
#endif /* WITH_OOB */
PMOD_EXPORT void *query_read_callback_data(int fd);
PMOD_EXPORT void *query_write_callback_data(int fd);
#ifdef WITH_OOB
PMOD_EXPORT void *query_read_oob_callback_data(int fd);
PMOD_EXPORT void *query_write_oob_callback_data(int fd);
#endif /* WITH_OOB */
void do_debug(void);
void backend(void);
PMOD_EXPORT int write_to_stderr(char *a, size_t len);
/* Prototypes end here */

#define add_backend_callback(X,Y,Z) \
  dmalloc_touch(struct callback *,debug_add_backend_callback((X),(Y),(Z)))

#endif
