/*
 * This code is (C) Francesco Chemolli, 1997.
 * You may use, modify and redistribute it freely under the terms
 * of the GNU General Public License, version 2.
 * $Id: msqlmod.c,v 1.11 2000/04/30 21:16:55 kinkie Exp $
 *
 * This version is intended for Pike/0.5 and later.
 * It won't compile under older versions of the Pike interpreter.
 */

/* All this code is pretty useless if we don't have a msql library...*/
#include "global.h"
#include "msql_config.h"
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
#include "module_support.h"
#include "svalue.h"
#include "program.h"
#include "array.h"
#include "mapping.h"
#include "stralloc.h"
#include "operators.h"
#include "multiset.h"

RCSID("$Id: msqlmod.c,v 1.11 2000/04/30 21:16:55 kinkie Exp $");
#include "version.h"

#ifdef _REENTRANT
MUTEX_T pike_msql_mutex;
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

#define THIS ((struct msql_my_data *) fp->current_storage)

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
		error("Could not select database.\n");
	}
	THIS->db_selected=1;
	return;
}

/* void shutdown() */
static void do_shutdown (INT32 args)
/* Notice: the msqlShutdown() function is undocumented. I'll have to go
	 through the source to find how to report errors.*/
{
	int status=0,socket=THIS->socket;
	check_all_args("Msql->shutdown",args,0);
	pop_n_elems(args);

	if (!THIS->connected)
		error ("Not connected to any server.\n");

	THREADS_ALLOW();
	MSQL_LOCK();
	status=msqlShutdown(socket);
	if (status>=0)
		msqlClose(socket); /*DBserver is shut down, might as well close */
	MSQL_UNLOCK();
	THREADS_DISALLOW();
	if (status<0) {
		report_error();
		error ("Error while shutting down the DBserver, connection not closed.\n");
	}
	THIS->connected=0;
	THIS->db_selected=0;
}

/* void reload_acl() */
static void do_reload_acl (INT32 args)
/* Undocumented mSQL function. */
{
	int socket,status=0;
	check_all_args("Msql->reload_acl",args,0);
	pop_n_elems(args);
	if (!THIS->connected)
		error ("Not connected to any server.\n");

	socket=THIS->socket;
	THREADS_ALLOW();
	MSQL_LOCK();
	status=msqlReloadAcls(socket);
	MSQL_UNLOCK();
	THREADS_DISALLOW();
	if (status<0) {
		report_error();
		error ("Could not reload ACLs.\n");
	}
}

/* void create (void|string dbserver, void|string dbname,
	void|string username, void|string passwd) */
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

	THREADS_ALLOW();
	/* msql won' support specifying a port number to connect to. 
	 * As far as I know, ':' is not a legal character in an hostname,
	 * so we'll silently ignore it.
	 */

	if (arg1) {
		colon=strchr(arg1->str,':');
		if (colon)
			*colon='\0';
	}

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
		error("Error while connecting to mSQL server.\n");
	}
	THIS->socket=sock;
	THIS->connected=1;
	if (!arg2)
		return;
	do_select_db(arg2->str);
	pop_n_elems(args);
}

/* array list_dbs(void|string wild) */
static void do_list_dbs (INT32 args)
{
	m_result * result;
	m_row row;
	int fields,numrows=0,socket=THIS->socket;

	check_all_args("Msql->list_dbs",args,BIT_STRING|BIT_VOID,0);

	if (!THIS->connected)
		error ("Not connected.\n");
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

/* array list_tables(void|string wild) */
static void do_list_tables (INT32 args)
	/* ARGH! There's much code duplication here... but the subtle differences
	 * between the various functions make it impervious to try to generalize..*/
{
	m_result * result;
	m_row row;
	int fields,numrows=0,socket=THIS->socket;

	check_all_args ("Msql->list_tables",args,BIT_STRING|BIT_VOID,0);

	if (!THIS->db_selected)
		error ("No database selected.\n");

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

/* void select_db(string dbname) */
static void select_db(INT32 args)
{
	struct pike_string * arg;
	int status;

	check_all_args("Msql->select_db",args,BIT_STRING,0);
	if (!THIS->connected)
		error ("Not connected.\n");
	arg=sp[-args].u.string;
	do_select_db(arg->str);
	pop_n_elems(args);
}

/* array(mapping(string:mixed)) query (string sqlquery) */
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
		error("Must connect to database server before querying it.\n");
	if (!THIS->db_selected)
		error("Must select database before querying it.\n");

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
		error("Error in SQL query.\n");
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
		error ("Memory exhausted.\n");

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
			error ("Huh? Weird! Null field returned at index %d.\n",j);
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

/* string server_info() */
static void do_info (INT32 args)
{
	char * info;

	check_all_args("Msql->info",args,0);
	pop_n_elems(args);
	if (!THIS->connected)
		error ("Not connected.\n");
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

/* string host_info() */
static void do_host_info (INT32 args)
{
	check_all_args("Msql->host_info",args,0);
	pop_n_elems(args);
	if (!THIS->connected)
		error ("Not connected.\n");
	/*it's local to the client library. Not even worth allowing
	 context switches*/
	push_text(msqlGetHostInfo()); 
	return;
}

/* string error() */
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

/* void create_db (string dbname) */
static void do_create_db (INT32 args)
{
	int dbresult;
	char * dbname;
	int socket;

	check_all_args("Msql->create_db",args,BIT_STRING,0);

	if (!THIS->connected)
		error("Not connected.\n");
	dbname = sp[-args].u.string->str;
	socket=THIS->socket;
	THREADS_ALLOW();
	MSQL_LOCK();
	dbresult=msqlCreateDB(socket,dbname);
	MSQL_UNLOCK();
	THREADS_DISALLOW();
	if (dbresult==-1) {
		report_error();
		error ("Could not create database.\n");
	}
	pop_n_elems(args);
}

/* void drop_db (string dbname) */
static void do_drop_db (INT32 args)
{
	int dbresult;
	char * dbname;
	int socket;
	
	check_all_args("Msql->drop_db",args,BIT_STRING,0);

	if (!THIS->connected)
		error("Not connected.\n");
	dbname = sp[-args].u.string->str;
	socket=THIS->socket;
	THREADS_ALLOW();
	MSQL_LOCK();
	dbresult=msqlDropDB(socket,dbname);
	MSQL_UNLOCK();
	THREADS_DISALLOW();
	if (dbresult==-1) {
		report_error();
		error ("Could not drop database.\n");
	}
	pop_n_elems(args);
	return;
}

/* mapping(string:array(string)) list_fields (string table) */
static void do_list_fields (INT32 args)
{
	m_result * result;
	m_field * field;
	char * table;
	int fields, j, socket=THIS->socket;

	check_all_args("Msql->list_fields",args,BIT_STRING,0);
	if (!THIS->connected)
		error ("Not connected.\n");
	if (!THIS->db_selected)
		error ("Must select a db first.\n");
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
		error ("No fields information.\n");
	}
	
	fields = msqlNumFields(result);
	if (!fields)
		error ("No such table.\n");

	for (j=0;j<fields;j++)
	{
		int flagsnum=0;
		field=msqlFetchField(result);
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
		f_aggregate_mapping(8); /*must aggregate on the fields above, except name*/
	}
	f_aggregate_mapping(fields*2);

	msqlFreeResult(result);
	return;
}

#ifdef MSQL_VERSION_2
/* int affected_rows() */
static void do_affected_rows (INT32 args)
{
	check_all_args("Msql->affected_rows",args,0);
	pop_n_elems(args);
	push_int(THIS->affected);
	return;
}

/* array list_index(string tablename, string indexname) */
static void do_list_index (INT32 args)
{
	char * arg1, *arg2;
	m_result * result;
	m_row row;
	int sock, rows, j;

	check_all_args("Msql->list_index",args,BIT_STRING,BIT_STRING,0);
	if (!THIS->db_selected)
		error ("No database selected.\n");
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

void pike_module_init(void)
{
	start_new_program();
	ADD_STORAGE(struct msql_my_data);

	set_init_callback (msql_object_created);
	set_exit_callback (msql_object_destroyed);

	/* function(void|string,void|string,void|string,void|string:void) */
  ADD_FUNCTION("create",msql_mod_create,tFunc(tOr(tVoid,tStr) tOr(tVoid,tStr) tOr(tVoid,tStr) tOr(tVoid,tStr),tVoid),
			OPT_EXTERNAL_DEPEND);
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
  ADD_FUNCTION("select_db",select_db,tFunc(tStr,tVoid),
		OPT_EXTERNAL_DEPEND);
	/* if no db was selected by connect, does it now.
	 * CAN raise an exception if there's no such database or we haven't selected
	 * an host.
	 */

	/* function(string:array(mapping(string:mixed))) */
  ADD_FUNCTION("query",do_query,tFunc(tStr,tArr(tMap(tStr,tMix))),
			OPT_ASSIGNMENT|OPT_TRY_OPTIMIZE|OPT_EXTERNAL_DEPEND|OPT_RETURN);
	/* Gets an SQL query, and returns an array of the results, one element
	 * for each result line, each row is a mapping with the column name as
	 * index and the data as value.
	 * CAN raise excaptions if there's no active database.
	 */

	/* function(void|string:array(string)) */
  ADD_FUNCTION("list_dbs",do_list_dbs,tFunc(tOr(tVoid,tStr),tArr(tStr)),
		OPT_ASSIGNMENT|OPT_EXTERNAL_DEPEND|OPT_RETURN);
	/* Lists the tables contained in the selected database. */

	/* function(void:array(string)) */
  ADD_FUNCTION("list_tables",do_list_tables,tFunc(tVoid,tArr(tStr)),
		OPT_ASSIGNMENT|OPT_EXTERNAL_DEPEND|OPT_RETURN);
	/* Lists the tables contained in the selected database. */

	/* function(string:mapping(string:array(mixed))) */
  ADD_FUNCTION("list_fields", do_list_fields,tFunc(tStr,tMap(tStr,tArr(tMix))),
		OPT_RETURN|OPT_EXTERNAL_DEPEND);
	/* Returns information on the the fields of the given table of the current
	  database */

	/* function(void:void|string) */
  ADD_FUNCTION("error",do_error,tFunc(tVoid,tOr(tVoid,tStr)),
		OPT_RETURN|OPT_EXTERNAL_DEPEND);
	/* return the last error reported by the server. */

	/* function(void:string) */
  ADD_FUNCTION("server_info", do_info,tFunc(tVoid,tStr),
		OPT_RETURN|OPT_EXTERNAL_DEPEND);
	/* Returns "msql/<server_version>" */

	/* function(void:string) */
  ADD_FUNCTION("host_info", do_host_info,tFunc(tVoid,tStr),
			OPT_EXTERNAL_DEPEND|OPT_RETURN);
	/* Returns information on the connection type and such */

	/* function(string:void) */
  ADD_FUNCTION("create_db", do_create_db,tFunc(tStr,tVoid),
		OPT_EXTERNAL_DEPEND);
	/* creates a new database with the name as argument */

	/* function(string:void) */
  ADD_FUNCTION("drop_db", do_drop_db,tFunc(tStr,tVoid),
		OPT_EXTERNAL_DEPEND);
	/* destroys a database and its contents */

	/* function(void:void) */
  ADD_FUNCTION("shutdown", do_shutdown,tFunc(tVoid,tVoid),
			OPT_EXTERNAL_DEPEND);
	/* Shuts the server down */

	/* function(void:void) */
  ADD_FUNCTION("reload_acl", do_reload_acl,tFunc(tVoid,tVoid),
			OPT_EXTERNAL_DEPEND);
	/* Reloads the ACL for the DBserver */

#ifdef MSQL_VERSION_2
	/* function(void:int) */
  ADD_FUNCTION("affected_rows", do_affected_rows,tFunc(tVoid,tInt),
		OPT_RETURN|OPT_EXTERNAL_DEPEND);
	/* Returns the number of rows 'touched' by last query */

	/* function(string,string:array) */
  ADD_FUNCTION("list_index", do_list_index,tFunc(tStr tStr,tArray),
			OPT_EXTERNAL_DEPEND);
	/* Returns the index structure on the specified table */
#endif

	end_class("msql",0);

	/* Versioning information, to be obtained as "Msql.version". Mainly a
	 * convenience for RCS
	 */
	add_string_constant("version",MSQLMOD_VERSION,0);
}

#else /*HAVE_MSQL*/
void pike_module_init(void) {}
#endif /*HAVE_MSQL*/

void pike_module_exit (void) { }
