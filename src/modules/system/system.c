/*
 * $Id: system.c,v 1.71 1999/06/01 21:44:49 grubba Exp $
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

RCSID("$Id: system.c,v 1.71 1999/06/01 21:44:49 grubba Exp $");
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
    error_msg = "Internal Pike error:Bad Pike string!";
    break;
  case EINVAL:
    error_msg = "Bad argument";
    break;
  case EIO:
    error_msg = "I/O error";
    break;
#ifdef ELOOP
  case ELOOP:
    error_msg = "Do deep nesting of symlinks";
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
  error("%s(): Failed: %s\n", function_name, error_msg);
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
      error("readlink(): Out of memory\n");
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
      error("resolvepath(): Out of memory\n");
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
    int setmask;
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
  int mode;
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
  int uid;
  int gid;
  int err;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    error("chown: permission denied.\n");
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

#ifdef HAVE_INITGROUPS
/* void initgroups(string name, int gid) */
void f_initgroups(INT32 args)
{
  char *user;
  int err;
  INT32 group;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    error("initgroups: permission denied.\n");
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
    error("cleargroups: permission denied.\n");
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
    error("setgroups: permission denied.\n");
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
      error("setgroups(): Bad element %d in array (expected int)\n", i);
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
  INT32 id;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    error("setuid: permission denied.\n");
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
    error("setgid: permission denied.\n");
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
  int id;
  int err;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    error("seteuid: permission denied.\n");
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
  int id;
  int err;

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    error("setegid: permission denied.\n");
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
      error("Bad argument 1 to getpgrp()\n");
    }
    pid = sp[-args].u.integer;
  }
  pop_n_elems(args);
#ifdef HAVE_GETPGID
  pgid = getpgid(pid);
#elif defined(HAVE_GETPGRP)
  if (pid && (pid != getpid())) {
    error("getpgrp(): Mode not supported on this OS\n");
  }
  pgid = getpgrp();
#endif
  if (pgid < 0) {
    report_error("getpgrp");
  }

  push_int(pgid);
}
#endif /* HAVE_GETPGID || HAVE_GETPGRP */

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
    error("chroot: permission denied.\n");
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
 
#ifdef HAVE_UNAME
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
void f_uname(INT32 args)
{
  struct svalue *old_sp;
  struct utsname foo;
 
  pop_n_elems(args);
  old_sp = sp;
 
  if(uname(&foo) < 0)
    error("uname() system call failed.\n");
 
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
#endif

#if defined(HAVE_UNAME) && (defined(SOLARIS) || !defined(HAVE_GETHOSTNAME))
void f_gethostname(INT32 args)
{
  struct utsname foo;
  pop_n_elems(args);
  if(uname(&foo) < 0)
    error("uname() system call failed.\n");
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
#endif /* HAVE_UNAME || HAVE_GETHOSTNAME */

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
      error("Malformed ip number.\n");

    addr->sin_addr.s_addr = inet_addr(name);
  }
  else
  {
#ifdef GETHOST_DECLARE
    GETHOST_DECLARE;
    CALL_GETHOSTBYNAME(name);

    if(!ret) {
      if (strlen(name) < 1024) {
	error("Invalid address '%s'\n",name);
      } else {
	error("Invalid address\n");
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
      error("Invalid address '%s'\n",name);
    } else {
      error("Invalid address\n");
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
    error("gethostbyaddr(): IP-address must be of the form a.b.c.d\n");
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

/*
 * Module linkage
 */

void pike_module_init(void)
{
  /*
   * From this file:
   */
#ifdef HAVE_LINK
  
/* function(string, string:void) */
  ADD_EFUN("hardlink", f_hardlink,tFunc(tStr tStr,tVoid), OPT_SIDE_EFFECT);
#endif /* HAVE_LINK */
#ifdef HAVE_SYMLINK
  
/* function(string, string:void) */
  ADD_EFUN("symlink", f_symlink,tFunc(tStr tStr,tVoid), OPT_SIDE_EFFECT);
#endif /* HAVE_SYMLINK */
#ifdef HAVE_READLINK
  
/* function(string:string) */
  ADD_EFUN("readlink", f_readlink,tFunc(tStr,tStr), OPT_EXTERNAL_DEPEND);
#endif /* HAVE_READLINK */
#ifdef HAVE_RESOLVEPATH
  
/* function(string:string) */
  ADD_EFUN("resolvepath", f_resolvepath,tFunc(tStr,tStr), OPT_EXTERNAL_DEPEND);
#endif /* HAVE_RESOLVEPATH */

  /* function(int|void:int) */
  ADD_EFUN("umask", f_umask, tFunc(tOr(tInt,tVoid),tInt), OPT_SIDE_EFFECT);

/* function(string, int:void) */
  ADD_EFUN("chmod", f_chmod,tFunc(tStr tInt,tVoid), OPT_SIDE_EFFECT);
#ifdef HAVE_CHOWN
  
/* function(string, int, int:void) */
  ADD_EFUN("chown", f_chown,tFunc(tStr tInt tInt,tVoid), OPT_SIDE_EFFECT);
#endif
#ifdef HAVE_INITGROUPS
  
/* function(string, int:void) */
  ADD_EFUN("initgroups", f_initgroups,tFunc(tStr tInt,tVoid), OPT_SIDE_EFFECT);
#endif /* HAVE_INITGROUPS */
#ifdef HAVE_SETGROUPS
  
/* function(:void) */
  ADD_EFUN("cleargroups", f_cleargroups,tFunc(,tVoid), OPT_SIDE_EFFECT);
  /* NOT Implemented in Pike 0.5 */
  
/* function(array(int):void) */
  ADD_EFUN("setgroups", f_setgroups,tFunc(tArr(tInt),tVoid), OPT_SIDE_EFFECT);
#endif /* HAVE_SETGROUPS */
#ifdef HAVE_GETGROUPS
  
/* function(:array(int)) */
  ADD_EFUN("getgroups", f_getgroups,tFunc(,tArr(tInt)), OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETGROUPS */
#ifdef HAVE_INNETGR
/* function(string, string|void, string|void, string|void:int) */
  ADD_EFUN("innetgr", f_innetgr,
	   tFunc(tStr tOr(tStr,tVoid) tOr(tStr,tVoid) tOr(tStr,tVoid), tInt),
	   OPT_EXTERNAL_DEPEND);
#endif /* HAVE_INNETGR */
#ifdef HAVE_SETUID
  
/* function(int:void) */
  ADD_EFUN("setuid", f_setuid,tFunc(tInt,tInt), OPT_SIDE_EFFECT);
#endif
#ifdef HAVE_SETGID
  
/* function(int:void) */
  ADD_EFUN("setgid", f_setgid,tFunc(tInt,tInt), OPT_SIDE_EFFECT);
#endif
#if defined(HAVE_SETEUID) || defined(HAVE_SETRESUID)
  
/* function(int:int) */
  ADD_EFUN("seteuid", f_seteuid,tFunc(tInt,tInt), OPT_SIDE_EFFECT);
#endif /* HAVE_SETEUID || HAVE_SETRESUID */
#if defined(HAVE_SETEGID) || defined(HAVE_SETRESGID)
  
/* function(int:int) */
  ADD_EFUN("setegid", f_setegid,tFunc(tInt,tInt), OPT_SIDE_EFFECT);
#endif /* HAVE_SETEGID || HAVE_SETRESGID */

#ifdef HAVE_GETUID
  
/* function(:int) */
  ADD_EFUN("getuid", f_getuid,tFunc(,tInt), OPT_EXTERNAL_DEPEND);
#endif

#ifdef HAVE_GETGID
  
/* function(:int) */
  ADD_EFUN("getgid", f_getgid,tFunc(,tInt), OPT_EXTERNAL_DEPEND);
#endif
 
#ifdef HAVE_GETEUID
  
/* function(:int) */
  ADD_EFUN("geteuid", f_geteuid,tFunc(,tInt), OPT_EXTERNAL_DEPEND);
  
/* function(:int) */
  ADD_EFUN("getegid", f_getegid,tFunc(,tInt), OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETEUID */
 
  
/* function(:int) */
  ADD_EFUN("getpid", f_getpid,tFunc(,tInt), OPT_EXTERNAL_DEPEND);
#ifdef HAVE_GETPPID
  
/* function(:int) */
  ADD_EFUN("getppid", f_getppid,tFunc(,tInt), OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETPPID */
 
#ifdef HAVE_GETPGRP
  
/* function(:int) */
  ADD_EFUN("getpgrp", f_getpgrp,tFunc(,tInt), OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETPGRP */

#ifdef HAVE_CHROOT 
  
/* function(string|object:int) */
  ADD_EFUN("chroot", f_chroot,tFunc(tOr(tStr,tObj),tInt), 
           OPT_EXTERNAL_DEPEND);
#endif /* HAVE_CHROOT */
 
#ifdef HAVE_UNAME
  
/* function(:mapping) */
  ADD_EFUN("uname", f_uname,tFunc(,tMapping), OPT_TRY_OPTIMIZE);
#endif /* HAVE_UNAME */
 
#if defined(HAVE_GETHOSTNAME) || defined(HAVE_UNAME)
  
/* function(:string) */
  ADD_EFUN("gethostname", f_gethostname,tFunc(,tStr),OPT_TRY_OPTIMIZE);
#endif /* HAVE_GETHOSTNAME || HAVE_UNAME */

#ifdef GETHOST_DECLARE
  
/* function(string:array) */
  ADD_EFUN("gethostbyname", f_gethostbyname,tFunc(tStr,tArray),
           OPT_TRY_OPTIMIZE);
  
/* function(string:array) */
  ADD_EFUN("gethostbyaddr", f_gethostbyaddr,tFunc(tStr,tArray),
           OPT_TRY_OPTIMIZE);
#endif /* GETHOST_DECLARE */

  /*
   * From syslog.c:
   */
#ifdef HAVE_SYSLOG
  
/* function(string,int,int:void) */
  ADD_EFUN("openlog", f_openlog,tFunc(tStr tInt tInt,tVoid), 0);
  
/* function(int,string:void) */
  ADD_EFUN("syslog", f_syslog,tFunc(tInt tStr,tVoid), 0);
  
/* function(:void) */
  ADD_EFUN("closelog", f_closelog,tFunc(,tVoid), 0);
#endif /* HAVE_SYSLOG */

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
}
