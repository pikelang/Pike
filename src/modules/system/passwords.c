/*
 * $Id: passwords.c,v 1.22 1998/07/22 01:06:55 grubba Exp $
 *
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

RCSID("$Id: passwords.c,v 1.22 1998/07/22 01:06:55 grubba Exp $");

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

 
/*
 * Functions
 */

#if defined(HAVE_GETPWNAM) || defined(HAVE_GETPWUID) || defined(HAVE_GETPWENT)

#define SAFE_PUSH_TEXT(X) do { char *text_ = (X); if(text_) push_text(text_); else push_constant_text(""); }while(0);
void push_pwent(struct passwd *ent)
{
  if(!ent)
  {
    push_int(0);
    return;
  }
  SAFE_PUSH_TEXT(ent->pw_name);

#ifdef HAVE_GETSPNAM
  if(!strcmp(ent->pw_passwd, "x"))
  {
    struct spwd *foo;
    THREADS_ALLOW();
    foo = getspnam(ent->pw_name);
    THREADS_DISALLOW();
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

#if defined(HAVE_GETGRNAM) || defined(HAVE_GETGRUID) || defined(HAVE_GETGRENT)
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
/* array getgrgid(int gid) */
void f_getgrgid(INT32 args)
{
  int gid;
  struct group *foo;
  get_all_args("getgrgid", args, "%d", &gid);

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
/* array getgrnam(string str) */
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
/* array getpwnam(string str) */
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
/* array getpwuid(int uid) */
void f_getpwuid(INT32 args)
{
  int uid;
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

#ifdef HAVE_SETPWENT
/* int setpwent() */
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
/* int endpwent() */
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
/* int|array getpwent() */ 
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
    struct pike_pwent *nppwent;

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

#ifdef HAVE_SETGRENT
/* int setgrent() */
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
/* int endgrent() */
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
/* int|array getgrent() */
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
    push_int(0);
    return;
  }
  push_grent(foo);

  UNLOCK_IMUTEX(&password_protection_mutex);
}

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
void f_get_groups_for_user(INT32 arg)
{
  struct group *gr;
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
  while(1)
  {
    int e;
    THREADS_ALLOW_UID();

    while(1)
    {
      gr=getgrent();
      if(!gr) break;
      if(gr->gr_gid == base_gid) continue;
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
  }
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
  add_efun("getpwnam", f_getpwnam, "function(string:array(int|string))", 
	   OPT_SIDE_EFFECT |OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GETPWUID
  add_efun("getpwuid", f_getpwuid, "function(int:array(int|string))", OPT_SIDE_EFFECT |OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GETGRNAM
  add_efun("getgrnam", f_getgrnam, "function(string:array(int|string|array(string)))",
	   OPT_SIDE_EFFECT |OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GETGRGID
  add_efun("getgrgid", f_getgrgid, "function(int:array(int|string|array(string)))", OPT_SIDE_EFFECT |OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GETPWENT
  add_efun("getpwent", f_getpwent, "function(void:array(int|string))",
           OPT_SIDE_EFFECT |OPT_EXTERNAL_DEPEND);
  add_efun("get_all_users", f_get_all_users, "function(void:array(array(int|string)))",
           OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_ENDPWENT
  add_efun("endpwent", f_endpwent, "function(void:int)", OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_SETPWENT
  add_efun("setpwent", f_setpwent, "function(void:int)", OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GETGRENT
  add_efun("getgrent", f_getgrent, "function(void:array(int|string|array(string)))",
           OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);
  add_efun("get_all_groups", f_get_all_groups, "function(void:array(array(int|string|array(string))))",
           OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);

#ifdef HAVE_GETPWENT
  add_efun("get_groups_for_user", f_get_groups_for_user, "function(int|string:array(int))",
           OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);
#endif /* HAVE_GETPWENT */
#endif
#ifdef HAVE_ENDGRENT
  add_efun("endgrent", f_endgrent, "function(void:int)", OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_SETGRENT
  add_efun("setgrent", f_setgrent, "function(void:int)", OPT_SIDE_EFFECT |OPT_EXTERNAL_DEPEND);
#endif
}
