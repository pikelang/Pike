/*
 * Postgres95 and PostgreSQL support for pike/0.5 and up
 *
 * (C) 1997 Francesco Chemolli <kinkie@kame.usr.dsi.unimi.it>
 *
 * This code is provided AS IS, and may be distributed under the terms
 * of the GNU General Public License, version 2.
 */

#define NO_PIKE_SHORTHAND

#include "global.h"
#include "pgres_config.h"
#ifdef HAVE_POSTGRES

#include "version.h"

/* #define PGDEBUG */

/* System includes */
#include <stdlib.h>
#include <stdio.h> /* Needed or libpq-fe.h pukes :( */
#include <string.h>

/* Pike includes */
#include "las.h"
#include "machine.h"
#include "pike_memory.h"
#include "svalue.h"
#include "threads.h"
#include "stralloc.h"
#include "object.h"
#include "module_support.h"
#include "operators.h"

#include <postgres.h>
#include <libpq-fe.h>

#include "pgresult.h"

/* Actual code */
#ifdef _REENTRANT
PIKE_MUTEX_T pike_postgres_mutex STATIC_MUTEX_INIT;
#define PQ_LOCK() mt_lock(&pike_postgres_mutex);
#define PQ_UNLOCK() mt_unlock(&pike_postgres_mutex);
#else
#define PQ_LOCK()
#define PQ_UNLOCK()
#endif

#include "pg_types.h"

/* must be included last */
#include "module_magic.h"

#ifdef PGDEBUG
#define pgdebug printf
#else
static void pgdebug (char * a, ...) {}
#endif

struct program * postgres_program;

RCSID("$Id: postgres.c,v 1.20 2000/12/01 08:10:21 hubbe Exp $");

#define THIS ((struct pgres_object_data *) Pike_fp->current_storage)

static void set_error (char * newerror)
{
	pgdebug("set_error(%s).\n",newerror);
	if (THIS->last_error)
		free_string(THIS->last_error);
	THIS->last_error=make_shared_string(newerror);
	return;
}

static void pgres_create (struct object * o) {
	pgdebug ("pgres_create().\n");
	THIS->dblink=NULL;
	THIS->last_error=NULL;
	THIS->notify_callback=(struct svalue*)xalloc(sizeof(struct svalue));
	THIS->notify_callback->type=PIKE_T_INT;
}

static void pgres_destroy (struct object * o)
{
	PGconn * conn;
	pgdebug ("pgres_destroy().\n");
	if ((conn=THIS->dblink)) {
		THREADS_ALLOW();
		PQ_LOCK();
		PQfinish(conn);
		PQ_UNLOCK();
		THREADS_DISALLOW();
		THIS->dblink=NULL;
	}
	if(THIS->last_error) {
		if (THIS->last_error)
			free_string(THIS->last_error);
		THIS->last_error=NULL;
	}
	if (THIS->notify_callback->type!=PIKE_T_INT) {
		free_svalue(THIS->notify_callback);
		free(THIS->notify_callback);
	}
}

/* create (host,database,username,password,port) */
static void f_create (INT32 args)
{
        /*port will be used as a string with a sprintf()*/
	char * host=NULL, *db=NULL, *user=NULL, *pass=NULL, *port=NULL;
	PGconn * conn;

	check_all_args("postgres->create",args,
			BIT_STRING|BIT_VOID,
		        BIT_STRING|BIT_VOID,
			BIT_STRING|BIT_VOID,
		        BIT_STRING|BIT_VOID,
			BIT_INT|BIT_VOID,
		        0);

	if (THIS->dblink) {
		conn=THIS->dblink;
		THREADS_ALLOW();
		PQ_LOCK();
		PQfinish(conn);
		PQ_UNLOCK();
		THREADS_DISALLOW();
	}

	/*no break;'s here, it's intentional*/
	switch(args) {
		default:
		case 5:
			if (Pike_sp[2-args].type==PIKE_T_INT &&  /*this check is maybe redundant*/
					Pike_sp[2-args].u.integer <=65535 && 
			    		Pike_sp[2-args].u.integer >= 0) {
				if (Pike_sp[2-args].u.integer>0) {
					port=xalloc(10*sizeof(char)); /*we only need 6, we just checked.*/
					sprintf(port,"%d",Pike_sp[2-args].u.integer);
				}
			}
		case 4:
			if (Pike_sp[3-args].type==PIKE_T_STRING && Pike_sp[3-args].u.string->len)
				pass=Pike_sp[3-args].u.string->str;
		case 3:
			if (Pike_sp[2-args].type==PIKE_T_STRING && Pike_sp[2-args].u.string->len)
				user=Pike_sp[2-args].u.string->str;
		case 2:
			if (Pike_sp[1-args].type==PIKE_T_STRING && Pike_sp[1-args].u.string->len)
				db=Pike_sp[1-args].u.string->str;
		case 1:
			if(Pike_sp[-args].type==PIKE_T_STRING && Pike_sp[-args].u.string->len)
				host=Pike_sp[-args].u.string->str;
		case 0:
			;
	}
#if 0
	/* Old arguments-checking code */
	if (args>=1)
		if(Pike_sp[-args].type==PIKE_T_STRING && Pike_sp[-args].u.string->len)
			host=Pike_sp[-args].u.string->str;
		/* postgres docs say they use hardwired defaults if no variable is found*/
	if (args>=2)
		if (Pike_sp[1-args].type==PIKE_T_STRING && Pike_sp[1-args].u.string->len)
			db=Pike_sp[1-args].u.string->str;
		/* This is not beautiful code, but it works:
		 * it specifies the port to connect to if there is a third
		 * argument greater than 0. It accepts integer arguments >= 0
		 * to allow simpler code in pike wrappers part.
		 */
	if (args==3)
		if (Pike_sp[2-args].type==PIKE_T_INT && Pike_sp[2-args].u.integer <=65535 && Pike_sp[2-args].u.integer >= 0) {
			if (Pike_sp[2-args].u.integer>0) {
				port=xalloc(10*sizeof(char)); /*it's enough, we need only 6*/
				sprintf(port,"%d",Pike_sp[2-args].u.integer);
			}
		}
		else
			Pike_error ("You must specify a TCP/IP port number as argument 5 "
					"to Sql.postgres->create().\n");
#endif
	pgdebug("f_create(host=%s, port=%s, db=%s, user=%s, pass=%s).\n",
			host,port,db,user,pass);
	THREADS_ALLOW();
	PQ_LOCK();
#ifdef HAVE_PQSETDBLOGIN
	conn=PQsetdbLogin(host,port,NULL,NULL,db,user,pass);
#else
	conn=PQsetdb(host,port,NULL,NULL,db);
#endif
	PQ_UNLOCK();
	THREADS_DISALLOW();
	if (!conn)
		Pike_error ("Could not conneect to server\n");
	if (PQstatus(conn)!=CONNECTION_OK) {
		set_error(PQerrorMessage(conn));
		THREADS_ALLOW();
		PQ_LOCK();
		PQfinish(conn);
		PQ_UNLOCK();
		THREADS_DISALLOW();
		Pike_error("Could not connect to database. Reason: \"%s\".\n",THIS->last_error->str);
	}
	THIS->dblink=conn;
	if (!THIS->dblink)
		Pike_error ("Huh? Weirdness here! Internal Pike_error!\n");
	pop_n_elems(args);
}

static void f_select_db (INT32 args)
{
	char *host, *port, *options, *tty, *db;
	PGconn * conn, *newconn;

	check_all_args("Postgres->select_db",args,BIT_STRING,0);
	
	if (!THIS->dblink)
		Pike_error ("Driver Pike_error. How can you possibly not be linked to a "
				"database already?\n");
	conn=THIS->dblink;
	THREADS_ALLOW();
	PQ_LOCK();
	host=PQhost(conn);
	port=PQport(conn);
	options=PQoptions(conn);
	tty=PQtty(conn);
	db=PQdb(conn);
	PQ_UNLOCK();
	THREADS_DISALLOW();
#if 0
	/* This is an optimization, but people may want to reset a connection
	 * re-selecting its database.
	 */
	if (!strcmp(Pike_sp[-args].u.string->str,db)) {
		pop_n_elems(args);
		return;
	}
#endif
	db=Pike_sp[-args].u.string->str;
	/* This could be really done calling f_create, but it's more efficient this
	 * way */
	THREADS_ALLOW();
	PQ_LOCK();
	/* using newconn is necessary or otherwise the datastructures I use
	 * as arguments get freed by PQfinish. Could be a problem under extreme
	 * situations (i.e. if the temporary use of _one_ more filedescriptor
	 * is not possible.
	 */
	newconn=PQsetdb(host,port,options,tty,db);
	PQfinish(conn);
	conn=newconn;
	PQ_UNLOCK();
	THREADS_DISALLOW();
	if (PQstatus(conn)==CONNECTION_BAD) {
		set_error(PQerrorMessage(conn));
		PQfinish(conn);
		Pike_error("Could not connect to database.\n");
	}
	THIS->dblink=conn;
	pop_n_elems(args);
}

static void f_big_query(INT32 args)
{
	PGconn *conn = THIS->dblink;
	PGresult * res;
	PGnotify * notification;
	char *query;

	check_all_args("Postgres->big_query",args,BIT_STRING,0);

	if (!conn)
		Pike_error ("Not connected.\n");

	if (Pike_sp[-args].u.string->len)
		query=Pike_sp[-args].u.string->str;
	else
		query=" ";

	THREADS_ALLOW();
	PQ_LOCK();
	pgdebug("f_big_query(\"%s\")\n",query);
	res=PQexec(conn,query);
	/* A dirty hack to fix the reconnect bug.
	 * we don't need to store the host/user/pass/db... etc..
	 * PQreset() does all the job.
	 *  Zsolt Varga <redax@agria.hu> 2000-apr-04
	 */
	if((PQstatus(conn) != CONNECTION_OK) ||
	   (PQresultStatus(res) == PGRES_FATAL_ERROR) ||
	   (PQresultStatus(res) == PGRES_BAD_RESPONSE)) {
	  PQclear(res);
	  PQreset(conn);
	  res=PQexec(conn,query);
	}

	notification=PQnotifies(conn);
	PQ_UNLOCK();
	THREADS_DISALLOW();

	pop_n_elems(args);
	if (notification!=NULL) {
		pgdebug("Incoming notification: \"%s\"\n",notification->relname);
		push_text(notification->relname);
		apply_svalue(THIS->notify_callback,1); 
		/* apply_svalue simply returns if the first argument is a PIKE_T_INT */
		free (notification);
	}
	if (!res) {
		set_error(PQerrorMessage(conn));
		if (!strncmp(THIS->last_error->str,"WARN",4)) {
			/* Sigh... woldn't a NONFATAL_ERROR be MUCH better? */
			push_int(1);
			return;
		}
		Pike_error ("Error in query.\n");
	}

	switch (PQresultStatus(res))
	{
		case PGRES_EMPTY_QUERY:
		case PGRES_COMMAND_OK:
			pgdebug("\tOk.\n");
			THIS->last_result=NULL;
			PQclear(res);
			push_int(0);
			return;
		case PGRES_NONFATAL_ERROR:
			pgdebug ("Warning.\n");
			set_error(PQerrorMessage(conn));
			push_int(1);
			return;
		case PGRES_BAD_RESPONSE:
		case PGRES_FATAL_ERROR:
			pgdebug("\tBad.\n");
			set_error(PQerrorMessage(conn));
			PQclear(res);
			Pike_error ("Error in frontend-backend communications.\n");
		case PGRES_TUPLES_OK:
			pgdebug("\tResult.\n");
			THIS->last_result=res;
			push_object(this_object());
			push_object(clone_object(pgresult_program,1));
			return;
		default:
			Pike_error ("Unimplemented server feature.\n");
	}
	Pike_error ("Internal Pike_error in postgresmodule.\n");
}

static void f_error (INT32 args)
{
	check_all_args("Postgres->Pike_error",args,0);

	if (THIS->last_error)
		ref_push_string(THIS->last_error);
	else
		push_int(0);
	return;
}

static void f_reset (INT32 args)
{
	PGconn * conn;

	check_all_args("Postgres->reset",args,0);

	if (!THIS->dblink)
		Pike_error ("Not connected.\n");
	conn=THIS->dblink;
	THREADS_ALLOW();
	PQ_LOCK();
	PQreset(conn);
	PQ_UNLOCK();
	THREADS_DISALLOW();
	if (PQstatus(conn)==CONNECTION_BAD) {
		set_error(PQerrorMessage(conn));
		Pike_error("Bad connection.\n");
	}
}

#if 0
/* This was cut (for now) because of the difficulty of obtaining
 * a valid FILE * from a fileobject.
 */
static void f_trace (INT32 args)
{
	if (args!=1)
		Pike_error ("Wrong args for trace().\n");
	if (Pike_sp[-args].type==PIKE_T_INT)
		if (Pike_sp[-args].u.integer==0)
			PQuntrace(THIS->dblink);
		else
			Pike_error ("Wrong argument for postgres->trace().\n");
	else
/* I really don't know how to check that the argument is an instance of
 * /precompiled/file... I guess there's the name stored somewhere..
 * For now let's presume that if it's an object, then it's a /precompiled/file*/
		PQtrace(THIS->dblink,
				((struct file_struct*)Pike_sp[-args].u.object->storage)->fd);
}
#endif

static void f_callback(INT32 args)
{
	check_all_args("postgres->_set_notify_callback()",BIT_INT|BIT_FUNCTION,0);

	if (Pike_sp[-args].type==PIKE_T_INT) {
		if (THIS->notify_callback->type!=PIKE_T_INT) {
			free_svalue(THIS->notify_callback);
			THIS->notify_callback->type=PIKE_T_INT;
		}
		pop_n_elems(args);
		return;
	}
	/*let's assume it's a function otherwise*/
	assign_svalue(THIS->notify_callback,Pike_sp-args);
	pop_n_elems(args);
}

static void f_host_info (INT32 args)
{
	check_all_args("Postgres->host_info",args,0);

	if (PQstatus(THIS->dblink)!=CONNECTION_BAD) {
		push_text("TCP/IP connection to ");
		pgdebug("adding reason\n");
		if(PQhost(THIS->dblink))
			push_text(PQhost(THIS->dblink));
		else
			push_text("<unknown>");
		pgdebug("done\n");
		f_add(2);
		return;
	}
	set_error(PQerrorMessage(THIS->dblink));
	Pike_error ("Bad connection.\n");
}

void pike_module_init (void)
{
	start_new_program();
	ADD_STORAGE(struct pgres_object_data);
	set_init_callback(pgres_create);
	set_exit_callback(pgres_destroy);

	/* sql-interface compliant functions */
	/* function(void|string,void|string,void|string,void|string,int|void:void) */
  ADD_FUNCTION("create",f_create,tFunc(tOr(tVoid,tStr) tOr(tVoid,tStr) tOr(tVoid,tStr) tOr(tVoid,tStr) tOr(tInt,tVoid),tVoid),
		OPT_EXTERNAL_DEPEND);
	/* That is: create(hostname,database,port)
	 * It depends on the environment variables:
	 * PGHOST, PGOPTIONS, PGPORT, PGTTY(don't use!), PGDATABASE
	 * Notice: Postgres _requires_ a database to be selected upon connection
	 */
	/* function(string:void) */
  ADD_FUNCTION("select_db",f_select_db,tFunc(tStr,tVoid),
			OPT_EXTERNAL_DEPEND);
	/* function(string:int|object) */
  ADD_FUNCTION("big_query",f_big_query,tFunc(tStr,tOr(tInt,tObj)),
			OPT_EXTERNAL_DEPEND|OPT_RETURN);
	/* function(void:string) */
  ADD_FUNCTION("Pike_error",f_error,tFunc(tVoid,tStr),
			OPT_EXTERNAL_DEPEND|OPT_RETURN);
	/* function(void:string) */
  ADD_FUNCTION("host_info",f_host_info,tFunc(tVoid,tStr),
			OPT_EXTERNAL_DEPEND|OPT_RETURN);

	/* postgres-specific functions */
	/* function(void:void) */
  ADD_FUNCTION("reset",f_reset,tFunc(tVoid,tVoid),OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);

#if 0
	 /* function(object|int:void) */
  ADD_FUNCTION("trace",f_trace,tFunc(tOr(tObj,tInt),tVoid),
		 OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);
	 /* If given a clone of /precompiled/file, traces to that file.
		* If given 0, stops tracing.
		* See note on the implementation.
		*/
#endif
	/* function(int|function(string:void):void) */
  ADD_FUNCTION("_set_notify_callback",f_callback,tFunc(tOr(tInt,tFunc(tStr,tVoid)),tVoid),
			OPT_SIDE_EFFECT);

	postgres_program = end_program();
	add_program_constant("postgres",postgres_program,0);

	add_string_constant("version",PGSQL_VERSION,0);

	pgresult_init(); 
}

void pike_module_exit(void)
{
  extern struct program * pgresult_program;

  if(postgres_program)
  {
    free_program(postgres_program);
    postgres_program=0;
  }

  if(pgresult_program)
  {
    free_program(pgresult_program);
    pgresult_program=0;
  }
}

#else /* HAVE_POSTGRES */
#include "module_magic.h"
void pike_module_init(void) {}
void pike_module_exit(void) {}
#endif /* HAVE_POSTGRES */

