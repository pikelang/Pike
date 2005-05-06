/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: passwords.c,v 1.47 2005/05/06 00:42:35 nilsson Exp $
*/

/*
 * Password handling for Pike.
 *
 * Henrik Grubbström 1997-01-28
 * Fixed to be semi-thread-safe by Hubbe. 1998-04-06
 * Notice: the *pw* and *gr* functions are NEVER really thread
 *         safe. If some other function executes a *pw* function
 *         without locking the password_protection_mutex, we are
 *         pretty much screwed.
 *
 * NOTE: To avoid deadlocks, any locking/unlocking of password_protection_mutex
 *       MUST be done with LOCK_IMUTEX()/UNLOCK_IMUTEX().
 */

/*
 * Includes
 */
#include "global.h"

#include "system_machine.h"
#include "system.h"
#include "module_support.h"
#include "interpret.h"
#include "stralloc.h"
#include "threads.h"
#include "svalue.h"
#include "builtin_functions.h"
#include "constants.h"

#ifdef HAVE_PASSWD_H
# include <passwd.h>
# include <group.h>
#endif /* HAVE_PASSWD_H */

#ifdef HAVE_PWD_H
# include <pwd.h>
#endif /* HAVE_PWD_H */

#ifdef HAVE_GRP_H
# include <grp.h>
#endif /* HAVE_GRP_H */

#ifdef HAVE_SHADOW_H
# include <shadow.h>
#endif /* HAVE_SHADOW_H */

#define sp Pike_sp

/*
 * Emulation
 */

DEFINE_IMUTEX(password_protection_mutex);

#ifdef HAVE_GETPWENT
#ifndef HAVE_GETPWNAM
struct passwd *getpwnam(char *name)
{
  struct passwd *pw;
  setpwent();
  while(pw=getpwent())
    if(strcmp(pw->pw_name,name))
      break;
  endpwent();
  return pw;
}
#define HAVE_GETPWNAM
#endif

#ifndef HAVE_GETPWUID
struct passwd *getpwuid(int uid)
{
  struct passwd *pw;
  setpwent();
  while(pw=getpwent())
    if(pw->pw_uid == uid)
      break;
  endpwent();
  return 0;
}
#define HAVE_GETPWUID
#endif
#endif

#ifdef HAVE_GETGRENT
#ifndef HAVE_GETGRNAM
struct group *getgrnam(char *name)
{
  struct group *gr;
  setgrent();
  while(pw=getgrent())
    if(strcmp(gr->gr_name,name))
      break;
  endgrent();
  return gr;
}
#define HAVE_GETGRNAM
#endif
#endif


#define SAFE_PUSH_TEXT(X) do {						\
    char *text_ = (X);							\
    if(text_) push_text(text_);						\
    else push_empty_string();						\
  } while(0)

/*
 * Functions
 */

#if defined(HAVE_GETPWNAM) || defined(HAVE_GETPWUID) || defined(HAVE_GETPWENT)

void push_pwent(struct passwd *ent)
{
  /* NOTE: password_protection_mutex is always locked
   *       when this function is called.
   */

  if(!ent)
  {
    push_int(0);
    return;
  }
  SAFE_PUSH_TEXT(ent->pw_name);

#if defined(HAVE_GETSPNAM) || defined(HAVE_GETSPNAM_R)
  if(!strcmp(ent->pw_passwd, "x") || !strcmp(ent->pw_passwd, "*NP*"))
  {
    /* 64-bit Solaris 7 SIGSEGV's with an access to address 0xffffffff
     * if the user is not root:
     *
     * Cannot access memory at address 0xffffffff.
     * (gdb) bt
     * #0  0xff3b9d18 in ?? ()
     * #1  0xff3bd004 in ?? ()
     * #2  0xff3bd118 in ?? ()
     * #3  0xff14b484 in SO_per_src_lookup () from /usr/lib/libc.so.1
     * #4  0xff14a1d0 in nss_get_backend_u () from /usr/lib/libc.so.1
     * #5  0xff14a9ac in nss_search () from /usr/lib/libc.so.1
     * #6  0xff18fbac in getspnam_r () from /usr/lib/libc.so.1
     *
     *  /grubba 1999-05-26
     */
    struct spwd *foo;
#ifdef HAVE_GETSPNAM_R
    struct spwd bar;
    /* NOTE: buffer can be static since this function is
     *       monitored by the password_protection_mutex mutex.
     *   /grubba 1999-05-26
     */
    static char buffer[2048];

    THREADS_ALLOW_UID();

#ifdef HAVE_SOLARIS_GETSPNAM_R
    foo = getspnam_r(ent->pw_name, &bar, buffer, sizeof(buffer));
#else /* !HAVE_SOLARIS_GETSPNAM_R */
    /* Assume Linux-style getspnam_r().
     * It would be nice if the function was documented...
     *   /grubba 1999-05-27
     */
    foo = NULL;
    if (getspnam_r(ent->pw_name, &bar, buffer, sizeof(buffer), &foo) < 0) {
      foo = NULL;
    }
#endif /* HAVE_SOLARIS_GETSPNAM_R */

    THREADS_DISALLOW_UID();

#else /* !HAVE_GETSPNAM_R */
    /* getspnam() is MT-unsafe!
     * /grubba 1999-05-26
     */
    /* THREADS_ALLOW_UID(); */
    foo = getspnam(ent->pw_name);
    /* THREADS_DISALLOW_UID(); */
#endif /* HAVE_GETSPNAM_R */
    if(foo)
      push_text(foo->sp_pwdp);
    else
      push_text("x");
  } else
#endif /* Shadow password support */
  SAFE_PUSH_TEXT(ent->pw_passwd);

  push_int(ent->pw_uid);
  push_int(ent->pw_gid);

#ifdef HAVE_PW_GECOS
  SAFE_PUSH_TEXT(ent->pw_gecos);
#else /* !HAVE_PW_GECOS */
  push_text("Mister Anonymous");
#endif /* HAVE_PW_GECOS */
  SAFE_PUSH_TEXT(ent->pw_dir);
  SAFE_PUSH_TEXT(ent->pw_shell);
  f_aggregate(7);
}
#endif

#if defined(HAVE_GETGRNAM) || defined(HAVE_GETGRGID) || defined(HAVE_GETGRENT)
void push_grent(struct group *ent)
{
  if(!ent)
  {
    push_int(0);
    return;
  }
  SAFE_PUSH_TEXT(ent->gr_name);
  SAFE_PUSH_TEXT(ent->gr_passwd);
  push_int(ent->gr_gid);
  {
    char **cp = ent->gr_mem;
    int i=0;
    while(cp[i]) push_text(cp[i++]);
    f_aggregate(i);
  }
  f_aggregate(4);
}
#endif

#ifdef HAVE_GETGRGID
/*! @decl array(int|string|array(string)) getgrgid(int gid)
 *!
 *!   Get the group entry for the group with the id @[gid] using the systemfunction
 *!   @tt{getgrid(3)@}.
 *!
 *! @param gid
 *!   The id of the group
 *!
 *! @returns
 *!   An array with the information about the group
 *!   @array
 *!     @elem string 0
 *!       Group name
 *!     @elem string 1
 *!       Group password (encrypted)
 *!     @elem int 2
 *!       ID of the group
 *!     @elem array 3..
 *!       Array with UIDs of group members
 *!   @endarray
 *!
 *! @seealso
 *!   @[getgrent()]
 *!   @[getgrnam()]
 */
void f_getgrgid(INT32 args)
{
  INT_TYPE gid;
  struct group *foo;
  get_all_args("getgrgid", args, "%i", &gid);

  LOCK_IMUTEX(&password_protection_mutex);

  THREADS_ALLOW_UID();
  foo = getgrgid( gid );
  THREADS_DISALLOW_UID();
  pop_n_elems( args );
  push_grent( foo );

  UNLOCK_IMUTEX(&password_protection_mutex);
}
#endif /* HAVE_GETGRGID */

#ifdef HAVE_GETGRNAM
/*! @decl array(int|string|array(string)) getgrnam(string str)
 *!   Get the group entry for the group with the name @[str] using the
 *!   systemfunction @tt{getgrnam(3)@}.
 *!
 *! @param str
 *!   The name of the group
 *!
 *! @returns
 *!   An array with the information about the group
 *!   @array
 *!     @elem string 0
 *!       Group name
 *!     @elem string 1
 *!       Group password (encrypted)
 *!     @elem int 2
 *!       ID of the group
 *!     @elem array 3..
 *!       Array with UIDs of group members
 *!   @endarray
 *!
 *! @seealso
 *!   @[getgrent()]
 *!   @[getgrgid()]
 */
void f_getgrnam(INT32 args)
{
  char *str;
  struct group *foo;
  get_all_args("getgrnam", args, "%s", &str);

  LOCK_IMUTEX(&password_protection_mutex);

  THREADS_ALLOW_UID();
  foo = getgrnam( str );
  THREADS_DISALLOW_UID();
  pop_n_elems( args );
  push_grent( foo );

  UNLOCK_IMUTEX(&password_protection_mutex);
}
#endif /* HAVE_GETGRNAM */

#ifdef HAVE_GETPWNAM
/*! @decl array(int|string) getpwnam(string str)
 *!
 *!   Get the user entry for login @[str] using the systemfunction @tt{getpwnam(3)@}.
 *!
 *! @param str
 *!   The login name of the user whos userrecord is requested.
 *!
 *! @returns
 *!   An array with the information about the user
 *!   @array
 *!     @elem string 0
 *!       Users username (loginname)
 *!     @elem string 1
 *!       User password (encrypted)
 *!     @elem int 2
 *!       Users ID
 *!     @elem int 3
 *!       Users primary group ID
 *!     @elem string 4
 *!       Users real name an possibly some other info
 *!     @elem string 5
 *!       Users home directory
 *!     @elem string 6
 *!       Users shell
 *!   @endarray
 *!
 *! @seealso
 *!   @[getpwuid()]
 *!   @[getpwent()]
 */
void f_getpwnam(INT32 args)
{
  char *str;
  struct passwd *foo;

  get_all_args("getpwnam", args, "%s", &str);

  LOCK_IMUTEX(&password_protection_mutex);

  THREADS_ALLOW_UID();
  foo = getpwnam(str);
  THREADS_DISALLOW_UID();

  pop_n_elems(args);
  push_pwent(foo);

  UNLOCK_IMUTEX(&password_protection_mutex);
}
#endif /* HAVE_GETPWNAM */

#ifdef HAVE_GETPWUID
/*! @decl array(int|string) getpwuid(int uid)
 *!
 *!   Get the user entry for UID @[uid] using the systemfunction @tt{getpwuid(3)@}.
 *!
 *! @param uid
 *!   The uid of the user whos userrecord is requested.
 *!
 *! @returns
 *!   An array with the information about the user
 *!   @array
 *!     @elem string 0
 *!       Users username (loginname)
 *!     @elem string 1
 *!       User password (encrypted)
 *!     @elem int 2
 *!       Users ID
 *!     @elem int 3
 *!       Users primary group ID
 *!     @elem string 4
 *!       Users real name an possibly some other info
 *!     @elem string 5
 *!       Users home directory
 *!     @elem string 6
 *!       Users shell
 *!   @endarray
 *!
 *! @seealso
 *!   @[getpwnam()]
 *!   @[getpwent()]
 */
void f_getpwuid(INT32 args)
{
  INT_TYPE uid;
  struct passwd *foo;

  get_all_args("getpwuid", args, "%i", &uid);

  LOCK_IMUTEX(&password_protection_mutex);

  THREADS_ALLOW_UID();
  foo = getpwuid(uid);
  THREADS_DISALLOW_UID();

  pop_n_elems(args);
  push_pwent(foo);

  UNLOCK_IMUTEX(&password_protection_mutex);
}
#endif /* HAVE_GETPWUID */

/*! @module System */

#ifdef HAVE_SETPWENT
/*! @decl int setpwent()
 *!
 *!   Resets the @[getpwent] function to the first entry in the passwd source
 *!   using the systemfunction @tt{setpwent(3)@}.
 *!
 *! @returns
 *!   Always @expr{0@} (zero)
 *!
 *! @seealso
 *!   @[get_all_users()]
 *!   @[getpwent()]
 *!   @[endpwent()]
 */
void f_setpwent(INT32 args)
{
  LOCK_IMUTEX(&password_protection_mutex);

  THREADS_ALLOW_UID();
  setpwent();
  THREADS_DISALLOW_UID();
  pop_n_elems(args);
  push_int(0);

  UNLOCK_IMUTEX(&password_protection_mutex);
}
#endif /* HAVE_SETPWENT */

#ifdef HAVE_ENDPWENT
/*! @decl int endpwent()
 *!
 *!   Closes the passwd source opened by @[getpwent] function using the
 *!   systemfunction @tt{endpwent(3)@}.
 *!
 *! @returns
 *!   Always @expr{0@} (zero)
 *!
 *! @seealso
 *!   @[get_all_users()]
 *!   @[getpwent()]
 *!   @[setpwent()]
 */
void f_endpwent(INT32 args)
{
  LOCK_IMUTEX(&password_protection_mutex);

  THREADS_ALLOW_UID();
  endpwent();
  THREADS_DISALLOW_UID();
  pop_n_elems(args);
  push_int(0);

  UNLOCK_IMUTEX(&password_protection_mutex);
}
#endif /* HAVE_ENDPWENT */

#ifdef HAVE_GETPWENT
/*! @decl array(int|string) getpwent()
 *!
 *!   When first called, the @[getpwent] function opens the passwd source
 *!   and returns the first record using the systemfunction @tt{getpwent(3)@}.
 *!   For each following call, it returns the next record until EOF.
 *!
 *!   Call @[endpwent] when done using @[getpwent].
 *!
 *! @returns
 *!   An array with the information about the user
 *!   @array
 *!     @elem string 0
 *!       Users username (loginname)
 *!     @elem string 1
 *!       User password (encrypted)
 *!     @elem int 2
 *!       Users ID
 *!     @elem int 3
 *!       Users primary group ID
 *!     @elem string 4
 *!       Users real name an possibly some other info
 *!     @elem string 5
 *!       Users home directory
 *!     @elem string 6
 *!       Users shell
 *!   @endarray
 *!
 *!   0 if EOF.
 *!
 *! @seealso
 *!   @[get_all_users()]
 *!   @[getpwnam()]
 *!   @[getpwent()]
 *!   @[setpwent()]
 *!   @[endpwent()]
 */
void f_getpwent(INT32 args)
{
  struct passwd *foo;
  pop_n_elems(args);

  LOCK_IMUTEX(&password_protection_mutex);

  THREADS_ALLOW_UID();
  foo = getpwent();
  THREADS_DISALLOW_UID();
  if(!foo)
  {
    UNLOCK_IMUTEX(&password_protection_mutex);

    push_int(0);
    return;
  }
  push_pwent(foo);

  UNLOCK_IMUTEX(&password_protection_mutex);
}

/*! @endmodule */

/*! @decl array(array(int|string)) get_all_users()
 *!
 *! Returns an array with all users in the system.
 *!
 *! @returns
 *!   An array with arrays of userinfo as in @[getpwent].
 *!
 *! @seealso
 *!   @[getpwent()]
 *!   @[getpwnam()]
 *!   @[getpwuid()]
 */
void f_get_all_users(INT32 args)
{
  struct array *a;

  pop_n_elems(args);
  a = low_allocate_array(0, 10);

  LOCK_IMUTEX(&password_protection_mutex);

  setpwent();
  while(1)
  {
    struct passwd *pw;

    THREADS_ALLOW_UID();
    pw=getpwent();
    THREADS_DISALLOW_UID();

    if(!pw) break;

    push_pwent(pw);
    a = append_array(a, sp-1);
    pop_stack();
  }
  endpwent();

  UNLOCK_IMUTEX(&password_protection_mutex);

  push_array(a);
}

#endif /* HAVE_GETPWENT */

/*! @module System */

#ifdef HAVE_SETGRENT
/*! @decl int setgrent()
 *!
 *!   Rewinds the @[getgrent] pointer to the first entry
 *!
 *! @seealso
 *!   @[get_all_groups()]
 *!   @[getgrent()]
 *!   @[endgrent()]
 */
void f_setgrent(INT32 args)
{
  LOCK_IMUTEX(&password_protection_mutex);

  THREADS_ALLOW_UID();
  setgrent();
  THREADS_DISALLOW_UID();

  UNLOCK_IMUTEX(&password_protection_mutex);

  pop_n_elems(args);
  push_int(0);
}
#endif /* HAVE_SETGRENT */

#ifdef HAVE_ENDGRENT
/*! @decl int endgrent()
 *!
 *!   Closes the /etc/groups file after using the @[getgrent] function.
 *!
 *! @seealso
 *!   @[get_all_groups()]
 *!   @[getgrent()]
 *!   @[setgrent()]
 */
void f_endgrent(INT32 args)
{
  LOCK_IMUTEX(&password_protection_mutex);

  THREADS_ALLOW_UID();
  endgrent();
  THREADS_DISALLOW_UID();

  UNLOCK_IMUTEX(&password_protection_mutex);

  pop_n_elems(args);
  push_int(0);
}
#endif /* HAVE_ENDGRENT */

#ifdef HAVE_GETGRENT
/*! @decl array(int|string|array(string)) getgrent()
 *!
 *!   Get a group entry from /etc/groups file.
 *!   @[getgrent] interates thru the groups source and returns
 *!   one entry per call using the systemfunction @tt{getgrent(3)@}.
 *!
 *!   Always call @[endgrent] when done using @[getgrent]!
 *!
 *! @returns
 *!   An array with the information about the group
 *!   @array
 *!     @elem string 0
 *!       Group name
 *!     @elem string 1
 *!       Group password (encrypted)
 *!     @elem int 2
 *!       ID of the group
 *!     @elem array 3..
 *!       Array with UIDs of group members
 *!   @endarray
 *!
 *! @seealso
 *!   @[get_all_groups()]
 *!   @[getgrnam()]
 *!   @[getgrgid()]
 */
void f_getgrent(INT32 args)
{
  struct group *foo;
  pop_n_elems(args);

  LOCK_IMUTEX(&password_protection_mutex);

  THREADS_ALLOW_UID();
  foo = getgrent();
  THREADS_DISALLOW_UID();
  if(!foo)
  {
    UNLOCK_IMUTEX(&password_protection_mutex);

    push_int(0);
    return;
  }
  push_grent(foo);

  UNLOCK_IMUTEX(&password_protection_mutex);
}

/*! @endmodule */

/*! @decl array(array(int|string|array(string))) get_all_groups()
 *!
 *!   Returns an array of arrays with all groups in the system groups source.
 *!   Each element in the returned array has the same structure as in
 *!   @[getgrent] function.
 *!
 *! @note
 *!   The groups source is system dependant. Refer to your system manuals for information
 *!   about how to set the source.
 *!
 *! @returns
 *!   @array
 *!     @elem array(int|string|array(string)) 0..
 *!       Array with info about the group
 *!   @endarray
 *!
 *! @seealso
 *!   @[getgrent()]
 */
void f_get_all_groups(INT32 args)
{
  struct array *a;

  pop_n_elems(args);

  a = low_allocate_array(0, 10);

  LOCK_IMUTEX(&password_protection_mutex);

  setgrent();
  while(1)
  {
    struct group *gr;

    THREADS_ALLOW_UID();
    gr=getgrent();
    THREADS_DISALLOW_UID();

    if(!gr) break;

    push_grent(gr);
    a = append_array(a, sp-1);
    pop_stack();
  }
  endgrent();

  UNLOCK_IMUTEX(&password_protection_mutex);

  push_array(a);
}

#ifdef HAVE_GETPWNAM
/*! @decl array(int) get_groups_for_user(int|string user)
 *!
 *!   Gets all groups which a given user is a member of.
 *!
 *! @param user
 *!   UID or loginname of the user
 *!
 *! @returns
 *!   @array
 *!     @elem array 0..
 *!       Information about all the users groups
 *!   @endarray
 *!
 *! @seealso
 *!   @[get_all_groups()]
 *!   @[getgrgid()]
 *!   @[getgrnam()]
 *!   @[getpwuid()]
 *!   @[getpwnam()]
 */
void f_get_groups_for_user(INT32 arg)
{
  struct group *gr = NULL;
  struct passwd *pw;
  struct array *a;
  char *user = NULL;	/* Keep compiler happy */
  ONERROR err;
  int base_gid;

  check_all_args("get_groups_for_user",arg,BIT_INT | BIT_STRING, 0);
  pop_n_elems(arg-1);
  a=low_allocate_array(0,10);
  if(sp[-1].type == T_INT)
  {
    int uid=sp[-1].u.integer;

    LOCK_IMUTEX(&password_protection_mutex);

    THREADS_ALLOW_UID();
    pw=getpwuid(uid);
    THREADS_DISALLOW_UID();

    if(pw)
    {
      sp[-1].u.string=make_shared_string(pw->pw_name);
      sp[-1].type=T_STRING;
      user=sp[-1].u.string->str;
    }
  }else{
    user=sp[-1].u.string->str;

    LOCK_IMUTEX(&password_protection_mutex);

    THREADS_ALLOW_UID();
    pw=getpwnam(user);
    THREADS_DISALLOW_UID();
  }

  /* NOTE: password_protection_mutex is still locked here. */

  if(!pw)
  {
    UNLOCK_IMUTEX(&password_protection_mutex);

    pop_stack();
    push_int(0);
    return;
  }
  push_int(base_gid=pw->pw_gid);
  a=append_array(a,sp-1);
  pop_stack();

  setgrent();

  do {
    int e;
    THREADS_ALLOW_UID();

    while ((gr = getgrent()))
    {
      if((size_t)gr->gr_gid == (size_t)base_gid) continue;
      for(e=0;gr->gr_mem[e];e++)
	if(!strcmp(gr->gr_mem[e],user))
	  break;
      if(gr->gr_mem[e]) break;
    }

    THREADS_DISALLOW_UID();		/* Is there a risk of deadlock here? */
    if(!gr) break;

    push_int(gr->gr_gid);
    a=append_array(a,sp-1);
    pop_stack();
  } while(gr);
  endgrent();

  UNLOCK_IMUTEX(&password_protection_mutex);

  pop_stack();
  push_array(a);
}
#endif /* HAVE_GETPWENT */
#endif /* HAVE_GETGRENT */

void init_passwd(void)
{
  init_interleave_mutex(&password_protection_mutex);

  /*
   * From passwords.c
   */
#ifdef HAVE_GETPWNAM

/* function(string:array(int|string)) */
  ADD_EFUN("getpwnam", f_getpwnam,tFunc(tStr,tArr(tOr(tInt,tStr))),
	   OPT_SIDE_EFFECT |OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GETPWUID

/* function(int:array(int|string)) */
  ADD_EFUN("getpwuid", f_getpwuid,tFunc(tInt,tArr(tOr(tInt,tStr))), OPT_SIDE_EFFECT |OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GETGRNAM

/* function(string:array(int|string|array(string))) */
  ADD_EFUN("getgrnam", f_getgrnam,tFunc(tStr,tArr(tOr3(tInt,tStr,tArr(tStr)))),
	   OPT_SIDE_EFFECT |OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GETGRGID

/* function(int:array(int|string|array(string))) */
  ADD_EFUN("getgrgid", f_getgrgid,tFunc(tInt,tArr(tOr3(tInt,tStr,tArr(tStr)))), OPT_SIDE_EFFECT |OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GETPWENT

  /* function(void:array(int|string)) */
  ADD_FUNCTION("getpwent", f_getpwent,tFunc(tVoid,tArr(tOr(tInt,tStr))),
	       OPT_SIDE_EFFECT |OPT_EXTERNAL_DEPEND);

  /* function(void:array(array(int|string))) */
  ADD_EFUN("get_all_users", f_get_all_users,tFunc(tVoid,tArr(tArr(tOr(tInt,tStr)))),
           OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_ENDPWENT

  /* function(void:int) */
  ADD_FUNCTION("endpwent", f_endpwent,tFunc(tVoid,tInt),
	       OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_SETPWENT

  /* function(void:int) */
  ADD_FUNCTION("setpwent", f_setpwent,tFunc(tVoid,tInt),
	       OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GETGRENT

  /* function(void:array(int|string|array(string))) */
  ADD_FUNCTION("getgrent", f_getgrent,
	       tFunc(tVoid,tArr(tOr3(tInt,tStr,tArr(tStr)))),
	       OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);

  /* function(void:array(array(int|string|array(string)))) */
  ADD_EFUN("get_all_groups", f_get_all_groups,tFunc(tVoid,tArr(tArr(tOr3(tInt,tStr,tArr(tStr))))),
           OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);

#ifdef HAVE_GETPWENT

/* function(int|string:array(int)) */
  ADD_EFUN("get_groups_for_user", f_get_groups_for_user,tFunc(tOr(tInt,tStr),tArr(tInt)),
           OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETPWENT */
#endif
#ifdef HAVE_ENDGRENT

  /* function(void:int) */
  ADD_FUNCTION("endgrent", f_endgrent,tFunc(tVoid,tInt),
	       OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_SETGRENT

  /* function(void:int) */
  ADD_FUNCTION("setgrent", f_setgrent,tFunc(tVoid,tInt),
	       OPT_SIDE_EFFECT |OPT_EXTERNAL_DEPEND);
#endif
}
