/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: file.c,v 1.351 2005/05/19 22:35:38 mast Exp $
*/

#define NO_PIKE_SHORTHAND
#include "global.h"
#include "fdlib.h"
#include "pike_netlib.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "mapping.h"
#include "object.h"
#include "pike_macros.h"
#include "backend.h"
#include "fd_control.h"
#include "module_support.h"
#include "operators.h"
#include "pike_security.h"
#include "bignum.h"
#include "builtin_functions.h"
#include "gc.h"

#include "file_machine.h"
#include "file.h"
#include "pike_error.h"
#include "signal_handler.h"
#include "pike_types.h"
#include "threads.h"
#include "program_id.h"

#ifdef HAVE_SYS_TYPE_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPE_H */

#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */
#include <errno.h>

#include <fcntl.h>
#include <signal.h>

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_LINUX_IF_H
#include <linux/if.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif /* HAVE_SYS_UIO_H */

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif /* HAVE_SYS_XATTR_H */

#if defined(HAVE_WINSOCK_H) || defined(HAVE_WINSOCK2_H)
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#ifndef EADDRINUSE
#define EADDRINUSE WSAEADDRINUSE
#endif
#endif

#ifdef HAVE_SYS_STREAM_H
#include <sys/stream.h>

/* Ugly patch for AIX 3.2 */
#ifdef u
#undef u
#endif

#endif

#ifdef HAVE_SYS_PROTOSW_H
#include <sys/protosw.h>
#endif

#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif

/* Fix warning on OSF/1
 *
 * NOERROR is defined by both sys/stream.h (-1), and arpa/nameser.h (0),
 * the latter is included by netdb.h.
 */
#ifdef NOERROR
#undef NOERROR
#endif /* NOERROR */

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_NET_NETDB_H
#include <net/netdb.h>
#endif /* HAVE_NET_NETDB_H */

#include "dmalloc.h"


#undef THIS
#define THIS ((struct my_file *)(Pike_fp->current_storage))
#define FD (THIS->box.fd)
#define ERRNO (THIS->my_errno)

#define READ_BUFFER 8192

/* Don't try to use socketpair() on AmigaOS, socketpair_ultra works better */
#ifdef __amigaos__
#undef HAVE_SOCKETPAIR
#endif

/* #define SOCKETPAIR_DEBUG */

struct program *file_program;
struct program *file_ref_program;

/*! @module Stdio
 */

/*! @class Fd_ref
 *!
 *! Proxy class that contains stub functions
 *! that call the corresponding functions in
 *! @[Fd].
 *!
 *! Used by @[File].
 */

/*! @decl Fd _fd
 *!  Object to which called functions are relayed.
 */

/*! @endclass
 */

/*! @class Fd
 *!
 *! Low level I/O operations. Use @[File] instead.
 */

/*! @endclass
 */

/* The class below is not accurate, but it's the lowest exposed API
 * interface, which make the functions appear where users actually
 * look for them. One could perhaps put them in Fd_ref and fix the doc
 * generator to show inherited functions in a better way, but if they
 * only are documented in Fd then the doc has to be duplicated in at
 * least Fd_ref. /mast */

/*! @class File
 */

static struct my_file *get_file_storage(struct object *o)
{
  struct my_file *f;
  struct object **ob;
  if(o->prog == file_program)
    return ((struct my_file *)(o->storage));

  if((f=(struct my_file *)get_storage(o,file_program)))
    return f;

  if((ob=(struct object **)get_storage(o,file_ref_program)))
     if(*ob && (f=(struct my_file *)get_storage(*ob, file_program)))
	return f;

  return 0;
}

#ifdef PIKE_DEBUG
static void debug_check_internals (struct my_file *f)
{
  size_t ev;

  if (f->box.ref_obj->prog && file_program &&
      !get_storage(f->box.ref_obj,file_program))
    Pike_fatal ("ref_obj is not a file object.\n");

  for (ev = 0; ev < NELEM (f->event_cbs); ev++)
    if (f->event_cbs[ev].type == PIKE_T_INT &&
	f->box.backend && f->box.events & (1 << ev))
      Pike_fatal ("Got event flag but no callback for event %"PRINTSIZET"d.\n", ev);
}
#else
#define debug_check_internals(f) do {} while (0)
#endif

#define ADD_FD_EVENTS(F, EVENTS) do {					\
    struct my_file *f_ = (F);						\
    if (!f_->box.backend)						\
      INIT_FD_CALLBACK_BOX (&f_->box, default_backend, f_->box.ref_obj,	\
			    f_->box.fd, (EVENTS), got_fd_event);	\
    else								\
      set_fd_callback_events (&f_->box, f_->box.events | (EVENTS));	\
  } while (0)

/* Note: The file object might be freed after this. */
#define SUB_FD_EVENTS(F, EVENTS) do {					\
    struct my_file *f_ = (F);						\
    if (f_->box.backend)						\
      set_fd_callback_events (&f_->box, f_->box.events & ~(EVENTS));	\
  } while (0)

static int got_fd_event (struct fd_callback_box *box, int event)
{
  struct my_file *f = (struct my_file *) box;
  struct svalue *cb = &f->event_cbs[event];

  f->my_errno = errno;		/* Propagate backend setting. */

  /* The event is turned on again in the read and write functions. */
  SUB_FD_EVENTS (f, 1 << event);

  check_destructed (cb);
  if (!UNSAFE_IS_ZERO (cb)) {
    apply_svalue (cb, 0);
    if (Pike_sp[-1].type == PIKE_T_INT && Pike_sp[-1].u.integer == -1) {
      pop_stack();
      return -1;
    }
    pop_stack();
  }
  return 0;
}

static void init_fd(int fd, int open_mode)
{
  size_t ev;
  FD=fd;
  ERRNO=0;
  THIS->box.backend = NULL;
  THIS->flags=0;
  THIS->open_mode=open_mode;
  for (ev = 0; ev < NELEM (THIS->event_cbs); ev++) {
    THIS->event_cbs[ev].type = PIKE_T_INT;
    THIS->event_cbs[ev].subtype = NUMBER_NUMBER;
    THIS->event_cbs[ev].u.integer = 0;
  }
#if defined(HAVE_FD_FLOCK) || defined(HAVE_FD_LOCKF)
  THIS->key=0;
#endif
#ifdef PIKE_DEBUG
  if (fd >= 0) debug_check_fd_not_in_use (fd);
#endif
}

static void free_fd_stuff(void)
{
  size_t ev;

#if defined(HAVE_FD_FLOCK) || defined(HAVE_FD_LOCKF)
  if(THIS->key)
  {
    destruct(THIS->key);
    THIS->key=0;
  }
#endif

  for (ev = 0; ev < NELEM (THIS->event_cbs); ev++) {
    free_svalue(& THIS->event_cbs[ev]);
    THIS->event_cbs[ev].type=PIKE_T_INT;
    THIS->event_cbs[ev].subtype = NUMBER_NUMBER;
    THIS->event_cbs[ev].u.integer = 0;
  }
}

static void close_fd_quietly(void)
{
  int fd=FD;
  if(fd<0) return;

  free_fd_stuff();
  change_fd_for_box (&THIS->box, -1);

  while(1)
  {
    int i, e;
    THREADS_ALLOW_UID();
    i=fd_close(fd);
    e=errno;
    THREADS_DISALLOW_UID();

    check_threads_etc();

    if(i < 0)
    {
      switch(e)
      {
	default: {
	  JMP_BUF jmp;
	  if (SETJMP (jmp))
	    call_handle_error();
	  else {
	    ERRNO=errno=e;
	    change_fd_for_box (&THIS->box, fd);
	    push_int(e);
	    f_strerror(1);
	    Pike_error("Failed to close file: %S\n", Pike_sp[-1].u.string);
	  }
	  UNSETJMP (jmp);
	  break;
	}

	case EBADF:
#ifdef SOLARIS
	  /* It's actually OK. This is a bug in Solaris 8. */
       case EAGAIN:
         break;
#endif

       case EINTR:
	 continue;
      }
    }
    break;
  }
}

static void close_fd(void)
{
  int fd=FD;
  if(fd<0) return;

  free_fd_stuff();
  change_fd_for_box (&THIS->box, -1);

  if ( (THIS->flags & FILE_NOT_OPENED) )
     return;

  while(1)
  {
    int i, e;
    THREADS_ALLOW_UID();
    i=fd_close(fd);
    e=errno;
    THREADS_DISALLOW_UID();

    check_threads_etc();

    if(i < 0)
    {
      switch(e)
      {
	default:
	  ERRNO=errno=e;
	  change_fd_for_box (&THIS->box, fd);
	  push_int(e);
	  f_strerror(1);
	  Pike_error("Failed to close file: %S\n", Pike_sp[-1].u.string);
	  break;

	case EBADF:
	  Pike_error("Internal error: Closing a non-active file descriptor %d.\n",fd);
	  break;

#ifdef SOLARIS
	  /* It's actually OK. This is a bug in Solaris 8. */
       case EAGAIN:
         break;
#endif

       case EINTR:
	 continue;
      }
    }
    break;
  }
}

void my_set_close_on_exec(int fd, int to)
{
  set_close_on_exec(fd, to);
}

void do_set_close_on_exec(void)
{
}

/* Parse "rw" to internal flags */
static int parse(char *a)
{
  int ret;
  ret=0;
  while(1)
  {
    switch(*(a++))
    {
    case 0: return ret;

    case 'r':
    case 'R':
      ret|=FILE_READ;
      break;

    case 'w':
    case 'W':
      ret|=FILE_WRITE;
      break;

    case 'a':
    case 'A':
      ret|=FILE_APPEND;
      break;

    case 'c':
    case 'C':
      ret|=FILE_CREATE;
      break;

    case 't':
    case 'T':
      ret|=FILE_TRUNC;
      break;

    case 'x':
    case 'X':
      ret|=FILE_EXCLUSIVE;
      break;
   }
  }
}

/* Translate internal flags to open(2) modes */
static int map(int flags)
{
  int ret;
  ret=0;
  switch(flags & (FILE_READ|FILE_WRITE))
  {
  case FILE_READ: ret=fd_RDONLY; break;
  case FILE_WRITE: ret=fd_WRONLY; break;
  case FILE_READ | FILE_WRITE: ret=fd_RDWR; break;
  }
  if(flags & FILE_APPEND) ret|=fd_APPEND;
  if(flags & FILE_CREATE) ret|=fd_CREAT;
  if(flags & FILE_TRUNC) ret|=fd_TRUNC;
  if(flags & FILE_EXCLUSIVE) ret|=fd_EXCL;
  ret |= fd_LARGEFILE;
  return ret;
}

static void free_dynamic_buffer(dynamic_buffer *b) { free(b->s.str); }

static struct pike_string *do_read(int fd,
				   INT32 r,
				   int all,
				   int *err)
{
  ONERROR ebuf;
  ptrdiff_t bytes_read, i;
  bytes_read=0;
  *err=0;

  if(r <= 65536)
  {
    struct pike_string *str;


    str=begin_shared_string(r);

    SET_ONERROR(ebuf, do_really_free_pike_string, str);

    do{
      int fd=FD;
      int e;
      THREADS_ALLOW();
      i = fd_read(fd, str->str+bytes_read, r);
      e=errno;
      THREADS_DISALLOW();

      check_threads_etc();

      if(i>0)
      {
	r-=i;
	bytes_read+=i;
	if(!all) break;
      }
      else if(i==0)
      {
	break;
      }
      else if(e != EINTR)
      {
	*err=e;
	if(!bytes_read)
	{
	  do_really_free_pike_string(str);
	  UNSET_ONERROR(ebuf);
	  return 0;
	}
	break;
      }
    }while(r);

    UNSET_ONERROR(ebuf);

    if(!SAFE_IS_ZERO(& THIS->event_cbs[PIKE_FD_READ]))
      ADD_FD_EVENTS (THIS, PIKE_BIT_FD_READ);

    if(bytes_read == str->len)
    {
      return end_shared_string(str);
    }else{
      return end_and_resize_shared_string(str, bytes_read);
    }

  }else{
    /* For some reason, 8k seems to work faster than 64k.
     * (4k seems to be about 2% faster than 8k when using linux though)
     * /Hubbe (Per pointed it out to me..)
     */
#define CHUNK ( 1024 * 8 )
    INT32 try_read;
    dynamic_buffer b;

    b.s.str=0;
    initialize_buf(&b);
    SET_ONERROR(ebuf, free_dynamic_buffer, &b);
    do{
      int e;
      char *buf;
      try_read=MINIMUM(CHUNK,r);

      buf = low_make_buf_space(try_read, &b);

      THREADS_ALLOW();
      i = fd_read(fd, buf, try_read);
      e=errno;
      THREADS_DISALLOW();

      check_threads_etc();

      if(i==try_read)
      {
	r-=i;
	bytes_read+=i;
	if(!all) break;
      }
      else if(i>0)
      {
	bytes_read+=i;
	r-=i;
	low_make_buf_space(i - try_read, &b);
	if(!all) break;
      }
      else if(i==0)
      {
	low_make_buf_space(-try_read, &b);
	break;
      }
      else
      {
	low_make_buf_space(-try_read, &b);
	if(e != EINTR)
	{
	  *err=e;
	  if(!bytes_read)
	  {
	    free(b.s.str);
	    UNSET_ONERROR(ebuf);
	    return 0;
	  }
	  break;
	}
      }
    }while(r);

    UNSET_ONERROR(ebuf);

    if(!SAFE_IS_ZERO(& THIS->event_cbs[PIKE_FD_READ]))
      ADD_FD_EVENTS (THIS, PIKE_BIT_FD_READ);

    return low_free_buf(&b);
#undef CHUNK
  }
}

static struct pike_string *do_read_oob(int fd,
				       INT32 r,
				       int all,
				       int *err)
{
  ONERROR ebuf;
  INT32 bytes_read,i;
  struct pike_string *str;

  bytes_read=0;
  *err=0;

    str=begin_shared_string(r);

    SET_ONERROR(ebuf, do_really_free_pike_string, str);

    do{
      int e;
      int fd=FD;
      THREADS_ALLOW();
      i=fd_recv(fd, str->str+bytes_read, r, MSG_OOB);
      e=errno;
      THREADS_DISALLOW();

      check_threads_etc();

      if(i>0)
      {
	r-=i;
	bytes_read+=i;
	if(!all) break;
      }
      else if(i==0)
      {
	break;
      }
      else if(e != EINTR)
      {
	*err=e;
	if(!bytes_read)
	{
	  do_really_free_pike_string(str);
	  UNSET_ONERROR(ebuf);
	  return 0;
	}
	break;
      }
    }while(r);

    UNSET_ONERROR(ebuf);

    if(!SAFE_IS_ZERO(& THIS->event_cbs[PIKE_FD_READ_OOB]))
      ADD_FD_EVENTS (THIS, PIKE_BIT_FD_READ_OOB);

    if(bytes_read == str->len)
    {
      return end_shared_string(str);
    }else{
      return end_and_resize_shared_string(str, bytes_read);
    }
}

/*! @decl string read()
 *! @decl string read(int len)
 *! @decl string read(int len, int(0..1) not_all)
 *!
 *! Read data from a file or a stream.
 *!
 *! Attempts to read @[len] bytes from the file, and return it as a
 *! string. Less than @[len] bytes can be returned if:
 *!
 *! @ul
 *!   @item
 *!     end-of-file is encountered for a normal file, or
 *!   @item
 *!     it's a stream that has been closed from the other end, or
 *!   @item
 *!     it's a stream in nonblocking mode, or
 *!   @item
 *!     it's a stream and @[not_all] is set, or
 *!   @item
 *!     @[not_all] isn't set and an error occurred (see below).
 *! @endul
 *!
 *! If @[not_all] is nonzero, @[read()] does not try its best to read
 *! as many bytes as you have asked for, but merely returns as much as
 *! the system read function returns. This is mainly useful with
 *! stream devices which can return exactly one row or packet at a
 *! time. If @[not_all] is used in blocking mode, @[read()] only
 *! blocks if there's no data at all available.
 *!
 *! If something goes wrong and @[not_all] is set, zero is returned.
 *! If something goes wrong and @[not_all] is zero or left out, then
 *! either zero or a string shorter than @[len] is returned. If the
 *! problem persists then a later call to @[read()] fails and returns
 *! zero, however.
 *!
 *! If everything went fine, a call to @[errno()] directly afterwards
 *! returns zero. That includes an end due to end-of-file or remote
 *! close.
 *!
 *! If no arguments are given, @[read()] reads to the end of the file
 *! or stream.
 *!
 *! @note
 *! It's not necessary to set @[not_all] to avoid blocking reading
 *! when nonblocking mode is used.
 *!
 *! @note
 *! When at the end of a file or stream, repeated calls to @[read()]
 *! returns the empty string since it's not considered an error. The
 *! empty string is never returned in other cases, unless nonblocking
 *! mode is used or @[len] is zero.
 *!
 *! @seealso
 *!   @[read_oob()], @[write()]
 */
static void file_read(INT32 args)
{
  struct pike_string *tmp;
  INT32 all, len;

  if(FD < 0)
    Pike_error("File not open.\n");

  if(!args)
  {
    len=0x7fffffff;
  }
  else
  {
    if(Pike_sp[-args].type != PIKE_T_INT)
      SIMPLE_BAD_ARG_ERROR("Stdio.File->read()", 1, "int");
    len=Pike_sp[-args].u.integer;
    if(len<0)
      Pike_error("Cannot read negative number of characters.\n");
  }

  if(args > 1 && !UNSAFE_IS_ZERO(Pike_sp+1-args))
  {
    all=0;
  }else{
    all=1;
  }

  pop_n_elems(args);

  if((tmp=do_read(FD, len, all, & ERRNO)))
    push_string(tmp);
  else {
    errno = ERRNO;
    push_int(0);
  }

  /* Race: A backend in another thread might have managed to set these
   * again for something that arrived after the read above. Not that
   * bad - it will get through in a later backend round. */
  THIS->box.revents &= ~(PIKE_BIT_FD_READ|PIKE_BIT_FD_READ_OOB);
}

#ifdef HAVE_AND_USE_POLL
#ifdef HAVE_POLL_H
#include <poll.h>
#else /* !HAVE_POLL_H */
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#else /* !HAVE_SYS_POLL_H */
#undef HAVE_AND_USE_POLL
#endif /* HAVE_SYS_POLL_H */
#endif /* HAVE_POLL_H */
#else /* HAVE_AND_USE_POLL */

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#endif

#ifndef __NT__
/*! @decl int(-1..1) peek()
 *! @decl int(-1..1) peek(int|float timeout)
 *!
 *! Check if there is data available to read,
 *! or wait some time for available data to read.
 *!
 *! Returns @expr{1@} if there is data available to read,
 *! @expr{0@} (zero) if there is no data available, and
 *! @expr{-1@} if something went wrong.
 *!
 *! @seealso
 *!   @[errno()], @[read()]
 *!
 *! @note
 *!    The function may be interrupted prematurely
 *!    of the timeout (due to signals); 
 *!    check the timing manually if this is imporant.
 */
static void file_peek(INT32 args)
{
#ifdef HAVE_AND_USE_POLL
  struct pollfd fds;
  int ret;
  int timeout=1;

  fds.fd=FD;
  fds.events=POLLIN;
  fds.revents=0;

  if (args)
  {
     FLOAT_TYPE tf;
     get_all_args("peek",args,"%F",&tf);
     timeout=(int)(tf*1000); /* ignore overflow for now */
  }

  THREADS_ALLOW();
  ret=poll(&fds, 1, timeout);
  THREADS_DISALLOW();

  if(ret < 0)
  {
    ERRNO=errno;
    ret=-1;
  }else{
    /* FIXME: What about POLLHUP and POLLERR? */
    ret = (ret > 0) && (fds.revents & POLLIN);
  }
#else
  int ret;
  fd_set tmp;
  struct timeval tv;

  tv.tv_usec=1;
  tv.tv_sec=0;
  fd_FD_ZERO(&tmp);
  fd_FD_SET(FD, &tmp);
  ret = FD;

  if (args)
  {
     FLOAT_TYPE tf;
     get_all_args("peek",args,"%F",&tf);
     tv.tv_sec=(int)tf;
     tv.tv_usec=(int)(1000000*(tf-tv.tv_sec));
  }

  THREADS_ALLOW();
  ret=select(ret+1,&tmp,0,0,&tv);
  THREADS_DISALLOW();

  if(ret < 0)
  {
    ERRNO=errno;
    ret=-1;
  }else{
    ret = (ret > 0) && fd_FD_ISSET(FD, &tmp);
  }
#endif
  pop_n_elems(args);
  push_int(ret);
}

/* NOTE: Some versions of AIX seem to have a
 *         #define events reqevents
 *       in one of the poll headerfiles. This will break
 *       the fd_box event handling.
 */
#undef events

#endif

/*! @decl string read_oob()
 *! @decl string read_oob(int len)
 *! @decl string read_oob(int len, int(0..1) not_all)
 *!
 *! Attempts to read @[len] bytes of out-of-band data from the stream,
 *! and returns it as a string. Less than @[len] bytes can be returned
 *! if:
 *!
 *! @ul
 *!   @item
 *!     the stream has been closed from the other end, or
 *!   @item
 *!     nonblocking mode is used, or
 *!   @item
 *!     @[not_all] is set, or
 *!   @item
 *!     @[not_all] isn't set and an error occurred (see below).
 *! @endul
 *!
 *! If @[not_all] is nonzero, @[read_oob()] only returns as many bytes
 *! of out-of-band data as are currently available.
 *!
 *! If something goes wrong and @[not_all] is set, zero is returned.
 *! If something goes wrong and @[not_all] is zero or left out, then
 *! either zero or a string shorter than @[len] is returned. If the
 *! problem persists then a later call to @[read_oob()] fails and
 *! returns zero, however.
 *!
 *! If everything went fine, a call to @[errno()] directly afterwards
 *! returns zero. That includes an end due to remote close.
 *!
 *! If no arguments are given, @[read_oob()] reads to the end of the
 *! stream.
 *!
 *! @note
 *!   Out-of-band data was not supported in Pike 0.5 and earlier, and
 *!   not in Pike 0.6 through 7.4 if they were compiled with the
 *!   option @tt{'--without-oob'@}.
 *!
 *! @note
 *!   It is not guaranteed that all out-of-band data sent from the
 *!   other end is received. Most streams only allow for a single byte
 *!   of out-of-band data at a time.
 *!
 *! @note
 *! It's not necessary to set @[not_all] to avoid blocking reading
 *! when nonblocking mode is used.
 *!
 *! @note
 *! When at the end of a file or stream, repeated calls to @[read()]
 *! returns the empty string since it's not considered an error. The
 *! empty string is never returned in other cases, unless nonblocking
 *! mode is used or @[len] is zero.
 *!
 *! @seealso
 *!   @[read()], @[write_oob()]
 */
static void file_read_oob(INT32 args)
{
  struct pike_string *tmp;
  INT32 all, len;

  if(FD < 0)
    Pike_error("File not open.\n");

  if(!args)
  {
    len=0x7fffffff;
  }
  else
  {
    if(Pike_sp[-args].type != PIKE_T_INT)
      SIMPLE_BAD_ARG_ERROR("Stdio.File->read_oob", 1, "int");
    len=Pike_sp[-args].u.integer;
    if(len<0)
      Pike_error("Cannot read negative number of characters.\n");
  }

  if(args > 1 && !UNSAFE_IS_ZERO(Pike_sp+1-args))
  {
    all=0;
  }else{
    all=1;
  }

  pop_n_elems(args);

  if((tmp=do_read_oob(FD, len, all, & ERRNO)))
    push_string(tmp);
  else {
    errno = ERRNO;
    push_int(0);
  }

  /* Race: A backend in another thread might have managed to set these
   * again for something that arrived after the read above. Not that
   * bad - it will get through in a later backend round. */
  THIS->box.revents &= ~(PIKE_BIT_FD_READ|PIKE_BIT_FD_READ_OOB);
}

static void set_fd_event_cb (struct my_file *f, struct svalue *cb, int event)
{
  if (UNSAFE_IS_ZERO (cb)) {
    free_svalue (&f->event_cbs[event]);
    f->event_cbs[event].type = PIKE_T_INT;
    f->event_cbs[event].subtype = NUMBER_NUMBER;
    f->event_cbs[event].u.integer = 0;
    SUB_FD_EVENTS (f, 1 << event);
  }
  else {
    assign_svalue (&f->event_cbs[event], cb);
    ADD_FD_EVENTS (f, 1 << event);
  }
}

#undef CBFUNCS
#define CBFUNCS(CB, EVENT)						\
  static void PIKE_CONCAT(file_set_,CB) (INT32 args)			\
  {									\
    if(!args)								\
      SIMPLE_TOO_FEW_ARGS_ERROR("Stdio.File->set_" #CB, 1);		\
    set_fd_event_cb (THIS, Pike_sp-args, EVENT);			\
  }									\
									\
  static void PIKE_CONCAT(file_query_,CB) (INT32 args)			\
  {									\
    pop_n_elems(args);							\
    push_svalue(& THIS->event_cbs[EVENT]);				\
  }

CBFUNCS(read_callback, PIKE_FD_READ)
CBFUNCS(write_callback, PIKE_FD_WRITE)
CBFUNCS(read_oob_callback, PIKE_FD_READ_OOB)
CBFUNCS(write_oob_callback, PIKE_FD_WRITE_OOB)

static void file__enable_callbacks(INT32 args)
{
  struct my_file *f = THIS;
  size_t ev;
  int cb_events = 0;

  if(FD<0)
    Pike_error("File is not open.\n");

  debug_check_internals (f);

  for (ev = 0; ev < NELEM (f->event_cbs); ev++)
    if (!UNSAFE_IS_ZERO (&f->event_cbs[ev]))
      cb_events |= 1 << ev;

  if (cb_events) ADD_FD_EVENTS (f, cb_events);

  pop_n_elems(args);
  push_int(0);
}

static void file__disable_callbacks(INT32 args)
{
  struct my_file *f = THIS;

  if(FD<0)
    Pike_error("File is not open.\n");

  SUB_FD_EVENTS (f, ~0);

  pop_n_elems(args);
  push_int(0);
}


/*! @decl int write(string data)
 *! @decl int write(string format, mixed ... extras)
 *! @decl int write(array(string) data)
 *! @decl int write(array(string) format, mixed ... extras)
 *!
 *! Write data to a file or a stream.
 *!
 *! Writes @[data] and returns the number of bytes that were
 *! actually written. It can be less than the size of the given data if
 *!
 *! @ul
 *!   @item
 *!     some data was written successfully and then something went
 *!     wrong, or
 *!   @item
 *!     nonblocking mode is used and not all data could be written
 *!     without blocking.
 *! @endul
 *!
 *! -1 is returned if something went wrong and no bytes were written.
 *! If only some data was written due to an error and that error
 *! persists, then a later call to @[write()] fails and returns -1.
 *!
 *! If everything went fine, a call to @[errno()] directly afterwards
 *! returns zero.
 *!
 *! If @[data] is an array of strings, they are written in sequence.
 *!
 *! If more than one argument is given, @[sprintf()] is used to format
 *! them using @[format]. If @[format] is an array, the strings in it
 *! are concatenated and the result is used as format string.
 *!
 *! @note
 *!   Writing of wide strings is not supported. You have to encode the
 *!   data somehow, e.g. with @[string_to_utf8] or with one of the
 *!   charsets supported by @[Locale.Charset.encoder].
 *!
 *! @seealso
 *!   @[read()], @[write_oob()]
 */
static void file_write(INT32 args)
{
  ptrdiff_t written, i;
  struct pike_string *str;

  if(args<1 || ((Pike_sp[-args].type != PIKE_T_STRING) &&
		(Pike_sp[-args].type != PIKE_T_ARRAY)))
    SIMPLE_BAD_ARG_ERROR("Stdio.File->write()", 1, "string|array(string)");

  if(FD < 0)
    Pike_error("File not open for write.\n");

  if (Pike_sp[-args].type == PIKE_T_ARRAY) {
    struct array *a = Pike_sp[-args].u.array;

    if( (a->type_field & ~BIT_STRING) &&
	(array_fix_type_field(a) & ~BIT_STRING) )
      SIMPLE_BAD_ARG_ERROR("Stdio.File->write()", 1, "string|array(string)");

    i = a->size;
    while(i--)
      if (a->item[i].u.string->size_shift)
	Pike_error("Bad argument 1 to file->write().\n"
		   "Element %ld is a wide string.\n",
		   DO_NOT_WARN((long)i));

#ifdef HAVE_WRITEV
    if (args > 1) {
#endif /* HAVE_WRITEV */
      ref_push_array(a);
      push_empty_string();
      o_multiply();
      Pike_sp--;
      dmalloc_touch_svalue(Pike_sp);
      Pike_sp[-args] = *Pike_sp;
      free_array(a);

#ifdef PIKE_DEBUG
      if (Pike_sp[-args].type != PIKE_T_STRING) {
	Pike_error("Bad return value from string multiplication.\n");
      }
#endif /* PIKE_DEBUG */
#ifdef HAVE_WRITEV
    } else if (!a->size) {
      /* Special case for empty array */
      ERRNO = 0;
      pop_stack();
      push_int(0);
      return;
    } else {
      struct iovec *iovbase =
	(struct iovec *)xalloc(sizeof(struct iovec)*a->size);
      struct iovec *iov = iovbase;
      int iovcnt = a->size;

      i = a->size;
      while(i--) {
	if (a->item[i].u.string->len) {
	  iov[i].iov_base = a->item[i].u.string->str;
	  iov[i].iov_len = a->item[i].u.string->len;
	} else {
	  iov++;
	  iovcnt--;
	}
      }

      for(written = 0; iovcnt; check_signals(0,0,0)) {
	int fd = FD;
	int e;
	int cnt = iovcnt;
	THREADS_ALLOW();

#ifdef IOV_MAX
	if (cnt > IOV_MAX) cnt = IOV_MAX;
#endif

#ifdef MAX_IOVEC
	if (cnt > MAX_IOVEC) cnt = MAX_IOVEC;
#endif
	i = writev(fd, iov, cnt);
	THREADS_DISALLOW();

	/* fprintf(stderr, "writev(%d, 0x%08x, %d) => %d\n",
	   fd, (unsigned int)iov, cnt, i); */

	e=errno; /* check_threads_etc may effect errno */
	check_threads_etc();

#ifdef _REENTRANT
	if (FD<0) {
	  free(iovbase);
	  Pike_error("File destructed while in file->write.\n");
	}
#endif
	if(i<0)
	{
	  /* perror("writev"); */
	  switch(e)
	  {
	  default:
	    free(iovbase);
	    ERRNO=errno=e;
	    pop_n_elems(args);
	    if (!written) {
	      push_int(-1);
	    } else {
	      push_int(written);
	    }
	    return;

	  case EINTR: continue;
	  case EWOULDBLOCK: break;
	  }
	  break;
	}else{
	  written += i;

	  /* Avoid extra writev() */
	  if(THIS->open_mode & FILE_NONBLOCKING)
	    break;

	  while(i) {
	    if ((ptrdiff_t)iov->iov_len <= i) {
	      i -= iov->iov_len;
	      iov++;
	      iovcnt--;
	    } else {
	      /* Use cast since iov_base might be a void pointer */
	      iov->iov_base = ((char *) iov->iov_base) + i;
	      iov->iov_len -= i;
	      i = 0;
	    }
	  }
	}
      }

      free(iovbase);

      if(!SAFE_IS_ZERO(& THIS->event_cbs[PIKE_FD_WRITE]))
	ADD_FD_EVENTS (THIS, PIKE_BIT_FD_WRITE);
      ERRNO=0;

      pop_stack();
      push_int(written);
      return;
    }
#endif /* HAVE_WRITEV */
  }

  /* At this point Pike_sp[-args].type is PIKE_T_STRING */

  if(args > 1)
  {
    f_sprintf(args);
    args=1;
  }

  str=Pike_sp[-args].u.string;
  if(str->size_shift)
    Pike_error("Stdio.File->write(): cannot output wide strings.\n");

  for(written=0;written < str->len;check_signals(0,0,0))
  {
    int fd=FD;
    int e;
    THREADS_ALLOW();
    i=fd_write(fd, str->str + written, str->len - written);
    e=errno;
    THREADS_DISALLOW();

    check_threads_etc();

#ifdef _REENTRANT
    if(FD<0) Pike_error("File destructed while in file->write.\n");
#endif

    if(i<0)
    {
      switch(e)
      {
      default:
	ERRNO=errno=e;
	pop_n_elems(args);
	if (!written) {
	  push_int(-1);
	} else {
	  push_int64(written);
	}
	/* Minor race - see below. */
	THIS->box.revents &= ~(PIKE_BIT_FD_WRITE|PIKE_BIT_FD_WRITE_OOB);
	return;

      case EINTR: continue;
      case EWOULDBLOCK: break;
      }
      break;
    }else{
      written+=i;

      /* Avoid extra write() */
      if(THIS->open_mode & FILE_NONBLOCKING)
	break;
    }
  }

  /* Race: A backend in another thread might have managed to set these
   * again for buffer space available after the write above. Not that
   * bad - it will get through in a later backend round. */
  THIS->box.revents &= ~(PIKE_BIT_FD_WRITE|PIKE_BIT_FD_WRITE_OOB);

  if(!SAFE_IS_ZERO(& THIS->event_cbs[PIKE_FD_WRITE]))
    ADD_FD_EVENTS (THIS, PIKE_BIT_FD_WRITE);
  ERRNO=0;

  pop_n_elems(args);
  push_int64(written);
}

/*! @decl int write_oob(string data)
 *! @decl int write_oob(string format, mixed ... extras)
 *!
 *! Write out-of-band data to a stream.
 *!
 *! Writes out-of-band data to a stream and returns how many bytes
 *! that were actually written. It can be less than the size of the
 *! given data if some data was written successfully and then
 *! something went wrong.
 *!
 *! -1 is returned if something went wrong and no bytes were written.
 *! If only some data was written due to an error and that error
 *! persists, then a later call to @[write_oob()] fails and returns
 *! -1.
 *!
 *! If everything went fine, a call to @[errno()] directly afterwards
 *! returns zero.
 *!
 *! If more than one argument is given, @[sprintf()] is used to format
 *! them.
 *!
 *! @note
 *!   Out-of-band data was not supported in Pike 0.5 and earlier, and
 *!   not in Pike 0.6 through 7.4 if they were compiled with the
 *!   option @tt{'--without-oob'@}.
 *!
 *! @note
 *!   It is not guaranteed that all out-of-band data sent from the
 *!   other end is received. Most streams only allow for a single byte
 *!   of out-of-band data at a time. Some streams sends the rest of
 *!   the data as ordinary data.
 *!
 *! @seealso
 *!   @[read_oob()], @[write()]
 */
static void file_write_oob(INT32 args)
{
  ptrdiff_t written, i;
  struct pike_string *str;

  if(args<1 || Pike_sp[-args].type != PIKE_T_STRING)
    SIMPLE_BAD_ARG_ERROR("Stdio.File->write_oob()",1,"string");

  if(args > 1)
  {
    f_sprintf(args);
    args=1;
  }

  if(FD < 0)
    Pike_error("File not open for write_oob.\n");

  written=0;
  str=Pike_sp[-args].u.string;
  if(str->size_shift)
    Pike_error("Stdio.File->write_oob(): cannot output wide strings.\n");

  while(written < str->len)
  {
    int fd=FD;
    int e;
    THREADS_ALLOW();
    i = fd_send(fd, str->str + written, str->len - written, MSG_OOB);
    e=errno;
    THREADS_DISALLOW();

    check_threads_etc();

#ifdef _REENTRANT
    if(FD<0) Pike_error("File destructed while in file->write_oob.\n");
#endif

    if(i<0)
    {
      switch(e)
      {
      default:
	ERRNO=errno=e;
	pop_n_elems(args);
	if (!written) {
	  push_int(-1);
	} else {
	  push_int64(written);
	}
	/* Minor race - see below. */
	THIS->box.revents &= ~(PIKE_BIT_FD_WRITE|PIKE_BIT_FD_WRITE_OOB);
	return;

      case EINTR: continue;
      case EWOULDBLOCK: break;
      }
      break;
    }else{
      written+=i;

      /* Avoid extra write() */
      if(THIS->open_mode & FILE_NONBLOCKING)
	break;
    }
  }

  /* Race: A backend in another thread might have managed to set these
   * again for buffer space available after the write above. Not that
   * bad - it will get through in a later backend round. */
  THIS->box.revents &= ~(PIKE_BIT_FD_WRITE|PIKE_BIT_FD_WRITE_OOB);

  if(!SAFE_IS_ZERO(& THIS->event_cbs[PIKE_FD_WRITE_OOB]))
    ADD_FD_EVENTS (THIS, PIKE_BIT_FD_WRITE_OOB);
  ERRNO=0;

  pop_n_elems(args);
  push_int64(written);
}

static int do_close(int flags)
{
  struct my_file *f = THIS;
  if(FD == -1) return 1; /* already closed */
  ERRNO=0;

  flags &= f->open_mode;

  switch(flags & (FILE_READ | FILE_WRITE))
  {
  case 0:
    return 0;

  case FILE_READ:
    if(f->open_mode & FILE_WRITE)
    {
      SUB_FD_EVENTS (f, PIKE_BIT_FD_READ|PIKE_BIT_FD_READ_OOB);
      fd_shutdown(FD, 0);
      f->open_mode &=~ FILE_READ;
      return 0;
    }else{
      f->flags&=~FILE_NOT_OPENED;
      close_fd();
      return 1;
    }

  case FILE_WRITE:
    if(f->open_mode & FILE_READ)
    {
      SUB_FD_EVENTS (f, PIKE_BIT_FD_WRITE|PIKE_BIT_FD_WRITE_OOB);
      fd_shutdown(FD, 1);
      f->open_mode &=~ FILE_WRITE;
      return 0;
    }else{
      f->flags&=~FILE_NOT_OPENED;
      close_fd();
      return 1;
    }

  case FILE_READ | FILE_WRITE:
    f->flags&=~FILE_NOT_OPENED;
    close_fd();
    return 1;

  default:
    Pike_fatal("Bug in switch implementation!\n");
    return 0; /* Make CC happy */
  }
}

/*! @decl string grantpt()
 *!
 *!  If this file has been created by calling @[openpt()], return the
 *!  filename of the associated pts-file. This function should only be
 *!  called once.
 *!
 *!  @note
 *!    This function is only available on some platforms.
 */
#if defined(HAVE_GRANTPT) || defined(USE_PT_CHMOD) || defined(USE_CHGPT)
static void file_grantpt( INT32 args )
{
  pop_n_elems(args);
#if defined(USE_PT_CHMOD) || defined(USE_CHGPT)
  push_constant_text("Process.Process");
  APPLY_MASTER("resolv", 1);

#ifdef USE_PT_CHMOD
  /* pt_chmod wants to get the fd number as the first argument. */
  push_constant_text(USE_PT_CHMOD);
  push_constant_text("4");
  f_aggregate(2);

  /* Send the pty as both fd 3 and fd 4. */
  push_constant_text("fds");
  ref_push_object(Pike_fp->current_object);
  ref_push_object(Pike_fp->current_object);
  f_aggregate(2);
  f_aggregate_mapping(2);
#else /* USE_CHGPT */
  /* chgpt on HPUX doesn't like getting any arguments... */
  push_constant_text(USE_CHGPT);
  f_aggregate(1);

  /* chgpt wants to get the pty on fd 0. */
  push_constant_text("stdin");
  ref_push_object(Pike_fp->current_object);
  f_aggregate_mapping(2);
#endif /* USE_PT_CHMOD */

  apply_svalue(Pike_sp-3, 2);
  apply(Pike_sp[-1].u.object, "wait", 0);
  if(!UNSAFE_IS_ZERO(Pike_sp-1)) {
    Pike_error(
#ifdef USE_PT_CHMOD
	       USE_PT_CHMOD
#else /* USE_CHGPT */
	       USE_CHGPT
#endif /* USE_PT_CHMOD */
	       " returned error %d.\n", Pike_sp[-1].u.integer);
  }
  pop_n_elems(3);
#else /* HAVE_GRANTPT */
  if( grantpt( FD ) )
    Pike_error("grantpt failed: %s\n", strerror(errno));
#endif /* USE_PT_CHMOD || USE_CHGPT */
  push_text( ptsname( FD ) );
#ifdef HAVE_UNLOCKPT
  if( unlockpt( FD ) )
    Pike_error("unlockpt failed: %s\n", strerror(errno));
#endif
}
#endif /* HAVE_GRANTPT || USE_PT_CHMOD || USE_CHGPT */

/*! @decl int close()
 *! @decl int close(string direction)
 *!
 *! Close a file or stream.
 *!
 *! If direction is not specified, both the read and the write
 *! direction is closed. Otherwise only the directions specified is
 *! closed.
 *!
 *! @returns
 *! Nonzero is returned if the file or stream wasn't open in the
 *! specified direction, zero otherwise.
 *!
 *! @throws
 *! An exception is thrown if an I/O error occurs.
 *!
 *! @note
 *! @[close()] has no effect if this file object has been associated
 *! with an already opened file, i.e. if @[open()] was given an
 *! integer as the first argument.
 *!
 *! @seealso
 *!   @[open()], @[open_socket()]
 */
static void file_close(INT32 args)
{
  int flags;
  if(args)
  {
    if(Pike_sp[-args].type != PIKE_T_STRING)
      Pike_error("Bad argument 1 to file->close()\n");
    flags=parse(Pike_sp[-args].u.string->str);
  }else{
    flags=FILE_READ | FILE_WRITE;
  }

  if (THIS->flags & FILE_LOCK_FD) {
    Pike_error("close() has been temporarily disabled on this file.\n");
  }

  if((THIS->open_mode & ~flags & (FILE_READ|FILE_WRITE)) && flags)
  {
    if(!(THIS->open_mode & fd_CAN_SHUTDOWN))
    {
      Pike_error("Cannot close one direction on this file.\n");
    }
  }

  flags=do_close(flags);
  pop_n_elems(args);
  push_int(flags);
}

/*! @decl int open(string filename, string mode)
 *! @decl int open(string filename, string mode, int access)
 *! @decl int open(int fd, string mode)
 *!
 *! Open a file, or use an existing fd.
 *!
 *! If @[filename] is given, attempt to open the named file. If @[fd]
 *! is given instead, it should be the file descriptor for an already
 *! opened file, which will then be used by this object.
 *!
 *! @[mode] describes how the file is opened. It's a case-insensitive
 *! string consisting of one or more of the following letters:
 *!
 *! @dl
 *!   @item "r"
 *!     Open for reading.
 *!   @item "w"
 *!     Open for writing.
 *!   @item "a"
 *!     Append new data to the end.
 *!   @item "c"
 *!     Create the file if it doesn't exist already.
 *!   @item "t"
 *!     Truncate the file to zero length if it already contains data.
 *!     Use only together with @expr{"w"@}.
 *!   @item "x"
 *!     Open exclusively - the open fails if the file already exists.
 *!     Use only together with @expr{"c"@}. Note that it's not safe to
 *!     assume that this is atomic on some systems.
 *! @enddl
 *!
 *! @[access] specifies the permissions to use if a new file is
 *! created. It is a UNIX style permission bitfield:
 *!
 *! @dl
 *!   @item 0400
 *!     User has read permission.
 *!   @item 0200
 *!     User has write permission.
 *!   @item 0100
 *!     User has execute permission.
 *!   @item 0040
 *!     Group has read permission.
 *!   @item 0020
 *!     Group has write permission.
 *!   @item 0010
 *!     Group has execute permission.
 *!   @item 0004
 *!     Others have read permission.
 *!   @item 0002
 *!     Others have write permission.
 *!   @item 0001
 *!     Others have execute permission.
 *! @enddl
 *!
 *! It's system dependent on which of these bits that are actually
 *! heeded. If @[access] is not specified, it defaults to
 *! @expr{00666@}, but note that on UNIX systems it's masked with the
 *! process umask before use.
 *!
 *! @seealso
 *!   @[close()]
 */
static void file_open(INT32 args)
{
  int flags,fd;
  int access;
  int err;
  struct pike_string *str, *flag_str;
  close_fd();

  if(args < 2)
    SIMPLE_TOO_FEW_ARGS_ERROR("Stdio.File->open", 2);

  if(Pike_sp[-args].type != PIKE_T_STRING &&
     Pike_sp[-args].type != PIKE_T_INT)
    SIMPLE_BAD_ARG_ERROR("Stdio.File->open", 1, "string|int");

  if(Pike_sp[1-args].type != PIKE_T_STRING)
    SIMPLE_BAD_ARG_ERROR("Stdio.File->open", 2, "string");

  if (args > 2)
  {
    if (Pike_sp[2-args].type != PIKE_T_INT)
      SIMPLE_BAD_ARG_ERROR("Stdio.File->open", 3, "int");
    access = Pike_sp[2-args].u.integer;
  } else
    access = 00666;

  flags = parse((flag_str = Pike_sp[1-args].u.string)->str);

  if (Pike_sp[-args].type==PIKE_T_STRING)
  {
     str=Pike_sp[-args].u.string;

     if (strlen(str->str) != (size_t)str->len) {
       /* Filenames with NUL are not supported. */
       ERRNO = errno = ENOENT;
       pop_n_elems(args);
       push_int(0);
       return;
     }

#ifdef PIKE_SECURITY
     if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
     {
	if(!CHECK_SECURITY(SECURITY_BIT_CONDITIONAL_IO))
	   Pike_error("Permission denied.\n");

	if(flags & (FILE_APPEND | FILE_TRUNC | FILE_CREATE | FILE_WRITE))
	{
	   push_text("write");
	}else{
	   push_text("read");
	}

	ref_push_object(Pike_fp->current_object);
	ref_push_string(str);
	ref_push_string(flag_str);
	push_int(access);

	safe_apply(OBJ2CREDS(CURRENT_CREDS)->user,"valid_open",5);
	switch(Pike_sp[-1].type)
	{
	   case PIKE_T_INT:
	      switch(Pike_sp[-1].u.integer)
	      {
		 case 0: /* return 0 */
		    ERRNO=errno=EPERM;
		    pop_n_elems(args+1);
		    push_int(0);
		    return;

		 case 1: /* return 1 */
		    pop_n_elems(args+1);
		    push_int(1);
		    return;

		 case 2: /* ok */
		    pop_stack();
		    break;

		 case 3: /* permission denied */
		    Pike_error("Stdio.file->open: permission denied.\n");

		 default:
		    Pike_error("Error in user->valid_open, wrong return value.\n");
	      }
	      break;

	   default:
	      Pike_error("Error in user->valid_open, wrong return type.\n");

	   case PIKE_T_STRING:
	      str=Pike_sp[-1].u.string;
	      args++;
	}
     }
#endif

     if(!( flags &  (FILE_READ | FILE_WRITE)))
	Pike_error("Must open file for at least one of read and write.\n");

     do {
       THREADS_ALLOW_UID();
       fd=fd_open(str->str,map(flags), access);
       err = errno;
       THREADS_DISALLOW_UID();
       if ((fd < 0) && (err == EINTR))
	 check_threads_etc();
     } while(fd < 0 && err == EINTR);

     if(!Pike_fp->current_object->prog)
     {
       if (fd >= 0)
	 while (fd_close(fd) && errno == EINTR) {}
       Pike_error("Object destructed in Stdio.File->open()\n");
     }

     if(fd < 0)
     {
	ERRNO=errno=err;
     }
     else
     {
	init_fd(fd,flags | fd_query_properties(fd, FILE_CAPABILITIES));
	set_close_on_exec(fd,1);
     }
  }
  else
  {
#ifdef PIKE_SECURITY
     if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
     {
	if(!CHECK_SECURITY(SECURITY_BIT_CONDITIONAL_IO))
	   Pike_error("Permission denied.\n");
	Pike_error("Permission denied.\n");
	/* FIXME!! Insert better security here */
     }
#endif
     fd=Pike_sp[-args].u.integer;
     if (fd<0)
	Pike_error("Not a valid FD.\n");

     init_fd(fd,flags | fd_query_properties(fd, FILE_CAPABILITIES));
     /* FIXME: What are the intended semantics for this flag?
      *        (grubba 2004-09-01
      */
     THIS->flags|=FILE_NOT_OPENED;
  }


  pop_n_elems(args);
  push_int(fd>=0);
}

#if !defined(__NT__) && (defined(HAVE_POSIX_OPENPT) || defined(PTY_MASTER_PATHNAME))
/*! @decl int openpt(string mode)
 *!
 *! Open the master end of a pseudo-terminal pair.
 *!
 *! @returns
 *! This function returns @expr{1@} for success, @expr{0@} otherwise.
 *!
 *! @seealso
 *!   @[grantpt()]
 */
static void file_openpt(INT32 args)
{
  int flags,fd;
#ifdef HAVE_POSIX_OPENPT
  struct pike_string *flag_str;
#endif
  close_fd();

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Stdio.File->openpt", 2);

  if(Pike_sp[-args].type != PIKE_T_STRING)
    SIMPLE_BAD_ARG_ERROR("Stdio.File->openpt", 1, "string");

#ifdef HAVE_POSIX_OPENPT
  flags = parse((flag_str = Pike_sp[-args].u.string)->str);
  
  if(!( flags &  (FILE_READ | FILE_WRITE)))
    Pike_error("Must open file for at least one of read and write.\n");

  do {
    THREADS_ALLOW_UID();
    fd=posix_openpt(map(flags));
    THREADS_DISALLOW_UID();
    if ((fd < 0) && (errno == EINTR))
      check_threads_etc();
  } while(fd < 0 && errno == EINTR);

  if(!Pike_fp->current_object->prog)
  {
    if (fd >= 0)
      while (fd_close(fd) && errno == EINTR) {}
    Pike_error("Object destructed in Stdio.File->openpt()\n");
  }

  if(fd < 0)
  {
    ERRNO=errno;
  }
  else
  {
    init_fd(fd,flags | fd_query_properties(fd, FILE_CAPABILITIES));
    set_close_on_exec(fd,1);
  }
  pop_n_elems(args);
  push_int(fd>=0);
#else
  if(args > 1)
    pop_n_elems(args - 1);
  push_constant_text(PTY_MASTER_PATHNAME);
  stack_swap();
  file_open(2);
#endif
}
#endif

#ifdef HAVE_FSYNC
/*! @decl int(0..1) sync()
 *!
 *!   Flush buffers to disk.
 *!
 *! @returns
 *!
 *!   Returns @expr{0@} (zero) and sets errno on failure.
 *!
 *!   Returns @expr{1@} on success.
 */
void file_sync(INT32 args)
{
  int ret;
  int fd = FD;
  int e;

  if(fd < 0)
    Pike_error("File not open.\n");

  pop_n_elems(args);

  do {
    THREADS_ALLOW();
    ret = fsync(fd);
    e = errno;
    THREADS_DISALLOW();
    check_threads_etc();
  } while ((ret < 0) && (e == EINTR));

  if (ret < 0) {
    ERRNO = errno = e;
    push_int(0);
  } else {
    push_int(1);
  }
}
#endif /* HAVE_FSYNC */

#if defined(INT64) && (defined(HAVE_LSEEK64) || defined(__NT__))
#define SEEK64
#endif

/*! @decl int seek(int pos)
 *! @decl int seek(int unit, int mult)
 *! @decl int seek(int unit, int mult, int add)
 *!
 *! Seek to a specified offset in a file.
 *!
 *! If @[mult] or @[add] are specified, @[pos] is calculated as
 *! @expr{@[pos] = @[unit]*@[mult] + @[add]@}.
 *!
 *! If @[pos] is negative then it is relative to the end of the file,
 *! otherwise it's an absolute offset from the start of the file.
 *!
 *! @returns
 *!   Returns the new offset, or @expr{-1@} on failure.
 *!
 *! @note
 *!   The arguments @[mult] and @[add] are considered obsolete, and
 *!   should not be used.
 *!
 *! @seealso
 *!   @[tell()]
 */
static void file_seek(INT32 args)
{
#ifdef SEEK64
  INT64 to = 0;
#else
  off_t to = 0;
#endif

  if( args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Stdio.File->seek", 1);

#if defined (SEEK64) && defined (AUTO_BIGNUM)
  if(is_bignum_object_in_svalue(&Pike_sp[-args])) {
    if (!int64_from_bignum(&to, Pike_sp[-args].u.object))
      Pike_error ("Bad argument 1 to Stdio.File->seek(). Offset too large.\n");
  }
  else
#endif
    if(Pike_sp[-args].type != PIKE_T_INT)
      SIMPLE_BAD_ARG_ERROR("Stdio.File->seek", 1, "int");
    else
      to=Pike_sp[-args].u.integer;

  if(FD < 0)
    Pike_error("File not open.\n");

  if(args>1)
  {
    if(Pike_sp[-args+1].type != PIKE_T_INT)
      SIMPLE_BAD_ARG_ERROR("Stdio.File->seek", 2, "int");
    to *= Pike_sp[-args+1].u.integer;
  }
  if(args>2)
  {
    if(Pike_sp[-args+2].type != PIKE_T_INT)
      SIMPLE_BAD_ARG_ERROR("Stdio.File->seek", 3, "int");
    to += Pike_sp[-args+2].u.integer;
  }

  ERRNO=0;

#if defined(HAVE_LSEEK64) && !defined(__NT__)
  to = lseek64(FD,to,to<0 ? SEEK_END : SEEK_SET);
#else
  to = fd_lseek(FD,to,to<0 ? SEEK_END : SEEK_SET);
#endif
  if(to<0) ERRNO=errno;

  pop_n_elems(args);
  push_int64(to);
}

#if defined(INT64) && (defined(HAVE_LSEEK64) || defined(__NT__))
#define TELL64
#endif

/*! @decl int tell()
 *!
 *! Returns the current offset in the file.
 *!
 *! @seealso
 *!   @[seek()]
 */
static void file_tell(INT32 args)
{
#ifdef TELL64
  INT64 to;
#else
  off_t to;
#endif

  if(FD < 0)
    Pike_error("File not open.\n");

  ERRNO=0;

#if defined(HAVE_LSEEK64) && !defined(__NT__)
  to = lseek64(FD, 0L, SEEK_CUR);
#else
  to = fd_lseek(FD, 0L, SEEK_CUR);
#endif

  if(to<0) ERRNO=errno;

  pop_n_elems(args);
  push_int64(to);
}

/*! @decl int(0..1) truncate(int length)
 *!
 *! Truncate a file.
 *!
 *! Truncates the file to the specified length @[length].
 *!
 *! @returns
 *!   Returns @expr{1@} on success, and @expr{0@} (zero) on failure.
 *!
 *! @seealso
 *!   @[open()]
 */
static void file_truncate(INT32 args)
{
#if defined(INT64)
  INT64 len = 0;
#else
  off_t len = 0;
#endif
  int res;

  if(args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Stdio.File->truncate", 1);

#if defined (INT64) && defined (AUTO_BIGNUM)
#if defined (HAVE_FTRUNCATE64) || SIZEOF_OFF_T > SIZEOF_INT_TYPE
  if(is_bignum_object_in_svalue(&Pike_sp[-args])) {
    if (!int64_from_bignum(&len, Pike_sp[-args].u.object))
      Pike_error ("Bad argument 1 to Stdio.File->truncate(). Length too large.\n");
  }
  else
#endif
#endif
    if(Pike_sp[-args].type != PIKE_T_INT)
      SIMPLE_BAD_ARG_ERROR("Stdio.File->truncate", 1, "int");
    else
      len = Pike_sp[-args].u.integer;

  if(FD < 0)
    Pike_error("File not open.\n");

  ERRNO=0;
#ifdef HAVE_FTRUNCATE64
  res = ftruncate64 (FD, len);
#else
  res=fd_ftruncate(FD, len);
#endif

  pop_n_elems(args);

  if(res<0)
     ERRNO=errno;

  push_int(!res);
}

/*! @decl Stat stat()
 *!
 *! Get status for an open file.
 *!
 *! This function returns the same information as the function
 *! @[file_stat()], but for the file it is called in. If file is not
 *! an open file, @expr{0@} (zero) is returned. Zero is also returned
 *! if file is a pipe or socket.
 *!
 *! @returns
 *!   See @[file_stat()] for a description of the return value.
 *!
 *! @note
 *!   Prior to Pike 7.1 this function returned an array(int).
 *!
 *! @seealso
 *!   @[file_stat()]
 */
static void file_stat(INT32 args)
{
  int fd;
  PIKE_STAT_T s;
  int tmp;

  if(FD < 0)
    Pike_error("File not open.\n");

  pop_n_elems(args);

  fd=FD;

 retry:
  THREADS_ALLOW();
  tmp=fd_fstat(fd, &s);
  THREADS_DISALLOW();

  if(tmp < 0)
  {
    if(errno == EINTR) {
      check_threads_etc();
      goto retry;
    }
    ERRNO=errno;
    push_int(0);
  }else{
    ERRNO=0;
    push_stat(&s);
  }
}

#if defined(HAVE_FSETXATTR) && defined(HAVE_FGETXATTR) && defined(HAVE_FLISTXATTR)
/* All A-OK.*/

/*! @decl array(string) listxattr( )
 *! 
 *! Return an array of all extended attributes set on the file
 */
static void file_listxattr(INT32 args)
{
  char buffer[1024];
  char *ptr = buffer;
  int mfd = FD, do_free = 0;
  ssize_t res;

  pop_n_elems( args );

  THREADS_ALLOW();
  do {
    res = flistxattr( mfd, buffer, sizeof(buffer) ); /* First try, for speed.*/
  } while( res < 0 && errno == EINTR );
  THREADS_DISALLOW();

  if( res<0 && errno==ERANGE )
  {
    /* Too little space in buffer.*/
    int blen = 65536;
    do_free = 1;
    ptr = xalloc( 1 );
    do {
      char *tmp = realloc( ptr, blen );
      if( !tmp )
	break;
      ptr = tmp;
      THREADS_ALLOW();
      do {
	res = flistxattr( mfd, ptr, blen );
      } while( res < 0 && errno == EINTR );
      THREADS_DISALLOW();
      blen *= 2;
    }
    while( (res < 0) && (errno == ERANGE) );
  }

  if( res < 0 )
  {
    if( do_free && ptr )
      free(ptr);
    push_int(0);
    ERRNO=errno;
    return;
  }

  push_string( make_shared_binary_string( ptr, res ) );
  ptr[0]=0;
  push_string( make_shared_binary_string( ptr, 1 ) );
  o_divide();
  push_empty_string();
  f_aggregate(1);
  o_subtract();

  if( do_free && ptr ) 
    free( ptr );
}

/*! @decl string getxattr(string attr)
 *! 
 *! Return the value of a specified attribute, or 0 if it does not exist
 */
static void file_getxattr(INT32 args)
{
  char buffer[1024];
  char *ptr = buffer;
  int mfd = FD, do_free = 0;
  ssize_t res;
  char *name;
  
  get_all_args( "getxattr", args, "%s", &name );

  THREADS_ALLOW();
  do {
    res = fgetxattr( mfd, name, buffer, sizeof(buffer) ); /* First try, for speed.*/
  } while( res < 0 && errno == EINTR );
  THREADS_DISALLOW();

  if( res<0 && errno==ERANGE )
  {
    /* Too little space in buffer.*/
    int blen = 65536;
    do_free = 1;
    ptr = xalloc( 1 );
    do {
      char *tmp = realloc( ptr, blen );
      if( !tmp )
	break;
      ptr = tmp;
      THREADS_ALLOW();
      do {
	res = fgetxattr( mfd, name, ptr, blen );
      } while( res < 0 && errno == EINTR );
      THREADS_DISALLOW();
      blen *= 2;
    }
    while( (res < 0) && (errno == ERANGE) );
  }

  if( res < 0 )
  {
    if( do_free && ptr )
      free(ptr);
    push_int(0);
    ERRNO=errno;
    return;
  }

  push_string( make_shared_binary_string( ptr, res ) );
  if( do_free && ptr ) 
    free( ptr );
}


/*! @decl void removexattr( string attr )
 *! Remove the specified extended attribute.
 */
static void file_removexattr( INT32 args )
{
  char *name;
  int mfd = FD;
  int rv;
  get_all_args( "removexattr", args, "%s", &name );
  THREADS_ALLOW();
  while( ((rv=fremovexattr( mfd, name )) < 0) && (errno == EINTR))
    ;
  THREADS_DISALLOW();

  pop_n_elems(args);
  if( rv < 0 )
  {
    ERRNO=errno;
    push_int(0);
  }
  else
  {
    push_int(1);
  }
}

/*! @decl void setxattr( string attr, string value, int flags)
 *!
 *! Set the attribute @[attr] to the value @[value].
 *!
 *! The flags parameter can be used to refine the semantics of the operation.  
 *!
 *! @[XATTR_CREATE] specifies a pure create, which
 *! fails if the named attribute exists already.  
 *!
 *! @[XATTR_REPLACE] specifies a pure replace operation, which fails if the named
 *! attribute does not already exist. 
 *!
 *! By default (no flags), the extended attribute will be created if need be, 
 *! or will simply replace the value if the attribute exists.
 *!
 *! @returns
 *! 1 if successful, 0 otherwise, setting errno.
 */
static void file_setxattr( INT32 args )
{
  char *ind;
  struct pike_string *val;
  int flags;
  int rv;
  int mfd = FD;
  get_all_args( "setxattr", args, "%s%S%d", &ind, &val, &flags );
  THREADS_ALLOW();
  while( ((rv=fsetxattr( mfd, ind, val->str,
			 (val->len<<val->size_shift), flags )) < 0) &&
	 (errno == EINTR))
    ;
  THREADS_DISALLOW();
  pop_n_elems(args);
  if( rv < 0 )
  {
    ERRNO=errno;
    push_int(0);
  }
  else
    push_int(1);
}
#endif

/*! @decl int errno()
 *!
 *! Return the errno for the latest failed file operation.
 */
static void file_errno(INT32 args)
{
  pop_n_elems(args);
  push_int(ERRNO);
}

/*! @decl int mode()
 *!
 *! Returns the open mode for the file.
 *!
 *! @int
 *!   @value 0x1000
 *!     FILE_READ
 *!   @value 0x2000
 *!     FILE_WRITE
 *!   @value 0x4000
 *!     FILE_APPEND
 *!   @value 0x8000
 *!     FILE_CREATE
 *!   @value 0x0100
 *!     FILE_TRUNC
 *!   @value 0x0200
 *!     FILE_EXCLUSIVE
 *!   @value 0x0400
 *!     FILE_NONBLOCKING
 *!   @value 0x0800
 *!     FILE_SET_CLOSE_ON_EXEC
 *!   @value 0x0001
 *!     FILE_HAS_INTERNAL_REF
 *!   @value 0x0002
 *!     FILE_NO_CLOSE_ON_DESTRUCT
 *!   @value 0x0004
 *!     FILE_LOCK_FD
 *!   @value 0x0010
 *!     FILE_NOT_OPENED
 *! @endint
 *!
 *! @seealso
 *!   @[open()]
 */
static void file_mode(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->open_mode);
}

/*! @decl void set_backend (Pike.Backend backend)
 *!
 *! Set the backend used for the callbacks.
 *!
 *! @note
 *! The backend keeps a reference to this object only when it is in
 *! callback mode. So if this object hasn't got any active callbacks
 *! and it runs out of other references, it will still be destructed
 *! quickly (after closing, if necessary).
 *!
 *! Also, this object does not keep a reference to the backend.
 *!
 *! @seealso
 *!   @[query_backend], @[set_nonblocking], @[set_read_callback], @[set_write_callback]
 */
static void file_set_backend (INT32 args)
{
  struct my_file *f = THIS;
  struct Backend_struct *backend;

  if (!args)
    SIMPLE_TOO_FEW_ARGS_ERROR ("Stdio.File->set_backend", 1);
  if (Pike_sp[-args].type != PIKE_T_OBJECT)
    SIMPLE_BAD_ARG_ERROR ("Stdio.File->set_backend", 1, "object(Pike.Backend)");
  backend = (struct Backend_struct *)
    get_storage (Pike_sp[-args].u.object, Backend_program);
  if (!backend)
    SIMPLE_BAD_ARG_ERROR ("Stdio.File->set_backend", 1, "object(Pike.Backend)");

  if (f->box.backend)
    change_backend_for_box (&f->box, backend);
  else
    INIT_FD_CALLBACK_BOX (&f->box, backend, f->box.ref_obj,
			  f->box.fd, 0, got_fd_event);

  pop_n_elems (args - 1);
}

/*! @decl Pike.Backend query_backend()
 *!
 *! Return the backend used for the callbacks.
 *!
 *! @seealso
 *!   @[set_backend]
 */
static void file_query_backend (INT32 args)
{
  pop_n_elems (args);
  ref_push_object (get_backend_obj (THIS->box.backend ? THIS->box.backend :
				    default_backend));
}

/*! @decl void set_nonblocking()
 *!
 *! Sets this file to nonblocking operation.
 *!
 *! @note
 *!   Nonblocking operation is not supported on all Stdio.File objects.
 *!   Notably it is not guaranteed to be supported on objects returned
 *!   by @[pipe()] unless @[PROP_NONBLOCK] was specified in the call
 *!   to @[pipe()].
 *!
 *! @seealso
 *!   @[set_blocking()]
 */
static void file_set_nonblocking(INT32 args)
{
  if(FD < 0) Pike_error("File not open.\n");

  if(!(THIS->open_mode & fd_CAN_NONBLOCK))
    Pike_error("This file does not support nonblocking operation.\n");

  if(set_nonblocking(FD,1))
  {
    ERRNO=errno;
    push_int (ERRNO);
    f_strerror (1);
    Pike_error("Stdio.File->set_nonblocking() failed: %S\n",
	       Pike_sp[-1].u.string);
  }

  THIS->open_mode |= FILE_NONBLOCKING;

  pop_n_elems(args);
}

/*! @decl void set_blocking()
 *!
 *! Sets this file to blocking operation.
 *!
 *! This is the inverse operation of @[set_nonblocking()].
 *!
 *! @seealso
 *!   @[set_nonblocking()]
 */
static void file_set_blocking(INT32 args)
{
  if(FD >= 0)
  {
    set_nonblocking(FD,0);
    THIS->open_mode &=~ FILE_NONBLOCKING;
  }
  pop_n_elems(args);
}

/*! @decl void set_close_on_exec(int(0..1) yes_no)
 *!
 *! Marks the file as to be closed in spawned processes.
 *!
 *! This function determines whether this file will be closed when
 *! calling exec().
 *!
 *! Default is that the file WILL be closed on exec except for
 *! stdin, stdout and stderr.
 *!
 *! @seealso
 *!   @[Process.create_process()], @[exec()]
 */
static void file_set_close_on_exec(INT32 args)
{
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Stdio.File->set_close_on_exec", 1);
  if(FD <0)
    Pike_error("File not open.\n");

  if(UNSAFE_IS_ZERO(Pike_sp-args))
  {
    my_set_close_on_exec(FD,0);
  }else{
    my_set_close_on_exec(FD,1);
  }
  pop_n_elems(args);
}

/*! @decl int is_open()
 *!
 *! Returns true if the file is open.
 *!
 *! @note
 *! If the file is a socket that has been closed from the remote side,
 *! this function might still return true.
 *!
 *! @note
 *! Most methods can't be called for a file descriptor that isn't
 *! open. Notable exceptions @[errno], @[mode], and the set and query
 *! functions for callbacks and backend.
 */
static void file_is_open (INT32 args)
{
  /* Note: Even though we'd like to, we can't accurately tell whether
   * a socket has been closed from the remote end or not. */
  pop_n_elems (args);
  push_int (FD >= 0);
}

/*! @decl int query_fd()
 *!
 *! Returns the file descriptor number associated with this object.
 */
static void file_query_fd(INT32 args)
{
  if(FD < 0)
    Pike_error("File not open.\n");

  pop_n_elems(args);
  push_int(FD);
}

/*! @decl int release_fd()
 *!
 *! Returns the file descriptor number associated with this object, in
 *! addition to releasing it so that this object behaves as if closed.
 *! Other settings like callbacks and backend remain intact.
 *! @[take_fd] can later be used to reinstate the file descriptor so
 *! that the state is restored.
 *!
 *! @seealso
 *!   @[query_fd()], @[take_fd()]
 */
static void file_release_fd(INT32 args)
{
  file_query_fd(args);
  change_fd_for_box(&THIS->box, -1);
}

/*! @decl void take_fd(int fd)
 *!
 *! Rehooks the given file descriptor number to be associated with
 *! this object. As opposed to using @[open] with a file descriptor
 *! number, it will be closed by this object upon destruct or when
 *! @[close] is called.
 *!
 *! @seealso
 *!   @[release_fd()]
 */
static void file_take_fd(INT32 args)
{
  if (args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR ("Stdio.File->take_fd", 1);
  if (Pike_sp[-args].type != PIKE_T_INT)
    SIMPLE_BAD_ARG_ERROR ("Stdio.File->take_fd", 0, "int");
  change_fd_for_box(&THIS->box, Pike_sp[-args].u.integer);
  pop_n_elems(args);
}

struct object *file_make_object_from_fd(int fd, int mode, int guess)
{
  struct object *o=low_clone(file_program);
  struct my_file *f = (struct my_file *) o->storage;
  call_c_initializers(o);
  f->box.fd=fd;
  if (fd >= 0) {
    f->open_mode=mode | fd_query_properties(fd, guess);
#ifdef PIKE_DEBUG
    debug_check_fd_not_in_use (fd);
#endif
  } else {
    f->open_mode = 0;
  }
  return o;
}

/*! @decl void set_buffer(int bufsize, string mode)
 *! @decl void set_buffer(int bufsize)
 *!
 *! Set internal socket buffer.
 *!
 *! This function sets the internal buffer size of a socket or stream.
 *!
 *! The second argument allows you to set the read or write buffer by
 *! specifying @expr{"r"@} or @expr{"w"@}.
 *!
 *! @note
 *!   It is not guaranteed that this function actually does anything,
 *!   but it certainly helps to increase data transfer speed when it does.
 *!
 *! @seealso
 *!   @[open_socket()], @[accept()]
 */
static void file_set_buffer(INT32 args)
{
  INT32 bufsize;
  int flags;

  if(FD==-1)
    Pike_error("Stdio.File->set_buffer() on closed file.\n");
  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("Stdio.File->set_buffer", 1);
  if(Pike_sp[-args].type!=PIKE_T_INT)
    SIMPLE_BAD_ARG_ERROR("Stdio.File->set_buffer", 1, "int");

  bufsize=Pike_sp[-args].u.integer;
  if(bufsize < 0)
    Pike_error("Bufsize must be larger than zero.\n");

  if(args>1)
  {
    if(Pike_sp[1-args].type != PIKE_T_STRING)
      SIMPLE_BAD_ARG_ERROR("Stdio.File->set_buffer", 2, "string");
    flags=parse(Pike_sp[1-args].u.string->str);
  }else{
    flags=FILE_READ | FILE_WRITE;
  }
  pop_n_elems(args);

#ifdef SOCKET_BUFFER_MAX
#if SOCKET_BUFFER_MAX
  if(bufsize>SOCKET_BUFFER_MAX) bufsize=SOCKET_BUFFER_MAX;
#endif
  flags &= THIS->open_mode;
#ifdef SO_RCVBUF
  if(flags & FILE_READ)
  {
    int tmp=bufsize;
    fd_setsockopt(FD,SOL_SOCKET, SO_RCVBUF, (char *)&tmp, sizeof(tmp));
  }
#endif /* SO_RCVBUF */

#ifdef SO_SNDBUF
  if(flags & FILE_WRITE)
  {
    int tmp=bufsize;
    fd_setsockopt(FD,SOL_SOCKET, SO_SNDBUF, (char *)&tmp, sizeof(tmp));
  }
#endif /* SO_SNDBUF */
#endif
}

#ifndef AF_UNIX
#define AF_UNIX	4711
#endif /* AF_UNIX */

#ifdef SOCKETPAIR_DEBUG
#define SP_DEBUG(X)	fprintf X
#else /* !SOCKETPAIR_DEBUG */
#define SP_DEBUG(X)
#endif /* SOCKETPAIR_DEBUG */

/* No socketpair() ?
 * No AF_UNIX sockets ?
 * No hope ?
 *
 * Don't despair, socketpair_ultra is here!
 * Tests done by an independant institute in Europe show that
 * socketpair_ultra is 50% more portable than other leading
 * brands of socketpair.
 *                                                   /Hubbe
 */

/* redefine socketpair to something that hopefully won't
 * collide with any libs or headers. Also useful when testing
 * this code on a system that _has_ socketpair...
 */

/* Protected since errno may expand to a function call. */
#ifndef errno
extern int errno;
#endif /* !errno */
static int socketpair_fd = -1;
int my_socketpair(int family, int type, int protocol, int sv[2])
{
  static struct sockaddr_in my_addr;
  struct sockaddr_in addr,addr2;
  int retries=0;
  /* Solaris and AIX think this variable should be a size_t, everybody else
   * thinks it should be an int.
   *
   * FIXME: Configure-test?
   */
  ACCEPT_SIZE_T len;

  MEMSET((char *)&addr,0,sizeof(struct sockaddr_in));

  /* We lie, we actually create an AF_INET socket... */
  if(family != AF_UNIX || type != SOCK_STREAM)
  {
    errno=EINVAL;
    return -1;
  }

  if(socketpair_fd==-1)
  {
    if((socketpair_fd=fd_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      SP_DEBUG((stderr, "my_socketpair:fd_socket() failed, errno:%d\n",
		errno));
      return -1;
    }

    /* I wonder what is most common a loopback on ip# 127.0.0.1 or
     * a loopback with the name "localhost"?
     * Let's hope those few people who don't have socketpair have
     * a loopback on 127.0.0.1
     */
    MEMSET((char *)&my_addr,0,sizeof(struct sockaddr_in));
    my_addr.sin_family=AF_INET;
    my_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    my_addr.sin_port=htons(0);


    /* Bind our sockets on any port */
    if(fd_bind(socketpair_fd, (struct sockaddr *)&my_addr, sizeof(addr)) < 0)
    {
      SP_DEBUG((stderr, "my_socketpair:fd_bind() failed, errno:%d\n",
		errno));
      while (fd_close(socketpair_fd) && errno == EINTR) {}
      socketpair_fd=-1;
      return -1;
    }

    /* Check what ports we got.. */
    len = sizeof(my_addr);
    if(fd_getsockname(socketpair_fd,(struct sockaddr *)&my_addr,&len) < 0)
    {
      SP_DEBUG((stderr, "my_socketpair:fd_getsockname() failed, errno:%d\n",
		errno));
      while (fd_close(socketpair_fd) && errno == EINTR) {}
      socketpair_fd=-1;
      return -1;
    }

    /* Listen to connections on our new socket */
    if(fd_listen(socketpair_fd, 5) < 0)
    {
      SP_DEBUG((stderr, "my_socketpair:fd_listen() failed, errno:%d\n",
		errno));
      while (fd_close(socketpair_fd) && errno == EINTR) {}
      socketpair_fd=-1;
      return -1;
    }

    set_close_on_exec(socketpair_fd, 1);

    set_nonblocking(socketpair_fd, 1);

    my_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
  }


  if((sv[1]=fd_socket(AF_INET, SOCK_STREAM, 0)) <0) {
    SP_DEBUG((stderr, "my_socketpair:fd_socket() failed, errno:%d (2)\n",
	      errno));
    return -1;
  }

/*  set_nonblocking(sv[1],1); */

retry_connect:
  retries++;
  if(fd_connect(sv[1], (struct sockaddr *)&my_addr, sizeof(addr)) < 0)
  {
/*    fprintf(stderr,"errno=%d (%d)\n",errno,EWOULDBLOCK); */
    SP_DEBUG((stderr, "my_socketpair:fd_connect() failed, errno:%d (%d)\n",
	      errno, EWOULDBLOCK));
    if((errno != EWOULDBLOCK)
#ifdef WSAEWOULDBLOCK
       && (errno != WSAEWOULDBLOCK)
#endif /* WSAEWOULDBLOCK */
       )
    {
      int tmp2;
      for(tmp2=0;tmp2<20;tmp2++)
      {
	int tmp;
	ACCEPT_SIZE_T len2;

	len2=sizeof(addr);
	tmp=fd_accept(socketpair_fd,(struct sockaddr *)&addr,&len2);

	if(tmp!=-1) {
	  SP_DEBUG((stderr, "my_socketpair:fd_accept() failed, errno:%d\n",
		    errno));
	  while (fd_close(tmp) && errno == EINTR) {}
	}
	else
	  break;
      }
      if(retries > 20) return -1;
      goto retry_connect;
    }
  }


  /* Accept connection
   * Make sure this connection was our OWN connection,
   * otherwise some wizeguy could interfere with our
   * pipe by guessing our socket and connecting at
   * just the right time... Pike is supposed to be
   * pretty safe...
   */
  do
  {
    ACCEPT_SIZE_T len3;

    len3=sizeof(addr);
  retry_accept:
    sv[0]=fd_accept(socketpair_fd,(struct sockaddr *)&addr,&len3);

    if(sv[0] < 0) {
      SP_DEBUG((stderr, "my_socketpair:fd_accept() failed, errno:%d (2)\n",
		errno));
      if(errno==EINTR) goto retry_accept;
      while (fd_close(sv[1]) && errno == EINTR) {}
      return -1;
    }

    set_nonblocking(sv[0],0);

    /* We do not trust accept */
    len=sizeof(addr);
    if(fd_getpeername(sv[0], (struct sockaddr *)&addr,&len)) {
      SP_DEBUG((stderr, "my_socketpair:fd_getpeername() failed, errno:%d\n",
		errno));
      return -1;
    }
    len=sizeof(addr);
    if(fd_getsockname(sv[1],(struct sockaddr *)&addr2,&len) < 0) {
      SP_DEBUG((stderr, "my_socketpair:fd_getsockname() failed, errno:%d\n",
		errno));
      return -1;
    }
  }while(len < (int)sizeof(addr) ||
	 addr2.sin_addr.s_addr != addr.sin_addr.s_addr ||
	 addr2.sin_port != addr.sin_port);

/*  set_nonblocking(sv[1],0); */

  SP_DEBUG((stderr, "my_socketpair: succeeded\n",
	    errno));

  return 0;
}

int socketpair_ultra(int family, int type, int protocol, int sv[2])
{
  int retries=0;

  while(1)
  {
    int ret=my_socketpair(family, type, protocol, sv);
    if(ret>=0) return ret;

    switch(errno)
    {
      case EAGAIN: break;

      case EADDRINUSE:
	if(retries++ > 10) return ret;
	break;

      default:
	return ret;
    }
  }
}

#ifndef HAVE_SOCKETPAIR
#define socketpair socketpair_ultra
#endif

/*! @decl Stdio.File pipe()
 *! @decl Stdio.File pipe(int flags)
 */
static void file_pipe(INT32 args)
{
  int inout[2] = { -1, -1 };
  int i = 0;

  int type=fd_CAN_NONBLOCK | fd_BIDIRECTIONAL;
  int reverse;

  check_all_args("file->pipe",args, BIT_INT | BIT_VOID, 0);
  if(args) type = Pike_sp[-args].u.integer;

  reverse = type & fd_REVERSE;
  type &= ~fd_REVERSE;

  close_fd();
  pop_n_elems(args);
  ERRNO=0;

  do
  {
#ifdef PIPE_CAPABILITIES
    if(!(type & ~(PIPE_CAPABILITIES)))
    {
      i=fd_pipe(&inout[0]);
      if (i >= 0) {
	type=PIPE_CAPABILITIES;
	break;
      }
    }
#endif

#ifdef UNIX_SOCKETS_WORK_WITH_SHUTDOWN
#undef UNIX_SOCKET_CAPABILITIES
#define UNIX_SOCKET_CAPABILITIES (fd_INTERPROCESSABLE | fd_BIDIRECTIONAL | fd_CAN_NONBLOCK | fd_CAN_SHUTDOWN)
#endif

#if defined(HAVE_SOCKETPAIR)
    if(!(type & ~(UNIX_SOCKET_CAPABILITIES)))
    {
      i=fd_socketpair(AF_UNIX, SOCK_STREAM, 0, &inout[0]);
      if (i >= 0) {
	type=UNIX_SOCKET_CAPABILITIES;
	break;
      }
    }
#endif

    if(!(type & ~(SOCKET_CAPABILITIES)))
    {
      i=socketpair_ultra(AF_UNIX, SOCK_STREAM, 0, &inout[0]);
      if (i >= 0) {
	type=SOCKET_CAPABILITIES;
	break;
      }
    }

    if (!i) {
      Pike_error("Cannot create a pipe matching those parameters.\n");
    }
  }while(0);

  if ((i<0) || (inout[0] < 0) || (inout[1] < 0))
  {
    ERRNO=errno;
    if (inout[0] >= 0) {
      while (fd_close(inout[0]) && errno == EINTR) {}
    }
    if (inout[1] >= 0) {
      while (fd_close(inout[1]) && errno == EINTR) {}
    }
    errno = ERRNO;
    push_int(0);
  }
  else if (reverse) 
  {
    init_fd(inout[1],FILE_WRITE | (type&fd_BIDIRECTIONAL?FILE_READ:0) |
	    fd_query_properties(inout[1], type));

    my_set_close_on_exec(inout[1],1);
    my_set_close_on_exec(inout[0],1);
    change_fd_for_box (&THIS->box, inout[1]);

    ERRNO=0;
    push_object(file_make_object_from_fd(inout[0], (type&fd_BIDIRECTIONAL?FILE_WRITE:0)| FILE_READ,type));
  } else {
    init_fd(inout[0],FILE_READ | (type&fd_BIDIRECTIONAL?FILE_WRITE:0) |
	    fd_query_properties(inout[0], type));

    my_set_close_on_exec(inout[0],1);
    my_set_close_on_exec(inout[1],1);
    change_fd_for_box (&THIS->box, inout[0]);

    ERRNO=0;
    push_object(file_make_object_from_fd(inout[1], (type&fd_BIDIRECTIONAL?FILE_READ:0)| FILE_WRITE,type));
  }
}

static void file_handle_events(int event)
{
  struct object *o=Pike_fp->current_object;
  struct my_file *f = THIS;
  switch(event)
  {
    case PROG_EVENT_INIT:
      init_fd (-1, 0);
      f->box.ref_obj = o;
      break;

    case PROG_EVENT_EXIT:
      if(!(f->flags & (FILE_NO_CLOSE_ON_DESTRUCT |
		       FILE_LOCK_FD |
		       FILE_NOT_OPENED)))
	close_fd_quietly();
      else
	free_fd_stuff();
      unhook_fd_callback_box (&f->box);
      break;

    case PROG_EVENT_GC_RECURSE:
      if (f->box.backend) {
	/* Need to deregister events if the gc has freed callbacks.
	 * This might lead to the file object being freed altogether. */
	int cb_events = 0;
	size_t ev;
	for (ev = 0; ev < NELEM (f->event_cbs); ev++)
	  if (f->event_cbs[ev].type != PIKE_T_INT)
	    cb_events |= 1 << ev;
	SUB_FD_EVENTS (f, ~cb_events);
      }
      break;
  }
}


static void low_dup(struct object *toob,
		    struct my_file *to,
		    struct my_file *from)
{
  size_t ev;

  debug_check_internals (from);

  my_set_close_on_exec(to->box.fd, to->box.fd > 2);

  to->open_mode=from->open_mode;
  to->flags = from->flags & ~(FILE_NO_CLOSE_ON_DESTRUCT |
			      FILE_LOCK_FD |
			      FILE_NOT_OPENED);

  /* Enforce that stdin, stdout and stderr aren't closed during
   * normal operation.
   */
  if (to->box.fd <= 2) {
    to->flags |= FILE_NO_CLOSE_ON_DESTRUCT;
    dmalloc_accept_leak_fd(to->box.fd);
  }

  /* Note: This previously enabled all events for which there were
   * callbacks instead of copying the event settings from the source
   * file. */

  unhook_fd_callback_box (&to->box);
  if (from->box.backend)
    INIT_FD_CALLBACK_BOX (&to->box, from->box.backend, to->box.ref_obj,
			  to->box.fd, from->box.events, got_fd_event);

  for (ev = 0; ev < NELEM (to->event_cbs); ev++)
    assign_svalue (&to->event_cbs[ev], &from->event_cbs[ev]);

  debug_check_internals (to);
}

/*! @decl int dup2(Stdio.File to)
 *!
 *! Duplicate a file over another.
 *!
 *! This function works similarly to @[assign()], but instead of making
 *! the argument a reference to the same file, it creates a new file
 *! with the same properties and places it in the argument.
 *!
 *! @note
 *!   In Pike 7.7 and later @[to] need not be open, in which
 *!   case a new fd is allocated.
 *!
 *! @seealso
 *!   @[assign()], @[dup()]
 */
static void file_dup2(INT32 args)
{
  struct object *o;
  struct my_file *fd;

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Stdio.File->dup2", 1);

  if(FD < 0)
    Pike_error("File not open.\n");

  if(Pike_sp[-args].type != PIKE_T_OBJECT)
    SIMPLE_BAD_ARG_ERROR("Stdio.File->dup2", 1, "Stdio.File");

  o=Pike_sp[-args].u.object;

  fd=get_file_storage(o);

  if(!fd)
    SIMPLE_BAD_ARG_ERROR("Stdio.File->dup2", 1, "Stdio.File");

  if(fd->box.fd < 0) {
    /* FIXME: Use change_fd_for_box here! */
    fd->box.revents = 0;
    if((fd->box.fd = fd_dup(FD)) < 0)
    {
      ERRNO = errno;
      pop_n_elems(args);
      push_int(0);
      return;
    }
  } else {
    if (fd->flags & FILE_LOCK_FD) {
      Pike_error("File has been temporarily locked from closing.\n");
    }

    THIS->box.revents = 0;
    if(fd_dup2(FD, fd->box.fd) < 0)
    {
      ERRNO = errno;
      pop_n_elems(args);
      push_int(0);
      return;
    }
  }
  ERRNO=0;
  low_dup(o, fd, THIS);

  pop_n_elems(args);
  push_int(1);
}

/*! @decl Stdio.Fd dup()
 */
static void file_dup(INT32 args)
{
  int fd;
  struct object *o;
  struct my_file *f;

  pop_n_elems(args);

  if(FD < 0)
    Pike_error("File not open.\n");


  if((fd=fd_dup(FD)) < 0)
  {
    ERRNO=errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }
  o=file_make_object_from_fd(fd, THIS->open_mode, THIS->open_mode);
  f=((struct my_file *)(o->storage));
  ERRNO=0;
  low_dup(o, f, THIS);
  push_object(o);
}

/*! @decl int(0..1) open_socket(int|void port, string|void addr, int|void family)
 */
static void file_open_socket(INT32 args)
{
  int fd;
  int family=-1;

  close_fd();

  if (args > 2 && Pike_sp[2-args].type == PIKE_T_INT &&
      Pike_sp[2-args].u.integer != 0)
    family = Pike_sp[2-args].u.integer;

  if (args && Pike_sp[-args].type == PIKE_T_INT &&
      Pike_sp[-args].u.integer < 0) {
    pop_n_elems(args);
    args = 0;
  }

/*   fprintf(stderr, "file_open_socket: family: %d\n", family); */

  if (args) {
    PIKE_SOCKADDR addr;
    int addr_len;
    char *name;
    int o;

    if (Pike_sp[-args].type != PIKE_T_INT &&
	(Pike_sp[-args].type != PIKE_T_STRING ||
	 Pike_sp[-args].u.string->size_shift)) {
      SIMPLE_BAD_ARG_ERROR("Stdio.File->open_socket", 1, "int|string (8bit)");
    }
    if (args > 1 && !UNSAFE_IS_ZERO(&Pike_sp[1-args])) {
      if (Pike_sp[1-args].type != PIKE_T_STRING) {
	SIMPLE_BAD_ARG_ERROR("Stdio.File->open_socket", 2, "string");
      }

      name = Pike_sp[1-args].u.string->str;
    } else {
      name = NULL;
    }
    addr_len = get_inet_addr(&addr, name,
			     (Pike_sp[-args].type == PIKE_T_STRING?
			      Pike_sp[-args].u.string->str : NULL),
			     (Pike_sp[-args].type == PIKE_T_INT?
			      Pike_sp[-args].u.integer : -1), 0);

    fd=fd_socket((family<0? SOCKADDR_FAMILY(addr):family), SOCK_STREAM, 0);
    if(fd < 0)
    {
      ERRNO=errno;
      pop_n_elems(args);
      push_int(0);
      return;
    }

    o=1;
    if(fd_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&o, sizeof(int)) < 0) {
      ERRNO=errno;
      while (fd_close(fd) && errno == EINTR) {}
      errno = ERRNO;
      pop_n_elems(args);
      push_int(0);
      return;
    }
    if (fd_bind(fd, (struct sockaddr *)&addr, addr_len) < 0) {
      ERRNO=errno;
      while (fd_close(fd) && errno == EINTR) {}
      errno = ERRNO;
      pop_n_elems(args);
      push_int(0);
      return;
    }
  } else {
    fd=fd_socket((family<0? AF_INET:family), SOCK_STREAM, 0);
    if(fd < 0)
    {
      ERRNO=errno;
      pop_n_elems(args);
      push_int(0);
      return;
    }
  }

  init_fd(fd, FILE_READ | FILE_WRITE | fd_query_properties(fd, SOCKET_CAPABILITIES));
  my_set_close_on_exec(fd,1);
  change_fd_for_box (&THIS->box, FD);
  ERRNO=0;

  pop_n_elems(args);
  push_int(1);
}

/*! @decl int(0..1) set_keepalive(int(0..1) on_off)
 */
static void file_set_keepalive(INT32 args)
{
  int tmp, i;
  INT_TYPE t;

  get_all_args("Stdio.File->set_keepalive", args, "%i", &t);

  /* In case int and INT_TYPE have different sizes */
  tmp = t;

#ifdef SO_KEEPALIVE
  i = fd_setsockopt(FD,SOL_SOCKET, SO_KEEPALIVE, (char *)&tmp, sizeof(tmp));
  if(i)
  {
    ERRNO=errno;
  }else{
    ERRNO=0;
  }
#else /* !SO_KEEPALIVE */
#ifdef ENOTSUP
  ERRNO = errno = ENOTSUP;
#else /* !ENOTSUP */
#ifdef ENOTTY
  ERRNO = errno = ENOTTY;
#else /* !ENOTTY */
  ERRNO = errno = EIO;
#endif /* ENOTTY */
#endif /* ENOTSUP */
#endif /* SO_KEEPALIVE */
  pop_n_elems(args);
  push_int(!i);
}

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>

#ifndef PATH_MAX
#ifdef _POSIX_PATH_MAX
#define PATH_MAX	_POSIX_PATH_MAX
#else /* !_POSIX_PATH_MAX */
#define PATH_MAX	255	/* Minimum according to POSIX. */
#endif /* _POSIX_PATH_MAX */
#endif /* !PATH_MAX */

/*! @decl int(0..1) connect_unix( string filename )
 *!
 *!   Open a UNIX domain socket connection to the specified destination.
 *!
 *!   In nonblocking mode, success is indicated with the write-callback,
 *!   and failure with the close-callback or the read_oob-callback.
 *!
 *! @returns
 *!   Returns @expr{1@} on success, and @expr{0@} on failure.
 *!
 *! @note
 *!   In nonblocking mode @expr{0@} (zero) may be returned and @[errno()] set
 *!   to @tt{EWOULDBLOCK@} or @tt{WSAEWOULDBLOCK@}. This should not be regarded
 *!   as a connection failure.
 */
static void file_connect_unix( INT32 args )
{
  struct sockaddr_un name;
  int tmp;

  if( args < 1 )
    SIMPLE_TOO_FEW_ARGS_ERROR("Stdio.File->connect_unix", 1);
  if( (Pike_sp[-args].type != PIKE_T_STRING) ||
      (Pike_sp[-args].u.string->size_shift) ||
      (Pike_sp[-args].u.string->len >= PATH_MAX) )
    Pike_error("Illegal argument. Expected string(8bit)\n");

  name.sun_family=AF_UNIX;
  strcpy( name.sun_path, Pike_sp[-args].u.string->str );
  pop_n_elems(args);

  close_fd();
  change_fd_for_box (&THIS->box, socket(AF_UNIX,SOCK_STREAM,0));

  if( FD < 0 )
  {
    ERRNO = errno;
    push_int(0);
    return;
  }

  init_fd(FD, FILE_READ | FILE_WRITE
	  | fd_query_properties(FD, SOCKET_CAPABILITIES));
  my_set_close_on_exec(FD, 1);

  tmp=connect(FD,(void *)&name,sizeof(struct sockaddr_un));
  if (tmp == -1) {
    ERRNO = errno;
    push_int(0);
  } else {
    push_int(1);
  }
}
#endif /* HAVE_SYS_UN_H */

/*! @decl int(0..1) connect(string dest_addr, int dest_port)
 *! @decl int(0..1) connect(string dest_addr, int dest_port, @
 *!                         string src_addr, int src_port)
 *!
 *!   Open a TCP/IP connection to the specified destination.
 *!
 *!   In nonblocking mode, success is indicated with the write-callback,
 *!   and failure with the close-callback or the read_oob-callback.
 *!
 *! @returns
 *!   Returns @expr{1@} on success, and @expr{0@} on failure.
 *!
 *! @note
 *!   In nonblocking mode @expr{0@} (zero) may be returned and @[errno()] set
 *!   to @tt{EWOULDBLOCK@} or @tt{WSAEWOULDBLOCK@}. This should not be regarded
 *!   as a connection failure.
 */
static void file_connect(INT32 args)
{
  PIKE_SOCKADDR addr;
  int addr_len;
  struct pike_string *dest_addr = NULL;
  struct pike_string *src_addr = NULL;
  struct svalue *dest_port = NULL;
  struct svalue *src_port = NULL;

  int tmp, was_closed = FD < 0;

  if (args < 4) {
    get_all_args("Stdio.File->connect", args, "%S%*", &dest_addr, &dest_port);
  } else {
    get_all_args("Stdio.File->connect", args, "%S%*%S%*",
		 &dest_addr, &dest_port, &src_addr, &src_port);
  }

  if(dest_port->type != PIKE_T_INT &&
     (dest_port->type != PIKE_T_STRING || dest_port->u.string->size_shift))
    SIMPLE_BAD_ARG_ERROR("Stdio.File->connect", 2, "int|string (8bit)");

  if(src_port && src_port->type != PIKE_T_INT &&
     (src_port->type != PIKE_T_STRING || src_port->u.string->size_shift))
    SIMPLE_BAD_ARG_ERROR("Stdio.File->connect", 4, "int|string (8bit)");

  addr_len = get_inet_addr(&addr, dest_addr->str,
			   (dest_port->type == PIKE_T_STRING?
			    dest_port->u.string->str : NULL),
			   (dest_port->type == PIKE_T_INT?
			    dest_port->u.integer : -1), 0);

/*   fprintf(stderr, "connect: family: %d\n", SOCKADDR_FAMILY(addr)); */

  if(was_closed)
  {
    if (args < 4) {
      push_int(-1);
      push_int(0);
      push_int(SOCKADDR_FAMILY(addr));
      file_open_socket(3);
    } else {
      push_svalue(src_port);
      ref_push_string(src_addr);
      file_open_socket(2);
    }
    if(UNSAFE_IS_ZERO(Pike_sp-1) || FD < 0)
      Pike_error("Stdio.File->connect(): Failed to open socket.\n");
    pop_stack();
  }

  tmp=FD;
  THREADS_ALLOW();
  tmp=fd_connect(tmp, (struct sockaddr *)&addr, addr_len);
  THREADS_DISALLOW();

  if(tmp < 0
#ifdef EINPROGRESS
     && !(errno==EINPROGRESS && (THIS->open_mode & FILE_NONBLOCKING))
#endif
#ifdef WSAEWOULDBLOCK
     && !(errno==WSAEWOULDBLOCK && (THIS->open_mode & FILE_NONBLOCKING))
#endif
#ifdef EWOULDBLOCK
     && !(errno==EWOULDBLOCK && (THIS->open_mode & FILE_NONBLOCKING))
#endif
    )
  {
    /* something went wrong */
    ERRNO=errno;
    if (was_closed) {
      while (fd_close (FD) && errno == EINTR) {}
      change_fd_for_box (&THIS->box, -1);
      errno = ERRNO;
    }
    pop_n_elems(args);
    push_int(0);
  }else{

    ERRNO=0;
    pop_n_elems(args);
    push_int(1);
  }
}

/*! @decl string query_address()
 *! @decl string query_address(int(0..1) local)
 *!
 *! Get address and port of a socket end-point.
 *!
 *! @param local
 *!   If the argument @[local] is not specified, or is @expr{0@}
 *!   (zero), the remote end-point is returned. Otherwise, if @[local]
 *!   is @expr{1@}, the local end-point is returned.
 *!
 *! @returns
 *!   This function returns the address and port of a socket end-point
 *!   on the form @expr{"x.x.x.x port"@} (IPv4) or
 *!   @expr{"x:x:x:x:x:x:x:x port"@} (IPv6).
 *!
 *!   If this file is not a socket, is not connected, or some other
 *!   error occurrs, @expr{0@} (zero) is returned.
 *!
 *! @seealso
 *!   @[connect()]
 */
static void file_query_address(INT32 args)
{
  PIKE_SOCKADDR addr;
  int i;
  char buffer[496],*q;
  /* XOPEN GROUP thinks this variable should be a size_t.
   * BSD thinks it should be an int.
   */
  ACCEPT_SIZE_T len;

  if(FD <0)
    Pike_error("Stdio.File->query_address(): Connection not open.\n");

  len=sizeof(addr);
  if(args > 0 && !UNSAFE_IS_ZERO(Pike_sp-args))
  {
    i=fd_getsockname(FD,(struct sockaddr *)&addr,&len);
  }else{
    i=fd_getpeername(FD,(struct sockaddr *)&addr,&len);
  }
  pop_n_elems(args);
  if(i < 0 || len < (int)sizeof(addr.ipv4))
  {
    ERRNO=errno;
    push_int(0);
    return;
  }
#ifdef HAVE_INET_NTOP
  inet_ntop(SOCKADDR_FAMILY(addr), SOCKADDR_IN_ADDR(addr),
	    buffer, sizeof(buffer)-20);
#else
  q=inet_ntoa(*SOCKADDR_IN_ADDR(addr));
  strncpy(buffer,q,sizeof(buffer)-20);
  buffer[sizeof(buffer)-20]=0;
#endif
  sprintf(buffer+strlen(buffer)," %d",(int)(ntohs(addr.ipv4.sin_port)));

  push_text(buffer);
}

/*! @decl Stdio.File `<<(string data)
 *! @decl Stdio.File `<<(mixed data)
 *!
 *! Write some data to a file.
 *!
 *! If @[data] is not a string, it is casted to string, and then
 *! written to the file.
 *!
 *! @note
 *!   Throws an error if not all data could be written.
 *!
 *! @seealso
 *!   @[write()]
 */
static void file_lsh(INT32 args)
{
  ptrdiff_t len;
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Stdio.File->`<<", 1);
  if(args > 1)
    pop_n_elems(args-1);

  if(Pike_sp[-1].type != PIKE_T_STRING)
  {
    ref_push_type_value(string_type_string);
    stack_swap();
    f_cast();
  }

  len=Pike_sp[-1].u.string->len;
  file_write(1);
  if(len != Pike_sp[-1].u.integer) Pike_error("Stdio.File << failed.\n");
  pop_stack();

  push_object(this_object());
}

/*! @decl void create(string filename)
 *! @decl void create(string filename, string mode)
 *! @decl void create(string filename, string mode, in access)
 *! @decl void create(int fd)
 *! @decl void create(int fd, string mode)
 *!
 *! See @[open()].
 *!
 *! @seealso
 *!   @[open()]
 */
static void file_create(INT32 args)
{
  if(!args) return;
  if(Pike_sp[-args].type != PIKE_T_STRING &&
     Pike_sp[-args].type != PIKE_T_INT)
    SIMPLE_BAD_ARG_ERROR("Stdio.File->create", 1, "int|string");

  close_fd();
  file_open(args);
}

#ifdef _REENTRANT

struct new_thread_data
{
  INT32 from, to;
};

static TH_RETURN_TYPE proxy_thread(void * data)
{
  char buffer[READ_BUFFER];
  struct new_thread_data *p=(struct new_thread_data *)data;

  while(1)
  {
    ptrdiff_t len, w;
    len = fd_read(p->from, buffer, READ_BUFFER);
    if(len==0) break;
    if(len<0)
    {
      if(errno==EINTR) continue;
/*      fprintf(stderr,"Threaded read failed with errno = %d\n",errno); */
      break;
    }

    w=0;
    while(w<len)
    {
      ptrdiff_t wl = fd_write(p->to, buffer+w, len-w);
      if(wl<0)
      {
	if(errno==EINTR) continue;
/*	fprintf(stderr,"Threaded write failed with errno = %d\n",errno); */
	break;
      }
      w+=wl;
    }
  }

/*  fprintf(stderr,"Closing %d and %d\n",p->to,p->from); */

  while (fd_close(p->to) && errno == EINTR) {}
  while (fd_close(p->from) && errno == EINTR) {}
  low_mt_lock_interpreter();	/* Can run even if threads_disabled. */
  num_threads--;
  mt_unlock_interpreter();
  free((char *)p);
  th_exit(0);
  return 0;
}

/*! @decl void proxy(Stdio.File from)
 *!
 *! Starts a thread that asynchronously copies data from @[from]
 *! to this file.
 *!
 *! @seealso
 *!   @[Stdio.sendfile()]
 */
void file_proxy(INT32 args)
{
  struct my_file *f;
  struct new_thread_data *p;
  int from, to;

  THREAD_T id;
  check_all_args("Stdio.File->proxy",args, BIT_OBJECT,0);
  f=get_file_storage(Pike_sp[-args].u.object);
  if(!f)
    SIMPLE_BAD_ARG_ERROR("Stdio.File->proxy", 1, "Stdio.File");

  from=fd_dup(f->box.fd);
  if(from<0)
  {
    ERRNO=errno;
    Pike_error("Failed to dup proxy fd. (errno=%d)\n",errno);
  }
  to=fd_dup(FD);
  if(from<0)
  {
    ERRNO=errno;
    while (fd_close(from) && errno == EINTR) {}
    errno = ERRNO;
    Pike_error("Failed to dup proxy fd.\n");
  }

  p=ALLOC_STRUCT(new_thread_data);
  p->from=from;
  p->to=to;

  num_threads++;
  if(th_create_small(&id,proxy_thread,p))
  {
    free((char *)p);
    Pike_error("Failed to create thread.\n");
  }

  th_destroy(& id);
  pop_n_elems(args);
  push_int(0);
}

void create_proxy_pipe(struct object *o, int for_reading)
{
  struct object *n,*n2;
  push_object(n=clone_object(file_program,0));
  push_int(fd_INTERPROCESSABLE);
  apply(n,"pipe",1);
  if(Pike_sp[-1].type!=PIKE_T_OBJECT)
    Pike_error("Failed to create proxy pipe (errno=%d)!\n",get_file_storage(n)->my_errno);
  n2=Pike_sp[-1].u.object;
  /* Stack is now: pipe(read), pipe(write) */
  if(for_reading)
  {
    ref_push_object(o);
    apply(n2,"proxy",1);
    pop_n_elems(2);
  }else{
    /* Swap */
    Pike_sp[-2].u.object=n2;
    Pike_sp[-1].u.object=n;
    apply(o,"proxy",1);
    pop_stack();
  }
}

#endif

#if defined(HAVE_FD_FLOCK) || defined(HAVE_FD_LOCKF)

static struct program * file_lock_key_program;

struct file_lock_key_storage
{
  struct my_file *f;
  struct object *file;
#ifdef _REENTRANT
  struct thread_state *owner;
  struct object *owner_obj;
#endif
};


#define OB2KEY(O) ((struct file_lock_key_storage *)((O)->storage))

static void low_file_lock(INT32 args, int flags)
{
  int ret,fd=FD;
  struct object *o;
  
  destruct_objects_to_destruct();

  if(FD < 0)
    Pike_error("Stdio.File->lock(): File is not open.\n");

  if(!args || UNSAFE_IS_ZERO(Pike_sp-args))
  {
    if(THIS->key
#ifdef _REENTRANT
       && OB2KEY(THIS->key)->owner == Pike_interpreter.thread_state
#endif
      )
    {
      if (flags & fd_LOCK_NB) {
#ifdef EWOULDBLOCK
	ERRNO = errno = EWOULDBLOCK;
#else /* !EWOULDBLOCK */
	ERRNO = errno = EAGAIN;
#endif /* EWOULDBLOCK */
	pop_n_elems(args);
	push_int(0);
	return;
      } else {
	Pike_error("Recursive file locks!\n");
      }
    }
  }

  o=clone_object(file_lock_key_program,0);

  THREADS_ALLOW();
#ifdef HAVE_FD_FLOCK
  ret=fd_flock(fd, flags);
#else
  ret=fd_lockf(fd, flags);
#endif
  THREADS_DISALLOW();

  if(ret<0)
  {
    free_object(o);
    ERRNO=errno;
    pop_n_elems(args);
    push_int(0);
  }else{
    THIS->key = o;
    OB2KEY(o)->f=THIS;
    add_ref(OB2KEY(o)->file = Pike_fp->current_object);
    pop_n_elems(args);
    push_object(o);
  }
}

/*! @decl Stdio.FileLockKey lock()
 *! @decl Stdio.FileLockKey lock(int(0..1) is_recursive)
 *!
 *! Makes an exclusive file lock on this file.
 *!
 *! @seealso
 *!   @[trylock()]
 */
static void file_lock(INT32 args)
{
  low_file_lock(args, fd_LOCK_EX);
}

/*! @decl Stdio.FileLockKey trylock()
 *! @decl Stdio.FileLockKey trylock(int(0..1) is_recursive)
 *!
 *! Attempts to place a file lock on this file.
 *!
 *! @seealso
 *!   @[lock()]
 */
/* If (fd_LOCK_EX | fd_LOCK_NB) is used with lockf, the result will be
 * F_TEST, which only tests for the existance of a lock on the file.
 */
#ifdef HAVE_FD_FLOCK
static void file_trylock(INT32 args)
{
  low_file_lock(args, fd_LOCK_EX | fd_LOCK_NB);
}
#else
static void file_trylock(INT32 args)
{
  low_file_lock(args, fd_LOCK_NB);
}
#endif

#define THIS_KEY ((struct file_lock_key_storage *)(Pike_fp->current_storage))
static void init_file_lock_key(struct object *o)
{
  THIS_KEY->f=0;
#ifdef _REENTRANT
  THIS_KEY->owner=Pike_interpreter.thread_state;
  add_ref(THIS_KEY->owner_obj=Pike_interpreter.thread_state->thread_obj);
#endif
}

static void exit_file_lock_key(struct object *o)
{
  if(THIS_KEY->f)
  {
    int fd=THIS_KEY->f->box.fd;
    int err;
#ifdef PIKE_DEBUG
    if(THIS_KEY->f->key != o)
      Pike_fatal("File lock key is wrong!\n");
#endif

    do
    {
      THREADS_ALLOW();
#ifdef HAVE_FD_FLOCK
      err=fd_flock(fd, fd_LOCK_UN);
#else
      err=fd_lockf(fd, fd_LOCK_UN);
#endif
      THREADS_DISALLOW();
      if ((err < 0) && (errno == EINTR)) {
	check_threads_etc();
      }
    }while(err<0 && errno==EINTR);

#ifdef _REENTRANT
    THIS_KEY->owner = NULL;
    if(THIS_KEY->owner_obj)
    {
      free_object(THIS_KEY->owner_obj);
      THIS_KEY->owner_obj = NULL;
    }
#endif
    THIS_KEY->f->key = 0;
    THIS_KEY->f = 0;
  }
}

static void init_file_locking(void)
{
  ptrdiff_t off;
  START_NEW_PROGRAM_ID (STDIO_FILE_LOCK_KEY);
  off = ADD_STORAGE(struct file_lock_key_storage);
#ifdef _REENTRANT
  MAP_VARIABLE("_owner",tObj,0,
	       off + OFFSETOF(file_lock_key_storage, owner_obj),
	       PIKE_T_OBJECT);
#endif
  MAP_VARIABLE("_file",tObj,0,
	       off + OFFSETOF(file_lock_key_storage, file),
	       PIKE_T_OBJECT);
  set_init_callback(init_file_lock_key);
  set_exit_callback(exit_file_lock_key);
  file_lock_key_program=end_program();
  file_lock_key_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;
}
static void exit_file_locking(void)
{
  if(file_lock_key_program)
  {
    free_program(file_lock_key_program);
    file_lock_key_program=0;
  }
}
#else /* !(HAVE_FD_FLOCK || HAVE_FD_LOCKF) */
#define init_file_locking()
#define exit_file_locking()
#endif /* HAVE_FD_FLOCK || HAVE_FD_LOCKF */

/*! @endclass
 */

/*! @decl array(int) get_all_active_fd()
 *! Returns the id of all the active file descriptors.
 */
static void f_get_all_active_fd(INT32 args)
{
  int i,fds,ne;
  PIKE_STAT_T foo;

  ne = MAX_OPEN_FILEDESCRIPTORS;

  pop_n_elems(args);
  for (i=fds=0; i<ne; i++)
  {
    int q;
    THREADS_ALLOW();
    q = fd_fstat(i,&foo);
    THREADS_DISALLOW();
    if(!q)
    {
      push_int(i);
      fds++;
    }
  }
  f_aggregate(fds);
}

#ifdef HAVE_NOTIFICATIONS

/*! @class File
 */

/*! @decl void notify(void|int notification, function(void:void) callback)
 *! Receive notification when change occur within the fd.
 *! To use, create a Stdio.File object of a directory like
 *! Stdio.File(".") and then call notify() with the appropriate
 *! parameters.
 *!
 *! @note
 *! When a program registers for some notification, only the first notification
 *! will be received unless DN_MULTISHOT is specified as part of the
 *! notification argument.
 *!
 *! @note
 *! At present, this function is Linux-specific and requires a kernel which
 *! supports the F_NOTIFY fcntl() call.
 *!
 *! @param notification
 *! What to notify the callback of. See the Stdio.DN_* constants for more
 *! information about possible notifications.
 *!
 *! @param callback
 *! Function which should be called when notification is received. The
 *! function gets the signal used to indicate the notification as its
 *! argument and shouldn't return anyting.
 */
void file_set_notify(INT32 args) {
  int notifications = 0;

  if (args == 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("notify",2);

  if (args > 2)
    SIMPLE_TOO_FEW_ARGS_ERROR("notify", 2);

  if (args && Pike_sp[-args].type!=PIKE_T_INT)
    SIMPLE_BAD_ARG_ERROR("notify", 0, "int");

  if (args && Pike_sp[1-args].type!=PIKE_T_FUNCTION)
    SIMPLE_BAD_ARG_ERROR("notify", 1, "function(void:void)");

  if (args) {
    notifications = Pike_sp[1-args].u.integer;
  }

#ifdef __linux__
  if (args) {
    pop_n_elems(1);
    push_int(SIGIO);

  }
  fcntl(FD, F_NOTIFY, notifications);
#endif /* __linux__ */

  pop_n_elems(args);
}

/*! @endclass
 */

#endif /* HAVE_NOTIFICATIONS */


/*! @decl constant PROP_BIDIRECTIONAL
 *! @fixme
 *! Document this constant.
 */

/*! @decl constant PROP_BUFFERED
 *! @fixme
 *! Document this constant.
 */

/*! @decl constant PROP_SHUTDOWN
 *! @fixme
 *! Document this constant.
 */

/*! @decl constant PROP_NONBLOCK
 *! @fixme
 *! Document this constant.
 */

/*! @decl constant PROP_REVERSE
 *! @fixme
 *! Document this constant.
 */

/*! @decl constant PROP_IPC
 *! @fixme
 *! Document this constant.
 */

/*! @decl constant IPPROTO
 *! @fixme
 *! Document this constant.
 */

/*! @decl constant __OOB__
 *! Implementation level of nonblocking I/O OOB support.
 *! @int
 *!   @value 0
 *!     Nonblocking OOB support is not supported.
 *!   @value 1
 *!     Nonblocking OOB works a little.
 *!   @value 2
 *!     Nonblocking OOB almost works.
 *!   @value 3
 *!     Nonblocking OOB works as intended.
 *!   @value -1
 *!     Unknown level of nonblocking OOB support.
 *! @endint
 *! This constant only exists when OOB operations are
 *! available, i.e. when @[__HAVE_OOB__] is 1.
 */

/*! @decl constant __HAVE_OOB__
 *!   Exists and has the value 1 if OOB operations are available.
 *!
 *! @note
 *!   In Pike 7.5 and later OOB operations are always present.
 */

void exit_files_stat(void);
void exit_files_efuns(void);
void exit_sendfile(void);
void port_exit_program(void);

PIKE_MODULE_EXIT
{
  exit_files_efuns();
  exit_files_stat();

  exit_sendfile();

  if(file_program)
  {
    free_program(file_program);
    file_program=0;
  }
  if(file_ref_program)
  {
    free_program(file_ref_program);
    file_ref_program=0;
  }
  exit_file_locking();
  if (socketpair_fd >= 0) {
    while (fd_close(socketpair_fd) && errno == EINTR) {}
    socketpair_fd = -1;
  }
  port_exit_program();
}

#define REF (*((struct object **)(Pike_fp->current_storage)))

#define FILE_FUNC(X,Y,Z)					\
  static ptrdiff_t PIKE_CONCAT(Y,_function_number);		\
  void PIKE_CONCAT(Y,_ref) (INT32 args) {			\
    struct object *o=REF;					\
    debug_malloc_touch(o);					\
    if(!o || !o->prog) {					\
      /* This is a temporary kluge */				\
      Pike_error("Stdio.File(): not open.\n");			\
    }								\
    if(o->prog != file_program)					\
      Pike_error("Wrong type of object in Stdio.File->_fd\n");	\
    apply_low(o, PIKE_CONCAT(Y,_function_number), args);	\
  }

#include "file_functions.h"

static void file___init_ref(struct object *o)
{
  REF = file_make_object_from_fd(-1, 0, 0);
}

/* Avoid loss of precision warnings. */
#ifdef __ECL
static inline long TO_LONG(ptrdiff_t x)
{
  return DO_NOT_WARN((long)x);
}
#else /* !__ECL */
#define TO_LONG(x)	((long)(x))
#endif /* __ECL */

#ifdef PIKE_DEBUG
void check_static_file_data(struct callback *a, void *b, void *c)
{
  if(file_program)
  {
#define FILE_FUNC(X,Y,Z)				    \
    if(PIKE_CONCAT(Y,_function_number)<0 ||		    \
       PIKE_CONCAT(Y,_function_number)>			    \
       file_program->num_identifier_references)		    \
      Pike_fatal(#Y "_function_number is incorrect: %ld\n", \
		 TO_LONG(PIKE_CONCAT(Y,_function_number)));
#include "file_functions.h"
  }
}
#endif

#if defined(HAVE_TERMIOS_H)
void file_tcgetattr(INT32 args);
void file_tcsetattr(INT32 args);
void file_tcsendbreak(INT32 args);
void file_tcflush(INT32 args);
/* void file_tcdrain(INT32 args); */
/* void file_tcflow(INT32 args); */
/* void file_tcgetpgrp(INT32 args); */
/* void file_tcsetpgrp(INT32 args); */
#endif

void init_files_efuns(void);
void init_files_stat(void);
void port_setup_program(void);
void init_sendfile(void);
void init_udp(void);

/*! @decl string _sprintf(int type, void|mapping flags)
 */
static void fd__sprintf(INT32 args)
{
  INT_TYPE type;

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("_sprintf",2);
  if(Pike_sp[-args].type!=PIKE_T_INT)
    SIMPLE_BAD_ARG_ERROR("_sprintf",0,"int");

  type = Pike_sp[-args].u.integer;
  pop_n_elems(args);
  switch( type )
  {
    case 'O':
    {
      char buf[20];
      sprintf (buf, "Fd(%d)", FD);
      push_text(buf);
      return;
    }

    case 't':
    {
      push_text("Fd");
      return;
    }
  }
  push_int( 0 );
  Pike_sp[-1].subtype = 1;
}


/*! @decl mapping(string:mapping) gethostip()
 *!
 *! Returns the IP addresses of the host.
 *!
 *! @returns
 *!   Returns a mapping that maps interface name to a mapping with
 *!   more information about that interface. That information mapping
 *!   looks as follows.
 *!   @mapping
 *!     @member array(string) "ips"
 *!       A list of all IP addresses bound to this interface.
 *!   @endmapping
 */

#define INTERFACES 256

static void f_gethostip(INT32 args) {
  int fd, i, j, up = 0;
  struct mapping *m;

  pop_n_elems(args);

  m = allocate_mapping(2);

#if defined(HAVE_LINUX_IF_H) && defined(HAVE_SYS_IOCTL_H)
  {
    struct ifconf ifc;
    struct sockaddr_in addr;
    char buffer[ INTERFACES * sizeof( struct ifreq ) ];
    struct svalue *sval;

    fd = fd_socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( fd < 0 ) Pike_error("gethostip: Failed to open socket.\n");

    ifc.ifc_len = sizeof( buffer );
    ifc.ifc_ifcu.ifcu_buf = (caddr_t)buffer;
    if( ioctl( fd, SIOCGIFCONF, &ifc ) < 0 )
      Pike_error("gethostip: Query failed.\n");

    for( i=0; i<ifc.ifc_len; ) {
      struct ifreq *ifr = (struct ifreq *)( (caddr_t)ifc.ifc_req+i );
      struct ifreq ifr2;
      i += sizeof(struct ifreq);

      strcpy( ifr2.ifr_name, ifr->ifr_name );
      if( ioctl( fd, SIOCGIFFLAGS, &ifr2 )<0 )
	Pike_error("gethostip: Query failed.\n");

      if( (ifr2.ifr_flags & IFF_LOOPBACK) ||
	  !(ifr2.ifr_flags & IFF_UP) ||
	  (ifr->ifr_addr.sa_family != AF_INET ) )
	continue;

      sval = simple_mapping_string_lookup( m, ifr->ifr_name );
      if( !sval ) {

	push_text( ifr->ifr_name );

	push_constant_text( "ips" );
	memcpy( &addr, &ifr->ifr_addr, sizeof(ifr->ifr_addr) );
	push_text( inet_ntoa( addr.sin_addr ) );
	f_aggregate(1);

	f_aggregate_mapping(2);
	mapping_insert(m, &Pike_sp[-2], &Pike_sp[-1]);
	pop_n_elems(2);
      }

      up++;
    }

    fd_close(fd);
  }
#endif /* defined(HAVE_LINUX_IF_H) && defined(HAVE_SYS_IOCTL_H) */

  push_mapping(m);
}

#ifdef HAVE_OPENPTY
#include <pty.h>
#endif

PIKE_MODULE_INIT
{
  struct object *o;

  Pike_compiler->new_program->id = PROG_MODULE_FILES_ID;

  init_files_efuns();
  init_files_stat();

  START_NEW_PROGRAM_ID(STDIO_FD);
  ADD_STORAGE(struct my_file);

#define FILE_FUNC(X,Y,Z)					\
  PIKE_CONCAT(Y,_function_number) = ADD_FUNCTION(X,Y,Z,0);
#define FILE_OBJ tObjImpl_STDIO_FD
#include "file_functions.h"

  MAP_VARIABLE("_read_callback",tMix,0,
	       OFFSETOF(my_file, event_cbs[PIKE_FD_READ]),PIKE_T_MIXED);
  MAP_VARIABLE("_write_callback",tMix,0,
	       OFFSETOF(my_file, event_cbs[PIKE_FD_WRITE]),PIKE_T_MIXED);
  MAP_VARIABLE("_read_oob_callback",tMix,0,
	       OFFSETOF(my_file, event_cbs[PIKE_FD_READ_OOB]),PIKE_T_MIXED);
  MAP_VARIABLE("_write_oob_callback",tMix,0,
	       OFFSETOF(my_file, event_cbs[PIKE_FD_WRITE_OOB]),PIKE_T_MIXED);

  /* function(int, void|mapping:string) */
  ADD_FUNCTION("_sprintf",fd__sprintf,
	       tFunc(tInt tOr(tVoid,tMapping),tString),0);

  init_file_locking();
  pike_set_prog_event_callback(file_handle_events);

  file_program=end_program();
  add_program_constant("Fd",file_program,0);

  o=file_make_object_from_fd(0, FILE_READ , fd_CAN_NONBLOCK);
  ((struct my_file *)(o->storage))->flags |= FILE_NO_CLOSE_ON_DESTRUCT;
  dmalloc_register_fd(0);
  dmalloc_accept_leak_fd(0);
  add_object_constant("_stdin",o,0);
  free_object(o);

  o=file_make_object_from_fd(1, FILE_WRITE, fd_CAN_NONBLOCK);
  ((struct my_file *)(o->storage))->flags |= FILE_NO_CLOSE_ON_DESTRUCT;
  dmalloc_register_fd(1);
  dmalloc_accept_leak_fd(1);
  add_object_constant("_stdout",o,0);
  free_object(o);

  o=file_make_object_from_fd(2, FILE_WRITE, fd_CAN_NONBLOCK);
  ((struct my_file *)(o->storage))->flags |= FILE_NO_CLOSE_ON_DESTRUCT;
  dmalloc_register_fd(2);
  dmalloc_accept_leak_fd(2);
  add_object_constant("_stderr",o,0);
  free_object(o);

  START_NEW_PROGRAM_ID (STDIO_FD_REF);
  ADD_STORAGE(struct object *);
  MAP_VARIABLE("_fd", tObj, 0, 0, PIKE_T_OBJECT);
  set_init_callback(file___init_ref);

#define FILE_FUNC(X,Y,Z)			\
  ADD_FUNCTION(X, PIKE_CONCAT(Y,_ref), Z, 0);
#define FILE_OBJ tObjImpl_STDIO_FD_REF
#include "file_functions.h"

  file_ref_program=end_program();
  add_program_constant("Fd_ref",file_ref_program,0);

  port_setup_program();
  init_sendfile();
  init_udp();

#if defined(HAVE_FSETXATTR)
  add_integer_constant("XATTR_CREATE", XATTR_CREATE, 0 );
  add_integer_constant("XATTR_REPLACE", XATTR_REPLACE, 0 );
#endif
  add_integer_constant("PROP_IPC",fd_INTERPROCESSABLE,0);
  add_integer_constant("PROP_NONBLOCK",fd_CAN_NONBLOCK,0);
  add_integer_constant("PROP_SHUTDOWN",fd_CAN_SHUTDOWN,0);
  add_integer_constant("PROP_BUFFERED",fd_BUFFERED,0);
  add_integer_constant("PROP_BIDIRECTIONAL",fd_BIDIRECTIONAL,0);
  add_integer_constant("PROP_REVERSE",fd_REVERSE,0);

  add_integer_constant("PROP_IS_NONBLOCKING", FILE_NONBLOCKING, 0);

#ifdef DN_ACCESS
  /*! @decl constant DN_ACCESS
   *! Used in @[File.notify()] to get a callback when files
   *! within a directory are accessed.
   */
  add_integer_constant("DN_ACCESS", DN_ACCESS, 0);
#endif

#ifdef DN_MODIFY
  /*! @decl constant DN_MODIFY
   *! Used in @[File.notify()] to get a callback when files
   *! within a directory are modified.
   */
  add_integer_constant("DN_MODIFY", DN_MODIFY, 0);
#endif

#ifdef DN_CREATE
  /*! @decl constant DN_CREATE
   *! Used in @[File.notify()] to get a callback when new
   *! files are created within a directory.
   */
  add_integer_constant("DN_CREATE", DN_CREATE, 0);
#endif

#ifdef DN_DELETE
  /*! @decl constant DN_DELETE
   *! Used in @[File.notify()] to get a callback when files
   *! are deleted within a directory.
   */
  add_integer_constant("DN_DELETE", DN_DELETE, 0);
#endif

#ifdef DN_RENAME
  /*! @decl constant DN_RENAME
   *! Used in @[File.notify()] to get a callback when files
   *! within a directory are renamed.
   */
  add_integer_constant("DN_RENAME", DN_RENAME, 0);
#endif

#ifdef DN_ATTRIB
  /*! @decl constant DN_ATTRIB
   *! Used in @[File.notify()] to get a callback when attributes
   *! of files within a directory are changed.
   */
  add_integer_constant("DN_ATTRIB", DN_ATTRIB, 0);
#endif

#ifdef DN_MULTISHOT
  /*! @decl constant DN_MULTISHOT
   *! Used in @[File.notify()]. If DN_MULTISHOT is used, signals will
   *! be sent for all notifications the program has registred for. Otherwise
   *! only the first event the program is listening for will be received and
   *! then the program must reregister for the events to receive futher events.
   */
  add_integer_constant("DN_MULTISHOT", DN_MULTISHOT, 0);
#endif

  add_integer_constant("__HAVE_OOB__",1,0);
#ifdef PIKE_OOB_WORKS
  add_integer_constant("__OOB__",PIKE_OOB_WORKS,0);
#else
  add_integer_constant("__OOB__",-1,0); /* unknown */
#endif

#ifdef HAVE_SYS_UN_H
  add_integer_constant("__HAVE_CONNECT_UNIX__",1,0);
#endif

#if !defined(__NT__) && (defined(HAVE_POSIX_OPENPT) || defined(PTY_MASTER_PATHNAME))
  add_integer_constant("__HAVE_OPENPT__",1,0);
#endif

  /* function(:array(int)) */
  ADD_FUNCTION2("get_all_active_fd", f_get_all_active_fd,
		tFunc(tNone,tArr(tInt)), 0, OPT_EXTERNAL_DEPEND);

  /* function(void:mapping) */
  ADD_FUNCTION2("gethostip", f_gethostip, tFunc(tNone,tMapping),
		0, OPT_EXTERNAL_DEPEND);

#ifdef PIKE_DEBUG
  dmalloc_accept_leak(add_to_callback(&do_debug_callbacks,
				      check_static_file_data,
				      0,
				      0));
#endif
}

/*! @endmodule
 */

/* Used from backend */
int pike_make_pipe(int *fds)
{
  return socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
}

int fd_from_object(struct object *o)
{
  extern int fd_from_portobject( struct object *o );
  struct my_file *f=get_file_storage(o);
  if(!f)
    return fd_from_portobject( o );
  return f->box.fd;
}
