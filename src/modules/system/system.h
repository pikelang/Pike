/*
 * $Id: system.h,v 1.3 1998/02/24 23:10:25 hubbe Exp $
 *
 * Prototypes for the Pike system-module
 *
 * Henrik Grubbström 1997-01-28
 */

#ifndef PIKE_MODULES_SYSTEM_H
#define PIKE_MODULES_SYSTEM_H

/*
 * Includes
 */

#include "global.h"
#include "pike_types.h"

/*
 * Prototypes
 */

/*
 * passwords.c
 */
void f_getgrnam(INT32 args);
void f_getgrgid(INT32 args);
void f_getpwnam(INT32 args);
void f_getpwuid(INT32 args);
void f_setpwent(INT32 args);
void f_endpwent(INT32 args);
void f_getpwent(INT32 args);

/*
 * syslog.c
 */
void f_openlog(INT32 args);
void f_syslog(INT32 args);
void f_closelog(INT32 args);

#endif /* PIKE_MODULES_SYSTEM_H */
