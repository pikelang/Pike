/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pike_netlib.h,v 1.2 2003/04/23 23:34:24 marcus Exp $
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

typedef union {
  struct sockaddr sa;
  struct sockaddr_in ipv4;
#ifdef HAVE_STRUCT_SOCKADDR_IN6
  struct sockaddr_in6 ipv6;
#endif
} SOCKADDR;

#define SOCKADDR_FAMILY(X) ((X).sa.sa_family)

#ifdef HAVE_STRUCT_SOCKADDR_IN6
#define SOCKADDR_IN_ADDR(X) (SOCKADDR_FAMILY(X)==AF_INET? \
  &(X).ipv4.sin_addr : (struct in_addr *)&(X).ipv6.sin6_addr)
#else
#define SOCKADDR_IN_ADDR(X) (&(X).ipv4.sin_addr)
#endif

#endif /* PIKE_NETLIB_H */
