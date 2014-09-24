/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef FD_CONTROL_H
#define FD_CONTROL_H

#ifndef HAVE_ACCEPT4
/* NB: The default values are compatible with Linux,
 *     but should work on other OSs as well, since
 *     accept4(2) is emulated by us anyway.
 */
enum pike_sock_flags {
#ifndef SOCK_CLOEXEC
#if !defined(SOCK_NONBLOCK) || (SOCK_NONBLOCK != 0x80000)
  SOCK_CLOEXEC = 0x80000,
#else
  /* Unlikely, but... */
  SOCK_CLOEXEC = 0x40000,
#endif /* !SOCK_NONBLOCK || SOCK_NONBLOCK != 0x80000 */
#define SOCK_CLOEXEC	SOCK_CLOEXEC
#endif
#ifndef SOCK_NONBLOCK
#if (SOCK_CLOEXEC != 0x00800)
  SOCK_NONBLOCK = 0x00800,
#else
  /* Unlikely, but... */
  SOCK_NONBLOCK = 0x00400,
#endif
#define SOCK_NONBLOCK	SOCK_NONBLOCK
#endif
};
#endif /* !HAVE_ACCEPT4 */

/* Prototypes begin here */
PMOD_EXPORT int set_nonblocking(int fd,int which);
PMOD_EXPORT int query_nonblocking(int fd);
PMOD_EXPORT int set_close_on_exec(int fd, int which);

#ifdef HAVE_BROKEN_F_SETFD
void do_close_on_exec(void);
void cleanup_close_on_exec(void);
#endif /* HAVE_BROKEN_F_SETFD */
#ifndef HAVE_ACCEPT4
int accept4(int fd, struct sockaddr *addr, ACCEPT_SIZE_T *addrlen, int flags)
#endif /* !HAVE_ACCEPT4 */
/* Prototypes end here */

#endif
