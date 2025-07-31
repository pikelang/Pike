/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*! @module Stdio
 */

/*! @class sendfile
 *!
 *! Send @expr{headers + from_fd[off..off+len-1] + trailers@} to
 *! @expr{to_fd@} asyncronously.
 *!
 *! @note
 *!   This is the low-level implementation, which has several limitations.
 *!   You probably want to use @[Stdio.sendfile()] instead.
 *!
 *! @seealso
 *!   @[Stdio.sendfile()]
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

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

#include <sys/stat.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif /* HAVE_SYS_UIO_H */

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif /* HAVE_NETINET_TCP_H */

#ifdef HAVE_SYS_SENDFILE_H
#ifndef HAVE_BROKEN_SYS_SENDFILE_H
#include <sys/sendfile.h>
#endif /* !HAVE_BROKEN_SYS_SENDFILE_H */
#endif /* HAVE_SYS_SENDFILE_H */

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

#if !defined(SOL_TCP) && defined(IPPROTO_TCP)
    /* SOL_TCP isn't defined in Solaris. */
#define SOL_TCP	IPPROTO_TCP
#endif

/*
 * Only support for threaded operation right now.
 */
#ifdef _REENTRANT

#undef THIS
#define THIS	((struct pike_sendfile *)(Pike_fp->current_storage))

/*
 * We assume sendfile(2) has been fixed on all OS's by now.
 * FIXME: Configure tests?
 *	/grubba 2004-03-30
 */
#if 0
/*
 * All known versions of sendfile(2) are broken.
 *	/grubba 2000-10-19
 */
#ifndef HAVE_BROKEN_SENDFILE
#define HAVE_BROKEN_SENDFILE
#endif /* !HAVE_BROKEN_SENDFILE */
#endif /* 0 */

/*
 * Disable any use of sendfile(2) if HAVE_BROKEN_SENDFILE is defined.
 */
#ifdef HAVE_BROKEN_SENDFILE
#undef HAVE_SENDFILE
#undef HAVE_FREEBSD_SENDFILE
#undef HAVE_HPUX_SENDFILE
#undef HAVE_MACOSX_SENDFILE
#endif /* HAVE_BROKEN_SENDFILE */

/*
 * Globals
 */

static struct program *pike_sendfile_prog = NULL;

/*
 * Struct init code.
 */

static void exit_pike_sendfile(struct object *UNUSED(o))
{
  SF_DFPRINTF((stderr, "sendfile: Exiting...\n"));

  if (THIS->iovs)
    free(THIS->iovs);

  if (THIS->buffer)
    free(THIS->buffer);

  if (THIS->headers)
    free_array(THIS->headers);

  if (THIS->trailers)
    free_array(THIS->trailers);

  if (THIS->from_file)
    free_object(THIS->from_file);

  if (THIS->to_file)
    free_object(THIS->to_file);

  /* This can occur if Pike exits before the backend has started. */
  if (THIS->self)
    free_object(THIS->self);

  if (THIS->backend_callback)
    remove_callback (THIS->backend_callback);
}

/*
 * Helper functions
 */

static void sf_call_callback(struct pike_sendfile *this)
{
  debug_malloc_touch(this->args);

  if (TYPEOF(this->callback) != T_INT) {
    if (((TYPEOF(this->callback) == T_OBJECT) ||
	 ((TYPEOF(this->callback) == T_FUNCTION) &&
	  (SUBTYPEOF(this->callback) != FUNCTION_BUILTIN))) &&
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
    SET_SVAL(this->callback, T_INT, NUMBER_NUMBER, integer, 0);
  } else {
    free_array(this->args);
    this->args = NULL;
  }
}

static void call_callback_and_free(struct callback *cb, void *this_, void *UNUSED(arg))
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

void low_do_sendfile(struct pike_sendfile *this)
{
  int oldbulkmode = bulkmode_start(this->to_fd);
  INT64 *offsetp = (this->offset >= 0) ? &this->offset : NULL;
  INT64 sent = 0;

  /* Make sure we're using blocking I/O */
  set_nonblocking(this->to_fd, 0);

  SF_DFPRINTF((stderr, "sendfile: Worker started\n"));

  do {
    sent = pike_sendfile(this->to_fd,
                         this->hd_iov, this->hd_cnt,
                         this->from_fd, offsetp, this->len,
                         this->tr_iov, this->tr_cnt);
  } while ((sent < 0) && (errno == EINTR));

  if (sent >= 0) {
    this->sent += sent;
  }

  bulkmode_restore(this->to_fd, oldbulkmode);

  SF_DFPRINTF((stderr,
	      "sendfile: Done. Setting up callback\n"
	      "%d bytes sent\n", this->sent));

  /* FIXME: Restore non-blocking mode here? */
}

static void worker(void *this_)
{
  struct pike_sendfile *this = this_;
  struct Backend_struct *backend;
  struct timeval tv;

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
	fd_close(this->from_fd);
      }
      /* Paranoia */
      change_fd_for_box (&this->from->box, -1);
    }
  }
  if (this->to_file->prog) {
    this->to->flags &= ~FILE_LOCK_FD;
  } else {
    /* Destructed */
    fd_close(this->to_fd);

    /* Paranoia */
    change_fd_for_box (&this->to->box, -1);
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

  if (!(backend = this->to->box.backend)) {
    backend = default_backend;
  }

  this->backend_callback =
    backend_add_backend_callback(backend, call_callback_and_free, this, 0);

  /* Call as soon as possible. */
  tv.tv_usec = 0;
  tv.tv_sec = 0;
  backend_lower_timeout(backend, &tv);

  /* Wake up the backend */
  backend_wake_up_backend(backend);

  /* We're gone... */
  num_threads--;

  mt_unlock_interpreter();

  /* Die */
  return;
}

/*
 * Functions callable from Pike code
 */

/*! @decl void create(array(string) headers, object from, int offset, int len, @
 *!             array(string) trailers, object to, @
 *!             function callback, mixed ... args)
 *!
 *! Low-level implementation of @[Stdio.sendfile()].
 *!
 *! Sends @[headers] followed by @[len] bytes starting at @[offset]
 *! from the file @[from] followed by @[trailers] to the file @[to].
 *! When completed @[callback] will be called with the total number of
 *! bytes sent as the first argument, followed by @[args].
 *!
 *! Any of @[headers], @[from] and @[trailers] may be left out
 *! by setting them to @expr{0@}.
 *!
 *! Setting @[offset] to @expr{-1@} means send from the current position in
 *! @[from].
 *!
 *! Setting @[len] to @expr{-1@} means send until @[from]'s end of file is
 *! reached.
 *!
 *! @note
 *!   Don't use this class directly! Use @[Stdio.sendfile()] instead.
 *!
 *!   In Pike 7.7 and later the @[callback] function will be called
 *!   from the backend associated with @[to].
 *!
 *! @note
 *!   May use blocking I/O and thus trigger process being killed
 *!   with @tt{SIGPIPE@} when the other end closes the connection.
 *!   Add a call to @[signal()] to avoid this.
 *!
 *! @seealso
 *!   @[Stdio.sendfile()]
 */
static void sf_create(INT32 args)
{
  struct pike_sendfile sf;
  int iovcnt = 0;
  struct svalue *cb = NULL;
  INT64 offset, len;

  if (THIS->to_file) {
    Pike_error("sendfile->create(): Called a second time!\n");
  }

  /* In case the user has succeeded in initializing _callback
   * before create() is called.
   */
  free_svalue(&(THIS->callback));
  SET_SVAL(THIS->callback, T_INT, NUMBER_NUMBER, integer, 0);

  /* NOTE: The references to the stuff in sf are held by the stack.
   * This means that we can throw errors without needing to clean up.
   */

  memset(&sf, 0, sizeof(struct pike_sendfile));
  SET_SVAL(sf.callback, T_INT, NUMBER_NUMBER, integer, 0);

  get_all_args(NULL, args, "%A%O%l%l%A%o%*",
	       &(sf.headers), &(sf.from_file), &offset,
	       &len, &(sf.trailers), &(sf.to_file), &cb);

  sf.offset = offset;
  sf.len = len;

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
  } else if (!(sf.to = get_storage(sf.to_file, file_program))) {
    struct svalue *sval;
    if (!(sval = get_storage(sf.to_file, file_ref_program)) ||
	(TYPEOF(*sval) != T_OBJECT) ||
	!(sf.to = get_storage(sval->u.object, file_program))) {
      SIMPLE_ARG_TYPE_ERROR("sendfile", 6, "object(Stdio.File)");
    }
    add_ref(sval->u.object);
#ifdef PIKE_DEBUG
    if ((TYPEOF(sp[5-args]) != T_OBJECT) ||
	(sp[5-args].u.object != sf.to_file)) {
      Pike_fatal("sendfile: Stack out of sync(1).\n");
    }
#endif /* PIKE_DEBUG */
    sf.to_file = sval->u.object;
    free_object(sp[5-args].u.object);
    sp[5-args].u.object = sf.to_file;
  }
  if ((sf.to->flags & FILE_LOCK_FD) ||
      (sf.to->box.fd < 0)) {
    SIMPLE_ARG_TYPE_ERROR("sendfile", 6, "object(Stdio.File)");
  }
  sf.to_fd = sf.to->box.fd;

  if (sf.from_file && !sf.len) {
    /* No need for the from_file object. */
    /* NOTE: We need to zap from_file from the stack too. */
    free_object(sf.from_file);
    sf.from_file = NULL;
    sf.from = NULL;
    SET_SVAL(sp[1-args], T_INT, NUMBER_NUMBER, integer, 0);
  }

  if (sf.from_file) {
    if (sf.from_file->prog == file_program) {
      sf.from = (struct my_file *)(sf.from_file->storage);
    } else if (!(sf.from = get_storage(sf.from_file, file_program))) {
      struct svalue *sval;
      if (!(sval = get_storage(sf.from_file, file_ref_program)) ||
	  (TYPEOF(*sval) != T_OBJECT) ||
	!(sf.from = get_storage(sval->u.object, file_program))) {
	SIMPLE_ARG_TYPE_ERROR("sendfile", 2, "object(Stdio.File)");
      }
      add_ref(sval->u.object);
#ifdef PIKE_DEBUG
      if ((TYPEOF(sp[1-args]) != T_OBJECT) ||
	  (sp[1-args].u.object != sf.from_file)) {
	Pike_fatal("sendfile: Stack out of sync(2).\n");
      }
#endif /* PIKE_DEBUG */
      sf.from_file = sval->u.object;
      free_object(sp[1-args].u.object);
      sp[1-args].u.object = sf.from_file;
    }
    if ((sf.from->flags & FILE_LOCK_FD) ||
	(sf.from->box.fd < 0)) {
      SIMPLE_ARG_TYPE_ERROR("sendfile", 2, "object(Stdio.File)");
    }
    sf.from_fd = sf.from->box.fd;
  } else {
    sf.from_fd = -1;
  }

  /* Do some extra arg checking */
  sf.hd_cnt = 0;
  if (sf.headers) {
    struct array *a = sf.headers;
    int i;

    for (i=0; i < a->size; i++) {
      if ((TYPEOF(a->item[i]) != T_STRING) || (a->item[i].u.string->size_shift)) {
	SIMPLE_ARG_TYPE_ERROR("sendfile", 1, "array(string)");
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
      if ((TYPEOF(a->item[i]) != T_STRING) || (a->item[i].u.string->size_shift)) {
	SIMPLE_ARG_TYPE_ERROR("sendfile", 5, "array(string)");
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

    sf.iovs = xalloc(sizeof(struct iovec) * iovcnt);

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
	sf.hd_cnt = 1;
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
    if ((TYPEOF(sp[-args]) != T_ARRAY) || (sp[-args].u.array != sf.headers)) {
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
    if ((TYPEOF(sp[4-args]) != T_ARRAY) || (sp[4-args].u.array != sf.trailers)) {
      Pike_fatal("sendfile: Stack out of sync(4).\n");
    }
#endif /* PIKE_DEBUG */
    free_array(sf.trailers);
    sp[4-args].u.array = a;
    sf.trailers = a;
  }

  if (sf.from_file) {
    /* We may need a buffer to hold the data */
    ONERROR tmp;
    SET_ONERROR(tmp, free, sf.iovs);
    sf.buffer = xalloc(BUF_SIZE);
    UNSET_ONERROR(tmp);
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
     *
     * Note also that the auto variable sf is copied to the object here.
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
      Pike_error("Failed to create thread.\n");
    }
#endif /* 0 */
  }
  return;
}

/*! @endclass
 */

#endif /* _REENTRANT */

/*
 * Module init code
 */
void init_stdio_sendfile(void)
{
#ifdef _REENTRANT
  START_NEW_PROGRAM_ID (STDIO_SENDFILE);
  ADD_STORAGE(struct pike_sendfile);
  PIKE_MAP_VARIABLE("_args", OFFSETOF(pike_sendfile, args),
                    tArray, T_ARRAY, 0);
  PIKE_MAP_VARIABLE("_callback", OFFSETOF(pike_sendfile, callback),
                    tFuncV(tInt,tMix,tVoid), T_MIXED, 0);

  /* function(array(string),object,int,int,array(string),object,function(int,mixed...:void),mixed...:void) */
  ADD_FUNCTION("create", sf_create,
	       tFuncV(tOr(tArr(tStr), tZero) tOr(tObj, tZero) tInt tInt
		      tOr(tArr(tStr), tZero) tObj
		      tFuncV(tInt, tUnknown, tVoid), tMix, tVoid), 0);

  set_exit_callback(exit_pike_sendfile);

  pike_sendfile_prog = end_program();
  add_program_constant("sendfile", pike_sendfile_prog, 0);
#endif /* _REENTRANT */
}

void exit_stdio_sendfile(void)
{
#ifdef _REENTRANT
  if (pike_sendfile_prog) {
    free_program(pike_sendfile_prog);
    pike_sendfile_prog = NULL;
  }
#endif /* _REENTRANT */
}

/*! @endmodule
 */
