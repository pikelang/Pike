/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: signal_handler.h,v 1.10 2000/03/22 00:56:14 hubbe Exp $
 */
#ifndef SIGNAL_H
#define SIGNAL_H

typedef RETSIGTYPE (*sigfunctype) (int);

/* Prototypes begin here */
struct sigdesc;
void my_signal(int sig, sigfunctype fun);
void check_signals(struct callback *foo, void *bar, void *gazonk);
void set_default_signal_handler(int signum, void (*func)(INT32));
void process_started(pid_t pid);
void process_done(pid_t pid, char *from);
struct wait_data;
struct pid_status;
struct perishables;
struct plimit;
struct perishables;
void f_set_priority( INT32 args );
void f_create_process(INT32 args);
void Pike_f_fork(INT32 args);
void f_atexit(INT32 args);
void init_signals(void);
void exit_signals(void);
/* Prototypes end here */


#endif
