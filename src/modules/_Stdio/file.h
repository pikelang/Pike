/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
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

#ifdef HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
#   define dirent direct
#   define NAMLEN(dirent) (dirent)->d_namlen
# else /* !HAVE_SYS_NDIR_H */
#  ifdef HAVE_SYS_DIR_H
#   include <sys/dir.h>
#   define dirent direct
#   define NAMLEN(dirent) (dirent)->d_namlen
#  else /* !HAVE_SYS_DIR_H */
#   ifdef HAVE_NDIR_H
#    include <ndir.h>
#    define dirent direct
#    define NAMLEN(dirent) (dirent)->d_namlen
#   else /* !HAVE_NDIR_H */
#    ifdef HAVE_DIRECT_H
#     include <direct.h>
#     define NAMLEN(dirent) strlen((dirent)->d_name)
#    endif /* HAVE_DIRECT_H */
#   endif /* HAVE_NDIR_H */
#  endif /* HAVE_SYS_DIR_H */
# endif /* HAVE_SYS_NDIR_H */
#endif /* HAVE_DIRENT_H */

/* Stupid patch to avoid trouble with Linux includes... */
#ifdef LITTLE_ENDIAN
#undef LITTLE_ENDIAN
#endif

#endif
#endif

#include "pike_netlib.h"
#include "pike_threadlib.h"
#include "backend.h"

#if defined(HAVE_IPPROTO_IPv6) && !defined(IPPROTO_IPV6)
// Hidden in an enum.
#define IPPROTO_IPV6 IPPROTO_IPV6
#endif

struct my_file
{
  struct fd_callback_box box;	/* Must be first. */
  /* The box is hooked in whenever box.backend is set. */

  struct svalue event_cbs[6];
  /* Callbacks can be set without having the corresponding bits in
   * box.events, but not the other way around. */

  short open_mode;
  short flags;
  INT_TYPE my_errno;

#ifdef HAVE_PIKE_SEND_FD
  int *fd_info;
  /* Info about fds pending to be sent.
   *   If non-NULL the first element is the array size,
   *   and the second is the number of fds pending to be
   *   sent. Elements three and onwards are fds to send.
   *
   *   Note that to avoid races between the call to
   *   send_fd() and the call to write(), these fds
   *   are dup(2)'ed in send_fd() and close(2)'ed after
   *   sending in write() (or close() or _destruct()).
   */
#endif

#if defined(HAVE_FD_FLOCK) || defined(HAVE_FD_LOCKF)
  struct object *key;
#endif
#ifdef _REENTRANT
  /* List of pike threads that need to be woken up on concurrent close(2). */
  struct thread_state *busy_threads;
#endif
};

#ifdef _REENTRANT

struct pike_sendfile
{
  struct object *self;

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

  INT64 sent;

  INT64 offset;
  INT64 len;

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

extern int fd_write_identifier_offset;

/* Note: Implemented in ../system/system.c! */
extern int get_inet_addr(PIKE_SOCKADDR *addr,char *name,char *service,
			 INT_TYPE port, int inet_flags);
#define PIKE_INET_FLAG_UDP	1
#define PIKE_INET_FLAG_IPV6	2
#define PIKE_INET_FLAG_NB	4

#ifdef _REENTRANT
void low_do_sendfile(struct pike_sendfile *);
#endif /* _REENTRANT */

/* Prototypes begin here */
void my_set_close_on_exec(int fd, int to);
void do_set_close_on_exec(void);

void init_stdio_efuns(void);
void exit_stdio_efuns(void);

void init_stdio_stat(void);
void exit_stdio_stat(void);

void init_stdio_port(void);
void exit_stdio_port(void);

void init_stdio_sendfile(void);
void exit_stdio_sendfile(void);

void init_stdio_udp(void);
void exit_stdio_udp(void);

PMOD_EXPORT struct object *file_make_object_from_fd(int fd, int mode, int guess);
PMOD_EXPORT void push_new_fd_object(int factory_fun_num,
				    int fd, int mode, int guess);
int my_socketpair(int family, int type, int protocol, int sv[2]);
int socketpair_ultra(int family, int type, int protocol, int sv[2]);
struct new_thread_data;
void file_proxy(INT32 args);
PMOD_EXPORT void create_proxy_pipe(struct object *o, int for_reading);
struct file_lock_key_storage;
void mark_ids(struct callback *foo, void *bar, void *gazonk);
PMOD_EXPORT int pike_make_pipe(int *fds);
PMOD_EXPORT int fd_from_object(struct object *o);
void push_stat(PIKE_STAT_T *s);

#ifndef __NT__
void low_get_dir(DIR *dir, ptrdiff_t name_max);
#endif
/* Prototypes end here */

/* Defined by winnt.h */
#ifdef FILE_CREATE
#undef FILE_CREATE
#endif

/* open_mode
 *
 * Note: The lowest 8 bits are reserved for the fd_* (aka PROP_*)
 *       flags from "fdlib.h".
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
#define FILE_HAVE_RECV_FD	0x0020

#endif
