/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
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

struct my_file
{
  INT32 refs;
  short open_mode;
  struct svalue id;
  struct svalue read_callback;
  struct svalue write_callback;
#ifdef WITH_OOB
  struct svalue read_oob_callback;
  struct svalue write_oob_callback;
#endif /* WITH_OOB */
  struct svalue close_callback;
#ifdef HAVE_FD_FLOCK
  struct object *key;
#endif
};

extern void get_inet_addr(struct sockaddr_in *addr,char *name);

/* Prototypes begin here */
struct file_struct;
void my_set_close_on_exec(int fd, int to);
void do_set_close_on_exec(void);
struct object *file_make_object_from_fd(int fd, int mode, int guess);
int my_socketpair(int family, int type, int protocol, int sv[2]);
int socketpair(int family, int type, int protocol, int sv[2]);
void pike_module_exit(void);
void mark_ids(struct callback *foo, void *bar, void *gazonk);
void pike_module_init(void);
int pike_make_pipe(int *fds);
/* Prototypes end here */

#define FILE_READ               0x1000
#define FILE_WRITE              0x2000
#define FILE_APPEND             0x4000
#define FILE_CREATE             0x8000
#define FILE_TRUNC              0x0100
#define FILE_EXCLUSIVE          0x0200
#define FILE_NONBLOCKING        0x0400
#define FILE_SET_CLOSE_ON_EXEC  0x0800


#endif
