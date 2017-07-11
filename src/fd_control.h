/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef FD_CONTROL_H
#define FD_CONTROL_H

#ifndef HAVE_ACCEPT4
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
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
int accept4(int fd, struct sockaddr *addr, ACCEPT_SIZE_T *addrlen, int flags);
#endif /* !HAVE_ACCEPT4 */
/* Prototypes end here */

#endif
