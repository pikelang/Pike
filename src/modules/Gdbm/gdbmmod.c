/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "gdbm_machine.h"
#include "threads.h"

/* Todo: make sure only one thread accesses the same gdbmmod */

#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "module.h"
#include "array.h"
#include "program.h"
#include "module_support.h"

#if defined(HAVE_GDBM_H) && defined(HAVE_LIBGDBM)

#include <gdbm.h>

#ifdef _REENTRANT
static MUTEX_T gdbm_lock STATIC_MUTEX_INIT;
#endif  

#define sp Pike_sp
static struct program *iterator;
struct gdbm_glue
{
  GDBM_FILE dbf;
  struct pike_string *iter;
};

#define THIS ((struct gdbm_glue *)(Pike_fp->current_storage))

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
  if(THIS->iter)
  {
    free_string(THIS->iter);
    THIS->iter=0;
  }
}

/* Compat with old gdbm. */
#ifndef GDBM_SYNC
#define GDBM_SYNC 0
#endif
#ifndef GDBM_NOLOCK
#define GDBM_NOLOCK 0
#endif

static int fixmods(char *mods)
{
  int mode = 0;
  int flags = GDBM_NOLOCK;
  while(1)
  {
    switch(*(mods++))
    {
    case 0:
      switch(mode) {
      default:
      case 0x0:
	Pike_error("No mode given for gdbm->open()\n"); 
      case 0x1:	/* r */
	return GDBM_READER;
      case 0x3:	/* rw */
	return GDBM_WRITER | flags;
      case 0x7:	/* rwc */
	return GDBM_WRCREAT | flags;
      case 0xf: /* rwct */
	return GDBM_NEWDB | flags;
      }

    case 'r': case 'R': mode = 0x1; break;
    case 'w': case 'W': mode = 0x3; break;
    case 'c': case 'C': mode = 0x7; break;
    case 't': case 'T': mode = 0xf; break;

      /*
       * Flags from this point on.
       */
    case 'f': case 'F': flags |= GDBM_FAST; break;

      /*
       * NOTE: The following are new in Pike 7.7.
       */

    case 's': case 'S': flags |= GDBM_SYNC; break;
      /* NOTE: This flag is inverted! */
    case 'l': case 'L': flags &= ~GDBM_NOLOCK; break;

    default:
      Pike_error("Bad mode flag '%c' in gdbm->open.\n", mods[-1]);
    }
  }
}

void gdbmmod_fatal(const char *err)
{
  Pike_error("GDBM: %s\n",err);
}

/*! @module Gdbm
 *!
 *! This module provides an interface to the GNU dbm database.
 *!
 *! The basic use of GDBM is to store key/data pairs in a data
 *! file. Each key must be unique and each key is paired with only one
 *! data item.
 *!
 *! The library provides primitives for storing key/data pairs,
 *! searching and retrieving the data by its key and deleting a key
 *! along with its data. It also support sequential iteration over all
 *! key/data pairs in a database.
 *!
 *! The @[DB] class also overloads enough operators to make it behave
 *! a lot like a @tt{mapping(string(8bit):string(8bit))@}, you can index it,
 *! assign indices and loop over it using foreach.
 *!
 */

/*! @class DB
 */

/*! @decl protected void create(void|string file, void|string(99..119) mode)
 *!
 *! Without arguments, this function does nothing. With one argument it
 *! opens the given file as a gdbm database, if this fails for some
 *! reason, an error will be generated. If a second argument is present,
 *! it specifies how to open the database using one or more of the follow
 *! flags in a string:
 *!
 *! @string
 *!   @value r
 *!     Open database for reading
 *!   @value w
 *!     Open database for writing
 *!   @value c
 *!     Create database if it does not exist
 *!   @value t
 *!     Overwrite existing database
 *!   @value f
 *!     Fast mode
 *!   @value s
 *!     Synchronous mode
 *!   @value l
 *!     Locking mode
 *! @endstring
 *!
 *! The fast mode prevents the database from syncronizing each change
 *! in the database immediately. This is dangerous because the database
 *! can be left in an unusable state if Pike is terminated abnormally.
 *!
 *! The default mode is @expr{"rwc"@}.
 *!
 *! @note
 *!  The gdbm manual states that it is important that the database is
 *!  closed properly. Unfortunately this will not be the case if Pike
 *!  calls exit() or returns from main(). You should therefore make sure
 *!  you call close or destruct your gdbm objects when exiting your
 *!  program.
 *! 
 *!  @[atexit] might be useful.
 *! 
 *!  This is very important if the database is used with the 'l' flag.
 */

static void gdbmmod_create(INT32 args)
{
  struct gdbm_glue *this=THIS;
  do_free();
  if(!args)
    Pike_error("Need at least one argument to Gdbm.DB, the filename\n");
  if(args)
  {
    GDBM_FILE tmp;
    struct pike_string *tmp2;
    int rwmode = GDBM_WRCREAT|GDBM_NOLOCK;

    if(TYPEOF(sp[-args]) != T_STRING)
      Pike_error("Bad argument 1 to gdbm->create()\n");

    if(args>1)
    {
      if(TYPEOF(sp[1-args]) != T_STRING)
	Pike_error("Bad argument 2 to gdbm->create()\n");

      rwmode=fixmods(sp[1-args].u.string->str);
    }

    if (this->dbf) {
      do_free();
    }

    tmp2=sp[-args].u.string;

    THREADS_ALLOW();
    mt_lock(& gdbm_lock);
    tmp=gdbm_open(tmp2->str, 512, rwmode, 00666, gdbmmod_fatal);
    mt_unlock(& gdbm_lock);
    THREADS_DISALLOW();

    if(!Pike_fp->current_object->prog)
    {
      if(tmp) gdbm_close(tmp);
      Pike_error("Object destructed in gdbm->open()n");
    }
    this->dbf=tmp;

    pop_n_elems(args);
    if(!this->dbf)
      Pike_error("Failed to open GDBM database: %d: %s.\n",
		 gdbm_errno, gdbm_strerror(gdbm_errno));
  }
}

#define STRING_TO_DATUM(dat, st) dat.dptr=st->str,dat.dsize=st->len;
#define DATUM_TO_STRING(dat) make_shared_binary_string(dat.dptr, dat.dsize)

/*! @decl string(8bit) fetch(string(8bit) key)
 *! @decl string(8bit) `[](string(8bit) key)
 *!
 *! Return the data associated with the key 'key' in the database.
 *! If there was no such key in the database, zero is returned.
 */
static void gdbmmod_fetch(INT32 args)
{
  struct gdbm_glue *this=THIS;
  datum key,ret;

  if(!args)
    Pike_error("Too few arguments to gdbm->fetch()\n");

  if(TYPEOF(sp[-args]) != T_STRING)
    Pike_error("Bad argument 1 to gdbm->fetch()\n");

  if(!THIS->dbf)
    Pike_error("GDBM database not open.\n");

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
    push_undefined();
  }
}

/*! @decl int(0..1) delete(string key)
 *!
 *! Remove a key from the database. Returns 1 if successful,
 *! otherwise 0, e.g. when the item is not present or the
 *! database is read only.
 */
static void gdbmmod_delete(INT32 args)
{
  struct gdbm_glue *this=THIS;
  datum key;
  int ret;
  if(!args)
    Pike_error("Too few arguments to gdbm->delete()\n");

  if(TYPEOF(sp[-args]) != T_STRING)
    Pike_error("Bad argument 1 to gdbm->delete()\n");

  if(!this->dbf)
    Pike_error("GDBM database not open.\n");

  STRING_TO_DATUM(key, sp[-args].u.string);

  THREADS_ALLOW();
  mt_lock(& gdbm_lock);
  ret=gdbm_delete(this->dbf, key);
  mt_unlock(& gdbm_lock);
  THREADS_DISALLOW();
  
  pop_n_elems(args);
  push_int( ret==0 );
}

/*! @decl string(8bit) _m_delete( string(8bit) key )
 *! 
 *! Provides overloading of the @[m_delete] function.
 *! 
 *! Will return the value the key had before it was removed, if any
 *!
 *! If the key exists but deletion fails (usually due to a read only
 *! database) this function will throw an error.
 */
static void gdbmmod_m_delete(INT32 args)
{
  struct gdbm_glue *this=THIS;
  datum key, ret;

  if(TYPEOF(sp[-args]) != T_STRING)
  {
    push_undefined();
    return;
  }

  if(!this->dbf)
    Pike_error("GDBM database not open.\n");

  STRING_TO_DATUM(key, sp[-args].u.string);

  THREADS_ALLOW();
  mt_lock(& gdbm_lock);
  ret=gdbm_fetch(this->dbf, key);
  if( ret.dptr )
    if( gdbm_delete(this->dbf, key) )
      Pike_error("Failed to delete key from database.\n");
  mt_unlock(& gdbm_lock);
  THREADS_DISALLOW();
  if(ret.dptr)
  {
    push_string(DATUM_TO_STRING(ret));
    free(ret.dptr);
  }else{
    push_undefined();
  }
}

/*! @decl string firstkey()
 *!
 *! Return the first key in the database, this can be any key in the
 *! database.
 *!
 *! Used together with @[nextkey] the databse can be iterated.
 *!
 *! @note
 *! The database also works as an @[Iterator], and can be used as the
 *! first argument to @[foreach].
 *!
 *! Adding or removing keys will change the iteration order,
 *! this can cause keys to be skipped while iterating.
 *!
 *! 
 *! @example
 *! @code
 *!  // Write the contents of the database
 *!  for(key=gdbm->firstkey(); k; k=gdbm->nextkey(k))
 *!    write(k+":"+gdbm->fetch(k)+"\n");
 *! @endcode
 *!
 *! Or, using foreach
 *! @code
 *!  // Write the contents of the database
 *! foreach( db; string key; string value )
 *!    write(key+":"+value+"\n");
 *! @endcode
 */

static void gdbmmod_firstkey(INT32 args)
{
  struct gdbm_glue *this=THIS;
  datum ret;
  pop_n_elems(args);

  if(!this->dbf) Pike_error("GDBM database not open.\n");

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

/*! @decl string(8bit) nextkey(string(8bit) key)
 *!
 *! This returns the key in database that follows the key 'key' key.
 *! This is of course used to iterate over all keys in the database.
 *!
 *! @note
 *! Changing (adding or removing keys) the database while iterating
 *! can cause keys to be skipped.
 *!
 *! The database also works as an @[Iterator], and can be used as the
 *! first argument to @[foreach].
 *! 
 *! @example
 *! @code
 *!  // Write the contents of the database
 *!  for(key=gdbm->firstkey(); k; k=gdbm->nextkey(k))
 *!    write(k+":"+gdbm->fetch(k)+"\n");
 *! @endcode
 *!
 *! Or, using foreach
 *! @code
 *!  // Write the contents of the database
 *! foreach( db; string key; string value )
 *!    write(key+":"+value+"\n");
 *! @endcode
 *!
 */

static void gdbmmod_nextkey(INT32 args)
{
  struct gdbm_glue *this=THIS;
  datum key,ret;
  if(!args)
    Pike_error("Too few arguments to gdbm->nextkey()\n");

  if(TYPEOF(sp[-args]) != T_STRING)
    Pike_error("Bad argument 1 to gdbm->nextkey()\n");

  if(!THIS->dbf)
    Pike_error("GDBM database not open.\n");

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

/*! @decl string `[]= (string(8bit) key, string(8bit) data)
 *!
 *! Associate the contents of 'data' with the key 'key'. If the key 'key'
 *! already exists in the database the data for that key will be replaced.
 *! If it does not exist it will be added. An error will be generated if
 *! the database was not open for writing.
 *!
 *! @example
 *!   gdbm[key] = data;
 *!
 *! @returns
 *!   Returns @[data] on success.
 *!
 *! @seealso
 *!   @[store()]
 */

/*! @decl int store(string key, string data)
 *!
 *! Associate the contents of 'data' with the key 'key'. If the key 'key'
 *! already exists in the database the data for that key will be replaced.
 *! If it does not exist it will be added. An error will be generated if
 *! the database was not open for writing.
 *!
 *! @example
 *!   gdbm->store(key, data);
 *!
 *! @returns
 *!   Returns @expr{1@} on success.
 *!
 *! @note
 *!   Note that the returned value differs from that of @[`[]=()].
 *!
 *! @seealso
 *!   @[`[]=()]
 */

static void gdbmmod_store(INT32 args)
{
  struct gdbm_glue *this=THIS;
  datum key,data;
  int method = GDBM_REPLACE;
  int ret;
  if(args<2)
    Pike_error("Too few arguments to gdbm->store()\n");

  if(TYPEOF(sp[-args]) != T_STRING)
    Pike_error("Bad argument 1 to gdbm->store()\n");

  if(TYPEOF(sp[1-args]) != T_STRING)
    Pike_error("Bad argument 2 to gdbm->store()\n");

  if (args > 2) {
    if (TYPEOF(sp[2-args]) != T_INT) {
      Pike_error("Bad argument 3 to gdbm->store()\n");
    }
    if (sp[2-args].u.integer) {
      method = GDBM_INSERT;
    }
  }

  if(!THIS->dbf)
    Pike_error("GDBM database not open.\n");

  STRING_TO_DATUM(key, sp[-args].u.string);
  STRING_TO_DATUM(data, sp[1-args].u.string);

  THREADS_ALLOW();
  mt_lock(& gdbm_lock);
  ret=gdbm_store(this->dbf, key, data, method);
  mt_unlock(& gdbm_lock);
  THREADS_DISALLOW();

  if(ret == -1) {
    Pike_error("GDBM database not open for writing.\n");
  } else if (ret == 1) {
    Pike_error("Duplicate key.\n");
  }
  ref_push_string(sp[1-args].u.string);
  stack_pop_n_elems_keep_top(args);
}

/* Compat */
static void gdbmmod_store_compat(INT32 args)
{
  gdbmmod_store(args);
  pop_stack();
  push_int(1);
}

/*! @decl int reorganize()
 *!
 *! Deletions and insertions into the database can cause fragmentation
 *! which will make the database bigger. This routine reorganizes the
 *! contents to get rid of fragmentation. Note however that this function
 *! can take a LOT of time to run if the database is big.
 */

static void gdbmmod_reorganize(INT32 args)
{
  struct gdbm_glue *this=THIS;
  int ret;
  pop_n_elems(args);

  if(!THIS->dbf) Pike_error("GDBM database not open.\n");
  THREADS_ALLOW();
  mt_lock(& gdbm_lock);
  ret=gdbm_reorganize(this->dbf);
  mt_unlock(& gdbm_lock);
  THREADS_DISALLOW();
  pop_n_elems(args);
  push_int(ret);
}

/*! @decl void sync()
 *!
 *! When opening the database with the 'f' flag writings to the database
 *! can be cached in memory for a long time. Calling sync will write
 *! all such caches to disk and not return until everything is stored
 *! on the disk.
 */

static void gdbmmod_sync(INT32 UNUSED(args))
{
  struct gdbm_glue *this=THIS;
  if(!THIS->dbf) Pike_error("GDBM database not open.\n");
  THREADS_ALLOW();
  mt_lock(& gdbm_lock);
  gdbm_sync(this->dbf);
  mt_unlock(& gdbm_lock);
  THREADS_DISALLOW();
  push_int(0);
}


static void gdbmmod_iter_first(INT32 UNUSED(args))
{
  struct gdbm_glue *this=THIS;
  gdbmmod_firstkey(0);
  if( Pike_sp[-1].u.string )
    this->iter = Pike_sp[-1].u.string;
  Pike_sp--;
  push_int( !!this->iter );
}

static void gdbmmod_iter_next(INT32 UNUSED(args))
{
  struct gdbm_glue *this=THIS;
  if(!this->iter) 
  {
      push_undefined();
      return;
  }
  push_string( this->iter );
  gdbmmod_nextkey(1);
  if( TYPEOF(Pike_sp[-1]) != PIKE_T_STRING )
  {
    this->iter = 0;
    push_undefined();
    return;
  }
  this->iter = Pike_sp[-1].u.string;
  push_int(1);
  return;

}

static void gdbmmod_iter_index(INT32 UNUSED(args))
{
  struct gdbm_glue *this=THIS;
  if( this->iter )
    ref_push_string( this->iter );
  else
    push_undefined();
}

static void gdbmmod_iter_no_value(INT32 UNUSED(args))
{
  push_int( !THIS->iter );
}

static void gdbmmod_iter_value(INT32 UNUSED(args))
{
  struct gdbm_glue *this=THIS;
  if( this->iter )
  {
    ref_push_string( this->iter );
    gdbmmod_fetch(1);    
  }
  else
    push_undefined();
}

/*! @decl array(string(8bit)) _indices()
 *!
 *! Provides overloading of @[indices].
 *! 
 *! @note
 *! Mainly useful when debugging, the returned list might not fit in
 *! memory for large databases.
 */
static void gdbmmod_indices(INT32 UNUSED(args))
{
  struct gdbm_glue *this=THIS;
  struct svalue *start = Pike_sp;
  gdbmmod_iter_first(0);
  pop_stack();
  while( this->iter )
  {
    ref_push_string( this->iter );
    gdbmmod_iter_next(0);
    pop_stack();
  }
  push_array(aggregate_array( Pike_sp-start ));
}

/*! @decl array(string(8bit)) _values()
 *!
 *! Provides overloading of @[values].
 *! 
 *! @note
 *! Mainly useful when debugging, the returned list might not fit in
 *! memory for large databases.
 */
static void gdbmmod_values(INT32 UNUSED(args))
{
  struct gdbm_glue *this=THIS;
  struct svalue *start = Pike_sp;
  gdbmmod_iter_first(0);
  pop_stack();

  while( this->iter )
  {
    ref_push_string( this->iter );
    gdbmmod_fetch(1);
    gdbmmod_iter_next(0);
    pop_stack();
  }
  push_array(aggregate_array( Pike_sp-start ));
}

static void gdbmmod_get_iterator(INT32 UNUSED(args))
{
  push_object( clone_object( iterator, 0 ) );
  *((struct gdbm_glue *)Pike_sp[-1].u.object->storage) = *THIS;
  apply(Pike_sp[-1].u.object, "first", 0);
  pop_stack();
}


/*! @decl void close()
 *!
 *! Closes the database. The object is no longer usable after this function has been called.
 *!
 *! This is also done automatically when the object is destructed for
 *! any reason (running out of references or explicit destruct, as an
 *! example)
 */

static void gdbmmod_close(INT32 args)
{
  pop_n_elems(args);

  do_free();
  push_int(0);
}

static void init_gdbm_glue(struct object *UNUSED(o))
{
  THIS->dbf=0;
  THIS->iter=0;
}

static void init_gdbm_iterator(struct object *UNUSED(o))
{
  THIS->dbf=0;
  THIS->iter=0;
}

static void exit_gdbm_glue(struct object *UNUSED(o))
{
  do_free();
}

static void exit_gdbm_iterator(struct object *UNUSED(o))
{
  if( THIS->iter )
    free_string( THIS->iter );
}

/*! @endclass
 */

/*! @class Iterator
 *! @inherit predef::Iterator
 *!
 *! Object keeping track of an iteration over a @[DB]
 *!
 *! @note
 *! Can not be usefully constructed manually, instead use the database
 *! as the first argument to @[foreach] or @[predef::get_iterator]
 *!
 */

 /*! @endclass
 */

/*! @endmodule
 */

#endif /* defined(HAVE_GDBM_H) && defined(HAVE_LIBGDBM) */

PIKE_MODULE_EXIT {
}

PIKE_MODULE_INIT
{
  struct program *db;
#if defined(HAVE_GDBM_H) && defined(HAVE_LIBGDBM)
  start_new_program();
  ADD_STORAGE(struct gdbm_glue);
  
  /* function(void|string,void|string:void) */
  ADD_FUNCTION("create", gdbmmod_create,
	       tFunc(tOr(tVoid,tStr) tOr(tVoid,tStr), tVoid), ID_PROTECTED);

  /* function(:void) */
  ADD_FUNCTION("close",gdbmmod_close,tFunc(tNone,tVoid),0);
  /* function(string, string, int(0..1)|void: int) */
  ADD_FUNCTION("store", gdbmmod_store_compat,
	       tFunc(tStr8 tStr8 tOr(tInt01, tVoid), tInt), 0);
  /* function(string, string, int(0..1)|void: string) */
  ADD_FUNCTION("`[]=", gdbmmod_store,
	       tFunc(tStr8 tSetvar(0, tStr8) tOr(tInt01, tVoid), tVar(0)), 0);
  /* function(string:string) */
  ADD_FUNCTION("fetch",gdbmmod_fetch,tFunc(tStr,tStr8),0);
  /* function(string:string) */
  ADD_FUNCTION("`[]",gdbmmod_fetch,tFunc(tStr,tStr8),0);
  /* function(string:int) */
  ADD_FUNCTION("delete",gdbmmod_delete,tFunc(tStr,tInt01),0);
  /* function(:string) */
  ADD_FUNCTION("firstkey",gdbmmod_firstkey,tFunc(tNone,tStr8),0);
  /* function(string:string) */
  ADD_FUNCTION("nextkey",gdbmmod_nextkey,tFunc(tStr,tStr8),0);
  /* function(:int) */
  ADD_FUNCTION("reorganize",gdbmmod_reorganize,tFunc(tNone,tInt),0);
  /* function(:void) */
  ADD_FUNCTION("sync",gdbmmod_sync,tFunc(tNone,tVoid),0);

  /* iterator API.*/
  ADD_FUNCTION("_get_iterator", gdbmmod_get_iterator,tFunc(tNone,tObj),0);

  /* some mapping operator overloading */
  ADD_FUNCTION("_m_delete",gdbmmod_m_delete,tFunc(tStr,tStr8),0);
  ADD_FUNCTION("_values",gdbmmod_values,tFunc(tNone,tArr(tStr8)),0);
  ADD_FUNCTION("_indices",gdbmmod_indices,tFunc(tNone,tArr(tStr8)),0);

  set_init_callback(init_gdbm_glue);
  set_exit_callback(exit_gdbm_glue);
  db = end_program();
  add_program_constant( "DB", db, 0 );
  add_program_constant( "gdbm", db, 0 ); /* compat (...-7.8). */
  free_program(db);

  start_new_program();
  ADD_STORAGE(struct gdbm_glue);
  ADD_FUNCTION("first", gdbmmod_iter_first,tFunc(tNone,tInt01),0);
  ADD_FUNCTION("next",  gdbmmod_iter_next, tFunc(tNone,tInt01),0);
  ADD_FUNCTION("index", gdbmmod_iter_index,tFunc(tNone,tStr8),0);
  ADD_FUNCTION("value", gdbmmod_iter_value,tFunc(tNone,tStr8),0);
  ADD_FUNCTION("`!",    gdbmmod_iter_no_value,tFunc(tNone,tInt01),0);
  set_init_callback(init_gdbm_iterator);
  set_exit_callback(exit_gdbm_iterator);
  iterator = end_program();
  add_program_constant( "Iterator", iterator, 0 );
  free_program(iterator);
#else
  HIDE_MODULE();
#endif
}
