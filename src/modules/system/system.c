/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: system.c,v 1.171 2004/09/18 20:50:57 nilsson Exp $
*/

/*
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
#include "pike_security.h"
#include "bignum.h"
#include "pike_rusage.h"
#include "pike_netlib.h"

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

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
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

#include "dmalloc.h"


#define sp Pike_sp

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

/*! @module System
 *!
 *! This module embodies common operating system calls, making them
 *! available to the Pike programmer.
 */

#ifdef HAVE_LINK
/*! @decl void hardlink(string from, string to)
 *!
 *! Create a hardlink named @[to] from the file @[from].
 *!
 *! @note
 *!   This function is not available on all platforms.
 *!
 *! @seealso
 *!   @[symlink()], @[mv()], @[rm()]
 */
void f_hardlink(INT32 args)
{
  char *from;
  char *to;
  int err;

  VALID_FILE_IO("hardlink","write");

  get_all_args("hardlink",args, "%s%s", &from, &to);

  do {
    THREADS_ALLOW_UID();
    err = link(from, to);
    THREADS_DISALLOW_UID();
    if (err >= 0 || errno != EINTR) break;
    check_threads_etc();
  } while (1);

  if (err < 0) {
    report_error("hardlink");
  }
  pop_n_elems(args);
}
#endif /* HAVE_LINK */

#ifdef HAVE_SYMLINK
/*! @decl void symlink(string from, string to)
 *!
 *! Create a symbolic link named @[to] that points to @[from].
 *!
 *! @note
 *!   This function is not available on all platforms.
 *!
 *! @seealso
 *!   @[hardlink()], @[readlink()], @[mv()], @[rm()]
 */
void f_symlink(INT32 args)
{
  char *from;
  char *to;
  int err;

  VALID_FILE_IO("symlink","write");

  get_all_args("symlink",args, "%s%s", &from, &to);

  do {
    THREADS_ALLOW_UID();
    err = symlink(from, to);
    THREADS_DISALLOW_UID();
    if (err >= 0 || errno != EINTR) break;
    check_threads_etc();
  } while (1);

  if (err < 0) {
    report_error("symlink");
  }
  pop_n_elems(args);
}
#endif /* HAVE_SYMLINK */

#ifdef HAVE_READLINK
/*! @decl string readlink(string path)
 *!
 *! Returns what the symbolic link @[path] points to.
 *!
 *! @note
 *!   This function is not available on all platforms.
 *!
 *! @seealso
 *!   @[symlink()]
 */
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

    do {
      THREADS_ALLOW_UID();
      err = readlink(path, buf, buflen);
      THREADS_DISALLOW_UID();
      if (err >= 0 || errno != EINTR) break;
      check_threads_etc();
    } while (1);
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
/*! @decl string resolvepath(string path)
 *!
 *! Resolve all symbolic links of a pathname.
 *!
 *! @note
 *!   This function is not available on all platforms.
 *!
 *! @seealso
 *!   @[readlink()], @[symlink()]
 */
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

    do {
      THREADS_ALLOW_UID();
      err = resolvepath(path, buf, buflen);
      THREADS_DISALLOW_UID();
      if (err >= 0 || errno != EINTR) break;
      check_threads_etc();
    } while (1);
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

/*! @decl int umask(void|int mask)
 *!
 *! Set the current umask to @[mask].
 *!
 *! If @[mask] is not specified the current umask will not be changed.
 *!
 *! @returns
 *!   Returns the old umask setting.
 */
void f_umask(INT32 args)
{
  int oldmask;

  VALID_FILE_IO("umask","status");

  if (args) {
    INT_TYPE setmask;
    get_all_args("umask", args, "%i", &setmask);
    oldmask = umask(setmask);
  }
  else {
    oldmask = umask(0);
    umask(oldmask);
  }

  pop_n_elems(args);
  push_int(oldmask);
}

/*! @decl void chmod(string path, int mode)
 *!
 *! Sets the protection mode of the specified path.
 *!
 *! @note
 *!   Throws errors on failure.
 *!
 *! @seealso
 *!   @[Stdio.File->open()], @[errno()]
 */
void f_chmod(INT32 args)
{
  char *path;
  INT_TYPE mode;
  int err;

  VALID_FILE_IO("chmod","chmod");

  get_all_args("chmod", args, "%s%i", &path, &mode);
  do {
    THREADS_ALLOW_UID();
    err = chmod(path, mode);
    THREADS_DISALLOW_UID();
    if (err >= 0 || errno != EINTR) break;
    check_threads_etc();
  } while (1);
  if (err < 0) {
    report_error("chmod");
  }
  pop_n_elems(args);
}

#ifdef HAVE_CHOWN
/*! @decl void chown(string path, int uid, int gid)
 *!
 *! Sets the owner and group of the specified path.
 *!
 *! @note
 *!   Throws errors on failure.
 *!
 *!   This function is not available on all platforms.
 */
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
  do {
    THREADS_ALLOW_UID();
    err = chown(path, uid, gid);
    THREADS_DISALLOW_UID();
    if (err >= 0 || errno != EINTR) break;
    check_threads_etc();
  } while (1);
  if (err < 0) {
    report_error("chown");
  }
  pop_n_elems(args);
}
#endif

#ifdef HAVE_UTIME
/*! @decl void utime(string path, int atime, int mtime)
 *!
 *! Set the last access time and last modification time for the
 *! path @[path] to @[atime] and @[mtime] repectively.
 *!
 *! @note
 *!   Throws errors on failure.
 *!
 *!   This function is not available on all platforms.
 */
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
  do {
    THREADS_ALLOW_UID();
    err = utime(path, &b);
    THREADS_DISALLOW_UID();
    if (err >= 0 || errno != EINTR) break;
    check_threads_etc();
  } while (1);
  if (err < 0) {
    report_error("utime");
  }
  pop_n_elems(args);
}
#endif

#ifdef HAVE_INITGROUPS
/*! @decl void initgroups(string name, int base_gid)
 *!
 *! Initializes the supplemental group access list according to the system
 *! group database. @[base_gid] is also added to the group access
 *! list.
 *!
 *! @note
 *!   Throws errors on failure.
 *!
 *!   This function is not available on all platforms.
 *!
 *! @seealso
 *!   @[setuid()], @[getuid()], @[setgid()], @[getgid()], @[seteuid()],
 *!   @[geteuid()], @[setegid()], @[getegid()], @[getgroups()], @[setgroups()]
 */
void f_initgroups(INT32 args)
{
  char *user;
  int err;
  INT_TYPE group;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    Pike_error("initgroups: permission denied.\n");
#endif

  get_all_args("initgroups", args, "%s%i", &user, &group);
  err = initgroups(user, group);
  if (err < 0) {
    report_error("initgroups");
  }
  pop_n_elems(args);
}
#endif /* HAVE_INITGROUPS */

#ifdef HAVE_SETGROUPS
/*! @decl void cleargroups()
 *!
 *! Clear the supplemental group access list.
 *!
 *! @note
 *!   Throws errors on failure.
 *!
 *!   This function is not available on all platforms.
 *!
 *! @seealso
 *!   @[setgroups()], @[initgroups()], @[getgroups()]
 */
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

/*! @decl void setgroups(array(int) gids)
 *!
 *! Set the supplemental group access list for this process.
 *!
 *! @note
 *!   Throws errors on failure.
 *!
 *!   This function is not available on all platforms.
 *!
 *! @seealso
 *!   @[initgroups()], @[cleargroups()], @[getgroups()],
 *!   @[getgid()], @[getgid()], @[getegid()], @[setegid()]
 */
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
    gids = (gid_t *)alloca(arr->size * sizeof(gid_t));
  } else {
    gids = safeguard;
  }

  for (i=0; i < size; i++) {
    if (arr->item[i].type != T_INT) {
      Pike_error("setgroups(): Bad element %d in array (expected int)\n", i);
    }
    gids[i] = arr->item[i].u.integer;
  }

  pop_n_elems(args);

  err = setgroups(size, gids);
  if (err < 0) {
    report_error("setgroups");
  }
}
#endif /* HAVE_SETGROUPS */

#ifdef HAVE_GETGROUPS
/*! @decl array(int) getgroups()
 *!
 *! Get the current supplemental group access list for this process.
 *!
 *! @note
 *!   Throws errors on failure.
 *!
 *!   This function is not available on all platforms.
 *!
 *! @seealso
 *!   @[setgroups()], @[cleargroups()], @[initgroups()],
 *!   @[getgid()], @[getgid()], @[getegid()], @[setegid()]
 */
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
/*! @decl int(0..1) innetgr(string netgroup, string|void machine, @
 *!                   string|void user, string|void domain)
 *!
 *! Searches for matching entries in the netgroup database (usually
 *! @tt{/etc/netgroup@}). If any of the @[machine], @[user] or @[domain]
 *! arguments are zero or missing, those fields will match any
 *! value in the selected @[netgroup].
 *!
 *! @note
 *!  This function isn't available on all platforms.
 */
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
/*! @decl int setuid(int uid)
 *!
 *! Sets the real user ID, effective user ID and saved user ID to @[uid].
 *!
 *! @returns
 *!   Returns the current errno.
 *!
 *! @note
 *!   This function isn't available on all platforms.
 *!
 *! @seealso
 *!   @[getuid()], @[setgid()], @[getgid()], @[seteuid()], @[geteuid()],
 *!   @[setegid()], @[getegid()]
 */
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
    if(pw==0)
      Pike_error("No \"nobody\" user on this system.\n");
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
/*! @decl int setgid(int gid)
 *!
 *! Sets the real group ID, effective group ID and saved group ID to @[gid].
 *! If @[gid] is @expr{-1@} the uid for "nobody" will be used.
 *!
 *! @throws
 *! Throws an error if no "nobody" user when @[gid] is @expr{-1@}.
 *!
 *! @returns
 *!   Returns the current errno.
 *!
 *! @note
 *!   This function is not available on all platforms.
 *!
 *! @seealso
 *!   @[getuid()], @[setuid()], @[getgid()], @[seteuid()], @[geteuid()],
 *!   @[setegid()], @[getegid()]
 */
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
    if(pw==0)
      Pike_error("No \"nobody\" user on this system.\n");
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
/*! @decl int seteuid(int euid)
 *!
 *! Set the effective user ID to @[euid]. If @[euid] is
 *! @expr{-1@} the uid for "nobody" will be used.
 *!
 *! @returns
 *! Returns the current errno.
 *!
 *! @throws
 *! Throws an error if there is no
 *! "nobody" user when @[euid] is @expr{-1@}.
 *!
 *! @note
 *! This function isn't available on all platforms.
 */
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
    if(pw==0)
      Pike_error("No \"nobody\" user on this system.\n");
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
/*! @decl int setegid(int egid)
 *!
 *! Set the effective group ID to @[egid]. If @[egid] is
 *! @expr{-1@} the uid for "nobody" will be used.
 *!
 *! @returns
 *! Returns the current errno.
 *!
 *! @throws
 *! Throws an error if there is no "nobody" user when
 *! @[egid] is @expr{-1@}.
 *!
 *! @note
 *! This function isn't available on all platforms.
 */
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
    if(pw==0)
      Pike_error("No \"nobody\" user on this system.\n");
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
/*! @decl int getpgrp(int|void pid)
 *!
 *! Get the process group id for the process @[pid].
 *! With no argguments or with 'pid' equal to zero,
 *! returns the process group ID of this process.
 *!
 *! @note
 *!   Not all platforms support getting the process group for other processes.
 *!
 *!   Not supported on all platforms.
 *!
 *! @seealso
 *!   @[getpid], @[getppid]
 */
void f_getpgrp(INT32 args)
{
  int pid = 0;
  int pgid = 0;

  if (args) {
    if (sp[-args].type != T_INT) {
      SIMPLE_BAD_ARG_ERROR("getpgrp", 1, "int");
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
/*! @decl int setpgrp()
 *!
 *! Make this process a process group leader.
 *!
 *! @note
 *!   Not supported on all platforms.
 */
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
/*! @decl int getsid(int|void pid)
 *!   Get the process session ID for the given process. If pid is
 *!   not specified, the session ID for the current process will
 *!   be returned.
 *! @note
 *!   This function is not available on all platforms.
 *! @throws
 *!   Throws an error if the system call fails.
 *! @seealso
 *!   @[getpid], @[getpgrp], @[setsid]
 */
void f_getsid(INT32 args)
{
  int pid = 0;
  if (args >= 1 && sp[-args].type != T_INT)
    SIMPLE_BAD_ARG_ERROR("getsid", 1, "int");
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
/*! @decl int setsid()
 *!   Set a new process session ID for the current process, and return it.
 *! @note
 *!   This function isn't available on all platforms.
 *! @throws
 *!   Throws an error if the system call fails.
 *! @seealso
 *!   @[getpid], @[setpgrp], @[getsid]
 */
void f_setsid(INT32 args)
{
  int pid;
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
 *!   Returns @expr{1@} if this process currently is capable of dumping core,
 *!   and @expr{0@} (zero) if not.
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
      Pike_error("Failed to set dumpable state to %"PRINTPIKEINT"d. "
		 "errno:%d\n", val, err);
    }
  }
  pop_n_elems(args);
  push_int(current);
}
#endif

#ifdef HAVE_SETRESUID
/*! @decl int setresuid(int ruid, int euid, int suid)
 *!
 *! Sets the real, effective and saved set-user-ID to @[ruid],
 *! @[euid] and @[suid] respectively.
 *!
 *! @returns
 *! Returns zero on success and errno on failure.
 */
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
/*! @decl int setresgid(int rgid, int egid, int sgid)
 *!
 *! Sets the real, effective and saved group ID to @[rgid],
 *! @[egid] and @[sgid] respectively.
 *!
 *! @returns
 *! Returns zero on success and errno on failure.
 */
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
/*! @decl int getuid()
 *!   Get the real user ID.
 *! @seealso
 *!   @[setuid], @[setgid], @[getgid], @[seteuid],
 *!   @[geteuid], @[setegid], @[getegid]
 */
f_get(f_getuid, getuid)
#endif

#ifdef HAVE_GETGID
/*! @decl int getgid()
 *!   Get the real group ID.
 *! @seealso
 *!   @[setuid], @[getuid], @[setgid],
 *!   @[seteuid], @[geteuid], @[getegid], @[setegid]
 */
f_get(f_getgid, getgid)
#endif
 
#ifdef HAVE_GETEUID
/*! @decl int geteuid()
 *!   Get the effective user ID.
 *! @seealso
 *!   @[setuid], @[getuid], @[setgid], @[getgid],
 *!   @[seteuid], @[getegid], @[setegid]
 */
f_get(f_geteuid, geteuid)

/*! @decl int getegid()
 *!   Get the effective group ID.
 *! @seealso
 *!   @[setuid], @[getuid], @[setgid], @[getgid],
 *!   @[seteuid], @[geteuid], @[setegid]
 */
f_get(f_getegid, getegid)
#endif

/*! @decl int getpid()
 *!   Returns the process ID of this process.
 *! @seealso
 *!   @[getppid], @[getpgrp]
 */
f_get(f_getpid, getpid)

#ifdef HAVE_GETPPID
/*! @decl int getppid()
 *!  Returns the process ID of the parent process.
 *! @seealso
 *!   @[getpid], @[getpgrp]
 */
f_get(f_getppid, getppid)
#endif
 
#undef f_get

#ifdef HAVE_CHROOT
/*! @decl int chroot(string newroot)
 *! @decl int chroot(Stdio.File newroot)
 *!
 *! Changes the root directory for this process to the indicated directory.
 *!
 *! @returns
 *!   A nonzero value is returned if the call is successful. If
 *!   there's an error then zero is returned and @[errno] is set
 *!   appropriately.
 *!
 *! @note
 *!   Since this function modifies the directory structure as seen from
 *!   Pike, you have to modify the environment variables PIKE_MODULE_PATH
 *!   and PIKE_INCLUDE_PATH to compensate for the new root-directory.
 *!
 *!   This function only exists on systems that have the chroot(2)
 *!   system call.
 *!
 *!   The second variant only works on systems that also have
 *!   the fchroot(2) system call.
 */
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
#  ifdef SI_HOSTNAME
#    define USE_SYSINFO
#  else
#    ifndef HAVE_UNAME
#      define USE_SYSINFO
#    endif
#  endif
#endif

#ifdef USE_SYSINFO

static const struct {
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

/*! @decl mapping(string:string) uname()
 *!
 *! Get operating system information.
 *!
 *! @returns
 *!   The resulting mapping contains the following fields:
 *!   @mapping
 *!     @member string "sysname"
 *!       Operating system name.
 *!     @member string "nodename"
 *!       Hostname.
 *!     @member string "release"
 *!       Operating system release.
 *!     @member string "version"
 *!       Operating system version.
 *!     @member string "machine"
 *!       Hardware architecture.
 *!     @member string "architecture"
 *!       Basic instruction set architecture.
 *!     @member string "isalist"
 *!       List of upported instruction set architectures.
 *!       Usually space-separated.
 *!     @member string "platform"
 *!       Specific model of hardware.
 *!     @member string "hw provider"
 *!       Manufacturer of the hardware.
 *!     @member string "hw serial"
 *!       Serial number of the hardware.
 *!     @member string "srpc domain"
 *!       Secure RPC domain.
 *!   @endmapping
 *!
 *! @note
 *!   This function only exists on systems that have the uname(2) or
 *!   sysinfo(2) system calls.
 *!
 *!   Only the first five elements are always available.
 */
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

/*! @decl string gethostname()
 *!
 *! Returns a string with the name of the host.
 *!
 *! @note
 *!   This function only exists on systems that have the gethostname(2)
 *!   or uname(2) system calls.
 */
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
  int i;
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

#ifdef HAVE_SOLARIS_GETSERVBYNAME_R

#define GETSERV_DECLARE \
    struct servent *ret; \
    struct servent result; \
    char data[2048]

#define CALL_GETSERVBYNAME(X,Y) \
    THREADS_ALLOW(); \
    ret=getservbyname_r((X), (Y), &result, data, sizeof(data)); \
    THREADS_DISALLOW()

#else /* HAVE_SOLARIS_GETSERVBYNAME_R */
#ifdef HAVE_OSF1_GETSERVBYNAME_R

#define GETSERV_DECLARE \
    struct servent *ret; \
    struct servent result; \
    struct servent_data data

#define CALL_GETSERVBYNAME(X,Y) \
    THREADS_ALLOW(); \
    MEMSET((char *)&data,0,sizeof(data)); \
    if(getservbyname_r((X), (Y), &result, &data) < 0) { \
      ret=0; \
    }else{ \
      ret=&result; \
    } \
    THREADS_DISALLOW()

#else /* HAVE_OSF1_GETSERVBYNAME_R */
static MUTEX_T getservbyname_mutex;
#define GETSERVBYNAME_MUTEX_EXISTS

#define GETSERV_DECLARE struct servent *ret

#define CALL_GETSERVBYNAME(X,Y) \
    THREADS_ALLOW(); \
    mt_lock(&getservbyname_mutex); \
    ret=getservbyname(X,Y); \
    mt_unlock(&getservbyname_mutex); \
    THREADS_DISALLOW()

#endif /* HAVE_OSF1_GETSERVBYNAME_R */
#endif /* HAVE_SOLARIS_GETSERVBYNAME_R */

#else /* _REENTRANT */

#ifdef HAVE_GETHOSTBYNAME

#define GETHOST_DECLARE struct hostent *ret
#define CALL_GETHOSTBYNAME(X) ret=gethostbyname(X)
#define CALL_GETHOSTBYADDR(X,Y,Z) ret=gethostbyaddr((X),(Y),(Z))
#endif

#ifdef HAVE_GETSERVBYNAME
#define GETSERV_DECLARE struct servent *ret
#define CALL_GETSERVBYNAME(X,Y) ret=getservbyname(X,Y)
#endif

#endif /* REENTRANT */

/* this is used from modules/file, and modules/spider! */
int get_inet_addr(PIKE_SOCKADDR *addr,char *name,char *service, INT_TYPE port, int udp)
{
#ifdef HAVE_GETADDRINFO
  struct addrinfo hints = { 0, PF_UNSPEC, 0, 0, 0, NULL, NULL, NULL, }, *res;
  char servnum_buf[200];
#endif /* HAVE_GETADDRINFO */

  MEMSET((char *)addr,0,sizeof(PIKE_SOCKADDR));
  if(name && !strcmp(name,"*"))
    name = NULL;

#ifdef HAVE_GETADDRINFO
  if(!name) {
    hints.ai_flags = AI_PASSIVE;
    /* Avoid creating an IPv6 address for "*". */
    /* For IN6ADDR_ANY, use "::". */
    hints.ai_family = PF_INET;
  }
  hints.ai_protocol = (udp? IPPROTO_UDP:IPPROTO_TCP);
  if(!service)
    sprintf(servnum_buf, "%"PRINTPIKEINT"d", (port<0? 0:port));
  if(!getaddrinfo(name, (service? service : servnum_buf), &hints, &res)) {
    struct addrinfo *p;
    size_t addr_len=0;
    for(p=res; p; p=p->ai_next)
      if(p->ai_addrlen > 0 && p->ai_addrlen <= sizeof(*addr))
	break;
    if(p && p->ai_addr)
      MEMCPY((char *)addr, (char *)p->ai_addr, addr_len = p->ai_addrlen);
    freeaddrinfo(res);
    if(addr_len)
      return addr_len;
  }
#endif /* HAVE_GETADDRINFO */
  
  SOCKADDR_FAMILY(*addr) = AF_INET;
  if(!name)
  {
    addr->ipv4.sin_addr.s_addr=htonl(INADDR_ANY);
  }
  else if(my_isipnr(name)) /* I do not entirely trust inet_addr */
  {
    if (((IN_ADDR_T)inet_addr(name)) == ((IN_ADDR_T)-1))
      Pike_error("Malformed ip number.\n");

    addr->ipv4.sin_addr.s_addr = inet_addr(name);
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

    SOCKADDR_FAMILY(*addr) = ret->h_addrtype;

#ifdef HAVE_H_ADDR_LIST
    MEMCPY((char *)SOCKADDR_IN_ADDR(*addr),
	   (char *)ret->h_addr_list[0],
	   ret->h_length);
#else
    MEMCPY((char *)SOCKADDR_IN_ADDR(*addr),
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

  if(service) {
#ifdef GETSERV_DECLARE
    GETSERV_DECLARE;
    CALL_GETSERVBYNAME(service, (udp? "udp":"tcp"));

    if(!ret) {
      if (strlen(service) < 1024) {
	Pike_error("Invalid service '%s'\n",service);
      } else {
	Pike_error("Invalid service\n");
      }
    }

    addr->ipv4.sin_port = ret->s_port;
#else
    if (strlen(service) < 1024) {
      Pike_error("Invalid service '%s'\n",service);
    } else {
      Pike_error("Invalid service\n");
    }
#endif
  } else if(port >= 0)
    addr->ipv4.sin_port = htons((unsigned INT16)port);
  else
    addr->ipv4.sin_port = 0;

  return (SOCKADDR_FAMILY(*addr) == AF_INET? sizeof(addr->ipv4):sizeof(*addr));
}


#ifdef GETHOST_DECLARE

static void describe_hostent(struct hostent *hp)
{
  push_text(hp->h_name);
  
#ifdef HAVE_H_ADDR_LIST
  {
    char **p;
    INT32 nelem = 0;

    for (p = hp->h_addr_list; *p != 0; p++) {
#ifdef HAVE_INET_NTOP
      char buffer[64];

      push_text(inet_ntop(hp->h_addrtype, *p, buffer, sizeof(buffer)));
#else
      struct in_addr in;
 
      MEMCPY(&in.s_addr, *p, sizeof (in.s_addr));
      push_text(inet_ntoa(in));
#endif
      nelem++;
    }

    f_aggregate(nelem);
 
    nelem=0;
    for (p = hp->h_aliases; *p != 0; p++) {
      push_text(*p);
      nelem++;
    }
    f_aggregate(nelem);
  }
#else
  {
#ifdef HAVE_INET_NTOP
    char buffer[64];

    push_text(inet_ntop(hp->h_addrtype, hp->h_addr, buffer, sizeof(buffer)));
#else
    struct in_addr in;
    MEMCPY(&in.s_addr, hp->h_addr, sizeof (in.s_addr));
    push_text(inet_ntoa(in));
#endif
  }

  f_aggregate(1);
  f_aggregate(0);
#endif /* HAVE_H_ADDR_LIST */
  f_aggregate(3);
}

/*! @decl array(string|array(string)) gethostbyaddr(string addr)
 *!
 *! Returns an array with information about the specified IP address.
 *!
 *! @returns
 *!   The returned array contains the same information as that returned
 *!   by @[gethostbyname()].
 *!
 *! @note
 *!   This function only exists on systems that have the gethostbyaddr(2)
 *!   or similar system call.
 *!
 *! @seealso
 *!   @[gethostbyname()]
 */
void f_gethostbyaddr(INT32 args)
{
  IN_ADDR_T addr;
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

/*! @decl array(string|array(string)) gethostbyname(string hostname)
 *!
 *! Returns an array with information about the specified host.
 *!
 *! @returns
 *!   The returned array contains the following:
 *!   @array
 *!     @elem string hostname
 *!       Name of the host.
 *!     @elem array(string) ips
 *!       Array of IP numbers for the host.
 *!     @elem array(string) aliases
 *!       Array of alternative names for the host.
 *!   @endarray
 *!
 *! @note
 *!   This function only exists on systems that have the gethostbyname(2)
 *!   or similar system call.
 *!
 *! @seealso
 *!   @[gethostbyaddr()]
 */
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

#if defined(GETHOSTBYNAME_MUTEX_EXISTS) || defined(GETSERVBYNAME_MUTEX_EXISTS)
static void cleanup_after_fork(struct callback *cb, void *arg0, void *arg1)
{
#ifdef GETHOSTBYNAME_MUTEX_EXISTS
  mt_init(&gethostbyname_mutex);
#endif
#ifdef GETSERVBYNAME_MUTEX_EXISTS
  mt_init(&getservbyname_mutex);
#endif
}
#endif

extern void init_passwd(void);
extern void init_system_memory(void);


#ifdef HAVE_SLEEP

/*! @decl int sleep(int seconds)
 *!
 *! Call the system sleep() function.
 *!
 *! This is not to be confused with the global function @[predef::sleep()]
 *! that does more elaborate things and can sleep with better precision
 *! (although dependant on a normal functioning system clock).
 *!
 *! @note
 *!   The system's sleep function often utilizes the alarm(2) call and might
 *!   not be perfectly thread safe in combination with simultaneous
 *!   sleep()'s or alarm()'s. It might also be interrupted by other signals.
 *!
 *!   If you don't need it to be independant of the system clock, use
 *!   @[predef::sleep()] instead.
 *!
 *!   May not be present; only exists if the function exists in the 
 *!   current system.
 *!
 *! @seealso
 *!   @[predef::sleep()] @[usleep()] @[nanosleep()]
 */
static void f_system_sleep(INT32 args)
{
   INT_TYPE seconds;
   get_all_args("sleep", args, "%i", &seconds);
   if (seconds<0) seconds=0; /* sleep takes unsinged */
   pop_n_elems(args);
   THREADS_ALLOW();
   seconds=(INT_TYPE)sleep( (unsigned int)seconds );
   THREADS_DISALLOW();
   push_int(seconds);
}
#endif /* HAVE_SLEEP */

#ifdef HAVE_USLEEP

/*! @decl void usleep(int usec)
 *!
 *! Call the system usleep() function. 
 *!
 *! This is not to be confused with the global function @[predef::sleep()]
 *! that does more elaborate things and can sleep with better precision
 *! (although dependant on a normal functioning system clock).
 *!
 *! @note
 *!   The system's usleep function often utilizes the alarm(2) call and might
 *!   not be perfectly thread safe in combination with simultaneous
 *!   sleep()'s or alarm()'s. It might also be interrupted by other signals.
 *!
 *!   If you don't need it to be independant of the system clock, use
 *!   @[predef::sleep()] instead.
 *!
 *!   May not be present; only exists if the function exists in the 
 *!   current system.
 *!
 *! @seealso
 *!   @[predef::sleep()] @[sleep()] @[nanosleep()]
 */
static void f_system_usleep(INT32 args)
{
   INT_TYPE usec;
   get_all_args("usleep", args, "%i", &usec);
   if (usec<0) usec=0; /* sleep takes unsinged */
   pop_n_elems(args);
   THREADS_ALLOW();
   usleep( (unsigned int)usec );
   THREADS_DISALLOW();
   push_int(0);
}
#endif /* HAVE_USLEEP */

#ifdef HAVE_NANOSLEEP

/*! @decl float nanosleep(int|float seconds)
 *!
 *! Call the system nanosleep() function. 
 *!
 *! This is not to be confused with the global function @[predef::sleep()]
 *! that does more elaborate things and can sleep with better precision
 *! (although dependant on a normal functioning system clock).
 *!
 *! Returns the remaining time to sleep (as the system function does).
 *!
 *! @seealso
 *!   @[predef::sleep()] @[sleep()] @[usleep()]
 *!
 *! @note
 *!   May not be present; only exists if the function exists in the 
 *!   current system.
 */
static void f_system_nanosleep(INT32 args)
{
   struct timespec req,rem;
   FLOAT_TYPE sec;

   get_all_args("nanosleep", args, "%F", &sec);
   if (sec<0.0) sec=0.0; /* unsigned */
   pop_n_elems(args);
   THREADS_ALLOW();

   req.tv_sec=(time_t)sec;
   req.tv_nsec=(long)((sec-req.tv_sec)*1e9);
   rem.tv_sec=0;
   rem.tv_nsec=0;

   nanosleep(&req,&rem);
   THREADS_DISALLOW();

   push_float(rem.tv_sec+rem.tv_nsec*1e-9);
}
#endif /* HAVE_NANOSLEEP */


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

#if defined(HAVE_GETRLIMIT) || defined(HAVE_SETRLIMIT)
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

static void make_rlimit_strings(void)
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
#endif /* HAVE_GETRLIMIT || HAVE_SETRLIMIT */

#ifdef HAVE_GETRLIMIT
/*! @decl array(int) getrlimit(string resource)
 *! Returns the current process limitation for the selected @[resource].
 *!
 *! @param resource
 *! @string
 *!   @value cpu
 *!     The CPU time limit in seconds.
 *!   @value fsize
 *!     The maximum size of files the process may create.
 *!   @value data
 *!     The maximum size of the process's data segment.
 *!   @value stack
 *!     The maximum size of process stack, in bytes.
 *!   @value core
 *!
 *!   @value rss
 *!     Specifies the limit of pages the process's resident set.
 *!   @value nproc
 *!     The maximum number of processes that can be created for
 *!     the real user ID of the calling process.
 *!   @value nofile
 *!     The maximum number of file descriptors the process can
 *!     open, +1.
 *!   @value memlock
 *!     The maximum number of bytes of virtual memory that
 *!     may be locked into RAM.
 *!   @value as
 *!   @value vmem
 *! @endstring
 *!
 *! @returns
 *! @array
 *!   @elem int 0
 *!     The soft limit for the resource.
 *!     @expr{-1@} means no limit.
 *!   @elem int 1
 *!     The hard limit for the resource.
 *!     @expr{-1@} means no limit.
 *! @endarray
 *!
 *! @note
 *!   This function nor all the resources are available on all systems.
 *!
 *! @seealso
 *!   @[getrlimits], @[setrlimit]
 */
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

/*! @decl mapping(string:array(int)) getrlimits()
 *! Returns all process limits in a mapping.
 *!
 *! @seealso
 *!   @[getrlimit], @[setrlimit]
 */
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
/*! @decl int(0..1) setrlimit(string resource, int soft, int hard)
 *! Sets the @[soft] and the @[hard] process limit on a @[resource].
 *! @seealso
 *!   @[getrlimit], @[getrlimits]
 */
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

/*! @decl constant ITIMER_REAL
 *!  Identifier for a timer that decrements in real time.
 *! @seealso
 *!   @[setitimer], @[getitimer]
 */

/*! @decl constant ITIMER_VIRTUAL
 *!  Identifier for a timer that decrements only when the
 *!  process is executing.
 *! @seealso
 *!   @[setitimer], @[getitimer]
 */

/*! @decl constant ITIMER_PROF
 *!  Identifier for a timer that decrements both when the process
 *!  is executing and when the system is executing on behalf of the
 *!  process.
 *! @seealso
 *!   @[setitimer], @[getitimer]
 */

#ifdef HAVE_SETITIMER
/*! @decl float setitimer(int timer, int|float value)
 *! Sets the @[timer] to the supplied @[value]. Returns the
 *! current timer interval.
 *! @param timer
 *!   One of @[ITIMER_REAL], @[ITIMER_VIRTUAL] and @[ITIMER_PROF].
 */
void f_system_setitimer(INT32 args)
{
   FLOAT_TYPE interval;
   INT_TYPE what;
   int res;
   struct itimerval itimer,otimer;

   otimer.it_value.tv_usec=0;
   otimer.it_value.tv_sec=0;

   get_all_args("setitimer",args,"%+%F",&what,&interval);

   if (interval<0.0)
      SIMPLE_BAD_ARG_ERROR("setitimer",2,"positive or zero int or float");
   else if (interval==0.0)
      res=setitimer( (int)what,NULL,&otimer );
   else
   {
      itimer.it_value.tv_usec=(int)((interval-(int)interval)*1000000);
      itimer.it_value.tv_sec=(int)interval;
      itimer.it_interval=itimer.it_value;

      res=setitimer((int)what,&itimer,&otimer);
   }

   if (res==-1)
   {
      switch (errno)
      {
	 case EINVAL:
	    Pike_error("setitimer: invalid timer %"PRINTPIKEINT"d\n",what);
	    break;
	 default:
	    Pike_error("setitimer: unknown error (errno=%d)\n",errno);
	    break;
      }
   }

   pop_n_elems(args);
   push_float(otimer.it_interval.tv_sec+otimer.it_interval.tv_usec*0.000001);
}
#endif

#ifdef HAVE_GETITIMER
/*! @decl array(float) getitimer(int timer)
 *! Shows the state of the selected @[timer].
 *!
 *! @returns
 *! @array
 *!   @elem float 0
 *!     The interval of the timer.
 *!   @elem float 1
 *!     The value of the timer.
 *! @endarray
 *!
 *! @param timer
 *!   One of @[ITIMER_REAL], @[ITIMER_VIRTUAL] and @[ITIMER_PROF].
 */
void f_system_getitimer(INT32 args)
{
   INT_TYPE what;
   struct itimerval otimer;

   otimer.it_value.tv_usec=0;
   otimer.it_value.tv_sec=0;
   otimer.it_interval.tv_usec=0;
   otimer.it_interval.tv_sec=0;

   get_all_args("getitimer",args,"%+",&what);

   if (getitimer((int)what,&otimer)==-1)
   {
      switch (errno)
      {
	 case EINVAL:
	    Pike_error("getitimer: invalid timer %"PRINTPIKEINT"d\n",what);
	    break;
	 default:
	    Pike_error("getitimer: unknown error (errno=%d)\n",errno);
	    break;
      }
   }

   pop_n_elems(args);
   push_float(otimer.it_interval.tv_sec+otimer.it_interval.tv_usec*0.000001);
   push_float(otimer.it_value.tv_sec+otimer.it_value.tv_usec*0.000001);
   f_aggregate(2);
}
#endif

#ifdef HAVE_NETINFO_NI_H
/*! @decl array(string) get_netinfo_property(string domain, string path, @
 *!                                          string property)
 *!
 *! Queries a NetInfo server for property values at the given path.
 *!
 *! @param domain
 *!  NetInfo domain. Use "." for the local domain.
 *! @param path
 *!  NetInfo path for the property.
 *! @param property
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


#ifdef HAVE_RDTSC

/*! @decl int rdtsc()
 *! Executes the rdtsc (clock pulse counter) instruction
 *! and returns the result.
 */

#define RDTSC(l,h)							\
   __asm__ __volatile__ (  "rdtsc"					\
			   :"=a" (l),					\
			   "=d" (h))

static INLINE long long rdtsc(void)
{
   long long now;
   unsigned long nl,nh;
   RDTSC(nl,nh);
   return (((long long)nh)<<32)|nl;
}

static void f_rdtsc(INT32 args)
{
   pop_n_elems(args);
   push_int64(rdtsc());
}

#endif


#ifdef HAVE_GETTIMEOFDAY

#ifndef HAVE_STRUCT_TIMEVAL
struct timeval
{
  long tv_sec;
  long tv_usec;
};
#endif

/*! @decl array(int) gettimeofday()
 *! Calls gettimeofday(); the result is an array of
 *! seconds, microseconds, and possible tz_minuteswes, tz_dstttime
 *! as given by the gettimeofday(2) system call 
 *! (read the man page).
 *!
 *! @seealso
 *!   @[time()], @[gethrtime()]
 */

static void f_gettimeofday(INT32 args)
{
   struct timeval tv;
#ifdef GETTIMEOFDAY_TAKES_TWO_ARGS
   struct timezone tz;
#endif
   pop_n_elems(args);
   if (gettimeofday(&tv,&tz))
      Pike_error("gettimeofday: gettimeofday failed, errno=%d\n",errno);
   push_int(tv.tv_sec);
   push_int(tv.tv_usec);
#ifdef GETTIMEOFDAY_TAKES_TWO_ARGS
   push_int(tz.tz_minuteswest);
   push_int(tz.tz_dsttime);
   f_aggregate(4);
#else
   f_aggregate(2);
#endif
}

#endif

/*! @decl string CPU_TIME_IS_THREAD_LOCAL
 *!
 *! This string constant tells whether or not the CPU time returned by
 *! @[gethrvtime] is thread local or not. The value is "yes" if it is
 *! and "no" if it isn't. The value is also "no" if there is no thread
 *! support.
 */

/*! @decl mapping(string:int) getrusage()
 *!
 *!   Return resource usage about the current process. An error is
 *!   thrown if it isn't supported or if the system fails to return
 *!   any information.
 *!
 *! @returns
 *!   Returns a mapping describing the current resource usage:
 *!   @mapping
 *!     @member int "utime"
 *!       Time in milliseconds spent in user code.
 *!     @member int "stime"
 *!       Time in milliseconds spent in system calls.
 *!     @member int "maxrss"
 *!       Maximum used resident size in kilobytes. [1]
 *!     @member int "ixrss"
 *!       Quote from GNU libc: An integral value expressed in
 *!       kilobytes times ticks of execution, which indicates the
 *!       amount of memory used by text that was shared with other
 *!       processes. [1]
 *!     @member int "idrss"
 *!       Quote from GNU libc: An integral value expressed the same
 *!       way, which is the amount of unshared memory used for data.
 *!       [1]
 *!     @member int "isrss"
 *!       Quote from GNU libc: An integral value expressed the same
 *!       way, which is the amount of unshared memory used for stack
 *!       space. [1]
 *!     @member int "minflt"
 *!       Minor page faults, i.e. TLB misses which required no disk I/O.
 *!     @member int "majflt"
 *!       Major page faults, i.e. paging with disk I/O required.
 *!     @member int "nswap"
 *!       Number of times the process has been swapped out entirely.
 *!     @member int "inblock"
 *!       Number of block input operations.
 *!     @member int "oublock"
 *!       Number of block output operations.
 *!     @member int "msgsnd"
 *!       Number of IPC messsages sent.
 *!     @member int "msgrcv"
 *!       Number of IPC messsages received.
 *!     @member int "nsignals"
 *!       Number of signals received.
 *!     @member int "nvcsw"
 *!       Number of voluntary context switches (usually to wait for
 *!       some service).
 *!     @member int "nivcsw"
 *!       Number of preemptions, i.e. context switches due to expired
 *!       time slices, or when processes with higher priority were
 *!       scheduled.
 *!     @member int "sysc"
 *!       Number of system calls. [2]
 *!     @member int "ioch"
 *!       Number of characters read and written. [2]
 *!     @member int "rtime"
 *!       Elapsed real time (ms). [2]
 *!     @member int "ttime"
 *!       Elapsed system trap (system call) time (ms). [2]
 *!     @member int "tftime"
 *!       Text page fault sleep time (ms). [2]
 *!     @member int "dftime"
 *!       Data page fault sleep time (ms). [2]
 *!     @member int "kftime"
 *!       Kernel page fault sleep time (ms). [2]
 *!     @member int "ltime"
 *!       User lock wait sleep time (ms). [2]
 *!     @member int "slptime"
 *!       Other sleep time (ms). [2]
 *!     @member int "wtime"
 *!       Wait CPU (latency) time (ms). [2]
 *!     @member int "stoptime"
 *!       Time spent in stopped (suspended) state. [2]
 *!     @member int "brksize"
 *!       Heap size. [3]
 *!     @member int "stksize"
 *!       Stack size. [3]
 *!   @endmapping
 *!
 *! @note
 *!   [1] Not if /proc rusage is used.
 *!
 *!   [2] Only from (Solaris?) /proc rusage.
 *!
 *!   [3] Only from /proc PRS usage.
 *!
 *!   On some systems, only utime will be filled in.
 *!
 *! @seealso
 *!   @[gethrvtime()]
 */

static void f_getrusage(INT32 args)
{
   pike_rusage_t rusage_values;
   int n=0;

   if (!pike_get_rusage(rusage_values))
     PIKE_ERROR("System.getrusage",
		"System usage information not available.\n", Pike_sp, args);

   pop_n_elems(args);
   
   push_text("utime");      push_int(rusage_values[n++]);
   push_text("stime");      push_int(rusage_values[n++]);
   push_text("maxrss");     push_int(rusage_values[n++]);
   push_text("ixrss");      push_int(rusage_values[n++]);
   push_text("idrss");      push_int(rusage_values[n++]);
   push_text("isrss");      push_int(rusage_values[n++]);
   push_text("minflt");     push_int(rusage_values[n++]);
   push_text("majflt");     push_int(rusage_values[n++]);
   push_text("nswap");      push_int(rusage_values[n++]);
   push_text("inblock");    push_int(rusage_values[n++]);
   push_text("oublock");    push_int(rusage_values[n++]);
   push_text("msgsnd");     push_int(rusage_values[n++]);
   push_text("msgrcv");     push_int(rusage_values[n++]);
   push_text("nsignals");   push_int(rusage_values[n++]);
   push_text("nvcsw");      push_int(rusage_values[n++]);
   push_text("nivcsw");     push_int(rusage_values[n++]);
   push_text("sysc");       push_int(rusage_values[n++]);
   push_text("ioch");       push_int(rusage_values[n++]);
   push_text("rtime");      push_int(rusage_values[n++]);
   push_text("ttime");      push_int(rusage_values[n++]);
   push_text("tftime");     push_int(rusage_values[n++]);
   push_text("dftime");     push_int(rusage_values[n++]);
   push_text("kftime");     push_int(rusage_values[n++]);
   push_text("ltime");      push_int(rusage_values[n++]);
   push_text("slptime");    push_int(rusage_values[n++]);
   push_text("wtime");      push_int(rusage_values[n++]);
   push_text("stoptime");   push_int(rusage_values[n++]);
   push_text("brksize");    push_int(rusage_values[n++]);
   push_text("stksize");    push_int(rusage_values[n++]);

   f_aggregate_mapping(n*2);
}

/*! @endmodule
 */

/*
 * Module linkage
 */

PIKE_MODULE_INIT
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
	   tFunc(tStr tOr(tStr,tVoid) tOr(tStr,tVoid) tOr(tStr,tVoid), tInt01),
	   OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("innetgr", f_innetgr,
	   tFunc(tStr tOr(tStr,tVoid) tOr(tStr,tVoid) tOr(tStr,tVoid), tInt),
	   0, OPT_EXTERNAL_DEPEND);
#endif /* HAVE_INNETGR */
#ifdef HAVE_SETUID
  
/* function(int:int) */
  ADD_EFUN("setuid", f_setuid,tFunc(tInt,tInt), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("setuid", f_setuid,tFunc(tInt,tInt), 0, OPT_SIDE_EFFECT);
#endif
#ifdef HAVE_SETGID
  
/* function(int:int) */
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
/* Also available as efun */
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
#ifdef HAVE_USLEEP
  ADD_FUNCTION("usleep",f_system_usleep,tFunc(tInt,tVoid), 0);
#endif /* HAVE_SLEEP */
#ifdef HAVE_NANOSLEEP
  ADD_FUNCTION("nanosleep",f_system_nanosleep,
	       tFunc(tOr(tInt,tFloat),tFloat), 0);
#endif /* HAVE_SLEEP */

/* there is always a pike_get_rusage */
  ADD_FUNCTION("getrusage", f_getrusage,
	       tFunc(tNone, tMap(tStr,tInt)), 0);

#ifdef ITIMER_TYPE_IS_02
#define tITimer tInt02
#else
#define tITimer tInt
#endif

#ifdef HAVE_SETITIMER
  ADD_FUNCTION("setitimer",f_system_setitimer,
	       tFunc(tITimer tOr(tIntPos,tFloat),tFloat),0);
#ifdef ITIMER_REAL
   ADD_INT_CONSTANT("ITIMER_REAL",ITIMER_REAL,0);
#endif
#ifdef ITIMER_VIRTUAL
   ADD_INT_CONSTANT("ITIMER_VIRTUAL",ITIMER_VIRTUAL,0);
#endif
#ifdef ITIMER_PROF
   ADD_INT_CONSTANT("ITIMER_PROF",ITIMER_PROF,0);
#endif

#ifdef HAVE_GETITIMER
  ADD_FUNCTION("getitimer",f_system_getitimer,
	       tFunc(tITimer,tArr(tFloat)),0);
#endif
#endif

#ifdef HAVE_RDTSC
  ADD_FUNCTION("rdtsc",f_rdtsc,
	       tFunc(tNone,tInt),0);
#endif
#ifdef HAVE_GETTIMEOFDAY
  ADD_FUNCTION("gettimeofday",f_gettimeofday,
	       tFunc(tNone,tArr(tInt)),0);
#endif

#if CPU_TIME_IS_THREAD_LOCAL == PIKE_YES
  add_string_constant ("CPU_TIME_IS_THREAD_LOCAL", "yes", 0);
#elif CPU_TIME_IS_THREAD_LOCAL == PIKE_NO
  add_string_constant ("CPU_TIME_IS_THREAD_LOCAL", "no", 0);
#endif

#ifdef HAVE_NETINFO_NI_H
  /* array(string) get_netinfo_property(string domain, string path,
                                        string property) */
  ADD_FUNCTION("get_netinfo_property", f_get_netinfo_property,
	       tFunc(tStr tStr tStr, tArray), 0);
#endif /* NETINFO */

  init_passwd();
  init_system_memory();

#if defined(GETHOSTBYNAME_MUTEX_EXISTS) || defined(GETSERVBYNAME_MUTEX_EXISTS)
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

PIKE_MODULE_EXIT
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
#if defined(HAVE_GETRLIMIT) || defined(HAVE_SETRLIMIT)
  if (s_cpu) {
    free_string(s_cpu);
    s_cpu=NULL;
  }
  if (s_fsize) {
    free_string(s_fsize);
    s_fsize=NULL;
  }
  if (s_data) {
    free_string(s_data);
    s_data=NULL;
  }
  if (s_stack) {
    free_string(s_stack);
    s_stack=NULL;
  }
  if (s_core) {
    free_string(s_core);
    s_core=NULL;
  }
  if (s_rss) {
    free_string(s_rss);
    s_rss=NULL;
  }
  if (s_nproc) {
    free_string(s_nproc);
    s_nproc=NULL;
  }
  if (s_nofile) {
    free_string(s_nofile);
    s_nofile=NULL;
  }
  if (s_memlock) {
    free_string(s_memlock);
    s_memlock=NULL;
  }
  if (s_as) {
    free_string(s_as);
    s_as=NULL;
  }
  if (s_vmem) {
    free_string(s_vmem);
    s_vmem=NULL;
  }
#endif /* HAVE_GETRLIMIT || HAVE_SETRLIMIT */
}
