/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef TESTING
#include "global.h"
#include "error.h"
#else
#ifndef _LARGEFILE_SOURCE
#  define _FILE_OFFSET_BITS 64
#  define _LARGEFILE_SOURCE 1
#  define _LARGEFILE64_SOURCE 1
#endif /* !_LARGERFILE_SOURCE */
/* HPUX needs these too... */
#ifndef __STDC_EXT__
#  define __STDC_EXT__
#endif /* !__STDC_EXT__ */
#undef DEBUG
#endif

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "fd_control.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

void set_nonblocking(int fd,int which)
{
#ifdef DEBUG
  if(fd<0 || fd >MAX_OPEN_FILEDESCRIPTORS)
    fatal("Filedescriptor out of range.\n");
#endif

#ifdef USE_IOCTL_FIONBIO
  ioctl(fd, FIONBIO, &which);
#else

#ifdef USE_FCNTL_O_NDELAY
  fcntl(fd, F_SETFL, which?O_NDELAY:0);
#else

#ifdef USE_FCNTL_O_NONBLOCK
  fcntl(fd, F_SETFL, which?O_NONBLOCK:0);
#else

#ifdef USE_FCNTL_FNDELAY
  fcntl(fd, F_SETFL, which?FNDELAY:0);
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

#ifdef USE_IOCTL_FIONBIO
  /* I don't know how to query this!!!! */
  return 0;
#else

#ifdef USE_FCNTL_O_NDELAY
  return fcntl(fd, F_GETFL, 0) & O_NDELAY;
#else

#ifdef USE_FCNTL_O_NONBLOCK
  return fcntl(fd, F_GETFL, 0) & O_NONBLOCK;
#else

#ifdef USE_FCNTL_FNDELAY
  return fcntl(fd, F_GETFL, 0) & FNDELAY;
#else
#error Do not know how to set your filedescriptors nonblocking.
#endif
#endif
#endif
#endif
}

int set_close_on_exec(int fd, int which)
{
  return fcntl(fd, F_SETFD, !!which);
}

#ifdef TESTING

#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
/* a part of the autoconf thingy */

RETSIGTYPE sigalrm_handler0(int tmp) { exit(0); }
RETSIGTYPE sigalrm_handler1(int tmp) { exit(1); }

main()
{
  int tmp[2];
  char foo[1000];

  tmp[0]=0;
  tmp[1]=0;
#ifdef HAVE_PIPE
  pipe(tmp);
#endif
  
  set_nonblocking(tmp[0],1);
  signal(SIGALRM, sigalrm_handler1);
  alarm(1);
  read(tmp[0],foo,999);
  set_nonblocking(tmp[0],0);
  signal(SIGALRM, sigalrm_handler0);
  alarm(1);
  read(tmp[0],foo,999);
  exit(1);
}
#endif
