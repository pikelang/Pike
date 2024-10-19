/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef FILE_MACHINE_H
#define FILE_MACHINE_H

@TOP@
@BOTTOM@

/* Define this if your <sys/sendfile.h> is broken. */
#undef HAVE_BROKEN_SYS_SENDFILE_H

/* Define this if you have IPPROTO_IPV6. */
#undef HAVE_IPPROTO_IPV6

/* Define this if you have ZFS_PROP_UTF8ONLY. */
#undef HAVE_ZFS_PROP_UTF8ONLY

/* Define this if you have a FreeBSD-style (7 args) sendfile(). */
#undef HAVE_FREEBSD_SENDFILE

/* Define this if you have a HP/UX-style (6 args) sendfile()
 * with no struct sf_hdtr. */
#undef HAVE_HPUX_SENDFILE

/* Define this if you have a MacOS X-style (6 args) sendfile()
 * with struct sf_hdtr. */
#undef HAVE_MACOSX_SENDFILE

/* Define this if you have a sendfile(2) where the length of the headers
 * are counted towards the file length argument.
 * This is the case for MacOS X and FreeBSD before FreeBSD 5.0. */
#undef HAVE_SENDFILE_HEADER_LEN_PROBLEM

/* Define this if you want to disable the use of sendfile(2). */
#undef HAVE_BROKEN_SENDFILE

/* Define this if your DIR has the field dd_fd (POSIX or X/OPEN). */
#undef HAVE_DIR_DD_FD

/* Define this if your DIR has the field d_fd (old Solaris). */
#undef HAVE_DIR_D_FD

/* Define this if you have a struct stat with 'blocks' member. */
#undef HAVE_STRUCT_STAT_BLOCKS

/* Define this if your struct stat has nanosecond resolution timefields. */
#undef HAVE_STRUCT_STAT_NSEC

/* Define this if you have a struct sockaddr_un with 'sun_len' member. */
#undef HAVE_STRUCT_SOCKADDR_UN_SUN_LEN

/* Define if your statfs() call takes 4 arguments */
#undef HAVE_SYSV_STATFS

/* Define if you have the struct statfs */
#undef HAVE_STRUCT_STATFS

/* Define if your statfs struct has the f_bavail member */
#undef HAVE_STATFS_F_BAVAIL

/* Define if your statfs struct has the f_fstypename member */
#undef HAVE_STATFS_F_FSTYPENAME

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

/* Define if you have Darwin/MacOSX style xattr */
#undef HAVE_DARWIN_XATTR

/* Define if you have strerror.  */
#undef HAVE_STRERROR

/* Do we have socketpair() ? */
#undef HAVE_SOCKETPAIR

/* Does select() work together with shutdown() on UNIX sockets? */
#undef UNIX_SOCKETS_WORKS_WITH_SHUTDOWN

/* Buffer size to use on open sockets */
#undef SOCKET_BUFFER_MAX

/* With termios */
#undef WITH_TERMIOS

/* Define to path of pseudo terminal master device if available */
#undef PTY_MASTER_PATHNAME

/* Define to path of chgpt to use chgpt directly rather than
   calling grantpt (needed on SysV's like HPUX & OSF/1) */
#undef USE_CHGPT

/* Define to path of pt_chmod to use pt_chmod directly rather than
   calling grantpt (needed on SysV's like Solaris) */
#undef USE_PT_CHMOD

#if (defined(HAVE_STRUCT_MSGHDR_MSG_CONTROL) && defined(SCM_RIGHTS)) ||	\
    defined(HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS) || \
    defined(I_SENDFD)
#ifndef __amigaos__
#define HAVE_PIKE_SEND_FD
#endif
#endif

#endif
