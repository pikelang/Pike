/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef BACKEND_H
#define BACKEND_H

#include "global.h"
#include "callback.h"

/*
 * POLL/SELECT selection
 */

#if defined(HAVE_SYS_DEVPOLL_H) && defined(PIKE_POLL_DEVICE)
/*
 * Backend using /dev/poll-style poll device.
 *
 * Used on:
 *   Solaris 7 + patches and above.
 *   OSF/1 + patches and above.
 */
#define BACKEND_USES_POLL_DEVICE
#define BACKEND_USES_DEVPOLL
#elif defined(HAVE_SYS_EPOLL_H) && defined(WITH_EPOLL)
/*
 * Backend using /dev/epoll-style poll device.
 *
 * Used on:
 *   Linux 2.6 and above.
 */
#define BACKEND_USES_POLL_DEVICE
#define BACKEND_USES_DEVEPOLL
#elif defined(HAVE_SYS_EVENT_H) && defined(HAVE_KQUEUE) /* && !HAVE_POLL */
/*
 * Backend using kqueue-style poll device.
 *
 * FIXME: Not fully implemented yet! Out of band data handling is missing.
 *
 * Used on
 *   FreeBSD 4.1 and above.
 *   MacOS X/Darwin 7.x and above.
 *   Various other BSDs.
 */
#define BACKEND_USES_KQUEUE
/* Currently kqueue doesn't differentiate between in-band and out-of-band
 * data.
 */
#define BACKEND_OOB_IS_SIMULATED

#ifdef HAVE_CFRUNLOOPRUNINMODE
/* Have kqueue+CFRunLoop variant (Mac OSX, iOS) */
#define BACKEND_USES_CFRUNLOOP
#endif /* HAVE_CFRUNLOOPRUNINMODE */

#elif defined(HAVE_POLL) && defined(HAVE_AND_USE_POLL)
/* We have poll(2), and it isn't simulated. */
/*
 * Backend using poll(2).
 *
 * This is used on most older SVR4- or POSIX-style systems.
 */
#define BACKEND_USES_POLL
#else /* !HAVE_POLL */
/*
 * Backend using select(2)
 *
 * This is used on most older BSD-style systems, and WIN32.
 */
#define BACKEND_USES_SELECT
#endif /* HAVE_SYS_DEVPOLL_H && PIKE_POLL_DEVICE */

struct Backend_struct;

PMOD_EXPORT extern struct Backend_struct *default_backend;
extern struct callback_list do_debug_callbacks;
PMOD_EXPORT extern struct program *Backend_program;

void count_memory_in_compat_cb_boxs(size_t *num, size_t *size);
void free_all_compat_cb_box_blocks(void);

PMOD_EXPORT void debug_check_fd_not_in_use (int fd);
#if 1
struct Backend_struct *get_backend_for_fd(int fd);
PMOD_EXPORT struct object *get_backend_obj_for_fd (int fd);
PMOD_EXPORT void set_backend_for_fd (int fd, struct Backend_struct *new_be);
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

/*
 * New style callback interface.
 */

struct fd_callback_box;

/**
 * Callback for when an event has triggered on a box.
 *
 * Two return values are defined:
 *   0  - Continue backend processing with the next event.
 *  -1  - Terminate the current round of the backend and restart processing.
 */
typedef int (*fd_box_callback) (struct fd_callback_box *box, int event);

/** The callback user should keep an instance of this struct around for as long
 * as callbacks are wanted. Use hook_fd_callback_box and
 * unhook_fd_callback_box to connect/disconnect it to/from a backend. */
struct fd_callback_box
{
  struct Backend_struct *backend; /**< Not refcounted. Cleared when the backend
				   * is destructed or the box is unhooked. */
  struct object *ref_obj;	/**< If set, it's the object containing the box.
				 * It then acts as the ref from the backend to
				 * the object and is refcounted by the backend
				 * whenever any event is wanted.
				 *
				 * It receives a ref for each when next and/or
				 * events are non-zero. */
  struct fd_callback_box *next;	/**< Circular list of active fds in a backend.
				 * NULL if the fd is not active in some
				 * backend. Note: ref_obj MUST be freed when
				 * the box is unlinked. */
  int fd;			/**< Use change_fd_for_box to change this. May
				 * be negative, in which case only the ref
				 * magic on backend and ref_obj is done. The
				 * backend might change a negative value to a
				 * different negative value. */
  int events;			/**< Bitfield with wanted events. Always use
				 * set_fd_callback_events to change this. It's
				 * ok to have hooked boxes where no events are
				 * wanted. */
  int revents;			/**< Bitfield with active events. Always clear
				 * the corresponding event if you perform an
				 * action that might affect it. */
  int flags; /** < Bitfield used for system dependent flags, such as for kqueue(2) events */
  int rflags; /** < Bitfield used for active flags, such as for kqueue(2) events */
  fd_box_callback callback;	/**< Function to call. Assumed to be valid if
				 * any event is wanted. */
};

#define INIT_FD_CALLBACK_BOX(BOX, BACKEND, REF_OBJ, FD, EVENTS, CALLBACK, FLAGS) do { \
    struct fd_callback_box *box__ = (BOX);				\
    box__->backend = (BACKEND);						\
    box__->ref_obj = (REF_OBJ);						\
    box__->next = NULL;							\
    box__->fd = (FD);							\
    box__->events = (EVENTS);						\
    box__->revents = 0;							\
    box__->callback = (CALLBACK);	\
    box__->flags = (FLAGS);			\
    if (box__->backend) hook_fd_callback_box (box__);			\
  } while (0)

/* The event types. */
#define PIKE_FD_READ		0
#define PIKE_FD_WRITE		1
#define PIKE_FD_READ_OOB	2
#define PIKE_FD_WRITE_OOB	3
#define PIKE_FD_ERROR		4
/* FS_EVENT is currently only implemented in kqueue backends. */
#define PIKE_FD_FS_EVENT		5
#define PIKE_FD_NUM_EVENTS	6

/* Flags for event bitfields. */
#define PIKE_BIT_FD_READ	(1 << PIKE_FD_READ)
#define PIKE_BIT_FD_WRITE	(1 << PIKE_FD_WRITE)
#define PIKE_BIT_FD_READ_OOB	(1 << PIKE_FD_READ_OOB)
#define PIKE_BIT_FD_WRITE_OOB	(1 << PIKE_FD_WRITE_OOB)
#define PIKE_BIT_FD_ERROR	(1 << PIKE_FD_ERROR)
/* FS_EVENT is currently only implemented in kqueue backends. */
#define PIKE_BIT_FD_FS_EVENT	(1 << PIKE_FD_FS_EVENT)

/* If an error condition occurs then all events except
 * PIKE_BIT_FD_ERROR are cleared from fd_callback_box.events. */

/* Note: If ref_obj is used, both unhook_fd_callback_box and
 * set_fd_callback_events may free the object containing the box.
 * They may be used from within the gc recurse passes. */
PMOD_EXPORT void hook_fd_callback_box (struct fd_callback_box *box);
PMOD_EXPORT void unhook_fd_callback_box (struct fd_callback_box *box);
PMOD_EXPORT void set_fd_callback_events (struct fd_callback_box *box, int events, int flags);
PMOD_EXPORT void change_backend_for_box (struct fd_callback_box *box,
					 struct Backend_struct *new_be);
PMOD_EXPORT void change_fd_for_box (struct fd_callback_box *box, int new_fd);

PMOD_EXPORT struct fd_callback_box *get_fd_callback_box_for_fd (struct Backend_struct *me, int fd);

/* Old style callback interface. This only accesses the default backend. It
 * can't be mixed with the new style interface above for the same fd. */

typedef int (*file_callback)(int,void *);

#if 1
PMOD_EXPORT void set_read_callback(int fd,file_callback cb,void *data);
PMOD_EXPORT void set_write_callback(int fd,file_callback cb,void *data);
PMOD_EXPORT void set_read_oob_callback(int fd,file_callback cb,void *data);
PMOD_EXPORT void set_write_oob_callback(int fd,file_callback cb,void *data);
PMOD_EXPORT void set_fs_event_callback(int fd,file_callback cb,void *data, int flags);
PMOD_EXPORT file_callback query_read_callback(int fd);
PMOD_EXPORT file_callback query_write_callback(int fd);
PMOD_EXPORT file_callback query_read_oob_callback(int fd);
PMOD_EXPORT file_callback query_write_oob_callback(int fd);
PMOD_EXPORT file_callback query_fs_event_callback(int fd);
PMOD_EXPORT void *query_read_callback_data(int fd);
PMOD_EXPORT void *query_write_callback_data(int fd);
PMOD_EXPORT void *query_read_oob_callback_data(int fd);
PMOD_EXPORT void *query_write_oob_callback_data(int fd);
PMOD_EXPORT void *query_fs_event_callback_data(int fd);
#endif

PMOD_EXPORT void backend_wake_up_backend(struct Backend_struct *be);
PMOD_EXPORT void backend_lower_timeout(struct Backend_struct *me,
				       struct timeval *tv);
PMOD_EXPORT struct object *get_backend_obj (struct Backend_struct *b);
PMOD_EXPORT struct callback *backend_debug_add_backend_callback(
    struct Backend_struct *be, callback_func call, void *arg,
    callback_func free_func);

#define add_backend_callback(X,Y,Z) \
  dmalloc_touch(struct callback *,debug_add_backend_callback((X),(Y),(Z)))

#define backend_add_backend_callback(B,X,Y,Z) \
  dmalloc_touch(struct callback *,\
                backend_debug_add_backend_callback((B),(X),(Y),(Z)))

#endif
