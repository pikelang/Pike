/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef TESTING
#include "global.h"
#include "pike_error.h"
#include "fdlib.h"

#else /* TESTING */

#define PMOD_EXPORT

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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <errno.h>

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#elif defined(HAVE_WINSOCK_H)
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

PMOD_EXPORT int set_nonblocking(int fd,int which)
{
  int ret;
#ifdef PIKE_DEBUG
  if(fd<0)
    Pike_fatal("File descriptor %d out of range.\n", fd);
#endif

  do
  {
#if defined(USE_IOCTL_FIONBIO) || defined(__NT__) || defined(__amigaos__)
    ret=fd_ioctl(fd, FIONBIO, &which);
#else

#ifdef USE_FCNTL_O_NDELAY
#define FCNTL_NBFLAG	O_NDELAY
#elif defined(USE_FCNTL_O_NONBLOCK)
#define FCNTL_NBFLAG	O_NONBLOCK
#elif defined(USE_FCNTL_FNDELAY)
#define FCNTL_NBFLAG	FNDELAY
#endif

#ifdef FCNTL_NBFLAG
    int flags = fcntl(fd, F_GETFL, 0);

    if (which) {
      flags |= FCNTL_NBFLAG;
    } else {
      flags &= ~FCNTL_NBFLAG;
    }
    ret = fcntl(fd, F_SETFL, flags);
#elif !defined(DISABLE_BINARY)
#error Do not know how to set your filedescriptors nonblocking.
#endif

#endif
  } while(ret <0 && errno==EINTR);
  return ret;
}

PMOD_EXPORT int query_nonblocking(int fd)
{
  int ret = 0;
#ifdef PIKE_DEBUG
  if(fd<0)
    Pike_fatal("Filedescriptor out of range.\n");
#endif

#ifdef FCNTL_NBFLAG
  do {
    ret = fcntl(fd, F_GETFL, 0);
  } while(ret <0 && errno==EINTR);
  return ret & FCNTL_NBFLAG;
#else
  return ret;
#endif
}

#if !defined(SOL_TCP) && defined(IPPROTO_TCP)
    /* SOL_TCP isn't defined in Solaris. */
#define SOL_TCP IPPROTO_TCP
#endif

#ifdef SOL_TCP
#ifdef TCP_CORK
static int gsockopt(int fd, int opt) {
  int val = 0;
  socklen_t old_len = (socklen_t) sizeof(int);
  while (getsockopt(fd, SOL_TCP, opt,
                    &val, &old_len) < 0 && errno == EINTR);
  return val;
}

static void ssockopt(int fd, int opt, int val) {
  while (setsockopt(fd, SOL_TCP, opt,
                    &val, sizeof(val)) < 0 && errno == EINTR);
}
#endif
#endif

PMOD_EXPORT int bulkmode_start(int fd) {
  int ret = 0;
  (void)fd;
  // FIXME Cache NODELAY/CORK state to avoid getsockopt() system calls
#ifdef SOL_TCP
#ifdef TCP_CORK
  if (!gsockopt(fd, TCP_CORK)) {
    ssockopt(fd, TCP_CORK, 1);		// Turn on cork mode.
    ret |= 2;
  }
#endif
#endif /* SOL_TCP */
  return ret;
}

PMOD_EXPORT void bulkmode_restore(int fd, int which) {
  (void)fd;
  (void)which;
#ifdef SOL_TCP
#ifdef TCP_CORK
  ssockopt(fd, TCP_CORK, 0);	 // Briefly unplug always, to flush data
  if (!(which & 2))		// Only replug cork if it was on before
    ssockopt(fd, TCP_CORK, 1);
#endif
#endif /* SOL_TCP */
}

/* The following code doesn't link without help, and
 * since it isn't needed by the nonblocking test, we
 * can safely disable it.
 */
#ifndef TESTING

#ifndef FD_CLOEXEC
#define FD_CLOEXEC 1
#endif /* FD_CLOEXEC */

#ifdef HAVE_BROKEN_F_SETFD
static int *fds_to_close;
static int fds_to_close_size = 0;
static int num_fds_to_close=0;

#define ASSURE_FDS_TO_CLOSE_SIZE(X) \
do{while(fds_to_close_size-1 < X) grow_fds_to_close();}while(0)

static void grow_fds_to_close(void)
{
  if(!fds_to_close_size)
    fds_to_close_size = 1;
  fds_to_close_size *= 2;
  fds_to_close = realloc( fds_to_close, sizeof( int ) * fds_to_close_size );
  if(!fds_to_close)
    Pike_fatal("Out of memory in fd_control::grow_fds_to_close()\n"
          "Tried to allocate %d fd_datum structs\n", fds_to_close_size);
  memset( fds_to_close+(fds_to_close_size/2), 0, fds_to_close_size*sizeof(int)/2 );
}

void do_close_on_exec(void)
{
  int i;
  for(i=0; i < num_fds_to_close; i++) {
    while( fd_close(fds_to_close[i]) <0 && errno==EINTR) ;
  }
  num_fds_to_close = 0;
}

void cleanup_close_on_exec(void)
{
  if (fds_to_close) {
    free(fds_to_close);
    fds_to_close = 0;
    fds_to_close_size = 0;
    num_fds_to_close = 0;
  }
}
#endif /* HAVE_BROKEN_F_SETFD */

PMOD_EXPORT int set_close_on_exec(int fd, int which)
{
#ifndef HAVE_BROKEN_F_SETFD
  int ret;
  if (which) {
    do {
      which = fcntl(fd, F_GETFD);
    } while (which < 0 && errno == EINTR);
    if (which < 0) which = FD_CLOEXEC;
    else which |= FD_CLOEXEC;
  } else {
    do {
      which = fcntl(fd, F_GETFD);
    } while (which < 0 && errno == EINTR);
    if (which < 0) which = 0;
    else which &= ~FD_CLOEXEC;
  }
  do
  {
    ret=fcntl(fd, F_SETFD, which);
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
    ASSURE_FDS_TO_CLOSE_SIZE((num_fds_to_close+1));
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

#ifndef HAVE_ACCEPT4
int accept4(int fd, struct sockaddr *addr, ACCEPT_SIZE_T *addrlen, int flags)
{
  fd = fd_accept(fd, addr, addrlen);
  if (!flags || (fd < 0)) return fd;
  if (((flags & SOCK_NONBLOCK) && (set_nonblocking(fd, 1) < 0)) ||
      ((flags & SOCK_CLOEXEC) && (set_close_on_exec(fd, 1) < 0))) {
    int e = errno;
    fd_close(fd);
    errno = e;
    return -1;
  }
  return fd;
}
#endif /* !HAVE_ACCEPT4 */

#endif /* !TESTING */

#ifdef TESTING

#include <stdlib.h>

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

RETSIGTYPE sigalrm_handler0(int tmp)
{
  signal(SIGALRM, SIG_IGN);
  alarm(0);
  _exit(0);
}
RETSIGTYPE sigalrm_handler1(int tmp)
{
  alarm(0);
  fprintf(stderr,"Failed in alarm handler 1\n");
  _exit(1);
}

int main()
{
  int tmp[2];
  char foo[1000];
  int res = 0;
  int e = 0;

  tmp[0]=0;
  tmp[1]=0;
#ifdef HAVE_SOCKETPAIR
  if(socketpair(AF_UNIX, SOCK_STREAM, 0, tmp) < 0)
    tmp[0]=tmp[1]=0;
#endif

  set_nonblocking(tmp[0],1);
  signal(SIGALRM, sigalrm_handler1);
  alarm(1);
  res = recv(tmp[0], foo, 999, 0);
  e = errno;
  alarm(0);
  if ((res >= 0) || (e != EAGAIN)) {
    fprintf(stderr,
	    "Unexpected behaviour of nonblocking read() res:%d, errno:%d\n",
	    res, e);
    exit(1);
  }
#ifdef SHORT_TEST
  /* Kludge for broken thread libraries.
   * eg: Solaris 2.4/-lthread
   */
  exit(0);
#endif /* SHORT_TEST */
  set_nonblocking(tmp[0],0);
  signal(SIGALRM, sigalrm_handler0);
  alarm(1);
  res = recv(tmp[0], foo, 999, 0);
  e = errno;
  fprintf(stderr,"Failed at end of main; res:%d, errno:%d\n", res, e);
  exit(1);
}
#endif
#endif
