/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#define NO_PIKE_SHORTHAND
#include "global.h"
RCSID("$Id: file.c,v 1.218 2004/11/15 22:53:29 mast Exp $");
#include "fdlib.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "object.h"
#include "pike_macros.h"
#include "backend.h"
#include "fd_control.h"
#include "module_support.h"
#include "opcodes.h"
#include "operators.h"
#include "security.h"
#include "bignum.h"

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

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif /* HAVE_SYS_UIO_H */

#ifdef HAVE_WINSOCK_H
#  include <winsock.h>
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
#define FD (THIS->fd)
#define ERRNO (THIS->my_errno)

#define READ_BUFFER 8192

/* Don't try to use socketpair() on AmigaOS, socketpair_ultra works better */
#ifdef __amigaos__
#undef HAVE_SOCKETPAIR
#endif

/* #define SOCKETPAIR_DEBUG */

struct program *file_program;
struct program *file_ref_program;

static void file_read_callback(int fd, void *data);
static void file_write_callback(int fd, void *data);

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
#ifdef WITH_OOB
#define OOBOP(X) X
#else
#define OOBOP(X)
#endif
#define CHECK_FILEP(o) \
do { if(o->prog && !get_storage(o,file_program)) fatal("%p is not a file object.\n",o); } while (0)
#define DEBUG_CHECK_INTERNAL_REFERENCE(X) do {				\
  if( ((X)->fd!=-1 && (							\
     (query_read_callback((X)->fd)==file_read_callback) ||		\
     (query_write_callback((X)->fd)==file_write_callback)		\
OOBOP( || (query_read_oob_callback((X)->fd)==file_read_oob_callback) ||	\
     (query_write_oob_callback((X)->fd)==file_write_oob_callback) ))) != \
  !!( (X)->flags & FILE_HAS_INTERNAL_REF ))				\
         fatal("Internal reference is wrong. %d\n",(X)->flags & FILE_HAS_INTERNAL_REF);		\
   } while (0)
#else
#define CHECK_FILEP(o)
#define DEBUG_CHECK_INTERNAL_REFERENCE(X)
#endif
#define SET_INTERNAL_REFERENCE(f) \
  do { CHECK_FILEP(f->myself); if(!(f->flags & FILE_HAS_INTERNAL_REF)) { add_ref(f->myself); f->flags|=FILE_HAS_INTERNAL_REF; } }while (0)

#define REMOVE_INTERNAL_REFERENCE(f) \
  do { CHECK_FILEP(f->myself); if(f->flags & FILE_HAS_INTERNAL_REF) {  f->flags&=~FILE_HAS_INTERNAL_REF; free_object(f->myself); } }while (0)

static void check_internal_reference(struct my_file *f)
{
  if(f->fd!=-1)
  {
    if(query_read_callback(f->fd) == file_read_callback ||
       query_write_callback(f->fd) == file_write_callback
#ifdef WITH_OOB
       || query_read_oob_callback(f->fd) == file_read_oob_callback ||
       query_write_oob_callback(f->fd) == file_write_oob_callback
#endif
       )
       {
	 SET_INTERNAL_REFERENCE(f);
	 return;
       }
  }

  REMOVE_INTERNAL_REFERENCE(f);
}

static void init_fd(int fd, int open_mode)
{
  FD=fd;
  ERRNO=0;
  REMOVE_INTERNAL_REFERENCE(THIS);
  THIS->flags=0;
  THIS->open_mode=open_mode;
  THIS->read_callback.type=PIKE_T_INT;
  THIS->read_callback.u.integer=0;
  THIS->write_callback.type=PIKE_T_INT;
  THIS->write_callback.u.integer=0;
#ifdef WITH_OOB
  THIS->read_oob_callback.type=PIKE_T_INT;
  THIS->read_oob_callback.u.integer=0;
  THIS->write_oob_callback.type=PIKE_T_INT;
  THIS->write_oob_callback.u.integer=0;
#endif /* WITH_OOB */
#if defined(HAVE_FD_FLOCK) || defined(HAVE_FD_LOCKF)
  THIS->key=0;
#endif
}


void reset_variables(void)
{
  free_svalue(& THIS->read_callback);
  THIS->read_callback.type=PIKE_T_INT;
  free_svalue(& THIS->write_callback);
  THIS->write_callback.type=PIKE_T_INT;
#ifdef WITH_OOB
  free_svalue(& THIS->read_oob_callback);
  THIS->read_oob_callback.type=PIKE_T_INT;
  free_svalue(& THIS->write_oob_callback);
  THIS->write_oob_callback.type=PIKE_T_INT;
#endif /* WITH_OOB */
}

static void free_fd_stuff(void)
{
#if defined(HAVE_FD_FLOCK) || defined(HAVE_FD_LOCKF)
  if(THIS->key)
  {
    destruct(THIS->key);
    THIS->key=0;
  }
#endif
  check_internal_reference(THIS);
}

static void close_fd_quietly(void)
{
  int fd=FD;
  if(fd<0) return;

  set_read_callback(FD,0,0);
  set_write_callback(FD,0,0);
#ifdef WITH_OOB
  set_read_oob_callback(FD,0,0);
  set_write_oob_callback(FD,0,0);
#endif /* WITH_OOB */
  check_internal_reference(THIS);

  FD=-1;
  while(1)
  {
    int i, e;
    THREADS_ALLOW_UID();
    i=fd_close(fd);
    e=errno;
    THREADS_DISALLOW_UID();

    if(i < 0)
    {
      switch(e)
      {
	default:
	{
	  /* Delay close until next gc() */
	  struct object *o=file_make_object_from_fd(fd,0,0);
	  ((struct my_file *)(o->storage))->read_callback.u.object=o;
	  ((struct my_file *)(o->storage))->read_callback.type=PIKE_T_OBJECT;
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

static void just_close_fd(void)
{
  int fd=FD;
  if(fd<0) return;

  set_read_callback(FD,0,0);
  set_write_callback(FD,0,0);
#ifdef WITH_OOB
  set_read_oob_callback(FD,0,0);
  set_write_oob_callback(FD,0,0);
#endif /* WITH_OOB */
  check_internal_reference(THIS);

  FD=-1;
  while(1)
  {
    int i, e;
    THREADS_ALLOW_UID();
    i=fd_close(fd);
    e=errno;
    THREADS_DISALLOW_UID();

    if(i < 0)
    {
      switch(e)
      {
	default:
	  ERRNO=errno=e;
	  FD=fd;
	  push_int(e);
	  f_strerror(1);
	  Pike_error("Failed to close file: %s\n", Pike_sp[-1].u.string->str);
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

static void close_fd(void)
{
  free_fd_stuff();
  reset_variables();
  if ( (THIS->flags & FILE_NOT_OPENED) )
  {
     FD=-1;
     return;
  }
  just_close_fd();
}

void my_set_close_on_exec(int fd, int to)
{
  set_close_on_exec(fd, to);
}

void do_set_close_on_exec(void)
{
}

/* Parse "rw" to internal flags. Stdio.pmod counts on that unknown
 * characters are ignored. */
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
  return ret;
}

static void call_free(char *s) { free(s); }
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

      check_signals(0,0,0);

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

    if(!IS_ZERO(& THIS->read_callback))
    {
      set_read_callback(FD, file_read_callback, THIS);
      SET_INTERNAL_REFERENCE(THIS);
    }

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

      check_signals(0,0,0);

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

    if(!IS_ZERO(& THIS->read_callback))
    {
      set_read_callback(FD, file_read_callback, THIS);
      SET_INTERNAL_REFERENCE(THIS);
    }

    return low_free_buf(&b);
#undef CHUNK
  }
}

#ifdef WITH_OOB
static struct pike_string *do_read_oob(int fd,
				       INT32 r,
				       int all,
				       int *err)
{
  ONERROR ebuf;
  INT32 bytes_read,i;
  bytes_read=0;
  *err=0;

  if(r <= 65536)
  {
    struct pike_string *str;

    str=begin_shared_string(r);

    SET_ONERROR(ebuf, do_really_free_pike_string, str);

    do{
      int e;
      int fd=FD;
      THREADS_ALLOW();
      i=fd_recv(fd, str->str+bytes_read, r, MSG_OOB);
      e=errno;
      THREADS_DISALLOW();

      check_signals(0,0,0);

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

    if(!IS_ZERO(& THIS->read_oob_callback))
    {
      set_read_oob_callback(FD, file_read_oob_callback, THIS);
      SET_INTERNAL_REFERENCE(THIS);
    }

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
      i=fd_recv(fd, buf, try_read, MSG_OOB);
      e=errno;
      THREADS_DISALLOW();

      check_signals(0,0,0);

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
    if(!IS_ZERO(& THIS->read_oob_callback))
    {
      set_read_oob_callback(FD, file_read_oob_callback, THIS);
      SET_INTERNAL_REFERENCE(THIS);
    }
    return low_free_buf(&b);
#undef CHUNK
  }
}
#endif /* WITH_OOB */

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
      Pike_error("Bad argument 1 to file->read().\n");
    len=Pike_sp[-args].u.integer;
    if(len<0)
      Pike_error("Cannot read negative number of characters.\n");
  }

  if(args > 1 && !IS_ZERO(Pike_sp+1-args))
  {
    all=0;
  }else{
    all=1;
  }

  pop_n_elems(args);

  if((tmp=do_read(FD, len, all, & ERRNO)))
    push_string(tmp);
  else
    push_int(0);
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
static void file_peek(INT32 args)
{
#ifdef HAVE_AND_USE_POLL
  struct pollfd fds;
  int ret;

  fds.fd=THIS->fd;
  fds.events=POLLIN;
  fds.revents=0;

  THREADS_ALLOW();
  ret=poll(&fds, 1, 1);
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
  fd_FD_SET(ret=THIS->fd, &tmp);

  THREADS_ALLOW();
  ret=select(ret+1,&tmp,0,0,&tv);
  THREADS_DISALLOW();

  if(ret < 0)
  {
    ERRNO=errno;
    ret=-1;
  }else{
    ret = (ret > 0) && fd_FD_ISSET(THIS->fd, &tmp);
  }
#endif
  pop_n_elems(args);
  push_int(ret);
}
#endif

#ifdef WITH_OOB
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
      Pike_error("Bad argument 1 to file->read_oob().\n");
    len=Pike_sp[-args].u.integer;
    if(len<0)
      Pike_error("Cannot read negative number of characters.\n");
  }

  if(args > 1 && !IS_ZERO(Pike_sp+1-args))
  {
    all=0;
  }else{
    all=1;
  }

  pop_n_elems(args);

  if((tmp=do_read_oob(FD, len, all, & ERRNO)))
    push_string(tmp);
  else
    push_int(0);
}
#endif /* WITH_OOB */

#undef CBFUNCS
#define CBFUNCS(X)						\
static void PIKE_CONCAT(file_,X) (int fd, void *data)		\
{								\
  struct my_file *f=(struct my_file *)data;			\
  PIKE_CONCAT(set_,X)(fd, 0, 0);				\
  check_internal_reference(f);                                  \
  check_destructed(& f->X );				        \
  if(!IS_ZERO(& f->X )) {                                       \
    apply_svalue(& f->X, 0);					\
    pop_stack();						\
  }     							\
}								\
								\
static void PIKE_CONCAT(file_set_,X) (INT32 args)		\
{								\
  if(FD<0)							\
    Pike_error("File is not open.\n");				\
  if(!args)							\
    Pike_error("Too few arguments to file_set_%s\n",#X);		\
  assign_svalue(& THIS->X, Pike_sp-args);				\
  if(IS_ZERO(Pike_sp-args))						\
  {								\
    PIKE_CONCAT(set_,X)(FD, 0, 0);				\
    check_internal_reference(THIS);                             \
  }else{							\
    PIKE_CONCAT(set_,X)(FD, PIKE_CONCAT(file_,X), THIS);	\
    SET_INTERNAL_REFERENCE(THIS);                               \
  }								\
}								\
								\
static void PIKE_CONCAT(file_query_,X) (INT32 args)		\
{								\
  pop_n_elems(args);						\
  push_svalue(& THIS->X);					\
}

CBFUNCS(read_callback)
CBFUNCS(write_callback)
#ifdef WITH_OOB
CBFUNCS(read_oob_callback)
CBFUNCS(write_oob_callback)
#endif

static void file__enable_callbacks(INT32 args)
{
  if(FD<0)
    Pike_error("File is not open.\n");

#define DO_TRIGGER(X) \
  if(IS_ZERO(& THIS->X )) \
  {								\
    PIKE_CONCAT(set_,X)(FD, 0, 0);				\
  }else{							\
    PIKE_CONCAT(set_,X)(FD, PIKE_CONCAT(file_,X), THIS);	\
  }

DO_TRIGGER(read_callback)
DO_TRIGGER(write_callback)
#ifdef WITH_OOB
DO_TRIGGER(read_oob_callback)
DO_TRIGGER(write_oob_callback)
#endif

  check_internal_reference(THIS);
  pop_n_elems(args);
  push_int(0);
}

static void file__disable_callbacks(INT32 args)
{
  if(FD<0)
    Pike_error("File is not open.\n");
#define DO_DISABLE(X) \
  PIKE_CONCAT(set_,X)(FD, 0, 0);


DO_DISABLE(read_callback)
DO_DISABLE(write_callback)
#ifdef WITH_OOB
DO_DISABLE(read_oob_callback)
DO_DISABLE(write_oob_callback)
#endif

  check_internal_reference(THIS);

  pop_n_elems(args);
  push_int(0);
}


static void file_write(INT32 args)
{
  ptrdiff_t written, i;
  struct pike_string *str;

  if(args<1 || ((Pike_sp[-args].type != PIKE_T_STRING) && (Pike_sp[-args].type != PIKE_T_ARRAY)))
    Pike_error("Bad argument 1 to file->write().\n"
	  "Type is %s. Expected string or array(string)\n",
	  get_name_of_type(Pike_sp[-args].type));

  if(FD < 0)
    Pike_error("File not open for write.\n");

  if (Pike_sp[-args].type == PIKE_T_ARRAY) {
    struct array *a = Pike_sp[-args].u.array;
    i = a->size;
    while(i--) {
      if (a->item[i].type != PIKE_T_STRING) {
	Pike_error("Bad argument 1 to file->write().\n"
	      "Element %ld is not a string.\n",
	      DO_NOT_WARN((long)i));
      } else if (a->item[i].u.string->size_shift) {
	Pike_error("Bad argument 1 to file->write().\n"
	      "Element %ld is a wide string.\n",
	      DO_NOT_WARN((long)i));
      }
    }

#ifdef HAVE_WRITEV
    if (args > 1) {
#endif /* HAVE_WRITEV */
      ref_push_array(a);
      push_constant_text("");
      o_multiply();
      Pike_sp--;
      dmalloc_touch_svalue(Pike_sp);
      Pike_sp[-args] = *Pike_sp;
      free_array(a);

#ifdef PIKE_DEBUG
      if (Pike_sp[-args].type != PIKE_T_STRING) {
	Pike_error("Bad return value from string multiplication.");
      }
#endif /* PIKE_DEBUG */
#ifdef HAVE_WRITEV
    } else if (!a->size) {
      /* Special case for empty array */
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

#ifdef _REENTRANT
	if (FD<0) {
	  free(iovbase);
	  Pike_error("File closed/destructed while in file->write.\n");
	}
#endif
	if(i<0)
	{
	  /* perror("writev"); */
	  switch(errno)
	  {
	  default:
	    free(iovbase);
	    ERRNO=errno;
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

      if(!IS_ZERO(& THIS->write_callback))
      {
	set_write_callback(FD, file_write_callback, THIS);
	SET_INTERNAL_REFERENCE(THIS);
      }
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

    check_signals(0,0,0);

#ifdef _REENTRANT
    if(FD<0) Pike_error("File closed/destructed while in file->write.\n");
#endif

    if(i<0)
    {
      switch(e)
      {
      default:
	ERRNO=e;
	pop_n_elems(args);
	if (!written) {
	  push_int(-1);
	} else {
	  push_int64(written);
	}
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

  if(!IS_ZERO(& THIS->write_callback))
  {
    set_write_callback(FD, file_write_callback, THIS);
    SET_INTERNAL_REFERENCE(THIS);
  }
  ERRNO=0;

  pop_n_elems(args);
  push_int64(written);
}

#ifdef WITH_OOB
static void file_write_oob(INT32 args)
{
  ptrdiff_t written, i;
  struct pike_string *str;

  if(args<1 || Pike_sp[-args].type != PIKE_T_STRING)
    Pike_error("Bad argument 1 to file->write().\n");

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

#ifdef _REENTRANT
    if(FD<0) Pike_error("File destructed while in file->write_oob.\n");
#endif

    if(i<0)
    {
      switch(e)
      {
      default:
	ERRNO=e;
	pop_n_elems(args);
	if (!written) {
	  push_int(-1);
	} else {
	  push_int64(written);
	}
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

  if(!IS_ZERO(& THIS->write_oob_callback))
  {
    set_write_oob_callback(FD, file_write_oob_callback, THIS);
    SET_INTERNAL_REFERENCE(THIS);
  }

  ERRNO=0;

  pop_n_elems(args);
  push_int64(written);
}
#endif /* WITH_OOB */

static int do_close(int flags)
{
  if(FD == -1) return 1; /* already closed */
  ERRNO=0;

  flags &= THIS->open_mode;

  switch(flags & (FILE_READ | FILE_WRITE))
  {
  case 0:
    return 0;

  case FILE_READ:
    if(THIS->open_mode & FILE_WRITE)
    {
      set_read_callback(FD,0,0);
#ifdef WITH_OOB
      set_read_oob_callback(FD,0,0);
#endif /* WITH_OOB */
      fd_shutdown(FD, 0);
      THIS->open_mode &=~ FILE_READ;
      check_internal_reference(THIS);
      return 0;
    }else{
      THIS->flags&=~FILE_NOT_OPENED;
      close_fd();
      return 1;
    }

  case FILE_WRITE:
    if(THIS->open_mode & FILE_READ)
    {
      set_write_callback(FD,0,0);
#ifdef WITH_OOB
      set_write_oob_callback(FD,0,0);
#endif /* WITH_OOB */
      fd_shutdown(FD, 1);
      THIS->open_mode &=~ FILE_WRITE;
      check_internal_reference(THIS);
      return 0;
    }else{
      THIS->flags&=~FILE_NOT_OPENED;
      close_fd();
      return 1;
    }

  case FILE_READ | FILE_WRITE:
    THIS->flags&=~FILE_NOT_OPENED;
    close_fd();
    return 1;

  default:
    fatal("Bug in switch implementation!\n");
    return 0; /* Make CC happy */
  }
}

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

static void file_open(INT32 args)
{
  int flags,fd;
  int access;
  int err;
  struct pike_string *str, *flag_str;
  close_fd();

  if(args < 2)
    Pike_error("Too few arguments to file->open()\n");

  if(Pike_sp[-args].type != PIKE_T_STRING &&
     Pike_sp[-args].type != PIKE_T_INT)
    Pike_error("Bad argument 1 to file->open()\n");

  if(Pike_sp[1-args].type != PIKE_T_STRING)
    Pike_error("Bad argument 2 to file->open()\n");

  if (args > 2)
  {
    if (Pike_sp[2-args].type != PIKE_T_INT)
      Pike_error("Bad argument 3 to file->open()\n");
    access = Pike_sp[2-args].u.integer;
  } else
    access = 00666;

  flags = parse((flag_str = Pike_sp[1-args].u.string)->str);

  if (Pike_sp[-args].type==PIKE_T_STRING)
  {
     str=Pike_sp[-args].u.string;

     if (strlen(str->str) != (size_t)str->len) {
       /* Filenames with NUL are not supported. */
       ERRNO = ENOENT;
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

	safe_apply(OBJ2CREDS(Pike_interpreter.current_creds)->user,"valid_open",5);
	switch(Pike_sp[-1].type)
	{
	   case PIKE_T_INT:
	      switch(Pike_sp[-1].u.integer)
	      {
		 case 0: /* return 0 */
		    ERRNO=EPERM;
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
#ifdef DEBUG_MALLOC
       extern int d_flag;
       /* This is a temporary kluge */
       if(d_flag)
       {
	 fprintf(stderr,"Possible gc() failure detected in open()\n");
	 describe(Pike_fp->current_object);
       }
#endif
       if (fd >= 0)
	 while (fd_close(fd) && errno == EINTR) {}
       Pike_error("Object destructed in file->open()\n");
     }

     if(fd < 0)
     {
	ERRNO=err;
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
     THIS->flags|=FILE_NOT_OPENED;
  }


  pop_n_elems(args);
  push_int(fd>=0);
}

#ifdef HAVE_FSYNC
static void file_sync(INT32 args)
{
  int ret;

  if(FD < 0)
    Pike_error("File not open.\n");

  pop_n_elems(args);

  do {
    ret = fsync(FD);
  } while ((ret < 0) && (errno == EINTR));

  if (ret < 0) {
    ERRNO = errno;
    push_int(0);
  } else {
    push_int(1);
  }
}
#endif /* HAVE_FSYNC */

static void file_seek(INT32 args)
{
#ifdef HAVE_LSEEK64
  INT64 to = 0;
#else
  ptrdiff_t to = 0;
#endif

#ifdef HAVE_LSEEK64
#ifdef AUTO_BIGNUM
  if(1 <= args && is_bignum_object_in_svalue(&Pike_sp[-args]))
    int64_from_bignum(&to, Pike_sp[-args].u.object);
  else
#endif /* AUTO_BIGNUM */
#endif
  if(args<1 || Pike_sp[-args].type != PIKE_T_INT)
    Pike_error("Bad argument 1 to file->seek(int to).\n");
  else
    to=Pike_sp[-args].u.integer;

  if(FD < 0)
    Pike_error("File not open.\n");

  if(args>1)
  {
    if(Pike_sp[-args+1].type != PIKE_T_INT)
      Pike_error("Bad argument 2 to file->seek(int unit,int mult).\n");
    to *= Pike_sp[-args+1].u.integer;
  }
  if(args>2)
  {
    if(Pike_sp[-args+2].type != PIKE_T_INT)
      Pike_error("Bad argument 3 to file->seek(int unit,int mult,int add).\n");
    to += Pike_sp[-args+2].u.integer;
  }

  ERRNO=0;

#ifdef HAVE_LSEEK64
  to = lseek64(FD,to,to<0 ? SEEK_END : SEEK_SET);
#else
  to = fd_lseek(FD,to,to<0 ? SEEK_END : SEEK_SET);
#endif
  if(to<0) ERRNO=errno;

  pop_n_elems(args);
  push_int64(to);
}

static void file_tell(INT32 args)
{
#ifdef HAVE_LSEEK64
  INT64 to;
#else
  ptrdiff_t to;
#endif

  if(FD < 0)
    Pike_error("File not open.\n");

  ERRNO=0;
#ifdef HAVE_LSEEK64
  to = lseek64(FD, 0L, SEEK_CUR);
#else
  to = fd_lseek(FD, 0L, SEEK_CUR);
#endif

  if(to<0) ERRNO=errno;

  pop_n_elems(args);
  push_int64(to);
}

static void file_truncate(INT32 args)
{
#ifdef HAVE_LSEEK64
  INT64 len;
#else
  INT32 len;
#endif
  int res;

  if(args<1 || Pike_sp[-args].type != PIKE_T_INT)
    Pike_error("Bad argument 1 to file->truncate(int length).\n");

  if(FD < 0)
    Pike_error("File not open.\n");

  len = Pike_sp[-args].u.integer;

  ERRNO=0;
  res=fd_ftruncate(FD, len);

  pop_n_elems(args);

  if(res<0)
     ERRNO=errno;

  push_int(!res);
}

static void file_stat(INT32 args)
{
  int fd;
  struct stat s;
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

static void file_errno(INT32 args)
{
  pop_n_elems(args);
  push_int(ERRNO);
}

static void file_mode(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->open_mode);
}

static void file_set_nonblocking(INT32 args)
{
  if(FD < 0) Pike_error("File not open.\n");

  if(!(THIS->open_mode & fd_CAN_NONBLOCK))
    Pike_error("That file does not support nonblocking operation.\n");

  if(set_nonblocking(FD,1))
  {
    ERRNO=errno;
    Pike_error("Stdio.File->set_nonbloblocking() failed.\n");
  }

  THIS->open_mode |= FILE_NONBLOCKING;
}


static void file_set_blocking(INT32 args)
{
  if(FD >= 0)
  {
    set_nonblocking(FD,0);
    THIS->open_mode &=~ FILE_NONBLOCKING;
  }
  pop_n_elems(args);
}

static void file_set_close_on_exec(INT32 args)
{
  if(args < 0)
    Pike_error("Too few arguments to file->set_close_on_exec()\n");
  if(FD <0)
    Pike_error("File not open.\n");

  if(IS_ZERO(Pike_sp-args))
  {
    my_set_close_on_exec(FD,0);
  }else{
    my_set_close_on_exec(FD,1);
  }
  pop_n_elems(args-1);
}

static void file_query_fd(INT32 args)
{
  if(FD < 0)
    Pike_error("File not open.\n");

  pop_n_elems(args);
  push_int(FD);
}

struct object *file_make_object_from_fd(int fd, int mode, int guess)
{
  struct object *o=low_clone(file_program);
  call_c_initializers(o);
  ((struct my_file *)(o->storage))->fd=fd;
  ((struct my_file *)(o->storage))->open_mode=mode | fd_query_properties(fd, guess);
  return o;
}

static void file_set_buffer(INT32 args)
{
  INT32 bufsize;
  int flags;

  if(FD==-1)
    Pike_error("file->set_buffer() on closed file.\n");
  if(!args)
    Pike_error("Too few arguments to file->set_buffer()\n");
  if(Pike_sp[-args].type!=PIKE_T_INT)
    Pike_error("Bad argument 1 to file->set_buffer()\n");

  bufsize=Pike_sp[-args].u.integer;
  if(bufsize < 0)
    Pike_error("Bufsize must be larger than zero.\n");

  if(args>1)
  {
    if(Pike_sp[1-args].type != PIKE_T_STRING)
      Pike_error("Bad argument 2 to file->set_buffer()\n");
    flags=parse(Pike_sp[1-args].u.string->str);
  }else{
    flags=FILE_READ | FILE_WRITE;
  }

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
    if(errno != EWOULDBLOCK)
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

static void file_pipe(INT32 args)
{
  int inout[2] = { -1, -1 };
  int i = 0;

  int type=fd_CAN_NONBLOCK | fd_BIDIRECTIONAL;

  check_all_args("file->pipe",args, BIT_INT | BIT_VOID, 0);
  if(args) type = Pike_sp[-args].u.integer;

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
    errno=ERRNO;
    push_int(0);
  }
  else
  {
    init_fd(inout[0],FILE_READ | (type&fd_BIDIRECTIONAL?FILE_WRITE:0) |
	    fd_query_properties(inout[0], type));

    my_set_close_on_exec(inout[0],1);
    my_set_close_on_exec(inout[1],1);
    FD=inout[0];

    ERRNO=0;
    push_object(file_make_object_from_fd(inout[1], (type&fd_BIDIRECTIONAL?FILE_READ:0)| FILE_WRITE,type));
  }
}

static void init_file_struct(struct object *o)
{
  FD=-1;
  ERRNO=0;
  THIS->open_mode=0;
  THIS->flags=0;
#if defined(HAVE_FD_FLOCK) || defined(HAVE_FD_LOCKF)
  THIS->key=0;
#endif /* HAVE_FD_FLOCK */
  THIS->myself=o;
  /* map_variable will take care of the rest */
}

static void exit_file_struct(struct object *o)
{
  if(!(THIS->flags & (FILE_NO_CLOSE_ON_DESTRUCT |
		      FILE_LOCK_FD |
		      FILE_NOT_OPENED)))
    close_fd_quietly();
  free_fd_stuff();

  REMOVE_INTERNAL_REFERENCE(THIS);
  /* map_variable will free callbacks */
}

#ifdef PIKE_DEBUG
static void gc_mark_file_struct(struct object *o)
{
  DEBUG_CHECK_INTERNAL_REFERENCE(THIS);
}
#endif

static void low_dup(struct object *toob,
		    struct my_file *to,
		    struct my_file *from)
{
  my_set_close_on_exec(to->fd, to->fd > 2);
  REMOVE_INTERNAL_REFERENCE(to);
  to->open_mode=from->open_mode;
  to->flags=from->flags & ~FILE_HAS_INTERNAL_REF;

  assign_svalue(& to->read_callback, & from->read_callback);
  assign_svalue(& to->write_callback, & from->write_callback);
#ifdef WITH_OOB
  assign_svalue(& to->read_oob_callback,
		& from->read_oob_callback);
  assign_svalue(& to->write_oob_callback,
		& from->write_oob_callback);
#endif /* WITH_OOB */

  if(IS_ZERO(& from->read_callback))
  {
    set_read_callback(to->fd, 0,0);
  }else{
    set_read_callback(to->fd, file_read_callback, to);
  }

  if(IS_ZERO(& from->write_callback))
  {
    set_write_callback(to->fd, 0,0);
  }else{
    set_write_callback(to->fd, file_write_callback, to);
  }

#ifdef WITH_OOB
  if(IS_ZERO(& from->read_oob_callback))
  {
    set_read_oob_callback(to->fd, 0,0);
  }else{
    set_read_oob_callback(to->fd, file_read_oob_callback, to);
  }

  if(IS_ZERO(& from->write_oob_callback))
  {
    set_write_oob_callback(to->fd, 0,0);
  }else{
    set_write_oob_callback(to->fd, file_write_oob_callback, to);
  }
#endif /* WITH_OOB */
  check_internal_reference(to);
}

static void file_dup2(INT32 args)
{
  struct object *o;
  struct my_file *fd;

  if(args < 1)
    Pike_error("Too few arguments to file->dup2()\n");

  if(FD < 0)
    Pike_error("File not open.\n");

  if(Pike_sp[-args].type != PIKE_T_OBJECT)
    Pike_error("Bad argument 1 to file->dup2()\n");

  o=Pike_sp[-args].u.object;

  fd=get_file_storage(o);

  if(!fd)
    Pike_error("Argument 1 to file->dup2() must be a clone of Stdio.File\n");


  if(fd->fd < 0)
    Pike_error("File given to dup2 not open.\n");

  if (fd->flags & FILE_LOCK_FD) {
    Pike_error("File has been temporarily locked from closing.\n");
  }

  if(fd_dup2(FD,fd->fd) < 0)
  {
    ERRNO=errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }
  ERRNO=0;
  low_dup(o, fd, THIS);

  pop_n_elems(args);
  push_int(1);
}

static void file_dup(INT32 args)
{
  int f;
  struct object *o;
  struct my_file *fd;

  pop_n_elems(args);

  if(FD < 0)
    Pike_error("File not open.\n");


  if((f=fd_dup(FD)) < 0)
  {
    ERRNO=errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }
  o=file_make_object_from_fd(f, THIS->open_mode, THIS->open_mode);
  fd=((struct my_file *)(o->storage));
  ERRNO=0;
  low_dup(o, fd, THIS);
  push_object(o);
}

/* file->open_socket(int|void port, string|void addr) */
static void file_open_socket(INT32 args)
{
  int fd;

  close_fd();
  FD=-1;
  fd=fd_socket(AF_INET, SOCK_STREAM, 0);
  if(fd < 0)
  {
    ERRNO=errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  if (args) {
    struct sockaddr_in addr;
    int o;

    if (Pike_sp[-args].type != PIKE_T_INT) {
      while (fd_close(fd) && errno == EINTR) {}
      Pike_error("Bad argument 1 to open_socket(), expected int\n");
    }
    if (args > 1) {
      if (Pike_sp[1-args].type != PIKE_T_STRING) {
	while (fd_close(fd) && errno == EINTR) {}
	Pike_error("Bad argument 2 to open_socket(), expected string\n");
      }
      get_inet_addr(&addr, Pike_sp[1-args].u.string->str);
    } else {
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      addr.sin_family = AF_INET;
    }
    addr.sin_port = htons( ((u_short)Pike_sp[-args].u.integer) );

    o=1;
    if(fd_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&o, sizeof(int)) < 0) {
      ERRNO=errno;
      while (fd_close(fd) && errno == EINTR) {}
      errno = ERRNO;
      pop_n_elems(args);
      push_int(0);
      return;
    }
    if (fd_bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      ERRNO=errno;
      while (fd_close(fd) && errno == EINTR) {}
      errno = ERRNO;
      pop_n_elems(args);
      push_int(0);
      return;
    }
  }

  init_fd(fd, FILE_READ | FILE_WRITE | fd_query_properties(fd, SOCKET_CAPABILITIES));
  my_set_close_on_exec(fd,1);
  FD = fd;
  ERRNO=0;

  pop_n_elems(args);
  push_int(1);
}

static void file_set_keepalive(INT32 args)
{
  int tmp, i;
  INT_TYPE t;

  get_all_args("file->set_keepalive", args, "%i", &t);

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
  ERRNO = ENOTSUP;
#else /* !ENOTSUP */
#ifdef ENOTTY
  ERRNO = ENOTTY;
#else /* !ENOTTY */
  ERRNO = EIO;
#endif /* ENOTTY */
#endif /* ENOTSUP */
#endif /* SO_KEEPALIVE */
  pop_n_elems(args);
  push_int(!i);
}

static void file_connect(INT32 args)
{
  struct sockaddr_in addr;
  struct pike_string *dest_addr = NULL;
  struct pike_string *src_addr = NULL;
  INT_TYPE dest_port = 0;
  INT_TYPE src_port = 0;

  int tmp, was_closed = FD < 0;

  if (args < 4) {
    get_all_args("file->connect", args, "%S%i", &dest_addr, &dest_port);
  } else {
    get_all_args("file->connect", args, "%S%i%S%i",
		 &dest_addr, &dest_port, &src_addr, &src_port);
  }

  if(was_closed)
  {
    if (args < 4) {
      file_open_socket(0);
    } else {
      push_int(src_port);
      ref_push_string(src_addr);
      file_open_socket(2);
    }
    if(IS_ZERO(Pike_sp-1) || FD < 0)
      Pike_error("file->connect(): Failed to open socket.\n");
    pop_stack();
  }

  get_inet_addr(&addr, dest_addr->str);
  addr.sin_port = htons(((u_short)dest_port));

  tmp=FD;
  THREADS_ALLOW();
  tmp=fd_connect(tmp, (struct sockaddr *)&addr, sizeof(addr));
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

static int isipnr(char *s)
{
  int e,i;
  for(e=0;e<3;e++)
  {
    i=0;
    while(*s==' ') s++;
    while(*s>='0' && *s<='9') s++,i++;
    if(!i) return 0;
    if(*s!='.') return 0;
    s++;
  }
  i=0;
  while(*s==' ') s++;
  while(*s>='0' && *s<='9') s++,i++;
  if(!i) return 0;
  while(*s==' ') s++;
  if(*s) return 0;
  return 1;
}

static void file_query_address(INT32 args)
{
  struct sockaddr_in addr;
  int i;
  char buffer[496],*q;
  /* XOPEN GROUP think this variable should a size_t, BSD thinks it should
   * be an int.
   */
  ACCEPT_SIZE_T len;

  if(FD <0)
    Pike_error("file->query_address(): Connection not open.\n");

  len=sizeof(addr);
  if(args > 0 && !IS_ZERO(Pike_sp-args))
  {
    i=fd_getsockname(FD,(struct sockaddr *)&addr,&len);
  }else{
    i=fd_getpeername(FD,(struct sockaddr *)&addr,&len);
  }
  pop_n_elems(args);
  if(i < 0 || len < (int)sizeof(addr))
  {
    ERRNO=errno;
    push_int(0);
    return;
  }

  q=inet_ntoa(addr.sin_addr);
  strncpy(buffer,q,sizeof(buffer)-20);
  buffer[sizeof(buffer)-20]=0;
  sprintf(buffer+strlen(buffer)," %d",(int)(ntohs(addr.sin_port)));

  push_string(make_shared_string(buffer));
}

static void file_lsh(INT32 args)
{
  ptrdiff_t len;
  if(args != 1)
    Pike_error("Too few/many args to file->`<<\n");

  if(Pike_sp[-1].type != PIKE_T_STRING)
  {
    ref_push_string(string_type_string);
    stack_swap();
    f_cast();
  }

  len=Pike_sp[-1].u.string->len;
  file_write(1);
  if(len != Pike_sp[-1].u.integer) Pike_error("File << failed.\n");
  pop_stack();

  push_object(this_object());
}

static void file_create(INT32 args)
{
  if(!args) return;
  if(Pike_sp[-args].type != PIKE_T_STRING &&
     Pike_sp[-args].type != PIKE_T_INT)
    Pike_error("Bad argument 1 to file->create()\n");

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
  mt_lock_interpreter();
  num_threads--;
  mt_unlock_interpreter();
  free((char *)p);
  th_exit(0);
  return 0;
}

void file_proxy(INT32 args)
{
  struct my_file *f;
  struct new_thread_data *p;
  int from, to;

  THREAD_T id;
  check_all_args("Stdio.File->proxy",args, BIT_OBJECT,0);
  f=get_file_storage(Pike_sp[-args].u.object);
  if(!f)
    Pike_error("Bad argument 1 to Stdio.File->proxy, not a Stdio.File object.\n");

  from=fd_dup(f->fd);
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
  struct object *owner;
#endif
};


#define OB2KEY(O) ((struct file_lock_key_storage *)((O)->storage))

static void low_file_lock(INT32 args, int flags)
{
  int ret,fd=FD;
  struct object *o;
  
  destruct_objects_to_destruct();

  if(FD==-1)
    Pike_error("File->lock(): File is not open.\n");

  if(!args || IS_ZERO(Pike_sp-args))
  {
    if(THIS->key
#ifdef _REENTRANT
       && OB2KEY(THIS->key)->owner == Pike_interpreter.thread_id
#endif
      )
    {
      if (flags & fd_LOCK_NB) {
#ifdef EWOULDBLOCK
	ERRNO = EWOULDBLOCK;
#else /* !EWOULDBLOCK */
	ERRNO = EAGAIN;
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

static void file_lock(INT32 args)
{
  low_file_lock(args, fd_LOCK_EX);
}

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
  THIS_KEY->owner=Pike_interpreter.thread_id;
  add_ref(Pike_interpreter.thread_id);
#endif
}

static void exit_file_lock_key(struct object *o)
{
  if(THIS_KEY->f)
  {
    int fd=THIS_KEY->f->fd;
    int err;
#ifdef PIKE_DEBUG
    if(THIS_KEY->f->key != o)
      fatal("File lock key is wrong!\n");
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
    if(THIS_KEY->owner)
    {
      free_object(THIS_KEY->owner);
      THIS_KEY->owner=0;
    }
#endif
    THIS_KEY->f->key = 0;
    THIS_KEY->f = 0;
  }
}

static void init_file_locking(void)
{
  ptrdiff_t off;
  start_new_program();
  off = ADD_STORAGE(struct file_lock_key_storage);
#ifdef _REENTRANT
  map_variable("_owner","object",0,
	       off + OFFSETOF(file_lock_key_storage, owner),
	       PIKE_T_OBJECT);
#endif
  map_variable("_file","object",0,
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
#else
#define init_file_locking()
#define exit_file_locking()
#endif /* HAVE_FD_FLOCK */

void exit_files_stat(void);

void pike_module_exit(void)
{
  extern void exit_files_efuns(void);
  extern void exit_sendfile(void);
  extern void port_exit_program(void);

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

void init_files_efuns(void);
void init_files_stat(void);

#define REF (*((struct object **)(Pike_fp->current_storage)))

#define FILE_FUNC(X,Y,Z) \
static ptrdiff_t PIKE_CONCAT(Y,_function_number);

#include "file_functions.h"


#define FILE_FUNC(X,Y,Z)					\
void PIKE_CONCAT(Y,_ref) (INT32 args) {				\
  struct object *o=REF;						\
  debug_malloc_touch(o);					\
  if(!o || !o->prog) { 						\
   /* This is a temporary kluge */                              \
   DO_IF_DMALLOC(						\
     extern int d_flag;                                         \
     if(d_flag)							\
     {								\
       fprintf(stderr,"Possible gc() failure detected\n");	\
       describe(Pike_fp->current_object);				\
       if(o) describe(o);					\
     }								\
   );								\
   Pike_error("Stdio.File(): not open.\n");				\
  }								\
  if(o->prog != file_program)					\
     Pike_error("Wrong type of object in Stdio.File->_fd\n");	\
  apply_low(o, PIKE_CONCAT(Y,_function_number), args);		\
}

#include "file_functions.h"

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
#define FILE_FUNC(X,Y,Z) \
    if(PIKE_CONCAT(Y,_function_number)<0 || PIKE_CONCAT(Y,_function_number)>file_program->num_identifier_references) \
      fatal(#Y "_function_number is incorrect: %ld\n", \
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

void pike_module_init(void)
{
  struct object *o;
  extern void port_setup_program(void);
  extern void init_sendfile(void);
  extern void init_udp(void);
  int e;

  init_files_efuns();
  init_files_stat();

  START_NEW_PROGRAM_ID(STDIO_FD);
  ADD_STORAGE(struct my_file);

#define FILE_FUNC(X,Y,Z) PIKE_CONCAT(Y,_function_number)=pike_add_function(X,Y,Z,0);
#include "file_functions.h"
  map_variable("_read_callback","mixed",0,OFFSETOF(my_file, read_callback),PIKE_T_MIXED);
  map_variable("_write_callback","mixed",0,OFFSETOF(my_file, write_callback),PIKE_T_MIXED);
#ifdef WITH_OOB
  map_variable("_read_oob_callback","mixed",0,OFFSETOF(my_file, read_oob_callback),PIKE_T_MIXED);
  map_variable("_write_oob_callback","mixed",0,OFFSETOF(my_file, write_oob_callback),PIKE_T_MIXED);
#endif


  init_file_locking();
  set_init_callback(init_file_struct);
  set_exit_callback(exit_file_struct);
#ifdef PIKE_DEBUG
  set_gc_check_callback(gc_mark_file_struct);
#endif

  file_program=end_program();
  add_program_constant("Fd",file_program,0);

  o=file_make_object_from_fd(0, FILE_READ , fd_CAN_NONBLOCK);
  ((struct my_file *)(o->storage))->flags |= FILE_NO_CLOSE_ON_DESTRUCT;
  add_object_constant("_stdin",o,0);
  free_object(o);

  o=file_make_object_from_fd(1, FILE_WRITE, fd_CAN_NONBLOCK);
  ((struct my_file *)(o->storage))->flags |= FILE_NO_CLOSE_ON_DESTRUCT;
  add_object_constant("_stdout",o,0);
  free_object(o);

  o=file_make_object_from_fd(2, FILE_WRITE, fd_CAN_NONBLOCK);
  ((struct my_file *)(o->storage))->flags |= FILE_NO_CLOSE_ON_DESTRUCT;
  add_object_constant("_stderr",o,0);
  free_object(o);

  start_new_program();
  ADD_STORAGE(struct object *);
  map_variable("_fd","object",0,0,PIKE_T_OBJECT);

#define FILE_FUNC(X,Y,Z) pike_add_function(X,PIKE_CONCAT(Y,_ref),Z,0);
#include "file_functions.h"

  file_ref_program=end_program();
  add_program_constant("Fd_ref",file_ref_program,0);

  port_setup_program();

  init_sendfile();
  init_udp();

  add_integer_constant("PROP_IPC",fd_INTERPROCESSABLE,0);
  add_integer_constant("PROP_NONBLOCK",fd_CAN_NONBLOCK,0);
  add_integer_constant("PROP_SHUTDOWN",fd_CAN_SHUTDOWN,0);
  add_integer_constant("PROP_BUFFERED",fd_BUFFERED,0);
  add_integer_constant("PROP_BIDIRECTIONAL",fd_BIDIRECTIONAL,0);
#ifdef WITH_OOB
  add_integer_constant("__HAVE_OOB__",1,0);
#ifdef PIKE_OOB_WORKS
  add_integer_constant("__OOB__",PIKE_OOB_WORKS,0);
#else
  add_integer_constant("__OOB__",-1,0); /* unknown */
#endif
#endif

#ifdef PIKE_DEBUG
  dmalloc_accept_leak(add_to_callback(&do_debug_callbacks,
				      check_static_file_data,
				      0,
				      0));
#endif
}

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
  return f->fd;
}
