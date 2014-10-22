/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#define PIKE_MAJOR_VERSION 8
#define PIKE_MINOR_VERSION 0
#define PIKE_BUILD_VERSION 11

#define LOWEST_COMPAT_MAJOR 7
#define LOWEST_COMPAT_MINOR 6

/* Prototypes begin here */
PMOD_EXPORT void f_version(INT32 args);
void push_compact_version(void);
/* Prototypes end here */
