/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include <sys/types.h>

#ifndef TESTING
#include "global.h"
#include "error.h"
#include "fdlib.h"
#else
#undef DEBUG
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

void set_nonblocking(int fd,int which)
{
#ifdef DEBUG
  if(fd<0 || fd >MAX_OPEN_FILEDESCRIPTORS)
    fatal("Filedescriptor out of range.\n");
#endif

#if defined(USE_IOCTL_FIONBIO) || __NT__
  fd_ioctl(fd, FIONBIO, &which);
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
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
/* a part of the autoconf thingy */

RETSIGTYPE sigalrm_handler0(int tmp) { exit(0); }
RETSIGTYPE sigalrm_handler1(int tmp) { exit(1); }

/* Protected since errno may expand to a function call. */
#ifndef errno
extern int errno;
#endif /* !errno */

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif



int my_socketpair(int family, int type, int protocol, int sv[2])
{
  static int fd=-1;
  static struct sockaddr_in my_addr;
  struct sockaddr_in addr,addr2;
  int len;

  for(len=0;len<sizeof(struct sockaddr_in);len++)
    ((char *)&addr)[len]=0;

  /* We lie, we actually create an AF_INET socket... */
  if(family != AF_UNIX || type != SOCK_STREAM)
  {
    errno=EINVAL;
    return -1; 
  }

  if(fd==-1)
  {
    if((fd=socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
    
    /* I wonder what is most common a loopback on ip# 127.0.0.1 or
     * a loopback with the name "localhost"?
     * Let's hope those few people who don't have socketpair have
     * a loopback on 127.0.0.1
     */
    my_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    my_addr.sin_port=htons(0);
    
    /* Bind our sockets on any port */
    if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      close(fd);
      fd=-1;
      return -1;
    }

    /* Check what ports we got.. */
    len=sizeof(my_addr);
    if(getsockname(fd,(struct sockaddr *)&my_addr,&len) < 0)
    {
      close(fd);
      fd=-1;
      return -1;
    }

    /* Listen to connections on our new socket */
    if(listen(fd, 5) < 0)
    {
      close(fd);
      fd=-1;
      return -1;
    }
  }
  
  if((sv[1]=socket(AF_INET, SOCK_STREAM, 0)) <0) return -1;

  addr.sin_addr.s_addr=inet_addr("127.0.0.1");

/*  set_nonblocking(sv[1],1); */
  
  if(connect(sv[1], (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
  {
    int tmp2;
    for(tmp2=0;tmp2<20;tmp2++)
    {
      int tmp;
      len=sizeof(addr);
      tmp=accept(fd,(struct sockaddr *)&addr,&len);

      if(tmp!=-1) close(tmp);
      if(connect(sv[1], (struct sockaddr *)&my_addr, sizeof(my_addr))>=0)
	break;
    }
    if(tmp2>=20)
      return -1;
  }

  len=sizeof(addr);
  if(getsockname(sv[1],(struct sockaddr *)&addr2,&len) < 0) return -1;

  /* Accept connection
   * Make sure this connection was our OWN connection,
   * otherwise some wizeguy could interfere with our
   * pipe by guessing our socket and connecting at
   * just the right time... Pike is supposed to be
   * pretty safe...
   */
  do
  {
    len=sizeof(addr);
    sv[0]=accept(fd,(struct sockaddr *)&addr,&len);

    if(sv[0] < 0) {
      close(sv[1]);
      return -1;
    }

    /* We do not trust accept */
    len=sizeof(addr);
    if(getpeername(sv[0], (struct sockaddr *)&addr,&len)) return -1;
  }while(len < (int)sizeof(addr) ||
	 addr2.sin_addr.s_addr != addr.sin_addr.s_addr ||
	 addr2.sin_port != addr.sin_port);

/*   set_nonblocking(sv[1],0); */

  return 0;
}

int socketpair_ultra(int family, int type, int protocol, int sv[2])
{
  int retries=0;

  while(1)
  {
    int ret=my_socketpair(family, type, protocol, sv);
    if(ret>=0) return ret;
    
    switch(errno)
    {
      case EAGAIN: break;

#ifdef EADDRINUSE
      case EADDRINUSE:
#endif

#ifdef WSAEADDRINUSE
	if(retries++ > 10) return ret;
	break;
#endif

      default:
	return ret;
    }
  }
}

int main()
{
  int tmp[2];
  char foo[1000];

  tmp[0]=0;
  tmp[1]=0;

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
#endif
