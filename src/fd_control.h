#ifndef FD_CONTROL_H
#define FD_CONTROL_H

/* Prototypes begin here */
void set_nonblocking(int fd,int which);
int query_nonblocking(int fd);
int set_close_on_exec(int fd, int which);
/* Prototypes end here */

#endif
