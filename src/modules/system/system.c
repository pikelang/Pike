/*
 * $Id: system.c,v 1.105 2003/10/17 15:39:50 mast Exp $
 *
 * System-call module for Pike
 *
 * Henrik Grubbström 1997-01-20
 */

/*
 * Includes
 */

#include "global.h"

#include "system_machine.h"
#include "system.h"

RCSID("$Id: system.c,v 1.105 2003/10/17 15:39:50 mast Exp $");
#ifdef HAVE_WINSOCK_H
#include <winsock.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include "module_support.h"
#include "las.h"
#include "interpret.h"
#include "stralloc.h"
#include "threads.h"
#include "svalue.h"
#include "mapping.h"
#include "builtin_functions.h"
#include "constants.h"
#include "pike_memory.h"
#include "security.h"

/* The sp macro conflicts with Solaris 2.5.1's <sys/conf.h>. */
#ifdef sp
#undef sp
#define STACKPOINTER_WAS_DEFINED
#endif /* sp */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif /* HAVE_PWD_H */
#ifdef HAVE_SYS_CONF_H
#include <sys/conf.h>
#endif /* HAVE_SYS_CONF_H */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */
#ifdef HAVE_GRP_H
#include <grp.h>
#endif /* HAVE_GRP_H */
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */
#ifdef HAVE_SYS_SYSTEMINFO_H
#include <sys/systeminfo.h>
#endif /* HAVE_SYS_SYSTEMINFO_H */
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_SYS_ID_H
#include <sys/id.h>
#endif /* HAVE_SYS_ID_H */

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif /* HAVE_SYS_PRCTL_H */

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif

#ifdef HAVE_SYS_UTIME_H
#include <sys/utime.h>
#endif

#ifdef HAVE_NETINFO_NI_H
#include <netinfo/ni.h>
#endif

/* Restore the sp macro */
#ifdef STACKPOINTER_WAS_DEFINED
#define sp Pike_sp
#undef STACK_POINTER_WAS_DEFINED
#endif /* STACKPOINTER_WAS_DEFINED */

#include "dmalloc.h"

#ifndef NGROUPS_MAX
#ifdef NGROUPS
#define NGROUPS_MAX	NGROUPS
#else /* !NGROUPS */
#define NGROUPS_MAX	256	/* Should be sufficient for most OSs */
#endif /* NGROUPS */
#endif /* !NGROUPS_MAX */

#ifdef HAVE_IN_ADDR_T
#define IN_ADDR_T	in_addr_t
#else /* !HAVE_IN_ADDR_T */
#define IN_ADDR_T	unsigned int
#endif /* HAVE_IN_ADDR_T */

/*
 * Functions
 */

/* Helper functions */

static void report_error(const char *function_name)
{
  char *error_msg = "Unknown reason";

  switch(errno) {
  case EACCES:
    error_msg = "Access denied";
    break;
#ifdef EDQUOT
  case EDQUOT:
    error_msg = "Out of quota";
    break;
#endif /* EDQUOT */
  case EEXIST:
    error_msg = "Destination already exists";
    break;
  case EFAULT:
    error_msg = "Internal Pike error: Bad Pike string!";
    break;
  case EINVAL:
    error_msg = "Bad argument";
    break;
  case EIO:
    error_msg = "I/O error";
    break;
#ifdef ELOOP
  case ELOOP:
    error_msg = "Too deep nesting of symlinks";
    break;
#endif /* ELOOP */
#ifdef EMLINK
  case EMLINK:
    error_msg = "Too many hardlinks";
    break;
#endif /* EMLINK */
#ifdef EMULTIHOP
  case EMULTIHOP:
    error_msg = "The filesystems do not allow hardlinks between them";
    break;
#endif /* EMULTIHOP */
#ifdef ENAMETOOLONG
  case ENAMETOOLONG:
    error_msg = "Filename too long";
    break;
#endif /* ENAMETOOLONG */
  case ENOENT:
    error_msg = "File not found";
    break;
#ifdef ENOLINK
  case ENOLINK:
    error_msg = "Link to remote machine no longer active";
    break;
#endif /* ENOLINK */
  case ENOSPC:
    error_msg = "Filesystem full";
    break;
  case ENOTDIR:
    error_msg = "A path component is not a directory";
    break;
  case EPERM:
    error_msg = "Permission denied";
    break;
#ifdef EROFS
  case EROFS:
    error_msg = "Read-only filesystem";
    break;
#endif /* EROFS */
  case EXDEV:
    error_msg = "Different filesystems";
    break;
#ifdef ESTALE
  case ESTALE:
    error_msg = "Stale NFS file handle";
    break;
#endif /* ESTALE */
  case ESRCH:
    error_msg = "No such process";
    break;
  }
  Pike_error("%s(): Failed: %s\n", function_name, error_msg);
}


/*
 * efuns
 */

#ifdef HAVE_LINK
/* void hardlink(string from, string to) */
void f_hardlink(INT32 args)
{
  char *from;
  char *to;
  int err;

  VALID_FILE_IO("hardlink","write");

  get_all_args("hardlink",args, "%s%s", &from, &to);

  THREADS_ALLOW_UID();
  do {
    err = link(from, to);
  } while ((err < 0) && (errno == EINTR));
  THREADS_DISALLOW_UID();

  if (err < 0) {
    report_error("hardlink");
  }
  pop_n_elems(args);
}
#endif /* HAVE_LINK */

#ifdef HAVE_SYMLINK
/* void symlink(string from, string to) */
void f_symlink(INT32 args)
{
  char *from;
  char *to;
  int err;

  VALID_FILE_IO("symlink","write");

  get_all_args("symlink",args, "%s%s", &from, &to);

  THREADS_ALLOW_UID();
  do {
    err = symlink(from, to);
  } while ((err < 0) && (errno == EINTR));
  THREADS_DISALLOW_UID();

  if (err < 0) {
    report_error("symlink");
  }
  pop_n_elems(args);
}
#endif /* HAVE_SYMLINK */

#ifdef HAVE_READLINK
/* string readlink(string path) */
void f_readlink(INT32 args)
{
  char *path;
  int buflen;
  char *buf;
  int err;

  VALID_FILE_IO("readlink","read");

  get_all_args("readlink",args, "%s", &path);

  buflen = 100;

  do {
    buflen *= 2;
    if (!(buf = alloca(buflen))) {
      Pike_error("readlink(): Out of memory\n");
    }

    THREADS_ALLOW_UID();
    do {
      err = readlink(path, buf, buflen);
    } while ((err < 0) && (errno == EINTR));
    THREADS_DISALLOW_UID();
  } while(
#ifdef ENAMETOOLONG
	  ((err < 0) && (errno == ENAMETOOLONG)) ||
#endif /* ENAMETOOLONG */
	  (err >= buflen - 1));

  if (err < 0) {
    report_error("readlink");
  }
  pop_n_elems(args);
  push_string(make_shared_binary_string(buf, err));
}
#endif /* HAVE_READLINK */

#ifndef HAVE_RESOLVEPATH
#ifdef HAVE_READLINK
/* FIXME: Write code that simulates resolvepath() here
 */
/* #define HAVE_RESOLVEPATH */
#endif /* HAVE_READLINK */
#endif /* !HAVE_RESOLVEPATH */

#ifdef HAVE_RESOLVEPATH
/* string resolvepath(string path) */
void f_resolvepath(INT32 args)
{
  char *path;
  int buflen;
  char *buf;
  int err;

  VALID_FILE_IO("resolvepath","read");

  get_all_args("resolvepath", args, "%s", &path);

  buflen = 100;

  do {
    buflen *= 2;
    if (!(buf = alloca(buflen))) {
      Pike_error("resolvepath(): Out of memory\n");
    }

    THREADS_ALLOW_UID();
    do {
      err = resolvepath(path, buf, buflen);
    } while ((err < 0) && (errno == EINTR));
    THREADS_DISALLOW_UID();
  } while(
#ifdef ENAMETOOLONG
	  ((err < 0) && (errno == ENAMETOOLONG)) ||
#endif /* ENAMETOOLONG */
	  (err >= buflen - 1));

  if (err < 0) {
    report_error("resolvepath");
  }
  pop_n_elems(args);
  push_string(make_shared_binary_string(buf, err));
}
#endif /* HAVE_RESOLVEPATH */

/* int umask(void|int mask) */
void f_umask(INT32 args)
{
  int oldmask;

  VALID_FILE_IO("umask","status");

  if (args) {
    INT_TYPE setmask;
    get_all_args("umask", args, "%d", &setmask);
    oldmask = umask(setmask);
  }
  else {
    oldmask = umask(0);
    umask(oldmask);
  }

  pop_n_elems(args);
  push_int(oldmask);
}

/* void chmod(string path, int mode) */
void f_chmod(INT32 args)
{
  char *path;
  INT_TYPE mode;
  int err;

  VALID_FILE_IO("chmod","chmod");

  get_all_args("chmod", args, "%s%i", &path, &mode);
  THREADS_ALLOW_UID();
  do {
    err = chmod(path, mode);
  } while ((err < 0) && (errno == EINTR));
  THREADS_DISALLOW_UID();
  if (err < 0) {
    report_error("chmod");
  }
  pop_n_elems(args);
}

#ifdef HAVE_CHOWN
void f_chown(INT32 args)
{
  char *path;
  INT_TYPE uid;
  INT_TYPE gid;
  int err;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    Pike_error("chown: permission denied.\n");
#endif

  get_all_args("chown", args, "%s%i%i", &path, &uid, &gid);
  THREADS_ALLOW_UID();
  do {
    err = chown(path, uid, gid);
  } while((err < 0) && (errno == EINTR));
  THREADS_DISALLOW_UID();
  if (err < 0) {
    report_error("chown");
  }
  pop_n_elems(args);
}
#endif

#ifdef HAVE_UTIME
void f_utime(INT32 args)
{
  char *path;
  INT_TYPE atime, mtime;
  int err;
  /*&#()&@(*#&$ NT ()*&#)(&*@$#*/
#ifdef _UTIMBUF_DEFINED
  struct _utimbuf b;
#else
  struct utimbuf b;
#endif

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    Pike_error("utime: permission denied.\n");
#endif

  get_all_args("utime", args, "%s%i%i", &path, &atime, &mtime);
  b.actime=atime;
  b.modtime=mtime;
  THREADS_ALLOW_UID();
  do {
    err = utime(path, &b);
  } while((err < 0) && (errno == EINTR));
  THREADS_DISALLOW_UID();
  if (err < 0) {
    report_error("utime");
  }
  pop_n_elems(args);
}
#endif

#ifdef HAVE_INITGROUPS
/* void initgroups(string name, int gid) */
void f_initgroups(INT32 args)
{
  char *user;
  int err;
  INT_TYPE group;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    Pike_error("initgroups: permission denied.\n");
#endif
  
  VALID_FILE_IO("initgroups","status");
  get_all_args("initgroups", args, "%s%i", &user, &group);
  err = initgroups(user, group);
  if (err < 0) {
    report_error("initgroups");
  }
  pop_n_elems(args);
}
#endif /* HAVE_INITGROUPS */

#ifdef HAVE_SETGROUPS
/* void cleargroups() */
void f_cleargroups(INT32 args)
{
  static gid_t gids[1] = { 65534 };	/* To safeguard against stupid OS's */
  int err;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    Pike_error("cleargroups: permission denied.\n");
#endif

  pop_n_elems(args);
  err = setgroups(0, gids);
  if (err < 0) {
    report_error("cleargroups");
  }
}

/* void setgroup(array(int) gids) */
/* NOT Implemented in Pike 0.5 */
void f_setgroups(INT32 args)
{
  static gid_t safeguard[1] = { 65534 };
  struct array *arr = NULL;
  gid_t *gids = NULL;
  INT32 i;
  INT32 size;
  int err;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    Pike_error("setgroups: permission denied.\n");
#endif

  get_all_args("setgroups", args, "%a", &arr);
  if ((size = arr->size)) {
    gids = (gid_t *)xalloc(arr->size * sizeof(gid_t));
  } else {
    gids = safeguard;
  }

  for (i=0; i < size; i++) {
    if (arr->item[i].type != T_INT) {
      /* Only reached if arr->size > 0
       * so we always have an allocated gids here.
       */
      free(gids);
      Pike_error("setgroups(): Bad element %d in array (expected int)\n", i);
    }
    gids[i] = arr->item[i].u.integer;
  }

  pop_n_elems(args);

  err = setgroups(size, gids);
  if (err < 0) {
    if (size) {
      free(gids);
    }
    report_error("setgroups");
  }
}
#endif /* HAVE_SETGROUPS */

#ifdef HAVE_GETGROUPS
/* array(int) getgroups() */
void f_getgroups(INT32 args)
{
  gid_t *gids = NULL;
  int numgrps;
  int i;

  pop_n_elems(args);

  numgrps = getgroups(0, NULL);
  if (numgrps <= 0) {
    /* OS which doesn't understand this convention */
    numgrps = NGROUPS_MAX;
  }
  gids = (gid_t *)xalloc(sizeof(gid_t) * numgrps);

  numgrps = getgroups(numgrps, gids);

  for (i=0; i < numgrps; i++) {
    push_int(gids[i]);
  }
  free(gids);

  if (numgrps < 0) {
    report_error("getgroups");
  }

  f_aggregate(numgrps);
}
#endif /* HAVE_GETGROUPS */

#ifdef HAVE_INNETGR
/* int innetgr(string netgroup, string|void machine,
 *             string|void user, string|void domain) */
void f_innetgr(INT32 args)
{
  char *strs[4] = { NULL, NULL, NULL, NULL };
  int i;
  int res;

  check_all_args("innetgr", args, BIT_STRING, BIT_STRING|BIT_INT|BIT_VOID,
		 BIT_STRING|BIT_INT|BIT_VOID, BIT_STRING|BIT_INT|BIT_VOID, 0);

  for(i = 0; i < args; i++) {
    if (sp[i-args].type == T_STRING) {
      if (sp[i-args].u.string->size_shift) {
	SIMPLE_BAD_ARG_ERROR("innetgr", i+1, "string (8bit)");
      }
      strs[i] = sp[i-args].u.string->str;
    } else if (sp[i-args].u.integer) {
      SIMPLE_BAD_ARG_ERROR("innetgr", i+1, "string|void");
    }
  }

  THREADS_ALLOW();
  res = innetgr(strs[0], strs[1], strs[2], strs[3]);
  THREADS_DISALLOW();

  pop_n_elems(args);
  push_int(res);
}
#endif /* HAVE_INNETGR */

#ifdef HAVE_SETUID 
void f_setuid(INT32 args)
{
  int err;
  INT_TYPE id;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    Pike_error("setuid: permission denied.\n");
#endif

  get_all_args("setuid", args, "%i", &id);
 
  if(id == -1) {
    struct passwd *pw = getpwnam("nobody");
    id = pw->pw_uid;
  } else {
    id = sp[-args].u.integer;
  }

  err = setuid(id);
  pop_n_elems(args);
  push_int(err);
}
#endif

#ifdef HAVE_SETGID
void f_setgid(INT32 args)
{
  int err;
  INT_TYPE id;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    Pike_error("setgid: permission denied.\n");
#endif
  get_all_args("setgid", args, "%i", &id);
 
  if(id == -1) {
    struct passwd *pw = getpwnam("nobody");
    id = pw->pw_gid;
  } else {
    id = sp[-args].u.integer;
  }

  err=setgid(id);
  pop_n_elems(args);
  push_int(err);
}
#endif

#if defined(HAVE_SETEUID) || defined(HAVE_SETRESUID)
/* int seteuid(int euid) */
void f_seteuid(INT32 args)
{
  INT_TYPE id;
  int err;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    Pike_error("seteuid: permission denied.\n");
#endif
  get_all_args("seteuid", args, "%i", &id);
 
  if(id == -1) {
    struct passwd *pw = getpwnam("nobody");
    id = pw->pw_uid;
  } else {
    id = sp[-args].u.integer;
  }

  /* FIXME: Check return-code */
#ifdef HAVE_SETEUID
  err = seteuid(id);
#else
  err = setresuid(-1, id, -1);
#endif /* HAVE_SETEUID */

  pop_n_elems(args);
  push_int(err);
}
#endif /* HAVE_SETEUID || HAVE_SETRESUID */
 
#if defined(HAVE_SETEGID) || defined(HAVE_SETRESGID)
/* int setegid(int egid) */
void f_setegid(INT32 args)
{
  INT_TYPE id;
  int err;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    Pike_error("setegid: permission denied.\n");
#endif

  get_all_args("setegid", args, "%i", &id);

  if(id == -1)
  {
    struct passwd *pw = getpwnam("nobody");
    id = pw->pw_gid;
  } else {
    id = sp[-args].u.integer;
  }
 
  /* FIXME: Check return-code */
#ifdef HAVE_SETEGID
  err = setegid(id);
#else
  err = setresgid(-1, id, -1);
#endif /* HAVE_SETEUID */

  pop_n_elems(args);
  push_int(err);
}
#endif /* HAVE_SETEGID || HAVE_SETRESGID */

#if defined(HAVE_GETPGID) || defined(HAVE_GETPGRP)
void f_getpgrp(INT32 args)
{
  int pid = 0;
  int pgid = 0;

  if (args) {
    if (sp[-args].type != T_INT) {
      Pike_error("Bad argument 1 to getpgrp()\n");
    }
    pid = sp[-args].u.integer;
  }
  pop_n_elems(args);
#ifdef HAVE_GETPGID
  pgid = getpgid(pid);
#elif defined(HAVE_GETPGRP)
  if (pid && (pid != getpid())) {
    Pike_error("getpgrp(): Mode not supported on this OS\n");
  }
  pgid = getpgrp();
#endif
  if (pgid < 0)
    report_error("getpgrp");

  push_int(pgid);
}
#endif /* HAVE_GETPGID || HAVE_GETPGRP */

#if defined(HAVE_SETPGID) || defined(HAVE_SETPGRP)
void f_setpgrp(INT32 args)
{
  int pid;
  pop_n_elems(args);
#ifdef HAVE_SETPGID
  pid = setpgid(0, 0);
#else /* !HAVE_SETPGID */
#ifdef HAVE_SETPGRP_BSD
  pid = setpgrp(0, 0);
#else /* !HAVE_SETPGRP_BSD */
  pid = setpgrp();
#endif /* HAVE_SETPGRP_BSD */
#endif /* HAVE_SETPGID */
  if (pid < 0)
    report_error("setpgrp");

  push_int(pid);
}
#endif

#if defined(HAVE_GETSID)
void f_getsid(INT32 args)
{
  int pid = 0;
  if (args >= 1 && sp[-args].type != T_INT)
       Pike_error("Bad argument for getsid().\n");
  if (args >= 1)
       pid = sp[-args].u.integer;
  pop_n_elems(args);
  pid = getsid(pid);
  if (pid < 0)
       report_error("getsid");
  push_int(pid);
}
#endif

#if defined(HAVE_SETSID)
void f_setsid(INT32 args)
{
  int pid;
  if (args > 0)
       Pike_error("setsid() takes no arguments.\n");
  pop_n_elems(args);
  pid = setsid();
  if (pid < 0)
       report_error("setsid");
  push_int(pid);
}
#endif

#if defined(HAVE_PRCTL) && defined(PR_SET_DUMPABLE)
/*! @decl int(0..1) dumpable(int(0..1)|void val)
 *!   Get and/or set whether this process should be able to
 *!   dump core.
 *!
 *! @param val
 *!   Optional argument to set the core dumping state.
 *!   @int
 *!     @value 0
 *!       Disable core dumping for this process.
 *!     @value 1
 *!       Enable core dumping for this process.
 *!   @endint
 *!
 *! @returns
 *!   Returns @tt{1@} if this process currently is capable of dumping core,
 *!   and @tt{0@} (zero) if not.
 *!
 *! @note
 *!   This function is currently only available on some versions of Linux.
 */ 
void f_dumpable(INT32 args)
{
  int current = prctl(PR_GET_DUMPABLE);

  if (current == -1) {
    int err = errno;
    Pike_error("Failed to get dumpable state. errno:%d\n", err);
  }
  if (args) {
    INT_TYPE val;
    get_all_args("dumpable", args, "%i", &val);
    if (val & ~1) {
      SIMPLE_BAD_ARG_ERROR("dumpable", 1, "int(0..1)");
    }
    if (prctl(PR_SET_DUMPABLE, val) == -1) {
      int err = errno;
      Pike_error("Failed to set dumpable state to %d. errno:%d\n", val, err);
    }
  }
  pop_n_elems(args);
  push_int(current);
}
#endif

#ifdef HAVE_SETRESUID
void f_setresuid(INT32 args)
{
  INT_TYPE ruid, euid,suid;
  int err;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    Pike_error("setresuid: permission denied.\n");
#endif
  get_all_args("setresuid", args, "%i%i%i", &ruid,&euid,&suid);
 
  err = setresuid(ruid,euid,suid);

  pop_n_elems(args);
  push_int(err);
}
#endif /* HAVE_SETRESUID */

#ifdef HAVE_SETRESGID
void f_setresgid(INT32 args)
{
  INT_TYPE rgid, egid,sgid;
  int err;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    Pike_error("setresgid: permission denied.\n");
#endif
  get_all_args("setresgid", args, "%i%i%i", &rgid,&egid,&sgid);
 
  err = setresgid(rgid,egid,sgid);

  pop_n_elems(args);
  push_int(err);
}
#endif /* HAVE_SETRESGID */


#define f_get(X,Y) void X(INT32 args){ pop_n_elems(args); push_int((INT32)Y()); }

#ifdef HAVE_GETUID
f_get(f_getuid, getuid)
#endif

#ifdef HAVE_GETGID
f_get(f_getgid, getgid)
#endif
 
#ifdef HAVE_GETEUID
f_get(f_geteuid, geteuid)
f_get(f_getegid, getegid)
#endif
f_get(f_getpid, getpid)

#ifdef HAVE_GETPPID
f_get(f_getppid, getppid)
#endif
 
#undef f_get

#ifdef HAVE_CHROOT
/* int chroot(string|object newroot) */
void f_chroot(INT32 args)
{
  int res;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    Pike_error("chroot: permission denied.\n");
#endif

#ifdef HAVE_FCHROOT
  check_all_args("chroot", args, BIT_STRING|BIT_OBJECT, 0);
#else
  check_all_args("chroot", args, BIT_STRING, 0);
#endif /* HAVE_FCHROOT */


#ifdef HAVE_FCHROOT
  if(sp[-args].type == T_STRING)
  {
#endif /* HAVE_FCHROOT */
    res = chroot((char *)sp[-args].u.string->str);
    pop_n_elems(args);
    push_int(!res);
    return;
#ifdef HAVE_FCHROOT
  } else
#if 0 
    if(sp[-args].type == T_OBJECT)
#endif /* 0 */
      {
	int fd;

	apply(sp[-args].u.object, "query_fd", 0);
	fd=sp[-1].u.integer;
	pop_stack();
	res=fchroot(fd);
	pop_n_elems(args);
	push_int(!res);
	return;
      }
#endif /* HAVE_FCHROOT */
}
#endif /* HAVE_CHROOT */
 
#ifdef HAVE_SYSINFO
#  ifdef SI_NODENAME
#    define USE_SYSINFO
#  else
#    ifndef HAVE_UNAME
#      define USE_SYSINFO
#    endif
#  endif
#endif

#ifdef USE_SYSINFO

static struct {
  char *name;
  int command;
} si_fields[] = {
#ifdef SI_SYSNAME
  { "sysname", SI_SYSNAME },
#endif /* SI_SYSNAME */
#ifdef SI_HOSTNAME
  { "nodename", SI_HOSTNAME },
#endif /* SI_HOSTNAME */
#ifdef SI_RELEASE
  { "release", SI_RELEASE },
#endif /* SI_RELEASE */
#ifdef SI_VERSION
  { "version", SI_VERSION },
#endif /* SI_VERSION */
#ifdef SI_MACHINE
  { "machine", SI_MACHINE },
#endif /* SI_MACHINE */
#ifdef SI_ARCHITECTURE
  { "architecture", SI_ARCHITECTURE },
#endif /* SI_ARCHITECTURE */
#ifdef SI_ISALIST
  { "isalist", SI_ISALIST },
#endif /* SI_ISALIST */
#ifdef SI_PLATFORM
  { "platform", SI_PLATFORM },
#endif /* SI_PLATFORM */
#ifdef SI_HW_PROVIDER
  { "hw provider", SI_HW_PROVIDER },
#endif /* SI_HW_PROVIDER */
#ifdef SI_HW_SERIAL
  { "hw serial", SI_HW_SERIAL },
#endif /* SI_HW_SERIAL */
#ifdef SI_SRPC_DOMAIN
  { "srpc domain", SI_SRPC_DOMAIN },
#endif /* SI_SRPC_DOMAIN */
  { "TERMINATOR", 0 }
};

/* Recomended is >257 */
#define PIKE_SI_BUFLEN	512

void f_uname(INT32 args)
{
  char buffer[PIKE_SI_BUFLEN];
  unsigned int i;
  struct svalue *old_sp;

  pop_n_elems(args);

  old_sp = sp;

  for(i=0; i < NELEM(si_fields)-1; i++) {
    long res;
    res = sysinfo(si_fields[i].command, buffer, PIKE_SI_BUFLEN);

    if (res >= 0) {
      push_text(si_fields[i].name);
      /* FIXME: Get the length from the return value? */
      push_text(buffer);
    }
  }

  f_aggregate_mapping(sp - old_sp);
}

#else /* !HAVE_SYSINFO */

#ifdef HAVE_UNAME
void f_uname(INT32 args)
{
  struct svalue *old_sp;
  struct utsname foo;
 
  pop_n_elems(args);
  old_sp = sp;
 
  if(uname(&foo) < 0)
    Pike_error("uname() system call failed.\n");
 
  push_text("sysname");
  push_text(foo.sysname);
 
  push_text("nodename");
  push_text(foo.nodename);
 
  push_text("release");
  push_text(foo.release);
 
  push_text("version");
  push_text(foo.version);
 
  push_text("machine");
  push_text(foo.machine);
 
  f_aggregate_mapping(sp-old_sp);
}
#endif /* HAVE_UNAME */
#endif /* HAVE_SYSINFO */

#if defined(HAVE_UNAME) && (defined(SOLARIS) || !defined(HAVE_GETHOSTNAME))
void f_gethostname(INT32 args)
{
  struct utsname foo;
  pop_n_elems(args);
  if(uname(&foo) < 0)
    Pike_error("uname() system call failed.\n");
  push_text(foo.nodename);
}
#elif defined(HAVE_GETHOSTNAME)
void f_gethostname(INT32 args)
{
  char name[1024];
  pop_n_elems(args);
  gethostname(name, 1024);
  push_text(name);
}
#elif defined(HAVE_SYSINFO) && defined(SI_HOSTNAME)
void f_gethostname(INT32 args)
{
  char name[1024];
  pop_n_elems(args);
  if (sysinfo(SI_HOSTNAME, name, sizeof(name)) < 0) {
    Pike_error("sysinfo() system call failed.\n");
  }
  push_text(name);
}
#endif /* HAVE_UNAME || HAVE_GETHOSTNAME || HAVE_SYSINFO */

/* RFC 1884
 *
 * 2.2 Text Representation of Addresses
 *
 *  There are three conventional forms for representing IPv6 addresses
 *  as text strings: 
 * 
 *   1. The preferred form is x:x:x:x:x:x:x:x, where the 'x's are the
 *      hexadecimal values of the eight 16-bit pieces of the address.
 * 	Examples:
 * 	    FEDC:BA98:7654:3210:FEDC:BA98:7654:3210
 * 	    1080:0:0:0:8:800:200C:417A
 * 	Note that it is not necessary to write the leading zeros in an
 * 	individual field, but there must be at least one numeral in
 * 	every field (except for the case described in 2.).
 *
 *   2. Due to the method of allocating certain styles of IPv6
 * 	addresses, it will be common for addresses to contain long
 * 	strings of zero bits.  In order to make writing addresses
 * 	containing zero bits easier a special syntax is available to
 * 	compress the zeros.  The use of "::" indicates multiple groups
 * 	of 16-bits of zeros.  The "::" can only appear once in an
 * 	address.  The "::" can also be used to compress the leading
 * 	and/or trailing zeros in an address.
 * 	For example the following addresses:
 * 	    1080:0:0:0:8:800:200C:417A  a unicast address
 * 	    FF01:0:0:0:0:0:0:43         a multicast address
 * 	    0:0:0:0:0:0:0:1             the loopback address
 * 	    0:0:0:0:0:0:0:0             the unspecified addresses
 * 	may be represented as:
 * 	    1080::8:800:200C:417A       a unicast address
 * 	    FF01::43                    a multicast address
 * 	    ::1                         the loopback address
 * 	    ::                          the unspecified addresses
 *
 *   3. An alternative form that is sometimes more convenient when
 * 	dealing with a mixed environment of IPv4 and IPv6 nodes is
 * 	x:x:x:x:x:x:d.d.d.d, where the 'x's are the hexadecimal values
 * 	of the six high-order 16-bit pieces of the address, and the 'd's
 * 	are the decimal values of the four low-order 8-bit pieces of the
 * 	address (standard IPv4 representation).  Examples:
 * 	    0:0:0:0:0:0:13.1.68.3
 * 	    0:0:0:0:0:FFFF:129.144.52.38
 * 	or in compressed form:
 * 	    ::13.1.68.3
 * 	    ::FFFF:129.144.52.38
 */
int my_isipnr(char *s)
{
  int e,i;
  for(e=0;e<3;e++)
  {
    i=0;
    while(*s==' ') s++;
    while(*s>='0' && *s<='9') s++,i++;
    if(!i) return 0;
    if(*s!='.') return 0;
    s++;
  }
  i=0;
  while(*s==' ') s++;
  while(*s>='0' && *s<='9') s++,i++;
  if(!i) return 0;
  while(*s==' ') s++;
  if(*s) return 0;
  return 1;
}

int my_isipv6nr(char *s)
{
  int i = 0;
  int field = 0;
  int compressed = 0;
  int is_hex = 0;
  int has_value = 0;

  for(i = 0; s[i]; i++) {
    switch(s[i]) {
    case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
    case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
      is_hex = 1;
      /* FALL_THROUGH */
    case '0':case '1':case '2':case '3':case '4':
    case '5':case '6':case '7':case '8':case '9':
      has_value++;
      if (has_value > 4) {
	/* Too long value field. */
	return 0;
      }
      break;
    case ':':
      if (s[i+1] == ':') {
	if (compressed) {
	  /* Only one compressed range is allowed. */
	  return 0;
	}
	compressed++;
	break;
      } else if (!has_value) {
	/* The first value can only be left out if it starts with '::'. */
	return 0;
      }
      is_hex = 0;
      has_value = 0;
      field++;
      if ((field + compressed) > 7) {
	/* Too many fields */
	return 0;
      }
      break;
    case '.':
      /* Dotted decimal. */
      if (is_hex || !has_value ||
	  (!compressed && (field != 6)) ||
	  (compressed && (field > 6))) {
	/* Hex not allowed in dotted decimal section.
	 * Must have a value before the first '.'.
	 * Must have 6 fields or a compressed range with at most 6 fields
	 * before the dotted decimal section.
	 */
	return 0;
      }
      return my_isipnr(s+i-1);
    default:
      return 0;
    }
  }
  if (((has_value) || (compressed && (s[i-2] == ':'))) &&
      ((compressed && (field < 7)) || (field == 7))) {
    return 1;
  }
  return 0;
}

#ifdef _REENTRANT
#ifdef HAVE_SOLARIS_GETHOSTBYNAME_R

#define GETHOST_DECLARE \
    struct hostent *ret; \
    struct hostent result; \
    char data[2048]; \
    int gh_errno

#define CALL_GETHOSTBYNAME(X) \
    THREADS_ALLOW(); \
    ret=gethostbyname_r((X), &result, data, sizeof(data), &gh_errno); \
    THREADS_DISALLOW()

#define CALL_GETHOSTBYADDR(X,Y,Z) \
    THREADS_ALLOW(); \
    ret=gethostbyaddr_r((X),(Y),(Z), &result, data, sizeof(data), &gh_errno); \
    THREADS_DISALLOW()

#else /* HAVE_SOLARIS_GETHOSTBYNAME_R */
#ifdef HAVE_OSF1_GETHOSTBYNAME_R

#define GETHOST_DECLARE \
    struct hostent *ret; \
    struct hostent result; \
    struct hostent_data data

#define CALL_GETHOSTBYNAME(X) \
    THREADS_ALLOW(); \
    MEMSET((char *)&data,0,sizeof(data)); \
    if(gethostbyname_r((X), &result, &data) < 0) { \
      ret=0; \
    }else{ \
      ret=&result; \
    } \
    THREADS_DISALLOW()

#define CALL_GETHOSTBYADDR(X,Y,Z) \
    THREADS_ALLOW(); \
    MEMSET((char *)&data,0,sizeof(data)); \
    if(gethostbyaddr_r((X),(Y),(Z), &result, &data) < 0) { \
      ret=0; \
    }else{ \
      ret=&result; \
    } \
    THREADS_DISALLOW()

#else /* HAVE_OSF1_GETHOSTBYNAME_R */
static MUTEX_T gethostbyname_mutex;
#define GETHOSTBYNAME_MUTEX_EXISTS

#define GETHOST_DECLARE struct hostent *ret

#define CALL_GETHOSTBYNAME(X) \
    THREADS_ALLOW(); \
    mt_lock(&gethostbyname_mutex); \
    ret=gethostbyname(X); \
    mt_unlock(&gethostbyname_mutex); \
    THREADS_DISALLOW()


#define CALL_GETHOSTBYADDR(X,Y,Z) \
    THREADS_ALLOW(); \
    mt_lock(&gethostbyname_mutex); \
    ret=gethostbyaddr((X),(Y),(Z)); \
    mt_unlock(&gethostbyname_mutex); \
    THREADS_DISALLOW()

#endif /* HAVE_OSF1_GETHOSTBYNAME_R */
#endif /* HAVE_SOLARIS_GETHOSTBYNAME_R */
#else /* _REENTRANT */

#ifdef HAVE_GETHOSTBYNAME

#define GETHOST_DECLARE struct hostent *ret
#define CALL_GETHOSTBYNAME(X) ret=gethostbyname(X)
#define CALL_GETHOSTBYADDR(X,Y,Z) ret=gethostbyaddr((X),(Y),(Z))
#endif

#endif /* REENTRANT */

/* this is used from modules/file, and modules/spider! */
void get_inet_addr(struct sockaddr_in *addr,char *name)
{
  MEMSET((char *)addr,0,sizeof(struct sockaddr_in));

  addr->sin_family = AF_INET;
  if(!strcmp(name,"*"))
  {
    addr->sin_addr.s_addr=htonl(INADDR_ANY);
  }
  else if(my_isipnr(name)) /* I do not entirely trust inet_addr */
  {
    if (((IN_ADDR_T)inet_addr(name)) == ((IN_ADDR_T)-1))
      Pike_error("Malformed ip number.\n");

    addr->sin_addr.s_addr = inet_addr(name);
  }
  else
  {
#ifdef GETHOST_DECLARE
    GETHOST_DECLARE;
    CALL_GETHOSTBYNAME(name);

    if(!ret) {
      if (strlen(name) < 1024) {
	Pike_error("Invalid address '%s'\n",name);
      } else {
	Pike_error("Invalid address\n");
      }
    }

#ifdef HAVE_H_ADDR_LIST
    MEMCPY((char *)&(addr->sin_addr),
	   (char *)ret->h_addr_list[0],
	   ret->h_length);
#else
    MEMCPY((char *)&(addr->sin_addr),
	   (char *)ret->h_addr,
	   ret->h_length);
#endif
#else
    if (strlen(name) < 1024) {
      Pike_error("Invalid address '%s'\n",name);
    } else {
      Pike_error("Invalid address\n");
    }
#endif
  }
}


#ifdef GETHOST_DECLARE
/* array(string|array(string)) gethostbyaddr(string addr) */

static void describe_hostent(struct hostent *hp)
{
  char **p;
  INT32 nelem;

  push_text(hp->h_name);
  
#ifdef HAVE_H_ADDR_LIST
  nelem=0;
  for (p = hp->h_addr_list; *p != 0; p++) {
    struct in_addr in;
 
    MEMCPY(&in.s_addr, *p, sizeof (in.s_addr));
    push_text(inet_ntoa(in));
    nelem++;
  }
  f_aggregate(nelem);
 
  nelem=0;
  for (p = hp->h_aliases; *p != 0; p++) {
    push_text(*p);
    nelem++;
  }
  f_aggregate(nelem);
#else
  {
    struct in_addr in;
    MEMCPY(&in.s_addr, hp->h_addr, sizeof (in.s_addr));
    push_text(inet_ntoa(in));
  }

  f_aggregate(1);
  f_aggregate(0);
#endif /* HAVE_H_ADDR_LIST */
  f_aggregate(3);
}

void f_gethostbyaddr(INT32 args)
{
  u_long addr;
  char *name;
  GETHOST_DECLARE;

  get_all_args("gethostbyaddr", args, "%s", &name);

  if ((int)(addr = inet_addr(name)) == -1) {
    Pike_error("gethostbyaddr(): IP-address must be of the form a.b.c.d\n");
  }
 
  pop_n_elems(args);

  CALL_GETHOSTBYADDR((char *)&addr, sizeof (addr), AF_INET);

  if(!ret) {
    push_int(0);
    return;
  }
 
  describe_hostent(ret);
}  

/* array(array(string)) gethostbyname(string hostname) */ 
void f_gethostbyname(INT32 args)
{
  char *name;
  GETHOST_DECLARE;

  get_all_args("gethostbyname", args, "%s", &name);

  CALL_GETHOSTBYNAME(name);
 
  pop_n_elems(args);
  
  if(!ret) {
    push_int(0);
    return;
  }
  describe_hostent(ret);
}  
#endif /* HAVE_GETHOSTBYNAME */

#ifdef GETHOSTBYNAME_MUTEX_EXISTS
static void cleanup_after_fork(struct callback *cb, void *arg0, void *arg1)
{
  mt_init(&gethostbyname_mutex);
}
#endif

extern void init_passwd(void);


#ifdef HAVE_SLEEP
/* int sleep(int seconds) */

static void f_system_sleep(INT32 args)
{
   INT_TYPE seconds;
   get_all_args("setegid", args, "%i", &seconds);
   if (seconds<0) seconds=0; /* sleep takes unsinged */
   pop_n_elems(args);
   THREADS_ALLOW();
   seconds=(INT_TYPE)sleep( (unsigned int)seconds );
   THREADS_DISALLOW();
   push_int(seconds);
}
#endif /* HAVE_SLEEP */

/* can't do this if we don't know the syntax */
#ifdef SETRLIMIT_SYNTAX_UNKNOWN
#ifdef HAVE_GETRLIMIT
#undef HAVE_GETRLIMIT
#endif
#ifdef HAVE_SETRLIMIT
#undef HAVE_SETRLIMIT
#endif
#endif

#ifdef SETRLIMIT_SYNTAX_BSD43
#define PIKE_RLIM_T int
#endif

#ifdef SETRLIMIT_SYNTAX_STANDARD
#define PIKE_RLIM_T rlim_t
#endif

#if HAVE_GETRLIMIT || HAVE_SETRLIMIT
static struct pike_string *s_cpu=NULL;
static struct pike_string *s_fsize=NULL;
static struct pike_string *s_data=NULL;
static struct pike_string *s_stack=NULL;
static struct pike_string *s_core=NULL;
static struct pike_string *s_rss=NULL;
static struct pike_string *s_nproc=NULL;
static struct pike_string *s_nofile=NULL;
static struct pike_string *s_memlock=NULL;
static struct pike_string *s_as=NULL;
static struct pike_string *s_vmem=NULL;

static void make_rlimit_strings()
{
   MAKE_CONSTANT_SHARED_STRING(s_cpu,"cpu");
   MAKE_CONSTANT_SHARED_STRING(s_fsize,"fsize");
   MAKE_CONSTANT_SHARED_STRING(s_data,"data");
   MAKE_CONSTANT_SHARED_STRING(s_stack,"stack");
   MAKE_CONSTANT_SHARED_STRING(s_core,"core");
   MAKE_CONSTANT_SHARED_STRING(s_rss,"rss");
   MAKE_CONSTANT_SHARED_STRING(s_nproc,"nproc");
   MAKE_CONSTANT_SHARED_STRING(s_nofile,"nofile");
   MAKE_CONSTANT_SHARED_STRING(s_memlock,"memlock");
   MAKE_CONSTANT_SHARED_STRING(s_as,"as");
   MAKE_CONSTANT_SHARED_STRING(s_vmem,"vmem");
}
#endif

#ifdef HAVE_GETRLIMIT
/* array(int) getrlimit(string resource) */
/* mapping(string:array(int)) getrlimits() */

static void f_getrlimit(INT32 args)
{
   struct rlimit rl;
   int res=-1;
   if (!s_cpu) make_rlimit_strings();
   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("getrlimit",1);
   if (sp[-args].type!=T_STRING) 
      SIMPLE_BAD_ARG_ERROR("getrlimit",1,"string");

#ifdef RLIMIT_CPU
   if (sp[-args].u.string==s_cpu)
      res=getrlimit(RLIMIT_CPU,&rl);
#endif
#ifdef RLIMIT_FSIZE
   else if (sp[-args].u.string==s_fsize)
      res=getrlimit(RLIMIT_FSIZE,&rl);
#endif
#ifdef RLIMIT_DATA
   else if (sp[-args].u.string==s_data)
      res=getrlimit(RLIMIT_DATA,&rl);
#endif
#ifdef RLIMIT_STACK
   else if (sp[-args].u.string==s_stack)
      res=getrlimit(RLIMIT_STACK,&rl);
#endif
#ifdef RLIMIT_CORE
   else if (sp[-args].u.string==s_core)
      res=getrlimit(RLIMIT_CORE,&rl);
#endif
#ifdef RLIMIT_RSS
   else if (sp[-args].u.string==s_rss)
      res=getrlimit(RLIMIT_RSS,&rl);
#endif
#ifdef RLIMIT_NPROC
   else if (sp[-args].u.string==s_nproc)
      res=getrlimit(RLIMIT_NPROC,&rl);
#endif
#ifdef RLIMIT_NOFILE
   else if (sp[-args].u.string==s_nofile)
      res=getrlimit(RLIMIT_NOFILE,&rl);
#endif
#ifdef RLIMIT_MEMLOCK
   else if (sp[-args].u.string==s_memlock)
      res=getrlimit(RLIMIT_MEMLOCK,&rl);
#endif
#ifdef RLIMIT_AS
   else if (sp[-args].u.string==s_as)
      res=getrlimit(RLIMIT_AS,&rl);
#endif
#ifdef RLIMIT_VMEM
   else if (sp[-args].u.string==s_vmem)
      res=getrlimit(RLIMIT_VMEM,&rl);
#endif
/* NOFILE is called OFILE on some systems */
#ifdef RLIMIT_OFILE
   else if (sp[-args].u.string==s_nofile)
      res=getrlimit(RLIMIT_OFILE,&rl);
#endif
   else if (sp[-args].u.string==s_cpu      ||
	    sp[-args].u.string==s_fsize    ||
	    sp[-args].u.string==s_data     ||
	    sp[-args].u.string==s_stack    ||
	    sp[-args].u.string==s_core     ||
	    sp[-args].u.string==s_rss      ||
	    sp[-args].u.string==s_nproc    ||
	    sp[-args].u.string==s_nofile   ||
	    sp[-args].u.string==s_memlock  ||
	    sp[-args].u.string==s_vmem     ||
	    sp[-args].u.string==s_as)
   {
/* no such resource on this system */
      rl.rlim_cur=(PIKE_RLIM_T)0;
      rl.rlim_max=(PIKE_RLIM_T)0;
      res=0;
   }
   else
      Pike_error("getrlimit: no such resource\n");

   if (res==-1)
   {
/* this shouldn't happen */
      Pike_error("getrlimit: error; errno=%d\n",errno);
   }

   pop_n_elems(args);
#ifdef RLIM_INFINITY
   if (rl.rlim_cur==RLIM_INFINITY)
      push_int(-1);
   else
#endif
      push_int( (INT_TYPE)rl.rlim_cur );

#ifdef RLIM_INFINITY
   if (rl.rlim_max==RLIM_INFINITY)
      push_int(-1);
   else
#endif
      push_int( (INT_TYPE)rl.rlim_max );

   f_aggregate(2);
}

static void f_getrlimits(INT32 args)
{
   int n=0;
   pop_n_elems(args); /* no args */

   if (!s_cpu) make_rlimit_strings();

#ifdef RLIMIT_CPU
   ref_push_string(s_cpu);
   ref_push_string(s_cpu);
   f_getrlimit(1);
   n+=2;
#endif

#ifdef RLIMIT_FSIZE
   ref_push_string(s_fsize);
   ref_push_string(s_fsize);
   f_getrlimit(1);
   n+=2;
#endif

#ifdef RLIMIT_DATA
   ref_push_string(s_data);
   ref_push_string(s_data);
   f_getrlimit(1);
   n+=2;
#endif

#ifdef RLIMIT_STACK
   ref_push_string(s_stack);
   ref_push_string(s_stack);
   f_getrlimit(1);
   n+=2;
#endif

#ifdef RLIMIT_CORE
   ref_push_string(s_core);
   ref_push_string(s_core);
   f_getrlimit(1);
   n+=2;
#endif

#ifdef RLIMIT_RSS
   ref_push_string(s_rss);
   ref_push_string(s_rss);
   f_getrlimit(1);
   n+=2;
#endif

#ifdef RLIMIT_NPROC
   ref_push_string(s_nproc);
   ref_push_string(s_nproc);
   f_getrlimit(1);
   n+=2;
#endif

#ifdef RLIMIT_NOFILE 
   ref_push_string(s_nofile);
   ref_push_string(s_nofile);
   f_getrlimit(1);
   n+=2;
#else
#ifdef RLIMIT_OFILE 
   ref_push_string(s_nofile);
   ref_push_string(s_nofile);
   f_getrlimit(1);
   n+=2;
#endif
#endif

#ifdef RLIMIT_MEMLOCK
   ref_push_string(s_memlock);
   ref_push_string(s_memlock);
   f_getrlimit(1);
   n+=2;
#endif

#ifdef RLIMIT_AS
   ref_push_string(s_as);
   ref_push_string(s_as);
   f_getrlimit(1);
   n+=2;
#endif
   
#ifdef RLIMIT_VMEM
#if RLIMIT_VMEM != RLIMIT_AS
/* they are alias on some systems, OSF1 for one */
   ref_push_string(s_vmem);
   ref_push_string(s_vmem);
   f_getrlimit(1);
   n+=2;
#endif
#endif
   
   f_aggregate_mapping(n);
}

#endif

#ifdef HAVE_SETRLIMIT
/* int(0..1) setrlimit(string resource, int cur, int max) */

static void f_setrlimit(INT32 args)
{
   struct rlimit rl;
   int res=-1;
   if (!s_cpu) make_rlimit_strings();
   if (args<3)
      SIMPLE_TOO_FEW_ARGS_ERROR("setrlimit",3);
   if (sp[-args].type!=T_STRING) 
      SIMPLE_BAD_ARG_ERROR("setrlimit",1,"string");
   if (sp[1-args].type!=T_INT ||
       sp[1-args].u.integer<-1) 
      SIMPLE_BAD_ARG_ERROR("setrlimit",2,"int(-1..)");
   if (sp[2-args].type!=T_INT ||
       sp[1-args].u.integer<-1) 
      SIMPLE_BAD_ARG_ERROR("setrlimit",3,"int(-1..)");

#ifdef RLIM_INFINITY
   if (sp[1-args].u.integer==-1)
      rl.rlim_cur=RLIM_INFINITY;
   else
#endif
      rl.rlim_cur=(PIKE_RLIM_T)sp[1-args].u.integer;

#ifdef RLIM_INFINITY
   if (sp[2-args].u.integer==-1)
      rl.rlim_max=RLIM_INFINITY;
   else
#endif
      rl.rlim_max=(PIKE_RLIM_T)sp[2-args].u.integer;


#ifdef RLIMIT_CPU
   if (sp[-args].u.string==s_cpu)
      res=setrlimit(RLIMIT_CPU,&rl);
#endif
#ifdef RLIMIT_FSIZE
   else if (sp[-args].u.string==s_fsize)
      res=setrlimit(RLIMIT_FSIZE,&rl);
#endif
#ifdef RLIMIT_DATA
   else if (sp[-args].u.string==s_data)
      res=setrlimit(RLIMIT_DATA,&rl);
#endif
#ifdef RLIMIT_STACK
   else if (sp[-args].u.string==s_stack)
      res=setrlimit(RLIMIT_STACK,&rl);
#endif
#ifdef RLIMIT_CORE
   else if (sp[-args].u.string==s_core)
      res=setrlimit(RLIMIT_CORE,&rl);
#endif
#ifdef RLIMIT_RSS
   else if (sp[-args].u.string==s_rss)
      res=setrlimit(RLIMIT_RSS,&rl);
#endif
#ifdef RLIMIT_NPROC
   else if (sp[-args].u.string==s_nproc)
      res=setrlimit(RLIMIT_NPROC,&rl);
#endif
#ifdef RLIMIT_NOFILE
   else if (sp[-args].u.string==s_nofile)
      res=setrlimit(RLIMIT_NOFILE,&rl);
#endif
#ifdef RLIMIT_MEMLOCK
   else if (sp[-args].u.string==s_memlock)
      res=setrlimit(RLIMIT_MEMLOCK,&rl);
#endif
#ifdef RLIMIT_AS
   else if (sp[-args].u.string==s_as)
      res=setrlimit(RLIMIT_AS,&rl);
#endif
#ifdef RLIMIT_VMEM
   else if (sp[-args].u.string==s_vmem)
      res=setrlimit(RLIMIT_VMEM,&rl);
#endif
/* NOFILE is called OFILE on some systems */
#ifdef RLIMIT_OFILE
   else if (sp[-args].u.string==s_nofile)
      res=setrlimit(RLIMIT_OFILE,&rl);
#endif
   else if (sp[-args].u.string==s_cpu      ||
	    sp[-args].u.string==s_fsize    ||
	    sp[-args].u.string==s_data     ||
	    sp[-args].u.string==s_stack    ||
	    sp[-args].u.string==s_core     ||
	    sp[-args].u.string==s_rss      ||
	    sp[-args].u.string==s_nproc    ||
	    sp[-args].u.string==s_nofile   ||
	    sp[-args].u.string==s_memlock  ||
	    sp[-args].u.string==s_vmem     ||
	    sp[-args].u.string==s_as)
      Pike_error("setrlimit: no %s resource on this system\n",
		 sp[-args].u.string->str);
   else
      Pike_error("setrlimit: no such resource\n");

   pop_n_elems(args);
   if (res==-1)
      push_int(0);
   else
      push_int(1);
}
#endif

#ifdef HAVE_NETINFO_NI_H
/*! @decl array(string) get_netinfo_property(string domain, string path,
 *!                                          string property)
 *!
 *! Queries a NetInfo server for property values at the given path.
 *!
 *! @param domain
 *!  NetInfo domain. Use "." for the local domain.
 *! @param path
 *!  NetInfo path for the property.
 *! @property
 *!  Name of the property to return.
 *! @returns
 *!  An array holding all property values. If the @[path] or @[property]
 *!  cannot be not found 0 is returned instead. If the NetInfo @[domain]
 *!  is not found or cannot be queried an exception is thrown.
 *!
 *! @example
 *!   system.get_netinfo_property(".", "/locations/resolver", "domain");
 *!   ({ 
 *!      "idonex.se"
 *!   })
 *! @endexample
 *!
 *! @note
 *!  Only available on operating systems which have NetInfo libraries
 *!  installed.
 */
static void f_get_netinfo_property(INT32 args)
{
  char         *domain_str, *path_str, *prop_str;
  void         *dom;
  ni_id        dir;
  ni_namelist  prop_list;
  ni_status    res;
  unsigned int i, num_replies;
  
  get_all_args("get_netinfo_property", args, "%s%s%s",
	       &domain_str, &path_str, &prop_str);

  /* open domain */
  num_replies = 0;
  res = ni_open(NULL, domain_str, &dom);
  if (res == NI_OK) {
    res = ni_pathsearch(dom, &dir, path_str);
    if (res == NI_OK) {
      res = ni_lookupprop(dom, &dir, prop_str, &prop_list);
      if (res == NI_OK) {
	for (i = 0; i < prop_list.ni_namelist_len; i++) {
	  push_text(prop_list.ni_namelist_val[i]);
	  num_replies++;
	}
	ni_namelist_free(&prop_list);
      }
    }
    ni_free(dom);
  } else {
    Pike_error("get_netinfo_property: error: %s\n", ni_error(res));
  }
  
  /* make array of all replies; missing properties or invalid directory
     are simply returned as integer 0 */
  if (res == NI_OK)
    f_aggregate(num_replies);
  else
    push_int(0);
  stack_pop_n_elems_keep_top (args);
}
#endif

/*
 * Module linkage
 */

void pike_module_init(void)
{
#ifdef GETHOSTBYNAME_MUTEX_EXISTS
  mt_init(&gethostbyname_mutex);
#endif

  /*
   * From this file:
   */
#ifdef HAVE_LINK
  
/* function(string, string:void) */
  ADD_EFUN("hardlink", f_hardlink,tFunc(tStr tStr,tVoid), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("hardlink", f_hardlink,tFunc(tStr tStr,tVoid), 0, OPT_SIDE_EFFECT);
#endif /* HAVE_LINK */
#ifdef HAVE_SYMLINK
  
/* function(string, string:void) */
  ADD_EFUN("symlink", f_symlink,tFunc(tStr tStr,tVoid), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("symlink", f_symlink,tFunc(tStr tStr,tVoid), 0, OPT_SIDE_EFFECT);
#endif /* HAVE_SYMLINK */
#ifdef HAVE_READLINK
  
/* function(string:string) */
  ADD_EFUN("readlink", f_readlink,tFunc(tStr,tStr), OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("readlink", f_readlink,tFunc(tStr,tStr), 0, OPT_EXTERNAL_DEPEND);
#endif /* HAVE_READLINK */
#ifdef HAVE_RESOLVEPATH
  
/* function(string:string) */
  ADD_EFUN("resolvepath", f_resolvepath,tFunc(tStr,tStr), OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("resolvepath", f_resolvepath,tFunc(tStr,tStr), 0, OPT_EXTERNAL_DEPEND);
#endif /* HAVE_RESOLVEPATH */

  /* function(int|void:int) */
  ADD_EFUN("umask", f_umask, tFunc(tOr(tInt,tVoid),tInt), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("umask", f_umask, tFunc(tOr(tInt,tVoid),tInt), 0, OPT_SIDE_EFFECT);

/* function(string, int:void) */
  ADD_EFUN("chmod", f_chmod,tFunc(tStr tInt,tVoid), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("chmod", f_chmod,tFunc(tStr tInt,tVoid), 0, OPT_SIDE_EFFECT);
#ifdef HAVE_CHOWN
  
/* function(string, int, int:void) */
  ADD_EFUN("chown", f_chown,tFunc(tStr tInt tInt,tVoid), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("chown", f_chown,tFunc(tStr tInt tInt,tVoid), 0, OPT_SIDE_EFFECT);
#endif

#ifdef HAVE_UTIME
  
/* function(string, int, int:void) */
  ADD_EFUN("utime", f_utime,tFunc(tStr tInt tInt,tVoid), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("utime", f_utime,tFunc(tStr tInt tInt,tVoid), 0, OPT_SIDE_EFFECT);
#endif

#ifdef HAVE_INITGROUPS
  
/* function(string, int:void) */
  ADD_EFUN("initgroups", f_initgroups,tFunc(tStr tInt,tVoid), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("initgroups", f_initgroups,tFunc(tStr tInt,tVoid), 0, OPT_SIDE_EFFECT);
#endif /* HAVE_INITGROUPS */
#ifdef HAVE_SETGROUPS
  
/* function(:void) */
  ADD_EFUN("cleargroups", f_cleargroups,tFunc(tNone,tVoid), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("cleargroups", f_cleargroups,tFunc(tNone,tVoid), 0, OPT_SIDE_EFFECT);
  /* NOT Implemented in Pike 0.5 */
  
/* function(array(int):void) */
  ADD_EFUN("setgroups", f_setgroups,tFunc(tArr(tInt),tVoid), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("setgroups", f_setgroups,tFunc(tArr(tInt),tVoid), 0, OPT_SIDE_EFFECT);
#endif /* HAVE_SETGROUPS */
#ifdef HAVE_GETGROUPS
  
/* function(:array(int)) */
  ADD_EFUN("getgroups", f_getgroups,tFunc(tNone,tArr(tInt)), OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("getgroups", f_getgroups,tFunc(tNone,tArr(tInt)), 0, OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETGROUPS */
#ifdef HAVE_INNETGR
/* function(string, string|void, string|void, string|void:int) */
  ADD_EFUN("innetgr", f_innetgr,
	   tFunc(tStr tOr(tStr,tVoid) tOr(tStr,tVoid) tOr(tStr,tVoid), tInt),
	   OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("innetgr", f_innetgr,
	   tFunc(tStr tOr(tStr,tVoid) tOr(tStr,tVoid) tOr(tStr,tVoid), tInt),
	   0, OPT_EXTERNAL_DEPEND);
#endif /* HAVE_INNETGR */
#ifdef HAVE_SETUID
  
/* function(int:void) */
  ADD_EFUN("setuid", f_setuid,tFunc(tInt,tInt), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("setuid", f_setuid,tFunc(tInt,tInt), 0, OPT_SIDE_EFFECT);
#endif
#ifdef HAVE_SETGID
  
/* function(int:void) */
  ADD_EFUN("setgid", f_setgid,tFunc(tInt,tInt), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("setgid", f_setgid,tFunc(tInt,tInt), 0, OPT_SIDE_EFFECT);
#endif
#if defined(HAVE_SETEUID) || defined(HAVE_SETRESUID)
  
/* function(int:int) */
  ADD_EFUN("seteuid", f_seteuid,tFunc(tInt,tInt), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("seteuid", f_seteuid,tFunc(tInt,tInt), 0, OPT_SIDE_EFFECT);
#endif /* HAVE_SETEUID || HAVE_SETRESUID */
#if defined(HAVE_SETEGID) || defined(HAVE_SETRESGID)
  
/* function(int:int) */
  ADD_EFUN("setegid", f_setegid,tFunc(tInt,tInt), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("setegid", f_setegid,tFunc(tInt,tInt), 0, OPT_SIDE_EFFECT);
#endif /* HAVE_SETEGID || HAVE_SETRESGID */


#ifdef HAVE_SETRESUID
  ADD_EFUN("setresuid",f_setresuid,tFunc(tInt tInt tInt, tInt), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("setresuid",f_setresuid,tFunc(tInt tInt tInt, tInt), 0, OPT_SIDE_EFFECT);
#endif
#ifdef HAVE_SETRESGID
  ADD_EFUN("setresgid",f_setresgid,tFunc(tInt tInt tInt, tInt), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("setresgid",f_setresgid,tFunc(tInt tInt tInt, tInt), 0, OPT_SIDE_EFFECT);
#endif

#ifdef HAVE_GETUID
  
/* function(:int) */
  ADD_EFUN("getuid", f_getuid,tFunc(tNone,tInt), OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("getuid", f_getuid,tFunc(tNone,tInt), 0, OPT_EXTERNAL_DEPEND);
#endif

#ifdef HAVE_GETGID
  
/* function(:int) */
  ADD_EFUN("getgid", f_getgid,tFunc(tNone,tInt), OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("getgid", f_getgid,tFunc(tNone,tInt), 0, OPT_EXTERNAL_DEPEND);
#endif
 
#ifdef HAVE_GETEUID
  
/* function(:int) */
  ADD_EFUN("geteuid", f_geteuid,tFunc(tNone,tInt), OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("geteuid", f_geteuid,tFunc(tNone,tInt), 0, OPT_EXTERNAL_DEPEND);
  
/* function(:int) */
  ADD_EFUN("getegid", f_getegid,tFunc(tNone,tInt), OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("getegid", f_getegid,tFunc(tNone,tInt), 0, OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETEUID */
 
  
/* function(:int) */
  ADD_EFUN("getpid", f_getpid,tFunc(tNone,tInt), OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("getpid", f_getpid,tFunc(tNone,tInt), 0, OPT_EXTERNAL_DEPEND);
#ifdef HAVE_GETPPID
  
/* function(:int) */
  ADD_EFUN("getppid", f_getppid,tFunc(tNone,tInt), OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("getppid", f_getppid,tFunc(tNone,tInt), 0, OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETPPID */
 
#ifdef HAVE_GETPGRP
/* function(:int) */
  ADD_EFUN("getpgrp", f_getpgrp, tFunc(tOr(tInt, tVoid), tInt),
                      OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("getpgrp", f_getpgrp, tFunc(tOr(tInt, tVoid), tInt), 0,
                      OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETPGRP */

#ifdef HAVE_SETPGRP
  ADD_EFUN("setpgrp", f_setpgrp, tFunc(tNone, tInt),
	   OPT_SIDE_EFFECT);
  ADD_FUNCTION2("setpgrp", f_setpgrp, tFunc(tNone, tInt), 0,
		OPT_SIDE_EFFECT);
#endif

#ifdef HAVE_GETSID
  ADD_EFUN("getsid", f_getsid, tFunc(tOr(tInt, tVoid), tInt),
                        OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("getsid", f_getsid, tFunc(tOr(tInt, tVoid), tInt), 0,
                        OPT_EXTERNAL_DEPEND);
#endif

#ifdef HAVE_SETSID
  ADD_EFUN("setsid", f_setsid, tFunc(tNone, tInt),
	   OPT_SIDE_EFFECT);
  ADD_FUNCTION2("setsid", f_setsid, tFunc(tNone, tInt), 0,
		OPT_SIDE_EFFECT);
#endif

#if defined(HAVE_PRCTL) && defined(PR_SET_DUMPABLE)
  ADD_FUNCTION2("dumpable", f_dumpable, tFunc(tOr(tInt01, tVoid), tInt01),
		0, OPT_SIDE_EFFECT);
#endif

#ifdef HAVE_GETRLIMIT
  ADD_FUNCTION2("getrlimit", f_getrlimit, tFunc(tString, tArr(tInt)), 
		0, OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("getrlimits", f_getrlimits, 
		tFunc(tNone, tMap(tStr,tArr(tInt))), 
		0, OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_SETRLIMIT
  ADD_FUNCTION2("setrlimit", f_setrlimit, tFunc(tString tInt tInt, tInt01), 
		0, OPT_SIDE_EFFECT);
#endif

#ifdef HAVE_CHROOT 
  
/* function(string|object:int) */
  ADD_EFUN("chroot", f_chroot,tFunc(tOr(tStr,tObj),tInt), 
           OPT_SIDE_EFFECT);
  ADD_FUNCTION2("chroot", f_chroot,tFunc(tOr(tStr,tObj),tInt), 0,
           OPT_SIDE_EFFECT);
#endif /* HAVE_CHROOT */
 
#if defined(HAVE_UNAME) || defined(HAVE_SYSINFO)
  
/* function(:mapping) */
  ADD_EFUN("uname", f_uname,tFunc(tNone,tMapping), OPT_TRY_OPTIMIZE);
  ADD_FUNCTION2("uname", f_uname,tFunc(tNone,tMapping), 0, OPT_TRY_OPTIMIZE);
#endif /* HAVE_UNAME */
 
#if defined(HAVE_GETHOSTNAME) || defined(HAVE_UNAME) || defined(HAVE_SYSINFO)
  
/* function(:string) */
  ADD_EFUN("gethostname", f_gethostname,tFunc(tNone,tStr),OPT_TRY_OPTIMIZE);
  ADD_FUNCTION2("gethostname", f_gethostname,tFunc(tNone,tStr), 0, OPT_TRY_OPTIMIZE);
#endif /* HAVE_GETHOSTNAME || HAVE_UNAME */

#ifdef GETHOST_DECLARE
  
/* function(string:array) */
  ADD_EFUN("gethostbyname", f_gethostbyname,tFunc(tStr,tArray),
           OPT_TRY_OPTIMIZE);
  ADD_FUNCTION2("gethostbyname", f_gethostbyname,tFunc(tStr,tArray), 0,
           OPT_TRY_OPTIMIZE);
  
/* function(string:array) */
  ADD_EFUN("gethostbyaddr", f_gethostbyaddr,tFunc(tStr,tArray),
           OPT_TRY_OPTIMIZE);
  ADD_FUNCTION2("gethostbyaddr", f_gethostbyaddr,tFunc(tStr,tArray), 0,
           OPT_TRY_OPTIMIZE);
#endif /* GETHOST_DECLARE */

  /*
   * From syslog.c:
   */
#ifdef HAVE_SYSLOG
  
/* function(string,int,int:void) */
  ADD_EFUN("openlog", f_openlog,tFunc(tStr tInt tInt,tVoid), OPT_SIDE_EFFECT);
  ADD_FUNCTION("openlog", f_openlog,tFunc(tStr tInt tInt,tVoid), 0);
  
/* function(int,string:void) */
  ADD_EFUN("syslog", f_syslog,tFunc(tInt tStr,tVoid), OPT_SIDE_EFFECT);
  ADD_FUNCTION("syslog", f_syslog,tFunc(tInt tStr,tVoid), 0);
  
/* function(:void) */
  ADD_EFUN("closelog", f_closelog,tFunc(tNone,tVoid), OPT_SIDE_EFFECT);
  ADD_FUNCTION("closelog", f_closelog,tFunc(tNone,tVoid), 0);
#endif /* HAVE_SYSLOG */

#ifdef HAVE_SLEEP
  ADD_FUNCTION("sleep",f_system_sleep,tFunc(tInt,tInt), 0);
#endif /* HAVE_SLEEP */

#ifdef HAVE_NETINFO_NI_H
  /* array(string) get_netinfo_property(string domain, string path,
                                        string property) */
  ADD_FUNCTION("get_netinfo_property", f_get_netinfo_property,
	       tFunc(tStr tStr tStr, tArray), 0);
#endif /* NETINFO */

  init_passwd();

#ifdef GETHOSTBYNAME_MUTEX_EXISTS
  dmalloc_accept_leak(add_to_callback(& fork_child_callback,
				      cleanup_after_fork, 0, 0));
#endif

#ifdef __NT__
  {
    extern void init_nt_system_calls(void);
    init_nt_system_calls();
  }
#endif

  /* errnos */
#include "add-errnos.h"
}

void pike_module_exit(void)
{
#ifdef __NT__
  {
    extern void exit_nt_system_calls(void);
    exit_nt_system_calls();
  }
#endif

#ifdef GETHOSTBYNAME_MUTEX_EXISTS
  mt_destroy(&gethostbyname_mutex);
#endif
}
