/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: backend.c,v 1.21 1998/01/21 20:05:32 hubbe Exp $");
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

struct selectors
{
  my_fd_set read;
  my_fd_set write;
};

static struct selectors selectors;

file_callback read_callback[MAX_OPEN_FILEDESCRIPTORS];
void *read_callback_data[MAX_OPEN_FILEDESCRIPTORS];
file_callback write_callback[MAX_OPEN_FILEDESCRIPTORS];
void *write_callback_data[MAX_OPEN_FILEDESCRIPTORS];

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
  my_FD_ZERO(&selectors.read);
  my_FD_ZERO(&selectors.write);
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
#ifdef DEBUG
  if(fd<0 || fd>=MAX_OPEN_FILEDESCRIPTORS)
    fatal("File descriptor out of range.\n %d",fd);
#endif

  read_callback[fd]=cb;
  read_callback_data[fd]=data;

  if(cb)
  {
    my_FD_SET(fd, &selectors.read);
    if(max_fd < fd) max_fd = fd;
    wake_up_backend();
  }else{
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
  }
}

void set_write_callback(int fd,file_callback cb,void *data)
{
#ifdef DEBUG
  if(fd<0 || fd>=MAX_OPEN_FILEDESCRIPTORS)
    fatal("File descriptor out of range.\n %d",fd);
#endif

  write_callback[fd]=cb;
  write_callback_data[fd]=data;

  if(cb)
  {
    my_FD_SET(fd, &selectors.write);
    if(max_fd < fd) max_fd = fd;
    wake_up_backend();
  }else{
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

    fd_copy_my_fd_set_to_fd_set(&rset, &selectors.read, max_fd+1);
    fd_copy_my_fd_set_to_fd_set(&wset, &selectors.write, max_fd+1);

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

    THREADS_ALLOW();
    i=fd_select(max_fd+1, &rset, &wset, 0, &next_timeout);
    GETTIMEOFDAY(&current_time);
    THREADS_DISALLOW();
    may_need_wakeup=0;

    if(i>=0)
    {
      for(i=0; i<max_fd+1; i++)
      {
	if(fd_FD_ISSET(i, &rset) && read_callback[i])
	  (*(read_callback[i]))(i,read_callback_data[i]);

	if(fd_FD_ISSET(i, &wset) && write_callback[i])
	  (*(write_callback[i]))(i,write_callback_data[i]);
      }
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

