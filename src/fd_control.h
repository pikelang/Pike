/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef FD_CONTROL_H
#define FD_CONTROL_H

/* Prototypes begin here */
void set_nonblocking(int fd,int which);
int query_nonblocking(int fd);
int set_close_on_exec(int fd, int which);
/* Prototypes end here */

#endif
