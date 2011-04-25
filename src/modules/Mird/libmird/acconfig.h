/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* define if there is an inline keyword */
#undef HAS_INLINE

/* define as the inline keyword */
#undef INLINE

/* define as the 16 bit integer (2 bytes) */
#define INT16 short

/* define as the 32 bit integer (4 bytes) */
#define INT32 long

/* define as the 64 bit integer (8 bytes) if any*/
#undef INT64

/* define if you want memory allocation debug */
#undef MEM_DEBUG

/* define if you want memory usage statistics */
#undef MEM_STATS

/* define as the way to print an off_t */
#define PRINTOFFT "%ld"

/* define as the system's default block unit size */
#undef STAT_BLOCKUNIT

/* define if you want to use fcntl(...FREESP...) to free blocks */
#undef USE_FCNTL_FREESP

/* define if flock() works */
#undef HAS_LOCKF

/* define if lockf() workds */
#undef HAS_FLOCK

/* define if O_APPEND works as it should */
#undef WORKING_O_APPEND

/* define if the machine handles enough unaligned memory access */
/* ie, htonl( <uneven address> ) */
#undef HANDLES_UNALIGNED_ACCESS

/* define this if ftrunc works */
#undef WORKING_FTRUNCATE

/* define as the off_t type to use */
#define MIRD_OFF_T off_t

/* set if you want syscall statistics */
#define SYSCALL_STATISTICS 1

/* set if you have/need the O_BINARY to open() */
#undef HAVE_O_BINARY

/* define this if libmird won't work on this system ;) */
#undef MIRD_WONT_WORK
