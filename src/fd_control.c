/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/

#ifndef TESTING
#include "global.h"
#include "error.h"
#include "fdlib.h"

RCSID("$Id: fd_control.c,v 1.27 1999/09/14 21:07:20 hubbe Exp $");

#else /* TESTING */

#ifndef _LARGEFILE_SOURCE
#  define _FILE_OFFSET_BITS 64
#  define _LARGEFILE_SOURCE 1
#  define _LARGEFILE64_SOURCE 1
#endif /* !_LARGERFILE_SOURCE */
/* HPUX needs these too... */
#ifndef __STDC_EXT__
#  define __STDC_EXT__
#endif /* !__STDC_EXT__ */
#include <sys/types.h>
#undef PIKE_DEBUG
#define fd_ioctl ioctl

#endif /* TESTING */

#include "fd_control.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <errno.h>

#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
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
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif


int set_nonblocking(int fd,int which)
{
  int ret;
#ifdef PIKE_DEBUG
  if(fd<0 || fd >MAX_OPEN_FILEDESCRIPTORS)
    fatal("Filedescriptor %d out of range [0,%d).\n",
	  fd, MAX_OPEN_FILEDESCRIPTORS);
#endif

  do 
  {
#if defined(USE_IOCTL_FIONBIO) || defined(__NT__)
    ret=fd_ioctl(fd, FIONBIO, &which);
#else

#ifdef USE_FCNTL_O_NDELAY
    ret=fcntl(fd, F_SETFL, which?O_NDELAY:0);
#else

#ifdef USE_FCNTL_O_NONBLOCK
    ret=fcntl(fd, F_SETFL, which?O_NONBLOCK:0);
#else

#ifdef USE_FCNTL_FNDELAY
    ret=fcntl(fd, F_SETFL, which?FNDELAY:0);
#else

#error Do not know how to set your filedescriptors nonblocking.

#endif
#endif
#endif
#endif
  } while(ret <0 && errno==EINTR);
  return ret;
}

int query_nonblocking(int fd)
{
  int ret;
#ifdef PIKE_DEBUG
  if(fd<0 || fd > MAX_OPEN_FILEDESCRIPTORS)
    fatal("Filedescriptor out of range.\n");
#endif

  do 
  {
#ifdef USE_FCNTL_O_NDELAY
    ret=fcntl(fd, F_GETFL, 0) & O_NDELAY;
#else

#ifdef USE_FCNTL_O_NONBLOCK
    ret=fcntl(fd, F_GETFL, 0) & O_NONBLOCK;
#else

#ifdef USE_FCNTL_FNDELAY
    ret=fcntl(fd, F_GETFL, 0) & FNDELAY;
#else
  return 0;
#endif
#endif
#endif
  } while(ret <0 && errno==EINTR);
  return ret;
}

#ifndef FD_CLOEXEC
#define FD_CLOEXEC 1
#endif /* FD_CLOEXEC */

#ifdef HAVE_BROKEN_F_SETFD
static int fds_to_close[MAX_OPEN_FILEDESCRIPTORS];
static int num_fds_to_close = 0;

void do_close_on_exec(void)
{
  int i,ret;
  for(i=0; i < num_fds_to_close; i++) {
    while( close(fds_to_close[i]) <0 && errno==EINTR) ;
  }
  num_fds_to_close = 0;
}
#endif /* HAVE_BROKEN_F_SETFD */

int set_close_on_exec(int fd, int which)
{
#ifndef HAVE_BROKEN_F_SETFD
  int ret;
  do 
  {
    ret=fcntl(fd, F_SETFD, which ? FD_CLOEXEC : 0);
  } while (ret <0 && errno==EINTR );
  return ret;
#else /* HAVE_BROKEN_F_SETFD */
  int i;
  if (which) {
    for(i = 0; i < num_fds_to_close; i++) {
      if (fds_to_close[i] == fd) {
	return(0);	/* Already marked */
      }
    }
    fds_to_close[num_fds_to_close++] = fd;
    return(0);
  } else {
    for(i = 0; i < num_fds_to_close; i++) {
      while (fds_to_close[i] == fd && (i < num_fds_to_close)) {
	fds_to_close[i] = fds_to_close[--num_fds_to_close];
      }
    }
    return(0);
  }
  return 0;
#endif /* !HAVE_BROKEN_F_SETFD */
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
