/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: gdbmmod.c,v 1.6 1999/02/01 02:42:09 hubbe Exp $");
#include "gdbm_machine.h"
#include "threads.h"

/* Todo: make sure only one thread accesses the same gdbmmod */

#if defined(HAVE_GDBM_H) && defined(HAVE_LIBGDBM)

#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "object.h"
#include "pike_macros.h"

#include <gdbm.h>

#ifdef _REENTRANT
static MUTEX_T gdbm_lock;
#endif  

struct gdbm_glue
{
  GDBM_FILE dbf;
};

#define THIS ((struct gdbm_glue *)(fp->current_storage))

static void do_free(void)
{
  if(THIS->dbf)
  {
    GDBM_FILE dbf;
    dbf=THIS->dbf;
    THIS->dbf=0;

    THREADS_ALLOW();
    mt_lock(& gdbm_lock);
    gdbm_close(dbf);
    mt_unlock(& gdbm_lock);
    THREADS_DISALLOW();
  }
}

static int fixmods(char *mods)
{
  int mode;
  mode=0;
  while(1)
  {
    switch(*(mods++))
    {
    case 0:
      switch(mode & 15)
      {
      default: error("No mode given for gdbm->open()\n"); 
      case 1|16:
      case 1: mode=GDBM_READER; break;
      case 3: mode=GDBM_WRITER; break;
      case 3|16: mode=GDBM_WRITER | GDBM_FAST; break;
      case 7: mode=GDBM_WRCREAT; break;
      case 7|16: mode=GDBM_WRCREAT | GDBM_FAST; break;
      case 15: mode=GDBM_NEWDB; break;
      case 15|16: mode=GDBM_NEWDB | GDBM_FAST; break;
      }
      return mode;

    case 'r': case 'R': mode|=1;  break;
    case 'w': case 'W': mode|=3;  break;
    case 'c': case 'C': mode|=7;  break;
    case 't': case 'T': mode|=15; break;
    case 'f': case 'F': mode|=16; break;

    default:
      error("Bad mode flag in gdbm->open.\n");
    }
  }
}

void gdbmmod_fatal(char *err)
{
  error("GDBM: %s\n",err);
}

static void gdbmmod_create(INT32 args)
{
  struct gdbm_glue *this=THIS;
  do_free();
  if(args)
  {
    GDBM_FILE tmp;
    struct pike_string *tmp2;
    int rwmode = GDBM_WRCREAT;

    if(sp[-args].type != T_STRING)
      error("Bad argument 1 to gdbm->create()\n");

    if(args>1)
    {
      if(sp[1-args].type != T_STRING)
	error("Bad argument 2 to gdbm->create()\n");

      rwmode=fixmods(sp[1-args].u.string->str);
    }

    tmp2=sp[-args].u.string;

    THREADS_ALLOW();
    mt_lock(& gdbm_lock);
    tmp=gdbm_open(tmp2->str, 512, rwmode, 00666, gdbmmod_fatal);
    mt_unlock(& gdbm_lock);
    THREADS_DISALLOW();

    if(!fp->current_object->prog)
    {
      if(tmp) gdbm_close(tmp);
      error("Object destructed in gdbm->open()n");
    }
    THIS->dbf=tmp;

    pop_n_elems(args);
    if(!THIS->dbf)
      error("Failed to open GDBM database.\n");
  }
}

#define STRING_TO_DATUM(dat, st) dat.dptr=st->str,dat.dsize=st->len;
#define DATUM_TO_STRING(dat) make_shared_binary_string(dat.dptr, dat.dsize)

static void gdbmmod_fetch(INT32 args)
{
  struct gdbm_glue *this=THIS;
  datum key,ret;

  if(!args)
    error("Too few arguments to gdbm->fetch()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to gdbm->fetch()\n");

  if(!THIS->dbf)
    error("GDBM database not open.\n");

  STRING_TO_DATUM(key, sp[-args].u.string);

  THREADS_ALLOW();
  mt_lock(& gdbm_lock);
  ret=gdbm_fetch(this->dbf, key);
  mt_unlock(& gdbm_lock);
  THREADS_DISALLOW();

  pop_n_elems(args);
  if(ret.dptr)
  {
    push_string(DATUM_TO_STRING(ret));
    free(ret.dptr);
  }else{
    push_int(0);
  }
}

static void gdbmmod_delete(INT32 args)
{
  struct gdbm_glue *this=THIS;
  datum key;
  int ret;
  if(!args)
    error("Too few arguments to gdbm->delete()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to gdbm->delete()\n");

  if(!this->dbf)
    error("GDBM database not open.\n");

  STRING_TO_DATUM(key, sp[-args].u.string);

  THREADS_ALLOW();
  mt_lock(& gdbm_lock);
  ret=gdbm_delete(this->dbf, key);
  mt_unlock(& gdbm_lock);
  THREADS_DISALLOW();
  
  pop_n_elems(args);
  push_int(0);
}

static void gdbmmod_firstkey(INT32 args)
{
  struct gdbm_glue *this=THIS;
  datum ret;
  pop_n_elems(args);

  if(!this->dbf) error("GDBM database not open.\n");

  THREADS_ALLOW();
  mt_lock(& gdbm_lock);
  ret=gdbm_firstkey(this->dbf);
  mt_unlock(& gdbm_lock);
  THREADS_DISALLOW();

  if(ret.dptr)
  {
    push_string(DATUM_TO_STRING(ret));
    free(ret.dptr);
  }else{
    push_int(0);
  }
}

static void gdbmmod_nextkey(INT32 args)
{
  struct gdbm_glue *this=THIS;
  datum key,ret;
  if(!args)
    error("Too few arguments to gdbm->nextkey()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to gdbm->nextkey()\n");

  if(!THIS->dbf)
    error("GDBM database not open.\n");

  STRING_TO_DATUM(key, sp[-args].u.string);

  THREADS_ALLOW();
  mt_lock(& gdbm_lock);
  ret=gdbm_nextkey(this->dbf, key);
  mt_unlock(& gdbm_lock);
  THREADS_DISALLOW();

  pop_n_elems(args);
  if(ret.dptr)
  {
    push_string(DATUM_TO_STRING(ret));
    free(ret.dptr);
  }else{
    push_int(0);
  }
}

static void gdbmmod_store(INT32 args)
{
  struct gdbm_glue *this=THIS;
  datum key,data;
  int ret;
  if(args<2)
    error("Too few arguments to gdbm->store()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to gdbm->store()\n");

  if(sp[1-args].type != T_STRING)
    error("Bad argument 2 to gdbm->store()\n");

  if(!THIS->dbf)
    error("GDBM database not open.\n");

  STRING_TO_DATUM(key, sp[-args].u.string);
  STRING_TO_DATUM(data, sp[1-args].u.string);

  THREADS_ALLOW();
  mt_lock(& gdbm_lock);
  ret=gdbm_store(this->dbf, key, data, GDBM_REPLACE);
  mt_unlock(& gdbm_lock);
  THREADS_DISALLOW();

  if(ret == -1)
    error("GDBM database not open for writing.\n");

  pop_n_elems(args);
  push_int(ret == 0);
}

static void gdbmmod_reorganize(INT32 args)
{
  struct gdbm_glue *this=THIS;
  int ret;
  pop_n_elems(args);

  if(!THIS->dbf) error("GDBM database not open.\n");
  THREADS_ALLOW();
  mt_lock(& gdbm_lock);
  ret=gdbm_reorganize(this->dbf);
  mt_unlock(& gdbm_lock);
  THREADS_DISALLOW();
  pop_n_elems(args);
  push_int(ret);
}

static void gdbmmod_sync(INT32 args)
{
  struct gdbm_glue *this=THIS;
  pop_n_elems(args);

  if(!THIS->dbf) error("GDBM database not open.\n");
  THREADS_ALLOW();
  mt_lock(& gdbm_lock);
  gdbm_sync(this->dbf);
  mt_unlock(& gdbm_lock);
  THREADS_DISALLOW();
  push_int(0);
}

static void gdbmmod_close(INT32 args)
{
  pop_n_elems(args);

  do_free();
  push_int(0);
}

static void init_gdbm_glue(struct object *o)
{
  THIS->dbf=0;
}

static void exit_gdbm_glue(struct object *o)
{
  do_free();
}

#endif

void pike_module_exit(void) {}

void pike_module_init(void)
{
#if defined(HAVE_GDBM_H) && defined(HAVE_LIBGDBM)
  start_new_program();
  ADD_STORAGE(struct gdbm_glue);
  
  add_function("create",gdbmmod_create,"function(void|string,void|string:void)",0);

  add_function("close",gdbmmod_close,"function(:void)",0);
  add_function("store",gdbmmod_store,"function(string,string:int)",0);
  add_function("`[]=",gdbmmod_store,"function(string,string:int)",0);
  add_function("fetch",gdbmmod_fetch,"function(string:string)",0);
  add_function("`[]",gdbmmod_fetch,"function(string:string)",0);
  add_function("delete",gdbmmod_delete,"function(string:int)",0);
  add_function("firstkey",gdbmmod_firstkey,"function(:string)",0);
  add_function("nextkey",gdbmmod_nextkey,"function(string:string)",0);
  add_function("reorganize",gdbmmod_reorganize,"function(:int)",0);
  add_function("sync",gdbmmod_sync,"function(:void)",0);

  set_init_callback(init_gdbm_glue);
  set_exit_callback(exit_gdbm_glue);
  end_class("gdbm",0);
#endif
}

