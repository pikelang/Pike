/*
 * $Id: system.c,v 1.3 1997/01/22 02:55:12 grubba Exp $
 *
 * System-call module for Pike
 *
 * Henrik Grubbström 1997-01-20
 */

/*
 * Includes
 */

#include "system_machine.h"

#include <global.h>
RCSID("$Id: system.c,v 1.3 1997/01/22 02:55:12 grubba Exp $");
#include <las.h>
#include <interpret.h>
#include <stralloc.h>
#include <threads.h>
#include <svalue.h>

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


/*
 * Functions
 */

/* Helper functions */

static volatile void report_error(const char *function_name)
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
  case ENOLINK:
    error_msg = "Link to remote machine nolonger active";
    break;
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
  }
  error("%s(): Failed:%s\n", function_name, error_msg);
}

#if defined(HAVE_STDARG_H) || !defined(HAVE_VARARGS_H)
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
/* Fallback to this if we have neither <stdarg.h> nor <varargs.h>.
 *
 * Should work on anything that passes all arguments on the stack,
 * and the stack grows downwards.
 *
 * /grubba 1997-01-21
 */
#define va_list			void *
#define va_start(var, last_var)	(var = (va_list)(&(last_var)+1))
/* Could be a NOP as well */
#define va_end(var)		(var = 0)
/* Is the following line legal C? */
#define va_arg(var, type)	(*(((type *)var)++))
#endif /* HAVE_STDARG_H */

static void check_args(const char *fnname, int args, int minargs, ... )
{
  va_list arglist;
  int argno;
  
  if (args < minargs) {
    error("Too few arguments to %s()\n", fnname);
  }

  va_start(arglist, minargs);

  for (argno=0; argno < minargs; argno++) {
    int type_mask = va_arg(arglist, unsigned INT32);

    if (!((1UL << sp[argno-args].type) & type_mask)) {
      va_end(arglist);
      error("Bad argument %d to %s()\n", argno+1, fnname);
    }
  }

  va_end(arglist);
}

#else /* Only HAVE_VARARGS_H */

#include <varargs.h>

static void check_args(va_alist)
va_dcl
{
  const char *fnname;
  int args;
  int minargs;
  va_list arglist;
  int argno;

  va_start(arglist);

  fnname = va_arg(arglist, const char *);
  args = va_arg(arglist, int);
  minargs = va_arg(arglist, int);
  
  if (args < minargs) {
    error("Too few arguments to %s()\n", fnname);
  }

  va_start(arglist, minargs);

  for (argno=0; argno < minargs; argno++) {
    int type_mask = va_arg(arglist, unsigned INT32);

    if (!((1UL << sp[argno-args].type) & type_mask)) {
      va_end(arglist);
      error("Bad argument %d to %s()\n", argno+1, fnname);
    }
  }

  va_end(arglist);
}

#endif /* HAVE_STDARG_H || !HAVE_VARARGS_H */

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

  check_args("hardlink", args, 2, BIT_STRING, BIT_STRING);
#if 0
  if (args < 2) {
    error("Too few arguments to hardlink()\n");
  }
  if (sp[-args].type != T_STRING) {
    error("Bad argument 1 to hardlink()\n");
  }
  if (sp[1-args].type != T_STRING) {
    error("Bad argument 2 to hardlink()\n");
  }
#endif /* 0 */
  from = sp[-args].u.string->str;
  to = sp[1-args].u.string->str;

  do {
    THREADS_ALLOW();

    err = link(from, to);
    
    THREADS_DISALLOW();
  } while ((err < 0) && (errno == EINTR));

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

  check_args("symlink", args, 2, BIT_STRING, BIT_STRING);
#if 0
  if (args < 2) {
    error("Too few arguments to symlink()\n");
  }
  if (sp[-args].type != T_STRING) {
    error("Bad argument 1 to symlink()\n");
  }
  if (sp[1-args].type != T_STRING) {
    error("Bad argument 2 to symlink()\n");
  }
#endif /* 0 */
  from = sp[-args].u.string->str;
  to = sp[1-args].u.string->str;

  do {
    THREADS_ALLOW();

    err = symlink(from, to);
    
    THREADS_DISALLOW();
  } while ((err < 0) && (errno == EINTR));

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

  check_args("readlink", args, 1, BIT_STRING);
#if 0
  if (!args) {
    error("Too few arguments to readlink()\n");
  }
  if (sp[-args].type != T_STRING) {
    error("Bad argument 1 to readlink()\n");
  }
#endif /* 0 */

  path = sp[-args].u.string->str;

  buflen = 100;

  do {
    buflen *= 2;
    if (!(buf = alloca(buflen))) {
      error("readlink(): Out of memory\n");
    }

    do {
      THREADS_ALLOW();

      err = readlink(path, buf, buflen);

      THREADS_DISALLOW();
    } while ((err < 0) && (errno == EINTR));
  } while (err >= buflen - 1);

  if (err < 0) {
    report_error("readlink");
  }
  pop_n_elems(args);
  push_string(make_shared_binary_string(buf, err));
}
#endif /* HAVE_READLINK */

#ifdef HAVE_INITGROUPS
/* void initgroups(string name, int gid) */
void f_initgroups(INT32 args)
{
  check_args("initgroups", args, 2, BIT_STRING, BIT_INT);
#if 0
  if(args < 2) {
    error("Too few arguments to initgroups()\n");
  }
 
  if(sp[-args].type != T_STRING) {
    error("Bad argument 1 to initgroups()\n");
  }
  if (sp[1-args].type != T_INT) {
    error("Bad argument 2 to initgroups()\n");
  }
#endif /* 0 */
 
  initgroups(sp[-args].u.string->str, sp[1-args].u.integer);
    
  pop_n_elems(args);
}
#endif /* HAVE_INITGROUPS */
 
void f_setuid(INT32 args)
{
  int id;

  check_args("setuid", args, 1, BIT_INT);
#if 0
  if (!args) {
    error("Too few arguments to setuid()\n");
  }
 
  if (sp[-args].type != T_INT) {
    error("Bad argument 1 to setuid()\n");
  }
#endif /* 0 */
 
  if(sp[-args].u.integer == -1) {
    struct passwd *pw = getpwnam("nobody");
    id = pw->pw_uid;
  } else {
    id = sp[-args].u.integer;
  }
 
  setuid(id);
  pop_n_elems(args-1);
}

void f_setgid(INT32 args)
{
  int id;

  check_args("setgid", args, 1, BIT_INT);
#if 0
  if (!args) {
    error("Too few arguments to setgid()\n");
  }
 
  if(sp[-args].type != T_INT) {
    error("Bad argumnet 1 to setgid()\n");
  }
#endif /* 0 */
 
  if(sp[-args].u.integer == -1) {
    struct passwd *pw = getpwnam("nobody");
    id = pw->pw_gid;
  } else {
    id = sp[-args].u.integer;
  }

  setgid(id);
  pop_n_elems(args-1);
}

#if defined(HAVE_SETEUID) || defined(HAVE_SETRESUID)
void f_seteuid(INT32 args)
{
  int id;

  check_args("seteuid", args, 1, BIT_INT);
#if 0
  if (!args) {
    error("Too few arguments to seteuid()\n");
  }
 
  if (sp[-args].type != T_INT) {
    error("Bad argument 1 to seteuid()\n");
  }
#endif /* 0 */
 
  if(sp[-args].u.integer == -1) {
    struct passwd *pw = getpwnam("nobody");
    id = pw->pw_uid;
  } else {
    id = sp[-args].u.integer;
  }

#ifdef HAVE_SETEUID
  seteuid(id);
#else
  setresuid(-1, id, -1);
#endif /* HAVE_SETEUID */
  pop_n_elems(args-1);
}
 
void f_setegid(INT32 args)
{
  int id;

  check_args("setegid", args, 1, BIT_INT);
#if 0
  if (!args) {
    error("Too few arguments to setegid()\n");
  }
 
  if(sp[-args].type != T_INT) {
    error("Bad argument 1 to setegid()\n");
  }
#endif /* 0 */

  if(sp[-args].u.integer == -1)
  {
    struct passwd *pw = getpwnam("nobody");
    id = pw->pw_gid;
  } else {
    id = sp[-args].u.integer;
  }
 
#ifdef HAVE_SETEUID
  setegid(id);
#else
  setresgid(-1, id, -1);
#endif /* HAVE_SETEUID */
  pop_n_elems(args-1);
}
#endif /* HAVE_SETEUID || HAVE_SETRESUID */

#if defined(HAVE_GETPGID) || defined(HAVE_GETPGRP)
void f_getpgrp(INT32 args)
{
  int pid = 0;
  int pgid = 0

  if (args) {
    if (sp[-args].type != T_INT) {
      error("Bad argument 1 to getpgrp()\n");
    }
    id = sp[-args].u.integer;
  }
  pop_n_elems(args);
#ifdef HAVE_GETPGID
  pgid = getpgid(pid);

  if (pgid < 0) {
    char *error_msg = "Unknown reason";

    switch (errno) {
    case EPERM:
      error_msg = "Permission denied";
      break;
    case ESRCH:
      error_msg = "No such process";
      break;
    }
    error("getpgrp(): Failed: %s\n", error_msg);
  }
#elif defined(HAVE_GETPGRP)
  if (pid && (pid != getpid())) {
    error("getpgrp(): Mode not supported on this OS\n");
  }
  pgid = getpgrp();
#endif

  push_int(pgid);
}
#endif /* HAVE_GETPGID || HAVE_GETPGRP */

#define get(X) void f_##X(INT32 args){ pop_n_elems(args); push_int((INT32)X()); }

get(getuid);
get(getgid);
 
#ifdef HAVE_GETEUID
get(geteuid);
get(getegid);
#endif
get(getpid);

#ifdef HAVE_GETPPID
get(getppid);
#endif
 
#undef get

/* int chroot(string|object newroot) */
void f_chroot(INT32 args)
{
  int res;

#ifdef HAVE_FCHROOT
  check_args("chroot", args, 1, BIT_STRING|BIT_OBJECT);
#else
  check_args("chroot", args, 1, BIT_STRING);
#endif /* HAVE_FCHROOT */

#if 0
  if(args < 1) {
    error("Too few arguments to chroot()\n");
  }
#endif /* 0 */

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

#ifdef HAVE_GETHOSTBYNAME
/* array(string|array(string)) gethostbyaddr(string addr) */
void f_gethostbyaddr(INT32 args)
{
  u_long addr;
  struct hostent *hp;
  char **p, **q;
  int nelem;

  check_args("gethostbyaddr", args, 1, BIT_STRING);
#if 0
  if (!args) {
    error("Too few arguments to gethostbyaddr()\n");
  }
  if (sp[-args].type != T_STRING) {
    error("Bad argument 1 to gethostbyaddr()\n");
  }
#endif /* 0 */ 

  if ((int)(addr = inet_addr(sp[-args].u.string->str)) == -1) {
    error("gethostbyaddr(): IP-address must be of the form a.b.c.d\n");
  }
 
  pop_n_elems(args);

  THREADS_ALLOW();

  hp = gethostbyaddr((char *)&addr, sizeof (addr), AF_INET);

  THREADS_DISALLOW();

  if(!hp) {
    push_int(0);
    return;
  }
 
  push_text(hp->h_name);
  
#ifdef HAVE_H_ADDR_LIST
  nelem=0;
  for (p = hp->h_addr_list; *p != 0; p++) {
    struct in_addr in;
 
    memcpy(&in.s_addr, *p, sizeof (in.s_addr));
    push_text(inet_ntoa(in));
    nelem++;
  }
  f_aggregate(nelem);
 
  nelem=0;
  for (q = hp->h_aliases; *q != 0; q++) {
    push_text(*q);
    nelem++;
  }
  f_aggregate(nelem);
#else
  f_aggregate(0);
  f_aggregate(0);
#endif /* HAVE_H_ADDR_LIST */
  f_aggregate(3);
}  

/* array(array(string)) gethostbyname(string hostname) */ 
void f_gethostbyname(INT32 args)
{
  struct hostent *hp;
  char **p, **q;
  struct svalue *old_sp;
  char *name;
  int nelem;

  check_args("gethostbyname", args, 1, BIT_STRING);
#if 0
  if (!args) {
    error("Too few arguments to gethostbyname()\n");
  }
  if (sp[-args].type != T_STRING) {
    error("Bad argument 1 to gethostbyname()\n");
  }
#endif /* 0 */
 
  name = sp[-args].u.string->str;

  THREADS_ALLOW();

  hp = gethostbyname(name);

  THREADS_DISALLOW();
 
  pop_n_elems(args);
  
  if(!hp) {
    push_int(0);
    return;
  }
  
#ifdef HAVE_H_ADDR_LIST
  nelem=0;
  for (p = hp->h_addr_list; *p != 0; p++) {
    struct in_addr in;
 
    memcpy(&in.s_addr, *p, sizeof (in.s_addr));
    push_text(inet_ntoa(in));
    nelem++;
  }
  f_aggregate(nelem);
 
  nelem=0;
  for (q = hp->h_aliases; *q != 0; q++) {
    push_text(*q);
    nelem++;
  }
  f_aggregate(nelem);
#else
  f_aggregate(0);
  f_aggregate(0);
#endif /* HAVE_H_ADDR_LIST */
  f_aggregate(3);
}  
#endif /* HAVE_GETHOSTBYNAME */

/*
 * Module linkage
 */

void init_system_efuns(void)
{
#ifdef HAVE_LINK
  add_efun("hardlink", f_hardlink, "function(string, string:void)", OPT_SIDE_EFFECT);
#endif /* HAVE_LINK */
#ifdef HAVE_SYMLINK
  add_efun("symlink", f_symlink, "function(string, string:void)", OPT_SIDE_EFFECT);
#endif /* HAVE_SYMLINK */
#ifdef HAVE_READLINK
  add_efun("readlink", f_readlink, "function(string:string)", OPT_EXTERNAL_DEPEND);
#endif /* HAVE_READLINK */
#ifdef HAVE_INITGROUPS
  add_efun("initgroups", f_initgroups, "function(string, int:void)", OPT_SIDE_EFFECT);
#endif /* HAVE_INITGROUPS */
  add_efun("setuid", f_setuid, "function(int:void)", OPT_SIDE_EFFECT);
  add_efun("setgid", f_setgid, "function(int:void)", OPT_SIDE_EFFECT);
#if defined(HAVE_SETEUID) || defined(HAVE_SETRESUID)
  add_efun("seteuid", f_seteuid, "function(int:void)", OPT_SIDE_EFFECT);
  add_efun("setegid", f_do_setegid, "function(int:void)", OPT_SIDE_EFFECT);
#endif /* HAVE_SETEUID || HAVE_SETRESUID */

  add_efun("getuid", f_getuid, "function(:int)", OPT_EXTERNAL_DEPEND);
  add_efun("getgid", f_getgid, "function(:int)", OPT_EXTERNAL_DEPEND);
 
#ifdef HAVE_GETEUID
  add_efun("geteuid", f_geteuid, "function(:int)", OPT_EXTERNAL_DEPEND);
  add_efun("getegid", f_getegid, "function(:int)", OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETEUID */
 
  add_efun("getpid", f_getpid, "function(:int)", OPT_EXTERNAL_DEPEND);
#ifdef HAVE_GETPPID
  add_efun("getppid", f_getpid, "function(:int)", OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETPPID */
 
#ifdef HAVE_GETPGRP
  add_efun("getpgrp", f_getpgrp, "function(:int)", OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETPGRP */
 
  add_efun("chroot", f_chroot, "function(string|object:int)", 
           OPT_EXTERNAL_DEPEND);
 
#ifdef HAVE_UNAME
  add_efun("uname", f_uname, "function(:mapping)", OPT_TRY_OPTIMIZE);
#endif /* HAVE_UNAME */
 
#if defined(HAVE_GETHOSTNAME) || defined(HAVE_UNAME)
  add_efun("gethostname", f_gethostname, "function(:string)",OPT_TRY_OPTIMIZE);
#endif /* HAVE_GETHOSTNAME || HAVE_UNAME */

#ifdef HAVE_GETHOSTBYNAME
  add_efun("gethostbyname", f_gethostbyname, "function(string:array)",
           OPT_TRY_OPTIMIZE);
  add_efun("gethostbyaddr", f_gethostbyaddr, "function(string:array)",
           OPT_TRY_OPTIMIZE);
#endif /* HAVE_GETHOSTBYNAME */
}

void init_system_programs(void)
{
}

void exit_system(void)
{
}
