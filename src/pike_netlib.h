/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef PIKE_NETLIB_H
#define PIKE_NETLIB_H

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#ifdef HAVE_WINSOCK2_H
#include <WinSock2.h>
#ifdef HAVE_WS2TCPIP_H
/* Needed for IPv6 support. */
#include <WS2tcpip.h>
#endif
#elif defined(HAVE_WINSOCK_H)
#include <winsock.h>
#endif

typedef union {
  struct sockaddr sa;
  struct sockaddr_in ipv4;
#ifdef HAVE_STRUCT_SOCKADDR_IN6
  struct sockaddr_in6 ipv6;
#endif
} PIKE_SOCKADDR;

#define SOCKADDR_FAMILY(X) ((X).sa.sa_family)

#ifdef HAVE_STRUCT_SOCKADDR_IN6
#define SOCKADDR_IN_ADDR(X) (SOCKADDR_FAMILY(X)==AF_INET? \
  &(X).ipv4.sin_addr : (struct in_addr *)&(X).ipv6.sin6_addr)
#else
#define SOCKADDR_IN_ADDR(X) (&(X).ipv4.sin_addr)
#endif

#endif /* PIKE_NETLIB_H */
