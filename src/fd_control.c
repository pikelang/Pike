/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

#ifndef TESTING
#include "global.h"
#include "error.h"
#include "fdlib.h"
#else
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1
#include <sys/types.h>
#undef DEBUG
#define fd_ioctl ioctl
#endif

#include "fd_control.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_ERRNO_H
#include <sys/socket.h>
#endif

#ifdef HAVE_WINSOCK_H
#include <winsock.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

int set_nonblocking(int fd,int which)
{
#ifdef DEBUG
  if(fd<0 || fd >MAX_OPEN_FILEDESCRIPTORS)
    fatal("Filedescriptor out of range.\n");
#endif

#if defined(USE_IOCTL_FIONBIO) || defined(__NT__)
  return fd_ioctl(fd, FIONBIO, &which);
#else

#ifdef USE_FCNTL_O_NDELAY
  return fcntl(fd, F_SETFL, which?O_NDELAY:0);
#else

#ifdef USE_FCNTL_O_NONBLOCK
  return fcntl(fd, F_SETFL, which?O_NONBLOCK:0);
#else

#ifdef USE_FCNTL_FNDELAY
  return fcntl(fd, F_SETFL, which?FNDELAY:0);
#else

#error Do not know how to set your filedescriptors nonblocking.

#endif
#endif
#endif
#endif
}

int query_nonblocking(int fd)
{
#ifdef DEBUG
  if(fd<0 || fd > MAX_OPEN_FILEDESCRIPTORS)
    fatal("Filedescriptor out of range.\n");
#endif

#ifdef USE_FCNTL_O_NDELAY
  return fcntl(fd, F_GETFL, 0) & O_NDELAY;
#else

#ifdef USE_FCNTL_O_NONBLOCK
  return fcntl(fd, F_GETFL, 0) & O_NONBLOCK;
#else

#ifdef USE_FCNTL_FNDELAY
  return fcntl(fd, F_GETFL, 0) & FNDELAY;
#else
  return 0;
#endif
#endif
#endif
}

int set_close_on_exec(int fd, int which)
{
#ifdef F_SETFD
  return fcntl(fd, F_SETFD, !!which);
#else
  return 0;
#endif
}

#ifdef TESTING


#if defined(HAVE_WINSOCK_H) && defined(USE_IOCTLSOCKET_FIONBIO)
int main()
{
  exit(0);
}
#else

#include <signal.h>
#include <stdio.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
/* a part of the autoconf thingy */

RETSIGTYPE sigalrm_handler0(int tmp) { exit(0); }
RETSIGTYPE sigalrm_handler1(int tmp)
{
  fprintf(stderr,"Failed in alarm handler 1\n");
  exit(1);
}
#ifdef HAVE_SOCKETPAIR
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_WINSOCK_H
#include <winsock.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#endif /* HAVE_SOCKETPAIR */

int main()
{
  int tmp[2];
  char foo[1000];

  tmp[0]=0;
  tmp[1]=0;
#ifdef HAVE_SOCKETPAIR
  if(socketpair(AF_UNIX, SOCK_STREAM, 0, tmp) < 0)
    tmp[0]=tmp[1]=0;
#endif

  set_nonblocking(tmp[0],1);
  signal(SIGALRM, sigalrm_handler1);
  alarm(1);
  read(tmp[0],foo,999);
  set_nonblocking(tmp[0],0);
  signal(SIGALRM, sigalrm_handler0);
  alarm(1);
  read(tmp[0],foo,999);
  fprintf(stderr,"Failed at end of main.\n");
  exit(1);
}
#endif
#endif
