/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

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
#include "time_stuff.h"
#include "fd_control.h"

#include "pike_error.h"
#include "signal_handler.h"
#include "pike_types.h"
#include "threads.h"
#include "bignum.h"

#include "module_support.h"
#include "builtin_functions.h"
#include "file.h"

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
#elif defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif /* HAVE_POLL_H || HAVE_SYS_POLL_H */

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

struct udp_storage {
  struct fd_callback_box box;	/* Must be first. */
  int my_errno;

  int inet_flags;

  int type;
  int protocol;

  struct svalue read_callback;	/* Mapped. */
  struct svalue write_callback;	/* Mapped. */
};

void zero_udp(struct object *ignored);
int low_exit_udp(void);
void exit_udp(struct object *UNUSED(ignored)) {
  low_exit_udp();
}

#undef THIS
#define THIS ((struct udp_storage *)Pike_fp->current_storage)
#define THISOBJ (Pike_fp->current_object)
#define FD (THIS->box.fd)

/*! @module _Stdio
 */

/*! @class UDP
 */

/*! @decl UDP fd_factory()
 *!
 *! Factory creating @[Stdio.UDP] objects.
 *!
 *! This function is called by @[dup()]
 *! and other functions creating new UDP objects.
 *!
 *! The default implementation calls @expr{object_program(this_object())()@}
 *! to create the new object, and returns the @[UDP] inherit in it.
 *!
 *! @note
 *!   Note that this function must return the @[UDP] inherit in the object.
 *!
 *! @seealso
 *!   @[dup()], @[Stdio.File()->fd_factory()]
 */
static int udp_fd_factory_fun_num = -1;
static void udp_fd_factory(INT32 args)
{
  pop_n_elems(args);
  push_object_inherit(clone_object_from_object(Pike_fp->current_object, 0),
		      Pike_fp->context - Pike_fp->current_program->inherits);
}

/*! @decl object(UDP) `_fd()
 *!
 *! Getter for the UDP object.
 */
static void udp_backtick__fd(INT32 args)
{
  pop_n_elems(args);
  ref_push_object_inherit(Pike_fp->current_object,
			  Pike_fp->context -
			  Pike_fp->current_program->inherits);
}

/* Use ptrdiff_t for the fd since we're passed a void * and should
 * read it as an integer of the same size. */
static void do_close_udp(void *_fd)
{
  int ret;
  ptrdiff_t fd = (ptrdiff_t)_fd;

  if (fd < 0) return;
  errno = 0;
  do {
    ret = fd_close(fd);
  } while ((ret == -1) && (errno == EINTR));
}

static void push_new_udp_object(int factory_fun_num, int fd)
{
  struct object *o = NULL;
  struct udp_storage *udp;
  ONERROR err;
  struct inherit *inh;

  SET_ONERROR(err, do_close_udp, (ptrdiff_t) fd);
  apply_current(factory_fun_num, 0);
  if ((TYPEOF(Pike_sp[-1]) != PIKE_T_OBJECT) ||
      !(o = Pike_sp[-1].u.object)->prog ||
      ((inh = &o->prog->inherits[SUBTYPEOF(Pike_sp[-1])])->prog !=
       Pike_fp->context->prog)) {
    Pike_error("Invalid return value from fd_factory(). "
	       "Expected object(is Stdio.UDP).\n");
  }
  udp = (struct udp_storage *)(o->storage + inh->storage_offset);
  if (udp->box.fd != -1) {
    Pike_error("Invalid return value from fd_factory(). "
	       "Expected unopened object(is Stdio.UDP). fd:%d\n",
	       udp->box.fd);
  }
  UNSET_ONERROR(err);
  change_fd_for_box(&udp->box, fd);
#ifdef PIKE_DEBUG
  if (fd >= 0) {
    debug_check_fd_not_in_use (fd);
  }
#endif
}

static int got_udp_event (struct fd_callback_box *box, int event);

static void low_dup_udp(struct udp_storage *to,
			struct udp_storage *from)
{
  my_set_close_on_exec(to->box.fd, to->box.fd > 2);

  unhook_fd_callback_box (&to->box);
  if (from->box.backend)
    INIT_FD_CALLBACK_BOX (&to->box, from->box.backend, to->box.ref_obj,
			  to->box.fd, from->box.events, got_udp_event,
			  from->box.flags);

  assign_svalue (&to->read_callback, &from->read_callback);
  assign_svalue (&to->write_callback, &from->write_callback);
}

/*! @decl Stdio.UDP dup()
 *!
 *!   Duplicate the udp socket.
 *!
 *! @seealso
 *!   @[fd_factory()]
 */
static void udp_dup(INT32 args)
{
  int fd;
  struct object *o;
  struct udp_storage *udp;

  pop_n_elems(args);

  if(FD < 0)
    Pike_error("Not open.\n");

  if((fd=fd_dup(FD)) < 0)
  {
    THIS->my_errno = errno;
    push_int(0);
    return;
  }
  push_new_udp_object(udp_fd_factory_fun_num, fd);
  o = Pike_sp[-1].u.object;
  udp = ((struct udp_storage *)
	 (o->storage + o->prog->inherits[SUBTYPEOF(Pike_sp[-1])].storage_offset));
  THIS->my_errno = 0;
  low_dup_udp(udp, THIS);

  /* Return the top-level object. */
  push_object(o);
}

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

/*! @decl UDP bind(int|string port, string|void address,@
 *!string|int(0..1) no_reuseaddr)
 *!
 *! Binds a port for receiving or transmitting UDP.
 *!
 *! @param port
 *!   Either a port number or the name of a service as listed in
 *!   @tt{/etc/services@}.
 *!
 *! @param address
 *!   Local address to bind to.
 *!
 *! @param no_reuseaddr
 *! 	If set to @expr{1@}, Pike will not set the @expr{SO_REUSEADDR@} option
 *! 	on the UDP port.
 *!
 *! @note
 *! 	@expr{SO_REUSEADDR@} is never applied when binding a random port
 *!	(@expr{bind(0)@}).
 *!
 *! 	In general, @expr{SO_REUSEADDR@} is not desirable on UDP ports.
 *! 	Unless used for receiving multicast, be sure to never bind a
 *! 	non-random port without setting @expr{no_reuseaddr@} to @expr{1@}.
 *!
 *! @throws
 *!   Throws error when unable to bind port.
 */
static void udp_bind(INT32 args)
{
  PIKE_SOCKADDR addr;
  int addr_len;
  int zero=0, one=1;
  int fd, port, tmp;
#if !defined(SOL_IP) && defined(HAVE_GETPROTOBYNAME)
  static int ip_proto_num = -1;
#endif /* !SOL_IP && HAVE_GETPROTOBYNAME */

  if(args < 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("bind", 1);

  if(TYPEOF(Pike_sp[-args]) != PIKE_T_INT &&
     (TYPEOF(Pike_sp[-args]) != PIKE_T_STRING ||
      Pike_sp[-args].u.string->size_shift))
    SIMPLE_ARG_TYPE_ERROR("bind", 1, "int|string(8bit)");

  if(args > 2 && TYPEOF(Pike_sp[2-args]) != PIKE_T_INT)
    SIMPLE_ARG_TYPE_ERROR("bind", 3, "int(0..1)");

#if 0
  f_backtrace(0);
  APPLY_MASTER("handle_error", 1);
  pop_stack();
#endif /* 0 */

  if(FD != -1)
  {
    fd = FD;
    change_fd_for_box (&THIS->box, -1);
    while (fd_close(fd) && errno == EINTR)
      ;
  }

  THIS->inet_flags &= ~PIKE_INET_FLAG_IPV6;

  port = TYPEOF(Pike_sp[-args])==PIKE_T_INT ? Pike_sp[-args].u.integer : -1;
  addr_len = get_inet_addr(&addr, (args > 1 && TYPEOF(Pike_sp[1-args])==PIKE_T_STRING?
				   Pike_sp[1-args].u.string->str : NULL),
			   (TYPEOF(Pike_sp[-args]) == PIKE_T_STRING?
			    Pike_sp[-args].u.string->str : NULL),
                           port,
			   THIS->inet_flags);
  INVALIDATE_CURRENT_TIME();

#ifdef AF_INET6
  if (SOCKADDR_FAMILY(addr) == AF_INET6) {
    THIS->inet_flags |= PIKE_INET_FLAG_IPV6;
  }
#endif

  fd = fd_socket(SOCKADDR_FAMILY(addr), THIS->type, THIS->protocol);
  if(fd < 0)
  {
    pop_n_elems(args);
    THIS->my_errno=errno;
    Pike_error("Failed to create socket (%d, %d, %d)\n",
	       SOCKADDR_FAMILY(addr), THIS->type, THIS->protocol);
  }

  /* Make sure this fd gets closed on exec. */
  set_close_on_exec(fd, 1);

  /* linux kernel commit f24d43c07e208372aa3d3bff419afbf43ba87698 introduces
   * a behaviour change where you can get used random ports if bound with
   * SO_REUSEADDR. */
  if(addr.ipv4.sin_port && (args > 2 ? !Pike_sp[2-args].u.integer : 1)
     && fd_setsockopt(fd, SOL_SOCKET,SO_REUSEADDR,
		      (char *)&one, sizeof(int)) < 0)
  {
    THIS->my_errno=errno;
    while (fd_close(fd) && errno == EINTR)
      ;
    Pike_error("setsockopt SO_REUSEADDR failed\n");
  }

#if defined(IPV6_V6ONLY) && defined(IPPROTO_IPV6)
  if (SOCKADDR_FAMILY(addr) == AF_INET6) {
    /* Attempt to enable dual-stack (ie mapped IPv4 adresses). Needed on WIN32.
     * cf http://msdn.microsoft.com/en-us/library/windows/desktop/bb513665(v=vs.85).aspx
     */
    fd_setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&zero, sizeof(int));
  }
#endif

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
     if(fd_setsockopt(fd, SOL_IP, IP_HDRINCL, (char *)&one, sizeof(int)))
        Pike_error("setsockopt IP_HDRINCL failed\n");
#endif /* IP_HDRINCL */

  THREADS_ALLOW_UID();

  tmp=fd_bind(fd, (struct sockaddr *)&addr, addr_len)<0;

  THREADS_DISALLOW_UID();

  if(tmp)
  {
    THIS->my_errno=errno;
    while (fd_close(fd) && errno == EINTR)
      ;
    Pike_error("Failed to bind to port %d\n", port);
    return;
  }

  if(!Pike_fp->current_object->prog)
  {
    while (fd_close(fd) && errno == EINTR)
      ;
    Pike_error("Object destructed.\n");
  }

  change_fd_for_box (&THIS->box, fd);
  pop_n_elems(args);
  ref_push_object(THISOBJ);
}


/*! @decl UDP set_fd(int fd)
 *!
 *! Use the file descriptor @[fd] for UDP.
 *! @seealso
 *!   @[bind]
 */
static void udp_set_fd(INT32 args)
{
  int fd;

  get_all_args(NULL, args, "%d", &fd);

  if(FD != -1)
  {
    int curfd = FD;
    change_fd_for_box (&THIS->box, -1);
    fd_close(curfd);
  }

  change_fd_for_box (&THIS->box, fd);
  pop_n_elems(args);
  ref_push_object(THISOBJ);
}

/*! @decl int query_fd()
 *!
 *! Gets the file descriptor for this UDP port.
 */
static void udp_query_fd(INT32 args)
{
  pop_n_elems(args);
  push_int(FD);
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
 *!   Set the local device for a multicast socket.
 *!
 *! @param reply_address
 *!   Local address that should appear in the multicast packets.
 *!
 *! See also the Unix man page for setsocketopt IPPROTO_IP IP_MULTICAST_IF
 *! and IPPROTO_IPV6 IPV6_MULTICAST_IF.
 *!
 *! @note
 *!   This function did not support IPv6 in Pike 7.8.
 */
void udp_enable_multicast(INT32 args)
{
  int result;
  char *ip;
  PIKE_SOCKADDR reply;

  get_all_args(NULL, args, "%c", &ip);

  get_inet_addr(&reply, ip, NULL, -1, THIS->inet_flags);
  INVALIDATE_CURRENT_TIME();

#ifdef AF_INET6
  if (THIS->inet_flags & PIKE_INET_FLAG_IPV6) {
    if(SOCKADDR_FAMILY(reply) != AF_INET6)
      Pike_error("Multicast only supported for IPv6.\n");

    result = fd_setsockopt(FD, IPPROTO_IPV6, IPV6_MULTICAST_IF,
			   (char *)&reply.ipv6.sin6_addr,
			   sizeof(reply.ipv6.sin6_addr));
    pop_n_elems(args);
    push_int(result);
    return;
  }
#endif
  if(SOCKADDR_FAMILY(reply) != AF_INET)
    Pike_error("Multicast only supported for IPv4.\n");

  result = fd_setsockopt(FD, IPPROTO_IP, IP_MULTICAST_IF,
			 (char *)&reply.ipv4.sin_addr,
			 sizeof(reply.ipv4.sin_addr));
  pop_n_elems(args);
  push_int(result);
}

/*! @decl int set_multicast_ttl(int ttl)
 *!   Set the time-to-live value of outgoing multicast packets
 *!   for this socket.
 *!
 *! @param ttl
 *!   The number of router hops sent multicast packets should
 *!   survive.
 *!
 *!   It is very important for multicast packets to set the
 *!   smallest TTL possible. The default before calling this
 *!   function is 1 which means that multicast packets don't
 *!   leak from the local network unless the user program
 *!   explicitly requests it.
 *!
 *! See also the Unix man page for setsocketopt IPPROTO_IP IP_MULTICAST_TTL
 *! and IPPROTO_IPV6 IPV6_MULTICAST_HOPS.
 *!
 *! @note
 *!   This function did not support IPv6 in Pike 7.8 and earlier.
 *!
 *! @seealso
 *!   @[add_membership()]
 */
void udp_set_multicast_ttl(INT32 args)
{
  int ttl;
  get_all_args(NULL, args, "%d", &ttl);
  pop_n_elems(args);
#ifdef AF_INET6
  if (THIS->inet_flags & PIKE_INET_FLAG_IPV6) {
    push_int( fd_setsockopt(FD, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
			    (char *)&ttl, sizeof(int)) );
    return;
  }
#endif
  push_int( fd_setsockopt(FD, IPPROTO_IP, IP_MULTICAST_TTL,
			  (char *)&ttl, sizeof(int)) );
}

/*! @decl int add_membership(string group, void|string address)
 *!   Join a multicast group.
 *!
 *! @param group
 *!   @[group] contains the address of the multicast group the
 *!   application wants to join. It must be a valid multicast address.
 *!
 *! @param address
 *!   @[address] is the address of the local interface with which
 *!   the system should join to the multicast group. If not provided
 *!   the system will select an appropriate interface.
 *!
 *! See also the Unix man page for setsocketopt IPPROTO_IP IP_ADD_MEMBERSHIP
 *! and IPPROTO_IPV6 IPV6_JOIN_GROUP.
 *!
 *! @note
 *!   The @[address] parameter is currently not supported for IPv6.
 *!
 *! @note
 *!   This function did not support IPv6 in Pike 7.8 and earlier.
 *!
 *! @seealso
 *!   @[drop_membership()]
 */
void udp_add_membership(INT32 args)
{
  int face=0;
  char *group;
  char *address=0;
  struct ip_mreq sock;
  PIKE_SOCKADDR addr;

  get_all_args(NULL, args, "%c.%c%d", &group, &address, &face);

  get_inet_addr(&addr, group, NULL, -1, THIS->inet_flags);
  INVALIDATE_CURRENT_TIME();

#ifdef AF_INET6
  if (THIS->inet_flags & PIKE_INET_FLAG_IPV6) {
    struct ipv6_mreq sock;

    if (address)
      Pike_error("Interface address is not supported for IPv6.\n");

    /* NB: This sets imr_interface to IN6ADDR_ANY,
     *     and clears imr_ifindex if it exists.
     */
    memset(&sock, 0, sizeof(sock));

    if(SOCKADDR_FAMILY(addr) != AF_INET6)
      Pike_error("Mixing IPv6 and other multicast is not supported.\n");

    sock.ipv6mr_multiaddr = addr.ipv6.sin6_addr;

    pop_n_elems(args);
    push_int( fd_setsockopt(FD, IPPROTO_IPV6, IPV6_JOIN_GROUP,
			    (char *)&sock, sizeof(sock)) );
    return;
  }
#endif /* AF_INET6 */

  /* NB: This sets imr_interface to INADDR_ANY,
   *     and clears imr_ifindex if it exists.
   */
  memset(&sock, 0, sizeof(sock));

  if(SOCKADDR_FAMILY(addr) != AF_INET)
    Pike_error("Mixing IPv6 and IPv4 multicast is not supported.\n");

  sock.imr_multiaddr = addr.ipv4.sin_addr;

  if (address) {
    get_inet_addr(&addr, address, NULL, -1, THIS->inet_flags);
    INVALIDATE_CURRENT_TIME();

    if(SOCKADDR_FAMILY(addr) != AF_INET)
      Pike_error("Mixing IPv6 and IPv4 multicast is not supported.\n");

    sock.imr_interface = addr.ipv4.sin_addr;
  }

  pop_n_elems(args);
  push_int( fd_setsockopt(FD, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			  (char *)&sock, sizeof(sock)) );
}

/*! @decl int drop_membership(string group, void|string address)
 *!   Leave a multicast group.
 *!
 *! @param group
 *!   @[group] contains the address of the multicast group the
 *!   application wants to join. It must be a valid multicast address.
 *!
 *! @param address
 *!   @[address] is the address of the local interface with which
 *!   the system should join to the multicast group. If not provided
 *!   the system will select an appropriate interface.
 *!
 *! See also the Unix man page for setsocketopt IPPROTO_IP IP_DROP_MEMBERSHIP
 *! and IPPROTO_IPV6 IPV6_LEAVE_GROUP.
 *!
 *! @note
 *!   The @[address] parameter is currently not supported for IPv6.
 *!
 *! @note
 *!   This function did not support IPv6 in Pike 7.8 and earlier.
 *!
 *! @seealso
 *!   @[add_membership()]
 */
void udp_drop_membership(INT32 args)
{
  int face=0;
  char *group;
  char *address=0;
  struct ip_mreq sock;
  PIKE_SOCKADDR addr;

  get_all_args(NULL, args, "%c.%c%d", &group, &address, &face);

  get_inet_addr(&addr, group, NULL, -1, THIS->inet_flags);
  INVALIDATE_CURRENT_TIME();

#ifdef AF_INET6
  if (THIS->inet_flags & PIKE_INET_FLAG_IPV6) {
    struct ipv6_mreq sock;

    if (address)
      Pike_error("Interface address is not supported for IPv6.\n");

    /* NB: This sets imr_interface to IN6ADDR_ANY,
     *     and clears imr_ifindex if it exists.
     */
    memset(&sock, 0, sizeof(sock));

    if(SOCKADDR_FAMILY(addr) != AF_INET6)
      Pike_error("Mixing IPv6 and other multicast is not supported.\n");

    sock.ipv6mr_multiaddr = addr.ipv6.sin6_addr;

    pop_n_elems(args);
    push_int( fd_setsockopt(FD, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
			    (char *)&sock, sizeof(sock)) );
    return;
  }
#endif /* AF_INET6 */

  /* NB: This sets imr_interface to INADDR_ANY,
   *     and clears imr_ifindex if it exists.
   */
  memset(&sock, 0, sizeof(sock));

  if(SOCKADDR_FAMILY(addr) != AF_INET)
    Pike_error("Mixing IPv6 and IPv4 multicast is not supported.\n");

  sock.imr_multiaddr = addr.ipv4.sin_addr;

  if (address) {
    get_inet_addr(&addr, address, NULL, -1, THIS->inet_flags);
    INVALIDATE_CURRENT_TIME();

    if(SOCKADDR_FAMILY(addr) != AF_INET)
      Pike_error("Mixing IPv6 and IPv4 multicast is not supported.\n");

    sock.imr_interface = addr.ipv4.sin_addr;
  }

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

  get_all_args(NULL, args, "%F", &timeout);

  if (timeout < 0.0) {
    timeout = 0.0;
  }

  if (fd < 0) {
    Pike_error("Port not bound!\n");
  }

#ifdef HAVE_POLL
  THREADS_ALLOW();

  pollfds->fd = fd;
  pollfds->events = POLLIN;
  pollfds->revents = 0;
  ms = timeout * 1000;

  do {
    res = poll(pollfds, 1, ms);
    e = errno;
  } while (res < 0 && e == EINTR)

  THREADS_DISALLOW();
  if (!res) {
    /* Timeout */
  } else if (res < 0) {
    /* Error */
    Pike_error("poll() failed with errno %d\n", e);
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

  fd_FD_ZERO(&rset);
  fd_FD_SET(fd, &rset);
  tv.tv_sec = (int)timeout;
  tv.tv_usec = (int)((timeout - ((int)timeout)) * 1000000.0);
  res = fd_select(fd+1, &rset, NULL, NULL, &tv);
  e = errno;

  THREADS_DISALLOW();
  if (!res) {
    /* Timeout */
  } else if (res < 0) {
    /* Error */
    Pike_error("select() failed with errno %d\n", e);
  } else {
    /* Success? */
    if (fd_FD_ISSET(fd, &rset)) {
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
 *!   @[set_read_callback()], @[MSG_OOB], @[MSG_PEEK]
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
      Pike_error("Illegal flags argument.\n");
    }
  }
  pop_n_elems(args);
  fd = FD;
  if (FD < 0)
    Pike_error("Not open\n");
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
	    set_fd_callback_events (&THIS->box, 0, 0);
	  Pike_error("Socket closed\n");
#ifdef ESTALE
       case ESTALE:
#endif
       case EIO:
	  if (THIS->box.backend)
	    set_fd_callback_events (&THIS->box, 0, 0);
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
       case EINVAL:
	 if (!(flags & MSG_OOB)) {
	   Pike_error("Socket read failed with EINVAL.\n");
	 }
	 /* FALLTHRU */
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
  push_static_text("data");
  push_string( make_shared_binary_string(buffer, res) );

  push_static_text("ip");
#ifdef fd_inet_ntop
  if (!fd_inet_ntop( SOCKADDR_FAMILY(from), SOCKADDR_IN_ADDR(from),
		     buffer, sizeof(buffer) )) {
    push_static_text("UNSUPPORTED");
  } else {
    /* NOTE: IPv6-mapped IPv4 addresses may only
     *       connect to other IPv4 addresses.
     *
     * Make the Pike-level code believe it has an actual IPv4 address
     * when getting a mapped address (::FFFF:a.b.c.d).
     */
    if ((!strncmp(buffer, "::FFFF:", 7) || !strncmp(buffer, "::ffff:", 7)) &&
	!strchr(buffer + 7, ':')) {
      push_text(buffer+7);
    } else {
      push_text(buffer);
    }
  }
#else
  push_text( inet_ntoa( *SOCKADDR_IN_ADDR(from) ) );
#endif

  push_constant_text("port");
  push_int(ntohs(from.ipv4.sin_port));
  f_aggregate_mapping( 6 );

  if (!(THIS->inet_flags & PIKE_INET_FLAG_NB))
    INVALIDATE_CURRENT_TIME();
}

/*! @decl int send(string to, int|string port, string message)
 *! @decl int send(string to, int|string port, string message, int flags)
 *!
 *! Send data to a UDP socket.
 *!
 *! @param to
 *!   The recipient address. For @[connect()]ed objects specifying a
 *!   recipient of either @[UNDEFINED] or @expr{""@} causes the default
 *!   recipient to be used.
 *!
 *! @param port
 *!   The recipient port number. For @[connect()]ed objects specifying
 *!   port number @expr{0@} casues the default recipient port to be used.
 *!
 *! @param flag
 *!   A flag bitfield with @expr{1@} for out of band data and
 *!   @expr{2@} for don't route flag.
 *!
 *! @returns
 *!   @int
 *!     @value 0..
 *!       The number of bytes that were actually written.
 *!     @value ..-1
 *!       Failed to send the @[message]. Check @[errno()] for
 *!       the cause. Common causes are:
 *!       @int
 *!         @value System.EMSGSIZE
 *!           The @[message] is too large to send unfragmented.
 *!         @value System.EWOULDBLOCK
 *!           The send buffers are full.
 *!       @endint
 *!  @endint
 *!
 *! @throws
 *!   Throws errors on invalid arguments and uninitialized object.
 *!
 *! @note
 *!   Versions of Pike prior to 8.1.5 threw errors also on EMSGSIZE
 *!   (@expr{"Too big message"@}) and EWOULDBLOCK
 *!   (@expr{"Message would block."@}). These versions of Pike also
 *!   did not update the object errno on this function failing.
 *!
 *! @note
 *!   Versions of Pike prior to 8.1.13 did not support the default
 *!   recipient for @[connect()]ed objects.
 *!
 *! @seealso
 *!   @[connect()], @[errno()], @[query_mtu()]
 */
void udp_sendto(INT32 args)
{
  int flags = 0, fd, e;
  ptrdiff_t res = 0;
  PIKE_SOCKADDR to_buf;
  struct sockaddr *to = NULL;
  int to_len = 0;
  char *str;
  ptrdiff_t len;

  if(FD < 0)
    Pike_error("UDP: not open\n");

  check_all_args(NULL, args,
		 BIT_ZERO|BIT_STRING, BIT_INT|BIT_STRING,
		 BIT_STRING, BIT_INT|BIT_VOID, 0);

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
      Pike_error("Illegal flags argument.\n");
    }
  }

  if ((TYPEOF(Pike_sp[-args]) == PIKE_T_STRING) &&
      Pike_sp[-args].u.string->len) {
    to_len = get_inet_addr(&to_buf, Pike_sp[-args].u.string->str,
			   (TYPEOF(Pike_sp[1-args]) == PIKE_T_STRING?
			    Pike_sp[1-args].u.string->str : NULL),
			   (TYPEOF(Pike_sp[1-args]) == PIKE_T_INT?
			    Pike_sp[1-args].u.integer : -1),
			   THIS->inet_flags);
    to = (struct sockaddr *)&to_buf;
  }

  INVALIDATE_CURRENT_TIME();

  fd = FD;
  str = Pike_sp[2-args].u.string->str;
  len = Pike_sp[2-args].u.string->len;

  do {
    THREADS_ALLOW();
    res = fd_sendto( fd, str, len, flags, to, to_len);
    e = errno;
    THREADS_DISALLOW();

    check_threads_etc();
  } while((res == -1) && e==EINTR);

  if(res<0)
  {
    THIS->my_errno = e;
    switch(e)
    {
       case EBADF:
	  if (THIS->box.backend)
	    set_fd_callback_events (&THIS->box, 0, 0);
	  Pike_error("Socket closed\n");
       case ENOMEM:
#ifdef ENOSR
       case ENOSR:
#endif /* ENOSR */
	  Pike_error("Out of memory\n");
       case EINVAL:
#ifdef ENOTSOCK
       case ENOTSOCK:
#endif
	  if (THIS->box.backend)
	    set_fd_callback_events (&THIS->box, 0, 0);
	  Pike_error("Not a socket!!!\n");
    }
  }
  pop_n_elems(args);
  push_int64(res);
  if (!(THIS->inet_flags & PIKE_INET_FLAG_NB))
    INVALIDATE_CURRENT_TIME();
}


static int got_udp_event (struct fd_callback_box *box, int event)
{
  struct udp_storage *u = (struct udp_storage *) box;
#ifdef PIKE_DEBUG
  if (event != PIKE_FD_READ)
#endif

  u->my_errno = errno;		/* Propagate backend setting. */

  switch(event) {
  case PIKE_FD_READ:
    check_destructed (&u->read_callback);
    if (UNSAFE_IS_ZERO (&u->read_callback))
      set_fd_callback_events (&u->box, u->box.events & ~PIKE_BIT_FD_READ, 0);
    else {
      apply_svalue (&u->read_callback, 0);
      if (TYPEOF(Pike_sp[-1]) == PIKE_T_INT && Pike_sp[-1].u.integer == -1) {
	pop_stack();
	return -1;
      }
      pop_stack();
    }
    break;
  case PIKE_FD_WRITE:
    check_destructed (&u->write_callback);
    if (UNSAFE_IS_ZERO (&u->write_callback))
      set_fd_callback_events (&u->box, u->box.events & ~PIKE_BIT_FD_WRITE, 0);
    else {
      apply_svalue (&u->write_callback, 0);
      if (TYPEOF(Pike_sp[-1]) == PIKE_T_INT && Pike_sp[-1].u.integer == -1) {
	pop_stack();
	return -1;
      }
      pop_stack();
    }
    break;
#ifdef PIKE_DEBUG
  default:
    Pike_fatal ("Got unexpected event %d.\n", event);
#endif
  }
  return 0;
}

void zero_udp(struct object *o)
{
  INIT_FD_CALLBACK_BOX(&THIS->box, NULL, o, -1, 0, got_udp_event, 0);
  THIS->my_errno = 0;
  THIS->inet_flags = PIKE_INET_FLAG_UDP;
  THIS->type=SOCK_DGRAM;
  THIS->protocol=0;
  /* map_variable handles read_callback and write_callback. */
}

int low_exit_udp()
{
  int fd = FD;
  int ret = 0;

  unhook_fd_callback_box(&THIS->box);
  FD = -1;

  if(fd != -1)
  {
    THREADS_ALLOW();
    ret = fd_close(fd);
    THREADS_DISALLOW();
  }

  /* map_variable handles read_callback and write_callback. */

  return ret;
}

static void udp_set_read_callback(INT32 args)
{
  struct udp_storage *u = THIS;
  if(args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("set_read_callback", 1);

  assign_svalue(& u->read_callback, Pike_sp-1);
  if (UNSAFE_IS_ZERO (Pike_sp - 1)) {
    if (u->box.backend)
      set_fd_callback_events (&u->box, u->box.events & ~PIKE_BIT_FD_READ, 0);
  }
  else {
    if (!u->box.backend)
      INIT_FD_CALLBACK_BOX (&u->box, default_backend, u->box.ref_obj,
			    u->box.fd, u->box.events | PIKE_BIT_FD_READ,
			    got_udp_event, 0);
    else
      set_fd_callback_events (&u->box, u->box.events | PIKE_BIT_FD_READ, 0);
  }

  pop_n_elems(args);
  ref_push_object(THISOBJ);
}

static void udp_set_write_callback(INT32 args)
{
  struct udp_storage *u = THIS;
  if(args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("set_write_callback", 1);

  assign_svalue(& u->write_callback, Pike_sp-1);
  if (UNSAFE_IS_ZERO (Pike_sp - 1)) {
    if (u->box.backend)
      set_fd_callback_events (&u->box, u->box.events & ~PIKE_BIT_FD_WRITE, 0);
  }
  else {
    if (!u->box.backend)
      INIT_FD_CALLBACK_BOX (&u->box, default_backend, u->box.ref_obj,
			    u->box.fd, u->box.events | PIKE_BIT_FD_WRITE,
			    got_udp_event, 0);
    else
      set_fd_callback_events (&u->box, u->box.events | PIKE_BIT_FD_WRITE, 0);
  }

  pop_n_elems(args);
  ref_push_object(THISOBJ);
}

/*! @decl object set_nonblocking(function|void rcb, function|void wcb)
 *!
 *! Sets this object to be nonblocking and the read and write callbacks.
 */
static void udp_set_nonblocking(INT32 args)
{
  if (FD < 0) Pike_error("File not open.\n");

  while (args < 2) {
    push_undefined();
    args++;
  }

  udp_set_write_callback(args - 1);
  pop_stack();

  udp_set_read_callback(1);
  pop_stack();

  set_nonblocking(FD,1);
  THIS->inet_flags |= PIKE_INET_FLAG_NB;
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
  THIS->inet_flags &= ~PIKE_INET_FLAG_NB;
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

  get_all_args(NULL, args, "%n%*", &dest_addr, &dest_port);

  if(TYPEOF(*dest_port) != PIKE_T_INT &&
     (TYPEOF(*dest_port) != PIKE_T_STRING || dest_port->u.string->size_shift))
    SIMPLE_ARG_TYPE_ERROR("connect", 2, "int|string(8bit)");

  addr_len =  get_inet_addr(&addr, dest_addr->str,
			    (TYPEOF(*dest_port) == PIKE_T_STRING?
			     dest_port->u.string->str : NULL),
			    (TYPEOF(*dest_port) == PIKE_T_INT?
			     dest_port->u.integer : -1),
			    THIS->inet_flags);
  INVALIDATE_CURRENT_TIME();

  if(FD < 0)
  {
     FD = fd_socket(SOCKADDR_FAMILY(addr), SOCK_DGRAM, 0);
     if(FD < 0)
     {
	THIS->my_errno=errno;
        Pike_error("Failed to create socket\n");
     }
     set_close_on_exec(FD, 1);

#if defined(IPV6_V6ONLY) && defined(IPPROTO_IPV6)
     if (SOCKADDR_FAMILY(addr) == AF_INET6) {
       /* Attempt to enable dual-stack (ie mapped IPv4 adresses).
	* Needed on WIN32.
	* cf http://msdn.microsoft.com/en-us/library/windows/desktop/bb513665(v=vs.85).aspx
	*/
       int o = 0;
       fd_setsockopt(FD, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&o, sizeof(int));
     }
#endif
  }

  tmp=FD;
  THREADS_ALLOW();
  tmp=fd_connect(tmp, (struct sockaddr *)&addr, addr_len);
  THREADS_DISALLOW();

  if (!(THIS->inet_flags & PIKE_INET_FLAG_NB))
    INVALIDATE_CURRENT_TIME();

  if(tmp < 0)
  {
    THIS->my_errno=errno;
    Pike_error("Failed to connect\n");
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
  char buffer[496];
  ACCEPT_SIZE_T len;

  if(fd <0)
    Pike_error("Port not bound yet.\n");

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

#ifdef fd_inet_ntop
  fd_inet_ntop(SOCKADDR_FAMILY(addr), SOCKADDR_IN_ADDR(addr),
	       buffer, sizeof(buffer)-20);
#else
  q=inet_ntoa(*SOCKADDR_IN_ADDR(addr));
  strncpy(buffer,q,sizeof(buffer)-20);
  buffer[sizeof(buffer)-20]=0;
#endif
  sprintf(buffer+strlen(buffer)," %d",(int)(ntohs(addr.ipv4.sin_port)));

  /* NOTE: IPv6-mapped IPv4 addresses may only connect to other IPv4 addresses.
   *
   * Make the Pike-level code believe it has an actual IPv4 address
   * when getting a mapped address (::FFFF:a.b.c.d).
   */
  if ((!strncmp(buffer, "::FFFF:", 7) || !strncmp(buffer, "::ffff:", 7)) &&
      !strchr(buffer + 7, ':')) {
    push_text(buffer+7);
  } else {
    push_text(buffer);
  }
}

#ifdef IP_MTU
/*! @decl int query_mtu()
 *!
 *! Get the Max Transfer Unit for the object (if any).
 *!
 *! @returns
 *!   @int
 *!     @value -1
 *!       Returns @expr{-1@} if the object is not a socket or
 *!       if the mtu is unknown.
 *!     @value 1..
 *!       Returns a positive value with the mtu on success.
 *!   @endint
 *!
 *! @note
 *!   The returned value is adjusted to take account for the
 *!   IP and UDP headers, so that it should be usable without
 *!   further adjustment unless further IP options are in use.
 */
static void udp_query_mtu(INT32 args)
{
  int mtu = -1;
  PIKE_SOCKADDR addr;
  ACCEPT_SIZE_T len = sizeof(addr);
  int level = SOL_SOCKET;
  int option = IP_MTU;
  int ip_udp_header_size = 20 + 8;	/* IPv4 and UDP header sizes. */

  if(FD <0)
    Pike_error("Port not bound yet.\n");

  if (fd_getsockname(FD, (struct sockaddr *)&addr, &len) < 0) {
    THIS->my_errno = errno;
    push_int(-1);
    return;
  }

  if (SOCKADDR_FAMILY(addr) == AF_INET) {
    level = IPPROTO_IP;
#ifdef IPV6_MTU
  } else if (SOCKADDR_FAMILY(addr) == AF_INET6) {
    level = IPPROTO_IPV6;
    option = IPV6_MTU;
    ip_udp_header_size = 40 + 8;	/* IPv6 and UDP header sizes. */
#endif
  }

  len = sizeof(mtu);
  if (fd_getsockopt(FD, level, option, (void *)&mtu, &len) < 0) {
    THIS->my_errno = errno;
    push_int(-1);
    return;
  }
  push_int(mtu - ip_udp_header_size);
}
#endif /* IP_MTU */

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

  if (args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("set_backend", 1);
  if (TYPEOF(Pike_sp[-args]) != PIKE_T_OBJECT)
    SIMPLE_ARG_TYPE_ERROR ("set_backend", 1, "Pike.Backend");
  backend = get_storage (Pike_sp[-args].u.object, Backend_program);
  if (!backend)
    SIMPLE_ARG_TYPE_ERROR ("set_backend", 1, "Pike.Backend");

  if (u->box.backend)
    change_backend_for_box (&u->box, backend);
  else
    INIT_FD_CALLBACK_BOX (&u->box, backend, u->box.ref_obj,
			  u->box.fd, 0, got_udp_event, 0);
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
   int type, proto = 0;

   get_all_args(NULL,args,"%d.%d",&type,&proto);

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

/*! @decl void set_buffer(int bufsize, string mode)
 *! @decl void set_buffer(int bufsize)
 *!
 *! Set internal socket buffer.
 *!
 *! This function sets the internal buffer size of a socket or stream.
 *!
 *! The second argument allows you to set the read or write buffer by
 *! specifying @expr{"r"@} or @expr{"w"@}.
 *!
 *! @note
 *!   It is not guaranteed that this function actually does anything,
 *!   but it certainly helps to increase data transfer speed when it does.
 *!
 *! @seealso
 *!   @[open_socket()], @[accept()]
 */
static void udp_set_buffer(INT32 args)
{
  INT_TYPE bufsize;
  int flags = FILE_READ | FILE_WRITE;
  char *c = NULL;

  if(FD==-1)
    Pike_error("Port is closed.\n");

  get_all_args(NULL, args, "%+.%c", &bufsize, &c);

  if(bufsize < 0)
    Pike_error("Bufsize must be larger than zero.\n");

  if(c)
  {
    flags = 0;
    do
    {
      switch( *c )
      {
      case 'w': flags |= FILE_WRITE; break;
      case 'r': flags |= FILE_READ;  break;
      }
    } while( *c++ );
  }

#ifdef SOCKET_BUFFER_MAX
#if SOCKET_BUFFER_MAX
  if(bufsize>SOCKET_BUFFER_MAX) bufsize=SOCKET_BUFFER_MAX;
#endif
#ifdef SO_RCVBUF
  if(flags & FILE_READ)
  {
    int tmp=bufsize;
    fd_setsockopt(FD,SOL_SOCKET, SO_RCVBUF, (char *)&tmp, sizeof(tmp));
  }
#endif /* SO_RCVBUF */

#ifdef SO_SNDBUF
  if(flags & FILE_WRITE)
  {
    int tmp=bufsize;
    fd_setsockopt(FD,SOL_SOCKET, SO_SNDBUF, (char *)&tmp, sizeof(tmp));
  }
#endif /* SO_SNDBUF */
#endif
}

/*! @decl constant MSG_OOB
 *!
 *! Flag to specify to @[read()] to read out of band packets.
 */

/*! @decl constant MSG_PEEK
 *!
 *! Flag to specify to @[read()] to cause it to not remove the packet
 *! from the input buffer.
 */

/*! @endclass
 */

/*! @module SOCK
 *!
 *! Module containing constants for specifying socket options.
 */

/*! @decl constant STREAM
 */

/*! @decl constant DGRAM
 */

/*! @decl constant SEQPACKET
 */

/*! @decl constant RAW
 */

/*! @decl constant RDM
 */

/*! @decl constant PACKET
 */

/*! @endmodule
 */

/*! @module IPPROTO
 *!
 *! Module containing various IP-protocol options.
 */

/*! @endmodule
 */

/*! @endmodule
 */

void init_stdio_udp(void)
{
  START_NEW_PROGRAM_ID (STDIO_UDP);

  ADD_STORAGE(struct udp_storage);

  PIKE_MAP_VARIABLE("_read_callback", OFFSETOF(udp_storage, read_callback),
		    tFunc(tNone, tInt_10), PIKE_T_MIXED, ID_PROTECTED|ID_PRIVATE);

  PIKE_MAP_VARIABLE("_write_callback", OFFSETOF(udp_storage, write_callback),
		    tFunc(tNone, tInt_10), PIKE_T_MIXED, ID_PROTECTED|ID_PRIVATE);

  udp_fd_factory_fun_num =
    ADD_FUNCTION("fd_factory", udp_fd_factory,
		 tFunc(tNone, tObjIs_STDIO_UDP), ID_PROTECTED);

  ADD_FUNCTION("`_fd", udp_backtick__fd, tFunc(tNone, tObjIs_STDIO_UDP), 0);

  ADD_FUNCTION("set_type",udp_set_type,
	       tFunc(tInt tOr(tVoid,tInt),tObj),0);
  ADD_FUNCTION("get_type",udp_get_type,
	       tFunc(tNone,tArr(tInt)),0);

  ADD_FUNCTION("close", udp_close, tFunc(tNone, tInt01), 0);

  ADD_FUNCTION("bind",udp_bind,
	       tFunc(tOr(tInt,tStr) tOr(tVoid,tStr) tOr(tVoid,tInt),tObj),0);

  ADD_FUNCTION("set_fd",udp_set_fd,
         tFunc(tInt,tObj),0);

  ADD_FUNCTION("query_fd",udp_query_fd,
         tFunc(tVoid,tInt),0);

  ADD_FUNCTION("dup", udp_dup, tFunc(tNone, tObjImpl_STDIO_UDP), 0);

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
	       tFunc(tOr(tFunc(tVoid, tInt_10), tVoid)
		     tOr(tFunc(tVoid, tInt_10), tVoid), tObj), 0 );
  ADD_FUNCTION("_set_read_callback", udp_set_read_callback,
	       tFunc(tFunc(tVoid, tInt_10), tObj), 0 );
  ADD_FUNCTION("_set_write_callback", udp_set_write_callback,
	       tFunc(tFunc(tVoid, tInt_10), tObj), 0 );

  ADD_FUNCTION("set_blocking",udp_set_blocking,tFunc(tVoid,tObj), 0 );
  ADD_FUNCTION("query_address",udp_query_address,tFunc(tNone,tStr),0);
#ifdef IP_MTU
  ADD_FUNCTION("query_mtu", udp_query_mtu, tFunc(tNone,tInt), 0);
#endif

  ADD_FUNCTION ("set_backend", udp_set_backend, tFunc(tObj,tVoid), 0);
  ADD_FUNCTION ("query_backend", udp_query_backend, tFunc(tVoid,tObj), 0);

  ADD_FUNCTION("errno",udp_errno,tFunc(tNone,tInt),0);

  ADD_FUNCTION("set_buffer", udp_set_buffer, tFunc(tInt tOr(tStr,tVoid),tVoid), 0);

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

void exit_stdio_udp(void)
{
}
