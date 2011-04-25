/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef FD_CONTROL_H
#define FD_CONTROL_H

/* Prototypes begin here */
PMOD_EXPORT int set_nonblocking(int fd,int which);
PMOD_EXPORT int query_nonblocking(int fd);
PMOD_EXPORT int set_close_on_exec(int fd, int which);

#ifdef HAVE_BROKEN_F_SETFD
void do_close_on_exec(void);
void cleanup_close_on_exec(void);
#endif /* HAVE_BROKEN_F_SETFD */
/* Prototypes end here */

#endif
