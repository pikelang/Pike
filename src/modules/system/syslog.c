/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: syslog.c,v 1.25 2004/05/19 11:45:13 nilsson Exp $
*/

/*
 * Access to syslog from Pike.
 *
 * Henrik Grubbström 1997-01-28
 */

/*
 * Includes
 */

#include "global.h"

#include "system_machine.h"
#include "system.h"

#ifdef HAVE_SYSLOG

RCSID("$Id: syslog.c,v 1.25 2004/05/19 11:45:13 nilsson Exp $");

#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "threads.h"
#include "module_support.h"
#include "builtin_functions.h"
#include "pike_security.h"

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#elif defined(HAVE_SYS_SYSLOG_H)
#include <sys/syslog.h>
#endif

#ifndef LOG_PID
#define LOG_PID 0
#endif
#ifndef LOG_AUTH
#define LOG_AUTH 0
#endif
#ifndef LOG_AUTHPRIV
#define LOG_AUTHPRIV 0
#endif
#ifndef LOG_CRON
#define LOG_CRON 0
#endif
#ifndef LOG_DAEMON
#define LOG_DAEMON 0
#endif
#ifndef LOG_KERN
#define LOG_KERN 0
#endif
#ifndef LOG_LOCAL0
#define LOG_LOCAL0 0
#endif
#ifndef LOG_LOCAL1
#define LOG_LOCAL1 0
#endif
#ifndef LOG_LOCAL2
#define LOG_LOCAL2 0
#endif
#ifndef LOG_LOCAL3
#define LOG_LOCAL3 0
#endif
#ifndef LOG_LOCAL4
#define LOG_LOCAL4 0
#endif
#ifndef LOG_LOCAL5
#define LOG_LOCAL5 0
#endif
#ifndef LOG_LOCAL6
#define LOG_LOCAL6 0
#endif
#ifndef LOG_LOCAL7
#define LOG_LOCAL7 0
#endif
#ifndef LOG_LPR
#define LOG_LPR 0
#endif
#ifndef LOG_MAIL
#define LOG_MAIL 0
#endif
#ifndef LOG_NEWS
#define LOG_NEWS 0
#endif
#ifndef LOG_SYSLOG
#define LOG_SYSLOG 0
#endif
#ifndef LOG_USER
#define LOG_USER 0
#endif
#ifndef LOG_UUCP
#define LOG_UUCP 0
#endif
#ifndef LOG_CONS
#define LOG_CONS 0
#endif
#ifndef LOG_NDELAY
#define LOG_NDELAY 0
#endif
#ifndef LOG_PERROR
#define LOG_PERROR 0
#endif

/*! @module System
 */

/*! @decl void openlog(string ident, int options, facility)
 *!
 *! Initializes the connection to syslogd.
 *!
 *! @param ident.
 *! The @[ident] argument specifies an identifier to tag all logentries
 *! with.
 *!
 *! @param options
 *! A bitfield specifying the behaviour of the message
 *! logging. Valid options are:
 *!
 *! @int
 *!   @value LOG_PID
 *!     Log the process ID with each message.
 *!   @value LOG_CONS
 *!     Write messages to the console if they can't be sent to syslogd.
 *!   @value LOG_NDELAY
 *!     Open the connection to syslogd now and not later.
 *!   @value LOG_NOWAIT
 *!     Do not wait for subprocesses talking to syslogd.
 *! @endint
 *!
 *! @param facility
 *! Specifies what subsystem you want to log as. Valid
 *! facilities are:
 *!
 *! @int
 *!   @value LOG_AUTH
 *!     Authorization subsystem
 *!   @value LOG_AUTHPRIV
 *!
 *!   @value LOG_CRON
 *!     Crontab subsystem
 *!   @value LOG_DAEMON
 *!     System daemons
 *!   @value LOG_KERN
 *!     Kernel subsystem (NOT USABLE)
 *!   @value LOG_LOCAL
 *!   @value LOG_LOCAL1
 *!   @value LOG_LOCAL2
 *!   @value LOG_LOCAL3
 *!   @value LOG_LOCAL4
 *!   @value LOG_LOCAL5
 *!   @value LOG_LOCAL6
 *!   @value LOG_LOCAL7
 *!     For local use
 *!   @value LOG_LPR
 *!     Line printer spooling system
 *!   @value LOG_MAIL
 *!     Mail subsystem
 *!   @value LOG_NEWS
 *!     Network news subsystem
 *!   @value LOG_SYSLOG
 *!
 *!   @value LOG_USER
 *!
 *!   @value LOG_UUCP
 *!     UUCP subsystem
 *! @endint
 *!
 *! @note
 *! Only available on systems with syslog(3).
 *!
 *! @bugs
 *! LOG_NOWAIT should probably always be specified.
 *!
 *! @seealso
 *!   @[syslog], @[closelog]
 */
void f_openlog(INT32 args)
{
  char *ident;
  INT_TYPE p_option, p_facility;
  INT_TYPE option=0, facility=0;

  ASSERT_SECURITY_ROOT("System.openlog");

  get_all_args("openlog", args, "%s%i%i", &ident, &p_option, &p_facility);

  if(p_option & (1<<0)) option |= LOG_CONS;
  if(p_option & (1<<1)) option |= LOG_NDELAY;
  if(p_option & (1<<2)) option |= LOG_PERROR;
  if(p_option & (1<<3)) option |= LOG_PID;
 
  if(p_facility & (1<<0)) facility |= LOG_AUTH; /* Don't use */
  if(p_facility & (1<<1)) facility |= LOG_AUTHPRIV;
  if(p_facility & (1<<2)) facility |= LOG_CRON;
  if(p_facility & (1<<3)) facility |= LOG_DAEMON;
  if(p_facility & (1<<4)) facility |= LOG_KERN;
  if(p_facility & (1<<5)) facility |= LOG_LOCAL0;
  if(p_facility & (1<<6)) facility |= LOG_LOCAL1;
  if(p_facility & (1<<7)) facility |= LOG_LOCAL2;
  if(p_facility & (1<<8)) facility |= LOG_LOCAL3;
  if(p_facility & (1<<9)) facility |= LOG_LOCAL4;
  if(p_facility & (1<<10)) facility |= LOG_LOCAL5;
  if(p_facility & (1<<11)) facility |= LOG_LOCAL6;
  if(p_facility & (1<<12)) facility |= LOG_LOCAL7;
  if(p_facility & (1<<13)) facility |= LOG_LPR;
  if(p_facility & (1<<14)) facility |= LOG_MAIL;
  if(p_facility & (1<<15)) facility |= LOG_NEWS;
  if(p_facility & (1<<16)) facility |= LOG_SYSLOG;
  if(p_facility & (1<<17)) facility |= LOG_USER;
  if(p_facility & (1<<18)) facility |= LOG_UUCP;

#ifdef LOG_NOWAIT
  /* Don't let syslog wait for forked processes */
  option |= LOG_NOWAIT;
#endif /* LOG_NOWAIT */

  THREADS_ALLOW();
  
  openlog(ident, option, facility);

  THREADS_DISALLOW();

  pop_n_elems(args);
}
 
/*! @decl void syslog(int priority, string msg)
 *!
 *! Writes the message @[msg] to the log with the priorities in
 *! @[priority].
 *!
 *! @param priority
 *!   Priority is a bit vector with the wanted priorities or:ed
 *!   together.
 *!   @int
 *!     @value 0
 *!       LOG_EMERG, system is unusable.
 *!     @value 1
 *!       LOG_ALERT, action must be taken immediately.
 *!     @value 2
 *!       LOG_CRIT, critical conditions.
 *!     @value 3
 *!       LOG_ERR, error conditions.
 *!     @value 4
 *!       LOG_WARNING, warnind conditions.
 *!     @value 5
 *!       LOG_NOTICE, normal, but significant, condition.
 *!     @value 6
 *!       LOG_INFO, informational message.
 *!     @value 7
 *!       LOG_DEBUG, debug-level message.
 *!   @endint
 */
void f_syslog(INT32 args)
{
  struct pike_string *s;
  INT_TYPE pri=0, i;

#ifdef PIKE_SECURITY
  int errno;
#define EPERM 0
#endif
  VALID_FILE_IO("System.syslog","write");

  get_all_args("syslog", args, "%i%S", &i, &s);
 
  if(i & (1<<0)) pri |= LOG_EMERG;
  if(i & (1<<1)) pri |= LOG_ALERT;
  if(i & (1<<2)) pri |= LOG_CRIT;
  if(i & (1<<3)) pri |= LOG_ERR;
  if(i & (1<<4)) pri |= LOG_WARNING;
  if(i & (1<<5)) pri |= LOG_NOTICE;
  if(i & (1<<6)) pri |= LOG_INFO;
  if(i & (1<<7)) pri |= LOG_DEBUG;

#ifndef MIGHT_HAVE_SYSLOG_RACES
  /* glibc/linuxthreads has a race in syslog(2) that can make write,
   * writev etc crash with the instruction pointer set to 0x1 when
   * signals are delivered. Not releasing the interpreter lock here
   * avoids that race most of the time but it's still not a 100%
   * solution.
   *
   * C.f. the report "bug in thread support" filed by Balazs Scheidler
   * in Oct 22, 2001 (http://sources.redhat.com/ml/libc-hacker/
   * 2001-10/msg00020.html). It's verified to still exist in glibc
   * 2.2.93 (RedHat 8.0) and 2.3.2 (RedHat 9). */
  THREADS_ALLOW();
#endif

  syslog(pri, "%s", s->str);

#ifndef MIGHT_HAVE_SYSLOG_RACES
  THREADS_DISALLOW();
#endif

  pop_n_elems(args);
}

/*! @decl void closelog()
 *!
 *! @fixme
 *!   Document this function.
 */
void f_closelog(INT32 args)
{
  ASSERT_SECURITY_ROOT("System.closelog");
  closelog();
  pop_n_elems(args);
}

/*! @endmodule
 */

#endif /* HAVE_SYSLOG */
