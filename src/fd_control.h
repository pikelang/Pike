/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: fd_control.h,v 1.3 1998/03/28 15:30:13 grubba Exp $
 */
#ifndef FD_CONTROL_H
#define FD_CONTROL_H

/* Prototypes begin here */
int set_nonblocking(int fd,int which);
int query_nonblocking(int fd);
int set_close_on_exec(int fd, int which);
/* Prototypes end here */

#endif
