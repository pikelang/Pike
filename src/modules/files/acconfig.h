/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: acconfig.h,v 1.24 2005/04/30 18:40:59 per Exp $
*/

#ifndef FILE_MACHINE_H
#define FILE_MACHINE_H

@TOP@
@BOTTOM@

/* Define this if your <sys/sendfile.h> is broken. */
#undef HAVE_BROKEN_SYS_SENDFILE_H

/* Define this if you have a FreeBSD-style (7 args) sendfile(). */
#undef HAVE_FREEBSD_SENDFILE

/* Define this if you have a HP/UX-style (6 args) sendfile(). */
#undef HAVE_HPUX_SENDFILE

/* Define this if you want to disable the use of sendfile(2). */
#undef HAVE_BROKEN_SENDFILE

/* Define this if you have a struct iovec */
#undef HAVE_STRUCT_IOVEC

/* Define this if you have a struct stat with 'blocks' member */
#undef HAVE_STRUCT_STAT_BLOCKS

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

/* Define if your statvfs struct has the f_fstr member */
#undef HAVE_STATVFS_F_FSTR

/* Define if your statfs struct has the f_fsid member */
#undef HAVE_STATFS_F_FSID

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

/* With termios */
#undef WITH_TERMIOS 

/* Filesystem notifications */
#undef HAVE_NOTIFICATIONS

/* Define to path of pseudo terminal master device if available */
#undef PTY_MASTER_PATHNAME

/* Define to path of chgpt to use chgpt directly rather than
   calling grantpt (needed on SysV's like HPUX & OSF/1) */
#undef USE_CHGPT

/* Define to path of pt_chmod to use pt_chmod directly rather than
   calling grantpt (needed on SysV's like Solaris) */
#undef USE_PT_CHMOD

#endif
