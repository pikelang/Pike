/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: backend.h,v 1.27 2004/06/30 21:41:55 nilsson Exp $
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
PMOD_EXPORT extern struct Backend_struct *default_backend;
extern struct callback_list do_debug_callbacks;
PMOD_EXPORT extern struct program *Backend_program;

PMOD_EXPORT void debug_check_fd_not_in_use (int fd);
#if 1
struct Backend_struct *get_backend_for_fd(int fd);
PMOD_EXPORT struct object *get_backend_obj_for_fd (int fd);
PMOD_EXPORT void set_backend_for_fd (int fd, struct Backend_struct *new);
#endif

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
				   * is destructed or the box is unhooked. */
  struct object *ref_obj;	/* If set, it's the object containing the box.
				 * It then acts as the ref from the backend to
				 * the object and is refcounted by the backend
				 * whenever any event is wanted. */
  int fd;			/* Use change_fd_for_box to change this. May
				 * be negative, in which case only the ref
				 * magic on backend and ref_obj is done. The
				 * backend might change a negative value. */
  int events;			/* Bitfield with wanted events. Always use
				 * set_fd_callback_events to change this. It's
				 * ok to have hooked boxes where no events are
				 * wanted. */
  fd_box_callback callback;	/* Function to call. Assumed to be valid if
				 * any event is wanted. */
};

#define INIT_FD_CALLBACK_BOX(BOX, BACKEND, REF_OBJ, FD, EVENTS, CALLBACK) do { \
    struct fd_callback_box *box__ = (BOX);				\
    box__->backend = (BACKEND);						\
    box__->ref_obj = (REF_OBJ);						\
    box__->fd = (FD);							\
    box__->events = (EVENTS);						\
    box__->callback = (CALLBACK);					\
    hook_fd_callback_box (box__);					\
  } while (0)

/* The event types. */
#define PIKE_FD_READ		0
#define PIKE_FD_WRITE		1
#define PIKE_FD_READ_OOB	2
#define PIKE_FD_WRITE_OOB	3
#define PIKE_FD_ERROR		4
#define PIKE_FD_NUM_EVENTS	5

/* Flags for event bitfields. */
#define PIKE_BIT_FD_READ	(1 << PIKE_FD_READ)
#define PIKE_BIT_FD_WRITE	(1 << PIKE_FD_WRITE)
#define PIKE_BIT_FD_READ_OOB	(1 << PIKE_FD_READ_OOB)
#define PIKE_BIT_FD_WRITE_OOB	(1 << PIKE_FD_WRITE_OOB)
#define PIKE_BIT_FD_ERROR	(1 << PIKE_FD_ERROR)

/* Note: If ref_obj is used, both unhook_fd_callback_box and
 * set_fd_callback_events might free the object containing the box.
 * They may be used from within the gc recurse passes. */
PMOD_EXPORT struct fd_callback_box *
 safe_get_box (struct Backend_struct *me, int fd);
PMOD_EXPORT void hook_fd_callback_box (struct fd_callback_box *box);
PMOD_EXPORT void unhook_fd_callback_box (struct fd_callback_box *box);
PMOD_EXPORT void set_fd_callback_events (struct fd_callback_box *box, int events);
PMOD_EXPORT void change_backend_for_box (struct fd_callback_box *box,
					 struct Backend_struct *new);
PMOD_EXPORT void change_fd_for_box (struct fd_callback_box *box, int new_fd);

/* Old style callback interface. This only accesses the default backend. It
 * can't be mixed with the new style interface above for the same fd. */

typedef int (*file_callback)(int,void *);

#if 1
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
#endif

#define add_backend_callback(X,Y,Z) \
  dmalloc_touch(struct callback *,debug_add_backend_callback((X),(Y),(Z)))

#endif
