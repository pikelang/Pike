#ifndef FDLIB_H
#define FDLIB_H

#include "global.h"

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_FCNTL_H
#include <errno.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_WINSOCK2_H


#ifndef FD_SETSIZE
#define FD_SETSIZE MAX_OPEN_FILEDESCRIPTORS
#endif

#include <winsock2.h>
#include <winbase.h>

typedef int FD;

#define SOCKFUN1(NAME,T1) int PIKE_CONCAT(fd_,NAME) (FD,T1);
#define SOCKFUN2(NAME,T1,T2) int PIKE_CONCAT(fd_,NAME) (FD,T1,T2);
#define SOCKFUN3(NAME,T1,T2,T3) int PIKE_CONCAT(fd_,NAME) (FD,T1,T2,T3);
#define SOCKFUN4(NAME,T1,T2,T3,T4) int PIKE_CONCAT(fd_,NAME) (FD,T1,T2,T3,T4);
#define SOCKFUN5(NAME,T1,T2,T3,T4,T5) int PIKE_CONCAT(fd_,NAME) (FD,T1,T2,T3,T4,T5);

/* Prototypes begin here */
void fd_init();
void fd_exit();
FD fd_open(char *file, int open_mode, int create_mode);
FD fd_socket(int domain, int type, int proto);
FD fd_accept(FD fd, struct sockaddr *addr, int *addrlen);
SOCKFUN2(bind, struct sockaddr *, int)
SOCKFUN2(connect, struct sockaddr *, int)
SOCKFUN4(getsockopt,int,int,void*,int*)
SOCKFUN4(setsockopt,int,int,void*,int)
SOCKFUN2(getsockname,struct sockaddr *,int*)
SOCKFUN2(getpeername,struct sockaddr *,int*)
SOCKFUN3(recv,void *,int,int)
SOCKFUN5(recvfrom,void *,int,int,struct sockaddr *,int*)
SOCKFUN5(sendto,void *,int,int,struct sockaddr *,int*)
SOCKFUN1(shutdown, int)
SOCKFUN1(listen, int)
int fd_close(FD fd);
long fd_write(FD fd, void *buf, long len);
long fd_read(FD fd, void *to, long len);
long fd_lseek(FD fd, long pos, int where);
int fd_fstat(FD fd, struct stat *s);
int fd_select(int fds, FD_SET *a, FD_SET *b, FD_SET *c, struct timeval *t);
int fd_ioctl(FD fd, int cmd, void *data);
FD fd_dup(FD from);
FD fd_dup2(FD from, FD to);
/* Prototypes end here */

#undef SOCKFUN1
#undef SOCKFUN2
#undef SOCKFUN3
#undef SOCKFUN4
#undef SOCKFUN5

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#ifndef EADDRINUSE
#define EADDRINUSE WSAEADDRINUSE
#endif

#ifndef ENOTSUPP
#define ENOTSUPP WSAEOPNOTSUPP
#endif

#define fd_RDONLY 1
#define fd_WRONLY 2
#define fd_RDWR 3
#define fd_APPEND 4
#define fd_CREAT 8
#define fd_TRUNC 16
#define fd_EXCL 32

#define fd_shutdown_read SD_RECEIVE
#define fd_shutdown_write SD_SEND
#define fd_shutdown_both SD_BOTH

extern long da_handle[MAX_OPEN_FILEDESCRIPTORS];

#define fd_FD_CLR(X,Y) FD_CLR((SOCKET)da_handle[X],Y)
#define fd_FD_SET(X,Y) FD_SET((SOCKET)da_handle[X],Y)
#define fd_FD_ISSET(X,Y) FD_ISSET((SOCKET)da_handle[X],Y)
#define fd_FD_ZERO(X) FD_ZERO(X)

#ifndef S_IFSOCK
#define S_IFSOCK 0140000
#endif

#else


typedef int FD;

#define fd_init()
#define fd_exit()

#define fd_RDONLY O_RDONLY
#define fd_WRONLY O_WRONLY
#define fd_RDWR O_RDWR
#define fd_APPEND O_APPEND
#define fd_CREAT O_CREAT
#define fd_TRUNC O_TRUNC
#define fd_EXCL O_EXCL

#define fd_open open
#define fd_close close
#define fd_read read
#define fd_write write
#define fd_ioctl ioctl

#define fd_socket socket
#define fd_bind bind
#define fd_connect connect
#define fd_getsockopt getsockopt
#define fd_setsockopt setsockopt
#define fd_getsockname getsockname
#define fd_getpeername getpeername
#define fd_recv recv
#define fd_sendto sendto
#define fd_recvfrom recvfrom
#define fd_shutdown shutdown
#define fd_accept accept
#define fd_lseek lseek
#define fd_fstat fstat
#define fd_dup dup
#define fd_dup2 dup2
#define fd_listen listen

#define fd_select select
#define fd_FD_CLR FD_CLR
#define fd_FD_SET FD_SET
#define fd_FD_ISSET FD_ISSET
#define fd_FD_ZERO FD_ZERO

#define fd_shutdown_read 0
#define fd_shutdown_write 1
#define fd_shutdown_both 2

#endif

#endif
