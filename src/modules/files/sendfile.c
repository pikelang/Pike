/*
 * $Id: sendfile.c,v 1.2 1999/04/14 21:43:01 grubba Exp $
 *
 * Sends headers + from_fd[off..off+len-1] + trailers to to_fd asyncronously.
 *
 * Henrik Grubbström 1999-04-02
 */

#include "global.h"
#include "config.h"

#include "fd_control.h"
#include "object.h"
#include "array.h"
#include "threads.h"
#include "interpret.h"
#include "svalue.h"
#include "callback.h"
#include "backend.h"
#include "module_support.h"

#include <errno.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#include <unistd.h>

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif /* HAVE_SYS_UIO_H */

/* #define SF_DEBUG */

#ifdef SF_DEBUG
#define SF_DFPRINTF(X)	fprintf X
#else /* !SF_DEBUG */
#define SF_DFPRINTF(X)
#endif /* SF_DEBUG */

#define BUF_SIZE 65536


/*
 * Only support for threaded operation right now.
 */
#ifdef _REENTRANT


/*
 * Struct's
 */

#ifndef HAVE_STRUCT_IOVEC
struct iovec {
  void *iov_base;
  int iov_len;
};
#endif /* !HAVE_STRUCT_IOVEC */


struct pike_sendfile
{
  struct object *self;

  int sent;

  struct array *headers;
  struct array *trailers;

  struct object *from_file;
  struct object *to_file;

  struct svalue callback;
  struct array *args;

  int from_fd;
  int to_fd;

  INT_TYPE offset;
  INT_TYPE len;

  struct iovec *hd_iov;
  struct iovec *tr_iov;

  int hd_cnt;
  int tr_cnt;

  struct iovec *iovs;
  char *buffer;
};

#define THIS	((struct pike_sendfile *)(fp->current_storage))

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

  THIS->callback.type = T_INT;
}

static void exit_pike_sendfile(struct object *o)
{
  if (THIS->iovs) {
    free(THIS->iovs);
  }
  if (THIS->buffer) {
    free(THIS->buffer);
  }
  if (THIS->headers) {
    free_array(THIS->headers);
  }
  if (THIS->trailers) {
    free_array(THIS->trailers);
  }
  if (THIS->from_file) {
    free_object(THIS->from_file);
  }
  if (THIS->to_file) {
    free_object(THIS->to_file);
  }
  if (THIS->args) {
    free_array(THIS->args);
  }
  if (THIS->self) {
    /* This can occur if Pike exits before the backend has started. */
    free_object(THIS->self);
    THIS->self = NULL;
  }
  free_svalue(&(THIS->callback));
}

/*
 * Fallback code
 */

#ifndef HAVE_WRITEV
#define writev my_writev
static int writev(int fd, struct iovec *iov, int n)
{
  if (n) {
    return write(fd, iov->iov_data, iov->iov_len);
  }
  return 0;
}
#endif /* !HAVE_WRITEV */

/*
 * Helper functions
 */

void sf_call_callback(struct pike_sendfile *this)
{
  if (this->callback.type != T_INT) {
    int sz = this->args->size;

    push_int(this->sent);
    push_array_items(this->args);
    this->args = NULL;

    apply_svalue(&this->callback, 1 + sz);
    pop_stack();

    free_svalue(&this->callback);
    this->callback.type = T_INT;
    this->callback.u.integer = 0;
  } else {
    free_array(this->args);
    this->args = NULL;
  }
}

void call_callback_and_free(struct callback *cb, void *this_, void *arg)
{
  struct pike_sendfile *this = this_;
  int sz;

  SF_DFPRINTF((stderr, "sendfile: Calling callback...\n"));

  remove_callback(cb);

#ifdef _REENTRANT
  if (this->self) {
    /* Make sure we get freed in case of error */
    push_object(this->self);
    this->self = NULL;
  }
#endif /* _REENTRANT */

  sf_call_callback(this);

  /* Free ourselves */
  pop_stack();
}

/*
 * Code called in the threaded case. 
 */

/* writev() without the IOV_MAX limit. */
int send_iov(int fd, struct iovec *iov, int iovcnt)
{
  int sent = 0;

  while (iovcnt) {
    int bytes;
    int cnt = iovcnt;

#ifdef IOV_MAX
    if (cnt > IOV_MAX) cnt = IOV_MAX;
#endif

#ifdef MAX_IOVEC
    if (cnt > MAX_IOVEC) cnt = MAX_IOVEC;
#endif

    bytes = writev(fd, iov, cnt);

    if ((bytes < 0) && (errno == EINTR)) {
      continue;
    } else if (bytes <= 0) {
      /* Error or file closed at other end. */
      return sent;
    } else {
      sent += bytes;

      while (bytes) {
	if (bytes >= iov->iov_len) {
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
  return sent;
}

void *worker(void *this_)
{
  struct pike_sendfile *this = this_;

  /* Make sure we're using blocking I/O */
  set_nonblocking(this->to_fd, 0);

  SF_DFPRINTF((stderr, "sendfile: Worker started\n"));

  if ((this->from_file) && (this->len)) {
    struct iovec *iov;
    int iovcnt;

#ifdef HAVE_FREEBSD_SENDFILE
    struct sf_hdtr hdtr = { NULL, 0, NULL, 0 };
    off_t sent = 0;
    int len = this->len;

    SF_DFPRINTF((stderr, "sendfile: Using FreeBSD-style sendfile()\n"));

    if (this->hd_cnt) {
      hdtr.headers = this->hd_iov;
      hdtr.hdr_cnt = this->hd_cnt;
    }
    if (this->tr_cnt) {
      hdtr.trailers = this->tr_iov;
      hdtr.trl_cnt = this->tr_cnt;
    }

    if (len < 0) {
      len = 0;
    }

    if (sendfile(this->from_fd, this->to_fd, len, this->offset,
		 &hdtr, &sent, 0) < 0) {
      switch(errno) {
      default:
      case ENOTSOCK:
      case EINVAL:
	/* Try doing it by hand instead. */
	goto fallback;
	break;
      case EFAULT:
	/* Bad arguments */
	fatal("FreeBSD style sendfile(): EFAULT\n");
	break;
      case EBADF:
      case ENOTCON:
      case EPIPE:
      case EIO:
      case EAGAIN:
	/* Bad fd's or socket has been closed at other end. */
	break;
      }
    }
    this->sent += sent;

    goto done;

  fallback:
#endif /* !HAVE_FREEBSD_SENDFILE */

    SF_DFPRINTF((stderr, "sendfile: Sending headers\n"));

    /* Send headers */
    if (this->hd_cnt) {
      this->sent += send_iov(this->to_fd, this->hd_iov, this->hd_cnt);
    }
    
#if defined(HAVE_SENDFILE) && !defined(HAVE_FREEBSD_SENDFILE)
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
#endif /* HAVE_SENDFILE && !HAVE_FREEBSD_SENDFILE */
    SF_DFPRINTF((stderr, "sendfile: Sending file by hand\n"));

    lseek(this->from_fd, this->offset, SEEK_SET);
    {
      int buflen;
      int len = this->len;
      if ((len > BUF_SIZE) || (len < 0)) {
	len = BUF_SIZE;
      }
      while ((buflen = read(this->from_fd, this->buffer, len)) > 0) {
	char *buf = this->buffer;
	this->len -= buflen;
	this->offset += buflen;
	while (buflen) {
	  int wrlen = write(this->to_fd, buf, buflen);
	  if ((wrlen < 0) && (errno == EINTR)) {
	    continue;
	  } else if (wrlen <= 0) {
	    goto send_trailers;
	  }
	  buf += wrlen;
	  buflen -= wrlen;
	  this->sent += wrlen;
	}
	len = this->len;
	if ((len > BUF_SIZE) || (len < 0)) {
	  len = BUF_SIZE;
	}
      }
    }
  send_trailers:
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

 done:

  SF_DFPRINTF((stderr,
	      "sendfile: Done. Setting up callback\n"
	      "%d bytes sent\n", this->sent));

  mt_lock(&interpreter_lock);

  /* Neither of the following can be done in our current context
   * so we do them from a backend callback.
   * * Call the callback.
   * * Get rid of extra ref to the object, and free ourselves.
   */
  add_backend_callback(call_callback_and_free, this, 0);

  /* Call as soon as possible. */
  next_timeout.tv_usec = 0;
  next_timeout.tv_sec = 0;

  /* Wake up the backend */
  wake_up_backend();
    
  mt_unlock(&interpreter_lock);

  /* Die */
  return NULL;
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
    error("sendfile->create(): Called a second time!\n");
  }

  MEMSET(&sf, 0, sizeof(struct pike_sendfile));
  sf.callback.type = T_INT;

  get_all_args("sendfile", args, "%A%O%i%i%A%o%*",
	       &(sf.headers), &(sf.from_file), &(sf.offset),
	       &(sf.len), &(sf.trailers), &(sf.to_file), &cb);
  sf.callback = *cb;

  /* Fix the trailing args */
  push_array(sf.args = aggregate_array(args-7));
  args = 8;

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

  /* Note: No need for safe_apply() */
  sf.from_fd = -1;
  if (sf.from_file) {
    if (sf.len) {
      apply(sf.from_file, "query_fd", 0);
      if (sp[-1].type != T_INT) {
	pop_stack();
	bad_arg_error("sendfile", sp-args, args, 2, "object", sp+1-args,
		      "Bad argument 2 to sendfile(): "
		      "query_fd() returned non-integer.\n");
      }
      sf.from_fd = sp[-1].u.integer;
      pop_stack();

      /* Fix offset */
      if (sf.offset < 0) {
	if (sf.from_fd >= 0) {
	  sf.offset = tell(sf.from_fd);
	} else {
	  apply(sf.from_file, "tell", 0);
	  if (sp[-1].type != T_INT) {
	    pop_stack();
	    bad_arg_error("sendfile", sp-args, args, 2, "object", sp+1-args,
			  "Bad argument 2 to sendfile(): "
			  "tell() returned non-integer.\n");
	  }
	  sf.offset = sp[-1].u.integer;
	  pop_stack();
	}
      }
      if (sf.offset < 0) {
	sf.offset = 0;
      }
    } else {
      /* No need for the from_file object. */
      /* NOTE: We need to zap from_file from the stack too. */
      free_object(sf.from_file);
      sf.from_file = NULL;
      sp[1-args].type = T_INT;
      sp[1-args].u.integer = 0;
    }
  }
  apply(sf.to_file, "query_fd", 0);
  if (sp[-1].type != T_INT) {
    pop_stack();
    bad_arg_error("sendfile", sp-args, args, 6, "object", sp+5-args,
		  "Bad argument 2 to sendfile(): "
		  "query_fd() returned non-integer.\n");
  }
  sf.to_fd = sp[-1].u.integer;
  pop_stack();

  /* Set up the iovec's */
  if (iovcnt) {
    sf.iovs = (struct iovec *)xalloc(sizeof(struct iovec) * iovcnt);

    sf.hd_iov = sf.iovs;
    sf.tr_iov = sf.iovs + sf.hd_cnt;

    if (sf.headers) {
      int i;
      for (i = sf.hd_cnt; i--;) {
	struct pike_string *s;
	if ((s = sf.headers->item[i].u.string)->len) {
	  sf.hd_iov[i].iov_base = s->str;
	  sf.hd_iov[i].iov_len = s->len;
	} else {
	  sf.hd_iov++;
	  sf.hd_cnt--;
	}
      }
    }
    if (sf.trailers) {
      int i;
      for (i = sf.tr_cnt; i--;) {
	struct pike_string *s;
	if ((s = sf.trailers->item[i].u.string)->len) {
	  sf.tr_iov[i].iov_base = s->str;
	  sf.tr_iov[i].iov_len = s->len;
	} else {
	  sf.tr_iov++;
	  sf.tr_cnt--;
	}
      }
    }
  }

  /* We need to copy the arrays since the user might do destructive
   * operations on them, and we need the arrays to keep references to
   * the strings.
   */
  if ((sf.headers) && (sf.headers->refs > 1)) {
    struct array *a = copy_array(sf.headers);
    free_array(sf.headers);
    sf.headers = a;
  }
  if ((sf.trailers) && (sf.trailers->refs > 1)) {
    struct array *a = copy_array(sf.trailers);
    free_array(sf.trailers);
    sf.trailers = a;
  }

  if (sf.from_file) {
    /* We may need a buffer to hold the data */
    sf.buffer = (char *)xalloc(BUF_SIZE);
  }

  /*
   * Setup done. Note that we keep refs to all refcounted svalues in
   * our object.
   */
  sp -= args;
  *THIS = sf;
  args = 0;

  if (((sf.from_fd >= 0) || (!sf.from_file)) && (sf.to_fd >= 0)) {
    /* Threaded blocking mode possible */
    THREAD_T th_id;

    /* Make sure both file objects are in blocking mode.
     */
    if (THIS->from_file) {
      apply(THIS->from_file, "set_blocking", 0);
      pop_stack();
    }
    apply(THIS->to_file, "set_blocking", 0);
    pop_stack();

    /* Note: we hold a reference to ourselves.
     * The gc() won't find it, so be carefull.
     */
    add_ref(THIS->self = fp->current_object);

    /* The worker will have a ref. */
    th_create_small(&th_id, worker, THIS);
  } else {
    /* Nonblocking operation needed */

    /* Not supported */
    error("Unsupported operation\n");
  }
  return;
}

#endif /* _REENTRANT */

/*
 * Module init code
 */
void pike_module_init(void)
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

void pike_module_exit(void)
{
#ifdef _REENTRANT
  if (pike_sendfile_prog) {
    free_program(pike_sendfile_prog);
    pike_sendfile_prog = NULL;
  }
#endif /* _REENTRANT */
}
