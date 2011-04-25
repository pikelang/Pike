/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 * System dependant definitions for the system module for Pike
 *
 * Henrik Grubbström 1997-01-20
 */
 
#ifndef SYSTEM_MACHINE_H
#define SYSTEM_MACHINE_H
 
@TOP@
@BOTTOM@ 

/* Define if you have solaris style gethostbyname_r. */
#undef HAVE_SOLARIS_GETHOSTBYNAME_R
 
/* Define if you have OSF1 style gethostbyname_r. */
#undef HAVE_OSF1_GETHOSTBYNAME_R

/* Define if you have solaris style getspnam_r. */
#undef HAVE_SOLARIS_GETSPNAM_R
 
/* Define if you have h_addr_list in the hostent struct */
#undef HAVE_H_ADDR_LIST

/* Define if you have pw_gecos in the passwd struct */
#undef HAVE_PW_GECOS

/* Define if <netinet/in.h> defines the in_addr_t type. */
#undef HAVE_IN_ADDR_T

/* Define if your setpgrp(2) takes two arguments (BSD). */
#undef HAVE_SETPGRP_BSD

/* Define if your get/setrlimit is using BSD 4.3 syntax. */
#undef SETRLIMIT_SYNTAX_BSD43

/* Define if your get/setrlimit is using stardard (with rlim_t) syntax. */
#undef SETRLIMIT_SYNTAX_STANDARD

/* Define if the get/setrlimit syntax is unknown */
#undef SETRLIMIT_SYNTAX_UNKNOWN

/* Define if you have a working <sys/resource.h> header file.  */
#undef HAVE_SYS_RESOURCE_H

/* Define if you have a working <sys/user.h> header file.  */
#undef HAVE_SYS_USER_H

/* Define if ITIMER_REAL, ITIMER_VIRTUAL and ITIMER_PROF is in tInt02 */
#undef ITIMER_TYPE_IS_02

/* Define if you have the RDTSC instruction */
#undef HAVE_RDTSC

/* Define if syslog() might have races which makes it unsafe to run unlocked. */
#undef MIGHT_HAVE_SYSLOG_RACES

#endif /* SYSTEM_MACHINE_H */
