/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: backend.c,v 1.67 2003/08/04 14:56:34 mast Exp $");
#include "fdlib.h"
#include "backend.h"
#include <errno.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <string.h>
#include "interpret.h"
#include "object.h"
#include "pike_error.h"
#include "fd_control.h"
#include "main.h"
#include "callback.h"
#include "threads.h"
#include "fdlib.h"

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
/* BeOS socket (select etc) stuff */
#ifdef HAVE_NET_SOCKET_H
#include <net/socket.h>
#endif
#endif
#include <sys/stat.h>

#define SELECT_READ 1
#define SELECT_WRITE 2

/* #define POLL_DEBUG */

#ifdef POLL_DEBUG
#define IF_PD(x)	x
#else /* !POLL_DEBUG */
#define IF_PD(x)
#endif /* POLL_DEBUG */

struct cb_data
{
  file_callback callback;
  void * data;
};

struct fd_datum
{
  struct cb_data read, write;
#ifdef WITH_OOB
  struct cb_data read_oob, write_oob;
#endif
};

struct fd_datum *fds;
PMOD_EXPORT int fds_size = 0;

#define ASSURE_FDS_SIZE(X) do{while(fds_size-1 < X) grow_fds();}while(0)

void grow_fds(void)
{
  int old_size = fds_size;
  if( !fds_size )
    fds_size = 16;
  fds_size *= 2;
  fds = realloc( fds, sizeof(struct fd_datum) * fds_size );
  if( !fds )
    fatal("Out of memory in backend::grow_fds()\n"
          "Tried to allocate %d fd_datum structs\n", fds_size);
  MEMSET(fds + old_size, 0, (fds_size - old_size) * sizeof(struct fd_datum));
}


#ifndef HAVE_AND_USE_POLL
#undef HAVE_POLL
#endif

#ifndef HAVE_POLL

struct selectors
{
  my_fd_set read;
  my_fd_set write;
#ifdef WITH_OOB
  /* except == incoming OOB data
   * outgoing OOB data is multiplexed on write
   */
  my_fd_set except;
#endif /* WITH_OOB */
};

static struct selectors selectors;

#else

#ifdef HAVE_POLL_H
#include <poll.h>
#endif /* HAVE_POLL_H */

#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif /* HAVE_SYS_POLL_H */

/* Some constants... */

/* Notes on POLLRDNORM and POLLIN:
 *
 * According to the AIX manual, POLLIN and POLLRDNORM are both set
 * if there's a nonpriority message on the read queue. POLLIN is
 * also set if the message is of 0 length.
 */

#ifndef POLLRDNORM
#define POLLRDNORM	POLLIN
#endif /* !POLLRDNORM */

#ifndef POLLRDBAND
#define POLLRDBAND	POLLPRI
#endif /* !POLLRDBAND */

#ifndef POLLWRBAND
#define POLLWRBAND	POLLOUT
#endif /* !POLLWRBAND */

struct pollfd *poll_fds = NULL;
int poll_fd_size = 0;
int num_in_poll = 0;
struct pollfd *active_poll_fds = NULL;
int active_poll_fd_size = 0;
int active_num_in_poll = 0;

void POLL_FD_SET(int fd, short add)
{
  int i;
  IF_PD(fprintf(stderr, "BACKEND: POLL_FD_SET(%d, 0x%04x)\n", fd, add));
  for(i=0; i<num_in_poll; i++)
    if(poll_fds[i].fd == fd)
    {
      poll_fds[i].events |= add;
      return;
    }
  num_in_poll++;
  if (num_in_poll > poll_fd_size) {
    poll_fd_size += num_in_poll;	/* Usually a doubling */
    if (!(poll_fds = realloc(poll_fds, sizeof(struct pollfd)*poll_fd_size))) {
      fatal("Out of memory in backend::POLL_FD_SET()\n"
	    "Tried to allocate %d pollfds\n", poll_fd_size);
    }
  }
  poll_fds[num_in_poll-1].fd = fd;
  poll_fds[num_in_poll-1].events = add;
}

void POLL_FD_CLR(int fd, short sub)
{
  int i;
  IF_PD(fprintf(stderr, "BACKEND: POLL_FD_CLR(%d, 0x%04x)\n", fd, sub));
  if(!poll_fds) return;
  for(i=0; i<num_in_poll; i++)
    if(poll_fds[i].fd == fd)
    {
      poll_fds[i].events &= ~sub;
      if(!poll_fds[i].events)
      {
	/* Note that num_in_poll is decreased here.
	 * This is to avoid a lot of -1's below.
	 * /grubba
	 */
	num_in_poll--;
	if(i != num_in_poll) {
	  *(poll_fds+i) = *(poll_fds+num_in_poll);
	}
	/* Might want to shrink poll_fds here, but probably not. */
      }
      break;
    }
  return;
}

void switch_poll_set(void)
{
  struct pollfd *tmp = active_poll_fds;
  int sz = active_poll_fd_size;

  IF_PD(fprintf(stderr, "BACKEND: switch_poll_set()\n"));

  active_num_in_poll = num_in_poll;

  if(!num_in_poll) return;

  active_poll_fds = poll_fds;
  active_poll_fd_size = poll_fd_size;

  poll_fds = tmp;
  poll_fd_size = sz;

  if (num_in_poll > poll_fd_size) {
    poll_fd_size += num_in_poll;	/* Usually a doubling */
    if (!(poll_fds = realloc(poll_fds, sizeof(struct pollfd)*poll_fd_size))) {
      fatal("Out of memory in backend::switch_poll_set()\n"
	    "Tried to allocate %d pollfds\n", poll_fd_size);
    }
  }

  MEMCPY(poll_fds, active_poll_fds, sizeof(struct pollfd)*num_in_poll);
}
#endif


static int max_fd;
PMOD_EXPORT struct timeval current_time;
PMOD_EXPORT struct timeval next_timeout;
static int wakeup_pipe[2];
static int may_need_wakeup=0;

static struct callback_list backend_callbacks;

PMOD_EXPORT struct callback *debug_add_backend_callback(callback_func call,
				      void *arg,
				      callback_func free_func)
{
  return add_to_callback(&backend_callbacks, call, arg, free_func);
}

static void wakeup_callback(int fd, void *foo)
{
  char buffer[1024];
  fd_read(fd, buffer, sizeof(buffer)); /* Clear 'flag' */
}

/* This is used by threaded programs and signals to wake up the
 * master 'thread'.
 */
PMOD_EXPORT void wake_up_backend(void)
{
  char foo=0;
  if(may_need_wakeup)
    fd_write(wakeup_pipe[1], &foo ,1);
}

extern int pike_make_pipe(int *);

void init_backend(void)
{
#ifndef HAVE_POLL
  my_FD_ZERO(&selectors.read);
  my_FD_ZERO(&selectors.write);
#ifdef WITH_OOB
  my_FD_ZERO(&selectors.except);
#endif /* WITH_OOB */
#endif /* !HAVE_POLL */
  if(pike_make_pipe(wakeup_pipe) < 0)
    fatal("Couldn't create backend wakup pipe! errno=%d.\n",errno);
  set_nonblocking(wakeup_pipe[0],1);
  set_nonblocking(wakeup_pipe[1],1);
  set_read_callback(wakeup_pipe[0], wakeup_callback, 0); 
  /* Don't keep these on exec! */
  set_close_on_exec(wakeup_pipe[0], 1);
  set_close_on_exec(wakeup_pipe[1], 1);
}

void cleanup_backend(void)
{
#ifdef HAVE_POLL
  if (poll_fds) {
    free(poll_fds);
    poll_fds = NULL;
    poll_fd_size = 0;
    num_in_poll = 0;
  }
  if (active_poll_fds) {
    free(active_poll_fds);
    active_poll_fds = NULL;
    active_poll_fd_size = 0;
    active_num_in_poll = 0;
  }
#endif /* HAVE_POLL */
#ifdef HAVE_BROKEN_F_SETFD
  cleanup_close_on_exec();
#endif /* HAVE_BROKEN_F_SETFD */
  if (fds) {
    free(fds);
    fds = NULL;
    fds_size = 0;
  }
}

void set_read_callback(int fd,file_callback cb,void *data)
{
#ifdef HAVE_POLL
  int was_set;
#endif
  IF_PD(fprintf(stderr, "BACKEND: set_read_callback(%d, %p, %p)\n",
		fd, cb, data));

  ASSURE_FDS_SIZE( fd );
#ifdef HAVE_POLL
  was_set = (fds[fd].read.callback!=0);
#endif
#ifdef PIKE_DEBUG
  if(fd<0)
    fatal("File descriptor out of range.\n %d",fd);
#endif
  fds[fd].read.callback=cb;
  fds[fd].read.data=data;

  if(cb)
  {
#ifdef HAVE_POLL
    if(!was_set)
      POLL_FD_SET(fd, POLLRDNORM|POLLIN);
#else
    my_FD_SET(fd, &selectors.read);
#endif
    if(max_fd < fd) max_fd = fd;
    wake_up_backend();
  }else{
#ifdef HAVE_POLL
#if defined(WITH_OOB)
    if (was_set && !fds[fd].read_oob.callback)
      POLL_FD_CLR(fd, POLLRDNORM|POLLIN);
#else /* !WITH_OOB */
    if(was_set)
      POLL_FD_CLR(fd, POLLRDNORM|POLLIN);
#endif /* WITH_OOB */
#else /* !HAVE_POLL */
    if(fd <= max_fd)
    {
      my_FD_CLR(fd, &selectors.read);

      if(fd == max_fd)
      {
	while(max_fd >=0 &&
	      !my_FD_ISSET(max_fd, &selectors.read) &&
	      !my_FD_ISSET(max_fd, &selectors.write)
#ifdef WITH_OOB
	      && !my_FD_ISSET(max_fd, &selectors.except)
#endif /* WITH_OOB */
	      )
	  max_fd--;
      }
    }
#endif /* HAVE_POLL */
  }
}

void set_write_callback(int fd,file_callback cb,void *data)
{
#ifdef HAVE_POLL
  int was_set;
#endif
  IF_PD(fprintf(stderr, "BACKEND: set_write_callback(%d, %p, %p)\n",
		fd, cb, data));

  ASSURE_FDS_SIZE( fd );
#ifdef HAVE_POLL
  was_set = (fds[fd].write.callback!=0);
#endif
#ifdef PIKE_DEBUG
  if(fd<0)
    fatal("File descriptor out of range.\n %d",fd);
#endif

  fds[fd].write.callback=cb;
  fds[fd].write.data=data;

  if(cb)
  {
#ifdef HAVE_POLL
    if(!was_set)
      POLL_FD_SET(fd, POLLOUT);
#else
    my_FD_SET(fd, &selectors.write);
#endif
    if(max_fd < fd) max_fd = fd;
    wake_up_backend();
  } else {
#ifdef HAVE_POLL
#if defined(WITH_OOB)
    if (was_set && !fds[fd].write_oob.callback)
      POLL_FD_CLR(fd, POLLOUT);
#else /* !WITH_OOB */
    if(was_set)
      POLL_FD_CLR(fd, POLLOUT);
#endif /* WITH_OOB */
#else /* !HAVE_POLL */
    if(fd <= max_fd)
    {
#ifdef WITH_OOB
      if (!fds[fd].write_oob.callback) {
#endif /* WITH_OOB */
	my_FD_CLR(fd, &selectors.write);
	if(fd == max_fd)
	{
	  while(max_fd >=0 &&
		!my_FD_ISSET(max_fd, &selectors.read) &&
		!my_FD_ISSET(max_fd, &selectors.write)
#ifdef WITH_OOB
		&& !my_FD_ISSET(max_fd, &selectors.except)
#endif /* WITH_OOB */
		)
	    max_fd--;
	}
#ifdef WITH_OOB
      }
#endif /* WITH_OOB */
    }
#endif /* HAVE_POLL */
  }
}

#ifdef WITH_OOB
void set_read_oob_callback(int fd,file_callback cb,void *data)
{
#ifdef HAVE_POLL
  int was_set;
#endif
  IF_PD(fprintf(stderr, "BACKEND: set_read_oob_callback(%d, %p, %p)\n",
		fd, cb, data));

  ASSURE_FDS_SIZE( fd );
#ifdef HAVE_POLL
  was_set = (fds[fd].read_oob.callback!=0);
#endif
#ifdef PIKE_DEBUG
  if(fd<0)
    fatal("File descriptor out of range.\n %d",fd);
#endif
  fds[fd].read_oob.callback=cb;
  fds[fd].read_oob.data=data;

  if(cb)
  {
#ifdef HAVE_POLL
    POLL_FD_SET(fd, POLLRDBAND|POLLRDNORM|POLLIN);
#else
    my_FD_SET(fd, &selectors.except);
#endif
    if(max_fd < fd) max_fd = fd;
    wake_up_backend();
  }else{
#ifdef HAVE_POLL
    if(was_set) {
      if (!fds[fd].read.callback) {
	POLL_FD_CLR(fd, POLLRDBAND|POLLRDNORM|POLLIN);
      } else {
#if (POLLRDBAND != POLLRDNORM) && (POLLRDBAND != POLLIN)
	POLL_FD_CLR(fd, POLLRDBAND);
#endif /* (POLLRDBAND != POLLRDNORM) && (POLLRDBAND != POLLIN) */
      }
    }
#else /* !HAVE_POLL */
    if(fd <= max_fd)
    {
      my_FD_CLR(fd, &selectors.except);

      if(fd == max_fd)
      {
	while(max_fd >=0 &&
	      !my_FD_ISSET(max_fd, &selectors.read) &&
	      !my_FD_ISSET(max_fd, &selectors.write) &&
	      !my_FD_ISSET(max_fd, &selectors.except))
	  max_fd--;
      }
    }
#endif /* HAVE_POLL */
  }
}

void set_write_oob_callback(int fd,file_callback cb,void *data)
{
#ifdef HAVE_POLL
  int was_set;
#endif
  IF_PD(fprintf(stderr, "BACKEND: set_write_oob_callback(%d, %p, %p)\n",
		fd, cb, data));

  ASSURE_FDS_SIZE( fd );
#ifdef HAVE_POLL
  was_set = (fds[fd].write_oob.callback!=0);
#endif
#ifdef PIKE_DEBUG
  if(fd<0)
    fatal("File descriptor out of range.\n %d",fd);
#endif

  fds[fd].write_oob.callback=cb;
  fds[fd].write_oob.data=data;

  if(cb)
  {
#ifdef HAVE_POLL
    POLL_FD_SET(fd, POLLWRBAND|POLLOUT);
#else
    my_FD_SET(fd, &selectors.write);	/* FIXME:? */
#endif
    if(max_fd < fd) max_fd = fd;
    wake_up_backend();
  }else{
#ifdef HAVE_POLL
    if(was_set) {
      if (!fds[fd].write.callback) {
	POLL_FD_CLR(fd, POLLWRBAND|POLLOUT);
      } else {
#if POLLWRBAND != POLLOUT
	POLL_FD_CLR(fd, POLLWRBAND);
#endif /* POLLWRBAND != POLLOUT */
      }
    }
#else /* !HAVE_POLL */
    /* FIXME:? */
    if(fd <= max_fd)
    {
      if (!fds[fd].write.callback) {
	my_FD_CLR(fd, &selectors.write);
	if(fd == max_fd)
	{
	  while(max_fd >=0 &&
		!my_FD_ISSET(max_fd, &selectors.read) &&
		!my_FD_ISSET(max_fd, &selectors.write) &&
		!my_FD_ISSET(max_fd, &selectors.except))
	    max_fd--;
	}
      }
    }
#endif /* HAVE_POLL */
  }
}
#endif /* WITH_OOB */

PMOD_EXPORT file_callback query_read_callback(int fd)
{
#ifdef PIKE_DEBUG
  if(fd<0)
    fatal("File descriptor out of range.\n %d",fd);
#endif
  IF_PD(fprintf(stderr, "BACKEND: query_read_callback(%d)\n", fd));
  ASSURE_FDS_SIZE( fd );
  return fds[fd].read.callback;
}

PMOD_EXPORT file_callback query_write_callback(int fd)
{
#ifdef PIKE_DEBUG
  if(fd<0)
    fatal("File descriptor out of range.\n %d",fd);
#endif
  IF_PD(fprintf(stderr, "BACKEND: query_write_callback(%d)\n", fd));
  ASSURE_FDS_SIZE( fd );
  return fds[fd].write.callback;
}

#ifdef WITH_OOB
PMOD_EXPORT file_callback query_read_oob_callback(int fd)
{
#ifdef PIKE_DEBUG
  if(fd<0)
    fatal("File descriptor out of range.\n %d",fd);
#endif
  IF_PD(fprintf(stderr, "BACKEND: query_read_oob_callback(%d)\n", fd));
  ASSURE_FDS_SIZE( fd );
  return fds[fd].read_oob.callback;
}

PMOD_EXPORT file_callback query_write_oob_callback(int fd)
{
#ifdef PIKE_DEBUG
  if(fd<0)
    fatal("File descriptor out of range.\n %d",fd);
#endif
  IF_PD(fprintf(stderr, "BACKEND: query_write_oob_callback(%d)\n", fd));
  ASSURE_FDS_SIZE( fd );
  return fds[fd].write_oob.callback;
}
#endif /* WITH_OOB */

PMOD_EXPORT void *query_read_callback_data(int fd)
{
#ifdef PIKE_DEBUG
  if(fd<0)
    fatal("File descriptor out of range.\n %d",fd);
#endif
  IF_PD(fprintf(stderr, "BACKEND: query_read_callback_data(%d)\n", fd));
  ASSURE_FDS_SIZE( fd );
  return fds[fd].read.data;
}

PMOD_EXPORT void *query_write_callback_data(int fd)
{
#ifdef PIKE_DEBUG
  if(fd<0)
    fatal("File descriptor out of range.\n %d",fd);
#endif
  IF_PD(fprintf(stderr, "BACKEND: query_write_callback_data(%d)\n", fd));
  ASSURE_FDS_SIZE( fd );
  return fds[fd].write.data;
}

#ifdef WITH_OOB
PMOD_EXPORT void *query_read_oob_callback_data(int fd)
{
#ifdef PIKE_DEBUG
  if(fd<0)
    fatal("File descriptor out of range.\n %d",fd);
#endif
  IF_PD(fprintf(stderr, "BACKEND: query_read_oob_callback_data(%d)\n", fd));
  ASSURE_FDS_SIZE( fd );
  return fds[fd].read_oob.data;
}

PMOD_EXPORT void *query_write_oob_callback_data(int fd)
{
#ifdef PIKE_DEBUG
  if(fd<0)
    fatal("File descriptor out of range.\n %d",fd);
#endif
  IF_PD(fprintf(stderr, "BACKEND: query_write_oob_callback_data(%d)\n", fd));
  ASSURE_FDS_SIZE( fd );
  return fds[fd].write_oob.data;
}
#endif /* WITH_OOB */

#ifdef PIKE_DEBUG

struct callback_list do_debug_callbacks;

void do_debug(void)
{
  int e;
  struct stat tmp;
  extern void check_all_arrays(void);
  extern void check_all_mappings(void);
  extern void check_all_programs(void);
  extern void check_all_objects(void);
  extern void verify_shared_strings_tables(void);
  extern void slow_check_stack(void);
  extern int do_gc(void);

  slow_check_stack();
  check_all_arrays();
  check_all_mappings();
  check_all_programs();
  check_all_objects();
  verify_shared_strings_tables();

  call_callback(& do_debug_callbacks, 0);

  /* FIXME: OOB? */
#ifndef HAVE_POLL
  for(e=0;e<=max_fd;e++)
  {
    if(my_FD_ISSET(e, &selectors.read) || my_FD_ISSET(e, &selectors.write)
#ifdef WITH_OOB
       || my_FD_ISSET(e, &selectors.except)
#endif /* WITH_OOB */
       )
    {
      int ret;
      do {
	ret = fd_fstat(e, &tmp);
      }while(ret < 0 && errno == EINTR);

      if(ret<0)
      {
	switch(errno)
	{
	  case EBADF:
	    fatal("Backend filedescriptor %d is bad.\n",e);
	    break;
	  case ENOENT:
	    fatal("Backend filedescriptor %d is not.\n",e);
	    break;
	}
      }
    }
  }
#else
  for(e=0;e<num_in_poll;e++)
  {
    int ret;
    do {
      ret=fd_fstat(poll_fds[e].fd, &tmp);
    }while(ret < 0 && errno == EINTR);

    if(ret<0)
    {
      switch(errno)
      {
       case EBADF:
	 fatal("Backend filedescriptor %ld is bad.\n", (long)poll_fds[e].fd);
	 break;
       case ENOENT:
	 fatal("Backend filedescriptor %ld is not.\n", (long)poll_fds[e].fd);
	 break;
      }
    }
  }
#endif

  if(d_flag>3) do_gc();
}
#endif

void backend(void)
{
  JMP_BUF back;
  int i, delay;
#ifndef HAVE_POLL
  fd_set rset;
  fd_set wset;
#ifdef WITH_OOB
  fd_set eset;
#endif /* WITH_OOB */
#endif /* !HAVE_POLL */

  if(SETJMP(back))
  {
    call_handle_error();
    throw_value.subtype=NUMBER_UNDEFINED;
    throw_value.u.integer=0;
  }

  while(first_object)
  {
    next_timeout.tv_usec = 0;
    next_timeout.tv_sec = 7 * 24 * 60 * 60;  /* See you in a week */
    my_add_timeval(&next_timeout, &current_time);

    may_need_wakeup=1;
    call_callback(& backend_callbacks, (void *)0);

    check_threads_etc();

#ifndef HAVE_POLL
    /* FIXME: OOB? */
    fd_copy_my_fd_set_to_fd_set(&rset, &selectors.read, max_fd+1);
    fd_copy_my_fd_set_to_fd_set(&wset, &selectors.write, max_fd+1);
#ifdef WITH_OOB
    fd_copy_my_fd_set_to_fd_set(&eset, &selectors.except, max_fd+1);
#endif /* WITH_OOB */
#else
    switch_poll_set();
#endif

    alloca(0);			/* Do garbage collect */
#ifdef PIKE_DEBUG
    if(d_flag > 1) do_debug();
#endif

#ifndef OWN_GETHRTIME
    GETTIMEOFDAY(&current_time);
#else
    /* good place to run the gethrtime-conversion update
       since we have to run gettimeofday anyway /Mirar */
    own_gethrtime_update(&current_time);
#endif

    if(my_timercmp(&next_timeout, > , &current_time))
    {
      my_subtract_timeval(&next_timeout, &current_time);
    }else{
      next_timeout.tv_usec = 0;
      next_timeout.tv_sec = 0;
    }

    THREADS_ALLOW();
#ifdef HAVE_POLL
    {
      int msec = (next_timeout.tv_sec*1000) + next_timeout.tv_usec/1000;
      IF_PD(fprintf(stderr, "BACKEND: poll(%p, %d, %ld)...",
		    active_poll_fds, active_num_in_poll, msec));
      i = poll(active_poll_fds, active_num_in_poll, msec);
      IF_PD(fprintf(stderr, " => %d\n", i));
    }
#else
    /* FIXME: OOB? */
    i = fd_select(max_fd+1, &rset, &wset, 
#ifdef WITH_OOB
		  &eset,
#else /* !WITH_OOB */
		  0, 
#endif /* WITH_OOB */
		  &next_timeout);
#endif
    THREADS_DISALLOW();
    may_need_wakeup=0;
    GETTIMEOFDAY(&current_time);

    if (!i) {
      /* Timeout */
    } else if (i>0) {
#ifdef PIKE_DEBUG
      int num_active = i;
#endif /* PIKE_DEBUG */
#ifndef HAVE_POLL
      /* FIXME: OOB? */
      for(i=0; i<max_fd+1; i++)
      {
#ifdef WITH_OOB
	if(fd_FD_ISSET(i, &eset) && fds[i].read_oob.callback)
	  (*(fds[i].read_oob.callback))(i, fds[i].read_oob.data);
#endif /* WITH_OOB */

	if(fd_FD_ISSET(i, &rset) && fds[i].read.callback)
	  (*(fds[i].read.callback))(i, fds[i].read.data);

	if(fd_FD_ISSET(i, &wset)) {
#ifdef WITH_OOB
	  if (fds[i].write_oob.callback) {
	    (*(fds[i].write_oob.callback))(i, fds[i].write_oob.data);
	  } else
#endif /* WITH_OOB */
	    if (fds[i].write.callback) {
	      (*(fds[i].write.callback))(i, fds[i].write.data);
	    }
	}
      }
#else
      for(i=0; i<active_num_in_poll; i++)
      {
	int fd = active_poll_fds[i].fd;
#ifdef PIKE_DEBUG
	int handled = 0;
#endif /* PIKE_DEBUG */
	if(active_poll_fds[i].revents & POLLNVAL)
	{
	  int j;
	  for(j=0;j<num_in_poll;j++)
	  {
	    if(poll_fds[j].fd == fd) /* It's still there... */
	    {
	      struct pollfd fds;
	      int ret;
	      fds.fd=fd;
	      fds.events=POLLIN;
	      fds.revents=0;
	      ret=poll(&fds, 1,1 );
	      if(fds.revents & POLLNVAL)
		fatal("Bad filedescriptor %d to poll().\n", fd);
	      break;
	    }
	  }
#ifdef PIKE_DEBUG
	  handled = 1;
#endif /* PIKE_DEBUG */
	}

#ifdef WITH_OOB
	if ((active_poll_fds[i].revents & POLLRDBAND) &&
	    fds[fd].read_oob.callback) {
	  IF_PD(fprintf(stderr, "BACKEND: POLLRDBAND\n"));
	  IF_PD(fprintf(stderr, "BACKEND: read_oob_callback(%d, %p)\n",
			fd, fds[fd].read_oob.data));
	  (*(fds[fd].read_oob.callback))(fd, fds[fd].read_oob.data);
#ifdef PIKE_DEBUG
	  handled = 1;
#endif /* PIKE_DEBUG */
	}
#endif /* WITH_OOB */

	if((active_poll_fds[i].revents & POLLHUP) ||
	   (active_poll_fds[i].revents & POLLERR)) {
	  /* Closed or error */
#ifdef PIKE_DEBUG
	  if (active_poll_fds[i].revents & POLLERR) {
	    fprintf(stderr, "Got POLLERR on fd %d\n", i);
	  }
#endif /* PIKE_DEBUG */
	  IF_PD(fprintf(stderr, "BACKEND: POLLHUP | POLLERR\n"));
	  /* We don't want to keep this fd anymore. */
	  POLL_FD_CLR(fd, ~0);
	  if (fds[fd].read.callback) {
	    IF_PD(fprintf(stderr, "BACKEND: read_callback(%d, %p)\n",
			  fd, fds[fd].read.data));
	    (*(fds[fd].read.callback))(fd,fds[fd].read.data);
	  } else if (fds[fd].write.callback) {
	    IF_PD(fprintf(stderr, "BACKEND: write_callback(%d, %p)\n",
			  fd, fds[fd].write.data));
	    (*(fds[fd].write.callback))(fd, fds[fd].write.data);
	  }
#ifdef PIKE_DEBUG
	  handled = 1;
#endif /* PIKE_DEBUG */
	}

	if(active_poll_fds[i].revents & (POLLRDNORM|POLLIN)) {
	  IF_PD(fprintf(stderr, "BACKEND: POLLRDNORM|POLLIN\n"));
	  if (fds[fd].read.callback) {
	    IF_PD(fprintf(stderr, "BACKEND: read_callback(%d, %p)\n",
			  fd, fds[fd].read.data));
	    (*(fds[fd].read.callback))(fd,fds[fd].read.data);
	  } else {
	    POLL_FD_CLR(fd, POLLRDNORM|POLLIN);
	  }
#ifdef PIKE_DEBUG
	  handled = 1;
#endif /* PIKE_DEBUG */
	}

#ifdef WITH_OOB
	if ((active_poll_fds[i].revents & POLLWRBAND) &&
	    fds[fd].write_oob.callback) {
	  IF_PD(fprintf(stderr, "BACKEND: POLLWRBAND\n"));
	  IF_PD(fprintf(stderr, "BACKEND: write_oob_callback(%d, %p)\n",
			fd, fds[fd].write_oob.data));
	  (*(fds[fd].write_oob.callback))(fd, fds[fd].write_oob.data);
#ifdef PIKE_DEBUG
	  handled = 1;
#endif /* PIKE_DEBUG */
	}
#endif /* WITH_OOB */

	if(active_poll_fds[i].revents & POLLOUT) {
	  IF_PD(fprintf(stderr, "BACKEND: POLLOUT\n"));
	  if (fds[fd].write.callback) {
	    IF_PD(fprintf(stderr, "BACKEND: write_callback(%d, %p)\n",
			  fd, fds[fd].write.data));
	    (*(fds[fd].write.callback))(fd, fds[fd].write.data);
	  } else {
	    POLL_FD_CLR(fd, POLLOUT);
	  }
#ifdef PIKE_DEBUG
	  handled = 1;
#endif /* PIKE_DEBUG */
	}
#ifdef PIKE_DEBUG
	num_active -= handled;
	if (!handled && active_poll_fds[i].revents) {
	  fprintf(stderr, "BACKEND: fd %ld has revents 0x%08lx, "
		  "but hasn't been handled.\n",
		  (long)active_poll_fds[i].fd,
		  (long)active_poll_fds[i].revents);
	}
#endif /* PIKE_DEBUG */
      }
#ifdef PIKE_DEBUG
      if (num_active) {
	fprintf(stderr, "BACKEND: %d more active fds than were handled.\n",
		num_active);
	for(i=0; i<active_num_in_poll; i++) {
	  fprintf(stderr, "BACKEND: fd %ld, events 0x%08lx, revents 0x%08lx\n",
		  (long)active_poll_fds[i].fd,
		  (long)active_poll_fds[i].events,
		  (long)active_poll_fds[i].revents);
	}
      }
#endif /* PIKE_DEBUG */
#endif
    }else{
      switch(errno)
      {
#ifdef __NT__
	default:
	  fatal("Error in backend %d\n",errno);
	  break;
#endif
	  
      case EINVAL:
	fatal("Invalid timeout to select().\n");
	break;

#ifdef WSAEINTR
      case WSAEINTR:
#endif
      case EINTR:		/* ignore */
	break;

#ifdef WSAEBADF
      case WSAEBADF:
#endif
#ifdef WSAENOTSOCK
      case WSAENOTSOCK:
#endif
      case EBADF:
	/* TODO: Fix poll version! */
#ifndef HAVE_POLL
	/* FIXME: OOB? */
	fd_copy_my_fd_set_to_fd_set(&rset, &selectors.read, max_fd+1);
	fd_copy_my_fd_set_to_fd_set(&wset, &selectors.write, max_fd+1);
#ifdef WITH_OOB
	fd_copy_my_fd_set_to_fd_set(&eset, &selectors.except, max_fd+1);
#endif /* WITH_OOB */
	next_timeout.tv_usec=0;
	next_timeout.tv_sec=0;
	if(fd_select(max_fd+1, &rset, &wset,
#ifdef WITH_OOB
		     &eset,
#else /* !WITH_OOB */
		     0,
#endif /* WITH_OOB */
		     &next_timeout) < 0)
	{
	  switch(errno)
	  {
#ifdef WSAEBADF
	    case WSAEBADF:
#endif
#ifdef WSAENOTSOCK
	    case WSAENOTSOCK:
#endif
	    case EBADF:
	    {
	      int i;
	      for(i=0;i<fds_size;i++)
	      {
		if(!my_FD_ISSET(i, &selectors.read) &&
		   !my_FD_ISSET(i, &selectors.write)
#ifdef WITH_OOB
		   && !my_FD_ISSET(i, &selectors.except)
#endif /* WITH_OOB */
		   )
		  continue;
		
		fd_FD_ZERO(& rset);
		fd_FD_ZERO(& wset);
#ifdef WITH_OOB
		fd_FD_ZERO(& eset);
#endif /* WITH_OOB */
		
		if(my_FD_ISSET(i, &selectors.read))  fd_FD_SET(i, &rset);
		if(my_FD_ISSET(i, &selectors.write)) fd_FD_SET(i, &wset);
#ifdef WITH_OOB
		if(my_FD_ISSET(i, &selectors.except)) fd_FD_SET(i, &eset);
#endif /* WITH_OOB */
		
		next_timeout.tv_usec=0;
		next_timeout.tv_sec=0;
		
		if(fd_select(max_fd+1, &rset, &wset,
#ifdef WITH_OOB
			     &eset,
#else /* !WITH_OOB */
			     0,
#endif /* WITH_OOB */
			     &next_timeout) < 0)
		{
		  switch(errno)
		  {
#ifdef __NT__
		    default:
#endif
		    case EBADF:
#ifdef WSAEBADF
		    case WSAEBADF:
#endif
#ifdef WSAENOTSOCK
		    case WSAENOTSOCK:
#endif
		      fatal("Filedescriptor %d (%s) caused fatal error %d in backend.\n",i,fd_info(i),errno);
		      
		    case EINTR:
		      break;
		  }
		}
	      }
	    }
	  }
#ifdef _REENTRANT
	  /* FIXME: Extra stderr messages should not be allowed.../Hubbe */
	  write_to_stderr("Bad filedescriptor to select().\n"
			  "fd closed in another thread?\n", 62);
#else /* !_REENTRANT */
	  fatal("Bad filedescriptor to select().\n");
#endif /* _REENTRANT */
	}
#endif
	break;

      }
    }
    call_callback(& backend_callbacks, (void *)(ptrdiff_t)1);
  }

  UNSETJMP(back);
}

PMOD_EXPORT int write_to_stderr(char *a, size_t len)
{
#ifdef __NT__
  size_t e;
  for(e=0;e<len;e++)
    putc(a[e],stderr);
#else
  int nonblock=0;
  size_t pos;
  int tmp;

  if(!len) return 1;

  for(pos=0;pos<len;pos+=tmp)
  {
    tmp=write(2,a+pos,len-pos);
    if(tmp<0)
    {
      tmp=0;
      switch(errno)
      {
#ifdef EWOULDBLOCK
	case EWOULDBLOCK:
	  nonblock=1;
	  set_nonblocking(2,0);
	  continue;
#endif

	case EINTR:
	  continue;
      }
      break;
    }
  }

  if(nonblock)
    set_nonblocking(2,1);

#endif
  return 1;
}

