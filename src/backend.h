/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: backend.h,v 1.20 2003/10/28 00:00:25 mast Exp $
*/

#ifndef BACKEND_H
#define BACKEND_H

#include "global.h"
#include "time_stuff.h"
#include "callback.h"

struct Backend_struct;
struct selectors;

PMOD_EXPORT extern struct timeval current_time;
PMOD_EXPORT extern struct timeval next_timeout;
extern struct Backend_struct *default_backend;
typedef int (*file_callback)(int,void *);
extern struct callback_list do_debug_callbacks;
PMOD_EXPORT extern int fds_size;
PMOD_EXPORT extern struct program *Backend_program;

/* Prototypes begin here */
struct Backend_struct *get_backend_for_fd(int fd);
PMOD_EXPORT struct callback *debug_add_backend_callback(callback_func call,
				      void *arg,
				      callback_func free_func);
PMOD_EXPORT void wake_up_backend(void);
void init_backend(void);
PMOD_EXPORT struct object *get_backend_obj_for_fd (int fd);
PMOD_EXPORT void set_backend_for_fd (int fd, struct Backend_struct *new);
PMOD_EXPORT void set_read_callback(int fd,file_callback cb,void *data);
PMOD_EXPORT void set_write_callback(int fd,file_callback cb,void *data);
PMOD_EXPORT void set_read_oob_callback(int fd,file_callback cb,void *data);
PMOD_EXPORT void set_write_oob_callback(int fd,file_callback cb,void *data);
PMOD_EXPORT file_callback query_read_callback(int fd);
PMOD_EXPORT file_callback query_write_callback(int fd);
PMOD_EXPORT file_callback query_read_oob_callback(int fd);
PMOD_EXPORT file_callback query_write_oob_callback(int fd);
PMOD_EXPORT void *query_read_callback_data(int fd);
PMOD_EXPORT void *query_write_callback_data(int fd);
PMOD_EXPORT void *query_read_oob_callback_data(int fd);
PMOD_EXPORT void *query_write_oob_callback_data(int fd);
void do_debug(void);
void backend(void);
void exit_backend(void);
PMOD_EXPORT int write_to_stderr(char *a, size_t len);
/* Prototypes end here */

#define add_backend_callback(X,Y,Z) \
  dmalloc_touch(struct callback *,debug_add_backend_callback((X),(Y),(Z)))

#endif
