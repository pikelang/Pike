/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
** libMird by Mirar <mirar@mirar.org>
** please submit bug reports and patches to the author
**
** also see http://www.mirar.org/mird/
*/

/* config.h */
/* this is for win32 systems, that doesn't work that well */
/* with configure/autoconf. Created by <paul@theV.net>.   */

/* define if there is an inline keyword */
/* #define HAS_INLINE 1 */

/* define as the inline keyword */
/* #define INLINE static inline */
#define INLINE

/* define as the 16 bit integer (2 bytes) */
#define INT16 short

/* define as the 32 bit integer (4 bytes) */
#define INT32 long

/* define as the 64 bit integer (8 bytes) if any*/
/* #define INT64 long long */
#define INT64 __int64

/* define if you want memory allocation debug */
/* #undef MEM_DEBUG */

/* define if you want memory usage statistics */
/* #undef MEM_STATS */

/* define as the way to print an off_t */
#define PRINTOFFT "l"

/* define as the system's default block unit size */
#define STAT_BLOCKUNIT DEV_BSIZE

/* define if you want to use fcntl(...FREESP...) to free blocks */
/* #undef USE_FCNTL_FREESP */

/* define if flock() works */
/* #define HAS_LOCKF 1 */

/* define if lockf() workds */
/* #define HAS_FLOCK 1 */

/* define if O_APPEND works as it should */
#define WORKING_O_APPEND 1

/* define if the machine handles enough unaligned memory access */
/* ie, htonl( <uneven address> ) */
/* #undef HANDLES_UNALIGNED_ACCESS */

/* define this if ftrunc works */
/* #define WORKING_FTRUNCATE 1 */

/* define as the off_t type to use */
/* #define MIRD_OFF_T signed long */
#define MIRD_OFF_T off_t

/* set if you want syscall statistics */
/* #define SYSCALL_STATISTICS 1 */

/* Define if you have the fdatasync function.  */
#define HAVE_FDATASYNC 1

/* Define if you have the ftruncate function.  */
/* #define HAVE_FTRUNCATE 1 */

/* Define if you have the llseek function.  */
#define HAVE_LLSEEK 1

/* Define if you have the lseek64 function.  */
#define HAVE_LSEEK64 1

/* Define if you have the memcmp function.  */
#define HAVE_MEMCMP 1

/* Define if you have the memcpy function.  */
#define HAVE_MEMCPY 1

/* Define if you have the memmove function.  */
#define HAVE_MEMMOVE 1

/* Define if you have the memset function.  */
#define HAVE_MEMSET 1

/* Define if you have the pread function.  */
#define HAVE_PREAD 1

/* Define if you have the pwrite function.  */
#define HAVE_PWRITE 1

/* Define if you have the <arpa/inet.h> header file.  */
#define HAVE_ARPA_INET_H 1

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <netinet/in.h> header file.  */
#define HAVE_NETINET_IN_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <sys/file.h> header file.  */
#define HAVE_SYS_FILE_H 1

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the <sys/stat.h> header file.  */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/types.h> header file.  */
#define HAVE_SYS_TYPES_H 1

/* Define if you have O_BINARY */
#define HAVE_O_BINARY 1

#define sync() _flushall()
#define fsync(x) _commit(x)
#define fdatasync(x) _commit(x)
