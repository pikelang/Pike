/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: backend.h,v 1.17 2005/03/22 11:46:37 jonasw Exp $
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
struct Backend_struct *get_backend_for_fd(int fd);
PMOD_EXPORT void set_backend_for_fd(int fd, struct Backend_struct *b);
PMOD_EXPORT struct callback *debug_add_backend_callback(callback_func call,
				      void *arg,
				      callback_func free_func);
PMOD_EXPORT void wake_up_backend(void);
void init_backend(void);
void set_read_callback(int fd,file_callback cb,void *data);
void set_write_callback(int fd,file_callback cb,void *data);
#ifdef WITH_OOB
PMOD_EXPORT void set_read_oob_callback(int fd,file_callback cb,void *data);
PMOD_EXPORT void set_write_oob_callback(int fd,file_callback cb,void *data);
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
void exit_backend(void);
PMOD_EXPORT int write_to_stderr(char *a, size_t len);
/* Prototypes end here */

#define add_backend_callback(X,Y,Z) \
  dmalloc_touch(struct callback *,debug_add_backend_callback((X),(Y),(Z)))

#endif
