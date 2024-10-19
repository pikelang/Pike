/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef SIGNAL_H
#define SIGNAL_H

typedef RETSIGTYPE (*sigfunctype) (int);

/* Prototypes begin here */
struct sigdesc;
void my_signal(int sig, sigfunctype fun);
PMOD_EXPORT void check_signals(struct callback *foo, void *bar, void *gazonk);
void set_default_signal_handler(int signum, void (*func)(INT32));
#ifdef __NT__
struct pid_status *pid_status_unlink_pty(struct pid_status *pid);
int check_pty_clients(struct my_pty *pty);
#endif
void process_started(pid_t pid);
void process_done(pid_t pid, const char *from);
struct wait_data;
struct pid_status;
struct perishables;
struct plimit;
struct perishables;
PMOD_EXPORT void restore_signal_handler(int sig);
PMOD_EXPORT void low_init_signals(void);
void f_set_priority( INT32 args );
void f_create_process(INT32 args);
void Pike_f_fork(INT32 args);
void f_atexit(INT32 args);
void init_signals(void);
void exit_signals(void);
/* Prototypes end here */


#endif
