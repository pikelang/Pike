/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: udp.c,v 1.70 2004/09/18 20:50:57 nilsson Exp $
*/

#define NO_PIKE_SHORTHAND
#include "global.h"

#include "file_machine.h"

#include "fdlib.h"
#include "pike_netlib.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "mapping.h"
#include "object.h"
#include "backend.h"
#include "fd_control.h"

#include "pike_error.h"
#include "signal_handler.h"
#include "pike_types.h"
#include "threads.h"
#include "bignum.h"

#include "module_support.h"
#include "builtin_functions.h"
#include "file.h"

#ifdef HAVE_SYS_TYPE_H
#include <sys/types.h>
#endif

#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#ifdef HAVE_POLL

#ifdef HAVE_POLL_H
#include <poll.h>
#endif /* HAVE_POLL_H */

#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif /* HAVE_SYS_POLL_H */

/* Some constants... */

#ifndef POLLRDNORM
#define POLLRDNORM	POLLIN
#endif /* !POLLRDNORM */

#ifndef POLLRDBAND
#define POLLRDBAND	POLLPRI
#endif /* !POLLRDBAND */

#else /* !HAVE_POLL */

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#endif /* HAVE_POLL */

#if ! defined(EWOULDBLOCK) && defined(WSAEWOULDBLOCK)
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#if ! defined(EADDRINUSE) && defined(WSAEADDRINUSE)
#define EADDRINUSE WSAEADDRINUSE
#endif


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

/* Fix warning on OSF/1
 *
 * NOERROR is defined by both sys/stream.h (-1), and arpa/nameser.h (0),
 * the latter is included by netdb.h.
 */
#ifdef NOERROR
#undef NOERROR
#endif /* NOERROR */

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include "dmalloc.h"

struct udp_storage {
  struct fd_callback_box box;	/* Must be first. */
  int my_errno;
   
  int type;
  int protocol;

  struct svalue read_callback;	/* Mapped. */
};

void zero_udp(struct object *ignored);
int low_exit_udp(void);
void exit_udp(struct object *ignored) {
  low_exit_udp();
}

#undef THIS
#define THIS ((struct udp_storage *)Pike_fp->current_storage)
#define THISOBJ (Pike_fp->current_object)
#define FD (THIS->box.fd)

/*! @module Stdio
 */

/*! @class UDP
 */

/*! @decl int(0..1) close()
 *!
 *! Closes an open UDP port.
 *!
 *! @note
 *!   This method was introduced in Pike 7.5.
 */
static void udp_close(INT32 args)
{
  int ret = low_exit_udp();
  zero_udp(NULL);
  pop_n_elems(args);
  push_int( ret==0 );
}

/*! @decl UDP bind(int|string port)
 *! @decl UDP bind(int|string port, string address)
 *!
 *! Binds a port for recieving or transmitting UDP.
 *! @throws
 *!   Throws error when unable to bind port.
 */
static void udp_bind(INT32 args)
{
  PIKE_SOCKADDR addr;
  int addr_len;
  int o;
  int fd,tmp;
#if !defined(SOL_IP) && defined(HAVE_GETPROTOBYNAME)
  static int ip_proto_num = -1;
#endif /* !SOL_IP && HAVE_GETPROTOBYNAME */

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Stdio.UDP->bind", 1);

  if(Pike_sp[-args].type != PIKE_T_INT &&
     (Pike_sp[-args].type != PIKE_T_STRING ||
      Pike_sp[-args].u.string->size_shift))
    SIMPLE_BAD_ARG_ERROR("Stdio.UDP->bind", 1, "int|string (8bit)");

  if(FD != -1)
  {
    fd_close(FD);
    change_fd_for_box (&THIS->box, -1);
  }

  addr_len = get_inet_addr(&addr, (args > 1 && Pike_sp[1-args].type==PIKE_T_STRING?
				   Pike_sp[1-args].u.string->str : NULL),
			   (Pike_sp[-args].type == PIKE_T_STRING?
			    Pike_sp[-args].u.string->str : NULL),
			   (Pike_sp[-args].type == PIKE_T_INT?
			    Pike_sp[-args].u.integer : -1), 1);

  fd = fd_socket(SOCKADDR_FAMILY(addr), THIS->type, THIS->protocol);
  if(fd < 0)
  {
    pop_n_elems(args);
    THIS->my_errno=errno;
    Pike_error("Stdio.UDP->bind: failed to create socket\n");
  }

  /* Make sure this fd gets closed on exec. */
  set_close_on_exec(fd, 1);

  o=1;
  if(fd_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&o, sizeof(int)) < 0)
  {
    fd_close(fd);
    THIS->my_errno=errno;
    Pike_error("Stdio.UDP->bind: setsockopt SO_REUSEADDR failed\n");
  }

#ifndef SOL_IP
#ifdef HAVE_GETPROTOBYNAME
  if (ip_proto_num == -1) {
    /* NOTE: getprotobyname(3N) is not MT-safe. */
    struct protoent *proto = getprotobyname("ip");
    if (!proto) {
      /* No entry for IP, assume zero. */
      ip_proto_num = 0;
    } else {
      ip_proto_num = proto->p_proto;
    }
  }
#define SOL_IP ip_proto_num
#else /* !HAVE_GETPROTOBYNAME */
#define SOL_IP 0
#endif /* HAVE_GETPROTOBYNAME */
#endif /* SOL_IP */

#ifdef IP_HDRINCL
  /*  From mtr-0.28:net.c: FreeBSD wants this to avoid sending
      out packets with protocol type RAW to the network. */

  if (THIS->type==SOCK_RAW && THIS->protocol==255 /* raw */)
     if(fd_setsockopt(fd, SOL_IP, IP_HDRINCL, (char *)&o, sizeof(int)))
	Pike_error("Stdio.UDP->bind: setsockopt IP_HDRINCL failed\n");
#endif /* IP_HDRINCL */

  THREADS_ALLOW_UID();

  tmp=fd_bind(fd, (struct sockaddr *)&addr, addr_len)<0;

  THREADS_DISALLOW_UID();

  if(tmp)
  {
    fd_close(fd);
    THIS->my_errno=errno;
    Pike_error("Stdio.UDP->bind: failed to bind to port %d\n",
	       (unsigned INT16)Pike_sp[-args].u.integer);
    return;
  }

  change_fd_for_box (&THIS->box, fd);
  pop_n_elems(args);
  ref_push_object(THISOBJ);
}

/*! @decl int(0..1) enable_broadcast()
 *!
 *! Set the broadcast flag. 
 *! If enabled then sockets receive packets sent  to  a  broadcast
 *! address  and  they are allowed to send packets to a
 *! broadcast address.
 *!
 *! @returns
 *!  Returns @expr{1@} on success, @expr{0@} (zero) otherwise.
 *!
 *! @note
 *!  This is normally only avalable to root users.
 */
void udp_enable_broadcast(INT32 args)
{
#ifdef SO_BROADCAST
  int o = 1;
  pop_n_elems(args);
  push_int(fd_setsockopt(FD, SOL_SOCKET, SO_BROADCAST, (char *)&o, sizeof(int)));
  THIS->my_errno=errno;
#else /* SO_BROADCAST */
  pop_n_elems(args);
  push_int(0);
#endif /* SO_BROADCAST */
}

/*! @decl int(0..1) enable_multicast(string reply_address)
 *! Set the local device for a multicast socket. See also the Unix man
 *! page for setsocketopt IPPROTO_IP IP_MULTICAST_IF.
 */
void udp_enable_multicast(INT32 args)
{
  int result;
  char *ip;
  PIKE_SOCKADDR reply;

  get_all_args("enable_multicast", args, "%s", &ip);

  get_inet_addr(&reply, ip, NULL, -1, 1);

  if(SOCKADDR_FAMILY(reply) != AF_INET)
    Pike_error("Multicast only supported for IPv4.\n");

  result = fd_setsockopt(FD, IPPROTO_IP, IP_MULTICAST_IF,
			 (char *)&reply.ipv4.sin_addr,
			 sizeof(reply.ipv4.sin_addr));
  pop_n_elems(args);
  push_int(result);
}

/*! @decl int set_multicast_ttl(int ttl)
 *! Set the time-to-live value of outgoing multicast packets for this
 *! socket. It is very important for multicast packets to set the
 *! smallest TTL possible. The default is 1 which means that multicast
 *! packets don't leacl the local network unless the user program
 *! explicitly request it. See also the Unix man page for setsocketopt
 *! IPPROTO_IP IP_MULTICAST_TTL.
 */
void udp_set_multicast_ttl(INT32 args)
{
  int ttl;
  get_all_args("set_multicast_ttl", args, "%d", &ttl);
  pop_n_elems(args);
  push_int( fd_setsockopt(FD, IPPROTO_IP, IP_MULTICAST_TTL,
			  (char *)&ttl, sizeof(int)) );
}

/*! @decl int add_membership(string group, void|string address)
 *! Join a multicast group. @[group] contains the address of the
 *! multicast group the application wants to join or leave. It must be
 *! a valid multicast address. @[address] is the address of the local
 *! interface with wich the system should join to the multicast group.
 *! If not provided the system will select an appropriate interface.
 *! See also the Unix man page for setsocketopt IPPROTO_IP
 *! ADD_MEMBERSHIP.
 *! @seealso
 *!   @[drop_membership]
 */
void udp_add_membership(INT32 args)
{
  int face=0;
  char *group;
  char *address=0;
  struct ip_mreq sock;
  PIKE_SOCKADDR addr;

  get_all_args("add_membership", args, "%s.%s%d", &group, &address, &face);

  get_inet_addr(&addr, group, NULL, -1, 1);

  if(SOCKADDR_FAMILY(addr) != AF_INET)
    Pike_error("Multicast only supported for IPv4.\n");

  sock.imr_multiaddr = addr.ipv4.sin_addr;

  if( !address )
    sock.imr_interface.s_addr = htonl( INADDR_ANY );
  else {
    get_inet_addr(&addr, address, NULL, -1, 1);

    if(SOCKADDR_FAMILY(addr) != AF_INET)
      Pike_error("Multicast only supported for IPv4.\n");

    sock.imr_interface = addr.ipv4.sin_addr;
  }
#if 0
  sock.imr_ifindex = face;
#endif

  pop_n_elems(args);
  push_int( fd_setsockopt(FD, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			  (char *)&sock, sizeof(sock)) );
}

/*! @decl int drop_membership(string group, void|string address)
 *! Leave a multicast group.
 *! @seealso
 *!   @[add_membership]
 */
void udp_drop_membership(INT32 args)
{
  int face=0;
  char *group;
  char *address=0;
  struct ip_mreq sock;
  PIKE_SOCKADDR addr;

  get_all_args("drop_membership", args, "%s.%s%d", &group, &address, &face);

  get_inet_addr(&addr, group, NULL, -1, 1);

  if(SOCKADDR_FAMILY(addr) != AF_INET)
    Pike_error("Multicast only supported for IPv4.\n");

  sock.imr_multiaddr = addr.ipv4.sin_addr;

  if( !address )
    sock.imr_interface.s_addr = htonl( INADDR_ANY );
  else {
    get_inet_addr(&addr, address, NULL, -1, 1);

    if(SOCKADDR_FAMILY(addr) != AF_INET)
      Pike_error("Multicast only supported for IPv4.\n");

    sock.imr_interface = addr.ipv4.sin_addr;
  }
#if 0
  sock.imr_ifindex = face;
#endif

  pop_n_elems(args);
  push_int( fd_setsockopt(FD, IPPROTO_IP, IP_DROP_MEMBERSHIP,
			  (char *)&sock, sizeof(sock)) );
}

/*! @decl int(0..1) wait(int|float timeout)
 *!
 *! Check for data and wait max. @[timeout] seconds.
 *!
 *! @returns
 *!  Returns @expr{1@} if data are ready, @expr{0@} (zero) otherwise.
 */
void udp_wait(INT32 args)
{
#ifdef HAVE_POLL
  struct pollfd pollfds[1];
  int ms;
#else /* !HAVE_POLL */
  fd_set rset;
  struct timeval tv;
#endif /* HAVE_POLL */
  FLOAT_TYPE timeout;
  int fd = FD;
  int res;
  int e;

  get_all_args("wait", args, "%F", &timeout);

  if (timeout < 0.0) {
    timeout = 0.0;
  }

  if (fd < 0) {
    Pike_error("udp->wait(): Port not bound!\n");
  }

#ifdef HAVE_POLL
  THREADS_ALLOW();

  pollfds->fd = fd;
  pollfds->events = POLLIN;
  pollfds->revents = 0;
  ms = timeout * 1000;
  res = poll(pollfds, 1, ms);
  e = errno;

  THREADS_DISALLOW();
  if (!res) {
    /* Timeout */
  } else if (res < 0) {
    /* Error */
    Pike_error("udp->wait(): poll() failed with errno %d\n", e);
  } else {
    /* Success? */
    if (pollfds->revents) {
      res = 1;
    } else {
      res = 0;
    }
  }
#else /* !HAVE_POLL */
  THREADS_ALLOW();

  FD_ZERO(&rset);
  FD_SET(fd, &rset);
  tv.tv_sec = DO_NOT_WARN((int)timeout);
  tv.tv_usec = DO_NOT_WARN((int)((timeout - ((int)timeout)) * 1000000.0));
  res = select(fd+1, &rset, NULL, NULL, &tv);
  e = errno;

  THREADS_DISALLOW();
  if (!res) {
    /* Timeout */
  } else if (res < 0) {
    /* Error */
    Pike_error("udp->wait(): select() failed with errno %d\n", e);
  } else {
    /* Success? */
    if (FD_ISSET(fd, &rset)) {
      res = 1;
    } else {
      res = 0;
    }
  }
#endif /* HAVE_POLL */

  pop_n_elems(args);

  push_int(res);
}

/* NOTE: Some versions of AIX seem to have a
 *         #define events reqevents
 *       in one of the poll headerfiles. This will break
 *       the fd_box event handling.
 */
#undef events

#define UDP_BUFFSIZE 65536

/*! @decl mapping(string:int|string) read()
 *! @decl mapping(string:int|string) read(int flag)
 *!
 *! Read from the UDP socket.
 *!
 *! Flag @[flag] is a bitfield, 1 for out of band data and 2 for peek
 *!
 *! @returns
 *!  mapping(string:int|string) in the form 
 *!	([
 *!	   "data" : string received data
 *!	   "ip" : string   received from this ip
 *!	   "port" : int    ...and this port
 *!	])
 *!
 *! @seealso
 *!   @[set_read_callback()]
 */
void udp_read(INT32 args)
{
  int flags = 0, res=0, fd, e;
  PIKE_SOCKADDR from;
  char buffer[UDP_BUFFSIZE];
  ACCEPT_SIZE_T fromlen = sizeof(from);
  
  if(args)
  {
    if(Pike_sp[-args].u.integer & 1) {
      flags |= MSG_OOB;
    }
    if(Pike_sp[-args].u.integer & 2) {
#ifdef MSG_PEEK
      flags |= MSG_PEEK;
#else /* !MSG_PEEK */
      /* FIXME: What should we do here? */
#endif /* MSG_PEEK */
    }
    if(Pike_sp[-args].u.integer & ~3) {
      Pike_error("Illegal 'flags' value passed to udp->read([int flags])\n");
    }
  }
  pop_n_elems(args);
  fd = FD;
  if (FD < 0)
    Pike_error("Stdio.UDP->read: not open\n");
  do {
    THREADS_ALLOW();
    res = fd_recvfrom(fd, buffer, UDP_BUFFSIZE, flags,
		      (struct sockaddr *)&from, &fromlen);
    e = errno;
    THREADS_DISALLOW();

    check_threads_etc();
  } while((res==-1) && (e==EINTR));

  THIS->my_errno=errno=e;

  if(res<0)
  {
    switch(e)
    {
#ifdef WSAEBADF
       case WSAEBADF:
#endif
       case EBADF:
	  if (THIS->box.backend)
	    set_fd_callback_events (&THIS->box, 0);
	  Pike_error("Socket closed\n");
#ifdef ESTALE
       case ESTALE:
#endif
       case EIO:
	  if (THIS->box.backend)
	    set_fd_callback_events (&THIS->box, 0);
	  Pike_error("I/O error\n");
       case ENOMEM:
#ifdef ENOSR
       case ENOSR:
#endif /* ENOSR */
	  Pike_error("Out of memory\n");
#ifdef ENOTSOCK
       case ENOTSOCK:
	  Pike_fatal("reading from non-socket fd!!!\n");
#endif
       case EWOULDBLOCK:
	  push_int( 0 );
	  return;

       default:
	  Pike_error("Socket read failed with errno %d.\n", e);
    }
  }
  /* Now comes the interresting part.
   * make a nice mapping from this stuff..
   */
  push_constant_text("data");
  push_string( make_shared_binary_string(buffer, res) );

  push_constant_text("ip");
#ifdef HAVE_INET_NTOP
  push_text( inet_ntop( SOCKADDR_FAMILY(from), SOCKADDR_IN_ADDR(from),
			buffer, sizeof(buffer) ) );
#else
  push_text( inet_ntoa( *SOCKADDR_IN_ADDR(from) ) );
#endif

  push_constant_text("port");
  push_int(ntohs(from.ipv4.sin_port));
  f_aggregate_mapping( 6 );
}

/*! @decl int send(string to, int|string port, string message)
 *! @decl int send(string to, int|string port, string message, int flags)
 *!
 *! Send data to a UDP socket. The recepient address will be @[to]
 *! and port will be @[port].
 *!
 *! Flag @[flag] is a bitfield, 1 for out of band data and
 *! 2 for don't route flag.
 *!
 *! @returns
 *! The number of bytes that were actually written.
 */
void udp_sendto(INT32 args)
{
  int flags = 0, fd, e;
  ptrdiff_t res = 0;
  PIKE_SOCKADDR to;
  int to_len;
  char *str;
  ptrdiff_t len;

  if(FD < 0)
    Pike_error("UDP: not open\n");
  
  check_all_args("send", args,
		 BIT_STRING, BIT_INT|BIT_STRING, BIT_STRING, BIT_INT|BIT_VOID, 0);
  
  if(args>3)
  {
    if(Pike_sp[3-args].u.integer & 1) {
      flags |= MSG_OOB;
    }
    if(Pike_sp[3-args].u.integer & 2) {
#ifdef MSG_DONTROUTE
      flags |= MSG_DONTROUTE;
#else /* !MSG_DONTROUTE */
      /* FIXME: What should we do here? */
#endif /* MSG_DONTROUTE */
    }
    if(Pike_sp[3-args].u.integer & ~3) {
      Pike_error("Illegal 'flags' value passed to "
		 "Stdio.UDP->send(string to, int|string port, string message, int flags)\n");
    }
  }

  to_len = get_inet_addr(&to, Pike_sp[-args].u.string->str,
			 (Pike_sp[1-args].type == PIKE_T_STRING?
			  Pike_sp[1-args].u.string->str : NULL),
			 (Pike_sp[1-args].type == PIKE_T_INT?
			  Pike_sp[1-args].u.integer : -1), 1);

  fd = FD;
  str = Pike_sp[2-args].u.string->str;
  len = Pike_sp[2-args].u.string->len;

  do {
    THREADS_ALLOW();
    res = fd_sendto( fd, str, len, flags, (struct sockaddr *)&to, to_len);
    e = errno;
    THREADS_DISALLOW();

    check_threads_etc();
  } while((res == -1) && e==EINTR);
  
  if(res<0)
  {
    switch(e)
    {
#ifdef EMSGSIZE
       case EMSGSIZE:
#endif
	  Pike_error("Too big message\n");
       case EBADF:
	  if (THIS->box.backend)
	    set_fd_callback_events (&THIS->box, 0);
	  Pike_error("Socket closed\n");
       case ENOMEM:
#ifdef ENOSR
       case ENOSR:
#endif /* ENOSR */
	  Pike_error("Out of memory\n");
       case EINVAL:
#ifdef ENOTSOCK
       case ENOTSOCK:
	  if (THIS->box.backend)
	    set_fd_callback_events (&THIS->box, 0);
	  Pike_error("Not a socket!!!\n");
#endif
       case EWOULDBLOCK:
	  Pike_error("Message would block.\n");
    }
  }
  pop_n_elems(args);
  push_int64(res);
}


void zero_udp(struct object *o)
{
  THIS->box.backend = NULL;
  THIS->box.ref_obj = o;
  FD = -1;
  THIS->my_errno = 0;
  THIS->type=SOCK_DGRAM;
  THIS->protocol=0;
  /* map_variable handles read_callback. */
}

int low_exit_udp()
{
  int fd = FD;
  int ret = 0;

  if(fd != -1)
  {
    THREADS_ALLOW();
    ret = fd_close(fd);
    THREADS_DISALLOW();
  }

  unhook_fd_callback_box (&THIS->box);

  /* map_variable handles read_callback. */

  return ret;
}

static int got_udp_event (struct fd_callback_box *box, int event)
{
  struct udp_storage *u = (struct udp_storage *) box;
#ifdef PIKE_DEBUG
  if (event != PIKE_FD_READ)
    Pike_fatal ("Got unexpected event %d.\n", event);
#endif

  u->my_errno = errno;		/* Propagate backend setting. */

  check_destructed (&u->read_callback);
  if (UNSAFE_IS_ZERO (&u->read_callback))
    set_fd_callback_events (&u->box, 0);
  else {
    apply_svalue (&u->read_callback, 0);
    if (Pike_sp[-1].type == PIKE_T_INT && Pike_sp[-1].u.integer == -1) {
      pop_stack();
      return -1;
    }
    pop_stack();
  }
  return 0;
}

static void udp_set_read_callback(INT32 args)
{
  struct udp_storage *u = THIS;
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Stdio.UDP->set_read_callback", 1);
  if(args > 1)
    pop_n_elems(args-1);

  assign_svalue(& u->read_callback, Pike_sp-1);
  if (UNSAFE_IS_ZERO (Pike_sp - 1)) {
    if (u->box.backend)
      set_fd_callback_events (&u->box, 0);
  }
  else {
    if (!u->box.backend)
      INIT_FD_CALLBACK_BOX (&u->box, default_backend, u->box.ref_obj,
			    u->box.fd, PIKE_BIT_FD_READ, got_udp_event);
    else
      set_fd_callback_events (&u->box, PIKE_BIT_FD_READ);
  }

  pop_stack();
  ref_push_object(THISOBJ);
}

static void udp_set_nonblocking(INT32 args)
{
  if (FD < 0) Pike_error("File not open.\n");

  if (args)
  {
     udp_set_read_callback(args);
     pop_stack();
  }
  set_nonblocking(FD,1);
  ref_push_object(THISOBJ);
}

/*! @decl object set_blocking()
 *!
 *! Sets this object to be blocking.
 */
static void udp_set_blocking(INT32 args)
{
  if (FD < 0) Pike_error("File not open.\n");
  set_nonblocking(FD,0);
  pop_n_elems(args);
  ref_push_object(THISOBJ);
}

/*! @decl int(0..1) connect(string address, int|string port)
 *!
 *!   Establish an UDP connection.
 *!
 *!   This function connects an UDP socket previously created with
 *!   @[Stdio.UDP()] to a remote socket. The @[address] is the IP name or
 *!   number for the remote machine. 
 *!
 *! @returns
 *!   Returns @expr{1@} on success, @expr{0@} (zero) otherwise.
 *!
 *! @note
 *!   If the socket is in nonblocking mode, you have to wait
 *!   for a write or close callback before you know if the connection
 *!   failed or not.
 *!
 *! @seealso
 *!   @[bind()], @[query_address()]
 */
static void udp_connect(INT32 args)
{
  PIKE_SOCKADDR addr;
  int addr_len;
  struct pike_string *dest_addr = NULL;
  struct svalue *dest_port = NULL;

  int tmp;

  get_all_args("UDP.connect", args, "%S%*", &dest_addr, &dest_port);

  if(dest_port->type != PIKE_T_INT &&
     (dest_port->type != PIKE_T_STRING || dest_port->u.string->size_shift))
    SIMPLE_BAD_ARG_ERROR("UDP.connect", 2, "int|string (8bit)");

  addr_len =  get_inet_addr(&addr, dest_addr->str,
			    (dest_port->type == PIKE_T_STRING?
			     dest_port->u.string->str : NULL),
			    (dest_port->type == PIKE_T_INT?
			     dest_port->u.integer : -1), 0);

  if(FD < 0)
  {
     FD = fd_socket(SOCKADDR_FAMILY(addr), SOCK_DGRAM, 0);
     if(FD < 0)
     {
	THIS->my_errno=errno;
	Pike_error("Stdio.UDP->connect: failed to create socket\n");
     }
     set_close_on_exec(FD, 1);
  }

  tmp=FD;
  THREADS_ALLOW();
  tmp=fd_connect(tmp, (struct sockaddr *)&addr, addr_len);
  THREADS_DISALLOW();

  if(tmp < 0)
  {
    THIS->my_errno=errno;
    Pike_error("Stdio.UDP->connect: failed to connect\n");
  }else{
    THIS->my_errno=0;
    pop_n_elems(args);
    push_int(1);
  }
}

/*! @decl string query_address()
 *! 
 *! Returns the local address of a socket on the form "x.x.x.x port".
 *! If this file is not a socket, not connected or some other error occurs,
 *! zero is returned.
 */
static void udp_query_address(INT32 args)
{
  PIKE_SOCKADDR addr;
  int i;
  int fd = FD;
  char buffer[496],*q;
  ACCEPT_SIZE_T len;

  if(fd <0)
    Pike_error("Stdio.UDP->query_address(): Port not bound yet.\n");

  THREADS_ALLOW();

  len=sizeof(addr);
  i=fd_getsockname(fd,(struct sockaddr *)&addr,&len);

  THREADS_DISALLOW();

  pop_n_elems(args);
  if(i < 0 || len < (int)sizeof(addr.ipv4))
  {
    push_int(0);
    return;
  }

#ifdef HAVE_INET_NTOP
  inet_ntop(SOCKADDR_FAMILY(addr), SOCKADDR_IN_ADDR(addr),
	    buffer, sizeof(buffer)-20);
#else
  q=inet_ntoa(*SOCKADDR_IN_ADDR(addr));
  strncpy(buffer,q,sizeof(buffer)-20);
  buffer[sizeof(buffer)-20]=0;
#endif
  sprintf(buffer+strlen(buffer)," %d",(int)(ntohs(addr.ipv4.sin_port)));

  push_text(buffer);
}

/*! @decl void set_backend (Pike.Backend backend)
 *!
 *! Set the backend used for the read callback.
 *!
 *! @note
 *! The backend keeps a reference to this object as long as there can
 *! be calls to the read callback, but this object does not keep a
 *! reference to the backend.
 *!
 *! @seealso
 *!   @[query_backend]
 */
static void udp_set_backend (INT32 args)
{
  struct udp_storage *u = THIS;
  struct Backend_struct *backend;

  if (!args)
    SIMPLE_TOO_FEW_ARGS_ERROR ("Stdio.UDP->set_backend", 1);
  if (Pike_sp[-args].type != PIKE_T_OBJECT)
    SIMPLE_BAD_ARG_ERROR ("Stdio.UDP->set_backend", 1, "object(Pike.Backend)");
  backend = (struct Backend_struct *)
    get_storage (Pike_sp[-args].u.object, Backend_program);
  if (!backend)
    SIMPLE_BAD_ARG_ERROR ("Stdio.UDP->set_backend", 1, "object(Pike.Backend)");

  if (u->box.backend)
    change_backend_for_box (&u->box, backend);
  else
    INIT_FD_CALLBACK_BOX (&u->box, backend, u->box.ref_obj,
			  u->box.fd, 0, got_udp_event);

  pop_n_elems (args - 1);
}

/*! @decl Pike.Backend query_backend()
 *!
 *! Return the backend used for the read callback.
 *!
 *! @seealso
 *!   @[set_backend]
 */
static void udp_query_backend (INT32 args)
{
  pop_n_elems (args);
  ref_push_object (get_backend_obj (THIS->box.backend ? THIS->box.backend :
				    default_backend));
}

/*! @decl int errno()
 *!
 *! Returns the error code for the last command on this object.
 *! Error code is normally cleared when a command is successful.
 */
static void udp_errno(INT32 args)
{
   pop_n_elems(args);
   push_int(THIS->my_errno);
}

/*! @decl UDP set_type(int sock_type)
 *! @decl UDP set_type(int sock_type, int family)
 *!
 *! Sets socket type and protocol family.
 */
static void udp_set_type(INT32 args)
{
   int type, proto;

   get_all_args("Stdio.UDP->set_type",args,"%d.%d",&type,&proto);

   THIS->type=type;
   THIS->protocol=proto;
   
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl array(int) get_type()
 *!
 *! Returns socket type and protocol family.
 */
static void udp_get_type(INT32 args)
{
   pop_n_elems(args);
   push_int(THIS->type);
   push_int(THIS->protocol);
   f_aggregate(2);
}

/*! @decl constant MSG_OOB
 *! @fixme
 *! Document this constant.
 */

/*! @decl constant MSG_PEEK
 *! @fixme
 *! Document this constant.
 */

/*! @endclass
 */

/*! @endmodule
 */

void init_udp(void)
{
  START_NEW_PROGRAM_ID (STDIO_UDP);

  ADD_STORAGE(struct udp_storage);

  PIKE_MAP_VARIABLE("_read_callback", OFFSETOF(udp_storage, read_callback),
		    tFunc(tNone, tInt_10), PIKE_T_MIXED, ID_STATIC|ID_PRIVATE);

  ADD_FUNCTION("set_type",udp_set_type,
	       tFunc(tInt tOr(tVoid,tInt),tObj),0);
  ADD_FUNCTION("get_type",udp_get_type,
	       tFunc(tNone,tArr(tInt)),0);

  ADD_FUNCTION("close", udp_close, tFunc(tNone, tInt01), 0);

  ADD_FUNCTION("bind",udp_bind,
	       tFunc(tOr(tInt,tStr) tOr(tVoid,tStr),tObj),0);

  ADD_FUNCTION("enable_broadcast", udp_enable_broadcast,
	       tFunc(tNone,tInt01), 0);

  ADD_FUNCTION("enable_multicast", udp_enable_multicast,
	       tFunc(tStr,tInt01), 0);

  ADD_FUNCTION("set_multicast_ttl", udp_set_multicast_ttl,
	       tFunc(tInt,tInt), 0);

  ADD_FUNCTION("add_membership", udp_add_membership,
	       tFunc(tStr tOr(tVoid,tStr tOr(tVoid,tInt)),tInt), 0);

  ADD_FUNCTION("drop_membership", udp_drop_membership,
	       tFunc(tStr tOr(tVoid,tStr tOr(tVoid,tInt)),tInt), 0);

  ADD_FUNCTION("wait", udp_wait, tFunc(tOr(tInt, tFloat), tInt), 0);

  ADD_FUNCTION("read",udp_read,
	       tFunc(tOr(tInt,tVoid),tMap(tStr,tOr(tInt,tStr))),0);

  add_integer_constant("MSG_OOB", 1, 0);
#ifdef MSG_PEEK
  add_integer_constant("MSG_PEEK", 2, 0);
#endif /* MSG_PEEK */

  ADD_FUNCTION("send",udp_sendto,
	       tFunc(tStr tOr(tInt,tStr) tStr tOr(tVoid,tInt),tInt),0);

  ADD_FUNCTION("connect",udp_connect,
	       tFunc(tString tOr(tInt,tStr),tInt),0);
  
  ADD_FUNCTION("_set_nonblocking", udp_set_nonblocking,
	       tFunc(tOr(tFunc(tVoid,tVoid),tVoid),tObj), 0 );
  ADD_FUNCTION("_set_read_callback", udp_set_read_callback,
	       tFunc(tFunc(tVoid,tVoid),tObj), 0 );

  ADD_FUNCTION("set_blocking",udp_set_blocking,tFunc(tVoid,tObj), 0 );
  ADD_FUNCTION("query_address",udp_query_address,tFunc(tNone,tStr),0);

  ADD_FUNCTION ("set_backend", udp_set_backend, tFunc(tObj,tVoid), 0);
  ADD_FUNCTION ("query_backend", udp_query_backend, tFunc(tVoid,tObj), 0);

  ADD_FUNCTION("errno",udp_errno,tFunc(tNone,tInt),0);

  set_init_callback(zero_udp);
  set_exit_callback(exit_udp);

  end_class("UDP",0);

  START_NEW_PROGRAM_ID (STDIO_SOCK);
#ifdef SOCK_STREAM
  add_integer_constant("STREAM",SOCK_STREAM,0);
#endif
#ifdef SOCK_DGRAM
  add_integer_constant("DGRAM",SOCK_DGRAM,0);
#endif
#ifdef SOCK_SEQPACKET
  add_integer_constant("SEQPACKET",SOCK_SEQPACKET,0);
#endif
#ifdef SOCK_RAW
  add_integer_constant("RAW",SOCK_RAW,0);
#endif
#ifdef SOCK_RDM
  add_integer_constant("RDM",SOCK_RDM,0);
#endif
#ifdef SOCK_PACKET
  add_integer_constant("PACKET",SOCK_PACKET,0);
#endif
   push_program(end_program());
   f_call_function(1);
   simple_add_constant("SOCK",Pike_sp-1,0);
   pop_stack();

   START_NEW_PROGRAM_ID (STDIO_IPPROTO);
   add_integer_constant("IP",0,0);       /* Dummy protocol for TCP.  */
   add_integer_constant("HOPOPTS",0,0);  /* IPv6 Hop-by-Hop options.  */
   add_integer_constant("ICMP",1,0);     /* Internet Control Message Protocol.  */
   add_integer_constant("IGMP",2,0);     /* Internet Group Management Protocol. */
   add_integer_constant("GGP", 3, 0);	 /* Gateway-Gateway Protocol.  */
   add_integer_constant("IPIP",4,0);     /* IPIP tunnels (older KA9Q tunnels use 94).  */
   add_integer_constant("TCP",6,0);      /* Transmission Control Protocol.  */
   add_integer_constant("EGP",8,0);      /* Exterior Gateway Protocol.  */
   add_integer_constant("PUP",12,0);     /* PUP protocol.  */
   add_integer_constant("UDP",17,0);     /* User Datagram Protocol.  */
   add_integer_constant("IDP",22,0);     /* XNS IDP protocol.  */
   add_integer_constant("RDP", 27, 0);	 /* "Reliable Datagram" Protocol.  */
   add_integer_constant("TP",29,0);      /* SO Transport Protocol Class 4.  */
   add_integer_constant("IPV6",41,0);    /* IPv6 header.  */
   add_integer_constant("ROUTING",43,0); /* IPv6 routing header.  */
   add_integer_constant("FRAGMENT",44,0);/* IPv6 fragmentation header.  */
   add_integer_constant("RSVP",46,0);    /* Reservation Protocol.  */
   add_integer_constant("GRE",47,0);     /* General Routing Encapsulation.  */
   add_integer_constant("ESP",50,0);     /* encapsulating security payload.  */
   add_integer_constant("AH",51,0);      /* authentication header.  */
   add_integer_constant("ICMPV6",58,0);  /* ICMPv6.  */
   add_integer_constant("NONE",59,0);    /* IPv6 no next header.  */
   add_integer_constant("DSTOPTS",60,0); /* IPv6 destination options.  */
   add_integer_constant("MTP",92,0);     /* Multicast Transport Protocol.  */
   add_integer_constant("ENCAP",98,0);   /* Encapsulation Header.  */
   add_integer_constant("PIM",103,0);    /* Protocol Independent Multicast.  */
   add_integer_constant("COMP",108,0);   /* Compression Header Protocol.  */
   add_integer_constant("RAW",255,0);    /* Raw IP packets.  */
   push_program(end_program());
   f_call_function(1);
   simple_add_constant("IPPROTO",Pike_sp-1,0);
   pop_stack();

}
