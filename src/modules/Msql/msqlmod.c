/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: msqlmod.c,v 1.29 2005/11/15 00:36:58 nilsson Exp $
*/

/* All this code is pretty useless if we don't have a msql library...*/
#include "global.h"
#include "module.h"
#include "msql_config.h"
#include "program.h"
#include "module_support.h"

#ifdef HAVE_MSQL

/* #define MSQL_DEBUG 1 */

#ifdef MSQL_DEBUG
#include <stdio.h>
#endif

#include <string.h>
#include <stdlib.h>

#include "threads.h"
#include "machine.h"
#include "interpret.h"
#include "builtin_functions.h"
#include "svalue.h"
#include "array.h"
#include "mapping.h"
#include "stralloc.h"
#include "operators.h"
#include "multiset.h"

#include "version.h"

#ifdef _REENTRANT
MUTEX_T pike_msql_mutex STATIC_MUTEX_INIT;
#define MSQL_LOCK() mt_lock(&pike_msql_mutex)
#define MSQL_UNLOCK() mt_unlock(&pike_msql_mutex)
#else
#define MSQL_LOCK() /**/
#define MSQL_UNLOCK() /**/
#endif

/* Avoid a redefinition */
#ifdef INT_TYPE
#undef INT_TYPE
#endif /* INT_TYPE */

#include <msql.h>

#define sp Pike_sp

/*! @module Msql
 *! This is an interface to the mSQL database server.
 *! This module may or may not be availible on your Pike, depending
 *! whether the appropriate include and library files (msql.h and libmsql.a
 *! respectively) could be found at compile-time. Note that you @b{do not@}
 *! need to have a mSQL server running on your host to use this module:
 *! you can connect to the database over a TCP/IP socket
 *!
 *! Please notice that unless you wish to specifically connect to a mSQL
 *! server, you'd better use the @[Sql.Sql] program instead. Using @[Sql.Sql]
 *! ensures that your Pike applications will run with any supported SQL
 *! server without changing a single line of code.
 *!
 *! Also notice that some functions may be mSQL/2.0-specific, and thus missing
 *! on hosts running mSQL/1.0.*
 *!
 *! @note
 *!  The mSQL C API has some extermal dependencies.
 *!  They take the form of certain environment variables
 *!  which, if defined in the environment of the pike interpreter, influence
 *!  the interface's behavior. Those are "MSQL_TCP_PORT" which forces the
 *!  server to connect to a port other than the default, "MSQL_UNIX_PORT", same
 *!  as above, only referring to the UNIX domain sockets. If you built your
 *!  mSQL server with the default setttings, you shouldn't worry about these.
 *!  The variable MINERVA_DEBUG can be used to debug the mSQL API (you
 *!  shouldn't worry about this either). Refer to the mSQL documentation
 *!  for further details.
 *!
 *!  Also note that THIS MODULE USES BLOCKING I/O to connect to the server.
 *!  mSQL should be reasonably fast, but you might want to consider this
 *!  particular aspect. It is thread-safe, and so it can be used
 *!  in a multithread environment.
 *!
 *! @fixme
 *! Although it seems that mSQL/2.0 has some support for server statistics,
 *! it's really VERY VERY primitive, so it won't be added for now.
 *!
 *! @seealso
 *!   @[Sql.Sql]
 */

/*! @class msql
 */

static char * decode_msql_type (int msql_type)
{
	switch (msql_type) {
		case INT_TYPE: return "int";
		case CHAR_TYPE: return "char";
		case REAL_TYPE: return "real";
		case IDENT_TYPE: return "ident";
		case NULL_TYPE: return "null";
#ifdef MSQL_VERSION_2
		case TEXT_TYPE: return "text";
		case UINT_TYPE: return "unsigned int";
		case IDX_TYPE: return "index";
		case SYSVAR_TYPE: return "sysvar";
		case ANY_TYPE: return "any";
#endif
		default: return "unknown";
	}
}

struct msql_my_data
{
	int socket; /* the communication socket between us and the database engine. */
	unsigned int db_selected:1; /*flag: if we selected a database*/
	unsigned int connected:1; /*flag: we connected to a server*/
	struct pike_string *error_msg;
#ifdef MSQL_VERSION_2
	int affected;
#endif
};

#define THIS ((struct msql_my_data *) Pike_fp->current_storage)

static void msql_object_created (struct object *o)
{
	THIS->connected=0;
	THIS->db_selected=0;
	THIS->error_msg=NULL;
}

static void msql_object_destroyed (struct object * o)
{
	if (THIS->connected) {
		int socket=THIS->socket;
		THREADS_ALLOW();
		MSQL_LOCK();
		msqlClose(socket);
		MSQL_UNLOCK();
		THREADS_DISALLOW();
	}
	if (THIS->error_msg)
		free_string(THIS->error_msg);
}

static void report_error (void)
{
	if (THIS->error_msg)
		free_string(THIS->error_msg);
	THIS->error_msg=make_shared_string(msqlErrMsg);
	/* msqlErrMsg is really a char[160] in mSQL/2.0, but I don't want
		 to take any chances, even if I'm wasting some time here. */
}

static void do_select_db(char * dbname)
{
	/* NOTICE: We're assuming we're connected. CHECK before calling! */
	int status,socket=THIS->socket;

	THREADS_ALLOW();
	MSQL_LOCK();
	status=msqlSelectDB(socket,dbname);
	MSQL_UNLOCK();
	THREADS_DISALLOW();

	if (status==-1)
	{
		THIS->db_selected=0;
		report_error();
		Pike_error("Could not select database.\n");
	}
	THIS->db_selected=1;
	return;
}

/*! @decl void shutdown()
 *!
 *! This function shuts a SQL-server down.
 */

static void do_shutdown (INT32 args)
/* Notice: the msqlShutdown() function is undocumented. I'll have to go
	 through the source to find how to report errors.*/
{
	int status=0,socket=THIS->socket;
	check_all_args("Msql->shutdown",args,0);
	pop_n_elems(args);

	if (!THIS->connected)
		Pike_error ("Not connected to any server.\n");

	THREADS_ALLOW();
	MSQL_LOCK();
	status=msqlShutdown(socket);
	if (status>=0)
		msqlClose(socket); /*DBserver is shut down, might as well close */
	MSQL_UNLOCK();
	THREADS_DISALLOW();
	if (status<0) {
		report_error();
		Pike_error ("Error while shutting down the DBserver, connection not closed.\n");
	}
	THIS->connected=0;
	THIS->db_selected=0;
}

/*! @decl void reload_acl()
 *!
 *! This function forces a server to reload its ACLs.
 *!
 *! @note
 *!  This function is @b{not@} part of the standard interface, so it is
 *!  @b{not@} availible through the @[Sql.Sql] interface, but only through
 *!  @[Sql.msql] and @[Msql.msql] programs.
 *!
 *! @seealso
 *!   @[create]
 */

static void do_reload_acl (INT32 args)
/* Undocumented mSQL function. */
{
	int socket,status=0;
	check_all_args("Msql->reload_acl",args,0);
	pop_n_elems(args);
	if (!THIS->connected)
		Pike_error ("Not connected to any server.\n");

	socket=THIS->socket;
	THREADS_ALLOW();
	MSQL_LOCK();
	status=msqlReloadAcls(socket);
	MSQL_UNLOCK();
	THREADS_DISALLOW();
	if (status<0) {
		report_error();
		Pike_error ("Could not reload ACLs.\n");
	}
}

/*! @decl void create (void|string dbserver, void|string dbname,@
 *!                    void|string username, void|string passwd)
 *! With one argument, this function
 *! tries to connect to the specified (use hostname or IP address) database
 *! server. To connect to a server running on the local host via UNIX domain
 *! sockets use @expr{"localhost"@}. To connect to the local host via TCP/IP
 *! sockets
 *! you have to use the IP address @expr{"127.0.0.1"@}.
 *! With two arguments it also selects a database to use on the server.
 *! With no arguments it tries to connect to the server on localhost, using
 *! UNIX sockets.
 *!
 *! @throws
 *! You need to have a database selected before using the sql-object,
 *! otherwise you'll get exceptions when you try to query it.
 *! Also notice that this function @b{can@} raise exceptions if the db
 *! server doesn't respond, if the database doesn't exist or is not accessible
 *! by you.
 *!
 *! @note
 *! You don't need bothering about syncronizing the connection to the database:
 *! it is automatically closed (and the database is sync-ed) when the msql
 *! object is destroyed.
 *!
 *! @seealso
 *!   @[select_db]
 */

static void msql_mod_create (INT32 args)
{
	struct pike_string * arg1=NULL, *arg2=NULL;
	int sock, status;
	char *colon;

	check_all_args("Msql->create",args,
			BIT_STRING|BIT_VOID,BIT_STRING|BIT_VOID,BIT_STRING|BIT_VOID,
			BIT_STRING|BIT_VOID,0);

	if (args)
	  if (sp[-args].u.string->len)
	    arg1=sp[-args].u.string;
	if (args >1)
	  if (sp[1-args].u.string->len)
	    arg2=sp[1-args].u.string;

	/*Okay. We had one or two arguments, and we must connect to a server
	and if needed select a database.
	First off let's check whether we are already connected. In this case,
	disconnect.*/
	if (THIS->connected)
	{
		msqlClose (THIS->socket);
		THIS->connected=0;
		THIS->db_selected=0;
	}

	/* msql won' support specifying a port number to connect to. 
	 * As far as I know, ':' is not a legal character in an hostname,
	 * so we'll silently ignore it.
	 */
	if (arg1) {
		colon=strchr(arg1->str,':');
		if (colon) {
		  arg1 = make_shared_binary_string(arg1->str,
						   colon - arg1->str);
		  free_string(sp[-args].u.string);
		  sp[-args].u.string = arg1;
		}
	}

	THREADS_ALLOW();
	MSQL_LOCK();
	/* Warning! If there were no args, we're deferencing a NULL pointer!*/
	if (!arg1 || !strcmp (arg1->str,"localhost"))
		sock=msqlConnect(NULL);
	else
		sock=msqlConnect(arg1->str);
	MSQL_UNLOCK();
	THREADS_DISALLOW();

	if (sock==-1) {
		THIS->db_selected=0;
		THIS->connected=0;
		report_error();
		Pike_error("Error while connecting to mSQL server.\n");
	}
	THIS->socket=sock;
	THIS->connected=1;
	if (arg2)
	  do_select_db(arg2->str);
	pop_n_elems(args);
}

/*! @decl array list_dbs(void|string wild)
 *!
 *! Returns an array containing the names of all databases availible on
 *! the system. Will throw an exception if there is no server connected.
 *! If an argument is specified, it will return only those databases
 *! whose name matches the given glob.
 */

static void do_list_dbs (INT32 args)
{
	m_result * result;
	m_row row;
	int fields,numrows=0,socket=THIS->socket;

	check_all_args("Msql->list_dbs",args,BIT_STRING|BIT_VOID,0);

	if (!THIS->connected)
		Pike_error ("Not connected.\n");
	if (args>0 && sp[-args].u.string->len)
		/* We have a glob. We should pop the arg and push it again for a later
		 * call to glob() */
		;
	else {
		pop_n_elems(args);
		args=0; /*Let's use args as a flag. If args==0, no globbing.*/
	}

	THREADS_ALLOW();
	MSQL_LOCK();
	result=msqlListDBs(socket);
	MSQL_UNLOCK();
	THREADS_DISALLOW();

	if (!result) {
		f_aggregate(0); /*empty array if no databases*/
		return;
	}
	while ((row=msqlFetchRow(result))) /*it's fast, we're in RAM*/
	{
		numrows++;
		push_text((char *)row[0]);
	}
	f_aggregate(numrows);
	msqlFreeResult(result);
	if (args)
		f_glob(2);
	return;
}

/*! @decl array list_tables(void|string wild)
 *!
 *! Returns an array containing the names of all the tables in the currently
 *! selected database. Will throw an exception if we aren't connected to
 *! a database.
 *! If an argument is specified, it will return only those tables
 *! whose name matches the given glob.
 */

static void do_list_tables (INT32 args)
	/* ARGH! There's much code duplication here... but the subtle differences
	 * between the various functions make it impervious to try to generalize..*/
{
	m_result * result;
	m_row row;
	int fields,numrows=0,socket=THIS->socket;

	check_all_args ("Msql->list_tables",args,BIT_STRING|BIT_VOID,0);

	if (!THIS->db_selected)
		Pike_error ("No database selected.\n");

	if (args>0 && sp[-args].u.string->len)
		/* We have a glob. We should pop the arg and push it again for a later
		 * call to glob() */
		;
	else {
		pop_n_elems(args);
		args=0; /*Let's use args as a flag. If args==0, no globbing.*/
	}

	THREADS_ALLOW();
	MSQL_LOCK();
	result=msqlListTables(socket);
	MSQL_UNLOCK();
	THREADS_DISALLOW();

	if (!result) {
		f_aggregate(0); /*empty array if no databases*/
		return;
	}
	while ((row=msqlFetchRow(result)))
	{
		numrows++;
		push_text((char *)row[0]);
	}
	f_aggregate(numrows);
	msqlFreeResult(result);
	if (args)
		f_glob(2);
	return;
}

/*! @decl void select_db(string dbname)
 *!
 *! Before querying a database you have to select it. This can be accomplished
 *! in two ways: the first is calling the @[create] function with two arguments,
 *! another is calling it with one or no argument and then calling @[select_db].
 *! You can also use this function to change the database you're querying,
 *! as long as it is on the same server you are connected to.
 *!
 *! @throws
 *! This function CAN raise exceptions in case something goes wrong
 *! (for example: unexistant database, insufficient permissions, whatever).
 *!
 *! @seealso
 *!  @[create], @[error]
 */

static void select_db(INT32 args)
{
	struct pike_string * arg;
	int status;

	check_all_args("Msql->select_db",args,BIT_STRING,0);
	if (!THIS->connected)
		Pike_error ("Not connected.\n");
	arg=sp[-args].u.string;
	do_select_db(arg->str);
	pop_n_elems(args);
}

/*! @decl array(mapping(string:mixed)) query (string sqlquery)
 *!
 *! This is all you need to query the database. It takes as argument an SQL
 *! query string (i.e.: "SELECT foo,bar FROM baz WHERE name like '%kinkie%'"
 *! or "INSERT INTO baz VALUES ('kinkie','made','this')")
 *! and returns a data structure containing the returned values.
 *! The structure is an array (one entry for each returned row) of mappings
 *! which have the column name as index and the column contents as data.
 *! So to access a result from the first example you would have to do
 *! "results[0]->foo".
 *!
 *! A query which returns no data results in an empty array (and NOT in a 0).
 *! Also notice that when there is a column name clash (that is: when you
 *! join two or more tables which have columns with the same name), the
 *! clashing columns are renamed to <tablename>+"."+<column name>.
 *! To access those you'll have to use the indexing operator '[]
 *! (i.e.: results[0]["foo.bar"]).
 *!
 *! @throws
 *! Errors (both from the interface and the SQL server) are reported via
 *! exceptions, and you definitely want to catch them. Error messages are
 *! not particularly verbose, since they account only for errors inside the
 *! driver. To get server-related error messages, you have to use the
 *! @[error] function.
 *!
 *! @note
 *! Note that if the query is NOT a of SELECT type, but UPDATE or
 *! MODIFY, the returned value is an empty array.
 *! @b{This is not an error@}. Errors are reported @b{only@} via exceptions.
 *!
 *! @seealso
 *!   @[error]
 */

static void do_query (INT32 args)
{
	int status, num_fields,num_rows,j,k,tmp_socket,*duplicate_names_map;
	m_result * result;
	m_field * current_field;
	m_row row;
	struct array *field_names, *retval;
	char * query, ** names;

	check_all_args("Msql->query",args,BIT_STRING,0);

	if (!THIS->connected)
		Pike_error("Must connect to database server before querying it.\n");
	if (!THIS->db_selected)
		Pike_error("Must select database before querying it.\n");

	tmp_socket=THIS->socket;
	query=sp[-args].u.string->str;

#ifdef MSQL_DEBUG
	printf ("MSQLMod: Query is\"%s\"\n",query);
#endif

	THREADS_ALLOW();
	MSQL_LOCK();
	status=msqlQuery(tmp_socket,query);
	MSQL_UNLOCK();
	THREADS_DISALLOW();

	if (status==-1) {
		report_error();
		Pike_error("Error in SQL query.\n");
	}

#ifdef MSQL_VERSION_2
	THIS->affected=status;
#endif
	pop_n_elems(args);
	/*We have what we want. We need to construct the returned structure*/
	THREADS_ALLOW();
	MSQL_LOCK();
	result=msqlStoreResult();
	MSQL_UNLOCK();
	THREADS_DISALLOW();
	if (!result || !(num_rows=msqlNumRows(result)) ) {
		f_aggregate(0);
		return;
	}

	/*First off: we need to know the field names
	 *in their correct order, and store them in arrays for further analysis
	 */
	num_fields=msqlNumFields(result);
	duplicate_names_map = malloc (sizeof(int)*num_fields);
	names = malloc (sizeof(char*)*num_fields);
	if (!duplicate_names_map || !names)
		Pike_error ("Memory exhausted.\n");

	/* initialize support structure for duplicate column names */
	for (j=0;j<num_fields;j++)
		duplicate_names_map[j]=0;

	/* Find duplicate column names */
	for (j=0;j<num_fields;j++) {
		current_field=msqlFetchField(result);
		names[j]=current_field->name;
	}
	for (j=0;j<num_fields;j++)
		for (k=j+1;k<num_fields;k++)
			if (!strcmp(names[j],names[k])) {
				duplicate_names_map[j]=1;
				duplicate_names_map[k]=1;
			}

	/* Reset field cursor */
	msqlFieldSeek(result,0);

	/* create array containing column names */
	for (j=0;j<num_fields;j++) {
		current_field=msqlFetchField(result);
		if (!current_field)
			Pike_error ("Huh? Weird! Null field returned at index %d.\n",j);
		if (duplicate_names_map[j]) { 
			/* If we have a clashing column name we need to prepend "tablename."*/
			push_text (current_field->table);
			push_text (".");
			push_text(current_field->name);
			f_add(3);
		}
		else
			push_text(current_field->name);
	}

	field_names=aggregate_array(num_fields);

	/* Let's fetch the rows and accumulate them as mappings on the stack. */
	for (j=0;j<num_rows;j++)
	{
		struct array * this_row;
		struct mapping * this_result;
		int k;
		row=msqlFetchRow(result);
		for (k=0;k<num_fields;k++) {
			if (!row[k]) {
				push_int(0);
				continue;
			}
			push_text((char *)row[k]); break;
		}
		this_row=aggregate_array(num_fields);
		this_result=mkmapping(field_names,this_row);
		push_mapping(this_result); /*we're putting mappings on the stack*/
		free_array(this_row);
	}
	
	/* Wrap it up and free the used memory */
	f_aggregate(num_rows); /* aggregate and push the resulting array */
	free_array(field_names);
	free(duplicate_names_map);
	free(names);
	msqlFreeResult(result);
}

/*! @decl string server_info()
 *!
 *! This function returns a string describing the server we are talking
 *! to. It has the form "servername/serverversion" (like the HTTP protocol
 *! description) and is most useful in conjunction with the generic SQL-server
 *! module.
 *!
 */

static void do_info (INT32 args)
{
	char * info;

	check_all_args("Msql->info",args,0);
	pop_n_elems(args);
	if (!THIS->connected)
		Pike_error ("Not connected.\n");
	push_text("msql/");
	THREADS_ALLOW();
	MSQL_LOCK();
	info=msqlGetServerInfo();
	MSQL_UNLOCK();
	THREADS_DISALLOW();
	push_text(info);
	f_add(2);
	return;
}

/*! @decl string host_info()
 *!
 *! This function returns a string describing what host are we talking to,
 *! and how (TCP/IP or UNIX sockets).
 */

static void do_host_info (INT32 args)
{
	check_all_args("Msql->host_info",args,0);
	pop_n_elems(args);
	if (!THIS->connected)
		Pike_error ("Not connected.\n");
	/*it's local to the client library. Not even worth allowing
	 context switches*/
	push_text(msqlGetHostInfo()); 
	return;
}

/*! @decl string error()
 *!
 *! This function returns the textual description of the last server-related
 *! error. Returns 0 if no error has occurred yet. It is not cleared upon
 *! reading (can be invoked multiple times, will return the same result
 *! until a new error occurs).
 *!
 *! @seealso
 *!   @[query]
 */

static void do_error (INT32 args)
{
	check_all_args("Msql->error",args,0);
	pop_n_elems(args);
	if (THIS->error_msg)
		ref_push_string(THIS->error_msg);
	else
	        push_int(0);
	return;
}

/*! @decl void create_db(string dbname)
 *!
 *! This function creates a new database with the given name (assuming we
 *! have enough permissions to do this).
 *!
 *! @seealso
 *!   @[drop_db]
 */

static void do_create_db (INT32 args)
{
	int dbresult;
	char * dbname;
	int socket;

	check_all_args("Msql->create_db",args,BIT_STRING,0);

	if (!THIS->connected)
		Pike_error("Not connected.\n");
	dbname = sp[-args].u.string->str;
	socket=THIS->socket;
	THREADS_ALLOW();
	MSQL_LOCK();
	dbresult=msqlCreateDB(socket,dbname);
	MSQL_UNLOCK();
	THREADS_DISALLOW();
	if (dbresult==-1) {
		report_error();
		Pike_error ("Could not create database.\n");
	}
	pop_n_elems(args);
}

/*! @decl void drop_db(string dbname)
 *!
 *! This function destroys a database and all the data it contains (assuming
 *! we have enough permissions to do so). USE WITH CAUTION!
 *!
 *! @seealso
 *!   @[create_db]
 */

static void do_drop_db (INT32 args)
{
	int dbresult;
	char * dbname;
	int socket;
	
	check_all_args("Msql->drop_db",args,BIT_STRING,0);

	if (!THIS->connected)
		Pike_error("Not connected.\n");
	dbname = sp[-args].u.string->str;
	socket=THIS->socket;
	THREADS_ALLOW();
	MSQL_LOCK();
	dbresult=msqlDropDB(socket,dbname);
	MSQL_UNLOCK();
	THREADS_DISALLOW();
	if (dbresult==-1) {
		report_error();
		Pike_error ("Could not drop database.\n");
	}
	pop_n_elems(args);
	return;
}

/*! @decl mapping(string:mapping(string:mixed)) list_fields(string table, void|string glob)
 *!
 *! Returns a mapping describing the fields of a table in the database.
 *! The returned value is a mapping, indexed on the column name,
 *! of mappings.The glob argument, if present, filters out the fields
 *! not matching the glob. These contain currently the fields:
 *!
 *! @mapping
 *!   @member string "type"
 *!      Describes the field's mSQL data type ("char","integer",...)
 *!   @member int "length"
 *!      It describes the field's length. It is only interesting for
 *!      char() fields, in fact.  Also
 *!      notice that the strings returned by msql->query() have the correct length.
 *!      This field only specifies the _maximum_ length a "char()" field can have.
 *!   @member string "table"
 *!      The table this field is in. Added only for interface compliancy.
 *!   @member multiset(string) "flags"
 *!      It's a multiset containing textual
 *!      descriptions of the server's flags associated with the current field.
 *!      Currently it can be empty, or contain "unique" or "not null".
 *! @endmapping
 *!
 *! @note
 *!   The version of this function in the Msql.msql() program is @b{not@}
 *!   sql-interface compliant (this is the main reason why using that program
 *!   directly is deprecated). Use @[Sql.Sql] instead.
 *!
 *! @seealso
 *!   @[query]
 */

static void do_list_fields (INT32 args)
{
	m_result * result;
	m_field * field;
	char * table;
	int fields, j, socket=THIS->socket;

	check_all_args("Msql->list_fields",args,BIT_STRING,0);
	if (!THIS->connected)
		Pike_error ("Not connected.\n");
	if (!THIS->db_selected)
		Pike_error ("Must select a db first.\n");
	table=sp[-args].u.string->str;
#ifdef MSQL_DEBUG
	printf ("list_fields: table=%s(%d)\n",sp[-args].u.string->str,sp[-args].u.string->len);
#endif

	THREADS_ALLOW();
	MSQL_LOCK();
	result=msqlListFields(socket,table);
	MSQL_UNLOCK();
	THREADS_DISALLOW();
	pop_n_elems(args);

	if (!result) {
		report_error();
		Pike_error ("No fields information.\n");
	}
	
	fields = msqlNumFields(result);
	if (!fields)
		Pike_error ("No such table.\n");

	for (j=0;j<fields;j++)
	{
		int flagsnum=0;
		field=msqlFetchField(result);
    
    push_text("name");
		push_text(field->name);
		push_text("type");
		push_text(decode_msql_type(field->type));
		push_text("length");
		push_int(field->length);
		push_text("table");
		push_text(field->table);
		push_text("flags");
#ifdef IS_UNIQUE
		if (IS_UNIQUE(field->flags)) {
			push_text("unique");
			flagsnum++;
		}
#endif
#ifdef IS_NOT_NULL
		if (IS_NOT_NULL(field->flags)) {
			push_text("not_null");
			flagsnum++;
		}
#endif
#ifdef IS_PRI_KEY
		if (IS_PRI_KEY(field->flags)) {
			push_text("primary_key");
			flagsnum++;
		}
#endif
		f_aggregate_multiset(flagsnum);
		f_aggregate_mapping(10);
	}
  f_aggregate(fields);

	msqlFreeResult(result);
	return;
}

#ifdef MSQL_VERSION_2

/*! @decl int affected_rows()
 *!
 *! This function returns how many rows in the database were affected by
 *! our last SQL query.
 *!
 *! @note
 *!  This function is availible only if you're using mSQL version 2
 *!  or later. (That means: if the includes and library of version 2 of mSQL
 *!  were availible when the module was compiled).
 *!
 *!  This function is @b{not@} part of the standard interface, so it is @b{not@}
 *!  availible through the @[Sql.Sql] interface, but only through @[Sql.msql] and
 *!  @[Msql.msql] programs
 */

static void do_affected_rows (INT32 args)
{
	check_all_args("Msql->affected_rows",args,0);
	pop_n_elems(args);
	push_int(THIS->affected);
	return;
}

/*! @decl array list_index(string tablename, string indexname)
 *!
 *! This function returns an array describing the index structure for the
 *! given table and index name, as defined by the non-standard SQL query
 *! 'create index' (see the mSQL documentation for further informations).
 *! More than one index can be created for a table. There's currently NO way
 *! to have a listing of the indexes defined for a table (blame it on
 *! the mSQL API).
 *!
 *! @note
 *!  This function is availible if you're using mSQL version 2
 *!  or later.
 *!
 *!  This function is @b{not@} part of the standard interface, so it is @b{not@}
 *!  availible through the @[Sql.Sql] interface, but only through @[Sql.msql] and
 *!  @[Msql.msql] programs.
 */

static void do_list_index (INT32 args)
{
	char * arg1, *arg2;
	m_result * result;
	m_row row;
	int sock, rows, j;

	check_all_args("Msql->list_index",args,BIT_STRING,BIT_STRING,0);
	if (!THIS->db_selected)
		Pike_error ("No database selected.\n");
	arg1=sp[-args].u.string->str;
	arg2=sp[1-args].u.string->str;
	sock=THIS->socket;
	THREADS_ALLOW();
	MSQL_LOCK();
	result=msqlListIndex(sock,arg1,arg2);
	MSQL_UNLOCK();
	THREADS_DISALLOW();
	pop_n_elems(args);
	if (!result || !(rows=msqlNumRows(result)) ) {
		f_aggregate(0);
		return;
	}
	msqlFetchRow(result); /*The first one is the internal type, useless*/
	rows--;
	for (j=0; j<rows; j++)
	{
		row=msqlFetchRow(result);
		push_text(row[0]);
	}
	f_aggregate(rows);
	return;
}
#endif

/*! @endclass
 */

/*! @decl constant version
 *!
 *! Should you need to report a bug to the author, please submit along with
 *! the report the driver version number, as returned by this call.
 */

/*! @endmodule
 */

PIKE_MODULE_INIT
{
  start_new_program();
  ADD_STORAGE(struct msql_my_data);

  set_init_callback (msql_object_created);
  set_exit_callback (msql_object_destroyed);

  /* function(void|string,void|string,void|string,void|string:void) */
  ADD_FUNCTION("create", msql_mod_create,
	       tFunc(tOr(tVoid,tStr) tOr(tVoid,tStr) tOr(tVoid,tStr)
		     tOr(tVoid,tStr),tVoid), 0);

  /* 1st arg: hostname or "localhost", 2nd arg: dbname or nothing 
   * CAN raise exception if there is no server listening, or no database
   * To connect using the UNIX socket instead of a localfunction use the
   * hostname "localhost", or use no argument. It will use UNIX sockets.
   * Third and fourth argument are currently ignored, since mSQL doesn't
   * support user/passwd authorization. The user will be the owner of
   * the current process.
   * The first argument can have the format "hostname:port". Since mSQL
   * doesn't support nonstandard ports, that portion is silently ignored,
   * and is provided only for generic-interface compliancy
   */

  /* function(string:void) */
  ADD_FUNCTION("select_db", select_db, tFunc(tStr,tVoid), 0);

  /* if no db was selected by connect, does it now.
   * CAN raise an exception if there's no such database or we haven't selected
   * an host.
   */

  /* function(string:array(mapping(string:mixed))) */
  ADD_FUNCTION("query", do_query, tFunc(tStr,tArr(tMap(tStr,tMix))), 0);

  /* Gets an SQL query, and returns an array of the results, one element
   * for each result line, each row is a mapping with the column name as
   * index and the data as value.
   * CAN raise excaptions if there's no active database.
   */

  /* function(void|string:array(string)) */
  ADD_FUNCTION("list_dbs", do_list_dbs, tFunc(tOr(tVoid,tStr),tArr(tStr)), 0);

  /* Lists the tables contained in the selected database. */

  /* function(void:array(string)) */
  ADD_FUNCTION("list_tables", do_list_tables, tFunc(tVoid,tArr(tStr)), 0);

  /* Lists the tables contained in the selected database. */

  /* function(string:mapping(string:array(mixed))) */
  ADD_FUNCTION("list_fields", do_list_fields,
	       tFunc(tStr,tArr(tMap(tStr,tMix))), 0);

  /* Returns information on the the fields of the given table of the current
     database */

  /* function(void:void|string) */
  ADD_FUNCTION("error", do_error, tFunc(tVoid,tOr(tVoid,tStr)), 0);

  /* return the last error reported by the server. */

  /* function(void:string) */
  ADD_FUNCTION("server_info", do_info, tFunc(tVoid,tStr), 0);

  /* Returns "msql/<server_version>" */

  /* function(void:string) */
  ADD_FUNCTION("host_info", do_host_info, tFunc(tVoid,tStr), 0);

  /* Returns information on the connection type and such */

  /* function(string:void) */
  ADD_FUNCTION("create_db", do_create_db, tFunc(tStr,tVoid), 0);

  /* creates a new database with the name as argument */

  /* function(string:void) */
  ADD_FUNCTION("drop_db", do_drop_db, tFunc(tStr,tVoid), 0);

  /* destroys a database and its contents */

  /* function(void:void) */
  ADD_FUNCTION("shutdown", do_shutdown, tFunc(tVoid,tVoid), 0);

  /* Shuts the server down */

  /* function(void:void) */
  ADD_FUNCTION("reload_acl", do_reload_acl, tFunc(tVoid,tVoid), 0);

  /* Reloads the ACL for the DBserver */

#ifdef MSQL_VERSION_2
  /* function(void:int) */
  ADD_FUNCTION("affected_rows", do_affected_rows, tFunc(tVoid,tInt), 0);

  /* Returns the number of rows 'touched' by last query */

  /* function(string,string:array) */
  ADD_FUNCTION("list_index", do_list_index, tFunc(tStr tStr,tArray), 0);

  /* Returns the index structure on the specified table */
#endif

  end_class("msql",0);

  /* Versioning information, to be obtained as "Msql.version". Mainly a
   * convenience for RCS
   */
  add_string_constant("version",MSQLMOD_VERSION,0);
}

#else /*HAVE_MSQL*/
PIKE_MODULE_INIT {
  if(!TEST_COMPAT(7,6))
    HIDE_MODULE();
}
#endif /*HAVE_MSQL*/

PIKE_MODULE_EXIT { }
