/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pgresult.c,v 1.30 2004/10/07 22:49:58 nilsson Exp $
*/

/*
 * Postgres95 support for pike/0.5 and up
 *
 * This code is provided AS IS, and may be copied and distributed freely,
 * under the terms of the GNU General Public License, version 2.
 *
 * You might notice that this code resembles Henrik Grubbestrom's mysql
 * module. It's just the most efficient way of doing things. This doesn't
 * imply I didn't peek at his code ^_^'
 */

/*
 * Although Postgres allows great flexibility in returning values from the
 * backend connection, in order to keep this code interface-compliant
 * I'll have to do some serious emulation stuff, somehow making pike
 * code less easy to write.
 * I'm sticking to the object result interface because it allows
 * for bigger query results.
 * For now I'm handling only text. (external) type handling for postgres
 *   is _REALLY_ messy. Not to talk the big_objects handling, and COPY and
 *   the status... yuck.
 *   The potential to powerfully store (also) binary data is there, but
 *   it's dampened by a really messy interface implementation.
 */

/*
 * Notes for Henrik:
 * - I suggest allowing negative seek() arguments for the generic interface,
 *   moving the check inside the actual sql classes.
 */

#define NO_PIKE_SHORTHAND

#include "global.h"
#include "pgres_config.h"
/* Some versions of Postgres define this, and it conflicts with pike_error.h */
#undef JMP_BUF

#ifdef HAVE_POSTGRES

/* #define PGRESDEBUG */

/* <sigh> Postgres stores strings internally padding with whitespaces
 * to their field length. If CUT_TRAILING_SPACES is defined, all
 * trailing spaces will be cut, regardless they were meant or not.
 * This is meant for 'compatibility' versus other database servers, like
 * Msql, where a declaration of char(20) means 'any string long at most
 * 20 characters', where in Postgres it means 'any string long exactly 20
 * characters'. With Postgres you have to use type varchar otherwise, but
 * this makes its SQL incompatible with other servers'.
 */

#define CUT_TRAILING_SPACES

#include <stdio.h>

/* Pike includes */
#include "stralloc.h"
#include "object.h"
#include "threads.h"
#include "array.h"
#include "mapping.h"
#include "builtin_functions.h"
#include "module_support.h"

/* <server/postgres_fe.h> doesn't suffice to be able to include
 * <server/catalog/pg_type.h>.
 */
#ifdef HAVE_SERVER_POSTGRES_H
#include <server/postgres.h>
#elif defined(HAVE_POSTGRES_H)
#include <postgres.h>
#endif
#ifdef HAVE_SERVER_CATALOG_PG_TYPE_H
#include <server/catalog/pg_type.h>
#elif defined(HAVE_CATALOG_PG_TYPE_H)
#include <catalog/pg_type.h>
#endif

#ifdef _REENTRANT
# ifdef PQ_THREADSAFE
#  define PQ_FETCH() PIKE_MUTEX_T *pg_mutex = &THIS->pgod->mutex;
#  define PQ_LOCK() mt_lock(pg_mutex)
#  define PQ_UNLOCK() mt_unlock(pg_mutex)
# else
PIKE_MUTEX_T pike_postgres_mutex STATIC_MUTEX_INIT;
#  define PQ_FETCH()
#define PQ_LOCK() mt_lock(&pike_postgres_mutex)
#define PQ_UNLOCK() mt_unlock(&pike_postgres_mutex)
# endif
#else
# define PQ_FETCH()
# define PQ_LOCK()
# define PQ_UNLOCK()
#endif

#include "pg_types.h"



#define THIS ((struct postgres_result_object_data *) Pike_fp->current_storage)

#ifdef PGRESDEBUG
#define pgdebug printf
#else
static void pgdebug (char * a, ...) {}
#endif

void result_create (struct object * o) {
	pgdebug("result_create().\n");
	THIS->result=NULL;
	THIS->cursor=0;
}

void result_destroy (struct object * o) {
	pgdebug("result_destroy().\n");
	if(THIS->pgod->docommit) {
	  PGconn * conn = THIS->pgod->dblink;
	  PGresult * res=THIS->result;
	  PQ_FETCH();
	  PQclear(res);
	  THIS->pgod->docommit=0;
	  THREADS_ALLOW();
	  PQ_LOCK();
	  res=PQexec(conn,"COMMIT");
	  PQ_UNLOCK();
	  THREADS_DISALLOW();
	  THIS->result=res;
	  THIS->pgod->lastcommit=1;
	}
	PQclear(THIS->result);
}

/*! @module Postgres
 *!
 *! @class postgres_result
 *!
 *! Contains the result of a Postgres-query.
 *!
 *! @seealso
 *!  Sql.postgres, Postgres.postgres, Sql.Sql, Sql.sql_result
 */

/*! @decl void create(object o)
 *!
 *! You can't create istances of this object yourself.
 *! The only way to create it is via a big_query to a Postgres
 *! database.
 */

static void f_create (INT32 args)
{
	char *storage;
	check_all_args("postgres_result->create",args,BIT_OBJECT,0);
	pgdebug("result->f_create(%d).\n",args);

	storage=get_storage(Pike_sp[-args].u.object,postgres_program);
	if (!storage)
		Pike_error ("I need a Postgres object or an heir of it.\n");
	THIS->result=
	  (THIS->pgod=(struct pgres_object_data *)storage)->last_result;
	((struct pgres_object_data *) Pike_sp[-args].u.object->storage)->last_result=NULL;
	/* no fear of memory leaks, we've only moved the pointer from there to here */

	pop_n_elems(args);
	if (!THIS->result) /*this ensures we _DO_ have a result*/
		Pike_error ("Bad result.\n");
#ifdef PGRESDEBUG
	pgdebug("Got %d tuples.\n",PQntuples(THIS->result));
#endif
}


/*! @decl int num_rows()
 *!
 *! Returns the number of rows in the result.
 */

static void f_num_rows (INT32 args)
{
        int rows;
	check_all_args("postgres_result->num_rows",args,0);
	if (PQresultStatus(THIS->result)!=PGRES_TUPLES_OK) {
		push_int(0);
		return;
	}
	rows=PQntuples(THIS->result);
	push_int(THIS->pgod->dofetch-1>rows?THIS->pgod->dofetch-1:rows);
	return;
}


/*! @decl int num_fields()
 *!
 *! Returns the number of fields in the result.
 */

static void f_num_fields (INT32 args)
{
	check_all_args("postgres_result->num_fields",args,0);
	if (PQresultStatus(THIS->result)!=PGRES_TUPLES_OK) {
		push_int(0);
		return;
	}
	push_int(PQnfields(THIS->result));
	return;
}


/*! @decl array(mapping(string:mixed)) fetch_fields()
 *!
 *! Returns an array with an entry for each field, each entry is
 *! a mapping with the following fields:
 *!
 *! @mapping
 *!   @member string "name"
 *!     Name of the column
 *!   @member int "type"
 *!     The type ID of the field. This is the database's internal
 *!     representation type ID.
 *!   @member int|string "length"
 *!     Can be an integer (the size of the contents in
 *!     bytes) or the word "variable".
 *! @endmapping
 *!
 *! @note
 *! For char() fields, length is to be intended as the MAXIMUM length
 *! of the field. This is not part of the interface specifications in fact,
 *! but a driver-choice. In fact char() fields are for Postgres _FIXED_
 *! length fields, and are space-padded. If CUT_TRAILING_SPACES is defined
 *! when the driver is compiled (default behavior) it will cut such spaces.
 */

static void f_fetch_fields (INT32 args)
{
	int j, numfields, tmp;
	PGresult * res=THIS->result;

	check_all_args("postgres_result->fetch_fields",args,0);
	numfields=PQnfields(res);
	for (j=0;j<numfields;j++)
	{
		push_text("name");
		push_text(PQfname(res,j));
		/* no table information is availible */
		/* no default value information is availible */
		push_text("type");
		push_int(PQftype(res,j));
		/* ARGH! I'd kill 'em! How am I supposed to know how types are coded
		 * internally!?!?!?!?
		 */
		push_text("length");
		tmp=PQfsize(res,j);
		if (tmp>=0)
			push_int(tmp);
		else
			push_text("variable");
		f_aggregate_mapping(6);
	}
	f_aggregate(numfields);
	return;
}


/*! @decl void seek()
 *!
 *! Moves the result cursor (ahead or backwards) the specified number of
 *! rows. Notice that when you fetch a row, the cursor is automatically
 *! moved forward one slot.
 */

static void f_seek (INT32 args)
{
	int howmuch;
	check_all_args("postgres_result->seek",args,BIT_INT,0);
	howmuch=Pike_sp[-args].u.integer;
	if (THIS->cursor+howmuch < 0)
		Pike_error ("Cannot seek to negative result indexes!\n");
	if (THIS->cursor+howmuch > PQntuples(THIS->result))
		Pike_error ("Cannot seek past result's end!.\n");
	pop_n_elems(args);
	THIS->cursor += howmuch;
	return;
}


/*! @decl array(string) fetch_row()
 *!
 *! Returns an array with the contents of the next row in the result.
 *! Advances the row cursor to the next row. Returns 0 at end of table.
 *!
 *! @bugs
 *! Since there's no generic way to know whether a type is numeric
 *! or not in Postgres, all results are returned as strings.
 *! You can typecast them in Pike to get the numeric value.
 *!
 *! @seealso
 *!   @[seek()]
 */
static void f_fetch_row (INT32 args)
{
	int j,k,numfields;
	char * value;

	check_all_args("postgres_result->fetch_row",args,0);
	pgdebug("f_fetch_row(); cursor=%d.\n",THIS->cursor);
	if (THIS->cursor>=PQntuples(THIS->result)) {
	  PGresult * res=THIS->result;
	  if(THIS->pgod->dofetch) {
	    PGconn * conn = THIS->pgod->dblink;
	    int docommit=THIS->pgod->docommit;
	    int dofetch=1;
	    PQ_FETCH();
	    PQclear(res);
	    THREADS_ALLOW();
	    PQ_LOCK();
	    res=PQexec(conn,FETCHCMD);
	    if(!res || PQresultStatus(res)!=PGRES_TUPLES_OK
	       || !PQntuples(res)) {
	      if(!docommit) {
		PQclear(res);
		res=PQexec(conn,"CLOSE "CURSORNAME);
	      }
	      dofetch=0;
	    }
	    PQ_UNLOCK();
	    THREADS_DISALLOW();
	    THIS->result = res;
	    if(!dofetch) {
	      THIS->pgod->dofetch=0;
	      goto badresult;
	    }
	    THIS->cursor=0;
	  } else {
badresult:
	    push_int(0);
	    return;
	  }
	}
	numfields=PQnfields(THIS->result);
	for (j=0;j<numfields;j++) {
	        void*binbuf = 0;
	        size_t binlen;
		value=PQgetvalue(THIS->result,THIS->cursor,j);
		k=PQgetlength(THIS->result,THIS->cursor,j);
		switch(PQftype(THIS->result,j)) {
		  /* FIXME: This code is questionable, and BPCHAROID
		   *        and BYTEAOID are usually not available
		   *        to Postgres frontends.
		   */
#if defined(CUT_TRAILING_SPACES) && defined(BPCHAROID)
		case BPCHAROID:
		  for(;k>0 && value[k]==' ';k--);
		  break;
#endif
#if defined(HAVE_PQUNESCAPEBYTEA) && defined(BYTEAOID)
		case BYTEAOID:
		  if(binbuf=PQunescapeBytea(value,&binlen))
		    value=binbuf,k=binlen;
		  break;
#endif
		}
		push_string(make_shared_binary_string(value,k));
		if(binbuf)
		  free(binbuf);
	}
	f_aggregate(numfields);
	THIS->cursor++;
	if(THIS->pgod->dofetch)
	  THIS->pgod->dofetch++;
	return;
}

/*! @endclass
 *!
 *! @endmodule
 */

struct program * pgresult_program;

void pgresult_init (void)
{
	start_new_program();
	ADD_STORAGE(struct postgres_result_object_data);
	set_init_callback(result_create);
	set_exit_callback(result_destroy);

	/* function(object:void) */
  ADD_FUNCTION("create",f_create,tFunc(tObj,tVoid),OPT_SIDE_EFFECT);
	/* function(void:int) */
  ADD_FUNCTION("num_rows",f_num_rows,tFunc(tVoid,tInt),
			OPT_EXTERNAL_DEPEND|OPT_RETURN);
	/* function(void:int) */
  ADD_FUNCTION("num_fields",f_num_fields,tFunc(tVoid,tInt),
			OPT_EXTERNAL_DEPEND|OPT_RETURN);
	/* function(void:void|array(mapping(string:mixed))) */
  ADD_FUNCTION("fetch_fields",f_fetch_fields,tFunc(tVoid,tOr(tVoid,tArr(tMap(tStr,tMix)))),
			OPT_EXTERNAL_DEPEND|OPT_RETURN);
	/* function(int:void) */
  ADD_FUNCTION("seek",f_seek,tFunc(tInt,tVoid),OPT_SIDE_EFFECT);
	/* function(void:void|array(mixed)) */
  ADD_FUNCTION("fetch_row",f_fetch_row,tFunc(tVoid,tOr(tVoid,tArr(tMix))),
			OPT_EXTERNAL_DEPEND|OPT_RETURN);
  pgresult_program=end_program();
  add_program_constant("postgres_result",pgresult_program,0);
}

#else

static int place_holder;	/* Keep the compiler happy */

#endif
