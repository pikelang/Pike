#include <config.h>

#include "global.h"
RCSID("$Id: dumudp.c,v 1.5 1997/03/04 21:45:40 grubba Exp $");
#include "types.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "object.h"
#include "macros.h"
#include "backend.h"
#include "fd_control.h"

#include "error.h"
#include "signal_handler.h"
#include "pike_types.h"
#include "threads.h"

#ifdef HAVE_SYS_TYPE_H
#include <sys/types.h>
#endif

#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>

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

#ifdef HAVE_SYS_PROTOSW_H
#include <sys/protosw.h>
#endif

#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

struct dumudp {
  int fd;
};

#define THIS ((struct dumudp *)fp->current_storage)

extern void get_inet_addr(struct sockaddr_in *addr,char *name);

static void udp_bind(INT32 args)
{
  struct sockaddr_in addr;
  int o;
  int fd,tmp;

  if(args < 1) error("Too few arguments to dumudp->bind()\n");

  if(sp[-args].type != T_INT)
    error("Bad argument 1 to dumudp->bind()\n");

  fd = socket(AF_INET, SOCK_DGRAM, 0);

  if(fd < 0)
  {
    pop_n_elems(args);
    error("socket failed\n");
    return;
  }

  o=1;
  if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&o, sizeof(int)) < 0)
  {
/*    THIS->my_errno=errno;*/
    close(fd);
    error("setsockopt failed\n");
    return;
  }

  /* set_close_on_exec(fd,1); */

  MEMSET((char *)&addr,0,sizeof(struct sockaddr_in));

  if(args > 2 && sp[2-args].type==T_STRING) {
    get_inet_addr(&addr, sp[2-args].u.string->str);
  } else {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  }

  addr.sin_port = htons( ((u_short)sp[-args].u.integer) );

  THREADS_ALLOW();
  tmp=bind(fd, (struct sockaddr *)&addr, sizeof(addr))<0;
  THREADS_DISALLOW();

  if(tmp)
  {
/*    THIS->my_errno=errno;*/
    close(fd);
    pop_n_elems(args);
    push_int(0);
    return;
  }

/*
  if(args > 1)
  {
    assign_svalue(& THIS->accept_callback, sp+1-args);
    if(!IS_ZERO(& THIS->accept_callback))
      set_read_callback(fd, port_accept_callback, (void *)THIS);
    set_nonblocking(fd,1);
  }
*/
  THIS->fd=fd;
/*  THIS->my_errno=0;*/
  pop_n_elems(args);
  push_int(1);
}

#define UDP_BUFFSIZE 65536

void udp_read(INT32 args)
{
  int flags = 0, res=0;
  struct sockaddr_in from;
  int  fromlen = sizeof(struct sockaddr_in);
  char buffer[UDP_BUFFSIZE];
  
  if(args)
  {
    if(sp[-args].u.integer == 1)
      flags = MSG_OOB;
    else if(sp[-args].u.integer == 2)
      flags = MSG_PEEK;
    else if(sp[-args].u.integer == 3)
      flags = MSG_PEEK|MSG_OOB;
    else
      error("Illegal 'flags' value passed to udp->read([int flags])\n");
  }
  pop_n_elems(args);
  THREADS_ALLOW();
  while(((res = recvfrom(THIS->fd, buffer, UDP_BUFFSIZE, flags,
			 (struct sockaddr *)&from, &fromlen))==-1)
	&&(errno==EINTR));
  THREADS_DISALLOW();

  if(res<0)
  {
    switch(errno)
    {
     case EBADF:
      error("Socket closed\n");
     case ESTALE:
     case EIO:
      error("I/O error\n");
     case ENOMEM:
#ifdef ENOSR
     case ENOSR:
#endif /* ENOSR */
      error("Out of memory\n");
     case ENOTSOCK:
      fatal("reading from non-socket fd!!!\n");
     case EWOULDBLOCK:
      return;
    }
  }
  /* Now comes the interresting part.
     make a nice mapping from this stuff..
     */
  push_text("ip");
  push_text( inet_ntoa( from.sin_addr ) );

  push_text("port");
  push_int(ntohs(from.sin_port));

  push_text("data");
  push_string( make_shared_binary_string(buffer, res) );
  f_aggregate_mapping( 6 );
}

void udp_sendto(INT32 args)
{
  int flags = 0, res=0, i, fd;
  struct sockaddr_in to;
  
  if(args>3)
  {
    if(sp[3-args].u.integer == 1)
      flags = MSG_OOB;
    else if(sp[3-args].u.integer == 2)
      flags = MSG_DONTROUTE;
    else if(sp[3-args].u.integer == 3)
      flags = MSG_DONTROUTE|MSG_OOB;
    else
      error("Illegal 'flags' value passed to udp->send(string m,string t,int p,[int flags])\n");
  }
  if(!args)
    error("Illegal number of arguments to udp->sendto(string to"
	  ", string message, int port[, int flags])\n");


  if( sp[-args].type==T_STRING ) 
    get_inet_addr(&to, sp[-args].u.string->str);
  else
    error("Illegal type of argument to sendto, got non-string to-address.\n");

  to.sin_port = sp[1-args].u.integer;

  THREADS_ALLOW();
  while(((res = sendto(THIS->fd, sp[2-args].u.string->str, sp[2-args].u.string->len, flags,
		       (struct sockaddr *)&to,
		       sizeof( struct sockaddr_in )))==-1)
	&& errno==EINTR);
  THREADS_DISALLOW();
  
  if(res<0)
  {
    switch(errno)
    {
     case EMSGSIZE:
      error("Too big message\n");
     case EBADF:
      error("Socket closed\n");
     case ENOMEM:
#ifdef ENOSR
     case ENOSR:
#endif /* ENOSR */
      error("Out of memory\n");
     case EINVAL:
     case ENOTSOCK:
      fatal("Odd error!!!\n");
     case EWOULDBLOCK:
      return;
    }
  }
  pop_n_elems(args);
  push_int( res );
}


void zero_udp()
{
  MEMSET(THIS, 0, sizeof(struct dumudp));
}

void exit_udp()
{
  if(THIS->fd) close(THIS->fd);
}

void init_udp()
{
  start_new_program();

  add_storage(sizeof(struct dumudp));
  add_function("bind",udp_bind,"function(int,void|function,void|string:int)",0);
  add_function("read",udp_read,"function(int|void:mapping(string:int|string))",0);
  add_function("send",udp_sendto,"function(string,int,string,void|int:int)",0);
  set_init_callback(zero_udp);
  set_exit_callback(exit_udp);
  end_class("dumUDP",0);
}
