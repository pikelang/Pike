/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
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
void f_setgrent(INT32 args);
void f_endgrent(INT32 args);
void f_getgrent(INT32 args);

/*
 * syslog.c
 */
void f_openlog(INT32 args);
void f_syslog(INT32 args);
void f_closelog(INT32 args);

#endif /* PIKE_MODULES_SYSTEM_H */

/*
 * memory.c
 */

#ifdef HAVE_WINDOWS_H
#define WIN32SHM
#endif

struct memory_storage
{
   unsigned char *p;
   size_t size;
   
#define MEM_READ        0x01
#define MEM_WRITE       0x02
#define MEM_FREE_FREE   0x10
#define MEM_FREE_MUNMAP 0x20   
#define MEM_FREE_SHMDEL 0x40
   unsigned long flags;
#ifdef WIN32SHM
   void *extra;
#endif
};
