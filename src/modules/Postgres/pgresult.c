/*
 * Postgres95 support for pike/0.5 and up
 *
 * (C) 1997 Francesco Chemolli <kinkie@comedia.it>
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

#include "pgres_config.h"
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
#include <libpq-fe.h>

/* Pike includes */
#include <global.h>
#include <stralloc.h>
#include <object.h>
#include <threads.h>
#include <array.h>
#include <mapping.h>
#include <builtin_functions.h>
#include <module_support.h>

#ifdef _REENTRANT
MUTEX_T pike_postgres_result_mutex;
#define PQ_LOCK() mt_lock(&pike_postgres_mutex)
#define PQ_UNLOCK() mt_unlock(&pike_postgres_mutex)
#else
#define PQ_LOCK() /**/
#define PQ_UNLOCK() /**/
#endif

#include "pg_types.h"

#define THIS ((struct postgres_result_object_data *) fp->current_storage)

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
	PQclear(THIS->result);
}

static void f_create (INT32 args)
{
	char *storage;
	check_all_args("postgres_result->create",args,BIT_OBJECT,0);
	pgdebug("result->f_create(%d).\n",args);

	storage=get_storage(sp[-args].u.object,postgres_program);
	if (!storage)
		error ("I need a Postgres object or an heir of it.\n");
	THIS->result=((struct pgres_object_data *)storage)->last_result;
	((struct pgres_object_data *) sp[-args].u.object->storage)->last_result=NULL;
	/* no fear of memory leaks, we've only moved the pointer from there to here */

	pop_n_elems(args);
	if (!THIS->result) /*this ensures we _DO_ have a result*/
		error ("Bad result.\n");
#ifdef PGRESDEBUG
	pgdebug("Got %d tuples.\n",PQntuples(THIS->result));
#endif
}

static void f_num_rows (INT32 args)
{
	check_all_args("postgres_result->num_rows",args,0);
	if (PQresultStatus(THIS->result)!=PGRES_TUPLES_OK) {
		push_int(0);
		return;
	}
	push_int(PQntuples(THIS->result));
	return;
}

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

static void f_seek (INT32 args)
{
	int howmuch;
	check_all_args("postgres_result->seek",args,BIT_INT,0);
	howmuch=sp[-args].u.integer;
	if (THIS->cursor+howmuch < 0)
		error ("Cannot seek to negative result indexes!\n");
	if (THIS->cursor+howmuch > PQntuples(THIS->result))
		error ("Cannot seek past result's end!.\n");
	pop_n_elems(args);
	THIS->cursor += howmuch;
	return;
}

static void f_fetch_row (INT32 args)
{
	int j,k,numfields;
	char * value;

	check_all_args("postgres_result->fetch_row",args,0);
	pgdebug("f_fectch_row(); cursor=%d.\n",THIS->cursor);
	if (THIS->cursor>=PQntuples(THIS->result)) {
		push_int(0);
		return;
	}
	numfields=PQnfields(THIS->result);
	for (j=0;j<numfields;j++) {
		value=PQgetvalue(THIS->result,THIS->cursor,j);
#ifdef CUT_TRAILING_SPACES
		/* damn! We need to cut the trailing whitespace... */
		for (k=PQgetlength(THIS->result,THIS->cursor,j)-1;k>=0;k--)
			if (value[k]!=' ')
				break;
		push_string(make_shared_binary_string(value,k+1));
#else
		push_text(value);
#endif
	}
	f_aggregate(numfields);
	THIS->cursor++;
	return;
}

struct program * pgresult_program;

void pgresult_init (void)
{
	start_new_program();
	add_storage(sizeof(struct postgres_result_object_data));
	set_init_callback(result_create);
	set_exit_callback(result_destroy);

	add_function("create",f_create,"function(object:void)",OPT_SIDE_EFFECT);
	add_function("num_rows",f_num_rows,"function(void:int)",
			OPT_EXTERNAL_DEPEND|OPT_RETURN);
	add_function("num_fields",f_num_fields,"function(void:int)",
			OPT_EXTERNAL_DEPEND|OPT_RETURN);
	add_function("fetch_fields",f_fetch_fields,
			"function(void:void|array(mapping(string:mixed)))",
			OPT_EXTERNAL_DEPEND|OPT_RETURN);
	add_function("seek",f_seek,"function(int:void)",OPT_SIDE_EFFECT);
	add_function("fetch_row",f_fetch_row,"function(void:void|array(mixed))",
			OPT_EXTERNAL_DEPEND|OPT_RETURN);
	pgresult_program=end_program();
	add_program_constant("postgres_result",pgresult_program,0);
	pgresult_program->refs++;
}

#endif
