/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#undef CAN_HAVE_SENDFILE
#undef CAN_HAVE_LINUX_SYSCALL4
#undef CAN_HAVE_NONSHARED_LINUX_SYSCALL4

/* Define if your sendfile() takes 7 args (FreeBSD style) */
#undef HAVE_FREEBSD_SENDFILE

/* Define this if you want to disable the use of sendfile(2). */
#undef HAVE_BROKEN_SENDFILE
