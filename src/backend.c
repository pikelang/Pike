/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: backend.c,v 1.22 1998/03/10 03:14:51 per Exp $");
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
#include "error.h"
#include "fd_control.h"
#include "main.h"
#include "callback.h"
#include "threads.h"
#include "fdlib.h"

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <sys/stat.h>

#define SELECT_READ 1
#define SELECT_WRITE 2

file_callback read_callback[MAX_OPEN_FILEDESCRIPTORS];
void *read_callback_data[MAX_OPEN_FILEDESCRIPTORS];
file_callback write_callback[MAX_OPEN_FILEDESCRIPTORS];
void *write_callback_data[MAX_OPEN_FILEDESCRIPTORS];

#ifndef HAVE_POLL
struct selectors
{
  my_fd_set read;
  my_fd_set write;
};

static struct selectors selectors;

#else
#include <poll.h>
struct pollfd *poll_fds, *active_poll_fds;
int num_in_poll=0;

void POLL_FD_SET(int fd, short add)
{
  int i;
  for(i=0; i<num_in_poll; i++)
    if(poll_fds[i].fd == fd)
    {
      poll_fds[i].events |= add;
      return;
    }
  num_in_poll++;
  poll_fds = realloc(poll_fds, sizeof(struct pollfd)*num_in_poll);
  poll_fds[num_in_poll-1].fd = fd;
  poll_fds[num_in_poll-1].events = add;
}

void POLL_FD_CLR(int fd, short sub)
{
  int i;
  if(!poll_fds) return;
  for(i=0; i<num_in_poll; i++)
    if(poll_fds[i].fd == fd)
    {
      poll_fds[i].events &= ~sub;
      if(!poll_fds[i].events)
      {
	if(i!=num_in_poll-1)
	  MEMCPY(poll_fds+i, poll_fds+i+1,
		 (num_in_poll-1-i)*sizeof(struct pollfd));
	num_in_poll--;
      }
      break;
    }
  return;
}

int active_num_in_poll;
void switch_poll_set()
{
  struct pollfd *tmp = active_poll_fds;

  active_num_in_poll = num_in_poll;

  if(!num_in_poll) return;

  active_poll_fds = poll_fds;

  poll_fds = realloc(tmp, sizeof(struct pollfd)*num_in_poll);

  MEMCPY(poll_fds, active_poll_fds, sizeof(struct pollfd)*num_in_poll);
}
#endif


static int max_fd;
struct timeval current_time;
struct timeval next_timeout;
static int wakeup_pipe[2];
static int may_need_wakeup=0;

static struct callback_list backend_callbacks;

struct callback *add_backend_callback(callback_func call,
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
void wake_up_backend(void)
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
#endif
  if(pike_make_pipe(wakeup_pipe) < 0)
    fatal("Couldn't create backend wakup pipe! errno=%d.\n",errno);
  set_nonblocking(wakeup_pipe[0],1);
  set_nonblocking(wakeup_pipe[1],1);
  set_read_callback(wakeup_pipe[0], wakeup_callback, 0); 
  /* Don't keep these on exec! */
  set_close_on_exec(wakeup_pipe[0], 1);
  set_close_on_exec(wakeup_pipe[1], 1);
}

void set_read_callback(int fd,file_callback cb,void *data)
{
#ifdef HAVE_POLL
  int was_set = (read_callback[fd]!=0);
#endif
#ifdef DEBUG
  if(fd<0 || fd>=MAX_OPEN_FILEDESCRIPTORS)
    fatal("File descriptor out of range.\n %d",fd);
#endif
  read_callback[fd]=cb;
  read_callback_data[fd]=data;

  if(cb)
  {
#ifdef HAVE_POLL
    POLL_FD_SET(fd,POLLRDNORM);
#else
    my_FD_SET(fd, &selectors.read);
#endif
    if(max_fd < fd) max_fd = fd;
    wake_up_backend();
  }else{
#ifndef HAVE_POLL
    if(fd <= max_fd)
    {
      my_FD_CLR(fd, &selectors.read);

      if(fd == max_fd)
      {
	while(max_fd >=0 &&
	      !my_FD_ISSET(max_fd, &selectors.read) &&
	      !my_FD_ISSET(max_fd, &selectors.write))
	  max_fd--;
      }
    }
#else
    if(was_set)
      POLL_FD_CLR(fd,POLLRDNORM);
#endif
  }
}

void set_write_callback(int fd,file_callback cb,void *data)
{
#ifdef HAVE_POLL
  int was_set = (write_callback[fd]!=0);
#endif
#ifdef DEBUG
  if(fd<0 || fd>=MAX_OPEN_FILEDESCRIPTORS)
    fatal("File descriptor out of range.\n %d",fd);
#endif

  write_callback[fd]=cb;
  write_callback_data[fd]=data;

  if(cb)
  {
#ifdef HAVE_POLL
    POLL_FD_SET(fd,POLLOUT);
#else
    my_FD_SET(fd, &selectors.write);
#endif
    if(max_fd < fd) max_fd = fd;
    wake_up_backend();
  }else{
#ifndef HAVE_POLL
    if(fd <= max_fd)
    {
      my_FD_CLR(fd, &selectors.write);
      if(fd == max_fd)
      {
	while(max_fd >=0 &&
	      !my_FD_ISSET(max_fd, &selectors.read) &&
	      !my_FD_ISSET(max_fd, &selectors.write))
	  max_fd--;
      }
    }
#else
    if(was_set)
      POLL_FD_CLR(fd,POLLOUT);
#endif
  }
}

file_callback query_read_callback(int fd)
{
#ifdef DEBUG
  if(fd<0 || fd>=MAX_OPEN_FILEDESCRIPTORS)
    fatal("File descriptor out of range.\n %d",fd);
#endif

  return read_callback[fd];
}

file_callback query_write_callback(int fd)
{
#ifdef DEBUG
  if(fd<0 || fd>=MAX_OPEN_FILEDESCRIPTORS)
    fatal("File descriptor out of range.\n %d",fd);
#endif

  return write_callback[fd];
}

void *query_read_callback_data(int fd)
{
#ifdef DEBUG
  if(fd<0 || fd>=MAX_OPEN_FILEDESCRIPTORS)
    fatal("File descriptor out of range.\n %d",fd);
#endif

  return read_callback_data[fd];
}

void *query_write_callback_data(int fd)
{
#ifdef DEBUG
  if(fd<0 || fd>=MAX_OPEN_FILEDESCRIPTORS)
    fatal("File descriptor out of range.\n %d",fd);
#endif

  return write_callback_data[fd];
}

#ifdef DEBUG

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

  slow_check_stack();
  check_all_arrays();
  check_all_mappings();
  check_all_programs();
  verify_all_objects();
  verify_shared_strings_tables();

  call_callback(& do_debug_callbacks, 0);

#ifndef HAVE_POLL
  for(e=0;e<=max_fd;e++)
  {
    if(my_FD_ISSET(e,&selectors.read) || my_FD_ISSET(e,&selectors.write))
    {
      int ret;
      do {
	ret=fd_fstat(e, &tmp);
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
	 fatal("Backend filedescriptor %d is bad.\n",poll_fds[e].fd);
	 break;
       case ENOENT:
	 fatal("Backend filedescriptor %d is not.\n",poll_fds[e].fd);
	 break;
      }
    }
  }
#endif
}
#endif

void backend(void)
{
  JMP_BUF back;
  int i, delay;
  fd_set rset;
  fd_set wset;

  if(SETJMP(back))
  {
    ONERROR tmp;
    SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");
    assign_svalue_no_free(sp++, & throw_value);
    APPLY_MASTER("handle_error", 1);
    pop_stack();
    UNSET_ONERROR(tmp);
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
    fd_copy_my_fd_set_to_fd_set(&rset, &selectors.read, max_fd+1);
    fd_copy_my_fd_set_to_fd_set(&wset, &selectors.write, max_fd+1);
#endif

    alloca(0);			/* Do garbage collect */
#ifdef DEBUG
    if(d_flag > 1) do_debug();
#endif

    GETTIMEOFDAY(&current_time);

    if(my_timercmp(&next_timeout, > , &current_time))
    {
      my_subtract_timeval(&next_timeout, &current_time);
    }else{
      next_timeout.tv_usec = 0;
      next_timeout.tv_sec = 0;
    }
#ifdef HAVE_POLL
    switch_poll_set();
#endif
    THREADS_ALLOW();
#ifdef HAVE_POLL
    {
      int msec = (next_timeout.tv_sec*1000) + next_timeout.tv_usec/1000;
      i=poll(active_poll_fds, active_num_in_poll, msec);
    }
#else
    i=fd_select(max_fd+1, &rset, &wset, 0, &next_timeout);
#endif
    GETTIMEOFDAY(&current_time);
    THREADS_DISALLOW();
    may_need_wakeup=0;

    if(i>=0)
    {
#ifndef HAVE_POLL
      for(i=0; i<max_fd+1; i++)
      {
	if(fd_FD_ISSET(i, &rset) && read_callback[i])
	  (*(read_callback[i]))(i,read_callback_data[i]);

	if(fd_FD_ISSET(i, &wset) && write_callback[i])
	  (*(write_callback[i]))(i,write_callback_data[i]);
      }
#else
      for(i=0; i<active_num_in_poll; i++)
      {
	int fd = active_poll_fds[i].fd;
	if(active_poll_fds[i].revents & POLLNVAL)
	{
	  int j;
	  for(j=0;j<num_in_poll;j++)
	    if(poll_fds[j].fd == fd) /* It's still there... */
	      fatal("Bad filedescriptor %d to select().\n", fd);
	}
	if((active_poll_fds[i].revents & POLLHUP) ||
	   (active_poll_fds[i].revents & POLLERR))
	  active_poll_fds[i].revents |= POLLRDNORM;

	if((active_poll_fds[i].revents & POLLRDNORM)&& read_callback[fd])
	  (*(read_callback[fd]))(fd,read_callback_data[fd]);

	if((active_poll_fds[i].revents & POLLOUT)&& write_callback[fd])
	  (*(write_callback[fd]))(fd,write_callback_data[fd]);
      }
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
	fd_copy_my_fd_set_to_fd_set(&rset, &selectors.read, max_fd+1);
	fd_copy_my_fd_set_to_fd_set(&wset, &selectors.write, max_fd+1);
	next_timeout.tv_usec=0;
	next_timeout.tv_sec=0;
	if(fd_select(max_fd+1, &rset, &wset, 0, &next_timeout) < 0)
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
	      for(i=0;i<MAX_OPEN_FILEDESCRIPTORS;i++)
	      {
		if(!my_FD_ISSET(i, &selectors.read) && !my_FD_ISSET(i,&selectors.write))
		  continue;
		
		fd_FD_ZERO(& rset);
		fd_FD_ZERO(& wset);
		
		if(my_FD_ISSET(i, &selectors.read))  fd_FD_SET(i, &rset);
		if(my_FD_ISSET(i, &selectors.write)) fd_FD_SET(i, &wset);
		
		next_timeout.tv_usec=0;
		next_timeout.tv_sec=0;
		
		if(fd_select(max_fd+1, &rset, &wset, 0, &next_timeout) < 0)
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
    call_callback(& backend_callbacks, (void *)1);
  }

  UNSETJMP(back);
}

int write_to_stderr(char *a, INT32 len)
{
#ifdef __NT__
  int e;
  for(e=0;e<len;e++)
    putc(a[e],stderr);
#else
  int nonblock=0;
  INT32 pos, tmp;

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

