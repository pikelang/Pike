/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: backend.h,v 1.21 2004/04/03 23:50:58 mast Exp $
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
extern struct callback_list do_debug_callbacks;
PMOD_EXPORT extern int fds_size;
PMOD_EXPORT extern struct program *Backend_program;

struct Backend_struct *get_backend_for_fd(int fd);
PMOD_EXPORT struct object *get_backend_obj_for_fd (int fd);
PMOD_EXPORT void set_backend_for_fd (int fd, struct Backend_struct *new);

PMOD_EXPORT struct object *get_backend_obj (struct Backend_struct *b);
PMOD_EXPORT void wake_up_backend(void);
void init_backend(void);
void do_debug(void);
void backend(void);
void exit_backend(void);
PMOD_EXPORT int write_to_stderr(char *a, size_t len);
PMOD_EXPORT struct callback *debug_add_backend_callback(callback_func call,
							void *arg,
							callback_func free_func);

/* New style callback interface. */

struct fd_callback_box;

typedef int (*fd_box_callback) (struct fd_callback_box *box, int event);

/* The callback user should keep an instance of this struct around for as long
 * as callbacks are wanted. Use hook_fd_callback_box and
 * unhook_fd_callback_box to connect/disconnect it to/from a backend. */
struct fd_callback_box
{
  struct Backend_struct *backend; /* Not refcounted. Cleared when the backend
				   * is destructed. */
  struct object *ref_obj;	/* If set, it's the object containing the box
				 * and it then acts as the ref from the
				 * backend to the object. */
  int fd;			/* May not change after hook-in. */
  int events;			/* Wanted events. Always use
				 * set_fd_callback_events to change this. It's
				 * ok to have hooked boxes where no events are
				 * wanted. */
  fd_box_callback callback;	/* Function to call. Assumed to be set if any
				 * event is registered. */
};

#define INIT_FD_CALLBACK_BOX(BOX, BACKEND, REF_OBJ, FD, CALLBACK) do {	\
    struct fd_callback_box *box__ = (BOX);				\
    box__->backend = (BACKEND);						\
    box__->ref_obj = (REF_OBJ);						\
    box__->fd = (FD);							\
    box__->events = 0;							\
    box__->callback = (CALLBACK);					\
  } while (0)

/* Flags for the events field. */
#define PIKE_FD_READ		0x0001
#define PIKE_FD_WRITE		0x0002
#define PIKE_FD_READ_OOB	0x0004
#define PIKE_FD_WRITE_OOB	0x0008

PMOD_EXPORT void hook_fd_callback_box (struct fd_callback_box *box);
PMOD_EXPORT void unhook_fd_callback_box (struct fd_callback_box *box);
PMOD_EXPORT void set_fd_callback_events (struct fd_callback_box *box, int events);
PMOD_EXPORT void change_backend_for_fd (struct fd_callback_box *box,
					struct Backend_struct *new);

/* Old style callback interface. This only accesses the default backend. It
 * can't be mixed with the new style interface above for the same fd. */

typedef int (*file_callback)(int,void *);

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

#define add_backend_callback(X,Y,Z) \
  dmalloc_touch(struct callback *,debug_add_backend_callback((X),(Y),(Z)))

#endif
