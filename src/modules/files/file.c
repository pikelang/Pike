/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: file.c,v 1.156 1999/05/19 14:23:28 mirar Exp $");
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
#include "gc.h"
#include "opcodes.h"
#include "operators.h"
#include "security.h"

#include "file_machine.h"
#include "file.h"
#include "error.h"
#include "signal_handler.h"
#include "pike_types.h"
#include "threads.h"

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
#define THIS ((struct my_file *)(fp->current_storage))
#define FD (THIS->fd)
#define ERRNO (THIS->my_errno)

#define READ_BUFFER 8192

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
  THIS->read_callback.type=T_INT;
  THIS->read_callback.u.integer=0;
  THIS->write_callback.type=T_INT;
  THIS->write_callback.u.integer=0;
#ifdef WITH_OOB
  THIS->read_oob_callback.type=T_INT;
  THIS->read_oob_callback.u.integer=0;
  THIS->write_oob_callback.type=T_INT;
  THIS->write_oob_callback.u.integer=0;
#endif /* WITH_OOB */
#if defined(HAVE_FD_FLOCK) || defined(HAVE_FD_LOCKF) 
  THIS->key=0;
#endif
}


void reset_variables(void)
{
  free_svalue(& THIS->read_callback);
  THIS->read_callback.type=T_INT;
  free_svalue(& THIS->write_callback);
  THIS->write_callback.type=T_INT;
#ifdef WITH_OOB
  free_svalue(& THIS->read_oob_callback);
  THIS->read_oob_callback.type=T_INT;
  free_svalue(& THIS->write_oob_callback);
  THIS->write_oob_callback.type=T_INT;
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
    int i;
    THREADS_ALLOW_UID();
    i=fd_close(fd);
    THREADS_DISALLOW_UID();
    
    if(i < 0)
    {
      switch(errno)
      {
	default:
	  ERRNO=errno;
	  FD=fd;
	  error("Failed to close file.\n");
	  
	case EBADF:
	  error("Internal error: Closing a non-active file descriptor %d.\n",fd);
	  
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
  just_close_fd();
}

void my_set_close_on_exec(int fd, int to)
{
#if 1
  set_close_on_exec(fd, to);
#else
  if(to)
  {
    files[fd].open_mode |= FILE_SET_CLOSE_ON_EXEC;
  }else{
    if(files[fd].open_mode & FILE_SET_CLOSE_ON_EXEC)
      files[fd].open_mode &=~ FILE_SET_CLOSE_ON_EXEC;
    else
      set_close_on_exec(fd, 0);
  }
#endif
}

void do_set_close_on_exec(void)
{
#if 0
  int e;
  for(e=0;e<MAX_OPEN_FILEDESCRIPTORS;e++)
  {
    if(files[e].open_mode & FILE_SET_CLOSE_ON_EXEC)
    {
      set_close_on_exec(e, 1);
      files[e].open_mode &=~ FILE_SET_CLOSE_ON_EXEC;
    }
  }
#endif
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
  INT32 bytes_read,i;
  bytes_read=0;
  *err=0;

  if(r <= 65536)
  {
    struct pike_string *str;

    str=begin_shared_string(r);

    SET_ONERROR(ebuf, call_free, str);

    do{
      int fd=FD;
      THREADS_ALLOW();
      i=fd_read(fd, str->str+bytes_read, r);
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
      else if(errno != EINTR)
      {
	*err=errno;
	if(!bytes_read)
	{
	  free((char *)str);
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
      struct pike_string *foo; /* Per */
      foo = make_shared_binary_string(str->str,bytes_read);
      free((char *)str);
      return foo;
    }
    
  }else{
#define CHUNK 65536
    INT32 try_read;
    dynamic_buffer b;

    b.s.str=0;
    initialize_buf(&b);
    SET_ONERROR(ebuf, free_dynamic_buffer, &b);
    do{
      char *buf;
      try_read=MINIMUM(CHUNK,r);
      
      buf = low_make_buf_space(try_read, &b);

      THREADS_ALLOW();
      i=fd_read(fd, buf, try_read);
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
	if(errno != EINTR)
	{
	  *err=errno;
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

    SET_ONERROR(ebuf, call_free, str);

    do{
      int fd=FD;
      THREADS_ALLOW();
      i=fd_recv(fd, str->str+bytes_read, r, MSG_OOB);
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
      else if(errno != EINTR)
      {
	*err=errno;
	if(!bytes_read)
	{
	  free((char *)str);
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
      struct pike_string *foo; /* Per */
      foo = make_shared_binary_string(str->str, bytes_read);
      free_string(end_shared_string(str));
      return foo;
    }
    
  }else{
#define CHUNK 65536
    INT32 try_read;
    dynamic_buffer b;

    b.s.str=0;
    initialize_buf(&b);
    SET_ONERROR(ebuf, free_dynamic_buffer, &b);
    do{
      char *buf;
      try_read=MINIMUM(CHUNK,r);
      
      buf = low_make_buf_space(try_read, &b);

      THREADS_ALLOW();
      i=fd_recv(fd, buf, try_read, MSG_OOB);
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
	if(errno != EINTR)
	{
	  *err=errno;
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
  }
}
#endif /* WITH_OOB */

static void file_read(INT32 args)
{
  struct pike_string *tmp;
  INT32 all, len;

  if(FD < 0)
    error("File not open.\n");

  if(!args)
  {
    len=0x7fffffff;
  }
  else
  {
    if(sp[-args].type != T_INT)
      error("Bad argument 1 to file->read().\n");
    len=sp[-args].u.integer;
    if(len<0)
      error("Cannot read negative number of characters.\n");
  }

  if(args > 1 && !IS_ZERO(sp+1-args))
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
    error("File not open.\n");

  if(!args)
  {
    len=0x7fffffff;
  }
  else
  {
    if(sp[-args].type != T_INT)
      error("Bad argument 1 to file->read_oob().\n");
    len=sp[-args].u.integer;
    if(len<0)
      error("Cannot read negative number of characters.\n");
  }

  if(args > 1 && !IS_ZERO(sp+1-args))
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
    error("File is not open.\n");				\
  if(!args)							\
    error("Too few arguments to file_set_%s\n",#X);		\
  assign_svalue(& THIS->X, sp-args);				\
  if(IS_ZERO(sp-args))						\
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
    error("File is not open.\n");

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
    error("File is not open.\n");
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
  INT32 written,i;
  struct pike_string *str;

  if(args<1 || ((sp[-args].type != T_STRING) && (sp[-args].type != T_ARRAY)))
    error("Bad argument 1 to file->write().\n"
	  "Type is %s. Expected string or array(string)\n",
	  type_name[sp[-args].type]);

  if(FD < 0)
    error("File not open for write.\n");
  
  if (sp[-args].type == T_ARRAY) {
    struct array *a = sp[-args].u.array;
    i = a->size;
    while(i--) {
      if (a->item[i].type != T_STRING) {
	error("Bad argument 1 to file->write().\n"
	      "Element %d is not a string.\n",
	      i);
      } else if (a->item[i].u.string->size_shift) {
	error("Bad argument 1 to file->write().\n"
	      "Element %d is a wide string.\n",
	      i);
      }
    }

#ifdef HAVE_WRITEV
    if (args > 1) {
#endif /* HAVE_WRITEV */
      ref_push_array(a);
      push_constant_text("");
      o_multiply();
      sp--;
      assign_svalue(sp-args, sp);

#ifdef PIKE_DEBUG
      if (sp[-args].type != T_STRING) {
	error("Bad return value from string multiplication.");
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
	  error("File destructed while in file->write.\n");
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
	    if (iov->iov_len <= i) {
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

  /* At this point sp[-args].type is T_STRING */

  if(args > 1)
  {
    extern void f_sprintf(INT32);
    f_sprintf(args);
    args=1;
  }

  str=sp[-args].u.string;

  for(written=0;written < str->len;check_signals(0,0,0))
  {
    int fd=FD;
    THREADS_ALLOW();
    i=fd_write(fd, str->str + written, str->len - written);
    THREADS_DISALLOW();

#ifdef _REENTRANT
    if(FD<0) error("File destructed while in file->write.\n");
#endif

    if(i<0)
    {
      switch(errno)
      {
      default:
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
  push_int(written);
}

#ifdef WITH_OOB
static void file_write_oob(INT32 args)
{
  INT32 written,i;
  struct pike_string *str;

  if(args<1 || sp[-args].type != T_STRING)
    error("Bad argument 1 to file->write().\n");

  if(args > 1)
  {
    extern void f_sprintf(INT32);
    f_sprintf(args);
    args=1;
  }

  if(FD < 0)
    error("File not open for write_oob.\n");
  
  written=0;
  str=sp[-args].u.string;

  while(written < str->len)
  {
    int fd=FD;
    THREADS_ALLOW();
    i=fd_send(fd, str->str + written, str->len - written, MSG_OOB);
    THREADS_DISALLOW();

#ifdef _REENTRANT
    if(FD<0) error("File destructed while in file->write_oob.\n");
#endif

    if(i<0)
    {
      switch(errno)
      {
      default:
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
  push_int(written);
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
      close_fd();
      return 1;
    }

  case FILE_READ | FILE_WRITE:
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
    if(sp[-args].type != T_STRING)
      error("Bad argument 1 to file->close()\n");
    flags=parse(sp[-args].u.string->str);
  }else{
    flags=FILE_READ | FILE_WRITE;
  }

  if (THIS->flags & FILE_LOCK_FD) {
    error("close() has been temporarily disabled on this file.\n");
  }

  if((THIS->open_mode & ~flags & (FILE_READ|FILE_WRITE)) && flags)
  {
    if(!(THIS->open_mode & fd_CAN_SHUTDOWN))
    {
      error("Cannot close one direction on this file.\n");
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
  struct pike_string *str, *flag_str;
  close_fd();
  
  if(args < 2)
    error("Too few arguments to file->open()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to file->open()\n");

  if(sp[1-args].type != T_STRING)
    error("Bad argument 2 to file->open()\n");

  if (args > 2)
  {
    if (sp[2-args].type != T_INT)
      error("Bad argument 3 to file->open()\n");
    access = sp[2-args].u.integer;
  } else
    access = 00666;

  str=sp[-args].u.string;
  
  flags = parse((flag_str = sp[1-args].u.string)->str);

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
  {
    if(!CHECK_SECURITY(SECURITY_BIT_CONDITIONAL_IO))
      error("Permission denied.\n");

    if(flags & (FILE_APPEND | FILE_TRUNC | FILE_CREATE | FILE_WRITE))
    {
      push_text("write");
    }else{
      push_text("read");
    }

    ref_push_object(fp->current_object);
    ref_push_string(str);
    ref_push_string(flag_str);
    push_int(access);

    safe_apply(OBJ2CREDS(current_creds)->user,"valid_open",5);
    switch(sp[-1].type)
    {
      case T_INT:
	switch(sp[-1].u.integer)
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
	    error("Stdio.file->open: permission denied.\n");
	    
	  default:
	    error("Error in user->valid_open, wrong return value.\n");
	}
	break;

      default:
	error("Error in user->valid_open, wrong return type.\n");

      case T_STRING:
	str=sp[-1].u.string;
	args++;
    }
  }
#endif
      
  if(!( flags &  (FILE_READ | FILE_WRITE)))
    error("Must open file for at least one of read and write.\n");

  THREADS_ALLOW_UID();
  do {
    fd=fd_open(str->str,map(flags), access);
  } while(fd < 0 && errno == EINTR);
  THREADS_DISALLOW_UID();

  if(!fp->current_object->prog)
    error("Object destructed in file->open()\n");

  if(fd >= MAX_OPEN_FILEDESCRIPTORS)
  {
    ERRNO=EBADF;
    close(fd);
    fd=-1;
  }
  else if(fd < 0)
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
}


static void file_seek(INT32 args)
{
#ifdef HAVE_LSEEK64
  long long to;
#else
  INT32 to;
#endif
  if(args<1 || sp[-args].type != T_INT)
    error("Bad argument 1 to file->seek(int to).\n");

  if(FD < 0)
    error("File not open.\n");

  to=sp[-args].u.integer;
  if(args>1)
  {
    if(sp[-args+1].type != T_INT)
      error("Bad argument 2 to file->seek(int unit,int mult).\n");
    to *= sp[-args+1].u.integer;
  }
  if(args>2)
  {
    if(sp[-args+2].type != T_INT)
      error("Bad argument 3 to file->seek(int unit,int mult,int add).\n");
    to += sp[-args+2].u.integer;
  }

  ERRNO=0;

#ifdef HAVE_LSEEK64
  to=lseek64(FD,to,to<0 ? SEEK_END : SEEK_SET);
#else
  to=fd_lseek(FD,to,to<0 ? SEEK_END : SEEK_SET);
#endif
  if(to<0) ERRNO=errno;

  pop_n_elems(args);
  push_int((INT32)to);
}

static void file_tell(INT32 args)
{
  INT32 to;

  if(FD < 0)
    error("File not open.\n");
  
  ERRNO=0;
  to=fd_lseek(FD, 0L, SEEK_CUR);

  if(to<0) ERRNO=errno;

  pop_n_elems(args);
  push_int(to);
}

static void file_truncate(INT32 args)
{
#ifdef HAVE_LSEEK64
  long long len;
#else
  INT32 len;
#endif
  int res;

  if(args<1 || sp[-args].type != T_INT)
    error("Bad argument 1 to file->truncate(int length).\n");

  if(FD < 0)
    error("File not open.\n");

  len = sp[-args].u.integer;
  
  ERRNO=0;
  res=fd_ftruncate(FD, len);

  pop_n_elems(args);

  if(res<0) 
     ERRNO=errno;

  push_int(!res);
}

struct array *encode_stat(struct stat *);

static void file_stat(INT32 args)
{
  int fd;
  struct stat s;
  int tmp;

  if(FD < 0)
    error("File not open.\n");
  
  pop_n_elems(args);

  fd=FD;

 retry:
  THREADS_ALLOW();
  tmp=fd_fstat(fd, &s);
  THREADS_DISALLOW();

  if(tmp < 0)
  {
    if(errno == EINTR) goto retry;
    ERRNO=errno;
    push_int(0);
  }else{
    ERRNO=0;
    push_array(encode_stat(&s));
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
  if(FD < 0) error("File not open.\n");

  if(!(THIS->open_mode & fd_CAN_NONBLOCK))
    error("That file does not support nonblocking operation.\n");

  if(set_nonblocking(FD,1))
  {
    ERRNO=errno;
    error("Stdio.File->set_nonbloblocking() failed.\n");
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
    error("Too few arguments to file->set_close_on_exec()\n");
  if(FD <0)
    error("File not open.\n");

  if(IS_ZERO(sp-args))
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
    error("File not open.\n");

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
    error("file->set_buffer() on closed file.\n");
  if(!args)
    error("Too few arguments to file->set_buffer()\n");
  if(sp[-args].type!=T_INT)
    error("Bad argument 1 to file->set_buffer()\n");

  bufsize=sp[-args].u.integer;
  if(bufsize < 0)
    error("Bufsize must be larger than zero.\n");

  if(args>1)
  {
    if(sp[1-args].type != T_STRING)
      error("Bad argument 2 to file->set_buffer()\n");
    flags=parse(sp[1-args].u.string->str);
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
int my_socketpair(int family, int type, int protocol, int sv[2])
{
  static int fd=-1;
  static struct sockaddr_in my_addr;
  struct sockaddr_in addr,addr2;
  int retries=0;
  /* Solaris and AIX think this variable should a size_t, everybody else
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

  if(fd==-1)
  {
    if((fd=fd_socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
    
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
    if(fd_bind(fd, (struct sockaddr *)&my_addr, sizeof(addr)) < 0)
    {
      fd_close(fd);
      fd=-1;
      return -1;
    }

    /* Check what ports we got.. */
    len = sizeof(my_addr);
    if(fd_getsockname(fd,(struct sockaddr *)&my_addr,&len) < 0)
    {
      fd_close(fd);
      fd=-1;
      return -1;
    }

    /* Listen to connections on our new socket */
    if(fd_listen(fd, 5) < 0)
    {
      fd_close(fd);
      fd=-1;
      return -1;
    }

    set_nonblocking(fd,1);

    my_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
  }
  

  if((sv[1]=fd_socket(AF_INET, SOCK_STREAM, 0)) <0) return -1;

/*  set_nonblocking(sv[1],1); */

retry_connect:
  retries++;
  if(fd_connect(sv[1], (struct sockaddr *)&my_addr, sizeof(addr)) < 0)
  {
/*    fprintf(stderr,"errno=%d (%d)\n",errno,EWOULDBLOCK); */
    if(errno != EWOULDBLOCK)
    {
      int tmp2;
      for(tmp2=0;tmp2<20;tmp2++)
      {
	int tmp;
	ACCEPT_SIZE_T len2;

	len2=sizeof(addr);
	tmp=fd_accept(fd,(struct sockaddr *)&addr,&len2);
	
	if(tmp!=-1)
	  fd_close(tmp);
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
    sv[0]=fd_accept(fd,(struct sockaddr *)&addr,&len3);

    if(sv[0] < 0) {
      if(errno==EINTR) goto retry_accept;
      fd_close(sv[1]);
      return -1;
    }

    set_nonblocking(sv[0],0);

    /* We do not trust accept */
    len=sizeof(addr);
    if(fd_getpeername(sv[0], (struct sockaddr *)&addr,&len)) return -1;
    len=sizeof(addr);
    if(fd_getsockname(sv[1],(struct sockaddr *)&addr2,&len) < 0) return -1;
  }while(len < (int)sizeof(addr) ||
	 addr2.sin_addr.s_addr != addr.sin_addr.s_addr ||
	 addr2.sin_port != addr.sin_port);

/*  set_nonblocking(sv[1],0); */

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
  if(args) type = sp[-args].u.integer;

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
      error("Cannot create a pipe matching those parameters.\n");
    }
  }while(0);
    
  if ((i<0) || (inout[0] < 0) || (inout[1] < 0))
  {
    if (inout[0] >= 0) {
      close(inout[0]);
    }
    if (inout[1] >= 0) {
      close(inout[1]);
    }
    ERRNO=errno;
    push_int(0);
  }
  else if((inout[0] >= MAX_OPEN_FILEDESCRIPTORS) ||
	  (inout[1] >= MAX_OPEN_FILEDESCRIPTORS))
  {
    ERRNO=EBADF;
    fd_close(inout[0]);
    fd_close(inout[1]);
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
  if(!(THIS->flags & (FILE_NO_CLOSE_ON_DESTRUCT | FILE_LOCK_FD)))
    just_close_fd();
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
    error("Too few arguments to file->dup2()\n");

  if(FD < 0)
    error("File not open.\n");

  if(sp[-args].type != T_OBJECT)
    error("Bad argument 1 to file->dup2()\n");

  o=sp[-args].u.object;

  fd=get_file_storage(o);

  if(!fd)
    error("Argument 1 to file->dup2() must be a clone of Stdio.File\n");


  if(fd->fd < 0)
    error("File given to dup2 not open.\n");

  if (fd->flags & FILE_LOCK_FD) {
    error("File has been temporarily locked from closing.\n");
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
    error("File not open.\n");


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
  if(fd >= MAX_OPEN_FILEDESCRIPTORS)
  {
    ERRNO=EBADF;
    pop_n_elems(args);
    push_int(0);
    return;
  }
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

    if (sp[-args].type != T_INT) {
      fd_close(fd);
      error("Bad argument 1 to open_socket(), expected int\n");
    }
    if (args > 1) {
      if (sp[1-args].type != T_STRING) {
	close(fd);
	error("Bad argument 2 to open_socket(), expected string\n");
      }
      get_inet_addr(&addr, sp[1-args].u.string->str);
    } else {
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      addr.sin_family = AF_INET;
    }
    addr.sin_port = htons( ((u_short)sp[-args].u.integer) );

    o=1;
    if(fd_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&o, sizeof(int)) < 0) {
      ERRNO=errno;
      close(fd);
      pop_n_elems(args);
      push_int(0);
      return;
    }
    if (fd_bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      ERRNO=errno;
      close(fd);
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
  int tmp;
  if(args < 2)
    error("Too few arguments to file->connect()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to file->connect()\n");
      
  if(sp[1-args].type != T_INT)
    error("Bad argument 2 to file->connect()\n");

  if(FD < 0)
  {
    file_open_socket(0);
    if(IS_ZERO(sp-1) || FD < 0)
      error("file->connect(): Failed to open socket.\n");
    pop_stack();
  }


  get_inet_addr(&addr, sp[-args].u.string->str);
  addr.sin_port = htons(((u_short)sp[1-args].u.integer));

  tmp=FD;
  THREADS_ALLOW();
  tmp=fd_connect(tmp, (struct sockaddr *)&addr, sizeof(addr));
  THREADS_DISALLOW();

  if(tmp < 0
#ifdef EINPROGRESS
     && !(errno==EINPROGRESS && (THIS->open_mode & FILE_NONBLOCKING))
#endif
    )
  {
    /* something went wrong */
    ERRNO=errno;
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
    error("file->query_address(): Connection not open.\n");

  len=sizeof(addr);
  if(args > 0 && !IS_ZERO(sp-args))
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
  INT32 len;
  if(args != 1)
    error("Too few/many args to file->`<<\n");

  if(sp[-1].type != T_STRING)
  {
    ref_push_string(string_type_string);
    stack_swap();
    f_cast();
  }

  len=sp[-1].u.string->len;
  file_write(1);
  if(len != sp[-1].u.integer) error("File << failed.\n");
  pop_stack();

  push_object(this_object());
}

static void file_create(INT32 args)
{
  if(!args || sp[-args].type == T_INT) return;
  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to file->create()\n");

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
    long len, w;
    len=fd_read(p->from, buffer, READ_BUFFER);
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
      long wl=fd_write(p->to, buffer+w, len-w);
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

  fd_close(p->to);
  fd_close(p->from);
  mt_lock(&interpreter_lock);
  num_threads--;
  mt_unlock(&interpreter_lock);
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
  f=get_file_storage(sp[-args].u.object);
  if(!f)
    error("Bad argument 1 to Stdio.File->proxy, not a Stdio.File object.\n");

  from=fd_dup(f->fd);
  if(from<0)
  {
    ERRNO=errno;
    error("Failed to dup proxy fd. (errno=%d)\n",errno);
  }
  to=fd_dup(FD);
  if(from<0)
  {
    ERRNO=errno;
    fd_close(from);
    error("Failed to dup proxy fd.\n");
  }
  
  p=ALLOC_STRUCT(new_thread_data);
  p->from=from;
  p->to=to;

  num_threads++;
  if(th_create_small(&id,proxy_thread,p))
  {
    free((char *)p);
    error("Failed to create thread.\n");
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
  if(sp[-1].type!=T_OBJECT)
    error("Failed to create proxy pipe (errno=%d)!\n",get_file_storage(n)->my_errno);
  n2=sp[-1].u.object;
  /* Stack is now: pipe(read), pipe(write) */
  if(for_reading)
  {
    ref_push_object(o);
    apply(n2,"proxy",1);
    pop_n_elems(2);
  }else{
    /* Swap */
    sp[-2].u.object=n2;
    sp[-1].u.object=n;
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
#ifdef _REENTRANT
  struct object *owner;
#endif
};


#define OB2KEY(O) ((struct file_lock_key_storage *)((O)->storage))

static void low_file_lock(INT32 args, int flags)
{
  int ret,fd=FD;
  struct object *o;

  if(FD==-1)
    error("File->lock(): File is not open.\n");

  if(!args || IS_ZERO(sp-args))
  {
    if(THIS->key
#ifdef _REENTRANT
       && OB2KEY(THIS->key)->owner == thread_id
#endif
      )
    {
      error("Recursive file locks!\n");
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
    THIS->key=o;
    OB2KEY(o)->f=THIS;
    pop_n_elems(args);
    push_object(o);
  }
}

static void file_lock(INT32 args)
{
  low_file_lock(args,fd_LOCK_EX);
}

/* If (fd_LOCK_EX | fd_LOCK_NB) is used with lockf, the result will be
 * F_TEST, which only tests for the existance of a lock on the file.
 */
#ifdef HAVE_FD_FLOCK
static void file_trylock(INT32 args)
{
  low_file_lock(args,fd_LOCK_EX | fd_LOCK_NB);
}
#else
static void file_trylock(INT32 args)
{
  low_file_lock(args, fd_LOCK_NB);
}
#endif

#define THIS_KEY ((struct file_lock_key_storage *)(fp->current_storage))
static void init_file_lock_key(struct object *o)
{
  THIS_KEY->f=0;
#ifdef _REENTRANT
  THIS_KEY->owner=thread_id;
  add_ref(thread_id);
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

    THREADS_ALLOW();
    do
    {
#ifdef HAVE_FD_FLOCK
      err=fd_flock(fd, fd_LOCK_UN);
#else
      err=fd_lockf(fd, fd_LOCK_UN);
#endif
    }while(err<0 && errno==EINTR);
    THREADS_DISALLOW();

#ifdef _REENTRANT
    if(THIS_KEY->owner)
    {
      free_object(THIS_KEY->owner);
      THIS_KEY->owner=0;
    }
#endif
    THIS_KEY->f->key=0;
    THIS_KEY->f=0;
  }
}

static void init_file_locking(void)
{
  INT32 off;
  start_new_program();
  off=ADD_STORAGE(struct file_lock_key_storage);
#ifdef _REENTRANT
  map_variable("_owner","object",0,
	       off + OFFSETOF(file_lock_key_storage, owner),
	       T_OBJECT);
#endif
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


void pike_module_exit(void)
{
  extern void exit_sendfile(void);

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
}

void init_files_efuns(void);

#define REF (*((struct object **)(fp->current_storage)))

#define FILE_FUNC(X,Y,Z) \
static int PIKE_CONCAT(Y,_function_number);

#include "file_functions.h"


#define FILE_FUNC(X,Y,Z) \
void PIKE_CONCAT(Y,_ref) (INT32 args) {\
  struct object *o=REF; \
  if(!o || !o->prog) error("Stdio.File(): not open.\n"); \
  if(o->prog != file_program) \
     error("Wrong type of object in Stdio.File->_fd\n"); \
  apply_low(o, PIKE_CONCAT(Y,_function_number), args); \
}

#include "file_functions.h"

#ifdef PIKE_DEBUG
void check_static_file_data(struct callback *a, void *b, void *c)
{
  if(file_program)
  {
#define FILE_FUNC(X,Y,Z) \
    if(PIKE_CONCAT(Y,_function_number)<0 || PIKE_CONCAT(Y,_function_number)>file_program->num_identifier_references) \
      fatal(#Y "_function_number is incorrect: %d\n",PIKE_CONCAT(Y,_function_number));
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
  int e;

  init_files_efuns();

  start_new_program();
  ADD_STORAGE(struct my_file);

#define FILE_FUNC(X,Y,Z) PIKE_CONCAT(Y,_function_number)=add_function(X,Y,Z,0);
#include "file_functions.h"
  map_variable("_read_callback","mixed",0,OFFSETOF(my_file, read_callback),T_MIXED);
  map_variable("_write_callback","mixed",0,OFFSETOF(my_file, write_callback),T_MIXED);
#ifdef WITH_OOB
  map_variable("_read_oob_callback","mixed",0,OFFSETOF(my_file, read_oob_callback),T_MIXED);
  map_variable("_write_oob_callback","mixed",0,OFFSETOF(my_file, write_oob_callback),T_MIXED);
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
  map_variable("_fd","object",0,0,T_OBJECT);

#define FILE_FUNC(X,Y,Z) add_function(X,PIKE_CONCAT(Y,_ref),Z,0);
#include "file_functions.h"

  file_ref_program=end_program();
  add_program_constant("Fd_ref",file_ref_program,0);

  port_setup_program();

  init_sendfile();

  add_integer_constant("PROP_IPC",fd_INTERPROCESSABLE,0);
  add_integer_constant("PROP_NONBLOCK",fd_CAN_NONBLOCK,0);
  add_integer_constant("PROP_SHUTDOWN",fd_CAN_SHUTDOWN,0);
  add_integer_constant("PROP_BUFFERED",fd_BUFFERED,0);
  add_integer_constant("PROP_BIDIRECTIONAL",fd_BIDIRECTIONAL,0);
#ifdef WITH_OOB
  add_integer_constant("__HAVE_OOB__",1,0);
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
  struct my_file *f=get_file_storage(o);
  if(!f) return -1;
  return f->fd;
}
