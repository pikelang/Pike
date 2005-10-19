/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: sendfile.c,v 1.64 2005/10/19 15:24:59 grubba Exp $
*/

/*
 * Sends headers + from_fd[off..off+len-1] + trailers to to_fd asyncronously.
 *
 * Henrik Grubbström 1999-04-02
 */

/*
 * FIXME:
 *	Support for NT.
 *
 *	Need to lock the fd's so that the user can't close them for us.
 */

#include "global.h"
#include "file_machine.h"

#include "fdlib.h"
#include "fd_control.h"
#include "object.h"
#include "array.h"
#include "threads.h"
#include "interpret.h"
#include "svalue.h"
#include "callback.h"
#include "backend.h"
#include "module_support.h"
#include "bignum.h"

#include "file.h"

#include <errno.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <sys/stat.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif /* HAVE_SYS_UIO_H */

#if 0
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#else /* !HAVE_SYS_MMAN_H */
#ifdef HAVE_LINUX_MMAN_H
#include <linux/mman.h>
#else /* !HAVE_LINUX_MMAN_H */
#ifdef HAVE_MMAP /* sys/mman.h is _probably_ there anyway. */
#include <sys/mman.h>
#endif /* HAVE_MMAP */
#endif /* HAVE_LINUX_MMAN_H */
#endif /* HAVE_SYS_MMAN_H */
#endif

#define sp Pike_sp

/* #define SF_DEBUG */

#ifdef SF_DEBUG
#define SF_DFPRINTF(X)	fprintf X
#else /* !SF_DEBUG */
#define SF_DFPRINTF(X)
#endif /* SF_DEBUG */

#define BUF_SIZE	0x00010000 /* 64K */

#define MMAP_SIZE	0x00100000 /* 1M */

#ifndef MAP_FAILED
#define MAP_FAILED	((void *)-1)
#endif /* !MAP_FAILED */

#ifndef MAP_FILE
#define MAP_FILE	0
#endif /* !MAP_FILE */

#ifndef S_ISREG
#ifdef S_IFREG
#define S_ISREG(mode)   (((mode) & (S_IFMT)) == (S_IFREG))
#else /* !S_IFREG */
#define S_ISREG(mode)   (((mode) & (_S_IFMT)) == (_S_IFREG))
#endif /* S_IFREG */
#endif /* !S_ISREG */


/*
 * Only support for threaded operation right now.
 */
#ifdef _REENTRANT

#undef THIS
#define THIS	((struct pike_sendfile *)(Pike_fp->current_storage))

/*
 * All known versions of sendfile(2) are broken.
 */
#ifndef HAVE_BROKEN_SENDFILE
#define HAVE_BROKEN_SENDFILE
#endif /* !HAVE_BROKEN_SENDFILE */

/*
 * Disable any use of sendfile(2) if HAVE_BROKEN_SENDFILE is defined.
 */
#ifdef HAVE_BROKEN_SENDFILE
#ifdef HAVE_SENDFILE
#undef HAVE_SENDFILE
#endif /* HAVE_SENDFILE */
#ifdef HAVE_FREEBSD_SENDFILE
#undef HAVE_FREEBSD_SENDFILE
#endif /* HAVE_FREEBSD_SENDFILE */
#ifdef HAVE_HPUX_SENDFILE
#undef HAVE_HPUX_SENDFILE
#endif /* HAVE_HPUX_SENDFILE */
#endif /* HAVE_BROKEN_SENDFILE */

/*
 * Globals
 */

static struct program *pike_sendfile_prog = NULL;

/*
 * Struct init code.
 */

static void init_pike_sendfile(struct object *o)
{
  MEMSET(THIS, 0, sizeof(struct pike_sendfile));

  /* callback doesn't actually need to be initialized since it is a
   * mapped variable, but since we just zapped it with zeroes we need
   * to set the type to T_INT again.. /Hubbe
   */
  THIS->callback.type = T_INT;
}

static void exit_pike_sendfile(struct object *o)
{
  SF_DFPRINTF((stderr, "sendfile: Exiting...\n"));

  if (THIS->iovs) {
    free(THIS->iovs);
    THIS->iovs = NULL;
  }
  if (THIS->buffer) {
    free(THIS->buffer);
    THIS->buffer = NULL;
  }
  if (THIS->headers) {
    free_array(THIS->headers);
    THIS->headers = NULL;
  }
  if (THIS->trailers) {
    free_array(THIS->trailers);
    THIS->trailers = NULL;
  }
  if (THIS->from_file) {
    free_object(THIS->from_file);
    THIS->from_file = NULL;
  }
  if (THIS->to_file) {
    free_object(THIS->to_file);
    THIS->to_file = NULL;
  }
  if (THIS->args) {
    free_array(THIS->args);
    THIS->args = NULL;
  }
  if (THIS->self) {
    /* This can occur if Pike exits before the backend has started. */
    free_object(THIS->self);
    THIS->self = NULL;
  }
  /* This is not required since this is a mapped variable.
   * /Hubbe
   * But we do it anyway for paranoia reasons.
   * /grubba 1999-10-14
   */
  free_svalue(&(THIS->callback));
  THIS->callback.type = T_INT;
  THIS->callback.u.integer = 0;
  if (THIS->backend_callback) {
    remove_callback (THIS->backend_callback);
    THIS->backend_callback = NULL;
  }
}

/*
 * Fallback code
 */

#ifndef HAVE_WRITEV
#define writev my_writev
static size_t writev(int fd, struct iovec *iov, int n)
{
  if (n) {
    return fd_write(fd, iov->iov_base, iov->iov_len);
  }
  return 0;
}
#endif /* !HAVE_WRITEV */

/*
 * Helper functions
 */

static void sf_call_callback(struct pike_sendfile *this)
{
  debug_malloc_touch(this->args);

  if (this->callback.type != T_INT) {
    if (((this->callback.type == T_OBJECT) ||
	 ((this->callback.type == T_FUNCTION) &&
	  (this->callback.subtype != FUNCTION_BUILTIN))) &&
	!this->callback.u.object->prog) {
      /* Destructed object or function. */
      free_array(this->args);
      this->args = NULL;
    } else {
      int sz = this->args->size;

      push_int64(this->sent);
      push_array_items(this->args);
      this->args = NULL;

      apply_svalue(&this->callback, 1 + sz);
      pop_stack();
    }

    free_svalue(&this->callback);
    this->callback.type = T_INT;
    this->callback.u.integer = 0;
  } else {
    free_array(this->args);
    this->args = NULL;
  }
}

static void call_callback_and_free(struct callback *cb, void *this_, void *arg)
{
  struct pike_sendfile *this = this_;

  SF_DFPRINTF((stderr, "sendfile: Calling callback...\n"));

  remove_callback(cb);
  this->backend_callback = NULL;

  if (this->self) {
    /* Make sure we get freed in case of error */
    push_object(this->self);
    this->self = NULL;
  }

  sf_call_callback(this);

  /* Free ourselves */
  pop_stack();
}

/*
 * Code called in the threaded case. 
 */

/* writev() without the IOV_MAX limit. */
static ptrdiff_t send_iov(int fd, struct iovec *iov, int iovcnt)
{
  ptrdiff_t sent = 0;

  SF_DFPRINTF((stderr, "sendfile: send_iov(%d, %p, %d)...\n",
	       fd, iov, iovcnt));
#ifdef SF_DEBUG
  {
    int cnt;
    for(cnt = 0; cnt < iovcnt; cnt++) {
      SF_DFPRINTF((stderr, "sendfile: %4d: iov_base: %p, iov_len: %ld\n",
		   cnt, iov[cnt].iov_base,
		   DO_NOT_WARN((long)iov[cnt].iov_len)));
    }
  }
#endif /* SF_DEBUG */

  while (iovcnt) {
    ptrdiff_t bytes;
    int cnt = iovcnt;

#ifdef IOV_MAX
    if (cnt > IOV_MAX) cnt = IOV_MAX;
#endif

#ifdef DEF_IOV_MAX
    if (cnt > DEF_IOV_MAX) cnt = DEF_IOV_MAX;
#endif

#ifdef MAX_IOVEC
    if (cnt > MAX_IOVEC) cnt = MAX_IOVEC;
#endif

    bytes = writev(fd, iov, cnt);

    if ((bytes < 0) && (errno == EINTR)) {
      continue;
    } else if (bytes < 0) {
      /* Error or file closed at other end. */
      SF_DFPRINTF((stderr, "sendfile: send_iov(): writev() failed with errno:%d.\n"
		   "sendfile: Sent %ld bytes so far.\n",
		   errno, DO_NOT_WARN((long)sent)));
      return sent;
    } else {
      sent += bytes;

      while (bytes) {
	if ((size_t)bytes >= (size_t)iov->iov_len) {
	  bytes -= iov->iov_len;
	  iov++;
	  iovcnt--;
	} else {
	  iov->iov_base = ((char *)iov->iov_base) + bytes;
	  iov->iov_len -= bytes;
	  break;
	}
      }
    }
  }
  SF_DFPRINTF((stderr, "sendfile: send_iov(): Sent %d bytes\n",
	       sent));
  return sent;
}

void low_do_sendfile(struct pike_sendfile *this)
{
  /* Make sure we're using blocking I/O */
  set_nonblocking(this->to_fd, 0);

  SF_DFPRINTF((stderr, "sendfile: Worker started\n"));

  if ((this->from_file) && (this->len)) {
#if defined(HAVE_FREEBSD_SENDFILE) || defined(HAVE_HPUX_SENDFILE)
    off_t sent = 0;
    int len = this->len;
    int res;

#ifdef HAVE_FREEBSD_SENDFILE
    struct sf_hdtr hdtr = { NULL, 0, NULL, 0 };
    SF_DFPRINTF((stderr, "sendfile: Using FreeBSD-style sendfile()\n"));

    if (this->hd_cnt) {
      hdtr.headers = this->hd_iov;
      hdtr.hdr_cnt = this->hd_cnt;
    }
    if (this->tr_cnt) {
      hdtr.trailers = this->tr_iov;
      hdtr.trl_cnt = this->tr_cnt;
    }

#else /* !HAVE_FREEBSD_SENDFILE */
    /* HPUX_SENDFILE */
    struct iovec hdtr[2] = { NULL, 0, NULL, 0 };
    SF_DFPRINTF((stderr, "sendfile: Using HP/UX-style sendfile()\n"));

    /* NOTE: hd_cnt/tr_cnt are always 0 or 1 since
     * we've joined the headers/trailers in sf_create().
     */
    if (this->hd_cnt) {
      hdtr[0].iov_base = this->hd_iov->iov_base;
      hdtr[0].iov_len = this->hd_iov->iov_len;
    }
    if (this->tr_cnt) {
      hdtr[1].iov_base = this->tr_iov->iov_base;
      hdtr[1].iov_len = this->tr_iov->iov_len;
    }

#endif /* HAVE_FREEBSD_SENDFILE */

    if (len < 0) {
      len = 0;
    }

#ifdef HAVE_FREEBSD_SENDFILE
    res = sendfile(this->from_fd, this->to_fd, this->offset, len,
		   &hdtr, &sent, 0);
#else /* !HAVE_FREEBSD_SENDFILE */
    res = sendfile(this->to_fd, this->from_fd, this->offset, len,
		   hdtr, 0);
#endif /* HAVE_FREEBSD_SENDFILE */

    SF_DFPRINTF((stderr, "sendfile: sendfile() returned %d\n", res));

    if (res < 0) {
      switch(errno) {
      default:
      case ENOTSOCK:
      case EINVAL:
	/* Try doing it by hand instead. */
	goto fallback;
	break;
      case EFAULT:
	/* Bad arguments */
#ifdef HAVE_FREEBSD_SENDFILE
	Pike_fatal("FreeBSD style sendfile(): EFAULT\n");
#else /* !HAVE_FREEBSD_SENDFILE */
	Pike_fatal("HP/UX style sendfile(): EFAULT\n");
#endif /* HAVE_FREEBSD_SENDFILE */
	break;
      case EBADF:
      case ENOTCONN:
      case EPIPE:
      case EIO:
      case EAGAIN:
	/* Bad fd's or socket has been closed at other end. */
	break;
      }
#ifndef HAVE_FREEBSD_SENDFILE
    } else {
      sent = res;
#endif /* !HAVE_FREEBSD_SENDFILE */
    }
    this->sent += sent;

    goto done;

  fallback:
#endif /* HAVE_FREEBSD_SENDFILE || HAVE_HPUX_SENDFILE */

    SF_DFPRINTF((stderr, "sendfile: Sending headers\n"));

    /* Send headers */
    if (this->hd_cnt) {
      this->sent += send_iov(this->to_fd, this->hd_iov, this->hd_cnt);
    }

    SF_DFPRINTF((stderr, "sendfile: Sent %ld bytes so far.\n",
		 DO_NOT_WARN((long)this->sent)));
    
#if defined(HAVE_SENDFILE) && !defined(HAVE_FREEBSD_SENDFILE) && !defined(HAVE_HPUX_SENDFILE)
    SF_DFPRINTF((stderr, "sendfile: Sending file with sendfile()\n"));

    {
      int fail = sendfile(this->to_fd, this->from_fd,
			  &this->offset, this->len);

      if (fail < 0) {
	/* Failed: Try normal... */
	goto normal;
      }
      this->sent += fail;
      goto send_trailers;
    }
  normal:
#endif /* HAVE_SENDFILE && !HAVE_FREEBSD_SENDFILE && !HAVE_HPUX_SENDFILE */
    SF_DFPRINTF((stderr, "sendfile: Sending file by hand\n"));

#if 0 /* mmap is slower than read/write on most if not all systems */
#if defined(HAVE_MMAP) && defined(HAVE_MUNMAP)
    {
      PIKE_STAT_T st;

      if (!fd_fstat(this->from_fd, &st) &&
	  S_ISREG(st.st_mode)) {
	/* Regular file, try using mmap(). */

	SF_DFPRINTF((stderr,
		     "sendfile: from is a regular file - trying mmap().\n"));

	while (this->len) {
	  void *mem;
	  ptrdiff_t len = st.st_size - this->offset; /* To end of file */
	  char *buf;
	  int buflen;
	  if ((len > this->len) && (this->len >= 0)) {
	    len = this->len;
	  }
	  /* Try to limit memory space usage */
	  if (len > MMAP_SIZE) {
	    len = MMAP_SIZE;
	  }

	  if (!len) {
	    /* Done */
	    goto send_trailers;
	  }

	  mem = mmap(NULL, len, PROT_READ, MAP_FILE|MAP_SHARED,
		     this->from_fd, this->offset);
	  if (((long)mem) == ((long)MAP_FAILED)) {
	    /* Try using read & write instead. */
	    goto use_read_write;
	  }
#if defined(HAVE_MADVISE) && defined(MADV_SEQUENTIAL)
	  madvise(mem, len, MADV_SEQUENTIAL);
#endif /* HAVE_MADVISE && MADV_SEQUENTIAL */
	  buf = mem;
	  buflen = len;
	  while (buflen) {
	    int wrlen = fd_write(this->to_fd, buf, buflen);

	    if ((wrlen < 0) && (errno == EINTR)) {
	      continue;
	    } else if (wrlen < 0) {
	      munmap(mem, len);
	      goto send_trailers;
	    }
	    buf += wrlen;
	    buflen -= wrlen;
	    this->sent += wrlen;
	    this->offset += wrlen;
	    if (this->len > 0) {
	      this->len -= wrlen;
	    }
	  }
	  munmap(mem, len);
	}

	/* Shouldn't there be a goto here ? /Hubbe */
	/* True. Fixed. /grubba */
	goto send_trailers;
      }
    }
  use_read_write:
#endif /* HAVE_MMAP && HAVE_MUNMAP */
#endif
    SF_DFPRINTF((stderr, "sendfile: Using read() and write().\n"));

    fd_lseek(this->from_fd, this->offset, SEEK_SET);
    {
      ptrdiff_t buflen;
      ptrdiff_t len = this->len;
      if ((len > this->buf_size) || (len < 0)) {
	len = this->buf_size;
      }
      while ((buflen = fd_read(this->from_fd, this->buffer, len)) > 0) {
	char *buf = this->buffer;
	this->len -= buflen;
	this->offset += buflen;
	while (buflen) {
	  ptrdiff_t wrlen = fd_write(this->to_fd, buf, buflen);
	  if ((wrlen < 0) && (errno == EINTR)) {
	    continue;
	  } else if (wrlen < 0) {
	    goto send_trailers;
	  }
	  buf += wrlen;
	  buflen -= wrlen;
	  this->sent += wrlen;
	}
	len = this->len;
	if ((len > this->buf_size) || (len < 0)) {
	  len = this->buf_size;
	}
      }
    }
  send_trailers:
    SF_DFPRINTF((stderr, "sendfile: Sent %ld bytes so far.\n",
		 DO_NOT_WARN((long)this->sent)));
    
    /* No more need for the buffer */
    free(this->buffer);
    this->buffer = NULL;

    SF_DFPRINTF((stderr, "sendfile: Sending trailers.\n"));

    if (this->tr_cnt) {
      this->sent += send_iov(this->to_fd, this->tr_iov, this->tr_cnt);
    }
  } else {
    /* Only headers & trailers */
    struct iovec *iov = this->hd_iov;
    int iovcnt = this->hd_cnt;

    SF_DFPRINTF((stderr, "sendfile: Only headers & trailers.\n"));

    if (!iovcnt) {
      /* Only trailers */
      iovcnt = this->tr_cnt;
      iov = this->tr_iov;
    } else if (this->tr_cnt) {
      /* Both headers & trailers */
      if (iov + this->hd_cnt != this->tr_iov) {
	/* They are not back-to-back. Fix! */
	int i;
	struct iovec *iov_tmp = iov + this->hd_cnt;
	for (i=0; i < this->tr_cnt; i++) {
	  iov_tmp[i] = this->tr_iov[i];
	}
      }
      /* They are now back-to-back. */
      iovcnt += this->tr_cnt;
    }
    /* All iovec's are now in iov & iovcnt */

    this->sent += send_iov(this->to_fd, iov, iovcnt);
  }

#if defined(HAVE_FREEBSD_SENDFILE) || defined(HAVE_HPUX_SENDFILE)
 done:
#endif /* HAVE_FREEBSD_SENDFILE || HAVE_HPUX_SENDFILE */

  SF_DFPRINTF((stderr,
	      "sendfile: Done. Setting up callback\n"
	      "%d bytes sent\n", this->sent));
}

static void worker(void *this_)
{
  struct pike_sendfile *this = this_;

  low_do_sendfile(this);

  low_mt_lock_interpreter();	/* Can run even if threads_disabled. */

  /*
   * Unlock the fd's
   */
  if (this->from) {
    if (this->from_file->prog) {
      this->from->flags &= ~FILE_LOCK_FD;
    } else {
      /* Destructed. */
      if (this->from_fd >= 0) {
	set_backend_for_fd(this->from_fd, NULL);
	fd_close(this->from_fd);
      }
      /* Paranoia */
      this->from->fd = -1;
    }
  }
  if (this->to_file->prog) {
    this->to->flags &= ~FILE_LOCK_FD;
  } else {
    /* Destructed */
    if (this->to_fd >= 0) {
      set_backend_for_fd(this->to_fd, NULL);
      fd_close(this->to_fd);
    }
    /* Paranoia */
    this->to->fd = -1;
  }
    
  /* Neither of the following can be done in our current context
   * so we do them from a backend callback.
   * * Call the callback.
   * * Get rid of extra ref to the object, and free ourselves.
   */
#ifdef PIKE_DEBUG
  if (this->backend_callback)
    Pike_fatal ("Didn't expect a backend callback to be installed already.\n");
#endif
  this->backend_callback =
    add_backend_callback(call_callback_and_free, this, 0);

  /* Call as soon as possible. */
  next_timeout.tv_usec = 0;
  next_timeout.tv_sec = 0;

  /* Wake up the backend */
  wake_up_backend();

  /* We're gone... */
  num_threads--;
    
  mt_unlock_interpreter();

  /* Die */
  return;
}

/*
 * Functions callable from Pike code
 */

/* void create(array(string) headers, object from, int offset, int len,
 *             array(string) trailers, object to, 
 *             function callback, mixed ... args)
 */
static void sf_create(INT32 args)
{
  struct pike_sendfile sf;
  int iovcnt = 0;
  struct svalue *cb = NULL;

  if (THIS->to_file) {
    Pike_error("sendfile->create(): Called a second time!\n");
  }

  /* In case the user has succeeded in initializing _callback
   * before create() is called.
   */
  free_svalue(&(THIS->callback));
  THIS->callback.type = T_INT;
  THIS->callback.u.integer = 0;

  /* NOTE: The references to the stuff in sf are held by the stack.
   * This means that we can throw errors without needing to clean up.
   */

  MEMSET(&sf, 0, sizeof(struct pike_sendfile));
  sf.callback.type = T_INT;

  get_all_args("sendfile", args, "%A%O%l%l%A%o%*",
	       &(sf.headers), &(sf.from_file), &(sf.offset),
	       &(sf.len), &(sf.trailers), &(sf.to_file), &cb);

  /* We need to give 'cb' another reference /Hubbe */
  /* No, we don't. We steal the reference from the stack.
   * /grubba
   */
  /* assign_svalue(&(sf.callback),cb); */
  sf.callback = *cb;

  /* Fix the trailing args */
  push_array(sf.args = aggregate_array(args-7));
  args = 8;

  /* FIXME: Ought to fix fp so that the backtraces look right. */

  /* Check that we're called with the right kind of objects. */
  if (sf.to_file->prog == file_program) {
    sf.to = (struct my_file *)(sf.to_file->storage);
  } else if (!(sf.to = (struct my_file *)get_storage(sf.to_file,
						     file_program))) {
    struct object **ob;
    if (!(ob = (struct object **)get_storage(sf.to_file, file_ref_program)) ||
	!(*ob) ||
	!(sf.to = (struct my_file *)get_storage(*ob, file_program))) {
      SIMPLE_BAD_ARG_ERROR("sendfile", 6, "object(Stdio.File)");
    }
    add_ref(*ob);
#ifdef PIKE_DEBUG
    if ((sp[5-args].type != T_OBJECT) ||
	(sp[5-args].u.object != sf.to_file)) {
      Pike_fatal("sendfile: Stack out of sync(1).\n");
    }
#endif /* PIKE_DEBUG */
    sf.to_file = *ob;
    free_object(sp[5-args].u.object);
    sp[5-args].u.object = sf.to_file;
  }
  if ((sf.to->flags & FILE_LOCK_FD) ||
      (sf.to->fd < 0)) {
    SIMPLE_BAD_ARG_ERROR("sendfile", 6, "object(Stdio.File)");
  }
  sf.to_fd = sf.to->fd;

  if (sf.from_file && !sf.len) {
    /* No need for the from_file object. */
    /* NOTE: We need to zap from_file from the stack too. */
    free_object(sf.from_file);
    sf.from_file = NULL;
    sf.from = NULL;
    sp[1-args].type = T_INT;
    sp[1-args].u.integer = 0;
  }

  if (sf.from_file) {
    if (sf.from_file->prog == file_program) {
      sf.from = (struct my_file *)(sf.from_file->storage);
    } else if (!(sf.from = (struct my_file *)get_storage(sf.from_file,
							 file_program))) {
      struct object **ob;
      if (!(ob = (struct object **)get_storage(sf.from_file,
					       file_ref_program)) ||
	!(*ob) ||
	!(sf.from = (struct my_file *)get_storage(*ob, file_program))) {
	SIMPLE_BAD_ARG_ERROR("sendfile", 2, "object(Stdio.File)");
      }
      add_ref(*ob);
#ifdef PIKE_DEBUG
      if ((sp[1-args].type != T_OBJECT) ||
	  (sp[1-args].u.object != sf.from_file)) {
	Pike_fatal("sendfile: Stack out of sync(2).\n");
      }
#endif /* PIKE_DEBUG */
      sf.from_file = *ob;
      free_object(sp[1-args].u.object);
      sp[1-args].u.object = sf.from_file;
    }
    if ((sf.from->flags & FILE_LOCK_FD) ||
	(sf.from->fd < 0)) {
      SIMPLE_BAD_ARG_ERROR("sendfile", 2, "object(Stdio.File)");
    }
    sf.from_fd = sf.from->fd;
  }

  /* Do some extra arg checking */
  sf.hd_cnt = 0;
  if (sf.headers) {
    struct array *a = sf.headers;
    int i;

    for (i=0; i < a->size; i++) {
      if ((a->item[i].type != T_STRING) || (a->item[i].u.string->size_shift)) {
	SIMPLE_BAD_ARG_ERROR("sendfile", 1, "array(string)");
      }
    }
    iovcnt = a->size;
    sf.hd_cnt = a->size;
  }

  sf.tr_cnt = 0;
  if (sf.trailers) {
    struct array *a = sf.trailers;
    int i;

    for (i=0; i < a->size; i++) {
      if ((a->item[i].type != T_STRING) || (a->item[i].u.string->size_shift)) {
	SIMPLE_BAD_ARG_ERROR("sendfile", 5, "array(string)");
      }
    }
    iovcnt += a->size;
    sf.tr_cnt = a->size;
  }

  /* Set up the iovec's */
  if (iovcnt) {
#ifdef HAVE_HPUX_SENDFILE
    iovcnt = 2;
#endif /* HAVE_HPUX_SENDFILE */

    sf.iovs = (struct iovec *)xalloc(sizeof(struct iovec) * iovcnt);

    sf.hd_iov = sf.iovs;
#ifdef HAVE_HPUX_SENDFILE
    sf.tr_iov = sf.iovs + 1;
#else /* !HAVE_HPUX_SENDFILE */
    sf.tr_iov = sf.iovs + sf.hd_cnt;
#endif /* HAVE_HPUX_SENDFILE */

    if (sf.headers) {
      int i;
      for (i = sf.hd_cnt; i--;) {
	struct pike_string *s;
#ifdef HAVE_HPUX_SENDFILE
	ref_push_string(sf.headers->item[i].u.string);
#else /* !HAVE_HPUX_SENDFILE */
	if ((s = sf.headers->item[i].u.string)->len) {
	  sf.hd_iov[i].iov_base = s->str;
	  sf.hd_iov[i].iov_len = s->len;
	} else {
	  sf.hd_iov++;
	  sf.hd_cnt--;
	}
#endif /* HAVE_HPUX_SENDFILE */
      }
#ifdef HAVE_HPUX_SENDFILE
      if (sf.hd_cnt) {
	f_add(sf.hd_cnt);
	free_string(sf.headers->item->u.string);
	sf.headers->item->u.string = sp[-1].u.string;
	sp--;
	dmalloc_touch_svalue(sp);
	sf.hd_iov->iov_base = sf.headers->item->u.string->str;
	sf.hd_iov->iov_len = sf.headers->item->u.string->len;
      } else {
	sf.hd_iov->iov_base = NULL;
	sf.hd_iov->iov_len = 0;
      }
#endif /* HAVE_HPUX_SENDFILE */
    }
    if (sf.trailers) {
      int i;
      for (i = sf.tr_cnt; i--;) {
	struct pike_string *s;
#ifdef HAVE_HPUX_SENDFILE
	ref_push_string(sf.trailers->item[i].u.string);
#else /* !HAVE_HPUX_SENDFILE */
	if ((s = sf.trailers->item[i].u.string)->len) {
	  sf.tr_iov[i].iov_base = s->str;
	  sf.tr_iov[i].iov_len = s->len;
	} else {
	  sf.tr_iov++;
	  sf.tr_cnt--;
	}
#endif /* HAVE_HPUX_SENDFILE */
      }
#ifdef HAVE_HPUX_SENDFILE
      if (sf.tr_cnt) {
	f_add(sf.tr_cnt);
	free_string(sf.trailers->item->u.string);
	sf.trailers->item->u.string = sp[-1].u.string;
	sp--;
	dmalloc_touch_svalue(sp);
	sf.tr_iov->iov_base = sf.trailers->item->u.string->str;
	sf.tr_iov->iov_len = sf.trailers->item->u.string->len;
      } else {
	sf.tr_iov->iov_base = NULL;
	sf.tr_iov->iov_len = 0;
      }
#endif /* HAVE_HPUX_SENDFILE */
    }
  }

  /* We need to copy the arrays since the user might do destructive
   * operations on them, and we need the arrays to keep references to
   * the strings.
   */
  if ((sf.headers) && (sf.headers->refs > 1)) {
    struct array *a = copy_array(sf.headers);
#ifdef PIKE_DEBUG
    if ((sp[-args].type != T_ARRAY) || (sp[-args].u.array != sf.headers)) {
      Pike_fatal("sendfile: Stack out of sync(3).\n");
    }
#endif /* PIKE_DEBUG */
    free_array(sf.headers);
    sp[-args].u.array = a;
    sf.headers = a;
  }
  if ((sf.trailers) && (sf.trailers->refs > 1)) {
    struct array *a = copy_array(sf.trailers);
#ifdef PIKE_DEBUG
    if ((sp[4-args].type != T_ARRAY) || (sp[4-args].u.array != sf.trailers)) {
      Pike_fatal("sendfile: Stack out of sync(4).\n");
    }
#endif /* PIKE_DEBUG */
    free_array(sf.trailers);
    sp[4-args].u.array = a;
    sf.trailers = a;
  }

  if (sf.from_file) {
    /* We may need a buffer to hold the data */
    if (sf.iovs) {
      ONERROR tmp;
      SET_ONERROR(tmp, free, sf.iovs);

      sf.buffer = (char *)xalloc(BUF_SIZE);

      UNSET_ONERROR(tmp);
    } else {
      sf.buffer = (char *)xalloc(BUF_SIZE);
    }
    sf.buf_size = BUF_SIZE;
  }

  {
    /* Threaded blocking mode possible */

    /* Make sure both file objects are in blocking mode.
     */
    if (sf.from_file) {
      /* set_blocking */
      set_nonblocking(sf.from_fd, 0);
      sf.from->open_mode &= ~FILE_NONBLOCKING;

      /* Fix offset */
      if (sf.offset < 0) {
	sf.offset = fd_lseek(sf.from_fd, 0, SEEK_CUR);
	if (sf.offset < 0) {
	  sf.offset = 0;
	}
      }
    }
    /* set_blocking */
    set_nonblocking(sf.to_fd, 0);
    sf.to->open_mode &= ~FILE_NONBLOCKING;

    /*
     * Setup done. Note that we keep refs to all refcounted svalues in
     * our object.
     */
    sp -= args;
    *THIS = sf;
    DO_IF_DMALLOC(while(args--) dmalloc_touch_svalue(sp + args));
    args = 0;

    /* FIXME: Ought to fix fp so that the backtraces look right. */

    /* Note: we hold a reference to ourselves.
     * The gc() won't find it, so be carefull.
     */
    add_ref(THIS->self = Pike_fp->current_object);

    /*
     * Make sure the user can't modify the fd's under us.
     */
    sf.to->flags |= FILE_LOCK_FD;
    if (sf.from) {
      sf.from->flags |= FILE_LOCK_FD;
    }

    /* It is a good idea to increase num_threads before the thread
     * is actually created, otherwise the backend (or somebody) may
     * be hogging the interpreter lock... /Hubbe
     */
    num_threads++;

    /* The worker will have a ref. */
    th_farm(worker, THIS);
#if 0
    {
      /* Failure */
      sf.to->flags &= ~FILE_LOCK_FD;
      if (sf.from) {
	sf.from->flags &= ~FILE_LOCK_FD;
      }
      free_object(THIS->self);
      resource_error("Stdio.sendfile", sp, 0, "threads", 1,
		     "Failed to create thread.\n");
    }
#endif /* 0 */
  }
  return;
}

#endif /* _REENTRANT */

/*
 * Module init code
 */
void init_sendfile(void)
{
#ifdef _REENTRANT
  start_new_program();
  ADD_STORAGE(struct pike_sendfile);
  map_variable("_args", "array(mixed)", 0, OFFSETOF(pike_sendfile, args),
	       T_ARRAY);
  map_variable("_callback", "function(int,mixed...:void)", 0,
	       OFFSETOF(pike_sendfile, callback), T_MIXED);

  /* function(array(string),object,int,int,array(string),object,function(int,mixed...:void),mixed...:void) */
  ADD_FUNCTION("create", sf_create,
	       tFuncV(tArr(tStr) tObj tInt tInt tArr(tStr) tObj
		      tFuncV(tInt, tMix, tVoid), tMix, tVoid), 0);

  set_init_callback(init_pike_sendfile);
  set_exit_callback(exit_pike_sendfile);

  pike_sendfile_prog = end_program();
  add_program_constant("sendfile", pike_sendfile_prog, 0);
#endif /* _REENTRANT */
}

void exit_sendfile(void)
{
#ifdef _REENTRANT
  if (pike_sendfile_prog) {
    free_program(pike_sendfile_prog);
    pike_sendfile_prog = NULL;
  }
#endif /* _REENTRANT */
}
