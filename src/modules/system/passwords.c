/*
 * $Id: passwords.c,v 1.10 1998/04/16 22:44:22 grubba Exp $
 *
 * Password handling for Pike.
 *
 * Henrik Grubbström 1997-01-28
 * Fixed to be semi-thread-safe by Hubbe. 1998-04-06
 * Notice: the *pw* and *gr* functions are NEVER really thread
 *         safe. If some other function executes a *pw* function
 *         without locking the password_protection_mutex, we are
 *         pretty much screwed.
 */

/*
 * Includes
 */
#include "system_machine.h"
#include "system.h"

#include "global.h"

RCSID("$Id: passwords.c,v 1.10 1998/04/16 22:44:22 grubba Exp $");

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

DEFINE_MUTEX(password_protection_mutex);

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
void push_pwent(struct passwd *ent)
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
    THREADS_ALLOW_UID();
    foo = getspnam(ent->pw_name);
    THREADS_DISALLOW_UID();
    if(foo)
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

#if defined(HAVE_GETGRNAM) || defined(HAVE_GETGRUID) || defined(HAVE_GETGRENT)
void push_grent(struct group *ent)
{
  if(!ent)
  {
    push_int(0);
    return;
  }
  push_text(ent->gr_name);
  push_text(ent->gr_passwd);
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
  THREADS_ALLOW_UID();
  mt_lock(&password_protection_mutex);
  foo = getgrgid( gid );
  THREADS_DISALLOW_UID();
  pop_n_elems( args );
  push_grent( foo );
  mt_unlock(&password_protection_mutex);
}
#endif /* HAVE_GETGRGID */

#ifdef HAVE_GETGRNAM
/* array getgrnam(string str) */
void f_getgrnam(INT32 args)
{
  char *str;
  struct group *foo;
  get_all_args("getgrnam", args, "%s", &str);
  THREADS_ALLOW_UID();
  mt_lock(&password_protection_mutex);
  foo = getgrnam( str );
  THREADS_DISALLOW_UID();
  pop_n_elems( args );
  push_grent( foo );
  mt_unlock(&password_protection_mutex);
}
#endif /* HAVE_GETGRNAM */

#ifdef HAVE_GETPWNAM
/* array getpwnam(string str) */
void f_getpwnam(INT32 args)
{
  char *str;
  struct passwd *foo;

  get_all_args("getpwnam", args, "%s", &str);

  THREADS_ALLOW_UID();
  mt_lock(&password_protection_mutex);
  foo = getpwnam(str);
  THREADS_DISALLOW_UID();

  pop_n_elems(args);
  push_pwent(foo);
  mt_unlock(&password_protection_mutex);
}
#endif /* HAVE_GETPWNAM */

#ifdef HAVE_GETPWUID
/* array getpwuid(int uid) */
void f_getpwuid(INT32 args)
{
  int uid;
  struct passwd *foo;
  
  get_all_args("getpwuid", args, "%i", &uid);

  THREADS_ALLOW_UID();
  mt_lock(&password_protection_mutex);
  foo = getpwuid(uid);
  THREADS_DISALLOW_UID();

  pop_n_elems(args);
  push_pwent(foo);
  mt_unlock(&password_protection_mutex);
}
#endif /* HAVE_GETPWUID */

#ifdef HAVE_SETPWENT
/* int setpwent() */
void f_setpwent(INT32 args)
{
  THREADS_ALLOW();
  mt_lock(&password_protection_mutex);
  setpwent();
  mt_unlock(&password_protection_mutex);
  THREADS_DISALLOW();
  pop_n_elems(args);
  push_int(0);
}
#endif /* HAVE_SETPWENT */
 
#ifdef HAVE_ENDPWENT
/* int endpwent() */
void f_endpwent(INT32 args)
{
  THREADS_ALLOW();
  mt_lock(&password_protection_mutex);
  endpwent();
  mt_unlock(&password_protection_mutex);
  THREADS_DISALLOW();
  pop_n_elems(args);
  push_int(0);
}
#endif /* HAVE_ENDPWENT */

#ifdef HAVE_GETPWENT
/* int|array getpwent() */ 
void f_getpwent(INT32 args)
{
  struct passwd *foo;
  pop_n_elems(args);
  THREADS_ALLOW_UID();
  mt_lock(&password_protection_mutex);
  foo = getpwent();
  THREADS_DISALLOW_UID();
  if(!foo)
  {
    push_int(0);
    return;
  }
  push_pwent(foo);
  mt_unlock(&password_protection_mutex);
}

/* This is used to save the pwents during THREADS_ALLOW(). */
struct pike_pwent {
  struct pike_pwent *next;
  struct passwd pw;
};

void f_get_all_users(INT32 args)
{
  struct pike_pwent *ppwents = NULL;
  int nels = 0;
  struct array *a;

  pop_n_elems(args);

  THREADS_ALLOW();
  mt_lock(&password_protection_mutex);

  setpwent();
  while(1)
  {
    struct passwd *pw;
    struct pike_pwent *nppwent;

    pw=getpwent();
    if(!pw) break;

    nppwent = malloc(sizeof(struct pike_pwent) + strlen(pw->pw_name) +
		     strlen(pw->pw_passwd) + strlen(pw->pw_gecos) +
		     strlen(pw->pw_dir) + strlen(pw->pw_shell) + 10);
    if (!nppwent) {
      /* FIXME: Out of memory... */
      break;
    }

    /* If pw changes here we lose... */

    /* Copy pw to nppwent. */
    nppwent->pw.pw_uid = pw->pw_uid;
    nppwent->pw.pw_gid = pw->pw_gid;
    nppwent->pw.pw_name =   (char *)(nppwent + 1);
    nppwent->pw.pw_passwd = nppwent->pw.pw_name +   strlen(pw->pw_name) + 1;
    nppwent->pw.pw_gecos =  nppwent->pw.pw_passwd + strlen(pw->pw_passwd) + 1;
    nppwent->pw.pw_dir =    nppwent->pw.pw_gecos +  strlen(pw->pw_gecos) + 1;
    nppwent->pw.pw_shell =  nppwent->pw.pw_dir +    strlen(pw->pw_dir) + 1;
    strcpy(nppwent->pw.pw_name,   pw->pw_name);
    strcpy(nppwent->pw.pw_passwd, pw->pw_passwd);
    strcpy(nppwent->pw.pw_gecos,  pw->pw_gecos);
    strcpy(nppwent->pw.pw_dir,    pw->pw_dir);
    strcpy(nppwent->pw.pw_shell,  pw->pw_shell);

    /* Link it in */
    nppwent->next = ppwents;
    ppwents = nppwent;
    nels++;
  }
  endpwent();

  mt_unlock(&password_protection_mutex);
  THREADS_DISALLOW();

  a=low_allocate_array(0, nels);

  while (ppwents) {
    struct pike_pwent *oppwent = ppwents;

    push_pwent(&(ppwents->pw));
    a = append_array(a, sp-1);
    pop_stack();

    ppwents = ppwents->next;
    free(oppwent);
  }

  push_array(a);
}

#endif /* HAVE_GETPWENT */

#ifdef HAVE_SETGRENT
/* int setgrent() */
void f_setgrent(INT32 args)
{
  THREADS_ALLOW();
  mt_lock(&password_protection_mutex);
  setgrent();
  mt_unlock(&password_protection_mutex);
  THREADS_DISALLOW();
  pop_n_elems(args);
  push_int(0);
}
#endif /* HAVE_SETGRENT */

#ifdef HAVE_ENDGRENT
/* int endgrent() */
void f_endgrent(INT32 args)
{
  THREADS_ALLOW();
  mt_lock(&password_protection_mutex);
  endgrent();
  mt_unlock(&password_protection_mutex);
  THREADS_DISALLOW();
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
  THREADS_ALLOW_UID();
  mt_lock(&password_protection_mutex);
  foo = getgrent();
  THREADS_DISALLOW_UID();
  if(!foo)
  {
    push_int(0);
    return;
  }
  push_grent(foo);
  mt_unlock(&password_protection_mutex);
}

/* This is used to save the grents during THREADS_ALLOW(). */
struct pike_grent {
  struct pike_grent *next;
  struct group gr;
};

void f_get_all_groups(INT32 args)
{
  struct pike_grent *pgrents = NULL;
  int nels = 0;
  struct array *a;

  pop_n_elems(args);

  THREADS_ALLOW();
  mt_lock(&password_protection_mutex);

  setgrent();
  while(1)
  {
    struct group *gr;
    struct pike_grent *npgrent;
    char **members;

    gr=getgrent();
    if(!gr) break;

    npgrent = malloc(sizeof(struct pike_grent) + strlen(gr->gr_name) +
		     strlen(gr->gr_passwd) + 5);

    if (!npgrent) {
      /* Out of memory... */
      break;
    }

    /* If gr changes here we lose... */

    if (gr->gr_mem) {
      int n_mem = 0;
      int memlen = 0;
      char *textspace = NULL;

      while (gr->gr_mem[n_mem]) {
	memlen += strlen(gr->gr_mem[n_mem++]) + 1;
      }

      n_mem += 2;	/* One NULL and one for buffer. */

      npgrent->gr.gr_mem = malloc(n_mem*sizeof(char *) + memlen);

      if (!npgrent->gr.gr_mem) {
	/* Out of memory... */
	free(npgrent);
	break;
      }
      textspace = (char *)(npgrent->gr.gr_mem + n_mem - 1) + 1;
      n_mem = 0;

      while(gr->gr_mem[n_mem]) {
	strcpy(textspace, gr->gr_mem[n_mem]);
	npgrent->gr.gr_mem[n_mem] = textspace;
	textspace += strlen(textspace) + 1;
	n_mem++;
      }
      npgrent->gr.gr_mem[n_mem] = NULL;
    }

    npgrent->gr.gr_gid = gr->gr_gid;
    npgrent->gr.gr_name = (char *)(npgrent + 1) + 1;
    npgrent->gr.gr_passwd = npgrent->gr.gr_name + strlen(gr->gr_name) + 1;
    strcpy(npgrent->gr.gr_name, gr->gr_name);
    strcpy(npgrent->gr.gr_passwd, gr->gr_passwd);

    npgrent->next = pgrents;
    pgrents = npgrent;
    nels++;
  }
  endgrent();

  mt_unlock(&password_protection_mutex);
  THREADS_DISALLOW();

  a = low_allocate_array(0, nels);

  while(pgrents) {
    struct pike_grent *opgrent = pgrents;

    push_grent(&(pgrents->gr));
    a = append_array(a, sp-1);
    pop_stack();

    pgrents = pgrents->next;

    if (opgrent->gr.gr_mem) {
      free(opgrent->gr.gr_mem);
    }
    free(opgrent);
  }

  push_array(a);
}

#ifdef HAVE_GETPWNAM
void f_get_groups_for_user(INT32 arg)
{
  struct group *gr;
  struct passwd *pw;
  struct array *a;
  char *user;
  ONERROR err;
  int base_gid;

  check_all_args("get_groups_for_user",arg,BIT_INT | BIT_STRING, 0);
  pop_n_elems(arg-1);
  a=low_allocate_array(0,10);
  mt_lock(&password_protection_mutex);
  if(sp[-1].type == T_INT)
  {
    int uid=sp[-1].u.integer;
    THREADS_ALLOW();
    pw=getpwuid(uid);
    THREADS_DISALLOW();
    sp[-1].u.string=make_shared_string(pw->pw_name);
    sp[-1].type=T_STRING;
    user=sp[-1].u.string->str;
  }else{
    user=sp[-1].u.string->str;
    THREADS_ALLOW();
    pw=getpwnam(user);
    THREADS_DISALLOW();
  }
  if(!pw)
  {
    mt_unlock(&password_protection_mutex);
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
    THREADS_ALLOW();

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

    THREADS_DISALLOW();
    if(!gr) break;

    push_int(gr->gr_gid);
    a=append_array(a,sp-1);
    pop_stack();
  }
  endgrent();
  mt_unlock(&password_protection_mutex);
  pop_stack();
  push_array(a);
}
#endif /* HAVE_GETPWENT */
#endif /* HAVE_GETGRENT */

void init_passwd(void)
{
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
