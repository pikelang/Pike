#include <config.h>

#include "global.h"
RCSID("$Id: dumudp.c,v 1.23 1997/10/14 00:58:38 grubba Exp $");
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "mapping.h"
#include "object.h"
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

#ifdef HAVE_SYS_STREAM_H
#include <sys/stream.h>
#endif

#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

struct dumudp {
  int fd;
  struct svalue read_callback;
};

#define THIS ((struct dumudp *)fp->current_storage)
#define FD (THIS->fd)

extern void get_inet_addr(struct sockaddr_in *addr,char *name);

static void udp_bind(INT32 args)
{
  struct sockaddr_in addr;
  int o;
  int fd,tmp;

  
  if(args < 1) error("Too few arguments to dumudp->bind()\n");

  if(sp[-args].type != T_INT)
    error("Bad argument 1 to dumudp->bind()\n");

  if(FD != -1)
  {
    set_read_callback( FD, 0, 0 );
    close(FD);	/* Shouldn't this be some other taste of close()? */
    FD = -1;
  }

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
    close(fd);
    error("setsockopt failed\n");
    return;
  }

  MEMSET((char *)&addr,0,sizeof(struct sockaddr_in));

  if(args > 2 && sp[2-args].type==T_STRING) {
    get_inet_addr(&addr, sp[2-args].u.string->str);
  } else {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  }

  addr.sin_port = htons( ((u_short)sp[-args].u.integer) );
  addr.sin_family = AF_INET;

  tmp=bind(fd, (struct sockaddr *)&addr, sizeof(addr))<0;

  if(tmp)
  {
    close(fd);
    pop_n_elems(args);
    push_int(0);
    return;
  }

  FD=fd;
  pop_n_elems(args);
  push_int(1);
}

void udp_enable_broadcast(INT32 args)
{
  pop_n_elmas(args);
#ifdef SO_BROADCAST
  o = 1;
  push_int(setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *)&o, sizeof(int)));
#else /* SO_BROADCAST */
  push_int(0);
#endif /* SO_BROADCAST */
}

#define UDP_BUFFSIZE 65536

void udp_read(INT32 args)
{
  int flags = 0, res=0, fd;
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
  fd = FD;
  THREADS_ALLOW();
  while(((res = recvfrom(fd, buffer, UDP_BUFFSIZE, flags,
			 (struct sockaddr *)&from, &fromlen))==-1)
	&&(errno==EINTR));
  THREADS_DISALLOW();

  if(res<0)
  {
    switch(errno)
    {
     case EBADF:
      set_read_callback( FD, 0, 0 );
      error("Socket closed\n");
     case ESTALE:
     case EIO:
      set_read_callback( FD, 0, 0 );
      error("I/O error\n");
     case ENOMEM:
#ifdef ENOSR
     case ENOSR:
#endif /* ENOSR */
      error("Out of memory\n");
     case ENOTSOCK:
      fatal("reading from non-socket fd!!!\n");
     case EWOULDBLOCK:
      push_int( 0 );
      return;

     default:
       error("Socket read failed with errno %d.\n",errno);
    }
  }
  /* Now comes the interresting part.
   * make a nice mapping from this stuff..
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
  char *str;
  INT32 len;
  if(FD < 0)
    error("UDP: not open\n");
  
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

  to.sin_port = htons( ((u_short)sp[1-args].u.integer) );

  fd = FD;
  str = sp[2-args].u.string->str;
  len = sp[2-args].u.string->len;
  THREADS_ALLOW();
  while(((res = sendto( fd, str, len, flags, (struct sockaddr *)&to,
		       sizeof( struct sockaddr_in ))) == -1) && errno==EINTR);
  THREADS_DISALLOW();
  
  if(res<0)
  {
    switch(errno)
    {
     case EMSGSIZE:
      error("Too big message\n");
     case EBADF:
      set_read_callback( FD, 0, 0 );
      error("Socket closed\n");
     case ENOMEM:
#ifdef ENOSR
     case ENOSR:
#endif /* ENOSR */
      error("Out of memory\n");
     case EINVAL:
     case ENOTSOCK:
      set_read_callback( FD, 0, 0 );
      error("Not a socket!!!\n");
     case EWOULDBLOCK:
      return;
    }
  }
  pop_n_elems(args);
  push_int( res );
}


void zero_udp(struct object *ignored)
{
  MEMSET(THIS, 0, sizeof(struct dumudp));
  THIS->read_callback.type=T_INT;
  FD = -1;
}

void exit_udp(struct object *ignored)
{
  if(FD != -1)
  {
    set_read_callback( FD, 0, 0 );
    if (& THIS->read_callback)
      free_svalue(& THIS->read_callback );
    close(FD);
  }
}

#define THIS_DATA ((struct dumudp *)data)

static void udp_read_callback( int fd, void *data )
{
  if(IS_ZERO(&THIS_DATA->read_callback))
    set_read_callback(THIS_DATA->fd, 0, 0);
  else
    apply_svalue(& THIS_DATA->read_callback, 0);
  pop_stack(); 
  return;
}

static void udp_set_read_callback(INT32 args)
{
  if(FD < 0)
    error("File is not open.\n");

  if(args != 1)
    error("Wrong number of arguments to file->set_read_callback().\n");
  
  if(IS_ZERO(& THIS->read_callback))
    assign_svalue(& THIS->read_callback, sp-1);
  else
    assign_svalue_no_free(& THIS->read_callback, sp-1);

  if(IS_ZERO(& THIS->read_callback))
    set_read_callback(FD, 0, 0);
  else
    set_read_callback(FD, udp_read_callback, THIS);
  pop_n_elems(args);
}

static void udp_set_nonblocking(INT32 args)
{
  if (FD < 0) error("File not open.\n");

  switch(args)
  {
   default: pop_n_elems(args-1);
   case 1: udp_set_read_callback(1);
  }
  set_nonblocking(FD,1);
}

static void udp_set_blocking(INT32 args)
{
  if (FD < 0) error("File not open.\n");
  set_nonblocking(FD,0);
}

static void udp_query_address(INT32 args)
{
  struct sockaddr_in addr;
  int i,len;
  char buffer[496],*q;

  if(THIS->fd <0)
    error("socket->query_address(): Port not bound yet.\n");

  len=sizeof(addr);
  i=getsockname(THIS->fd,(struct sockaddr *)&addr,&len);
  pop_n_elems(args);
  if(i < 0 || len < (int)sizeof(addr))
  {
    push_int(0);
    return;
  }

  q=inet_ntoa(addr.sin_addr);
  strncpy(buffer,q,sizeof(buffer)-20);
  buffer[sizeof(buffer)-20]=0;
  sprintf(buffer+strlen(buffer)," %d",(int)(ntohs(addr.sin_port)));

  push_string(make_shared_string(buffer));
}


void init_udp(void)
{
  start_new_program();

  add_storage(sizeof(struct dumudp));
  add_function("bind",udp_bind,"function(int,void|function,void|string:int)",0);
  add_function("enable_broadcast", udp_enable_broadcast, "function(:void)", 0);
  add_function("read",udp_read,"function(int|void:mapping(string:int|string))",0);
  add_function("send",udp_sendto,"function(string,int,string,void|int:int)",0);
  add_function( "set_nonblocking", udp_set_nonblocking,
		"function(function(void:void):void)", 0 );
  add_function( "set_read_callback", udp_set_read_callback,
		"function(function(void:void):void)", 0 );
  add_function( "set_blocking", udp_set_blocking,"function(void:void)", 0 );
  add_function("query_address",udp_query_address,"function(:string)",0);
  set_init_callback(zero_udp);
  set_exit_callback(exit_udp);
  end_class("dumUDP",0);
}

