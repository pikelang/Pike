/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "types.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "mapping.h"
#include "macros.h"
#include "fd_control.h"
#include "threads.h"

#include "file_machine.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/stat.h>
#include <sys/param.h>
#include <signal.h>
#include <errno.h>

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

struct array *encode_stat(struct stat *s)
{
  struct array *a;
  a=allocate_array(7);
  ITEM(a)[0].u.integer=s->st_mode;
  switch(S_IFMT & s->st_mode)
  {
  case S_IFREG: ITEM(a)[1].u.integer=s->st_size; break;
  case S_IFDIR: ITEM(a)[1].u.integer=-2; break;
  case S_IFLNK: ITEM(a)[1].u.integer=-3; break;
  default: ITEM(a)[1].u.integer=-4; break;
  }
  ITEM(a)[2].u.integer=s->st_atime;
  ITEM(a)[3].u.integer=s->st_mtime;
  ITEM(a)[4].u.integer=s->st_ctime;
  ITEM(a)[5].u.integer=s->st_uid;
  ITEM(a)[6].u.integer=s->st_gid;
  return a;
}


void f_file_stat(INT32 args)
{
  struct stat st;
  int i;

  if(args<1)
    error("Too few arguments to file_stat()\n");
  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to file_stat()\n");
  if(args>1 && !IS_ZERO(sp-1-args))
  {
    i=lstat(sp[-args].u.string->str, &st);
  }else{
    i=stat(sp[-args].u.string->str, &st);
  }
  pop_n_elems(args);
  if(i==-1)
  {
    push_int(0);
  }else{
    push_array(encode_stat(&st));
  }
}

void f_werror(INT32 args)
{
  if(!args)
    error("Too few arguments to perror.\n");
  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to perror().\n");

  write_to_stderr(sp[-args].u.string->str, sp[-args].u.string->len);
  pop_n_elems(args);
}

void f_rm(INT32 args)
{
  struct stat st;
  INT32 i;

  if(!args)
    error("Too few arguments to rm()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to rm().\n");

  i=lstat(sp[-args].u.string->str, &st) != -1;

  if(i)
  {
    if(S_IFDIR == (S_IFMT & st.st_mode))
    {
      i=rmdir(sp[-args].u.string->str) != -1;
    }else{
      i=unlink(sp[-args].u.string->str) != -1;
    }
  }
      
  pop_n_elems(args);
  push_int(i);
}

void f_mkdir(INT32 args)
{
  INT32 i;
  if(!args)
    error("Too few arguments to mkdir()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to mkdir().\n");

  i=0770;

  if(args > 1)
  {
    if(sp[1-args].type != T_INT)
      error("Bad argument 2 to mkdir.\n");

    i=sp[1-args].u.integer;
  }
  i=mkdir(sp[-args].u.string->str, i) != -1;
  pop_n_elems(args);
  push_int(i);
}

void f_get_dir(INT32 args)
{
  struct svalue *save_sp=sp;
  DIR *dir;
  struct dirent *d;
  struct array *a=0;

  if(!args)
    error("Too few arguments to get_dir()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to get_dir()\n");

  dir=opendir(sp[-args].u.string->str);
  if(dir)
  {
    for(d=readdir(dir); d; d=readdir(dir))
    {
      if(d->d_name[0]=='.')
      {
	if(!d->d_name[1]) continue;
	if(d->d_name[1]=='.' && !d->d_name[2]) continue;
      }
      push_string(make_shared_binary_string(d->d_name,NAMLEN(d)));
    }
    closedir(dir);
    a=aggregate_array(sp-save_sp);
  }

  pop_n_elems(args);
  if(a)
    push_array(a);
  else
    push_int(0);
}

void f_cd(INT32 args)
{
  INT32 i;
  if(!args)
    error("Too few arguments to cd()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to cd()\n");

  i=chdir(sp[-args].u.string->str) != -1;
  pop_n_elems(args);
  push_int(i);
}

void f_getcwd(INT32 args)
{
  char *e;
#if defined(HAVE_WORKING_GETCWD) || !defined(HAVE_GETWD)
  char *tmp;
  INT32 size;

  size=1000;
  do {
    tmp=(char *)xalloc(size);
    e=(char *)getcwd(tmp,1000); 
    if (e || errno!=ERANGE) break;
    free((char *)tmp);
    size*=2;
  } while (size < 10000);
#else

#ifndef MAXPATHLEN
#define MAXPATHLEN 32768
#endif
  e=(char *)getwd((char *)malloc(MAXPATHLEN+1));
#endif
  if(!e)
    error("Failed to fetch current path.\n");

  pop_n_elems(args);
  push_string(make_shared_string(e));
  free(e);
}

void f_fork(INT32 args)
{
  pop_n_elems(args);
#if defined(HAVE_FORK1) && defined(_REENTRANT)
  push_int(fork1());
#else
  push_int(fork());
#endif
}


void f_exece(INT32 args)
{
  INT32 e;
  char **argv, **env;
  extern char **environ;
  struct svalue *save_sp;
  struct mapping *en;

  save_sp=sp-args;

  if(args < 2)
    error("Too few arguments to exece().\n");

  e=0;
  en=0;
  switch(args)
  {
  default:
    if(sp[2-args].type != T_MAPPING)
      error("Bad argument 3 to exece().\n");
    en=sp[2-args].u.mapping;
    mapping_fix_type_field(en);

    if(m_ind_types(en) & ~BIT_STRING)
      error("Bad argument 3 to exece().\n");
    if(m_val_types(en) & ~BIT_STRING)
      error("Bad argument 3 to exece().\n");

  case 2:
    if(sp[1-args].type != T_ARRAY)
      error("Bad argument 2 to exece().\n");
    array_fix_type_field(sp[1-args].u.array);

    if(sp[1-args].u.array->type_field & ~BIT_STRING)
      error("Bad argument 2 to exece().\n");

  case 1:
    if(sp[0-args].type != T_STRING)
      error("Bad argument 1 to exece().\n");
  }

  argv=(char **)xalloc((2+sp[1-args].u.array->size) * sizeof(char *));
  
  argv[0]=sp[0-args].u.string->str;

  for(e=0;e<sp[1-args].u.array->size;e++)
  {
    union anything *a;
    a=low_array_get_item_ptr(sp[1-args].u.array,e,T_STRING);
    argv[e+1]=a->string->str;
  }
  argv[e+1]=0;

  if(en)
  {
    INT32 e;
    struct array *i,*v;

    env=(char **)xalloc((1+m_sizeof(en)) * sizeof(char *));

    i=mapping_indices(en);
    v=mapping_values(en);
    
    for(e=0;e<i->size;e++)
    {
      push_string(ITEM(i)[e].u.string);
      push_string(make_shared_string("="));
      push_string(ITEM(v)[e].u.string);
      f_add(3);
      env[e]=sp[-1].u.string->str;
      sp--;
    }
      
    free_array(i);
    free_array(v);
    env[e]=0;
  }else{
    env=environ;
  }

  set_close_on_exec(0,0);
  set_close_on_exec(1,0);
  set_close_on_exec(2,0);

  execve(argv[0],argv,env);

  free((char *)argv);
  if(env != environ) free((char *)env);
  pop_n_elems(sp-save_sp);
  push_int(0);
}

void f_mv(INT32 args)
{
  INT32 i;
  if(args<2)
    error("Too few arguments to mv()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to mv().\n");

  if(sp[-args+1].type != T_STRING)
    error("Bad argument 2 to mv().\n");

  i=rename((char *)sp[-args].u.string->str, 
	   (char *)sp[-args+1].u.string->str);

  pop_n_elems(args);
  push_int(!i);
}

#ifdef HAVE_STRERROR
void f_strerror(INT32 args)
{
  char *s;

  if(!args) 
    error("Too few arguments to strerror()\n");
  if(sp[-args].type != T_INT)
    error("Bad argument 1 to strerror()\n");

  if(sp[-args].u.integer < 0 || sp[-args].u.integer > 256 )
    s=0;
  else
    s=strerror(sp[-args].u.integer);
  pop_n_elems(args);
  if(s)
    push_text(s);
  else
    push_int(0);
}
#endif

void f_errno(INT32 args)
{
  pop_n_elems(args);
  push_int(errno);
}

void init_files_efuns()
{
  set_close_on_exec(0,1);
  set_close_on_exec(1,1);
  set_close_on_exec(2,1);

  add_efun("file_stat",f_file_stat,
	   "function(string,int|void:int *)",OPT_EXTERNAL_DEPEND);
  add_efun("errno",f_errno,"function(:int)",OPT_EXTERNAL_DEPEND);
  add_efun("werror",f_werror,"function(string:void)",OPT_SIDE_EFFECT);
  add_efun("rm",f_rm,"function(string:int)",OPT_SIDE_EFFECT);
  add_efun("mkdir",f_mkdir,"function(string,void|int:int)",OPT_SIDE_EFFECT);
  add_efun("mv", f_mv, "function(string,string:int)", OPT_SIDE_EFFECT);
  add_efun("get_dir",f_get_dir,"function(string:string *)",OPT_EXTERNAL_DEPEND);
  add_efun("cd",f_cd,"function(string:int)",OPT_SIDE_EFFECT);
  add_efun("getcwd",f_getcwd,"function(:string)",OPT_EXTERNAL_DEPEND);
  add_efun("fork",f_fork,"function(:int)",OPT_SIDE_EFFECT);
  add_efun("exece",f_exece,"function(string,mixed*,void|mapping(string:string):int)",OPT_SIDE_EFFECT); 

#ifdef HAVE_STRERROR
  add_efun("strerror",f_strerror,"function(int:string)",0);
#endif
}
