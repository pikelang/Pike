/*
 * $Id: passwords.c,v 1.1 1997/01/28 22:40:06 grubba Exp $
 *
 * Password handling for Pike.
 *
 * Henrik Grubbström 1997-01-28
 */

/*
 * Includes
 */

#include "system_machine.h"
#include "system.h"

#include <global.h>

RCSID("$Id: passwords.c,v 1.1 1997/01/28 22:40:06 grubba Exp $");

#include <module_support.h>
#include <interpret.h>
#include <stralloc.h>
#include <threads.h>
#include <svalue.h>

#ifdef HAVE_PASSWD_H
# include <passwd.h>
#endif /* HAVE_PASSWD_H */
 
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif /* HAVE_PWD_H */
 
#ifdef HAVE_SHADOW_H
# include <shadow.h>
#endif /* HAVE_SHADOW_H */
 
/*
 * Functions
 */

#if defined(HAVE_GETPWNAM) || defined(HAVE_GETPWUID) || defined(HAVE_SETPWENT) || defined(HAVE_ENDPWENT) || defined(HAVE_GETPWENT)
static void push_pwent(struct passwd *ent)
{
  if(!ent)
  {
    push_int(0);
    return;
  }
  push_text(ent->pw_name);
 
#ifdef HAVE_GETSPNAM
  if(!strcmp(ent->pw_passwd, "x"))
  {
    struct spwd *foo;
    if((foo = getspnam(ent->pw_name)))
      push_text(foo->sp_pwdp);
    else
      push_text("x");
  } else 
#endif /* Shadow password support */
    push_text(ent->pw_passwd);
  push_int(ent->pw_uid);
  push_int(ent->pw_gid);
  push_text(ent->pw_gecos);
  push_text(ent->pw_dir);
  push_text(ent->pw_shell);
  f_aggregate(7);
}
#endif
 
#ifdef HAVE_GETPWNAM
/* array getpwnam(string str) */
void f_getpwnam(INT32 args)
{
  char *str;
  struct passwd *foo;

  get_all_args("getpwnam", args, "%s", &str);

  foo = getpwnam(str);

  pop_n_elems(args);
  push_pwent(foo);
}

/* array getpwuid(int uid) */
void f_getpwuid(INT32 args)
{
  int uid;
  struct passwd *foo;
  
  get_all_args("getpwuid", args, "%i", &uid);

  foo = getpwuid(uid);

  pop_n_elems(args);
  push_pwent(foo);
}
#endif
 
#ifdef HAVE_GETPWENT
/* int setpwent() */
void f_setpwent(INT32 args)
{
  setpwent();
  pop_n_elems(args);
  push_int(0);
}
 
/* int endpwent() */
void f_endpwent(INT32 args)
{
  endpwent();
  pop_n_elems(args);
  push_int(0);
}

/* int|array getpwent() */ 
void f_getpwent(INT32 args)
{
  struct passwd *foo;
  pop_n_elems(args);
  foo = getpwent();
  if(!foo)
  {
    push_int(0);
    return;
  }
  push_pwent(foo);
}

#endif
 
