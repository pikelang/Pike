/*
 * $Id: acconfig.h,v 1.2 1997/06/01 22:57:26 grubba Exp $
 */

#ifndef FILE_MACHINE_H
#define FILE_MACHINE_H

@TOP@
@BOTTOM@

/* Define if your statfs() call takes 4 arguments */
#undef HAVE_SGI_STATFS

/* Define if your statfs struct has the f_bavail member */
#undef HAVE_STATFS_F_BAVAIL

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

/* Buffer size to use on open sockets */
#undef SOCKET_BUFFER_MAX 

#endif

