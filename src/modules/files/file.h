/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: file.h,v 1.37 2004/11/09 17:07:34 nilsson Exp $
*/

#ifndef FILE_H
#define FILE_H

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
#ifndef ARPA_INET_H
#include <arpa/inet.h>
#define ARPA_INET_H

/* Stupid patch to avoid trouble with Linux includes... */
#ifdef LITTLE_ENDIAN
#undef LITTLE_ENDIAN
#endif

#endif
#endif

#include "pike_netlib.h"
#include "backend.h"

struct my_file
{
  struct fd_callback_box box;	/* Must be first. */
  /* The box is hooked in whenever box.backend is set. */

  struct svalue event_cbs[4];
  /* Callbacks can be set without having the corresponding bits in
   * box.events, but not the other way around. */

  short open_mode;
  short flags;
  int my_errno;

#if defined(HAVE_FD_FLOCK) || defined(HAVE_FD_LOCKF)
  struct object *key;
#endif
};

#ifdef _REENTRANT

#ifndef HAVE_STRUCT_IOVEC
struct iovec {
  void *iov_base;
  size_t iov_len;
};
#endif /* !HAVE_STRUCT_IOVEC */


struct pike_sendfile
{
  struct object *self;

  LONGEST sent;

  struct array *headers;
  struct array *trailers;

  struct object *from_file;
  struct object *to_file;

  struct callback *backend_callback;
  struct svalue callback;
  struct array *args;

  int from_fd;
  int to_fd;

  struct my_file *from;
  struct my_file *to;

  LONGEST offset;
  LONGEST len;

  struct iovec *hd_iov;
  struct iovec *tr_iov;

  int hd_cnt;
  int tr_cnt;

  struct iovec *iovs;
  char *buffer;
  ptrdiff_t buf_size;
};

#endif /* _REENTRANT */

extern struct program *file_program;
extern struct program *file_ref_program;

extern int get_inet_addr(PIKE_SOCKADDR *addr,char *name,char *service, INT_TYPE port, int udp);

#ifdef _REENTRANT
void low_do_sendfile(struct pike_sendfile *);
#endif /* _REENTRANT */

/* Prototypes begin here */
void my_set_close_on_exec(int fd, int to);
void do_set_close_on_exec(void);

#define CBFUNCS(X) \
static void PIKE_CONCAT(file_set_,X) (INT32 args); \
static void PIKE_CONCAT(file_query_,X) (INT32 args);
CBFUNCS(read_callback)
CBFUNCS(write_callback)
CBFUNCS(read_oob_callback)
CBFUNCS(write_oob_callback)

static void file_write(INT32 args);
struct object *file_make_object_from_fd(int fd, int mode, int guess);
int my_socketpair(int family, int type, int protocol, int sv[2]);
int socketpair_ultra(int family, int type, int protocol, int sv[2]);
struct new_thread_data;
void file_proxy(INT32 args);
void create_proxy_pipe(struct object *o, int for_reading);
struct file_lock_key_storage;
void mark_ids(struct callback *foo, void *bar, void *gazonk);
int pike_make_pipe(int *fds);
int fd_from_object(struct object *o);
void f_strerror(INT32 args);
void push_stat(PIKE_STAT_T *s);
/* Prototypes end here */

/* Defined by winnt.h */
#ifdef FILE_CREATE
#undef FILE_CREATE
#endif

/* open_mode
 *
 * Note: The lowest 8 bits are reserved for the fd_* flags from "fdlib.h".
 */
#define FILE_READ               0x1000
#define FILE_WRITE              0x2000
#define FILE_APPEND             0x4000
#define FILE_CREATE             0x8000

#define FILE_TRUNC              0x0100
#define FILE_EXCLUSIVE          0x0200
#define FILE_NONBLOCKING        0x0400
#define FILE_SET_CLOSE_ON_EXEC  0x0800

/* flags */
#define FILE_NO_CLOSE_ON_DESTRUCT 0x0002
#define FILE_LOCK_FD		0x0004
#define FILE_NOT_OPENED         0x0010

#endif
