/*
 * $Id: fdlib.h,v 1.16 1998/05/22 11:29:15 grubba Exp $
 */
#ifndef FDLIB_H
#define FDLIB_H

#include "global.h"
#include "pike_macros.h"

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

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#ifdef HAVE_SOCKET_H
#include <socket.h>
#endif /* HAVE_SOCKET_H */


#define fd_INTERPROCESSABLE   1
#define fd_CAN_NONBLOCK       2
#define fd_CAN_SHUTDOWN       4
#define fd_BUFFERED           8
#define fd_BIDIRECTIONAL     16


#ifdef HAVE_WINSOCK_H

#define HAVE_FD_FLOCK


#define FILE_CAPABILITIES (fd_INTERPROCESSABLE)
#define PIPE_CAPABILITIES (fd_INTERPROCESSABLE | fd_BUFFERED)
#define SOCKET_CAPABILITIES (fd_BIDIRECTIONAL | fd_CAN_NONBLOCK | fd_CAN_SHUTDOWN)

#ifndef FD_SETSIZE
#define FD_SETSIZE MAX_OPEN_FILEDESCRIPTORS
#endif

#include <winsock.h>
#include <winbase.h>

typedef int FD;

#define SOCKFUN1(NAME,T1) int PIKE_CONCAT(fd_,NAME) (FD,T1);
#define SOCKFUN2(NAME,T1,T2) int PIKE_CONCAT(fd_,NAME) (FD,T1,T2);
#define SOCKFUN3(NAME,T1,T2,T3) int PIKE_CONCAT(fd_,NAME) (FD,T1,T2,T3);
#define SOCKFUN4(NAME,T1,T2,T3,T4) int PIKE_CONCAT(fd_,NAME) (FD,T1,T2,T3,T4);
#define SOCKFUN5(NAME,T1,T2,T3,T4,T5) int PIKE_CONCAT(fd_,NAME) (FD,T1,T2,T3,T4,T5);

/* Prototypes begin here */
char *fd_info(int fd);
int fd_query_properties(int fd, int guess);
void fd_init();
void fd_exit();
FD fd_open(char *file, int open_mode, int create_mode);
FD fd_socket(int domain, int type, int proto);
int fd_pipe(int fds[2]);
FD fd_accept(FD fd, struct sockaddr *addr, int *addrlen);
SOCKFUN2(bind, struct sockaddr *, int)
int fd_connect (FD fd, struct sockaddr *a, int len);
SOCKFUN4(getsockopt,int,int,void*,int*)
SOCKFUN4(setsockopt,int,int,void*,int)
SOCKFUN2(getsockname,struct sockaddr *,int *)
SOCKFUN2(getpeername,struct sockaddr *,int *)
SOCKFUN3(recv,void *,int,int)
SOCKFUN5(recvfrom,void *,int,int,struct sockaddr *,int*)
SOCKFUN3(send,void *,int,int)
SOCKFUN5(sendto,void *,int,int,struct sockaddr *,int*)
SOCKFUN1(shutdown, int)
SOCKFUN1(listen, int)
int fd_close(FD fd);
long fd_write(FD fd, void *buf, long len);
long fd_read(FD fd, void *to, long len);
long fd_lseek(FD fd, long pos, int where);
int fd_flock(FD fd, int oper);
int fd_fstat(FD fd, struct stat *s);
int fd_select(int fds, FD_SET *a, FD_SET *b, FD_SET *c, struct timeval *t);
int fd_ioctl(FD fd, int cmd, void *data);
FD fd_dup(FD from);
FD fd_dup2(FD from, FD to);
struct fd_mapper;
void init_fd_mapper(struct fd_mapper *x);
void exit_fd_mapper(struct fd_mapper *x);
void fd_mapper_set(struct fd_mapper *x, FD fd, void *data);
void *fd_mapper_get(struct fd_mapper *x, FD fd);
struct fd_mapper_data;
struct fd_mapper;
void init_fd_mapper(struct fd_mapper *x);
void exit_fd_mapper(struct fd_mapper *x);
void fd_mapper_set(struct fd_mapper *x, FD fd, void *data);
void *fd_mapper_get(struct fd_mapper *x, FD fd);
struct fd_data_hash;
struct fd_data_hash_block;
int get_fd_data_key(void);
void store_fd_data(FD fd, int key, void *data);
void *get_fd_data(FD fd, int key);
struct event;
struct fd_waitor;
void fd_waitor_set_customer(struct fd_waitor *x, FD customer, int flags);
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

#define FD_PIPE -5
#define FD_SOCKET -4
#define FD_CONSOLE -3
#define FD_FILE -2
#define FD_NO_MORE_FREE -1


#define fd_LOCK_SH 1
#define fd_LOCK_EX 2
#define fd_LOCK_UN 4
#define fd_LOCK_NB 8

struct my_fd_set_s
{
  char bits[MAX_OPEN_FILEDESCRIPTORS/8];
};

typedef struct my_fd_set_s my_fd_set;

#ifdef DEBUG
#define fd_check_fd(X) do { if(fd_type[X]>=0) fatal("FD_SET on closed fd %d (%d) %s:%d.\n",X,da_handle[X],__FILE__,__LINE__); }while(0)
#else
#define fd_check_fd(X)
#endif
#define my_FD_CLR(FD,S) ((S)->bits[(FD)>>3]&=~ (1<<(FD&7)))
#define my_FD_SET(FD,S) do{ fd_check_fd(FD); ((S)->bits[(FD)>>3]|= (1<<(FD&7))); }while(0)
#define my_FD_ISSET(FD,S) ((S)->bits[(FD)>>3]&(1<<(FD&7)))
#define my_FD_ZERO(S) MEMSET(& (S)->bits, 0, sizeof(my_fd_set))

#define fd_copy_my_fd_set_to_fd_set(TO,FROM,max) do {			\
   int e_,d_,max_=MINIMUM(MAX_OPEN_FILEDESCRIPTORS>>3,(max+7)>>3);	\
   (TO)->fd_count=0;							\
   for(e_=0;e_<max_;e_++)						\
   {									\
     int b_=(FROM)->bits[e_];						\
     if(b_)								\
     {									\
       for(d_=0;d_<8;d_++)						\
       {								\
         if(b_ & (1<<d_))						\
         {								\
           int fd_=(e_<<3)+d_;						\
           fd_check_fd(fd_);						\
           (TO)->fd_array[(TO)->fd_count++]=(SOCKET)da_handle[fd_];	\
         }								\
       }								\
     }									\
   }									\
}while(0)

extern long da_handle[MAX_OPEN_FILEDESCRIPTORS];
extern int fd_type[MAX_OPEN_FILEDESCRIPTORS];

#define fd_FD_CLR(X,Y) FD_CLR((SOCKET)da_handle[X],Y)
#define fd_FD_SET(X,Y) \
 do { fd_check_fd(X); FD_SET((SOCKET)da_handle[X],Y); }while(0)
#define fd_FD_ISSET(X,Y) FD_ISSET((SOCKET)da_handle[X],Y)
#define fd_FD_ZERO(X) FD_ZERO(X)

#ifndef S_IFSOCK
#define S_IFSOCK 0140000
#endif

#else // HAVE_WINSOCK


typedef int FD;

#define fd_info(X) ""
#define fd_init()
#define fd_exit()

#define fd_RDONLY O_RDONLY
#define fd_WRONLY O_WRONLY
#define fd_RDWR O_RDWR
#define fd_APPEND O_APPEND
#define fd_CREAT O_CREAT
#define fd_TRUNC O_TRUNC
#define fd_EXCL O_EXCL

#define fd_query_properties(X,Y) ( fd_INTERPROCESSABLE | (Y))
#define fd_open open
#define fd_close close
#define fd_read read
#define fd_write write
#define fd_ioctl ioctl
#define fd_pipe pipe

#define fd_socket socket
#define fd_bind bind
#define fd_connect connect
#define fd_getsockopt getsockopt
#define fd_setsockopt setsockopt
#define fd_getsockname getsockname
#define fd_getpeername getpeername
#define fd_recv recv
#define fd_send send
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
#define fd_socketpair socketpair

#define fd_fd_set fd_set
#define fd_FD_CLR FD_CLR
#define fd_FD_SET FD_SET
#define fd_FD_ISSET FD_ISSET
#define fd_FD_ZERO FD_ZERO

#ifdef HAVE_FLOCK
#define HAVE_FD_FLOCK
#define fd_flock flock
#define fd_LOCK_SH LOCK_SH
#define fd_LOCK_EX LOCK_EX
#define fd_LOCK_UN LOCK_UN
#define fd_LOCK_NB LOCK_NB
#else
#ifdef HAVE_LOCKF
#define HAVE_FD_LOCKF
#define fd_LOCK_EX F_LOCK
#define fd_LOCK_UN F_ULOCK
#define fd_LOCK_NB F_TLOCK
#define fd_lockf(fd,mode) lockf(fd,mode,0)
#endif
#endif




#define fd_shutdown_read 0
#define fd_shutdown_write 1
#define fd_shutdown_both 2

struct my_fd_set_s
{
  fd_set tmp;
};

typedef struct my_fd_set_s my_fd_set;

#define my_FD_CLR(FD,S) FD_CLR((FD), & (S)->tmp)
#define my_FD_SET(FD,S) FD_SET((FD), & (S)->tmp)
#define my_FD_ISSET(FD,S) FD_ISSET((FD), & (S)->tmp)
#define my_FD_ZERO(S) FD_ZERO(& (S)->tmp)

#define fd_copy_my_fd_set_to_fd_set(TO,FROM,max) \
   MEMCPY((TO),&(FROM)->tmp,sizeof(*(TO)))

#define FILE_CAPABILITIES (fd_INTERPROCESSABLE | fd_CAN_NONBLOCK)
#define PIPE_CAPABILITIES (fd_INTERPROCESSABLE | fd_BUFFERED | fd_CAN_NONBLOCK)
#define UNIX_SOCKET_CAPABILITIES (fd_INTERPROCESSABLE | fd_BIDIRECTIONAL | fd_CAN_NONBLOCK)
#define SOCKET_CAPABILITIES (fd_INTERPROCESSABLE | fd_BIDIRECTIONAL | fd_CAN_NONBLOCK | fd_CAN_SHUTDOWN)

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

#endif // Don't HAVE_WINSOCK

#endif // FDLIB_H
