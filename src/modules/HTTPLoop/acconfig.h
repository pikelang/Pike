/*
 * $Id: acconfig.h,v 1.2 2000/03/26 14:47:39 grubba Exp $
 */
#undef CAN_HAVE_SENDFILE
#undef CAN_HAVE_LINUX_SYSCALL4
#undef CAN_HAVE_NONSHARED_LINUX_SYSCALL4

/* Define if your sendfile() takes 7 args (FreeBSD style) */
#undef HAVE_FREEBSD_SENDFILE

/* Define this if you want to disable the use of sendfile(2). */
#undef HAVE_BROKEN_SENDFILE
