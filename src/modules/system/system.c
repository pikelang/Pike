/*
 * $Id: system.c,v 1.38 1998/01/21 19:59:11 hubbe Exp $
 *
 * System-call module for Pike
 *
 * Henrik Grubbström 1997-01-20
 */

/*
 * Includes
 */

#include "system_machine.h"
#include "system.h"

#include "global.h"
RCSID("$Id: system.c,v 1.38 1998/01/21 19:59:11 hubbe Exp $");
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
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
    error_msg = "No suck process";
    break;
  }
  error("%s(): Failed:%s\n", function_name, error_msg);
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

  get_all_args("hardlink",args, "%s%s", &from, &to);

  THREADS_ALLOW();
  do {
    err = link(from, to);
  } while ((err < 0) && (errno == EINTR));
  THREADS_DISALLOW();

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

  get_all_args("symlink",args, "%s%s", &from, &to);

  THREADS_ALLOW();
  do {
    err = symlink(from, to);
  } while ((err < 0) && (errno == EINTR));
  THREADS_DISALLOW();

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

  get_all_args("readlink",args, "%s", &path);

  buflen = 100;

  do {
    buflen *= 2;
    if (!(buf = alloca(buflen))) {
      error("readlink(): Out of memory\n");
    }

    THREADS_ALLOW();
    do {
      err = readlink(path, buf, buflen);
    } while ((err < 0) && (errno == EINTR));
    THREADS_DISALLOW();
  } while (err >= buflen - 1);

  if (err < 0) {
    report_error("readlink");
  }
  pop_n_elems(args);
  push_string(make_shared_binary_string(buf, err));
}
#endif /* HAVE_READLINK */

/* void chmod(string path, int mode) */
void f_chmod(INT32 args)
{
  char *path;
  int mode;
  int err;

  get_all_args("chmod", args, "%s%i", &path, &mode);
  THREADS_ALLOW();
  do {
    err = chmod(path, mode);
  } while ((err < 0) && (errno == EINTR));
  THREADS_DISALLOW();
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

  get_all_args("chown", args, "%s%i%i", &path, &uid, &gid);
  THREADS_ALLOW();
  do {
    err = chown(path, uid, gid);
  } while((err < 0) && (errno == EINTR));
  THREADS_DISALLOW();
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
    report_error("cleargroups");
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

#ifdef HAVE_SETUID 
void f_setuid(INT32 args)
{
  INT32 id;

  get_all_args("setuid", args, "%i", &id);
 
  if(id == -1) {
    struct passwd *pw = getpwnam("nobody");
    id = pw->pw_uid;
  } else {
    id = sp[-args].u.integer;
  }

  /* FIXME: Check return-code */
  setuid(id);
  pop_n_elems(args-1);
}
#endif

#ifdef HAVE_SETGID
void f_setgid(INT32 args)
{
  int id;

  get_all_args("setgid", args, "%i", &id);
 
  if(id == -1) {
    struct passwd *pw = getpwnam("nobody");
    id = pw->pw_gid;
  } else {
    id = sp[-args].u.integer;
  }

  /* FIXME: Check return-code */
  setgid(id);
  pop_n_elems(args-1);
}
#endif

#if defined(HAVE_SETEUID) || defined(HAVE_SETRESUID)
/* int seteuid(int euid) */
void f_seteuid(INT32 args)
{
  int id;
  int err;

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
  pop_n_elems(args-1);
  push_int(err);
}
#endif /* HAVE_SETEUID || HAVE_SETRESUID */
 
#if defined(HAVE_SETEGID) || defined(HAVE_SETRESGID)
/* int setegid(int egid) */
void f_setegid(INT32 args)
{
  int id;
  int err;

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
  pop_n_elems(args-1);
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

/* this is used from modules/file/file.c ! */
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
    if ((long)inet_addr(name) == (long)-1)
      error("Malformed ip number.\n");

    addr->sin_addr.s_addr = inet_addr(name);
  }
  else
  {
#ifdef GETHOST_DECLARE
    GETHOST_DECLARE;
    CALL_GETHOSTBYNAME(name);

    if(!ret)
      error("Invalid address '%s'\n",name);

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
    error("Invalid address '%s'\n",name);
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

#ifdef __NT__
static void f_cp(INT32 args)
{
  char *from, *to;
  int ret;
  get_all_args("cp",args,"%s%s",&from,&to);
  ret=CopyFile(from, to, 0);
  if(!ret) errno=GetLastError();
  pop_n_elems(args);
  push_int(ret);
}
#endif


/*
 * Module linkage
 */

void pike_module_init(void)
{
  /*
   * From this file:
   */
#ifdef HAVE_LINK
  add_efun("hardlink", f_hardlink, "function(string, string:void)", OPT_SIDE_EFFECT);
#endif /* HAVE_LINK */
#ifdef HAVE_SYMLINK
  add_efun("symlink", f_symlink, "function(string, string:void)", OPT_SIDE_EFFECT);
#endif /* HAVE_SYMLINK */
#ifdef HAVE_READLINK
  add_efun("readlink", f_readlink, "function(string:string)", OPT_EXTERNAL_DEPEND);
#endif /* HAVE_READLINK */
  add_efun("chmod", f_chmod, "function(string, int:void)", OPT_SIDE_EFFECT);
#ifdef HAVE_CHOWN
  add_efun("chown", f_chown, "function(string, int, int:void)", OPT_SIDE_EFFECT);
#endif
#ifdef HAVE_INITGROUPS
  add_efun("initgroups", f_initgroups, "function(string, int:void)", OPT_SIDE_EFFECT);
#endif /* HAVE_INITGROUPS */
#ifdef HAVE_SETGROUPS
  add_efun("cleargroups", f_cleargroups, "function(:void)", OPT_SIDE_EFFECT);
  /* NOT Implemented in Pike 0.5 */
  add_efun("setgroups", f_setgroups, "function(array(int):void)", OPT_SIDE_EFFECT);
#endif /* HAVE_SETGROUPS */
#ifdef HAVE_GETGROUPS
  add_efun("getgroups", f_getgroups, "function(:array(int))", OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETGROUPS */
#ifdef HAVE_SETUID
  add_efun("setuid", f_setuid, "function(int:void)", OPT_SIDE_EFFECT);
#endif
#ifdef HAVE_SETGID
  add_efun("setgid", f_setgid, "function(int:void)", OPT_SIDE_EFFECT);
#endif
#if defined(HAVE_SETEUID) || defined(HAVE_SETRESUID)
  add_efun("seteuid", f_seteuid, "function(int:int)", OPT_SIDE_EFFECT);
#endif /* HAVE_SETEUID || HAVE_SETRESUID */
#if defined(HAVE_SETEGID) || defined(HAVE_SETRESGID)
  add_efun("setegid", f_setegid, "function(int:int)", OPT_SIDE_EFFECT);
#endif /* HAVE_SETEGID || HAVE_SETRESGID */

#ifdef HAVE_GETUID
  add_efun("getuid", f_getuid, "function(:int)", OPT_EXTERNAL_DEPEND);
#endif

#ifdef HAVE_GETGID
  add_efun("getgid", f_getgid, "function(:int)", OPT_EXTERNAL_DEPEND);
#endif
 
#ifdef HAVE_GETEUID
  add_efun("geteuid", f_geteuid, "function(:int)", OPT_EXTERNAL_DEPEND);
  add_efun("getegid", f_getegid, "function(:int)", OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETEUID */
 
  add_efun("getpid", f_getpid, "function(:int)", OPT_EXTERNAL_DEPEND);
#ifdef HAVE_GETPPID
  add_efun("getppid", f_getppid, "function(:int)", OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETPPID */
 
#ifdef HAVE_GETPGRP
  add_efun("getpgrp", f_getpgrp, "function(:int)", OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETPGRP */

#ifdef HAVE_CHROOT 
  add_efun("chroot", f_chroot, "function(string|object:int)", 
           OPT_EXTERNAL_DEPEND);
#endif /* HAVE_CHROOT */
 
#ifdef HAVE_UNAME
  add_efun("uname", f_uname, "function(:mapping)", OPT_TRY_OPTIMIZE);
#endif /* HAVE_UNAME */
 
#if defined(HAVE_GETHOSTNAME) || defined(HAVE_UNAME)
  add_efun("gethostname", f_gethostname, "function(:string)",OPT_TRY_OPTIMIZE);
#endif /* HAVE_GETHOSTNAME || HAVE_UNAME */

#ifdef GETHOST_DECLARE
  add_efun("gethostbyname", f_gethostbyname, "function(string:array)",
           OPT_TRY_OPTIMIZE);
  add_efun("gethostbyaddr", f_gethostbyaddr, "function(string:array)",
           OPT_TRY_OPTIMIZE);
#endif /* GETHOST_DECLARE */

  /*
   * From syslog.c:
   */
#ifdef HAVE_SYSLOG
  add_efun("openlog", f_openlog, "function(string,int,int:void)", 0);
  add_efun("syslog", f_syslog, "function(int,string:void)", 0);
  add_efun("closelog", f_closelog, "function(:void)", 0);
#endif /* HAVE_SYSLOG */

  /*
   * From passwords.c
   */
#ifdef HAVE_GETPWNAM
  add_efun("getpwnam", f_getpwnam, "function(string:array)", 
	   OPT_EXTERNAL_DEPEND);
  add_efun("getpwuid", f_getpwuid, "function(int:array)", OPT_EXTERNAL_DEPEND);

  add_efun("getgrnam", f_getgrnam, "function(string:array)",
	   OPT_EXTERNAL_DEPEND);
  add_efun("getgrgid", f_getgrgid, "function(int:array)", OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GETPWENT
  add_efun("getpwent", f_getpwent, "function(void:int|array)",
           OPT_EXTERNAL_DEPEND);
  add_efun("endpwent", f_endpwent, "function(void:int)", OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_SETPWENT
  add_efun("setpwent", f_setpwent, "function(void:int)", OPT_EXTERNAL_DEPEND);
#endif

#ifdef GETHOSTBYNAME_MUTEX_EXISTS
  add_to_callback(& fork_child_callback, cleanup_after_fork, 0, 0);
#endif

#ifdef __NT__
  add_function("cp",f_cp,"function(string,string:int)", 0);
#endif
}

void pike_module_exit(void)
{
}
