/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * System-call module for Pike
 *
 * Henrik Grubbstr�m 1997-01-20
 */

/*
 * Includes
 */

#include "global.h"

#include "system_machine.h"
#include "system.h"

#ifdef HAVE_IO_H
#include <io.h>
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
#include "time_stuff.h"
#include "pike_memory.h"
#include "bignum.h"
#include "pike_rusage.h"
#include "pike_netlib.h"
#include "pike_cpulib.h"
#include "sprintf.h"
#include "operators.h"
#include "fdlib.h"

#include <errno.h>

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif /* HAVE_PWD_H */
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

#ifdef HAVE_NETINFO_NI_H
#include <netinfo/ni.h>
#endif

#ifdef HAVE_SYS_LOADAVG_H
#include <sys/loadavg.h>
#endif

#ifdef HAVE_SYS_CLONEFILE_H
#include <sys/clonefile.h>
#endif

#ifdef HAVE_LINUX_FS_H
#include <linux/fs.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

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

#ifdef SOCK_DEBUG
#define SOCKWERR(X, ...)	fprintf(stderr, X, __VA_ARGS__)
#else
#define SOCKWERR(X, ...)
#endif

#ifdef USE_VALGRIND
#undef HAS___BUILTIN_IA32_RDRAND64_STEP
#endif

/*
 * Globals
 */

struct mapping *strerror_lookup = NULL;

/*
 * Functions
 */

static struct pike_string *pike_strerror(int e)
{
  struct svalue s;
  struct svalue *val;

#if EINVAL < 0
  /* Under some circumstances Haiku can apparently return
   * positive errnos.
   */
  if (e > 0) e = -e;
#endif

  SET_SVAL(s, PIKE_T_INT, NUMBER_NUMBER, integer, e);

  val = strerror_lookup? low_mapping_lookup(strerror_lookup, &s): NULL;

  if (!val) return NULL;

  if (TYPEOF(*val) == PIKE_T_STRING) return val->u.string;

  return NULL;
}

/* Helper functions */

void report_os_error(const char *function_name)
{
  int e = errno;
  struct pike_string *s = pike_strerror(e);

  if (s) {
    Pike_error("%s(): Failed: %pS\n", function_name, s);
  }
  Pike_error("%s(): Failed: errno %d\n", function_name, e);
}


/*
 * efuns
 */

/*! @module System
 *!
 *! This module embodies common operating system calls, making them
 *! available to the Pike programmer.
 */

#if defined(HAVE_LINK) || defined(__NT__)
/*! @decl void hardlink(string from, string to)
 *!
 *! Create a hardlink named @[to] from the file @[from].
 *!
 *! @note
 *!   This function is not available on all platforms.
 *!
 *! @seealso
 *!   @[symlink()], @[clonefile()], @[mv()], @[rm()]
 */
void f_hardlink(INT32 args)
{
  char *from;
  char *to;
  int err;

  get_all_args("hardlink",args, "%c%c", &from, &to);

  do {
    THREADS_ALLOW_UID();
    err = fd_link(from, to);
    THREADS_DISALLOW_UID();
    if (err >= 0 || errno != EINTR) break;
    check_threads_etc();
  } while (1);

  if (err < 0) {
    report_os_error("hardlink");
  }
  pop_n_elems(args);
}
#endif /* HAVE_LINK || __NT__ */

#if defined(HAVE_SYMLINK) || defined(__NT__)
/*! @decl void symlink(string from, string to)
 *!
 *! Create a symbolic link named @[to] that points to @[from].
 *!
 *! @note
 *!   This function is not available on all platforms.
 *!
 *! @seealso
 *!   @[hardlink()], @[readlink()], @[clonefile()], @[mv()], @[rm()]
 */
void f_symlink(INT32 args)
{
  char *from;
  char *to;
  int err;

  get_all_args("symlink",args, "%c%c", &from, &to);

  do {
    THREADS_ALLOW_UID();
    err = fd_symlink(from, to);
    THREADS_DISALLOW_UID();
    if (err >= 0 || errno != EINTR) break;
    check_threads_etc();
  } while (1);

  if (err < 0) {
    report_os_error("symlink");
  }
  pop_n_elems(args);
}
#endif /* HAVE_SYMLINK || __NT__ */

#if defined(HAVE_READLINK) || defined(__NT__)
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

  get_all_args("readlink",args, "%c", &path);

  buflen = 100;

  do {
    buflen *= 2;
    if (!(buf = alloca(buflen))) {
      Pike_error("Out of memory.\n");
    }

    do {
      THREADS_ALLOW_UID();
      err = fd_readlink(path, buf, buflen);
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
    report_os_error("readlink");
  }
  pop_n_elems(args);
  push_string(make_shared_binary_string(buf, err));
}
#endif /* HAVE_READLINK || __NT__ */

#if !defined(HAVE_RESOLVEPATH) && !defined(HAVE_REALPATH)
#ifdef HAVE_READLINK
/* FIXME: Write code that simulates resolvepath() here.
 */
/* #define HAVE_RESOLVEPATH */
#endif /* HAVE_READLINK */
#endif /* !HAVE_RESOLVEPATH && !HAVE_REALPATH */

#if defined(HAVE_RESOLVEPATH) || defined(HAVE_REALPATH)
/*! @decl string resolvepath(string path)
 *!
 *!   Resolve all symbolic links of a pathname.
 *!
 *!   This function resolves all symbolic links, extra ``/'' characters and
 *!   references to /./ and /../ in @[pathname], and returns the resulting
 *!   absolute path, or @tt{0@} (zero) if an error occurs.
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
  int len = -1;

  get_all_args("resolvepath", args, "%c", &path);

#ifdef HAVE_RESOLVEPATH
  buflen = 100;

  do {
    buflen *= 2;
    if (!(buf = alloca(buflen))) {
      Pike_error("Out of memory.\n");
    }

    do {
      THREADS_ALLOW_UID();
      len = resolvepath(path, buf, buflen);
      THREADS_DISALLOW_UID();
      if (len >= 0 || errno != EINTR) break;
      check_threads_etc();
    } while (1);
  } while(
#ifdef ENAMETOOLONG
	  ((len < 0) && (errno == ENAMETOOLONG)) ||
#endif /* ENAMETOOLONG */
	  (len >= buflen - 1));
#elif defined(HAVE_REALPATH)
#ifdef PATH_MAX
  buflen = PATH_MAX+1;

  if (!(buf = alloca(buflen))) {
    Pike_error("Out of memory.\n");
  }
#else
  /* Later revisions of POSIX define realpath() to dynamically
   * allocate the result if passed NULL.
   * cf https://www.gnu.org/software/hurd/hurd/porting/guidelines.html
   */
  buf = NULL;
#endif
  if ((buf = realpath(path, buf))) {
    len = strlen(buf);
  }
#else /* !HAVE_RESOLVEPATH && !HAVE_REALPATH */
#error "f_resolvepath with neither resolvepath nor realpath."
#endif /* HAVE_RESOLVEPATH */

  if (len < 0) {
    report_os_error("resolvepath");
  }
  pop_n_elems(args);
  push_string(make_shared_binary_string(buf, len));
#if !defined(HAVE_RESOLVEPATH) && !defined(PATH_MAX)
  /* Free the dynamically allocated buf. */
  free(buf);
#endif
}
#endif /* HAVE_RESOLVEPATH || HAVE_REALPATH */

#if !defined(HAVE_CLONEFILE) && defined(HAVE_COPY_FILE_RANGE)
int clonefile(const char *from, const char *to, int flags)
{
  int ret = -1;
  int fromfd = -1;
  int tofd = -1;
  struct stat stat;

  if (flags) {
    /* No flags are supported currently. */
    errno = EINVAL;
    return -1;
  }

  fromfd = fd_open(from, fd_RDONLY, 0);
  if (fromfd < 0) return -1;

  if (fd_fstat(fromfd, &stat) < 0) goto cleanup;

  if ((stat.st_mode & S_IFMT) != S_IFREG) {
    errno = EINVAL;
    goto cleanup;
  }

  tofd = fd_open(to, fd_CREAT|fd_TRUNC|fd_RDWR, stat.st_mode);
  if (tofd < 0) goto cleanup;

  ret = copy_file_range(fromfd, NULL, tofd, NULL, stat.st_size, 0);

 cleanup:
  if (tofd >= 0) close(tofd);

  if (fromfd >= 0) close(fromfd);

  return ret;
}

#define HAVE_CLONEFILE
#endif

#ifdef HAVE_CLONEFILE
/*! @decl void clonefile(string from, string to)
 *!
 *! Copy a file @[from] with copy-on-write semantics to the destination named
 *! @[to].
 *!
 *! @note
 *!   This function is currently only available on macOS and Linux, and then
 *!   only when @[from] and @[to] reference a common file system with
 *!   copy-on-write support (e.g. an APFS volume).
 *!
 *! @seealso
 *!   @[hardlink()], @[symlink()]
 */
void f_clonefile(INT32 args)
{
  char *from;
  char *to;
  int err;

  get_all_args("clonefile",args, "%s%s", &from, &to);

  do {
    THREADS_ALLOW_UID();
    err = clonefile(from, to, 0);
    THREADS_DISALLOW_UID();
    if (err >= 0 || errno != EINTR) break;
    check_threads_etc();
  } while (1);

  if (err < 0) {
    report_os_error("clonefile");
  }
  pop_n_elems(args);
}
#endif /* HAVE_CLONEFILE */

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

  get_all_args("chmod", args, "%c%i", &path, &mode);
  do {
    THREADS_ALLOW_UID();
    err = chmod(path, mode);
    THREADS_DISALLOW_UID();
    if (err >= 0 || errno != EINTR) break;
    check_threads_etc();
  } while (1);
  if (err < 0) {
    report_os_error("chmod");
  }
  pop_n_elems(args);
}

#ifdef HAVE_CHOWN
/*! @decl void chown(string path, int uid, int gid, void|int symlink)
 *!
 *! Sets the owner and group of the specified path.
 *!
 *! If @[symlink] is set and @[path] refers to a symlink, then the
 *! owner and group for the symlink are set. Symlinks are dereferenced
 *! otherwise.
 *!
 *! @note
 *!   Throws errors on failure.
 *!
 *!   This function is not available on all platforms. On some
 *!   platforms the @[symlink] flag isn't supported. In that case, the
 *!   function does nothing if @[path] is a symlink.
 */
void f_chown(INT32 args)
{
  char *path;
  INT_TYPE uid;
  INT_TYPE gid;
  int symlink = 0;
  int err;

  get_all_args("chown", args, "%c%i%i.%d", &path, &uid, &gid, &symlink);

#ifndef HAVE_LCHOWN
#ifdef HAVE_LSTAT
  {
    PIKE_STAT_T st;
    int orig_errno = errno;
    do {
      THREADS_ALLOW_UID();
      err = fd_lstat (path, &st);
      THREADS_DISALLOW_UID();
      if (err >= 0 || errno != EINTR) break;
    } while (1);
    errno = orig_errno;

    if (!err && ((st.st_mode & S_IFMT) == S_IFLNK)) {
      pop_n_elems (args);
      return;
    }
  }

#else
  /* Awkward situation if there's no lstat. Best we can do is just to
   * go ahead, except we don't complain for ENOENT, to avoid throwing
   * errors on dangling symlinks. */
#define CHOWN_IGNORE_ENOENT
#endif
#endif	/* !HAVE_LCHOWN */

  do {
    THREADS_ALLOW_UID();
#ifdef HAVE_LCHOWN
    if (symlink)
      err = lchown (path, uid, gid);
    else
#endif
      err = chown(path, uid, gid);
    THREADS_DISALLOW_UID();
    if (err >= 0 || errno != EINTR) break;
    check_threads_etc();
  } while (1);

#ifdef CHOWN_IGNORE_ENOENT
  if (err < 0 && symlink && errno == ENOENT)
    err = 0;
#endif

  if (err < 0) {
    report_os_error("chown");
  }
  pop_n_elems(args);
}
#endif

#if defined (HAVE_UTIME) || defined (HAVE__UTIME)
/*! @decl void utime(string path, int atime, int mtime, void|int symlink)
 *!
 *! Set the last access time and last modification time for the path
 *! @[path] to @[atime] and @[mtime] repectively. They are specified
 *! as unix timestamps with 1 second resolution.
 *!
 *! If @[symlink] is set and @[path] refers to a symlink, then the
 *! timestamps for the symlink are set. Symlinks are dereferenced
 *! otherwise.
 *!
 *! @note
 *!   Throws errors on failure.
 *!
 *!   This function is not available on all platforms. On some
 *!   platforms the @[symlink] flag isn't supported. In that case, the
 *!   function does nothing if @[path] is a symlink.
 *!
 *! @seealso
 *! @[System.set_file_atime], @[System.set_file_mtime]
 */
void f_utime(INT32 args)
{
  char *path;
  INT_TYPE atime, mtime;
  int symlink = 0;
  int err;

  get_all_args("utime", args, "%c%i%i.%d", &path, &atime, &mtime, &symlink);

  if (symlink) {
#ifdef HAVE_LUTIMES
    struct timeval tv[2];
    tv[0].tv_sec = atime;
    tv[0].tv_usec = 0;
    tv[1].tv_sec = mtime;
    tv[1].tv_usec = 0;
    do {
      THREADS_ALLOW_UID();
      err = lutimes (path, tv);
      THREADS_DISALLOW_UID();
      if (err >= 0 || errno != EINTR) break;
      check_threads_etc();
    } while (1);
    if (err < 0)
      report_os_error("utime");
    pop_n_elems(args);
    return;

#elif defined (HAVE_LSTAT)
    PIKE_STAT_T st;
    int orig_errno = errno;
    do {
      THREADS_ALLOW_UID();
      err = fd_lstat (path, &st);
      THREADS_DISALLOW_UID();
      if (err >= 0 || errno != EINTR) break;
    } while (1);
    errno = orig_errno;

    if (!err && ((st.st_mode & S_IFMT) == S_IFLNK)) {
      pop_n_elems (args);
      return;
    }

#else
    /* Awkward situation if there's no lstat. Best we can do is just
     * to go ahead, except we don't complain for ENOENT, to avoid
     * throwing errors on dangling symlinks. */
#define UTIME_IGNORE_ENOENT
#endif
  }

  {
    /*&#()&@(*#&$ NT ()*&#)(&*@$#*/
    struct fd_utimbuf b;

    b.actime=atime;
    b.modtime=mtime;
    do {
      THREADS_ALLOW_UID();
      err = fd_utime(path, &b);
      THREADS_DISALLOW_UID();
      if (err >= 0 || errno != EINTR) break;
      check_threads_etc();
    } while (1);
  }

#ifdef UTIME_IGNORE_ENOENT
  if (err < 0 && symlink && errno == ENOENT)
    err = 0;
#endif

  if (err < 0) {
    report_os_error("utime");
  }
  pop_n_elems(args);
}
#endif

#if !defined(HAVE_SYNC) && defined(__NT__)
static void sync(void)
{
  /* NB: For some stupid reason FindFirstVolume()/FindNextVolume()
   *     will only list local filesystems. So we have to fall
   *     back to using GetLogicalDrives().
   */
  /* NB: \\.\ is the DOS device namespace prefix. */
  char drive[8] = "\\\\.\\A:";
  char *driveletter = drive + 4;
  DWORD drives = GetLogicalDrives();
  DWORD mask = 1;
  /* Loop over all mounted volumes. */
  while (mask) {
    if (mask & drives) {
      char device[MAX_PATH*2];
      HANDLE volfile;

      if (QueryDosDeviceA(driveletter, device, MAX_PATH*2)) {
        volfile = CreateFileA(device, GENERIC_WRITE|GENERIC_READ,
                              FILE_SHARE_WRITE|FILE_SHARE_READ,
			      NULL, OPEN_EXISTING, 0, NULL);
	if (volfile != INVALID_HANDLE_VALUE) {
          FlushFileBuffers(volfile);
	  CloseHandle(volfile);
	}
      }

      /* Flush the drive. */
      volfile = CreateFileA(drive, GENERIC_WRITE|GENERIC_READ,
                            FILE_SHARE_WRITE|FILE_SHARE_READ,
			    NULL, OPEN_EXISTING, 0, NULL);
      if (volfile != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(volfile);
	CloseHandle(volfile);
      }
    }
    *driveletter++;
    mask = mask << 1;
  }
}
#define HAVE_SYNC
#endif

#ifdef HAVE_SYNC
/*! @decl void sync()
 *!
 *! Flush operating system disk buffers to permanent storage.
 *!
 *! @note
 *!   On some operating systems this may require
 *!   administrative privileges.
 */
void f_sync(INT32 args)
{
  pop_n_elems(args);
  THREADS_ALLOW_UID()
  sync();
  THREADS_DISALLOW_UID();
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

  get_all_args("initgroups", args, "%c%i", &user, &group);
  err = initgroups(user, group);
  if (err < 0) {
    report_os_error("initgroups");
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
  static const gid_t gids[1]={ 65534 }; /* To safeguard against stupid OS's */
  int err;

  pop_n_elems(args);
  err = setgroups(0, (gid_t *)gids);
  if (err < 0) {
    report_os_error("cleargroups");
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
  static const gid_t safeguard[1] = { 65534 };
  struct array *arr = NULL;
  gid_t *gids = NULL;
  INT32 i;
  INT32 size;
  int err;

  get_all_args("setgroups", args, "%a", &arr);
  if ((size = arr->size)) {
    gids = alloca(arr->size * sizeof(gid_t));
    if (!gids) Pike_error("Too large array (%d).\n", arr->size);
  } else {
    gids = (gid_t *)safeguard;
  }

  for (i=0; i < size; i++) {
    if (TYPEOF(arr->item[i]) != T_INT) {
      Pike_error("Bad element %d in array (expected int).\n", i);
    }
    gids[i] = arr->item[i].u.integer;
  }

  pop_n_elems(args);

  err = setgroups(size, gids);
  if (err < 0) {
    report_os_error("setgroups");
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
  gids = xalloc(sizeof(gid_t) * numgrps);

  numgrps = getgroups(numgrps, gids);

  for (i=0; i < numgrps; i++) {
    push_int(gids[i]);
  }
  free(gids);

  if (numgrps < 0) {
    report_os_error("getgroups");
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
    if (TYPEOF(sp[i-args]) == T_STRING) {
      if (sp[i-args].u.string->size_shift) {
	SIMPLE_ARG_TYPE_ERROR("innetgr", i+1, "string (8bit)");
      }
      strs[i] = sp[i-args].u.string->str;
    } else if (sp[i-args].u.integer) {
      SIMPLE_ARG_TYPE_ERROR("innetgr", i+1, "string|void");
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
    if (TYPEOF(sp[-args]) != T_INT) {
      SIMPLE_ARG_TYPE_ERROR("getpgrp", 1, "int");
    }
    pid = sp[-args].u.integer;
  }
  pop_n_elems(args);
#ifdef HAVE_GETPGID
  pgid = getpgid(pid);
#elif defined(HAVE_GETPGRP)
  if (pid && (pid != getpid())) {
    Pike_error("Mode not supported on this OS.\n");
  }
  pgid = getpgrp();
#endif
  if (pgid < 0)
    report_os_error("getpgrp");

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
    report_os_error("setpgrp");

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
  if (args >= 1 && TYPEOF(sp[-args]) != T_INT)
    SIMPLE_ARG_TYPE_ERROR("getsid", 1, "int");
  if (args >= 1)
       pid = sp[-args].u.integer;
  pop_n_elems(args);
  pid = getsid(pid);
  if (pid < 0)
    report_os_error("getsid");
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
    report_os_error("setsid");
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
    get_all_args(NULL, args, "%i", &val);
    if (val & ~1) {
      SIMPLE_ARG_TYPE_ERROR("dumpable", 1, "int(0..1)");
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
 *!
 *! @note
 *!   On success the current working directory will be changed to
 *!   the new @expr{"/"@}. This behavior was added in Pike 7.9.
 *!
 *! @note
 *!   This function could be interrupted by signals prior to Pike 7.9.
 */
void f_chroot(INT32 args)
{
  int res;

#ifdef HAVE_FCHROOT
  check_all_args("chroot", args, BIT_STRING|BIT_OBJECT, 0);
#else
  check_all_args("chroot", args, BIT_STRING, 0);
#endif /* HAVE_FCHROOT */


#ifdef HAVE_FCHROOT
  if(TYPEOF(sp[-args]) == T_STRING)
  {
#endif /* HAVE_FCHROOT */
    while (((res = chroot((char *)sp[-args].u.string->str)) == -1) &&
	   (errno == EINTR))
      ;
    if (!res) {
      while ((chdir("/") == -1) && (errno == EINTR))
	;
    }
    pop_n_elems(args);
    push_int(!res);
    return;
#ifdef HAVE_FCHROOT
  } else {
    int fd;

    apply(sp[-args].u.object, "query_fd", 0);
    fd=sp[-1].u.integer;
    pop_stack();
    while (((res = fchroot(fd)) == -1) && (errno == EINTR))
      ;
    if (!res) {
      while ((fchdir(fd) == -1) && (errno == EINTR))
	;
    }
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
    Pike_error("System call failed.\n");

  push_static_text("sysname");
  push_text(foo.sysname);

  push_static_text("nodename");
  push_text(foo.nodename);

  push_static_text("release");
  push_text(foo.release);

  push_static_text("version");
  push_text(foo.version);

  push_static_text("machine");
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
    Pike_error("System call failed.\n");
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
    Pike_error("System call failed.\n");
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
      /* FALLTHRU */
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
    memset(&data,0,sizeof(data)); \
    if(gethostbyname_r((X), &result, &data) < 0) { \
      ret=0; \
    }else{ \
      ret=&result; \
    } \
    THREADS_DISALLOW()

#define CALL_GETHOSTBYADDR(X,Y,Z) \
    THREADS_ALLOW(); \
    memset(&data,0,sizeof(data)); \
    if(gethostbyaddr_r((X),(Y),(Z), &result, &data) < 0) { \
      ret=0; \
    }else{ \
      ret=&result; \
    } \
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
    memset(&data,0,sizeof(data)); \
    if(getservbyname_r((X), (Y), &result, &data) < 0) { \
      ret=0; \
    }else{ \
      ret=&result; \
    } \
    THREADS_DISALLOW()

#endif /* HAVE_OSF1_GETSERVBYNAME_R */
#endif /* HAVE_SOLARIS_GETSERVBYNAME_R */

#endif /* REENTRANT */

#ifndef GETHOST_DECLARE
#ifdef HAVE_GETHOSTBYNAME
#define GETHOST_DECLARE struct hostent *ret
#define CALL_GETHOSTBYNAME(X) ret=gethostbyname(X)
#define CALL_GETHOSTBYADDR(X,Y,Z) ret=gethostbyaddr((X),(Y),(Z))
#endif
#endif /* !GETHOST_DECLARE */

#ifndef GETSERV_DECLARE
#ifdef HAVE_GETSERVBYNAME
#define GETSERV_DECLARE struct servent *ret
#define CALL_GETSERVBYNAME(X,Y) ret=getservbyname(X,Y)
#endif
#endif /* !GETSERV_DECLARE */

/* Ensure that the hint flags exist. */
#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG	0
#endif
#ifndef AI_NUMERICHOST
#define AI_NUMERICHOST	0
#endif
#ifndef AI_NUMERICSERV
#define AI_NUMERICSERV	0
#endif
#ifndef AI_V4MAPPED
#define AI_V4MAPPED	0
#endif

/* this is also used from modules/_Stdio */
int get_inet_addr(PIKE_SOCKADDR *addr,char *name,char *service, INT_TYPE port,
                  int inet_flags)
{
#ifdef HAVE_GETADDRINFO
  struct addrinfo hints = {
    0, PF_UNSPEC, 0, 0,
#ifdef STRUCT_ADDRINFO_HAS__AI_PAD
    0,	/* _ai_pad, only present on Solaris 11 when compiling for SparcV9. */
#endif
    0, NULL, NULL, NULL, }, *res;
  char servnum_buf[20];
#endif /* HAVE_GETADDRINFO */
  int err;
  int udp = inet_flags & 1;

  memset(addr,0,sizeof(PIKE_SOCKADDR));
  if(name && !strcmp(name,"*"))
    name = NULL;

#ifdef HAVE_GETADDRINFO
  SOCKWERR("get_inet_addr(): Trying getaddrinfo\n");
  if(!name) {
    hints.ai_flags = AI_PASSIVE;
    /* Avoid creating an IPv6 address for "*". */
    /* For IN6ADDR_ANY, use "::". */
    hints.ai_family = AF_INET;
#ifdef PF_INET6
  } else if (inet_flags & 2) {
    /* Force IPv6. */
    /* Map all addresses to the IPv6 namespace,
     * to avoid address conflicts when once end
     * of the socket is IPv4 and one end is IPv6.
     * This is needed on eg Linux where a socket
     * bound to ::FFFF:127.0.0.1 can't connect
     * to the IPv4 address 127.0.0.1.
     */
    hints.ai_family = AF_INET6;
    hints.ai_flags = AI_V4MAPPED;
  }
#endif
  hints.ai_flags |= AI_NUMERICHOST|AI_ADDRCONFIG;
  hints.ai_protocol = (udp? IPPROTO_UDP:IPPROTO_TCP);
  if (!service && (port > 0) && (port & 0xffff)) {
    /* NB: MacOS X doesn't like the combination of AI_NUMERICSERV and port #0.
     *     cf [bug 7599] and https://bugs.python.org/issue17269 .
     */
    hints.ai_flags |= AI_NUMERICSERV;
    sprintf(service = servnum_buf, "%"PRINTPIKEINT"d", port & 0xffff);
  }

#if AI_NUMERICHOST != 0
  if( (err=getaddrinfo(name, service, &hints, &res)) )
  {
    /* Try again without AI_NUMERICHOST. */
    hints.ai_flags &= ~AI_NUMERICHOST;
    err=getaddrinfo(name, service, &hints, &res);
  }
#endif
  if(!err)
  {
    struct addrinfo *p, *found = NULL;
    size_t addr_len=0;
    for(p=res; p; p=p->ai_next) {
      if(p->ai_addrlen > addr_len &&
         p->ai_addrlen <= sizeof(*addr) &&
         p->ai_addr)
        addr_len = (found = p)->ai_addrlen;
    }
    if(found) {
      SOCKWERR("Got %d bytes (family: %d (%d))\n",
	       addr_len, found->ai_addr->sa_family, found->ai_family);
      memcpy(addr, found->ai_addr, addr_len);
    }
    freeaddrinfo(res);
    if(addr_len) {
      SOCKWERR("family: %d\n", SOCKADDR_FAMILY(*addr));
      return addr_len;
    }
  } else {
    SOCKWERR("Failed with err: %d: %s, errno %d: %s, flags: %d\n",
	     err, gai_strerror(err), errno, strerror(errno), hints.ai_flags);
  }
  if (service == servnum_buf) service = NULL;
#endif /* HAVE_GETADDRINFO */

  SOCKADDR_FAMILY(*addr) = AF_INET;
#ifdef AF_INET6
  if (inet_flags & 2) {
    SOCKADDR_FAMILY(*addr) = AF_INET6;
    /* Note: This is equivalent to :: (aka IPv6 ANY). */
    memset(&addr->ipv6.sin6_addr, 0, sizeof(addr->ipv6.sin6_addr));
  }
#endif

  if(!name)
  {
    SOCKWERR("get_inet_addr(): ANY\n");
#ifdef AF_INET6
    if (!(inet_flags & 2))
#endif
      addr->ipv4.sin_addr.s_addr=htonl(INADDR_ANY);
  }
  /* FIXME: Ought to check for IPv6 literal address here, but
   *        it will typically be handled by getaddrinfo() above.
   */
  else if(my_isipnr(name)) /* I do not entirely trust inet_addr */
  {
    SOCKWERR("get_inet_addr(): IP\n");
    if (((IN_ADDR_T)inet_addr(name)) == ((IN_ADDR_T)-1))
      Pike_error("Malformed ip number.\n");
#ifdef AF_INET6
    if (inet_flags & 2) {
      /* Convert to IPv4 compat address: ::FFFF:a.b.c.d */
      struct sockaddr_in ipv4;
      ipv4.sin_addr.s_addr = inet_addr(name);
      addr->ipv6.sin6_addr.s6_addr[10] = 0xff;
      addr->ipv6.sin6_addr.s6_addr[11] = 0xff;
      memcpy(addr->ipv6.sin6_addr.s6_addr + 12, &ipv4.sin_addr.s_addr, 4);
    } else
#endif
      addr->ipv4.sin_addr.s_addr = inet_addr(name);
  }
  else
  {
#ifdef GETHOST_DECLARE
    GETHOST_DECLARE;
    SOCKWERR("get_inet_addr(): Trying gethostbyname()\n");
    CALL_GETHOSTBYNAME(name);

    if(!ret) {
      if (strlen(name) < 1024) {
        Pike_error("Invalid address '%s'.\n",name);
      } else {
        Pike_error("Invalid address.\n");
      }
    }

    SOCKADDR_FAMILY(*addr) = ret->h_addrtype;

#ifdef HAVE_H_ADDR_LIST
    memcpy(SOCKADDR_IN_ADDR(*addr),
           ret->h_addr_list[0],
           ret->h_length);
#else
    memcpy(SOCKADDR_IN_ADDR(*addr),
           ret->h_addr,
           ret->h_length);
#endif
#else
    if (strlen(name) < 1024) {
      Pike_error("Invalid address '%s'.\n",name);
    } else {
      Pike_error("Invalid address.\n");
    }
#endif
  }

  if(service) {
#ifdef GETSERV_DECLARE
    GETSERV_DECLARE;
    SOCKWERR("get_inet_addr(): Trying getserv()\n");
    CALL_GETSERVBYNAME(service, (udp? "udp":"tcp"));

    if(!ret) {
      if (strlen(service) < 1024) {
        Pike_error("Invalid service '%s'.\n",service);
      } else {
        Pike_error("Invalid service.\n");
      }
    }

#ifdef AF_INET6
    if (SOCKADDR_FAMILY(*addr) == AF_INET6) {
      addr->ipv6.sin6_port = ret->s_port;
    } else
#endif
      addr->ipv4.sin_port = ret->s_port;
#else
    if (strlen(service) < 1024) {
      Pike_error("Invalid service '%s'.\n",service);
    } else {
      Pike_error("Invalid service.\n");
    }
#endif
  } else if(port > 0) {
    SOCKWERR("get_inet_addr(): port()\n");
#ifdef AF_INET6
    if (SOCKADDR_FAMILY(*addr) == AF_INET6) {
      addr->ipv6.sin6_port = htons((unsigned INT16)port);
    } else
#endif
      addr->ipv4.sin_port = htons((unsigned INT16)port);
  } else {
    SOCKWERR("get_inet_addr(): ANY port()\n");
#ifdef AF_INET6
    if (SOCKADDR_FAMILY(*addr) == AF_INET6) {
      addr->ipv6.sin6_port = 0;
    } else
#endif
      addr->ipv4.sin_port = 0;
  }

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
#ifdef fd_inet_ntop
      /* Max size for IPv4 is 4*3(digits) + 3(separator) + 1(nul) == 16.
       *    nnn.nnn.nnn.nnn
       * Max size for IPv6 is 8*4(digits) + 7(separator) + 1(nul) == 40.
       *    xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx
       * Max size for mixed mode is 6*4 + 4*3 + 6+3(separator) + 1(nul) == 46.
       *    xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:nnn.nnn.nnn.nnn
       *
       * 64 should be safe for now.
       */
      char buffer[64];
      const char *addr =
        fd_inet_ntop(hp->h_addrtype, *p, buffer, sizeof(buffer));
      if (addr) {
        push_text(addr);
      } else {
        /* Very unlikely. */
        Pike_error("Invalid address.\n");
      }
#else
      struct in_addr in;

      memcpy(&in.s_addr, *p, sizeof (in.s_addr));
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
#ifdef fd_inet_ntop
    char buffer[64];

    push_text(fd_inet_ntop(hp->h_addrtype, hp->h_addr, buffer, sizeof(buffer)));
#else
    struct in_addr in;
    memcpy(&in.s_addr, hp->h_addr, sizeof (in.s_addr));
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

  get_all_args("gethostbyaddr", args, "%c", &name);

  if ((int)(addr = inet_addr(name)) == -1) {
    Pike_error("IP-address must be of the form a.b.c.d.\n");
  }

  pop_n_elems(args);

  CALL_GETHOSTBYADDR((char *)&addr, sizeof (addr), AF_INET);
  INVALIDATE_CURRENT_TIME();

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

  get_all_args("gethostbyname", args, "%c", &name);

  CALL_GETHOSTBYNAME(name);
  INVALIDATE_CURRENT_TIME();

  pop_n_elems(args);

  if(!ret) {
    push_int(0);
    return;
  }
  describe_hostent(ret);
}
#endif /* HAVE_GETHOSTBYNAME */

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

#ifdef HAVE_GETRLIMIT
static const struct {
    const char *name;
    const int lim;
} __limits[] = {
#ifdef RLIMIT_CORE
    {"core", RLIMIT_CORE},
#endif
#ifdef RLIMIT_CPU
    {"cpu", RLIMIT_CPU},
#endif
#ifdef RLIMIT_DATA
    {"data", RLIMIT_DATA},
#endif
#ifdef RLIMIT_FSIZE
    {"fsize", RLIMIT_FSIZE},
#endif
#ifdef RLIMIT_NOFILE
    {"nofile", RLIMIT_NOFILE},
#endif
#ifdef RLIMIT_MEMLOCK
    {"memlock", RLIMIT_MEMLOCK},
#endif
#ifdef RLIMIT_MSGQUEUE
    {"msgqueue", RLIMIT_MSGQUEUE},
#endif
#ifdef RLIMIT_NICE
    {"nice", RLIMIT_NICE},
#endif
#ifdef RLIMIT_RTPRIO
    {"rtprio", RLIMIT_RTPRIO},
#endif
#ifdef RLIMIT_RTTIME
    {"rttime", RLIMIT_RTTIME},
#endif
#ifdef RLIMIT_NPROC
    {"nproc", RLIMIT_NPROC},
#endif
#ifdef RLIMIT_RSS
    {"rss", RLIMIT_RSS},
#endif
#ifdef RLIMIT_SIGPENDING
    {"sigpending", RLIMIT_SIGPENDING},
#endif
#ifdef RLIMIT_STACK
    {"stack", RLIMIT_STACK},
#endif
#ifdef RLIMIT_VMEM
    {"map_mem", RLIMIT_VMEM},
    {"vmem", RLIMIT_VMEM},
#endif
#ifdef RLIMIT_AS
    {"as", RLIMIT_AS},
    {"mem", RLIMIT_AS},
#endif
};

int get_limit_id( const char *limit )
{
   unsigned int i;
   for( i=0; i<sizeof(__limits)/sizeof(__limits[0]); i++ )
     if( !strcmp( limit, __limits[i].name ) )
       return __limits[i].lim;
   return -1;
}

int get_one_limit( const char *limit, struct rlimit *rl )
{
   int lim = get_limit_id( limit );
   if( lim < 0 )
       return -2;
   return getrlimit( lim, rl );
}

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
   if (args!=1)
      SIMPLE_WRONG_NUM_ARGS_ERROR("getrlimit",1);
   if (TYPEOF(sp[-args]) != T_STRING)
      SIMPLE_ARG_TYPE_ERROR("getrlimit",1,"string");
   res = get_one_limit( Pike_sp[-args].u.string->str, &rl );
   if( res == -2 )
   {
/* no such resource on this system */
      rl.rlim_cur=(PIKE_RLIM_T)0;
      rl.rlim_max=(PIKE_RLIM_T)0;
      res=0;
   }

   if (res==-1)
   {
/* this shouldn't happen */
      Pike_error("error; errno=%d.\n",errno);
   }
   pop_n_elems(args);
#ifdef RLIM_INFINITY
   if (rl.rlim_cur==(rlim_t)RLIM_INFINITY)
      push_int(-1);
   else
#endif
      push_int64( (INT_TYPE)rl.rlim_cur );

#ifdef RLIM_INFINITY
   if (rl.rlim_max==(rlim_t)RLIM_INFINITY)
      push_int(-1);
   else
#endif
      push_int64( (INT_TYPE)rl.rlim_max );

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
   unsigned int i;
   int n=0;
   pop_n_elems(args); /* no args */
   for( i=0; i<sizeof(__limits)/sizeof(__limits[0]); i++ )
   {
       push_text( __limits[i].name );
       push_text( __limits[i].name );
       f_getrlimit(1);
       n += 2;
   }
   f_aggregate_mapping(n);
}

#endif

#ifdef HAVE_SETRLIMIT
int set_one_limit( const char *limit, INT64 soft, INT64 hard )
{
   struct rlimit rl;
   int lim;
#ifdef RLIM_INFINITY
   if( soft == -1 )
       rl.rlim_cur = RLIM_INFINITY;
   else
       rl.rlim_cur = soft;
   if( hard == -1 )
       rl.rlim_max = RLIM_INFINITY;
   else
       rl.rlim_max = hard;
#else
   rl.rlim_cur = soft;
   rl.rlim_max = hard;
#endif
   lim = get_limit_id( limit );
   if( lim < 0 )
       return -2;
   return setrlimit( lim, &rl );
}

/*! @decl int(0..1) setrlimit(string resource, int soft, int hard)
 *! Sets the @[soft] and the @[hard] process limit on a @[resource].
 *! @seealso
 *!   @[getrlimit], @[getrlimits]
 */
static void f_setrlimit(INT32 args)
{
   if (args!=3)
      SIMPLE_WRONG_NUM_ARGS_ERROR("setrlimit",3);
   if (TYPEOF(sp[-args]) != T_STRING)
      SIMPLE_ARG_TYPE_ERROR("setrlimit",1,"string");
   if (TYPEOF(sp[1-args]) != T_INT ||
       sp[1-args].u.integer<-1)
      SIMPLE_ARG_TYPE_ERROR("setrlimit",2,"int(-1..)");
   if (TYPEOF(sp[2-args]) != T_INT ||
       sp[2-args].u.integer<-1)
      SIMPLE_ARG_TYPE_ERROR("setrlimit",3,"int(-1..)");

   switch( set_one_limit( Pike_sp[-args].u.string->str, Pike_sp[1-args].u.integer, Pike_sp[2-args].u.integer ) )
   {
    case -2:
        Pike_error("No %s resource on this system.\n",
        sp[-args].u.string->str);
        break;
    case -1:
        pop_n_elems(args);
        push_int(0);
        break;
    default:
        pop_n_elems(args);
        push_int(1);
        break;
   }
}
#endif

#ifdef HAVE_SETPROCTITLE
/*! @decl void setproctitle(string title, mixed ... extra)
 *! Sets the processes title.
 */
void f_setproctitle(INT32 args)
{
  char *title;

  if (args > 1) f_sprintf(args);
  get_all_args(NULL, args, "%c", &title);
  setproctitle("%s", title);
  pop_stack();
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
   int res = 0;
   struct itimerval itimer,otimer;

   otimer.it_value.tv_usec=0;
   otimer.it_value.tv_sec=0;

   get_all_args("setitimer",args,"%+%F",&what,&interval);

   if (interval<0.0)
      SIMPLE_ARG_TYPE_ERROR("setitimer",2,"positive or zero int or float");
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
            Pike_error("Invalid timer %"PRINTPIKEINT"d.\n",what);
	    break;
	 default:
            Pike_error("Unknown error (errno=%d).\n",errno);
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
            Pike_error("Invalid timer %"PRINTPIKEINT"d.\n",what);
	    break;
	 default:
            Pike_error("Unknown error (errno=%d).\n",errno);
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

  get_all_args("get_netinfo_property", args, "%c%c%c",
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
    Pike_error("Error: %s.\n", ni_error(res));
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

#ifdef HAVE_GETLOADAVG
/*! @decl array(float) getloadavg()
 *! Get system load averages.
 *!
 *! @returns
 *! @array
 *!   @elem float 0
 *!     Load average over the last minute.
 *!   @elem float 1
 *!     Load average over the last 5 minutes.
 *!   @elem float 2
 *!     Load average over the last 15 minutes.
 *! @endarray
 */
void f_system_getloadavg(INT32 args)
{
  double load[3] = { 0.0, 0.0, 0.0 };
  int i;

  pop_n_elems(args); // No args.

  /* May return less than requested number of values */
  if (getloadavg(load, 3) == -1) {
    Pike_error("getloadavg failed.\n");
  }

  for (i = 0; i <= 2; i++) {
    push_float((FLOAT_TYPE)load[i]);
  }
  f_aggregate(3);
}
#endif // HAVE_GETLOADAVG

#ifdef RDTSC

/*! @decl int rdtsc()
 *! Executes the rdtsc (clock pulse counter) instruction
 *! and returns the result.
 */

static void f_rdtsc(INT32 args)
{
   INT64 tsc;
   pop_n_elems(args);
   RDTSC (tsc);
   push_int64(tsc);
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
      Pike_error("gettimeofday failed, errno=%d.\n",errno);
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

/*! @decl constant string CPU_TIME_IS_THREAD_LOCAL
 *!
 *! This string constant tells whether or not the CPU time, returned
 *! by e.g. @[gethrvtime], is thread local or not. The value is "yes"
 *! if it is and "no" if it isn't. The value is also "no" if there is
 *! no thread support.
 *!
 *! @seealso
 *!   @[gethrvtime], @[gauge]
 */

/*! @decl constant int CPU_TIME_RESOLUTION
 *!
 *! The resolution of the CPU time, returned by e.g. @[gethrvtime], in
 *! nanoseconds. It is @expr{-1@} if the resolution isn't known.
 *!
 *! @seealso
 *!   @[gethrvtime], @[gauge]
 */

/*! @decl constant string CPU_TIME_IMPLEMENTATION
 *!
 *! This string constant identifies the internal interface used to get
 *! the CPU time. It is an implementation detail - see rusage.c for
 *! possible values and their meanings.
 *!
 *! @seealso
 *!   @[gethrvtime], @[gauge]
 */

/*! @decl constant string REAL_TIME_IS_MONOTONIC
 *!
 *! This string constant tells whether or not the high resolution real
 *! time returned by @[gethrtime], is monotonic or not. The value is
 *! "yes" if it is and "no" if it isn't.
 *!
 *! Monotonic time is not affected by clock adjustments that might
 *! happen to keep the calendaric clock in synch. It's therefore more
 *! suited to measure time intervals in programs.
 *!
 *! @seealso
 *!   @[gethrtime]
 */

/*! @decl constant int REAL_TIME_RESOLUTION
 *!
 *! The resolution of the real time returned by @[gethrtime], in
 *! nanoseconds. It is @expr{-1@} if the resolution isn't known.
 *!
 *! @seealso
 *!   @[gethrtime]
 */

/*! @decl constant string REAL_TIME_IMPLEMENTATION
 *!
 *! This string constant identifies the internal interface used to get
 *! the high resolution real time. It is an implementation detail -
 *! see rusage.c for possible values and their meanings.
 *!
 *! @seealso
 *!   @[gethrtime]
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

   push_static_text("utime");      push_int(rusage_values[n++]);
   push_static_text("stime");      push_int(rusage_values[n++]);
   push_static_text("maxrss");     push_int(rusage_values[n++]);
   push_static_text("ixrss");      push_int(rusage_values[n++]);
   push_static_text("idrss");      push_int(rusage_values[n++]);
   push_static_text("isrss");      push_int(rusage_values[n++]);
   push_static_text("minflt");     push_int(rusage_values[n++]);
   push_static_text("majflt");     push_int(rusage_values[n++]);
   push_static_text("nswap");      push_int(rusage_values[n++]);
   push_static_text("inblock");    push_int(rusage_values[n++]);
   push_static_text("oublock");    push_int(rusage_values[n++]);
   push_static_text("msgsnd");     push_int(rusage_values[n++]);
   push_static_text("msgrcv");     push_int(rusage_values[n++]);
   push_static_text("nsignals");   push_int(rusage_values[n++]);
   push_static_text("nvcsw");      push_int(rusage_values[n++]);
   push_static_text("nivcsw");     push_int(rusage_values[n++]);
   push_static_text("sysc");       push_int(rusage_values[n++]);
   push_static_text("ioch");       push_int(rusage_values[n++]);
   push_static_text("rtime");      push_int(rusage_values[n++]);
   push_static_text("ttime");      push_int(rusage_values[n++]);
   push_static_text("tftime");     push_int(rusage_values[n++]);
   push_static_text("dftime");     push_int(rusage_values[n++]);
   push_static_text("kftime");     push_int(rusage_values[n++]);
   push_static_text("ltime");      push_int(rusage_values[n++]);
   push_static_text("slptime");    push_int(rusage_values[n++]);
   push_static_text("wtime");      push_int(rusage_values[n++]);
   push_static_text("stoptime");   push_int(rusage_values[n++]);
   push_static_text("brksize");    push_int(rusage_values[n++]);
   push_static_text("stksize");    push_int(rusage_values[n++]);

   f_aggregate_mapping(n*2);
}

#ifdef HAVE_DAEMON
/*! @decl int daemon(int nochdir, int noclose)
 *! Low level system daemon() function, see also @[Process.daemon()]
 */
void f_daemon(INT32 args)
{
   INT_TYPE a, b;
   get_all_args("daemon", args, "%i%i", &a, &b);
   push_int( daemon( a, b) );
}
#endif /* HAVE_DAEMON */

#if HAS___BUILTIN_IA32_RDRAND64_STEP

static UINT64 rand64()
{
  unsigned long long rnd;
  int i;
  for( i=0; i<10; i++ )
  {
    if( __builtin_ia32_rdrand64_step( &rnd ) )
      return rnd;
  }
  Pike_error("Unable to read random from hardware.\n");
}

/*! @decl string(8bit) hw_random(int(0..) length)
 *! Read a specified number of bytes from the hardware random
 *! generator, if available. This function will not exist if hardware
 *! random is not available. Currently only supports Intel RDRAND CPU
 *! instruction.
 */
void f_hw_random(INT32 args)
{
  INT_TYPE len, e=0;
  if(args!=1 || TYPEOF(Pike_sp[-1])!=T_INT || (len=Pike_sp[-1].u.integer)<0)
    SIMPLE_ARG_TYPE_ERROR("hw_random", 1, "int(0..)");

  UINT64 *str;
  ONERROR err;
  struct pike_string *ret = begin_shared_string(len);
  SET_ONERROR(err, do_free_unlinked_pike_string, ret);
  str = (UINT64 *)ret->str;
  while( (e+=sizeof(INT64)) <= len )
  {
    str[0] = rand64();
    str++;
  }

  UINT64 rnd = rand64();
  for(e-=sizeof(INT64); e<len; e++)
  {
    ret->str[e] = rnd&0xff;
    rnd >>= 8;
  }

  UNSET_ONERROR(err);
  pop_stack();
  push_string(end_shared_string(ret));
}
#endif

/*! @endmodule
 */

/*! @decl string strerror(int errno)
 *!
 *! This function returns a description of an error code. The error
 *! code is usually obtained from eg @[Stdio.File->errno()].
 *!
 *! @note
 *!   On some platforms the string returned can be somewhat nondescriptive.
 */
void f_strerror(INT32 args)
{
  struct pike_string *s;
  int err;

  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("strerror", 1);
  if(TYPEOF(sp[-args]) != T_INT)
    SIMPLE_ARG_TYPE_ERROR("strerror", 1, "int");

  err = sp[-args].u.integer;
  pop_n_elems(args);

  s = pike_strerror(err);
  if (s) {
    ref_push_string(s);
    return;
  }

  /* An errno we do not know about, or the lookup table
   * has not been initialized yet.
   */

#ifdef HAVE_STRERROR
  {
    char *s = NULL;
#if EINVAL > 0
    /* Some implementations of strerror index an array without
     * range checking...
     */
    if(err > 0 && err < 256 ) {
      s=strerror(err);
    }
#else
    /* Negative errnos are apparently valid on this platform
     * (eg Haiku or BeOS).
     */
    s = strerror(err);
#endif
    if(s) push_text(s);
  }
#endif

  push_static_text("Error ");
  push_int(err);
  f_add(2);
}

/*
 * Module linkage
 */

PIKE_MODULE_INIT
{
  int sz;

  /*
   * From this file:
   */
#if defined(HAVE_LINK) || defined(__NT__)

#ifdef __NT__
  if (Pike_NT_CreateHardLinkW) {
#endif
    /* function(string, string:void) */
    ADD_EFUN("hardlink", f_hardlink, tFunc(tStr tStr, tVoid), OPT_SIDE_EFFECT);
    ADD_FUNCTION2("hardlink", f_hardlink, tFunc(tStr tStr, tVoid), 0, OPT_SIDE_EFFECT);
#ifdef __NT__
  }
#endif
#endif /* HAVE_LINK || __NT__ */
#if defined(HAVE_SYMLINK) || defined(__NT__)

#ifdef __NT__
  if (Pike_NT_CreateSymbolicLinkW) {
#endif
    /* function(string, string:void) */
    ADD_EFUN("symlink", f_symlink, tFunc(tStr tStr, tVoid), OPT_SIDE_EFFECT);
    ADD_FUNCTION2("symlink", f_symlink, tFunc(tStr tStr, tVoid), 0, OPT_SIDE_EFFECT);
#ifdef __NT__
  }
#endif
#endif /* HAVE_SYMLINK || __NT__ */
#if defined(HAVE_READLINK) || defined(__NT__)

/* function(string:string) */
  ADD_EFUN("readlink", f_readlink,tFunc(tStr,tStr), OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("readlink", f_readlink,tFunc(tStr,tStr), 0, OPT_EXTERNAL_DEPEND);
#endif /* HAVE_READLINK || __NT__ */
#if defined(HAVE_RESOLVEPATH) || defined(HAVE_REALPATH)

/* function(string:string) */
  ADD_EFUN("resolvepath", f_resolvepath,tFunc(tStr,tStr), OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("resolvepath", f_resolvepath,tFunc(tStr,tStr), 0, OPT_EXTERNAL_DEPEND);
#endif /* HAVE_RESOLVEPATH || HAVE_REALPATH */

#ifdef HAVE_CLONEFILE
/* function(string, string:void) */
  ADD_FUNCTION2("clonefile", f_clonefile,tFunc(tStr tStr,tVoid), 0, OPT_SIDE_EFFECT);
#endif /* HAVE_CLONEFILE */

  /* function(int|void:int) */
  ADD_EFUN("umask", f_umask, tFunc(tOr(tInt,tVoid),tInt), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("umask", f_umask, tFunc(tOr(tInt,tVoid),tInt), 0, OPT_SIDE_EFFECT);

/* function(string, int:void) */
  ADD_EFUN("chmod", f_chmod,tFunc(tStr tInt,tVoid), OPT_SIDE_EFFECT);
  ADD_FUNCTION2("chmod", f_chmod,tFunc(tStr tInt,tVoid), 0, OPT_SIDE_EFFECT);

#ifdef HAVE_CHOWN
  ADD_EFUN("chown", f_chown, tFunc(tStr tInt tInt tOr(tVoid, tInt),tVoid),
	   OPT_SIDE_EFFECT);
  ADD_FUNCTION2("chown", f_chown, tFunc(tStr tInt tInt tOr(tVoid, tInt),tVoid),
		0, OPT_SIDE_EFFECT);
#endif

#if defined (HAVE_UTIME) || defined (HAVE__UTIME)
  ADD_EFUN("utime", f_utime,tFunc(tStr tInt tInt tOr(tVoid, tInt),tVoid),
	   OPT_SIDE_EFFECT);
  ADD_FUNCTION2("utime", f_utime,tFunc(tStr tInt tInt tOr(tVoid, tInt),tVoid),
		0, OPT_SIDE_EFFECT);
#endif

#ifdef HAVE_SYNC
  ADD_FUNCTION2("sync", f_sync, tFunc(tNone, tVoid), 0, OPT_SIDE_EFFECT);
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

#if defined(HAVE_SETPGID) || defined(HAVE_SETPGRP)
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
#ifdef HAVE_SETPROCTITLE
  ADD_FUNCTION2("setproctitle", f_setproctitle, tFuncV(tString, tMix, tVoid),
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
  ADD_EFUN("gethostname", f_gethostname,tFunc(tNone,tStr),OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("gethostname", f_gethostname,tFunc(tNone,tStr), 0,
                OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETHOSTNAME || HAVE_UNAME */

#ifdef GETHOST_DECLARE

/* function(string:array) */
  ADD_EFUN("gethostbyname", f_gethostbyname,tFunc(tStr,tArray),
           OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("gethostbyname", f_gethostbyname,tFunc(tStr,tArray), 0,
           OPT_EXTERNAL_DEPEND);

/* function(string:array) */
  ADD_EFUN("gethostbyaddr", f_gethostbyaddr,tFunc(tStr,tArray),
           OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION2("gethostbyaddr", f_gethostbyaddr,tFunc(tStr,tArray), 0,
           OPT_EXTERNAL_DEPEND);
#endif /* GETHOST_DECLARE */

  /*
   * From syslog.c:
   */
#ifdef HAVE_SYSLOG

/* function(string,int,int:void) */
  ADD_FUNCTION2("openlog", f_openlog,tFunc(tStr tInt tInt,tVoid), 0,
                OPT_SIDE_EFFECT);

/* function(int,string:void) */
  ADD_FUNCTION2("syslog", f_syslog,tFunc(tInt tStr,tVoid), 0,
                OPT_SIDE_EFFECT);

/* function(:void) */
  ADD_FUNCTION2("closelog", f_closelog,tFunc(tNone,tVoid), 0, OPT_SIDE_EFFECT);
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

#ifdef RDTSC
  ADD_FUNCTION("rdtsc",f_rdtsc,
	       tFunc(tNone,tInt),0);
#endif
#ifdef HAVE_GETTIMEOFDAY
  ADD_FUNCTION("gettimeofday",f_gettimeofday,
	       tFunc(tNone,tArr(tInt)),0);
#endif

  add_string_constant ("CPU_TIME_IS_THREAD_LOCAL",
		       cpu_time_is_thread_local ? "yes" : "no", 0);
  add_string_constant ("CPU_TIME_IMPLEMENTATION", get_cpu_time_impl, 0);
  add_integer_constant ("CPU_TIME_RESOLUTION", get_cpu_time_res(), 0);

  add_string_constant ("REAL_TIME_IS_MONOTONIC",
		       real_time_is_monotonic ? "yes" : "no", 0);
  add_string_constant ("REAL_TIME_IMPLEMENTATION", get_real_time_impl, 0);
  add_integer_constant ("REAL_TIME_RESOLUTION", get_real_time_res(), 0);

#ifdef HAVE_NETINFO_NI_H
  /* array(string) get_netinfo_property(string domain, string path,
                                        string property) */
  ADD_FUNCTION("get_netinfo_property", f_get_netinfo_property,
	       tFunc(tStr tStr tStr, tArray), 0);
#endif /* NETINFO */

#ifdef HAVE_GETLOADAVG
  ADD_FUNCTION("getloadavg", f_system_getloadavg, tFunc(tNone,tArr(tFloat)), 0);
#endif

  init_passwd();
  init_system_memory();

#ifdef HAVE_DAEMON
  ADD_FUNCTION2("daemon", f_daemon, tFunc(tInt tInt, tInt),
                0, OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);
#endif /* HAVE_DAEMON */

#if HAS___BUILTIN_IA32_RDRAND64_STEP
  {
    INT32 cpuid[4];
    x86_get_cpuid (1, cpuid);
    if( cpuid[3] & bit_RDRND_2 )
      ADD_FUNCTION("hw_random", f_hw_random, tFunc(tIntPos,tStr8), 0);
  }
#endif

#ifdef __NT__
  {
    extern void init_nt_system_calls(void);
    init_nt_system_calls();
  }
#endif

  /* errnos */
  sz = Pike_compiler->new_program->num_identifiers;
#define ADD_ERRNO(VAL, SYM, DESC) add_integer_constant(SYM, VAL, 0);
#include "add-errnos.h"
#undef ADD_ERRNO

  /* strerror lookup */
  strerror_lookup =
    allocate_mapping(Pike_compiler->new_program->num_identifiers - sz);
#define ADD_ERRNO(VAL, SYM, DESC)				\
  if (DESC[0]) {						\
    push_int(VAL);						\
    push_text(DESC);						\
    mapping_insert(strerror_lookup, Pike_sp-2, Pike_sp-1);	\
    pop_n_elems(2);						\
  }
#include "add-errnos.h"
#undef ADD_ERRNO

/* function(int:string) */
  ADD_EFUN("strerror", f_strerror, tFunc(tInt, tStr), 0);
}

PIKE_MODULE_EXIT
{
#ifdef __NT__
  {
    extern void exit_nt_system_calls(void);
    exit_nt_system_calls();
  }
#endif
  free_mapping(strerror_lookup);
  strerror_lookup = NULL;
}
