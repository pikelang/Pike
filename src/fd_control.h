/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: fd_control.h,v 1.5 2000/05/20 18:58:29 grubba Exp $
 */
#ifndef FD_CONTROL_H
#define FD_CONTROL_H

/* Prototypes begin here */
int set_nonblocking(int fd,int which);
int query_nonblocking(int fd);
int set_close_on_exec(int fd, int which);

#ifdef HAVE_BROKEN_F_SETFD
void do_close_on_exec(void);
void cleanup_close_on_exec(void);
#endif /* HAVE_BROKEN_F_SETFD */
/* Prototypes end here */

#endif
