#include "global.h"
#include "types.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "mapping.h"
#include "macros.h"
#include "fd_control.h"

#include "file_machine.h"

#include <sys/types.h>
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
  a=allocate_array_no_init(7,0,T_INT);
  SHORT_ITEM(a)[0].integer=s->st_mode;
  switch(S_IFMT & s->st_mode)
  {
  case S_IFREG: SHORT_ITEM(a)[1].integer=s->st_size; break;
  case S_IFDIR: SHORT_ITEM(a)[1].integer=-2; break;
  case S_IFLNK: SHORT_ITEM(a)[1].integer=-3; break;
  default: SHORT_ITEM(a)[1].integer=-4; break;
  }
  SHORT_ITEM(a)[2].integer=s->st_atime;
  SHORT_ITEM(a)[3].integer=s->st_mtime;
  SHORT_ITEM(a)[4].integer=s->st_ctime;
  SHORT_ITEM(a)[5].integer=s->st_uid;
  SHORT_ITEM(a)[6].integer=s->st_gid;
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

void f_perror(INT32 args)
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
    a=aggregate_array(sp-save_sp, T_STRING);
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
  pop_n_elems(args);

#ifdef HAVE_GETWD
  e=(char *)getwd((char *)malloc(MAXPATHLEN+1));
  if(!e)
    fatal("Couldn't fetch current path.\n");
#else
#ifdef HAVE_GETCWD
  e=(char *)getcwd(0,1000); 
#endif
#endif
  push_string(make_shared_string(e));
  free(e);
}

void f_fork(INT32 args)
{
  pop_n_elems(args);
  push_int(fork());
}

void f_kill(INT32 args)
{
  if(args < 2)
    error("Too few arguments to kill().\n");
  if(sp[-args].type != T_INT)
    error("Bad argument 1 to kill().\n");
  if(sp[1-args].type != T_INT)
    error("Bad argument 1 to kill().\n");

  sp[-args].u.integer=!kill(sp[-args].u.integer,sp[1-args].u.integer);
  pop_n_elems(args-1);
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
  switch(args)
  {
  default:
    if(sp[2-args].type != T_MAPPING)
      error("Bad argument 3 to exece().\n");
    en=sp[2-args].u.mapping;
    array_fix_type_field(en->ind);
    array_fix_type_field(en->val);

    if(en->ind->type_field & ~BIT_STRING)
      error("Bad argument 3 to exece().\n");
    if(en->val->type_field & ~BIT_STRING)
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

  if(args>2)
  {
    env=(char **)xalloc((1+en->ind->size) * sizeof(char *));

    for(e=0;e<en->ind->size;e++)
    {
      union anything *a;
      a=low_array_get_item_ptr(en->ind,e,T_STRING);
      push_string(a->string);
      a->string->refs++;

      push_string(make_shared_string("="));
      a=low_array_get_item_ptr(en->val,e,T_STRING);
      push_string(a->string);
      a->string->refs++;

      f_sum(3);

      env[e]=sp[-1].u.string->str;
    }
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

void init_files_efuns()
{
  set_close_on_exec(0,1);
  set_close_on_exec(1,1);
  set_close_on_exec(2,1);

  add_efun("file_stat",f_file_stat,
	   "function(string,int|void:int *)",OPT_EXTERNAL_DEPEND);
  add_efun("perror",f_perror,"function(string:void)",OPT_SIDE_EFFECT);
  add_efun("rm",f_rm,"function(string:int)",OPT_SIDE_EFFECT);
  add_efun("mkdir",f_mkdir,"function(string,void|int:int)",OPT_SIDE_EFFECT);
  add_efun("mv", f_mv, "function(string,string:int)", OPT_SIDE_EFFECT);
  add_efun("get_dir",f_get_dir,"function(string:string *)",OPT_EXTERNAL_DEPEND);
  add_efun("cd",f_cd,"function(string:int)",OPT_SIDE_EFFECT);
  add_efun("getcwd",f_getcwd,"function(:string)",OPT_EXTERNAL_DEPEND);
  add_efun("fork",f_fork,"function(:int)",OPT_SIDE_EFFECT);
  add_efun("kill",f_kill,"function(int,int:int)",OPT_SIDE_EFFECT);
  add_efun("exece",f_exece,"function(string,mixed*,void|mapping(string:string):int)",OPT_SIDE_EFFECT); 
}
