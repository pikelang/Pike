/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef SIGNAL_H
#define SIGNAL_H

typedef RETSIGTYPE (*sigfunctype) (int);

/* Prototypes begin here */
struct wait_data;
struct sigdesc;
void my_signal(int sig, sigfunctype fun);
struct pid_status;
void f_create_process(INT32 args);
void f_fork(INT32 args);
void check_signals(struct callback *foo, void *bar, void *gazonk);
void init_signals(void);
void exit_signals(void);
/* Prototypes end here */

#endif
