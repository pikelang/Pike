/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: fd_control.h,v 1.6 2000/12/16 05:24:40 marcus Exp $
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
