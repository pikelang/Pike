/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include <errno.h>
#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/param.h>
#include <string.h>
#include "interpret.h"
#include "object.h"
#include "types.h"
#include "error.h"
#include "call_out.h"
#include "backend.h"
#include "fd_control.h"
#include "main.h"
#include "debug.h"
#include "callback.h"

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#define SELECT_READ 1
#define SELECT_WRITE 2

struct selectors
{
  fd_set read;
  fd_set write;
};

static struct selectors selectors;

callback read_callback[MAX_OPEN_FILEDESCRIPTORS];
void *read_callback_data[MAX_OPEN_FILEDESCRIPTORS];
callback write_callback[MAX_OPEN_FILEDESCRIPTORS];
void *write_callback_data[MAX_OPEN_FILEDESCRIPTORS];

static int max_fd;
time_t current_time;

static struct callback_list *backend_callbacks;

struct callback_list *add_backend_callback(struct array *a)
{
  return add_to_callback_list(&backend_callbacks, a);
}

void init_backend()
{
  FD_ZERO(&selectors.read);
  FD_ZERO(&selectors.write);
}

void set_read_callback(int fd,callback cb,void *data)
{
#ifdef DEBUG
  if(fd<0 || fd>=MAX_OPEN_FILEDESCRIPTORS)
    fatal("File descriptor out of range.\n %d",fd);
#endif

  read_callback[fd]=cb;
  read_callback_data[fd]=data;

  if(cb)
  {
    FD_SET(fd, &selectors.read);
    if(max_fd < fd) max_fd = fd;
  }else{
    if(fd <= max_fd)
    {
      FD_CLR(fd, &selectors.read);
      if(fd == max_fd)
      {
	while(max_fd >=0 &&
	      !FD_ISSET(max_fd, &selectors.read) &&
	      !FD_ISSET(max_fd, &selectors.write))
	  max_fd--;
      }
    }
  }
}

void set_write_callback(int fd,callback cb,void *data)
{
#ifdef DEBUG
  if(fd<0 || fd>=MAX_OPEN_FILEDESCRIPTORS)
    fatal("File descriptor out of range.\n %d",fd);
#endif

  write_callback[fd]=cb;
  write_callback_data[fd]=data;

  if(cb)
  {
    FD_SET(fd, &selectors.write);
    if(max_fd < fd) max_fd = fd;
  }else{
    if(fd <= max_fd)
    {
      FD_CLR(fd, &selectors.write);
      if(fd == max_fd)
      {
	while(max_fd >=0 &&
	      !FD_ISSET(max_fd, &selectors.read) &&
	      !FD_ISSET(max_fd, &selectors.write))
	  max_fd--;
      }
    }
  }
}

callback query_read_callback(int fd)
{
#ifdef DEBUG
  if(fd<0 || fd>=MAX_OPEN_FILEDESCRIPTORS)
    fatal("File descriptor out of range.\n %d",fd);
#endif

  return read_callback[fd];
}

callback query_write_callback(int fd)
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
void do_debug()
{
  extern void check_all_arrays();
  extern void check_all_mappings();
  extern void check_all_programs();
  extern void check_all_objects();
  extern void verify_shared_strings_tables();
  extern void slow_check_stack();

  slow_check_stack();
  check_all_arrays();
  check_all_mappings();
  check_all_programs();
  verify_all_objects();
  verify_shared_strings_tables();
  verify_all_call_outs();
}
#endif

void backend()
{
  JMP_BUF back;
  int i, delay;
  struct timeval timeout;
  struct selectors sets;

  if(SETJMP(back))
  {
    automatic_fatal="Error in handle_error in master object!\nPrevious error:";
    assign_svalue_no_free(sp++, & throw_value);
    APPLY_MASTER("handle_error", 1);
    pop_stack();
    automatic_fatal=0;
  }

  while(first_object)
  {
    delay = get_next_call_out();
    if(delay)
    {
      delay -= get_current_time();
      if(delay < 0) delay = 0;
    } else {
      delay = 7 * 24 * 60 * 60; /* See you in a week */
    }
    timeout.tv_usec = 0;
    timeout.tv_sec = delay;

    sets=selectors;

    i=select(max_fd+1, &sets.read, &sets.write, 0, &timeout);

    current_time = get_current_time();
    check_signals();

    if(i>=0)
    {
      for(i=0; i<max_fd+1; i++)
      {
	if(FD_ISSET(i, &sets.read) && read_callback[i])
	  (*(read_callback[i]))(i,read_callback_data[i]);

	if(FD_ISSET(i, &sets.write) && write_callback[i])
	  (*(write_callback[i]))(i,write_callback_data[i]);
      }
    }else{
      switch(errno)
      {
      case EINVAL:
	fatal("Invalid timeout to select().\n");
	break;

      case EINTR:		/* ignore */
	break;

      case EBADF:
	fatal("Bad filedescriptor to select().\n");
	break;

      }
    }

    do_call_outs();
    call_callback_list(& backend_callbacks);

    alloca(0);			/* Do garbage collect */
#ifdef DEBUG
    if(d_flag > 1)
      do_debug();
#endif
  }

  UNSETJMP(back);
}

int write_to_stderr(char *a, INT32 len)
{
  int nonblock;
  INT32 pos, tmp;

  if((nonblock=query_nonblocking(2)))
    set_nonblocking(2,0);

  for(pos=0;pos<len;pos+=tmp)
  {
    tmp=write(2,a+pos,len-pos);
    if(tmp<0) break;
  }
  if(nonblock)
    set_nonblocking(2,1);
  return 1;
}

