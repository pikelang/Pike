/*
 * $Id: acconfig.h,v 1.8 1998/05/17 21:07:33 grubba Exp $
 */

#ifndef FILE_MACHINE_H
#define FILE_MACHINE_H

@TOP@
@BOTTOM@

/* Define if your statfs() call takes 4 arguments */
#undef HAVE_SYSV_STATFS

/* Define if you have the struct statfs */
#undef HAVE_STRUCT_STATFS

/* Define if your statfs struct has the f_bavail member */
#undef HAVE_STATFS_F_BAVAIL

/* Define if you have the struct fs_data */
#undef HAVE_STRUCT_FS_DATA

/* Define if you have the struct statvfs */
#undef HAVE_STRUCT_STATVFS

/* Define if your statvfs struct has the member f_basetype */
#undef HAVE_STATVFS_F_BASETYPE

/* Define if your readdir_r is POSIX compatible. */
#undef HAVE_POSIX_READDIR_R

/* Define if your readdir_r is Solaris compatible. */
#undef HAVE_SOLARIS_READDIR_R

/* Define if your readdir_r is HPUX compatible. */
#undef HAVE_HPUX_READDIR_R

/* Define if you have strerror.  */
#undef HAVE_STRERROR

/* Define if you have a working getcwd */
#undef HAVE_WORKING_GETCWD

/* Do we have socketpair() ? */
#undef HAVE_SOCKETPAIR

/* Does select() work together with shutdown() on UNIX sockets? */
#undef UNIX_SOCKETS_WORKS_WITH_SHUTDOWN

/* Buffer size to use on open sockets */
#undef SOCKET_BUFFER_MAX 

/* Number of args to mkdir() */
#define MKDIR_ARGS 2

#endif

