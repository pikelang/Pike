/*
 * $Id: mysql.c,v 1.9 1997/11/05 14:50:00 grubba Exp $
 *
 * SQL database functionality for Pike
 *
 * Henrik Grubbström 1996-12-21
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_MYSQL

/*
 * Includes
 */

/* From the mysql-dist */
/* Workaround for versions prior to 3.20.0 not beeing protected for
 * multiple inclusion.
 */
#ifndef _mysql_h
#ifdef HAVE_MYSQL_H
#include <mysql.h>
#else
#ifdef HAVE_MYSQL_MYSQL_H
#include <mysql/mysql.h>
#else
#error Need mysql.h header-file
#endif /* HAVE_MYSQL_MYSQL_H */
#endif /* HAVE_MYSQL_H */
#ifndef _mysql_h
#define _mysql_h
#endif
#endif

/* dynamic_buffer.h contains a conflicting typedef for string
 * we don't use any dynamic buffers, so we have this work-around
 */
#define DYNAMIC_BUFFER_H
typedef struct dynamic_buffer_s dynamic_buffer;

#endif /* HAVE_MYSQL */

/* From the Pike-dist */
#include "global.h"
#include "svalue.h"
#include "object.h"
#include "stralloc.h"
#include "interpret.h"
#include "port.h"
#include "error.h"
#include "threads.h"
#include "program.h"
#include "operators.h"
#include "builtin_functions.h"

/* System includes */
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#ifdef HAVE_MYSQL

/* Local includes */
#include "precompiled_mysql.h"

/*
 * Globals
 */

RCSID("$Id: mysql.c,v 1.9 1997/11/05 14:50:00 grubba Exp $");

struct program *mysql_program = NULL;

/*
 * Functions
 */

/*
 * State maintenance
 */

static void init_mysql_struct(struct object *o)
{
  MEMSET(PIKE_MYSQL, 0, sizeof(struct precompiled_mysql));
}

static void exit_mysql_struct(struct object *o)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  MYSQL_RES *last_result = PIKE_MYSQL->last_result;

  PIKE_MYSQL->last_result = NULL;
  PIKE_MYSQL->socket = NULL;

  if (PIKE_MYSQL->password) {
    free_string(PIKE_MYSQL->password);
    PIKE_MYSQL->password = NULL;
  }
  if (PIKE_MYSQL->user) {
    free_string(PIKE_MYSQL->user);
    PIKE_MYSQL->user = NULL;
  }
  if (PIKE_MYSQL->database) {
    free_string(PIKE_MYSQL->database);
    PIKE_MYSQL->database = NULL;
  }
  if (PIKE_MYSQL->host) {
    free_string(PIKE_MYSQL->host);
    PIKE_MYSQL->host = NULL;
  }

  THREADS_ALLOW();

  if (last_result) {
    mysql_free_result(last_result);
  }
  if (socket) {
    mysql_close(socket);
  }

  THREADS_DISALLOW();
}


static void pike_mysql_reconnect(void)
{
  MYSQL *mysql = &(PIKE_MYSQL->mysql);
  MYSQL *socket;
  char *host = NULL;
  char *database = NULL;
  char *user = NULL;
  char *password = NULL;

  if (PIKE_MYSQL->host) {
    host = PIKE_MYSQL->host->str;
  }
  if (PIKE_MYSQL->database) {
    database = PIKE_MYSQL->database->str;
  }
  if (PIKE_MYSQL->user) {
    user = PIKE_MYSQL->user->str;
  }
  if (PIKE_MYSQL->password) {
    password = PIKE_MYSQL->password->str;
  }

  socket = PIKE_MYSQL->socket;
  PIKE_MYSQL->socket = NULL;

  THREADS_ALLOW();

  if (socket) {
    /* Disconnect the old connection */
    mysql_close(socket);
  }

  socket = mysql_connect(mysql, host, user, password);

  THREADS_DISALLOW();

  if (!(PIKE_MYSQL->socket = socket)) {
    error("Mysql.mysql(): Couldn't reconnect to SQL-server\n");
  }
  if (database) {
    int tmp;

    THREADS_ALLOW();

    tmp = mysql_select_db(socket, database);

    THREADS_DISALLOW();

    if (tmp < 0) {
      mysql_close(PIKE_MYSQL->socket);
      PIKE_MYSQL->socket = NULL;
      error("Mysql.mysql(): Couldn't select database \"%s\"\n", database);
    }
  }
}

/*
 * Methods
 */

/* void create(string|void host, string|void database, string|void user, string|void password) */
static void f_create(INT32 args)
{
  MYSQL *mysql = &PIKE_MYSQL->mysql;

  if (args >= 1) {
    if (sp[-args].type != T_STRING) {
      error("Bad argument 1 to mysql()\n");
    }
    if (sp[-args].u.string->len) {
      PIKE_MYSQL->host = sp[-args].u.string;
      PIKE_MYSQL->host->refs++;
    }

    if (args >= 2) {
      if (sp[1-args].type != T_STRING) {
	error("Bad argument 2 to mysql()\n");
      }
      if (sp[1-args].u.string->len) {
	PIKE_MYSQL->database = sp[1-args].u.string;
	PIKE_MYSQL->database->refs++;
      }

      if (args >= 3) {
	if (sp[2-args].type != T_STRING) {
	  error("Bad argument 3 to mysql()\n");
	}
	if (sp[2-args].u.string->len) {
	  PIKE_MYSQL->user = sp[2-args].u.string;
	  PIKE_MYSQL->user->refs++;
	}

	if (args >= 4) {
	  if (sp[3-args].type != T_STRING) {
	    error("Bad argument 4 to mysql()\n");
	  }
	  if (sp[3-args].u.string->len) {
	    PIKE_MYSQL->password = sp[3-args].u.string;
	    PIKE_MYSQL->password->refs++;
	  }
	}
      }
    }
  }

  pop_n_elems(args);

  pike_mysql_reconnect();
}

/* int affected_rows() */
static void f_affected_rows(INT32 args)
{
  if (!PIKE_MYSQL->socket) {
    pike_mysql_reconnect();
  }
  pop_n_elems(args);
  push_int(mysql_affected_rows(PIKE_MYSQL->socket));
}

/* int insert_id() */
static void f_insert_id(INT32 args)
{
  if (!PIKE_MYSQL->socket) {
    pike_mysql_reconnect();
  }
  pop_n_elems(args);
  push_int(mysql_insert_id(PIKE_MYSQL->socket));
}

/* int|string error() */
static void f_error(INT32 args)
{
  char *error_msg;

  if (!PIKE_MYSQL->socket) {
    pike_mysql_reconnect();
  }

  error_msg = mysql_error(PIKE_MYSQL->socket);

  pop_n_elems(args);

  if (error_msg && *error_msg) {
    push_text(error_msg);
  } else {
    push_int(0);
  }
}

/* void select_db(string database) */
static void f_select_db(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  char *database;
  int tmp = -1;

  if (!args) {
    error("Too few arguments to mysql->select_db()\n");
  }
  if (sp[-args].type != T_STRING) {
    error("Bad argument 1 to mysql->select_db()\n");
  }

  database = sp[-args].u.string->str;

  if (socket) {
    THREADS_ALLOW();

    tmp = mysql_select_db(socket, database);

    THREADS_DISALLOW();
  }
  if (!socket || (tmp < 0)) {
    /* The connection might have been closed. */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    THREADS_ALLOW();

    tmp = mysql_select_db(socket, database);

    THREADS_DISALLOW();
  }

  if (tmp < 0) {
    error("mysql->select_db(): Couldn't select database \"%s\" (%s)\n",
	  sp[-args].u.string->str, mysql_error(PIKE_MYSQL->socket));
  }
  if (PIKE_MYSQL->database) {
    free_string(PIKE_MYSQL->database);
  }
  PIKE_MYSQL->database = sp[-args].u.string;
  PIKE_MYSQL->database->refs++;

  pop_n_elems(args);
}

/* object(mysql_result) big_query(string q) */
static void f_big_query(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  MYSQL_RES *result;
  char *query;
  int qlen;
  int tmp = -1;

  if (!args) {
    error("Too few arguments to mysql->big_query()\n");
  }
  if (sp[-args].type != T_STRING) {
    error("Bad argument 1 to mysql->big_query()\n");
  }

  query = sp[-args].u.string->str;
  qlen = sp[-args].u.string->len;

  if (socket) {
    THREADS_ALLOW();

#ifdef HAVE_MYSQL_REAL_QUERY
    tmp = mysql_real_query(socket, query, qlen);
#else
    tmp = mysql_query(socket, query);
#endif /* HAVE_MYSQL_REAL_QUERY */

    THREADS_DISALLOW();
  }
  if (!socket || (tmp < 0)) {
    /* The connection might have been closed. */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    THREADS_ALLOW();

#ifdef HAVE_MYSQL_REAL_QUERY
    tmp = mysql_real_query(socket, query, qlen);
#else
    tmp = mysql_query(socket, query);
#endif /* HAVE_MYSQL_REAL_QUERY */

    THREADS_DISALLOW();
  }

  if (tmp < 0) {
    error("mysql->big_query(): Query \"%s\" failed (%s)\n",
	  sp[-args].u.string->str, mysql_error(PIKE_MYSQL->socket));
  }

  THREADS_ALLOW();

  /* The same thing applies here */

  result = mysql_store_result(socket);

  THREADS_DISALLOW();

  pop_n_elems(args);

  if (!(PIKE_MYSQL->last_result = result)) {
    if (mysql_num_fields(socket) && mysql_error(socket)[0]) {
      error("mysql->big_query(): Couldn't create result for query\n");
    }
    /* query was INSERT or similar - return 0 */

    push_int(0);
  } else {
    /* Return the result-object */

    push_object(fp->current_object);
    fp->current_object->refs++;

    push_object(clone_object(mysql_result_program, 1));
  }
}

/* void create_db(string database) */
static void f_create_db(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  char *database;
  int tmp = -1;

  if (!args) {
    error("Too few arguments to mysql->create_db()\n");
  }
  if (sp[-args].type != T_STRING) {
    error("Bad argument 1 to mysql->create_db()\n");
  }
  if (sp[-args].u.string->len > 127) {
    error("Database name \"%s\" is too long (max 127 characters)\n",
	  sp[-args].u.string->str);
  }
  database = sp[-args].u.string->str;

  if (socket) {
    THREADS_ALLOW();

    tmp = mysql_create_db(socket, database);

    THREADS_DISALLOW();
  }
  if (!socket || (tmp < 0)) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    THREADS_ALLOW();

    tmp = mysql_create_db(socket, database);

    THREADS_DISALLOW();
  }

  if (tmp < 0) {
    error("mysql->create_db(): Creation of database \"%s\" failed\n",
	  sp[-args].u.string->str);
  }

  pop_n_elems(args);
}

/* void drop_db(string database) */
static void f_drop_db(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  char *database;
  int tmp = -1;

  if (!args) {
    error("Too few arguments to mysql->drop_db()\n");
  }
  if (sp[-args].type != T_STRING) {
    error("Bad argument 1 to mysql->drop_db()\n");
  }
  if (sp[-args].u.string->len > 127) {
    error("Database name \"%s\" is too long (max 127 characters)\n",
	  sp[-args].u.string->str);
  }
  database = sp[-args].u.string->str;

  if (socket) {
    THREADS_ALLOW();

    tmp = mysql_drop_db(socket, database);

    THREADS_DISALLOW();
  }
  if (!socket || (tmp < 0)) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    THREADS_ALLOW();

    tmp = mysql_drop_db(socket, database);

    THREADS_DISALLOW();
  }    

  if (tmp < 0) {
    error("mysql->drop_db(): Drop of database \"%s\" failed\n",
	  sp[-args].u.string->str);
  }

  pop_n_elems(args);
}

/* void shutdown() */
static void f_shutdown(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  int tmp = -1;

  if (socket) {
    THREADS_ALLOW();
  
    tmp = mysql_shutdown(socket);

    THREADS_DISALLOW();
  }
  if (!socket || (tmp < 0)) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    THREADS_ALLOW();
  
    tmp = mysql_shutdown(socket);

    THREADS_DISALLOW();
  }

  if (tmp < 0) {
    error("mysql->shutdown(): Shutdown failed\n");
  }

  pop_n_elems(args);
}

/* void reload() */
static void f_reload(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  int tmp = -1;

  if (socket) {
    THREADS_ALLOW();

    tmp = mysql_reload(socket);

    THREADS_DISALLOW();
  }
  if (!socket || (tmp < 0)) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    THREADS_ALLOW();

    tmp = mysql_reload(socket);

    THREADS_DISALLOW();
  }

  if (tmp < 0) {
    error("mysql->reload(): Reload failed\n");
  }

  pop_n_elems(args);
}

/* string statistics() */
static void f_statistics(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  char *stats;

  if (!socket) {
    pike_mysql_reconnect();
    socket = PIKE_MYSQL->socket;
  }

  pop_n_elems(args);

  THREADS_ALLOW();

  stats = mysql_stat(socket);

  THREADS_DISALLOW();

  push_text(stats);
}

/* string server_info() */
static void f_server_info(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  char *info;

  if (!socket) {
    pike_mysql_reconnect();
    socket = PIKE_MYSQL->socket();
  }

  pop_n_elems(args);

  push_text("mysql/");

  THREADS_ALLOW();

  info = mysql_get_server_info(socket);

  THREADS_DISALLOW();

  push_text(info);
  f_add(2);
}

/* string host_info() */
static void f_host_info(INT32 args)
{
  if (!PIKE_MYSQL->socket) {
    pike_mysql_reconnect();
  }

  pop_n_elems(args);

  push_text(mysql_get_host_info(PIKE_MYSQL->socket));
}

/* int protocol_info() */
static void f_protocol_info(INT32 args)
{
  if (!PIKE_MYSQL->socket) {
    pike_mysql_reconnect();
  }

  pop_n_elems(args);

  push_int(mysql_get_proto_info(PIKE_MYSQL->socket));
}

/* object(mysql_res) list_dbs(void|string wild) */
static void f_list_dbs(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  MYSQL_RES *result;
  char *wild = NULL;

  if (args) {
    if (sp[-args].type != T_STRING) {
      error("Bad argument 1 to mysql->list_dbs()\n");
    }
    if (sp[-args].u.string->len > 80) {
      error("Wildcard \"%s\" is too long (max 80 characters)\n",
	    sp[-args].u.string->str);
    }
    wild = sp[-args].u.string->str;
  }

  if (socket) {
    THREADS_ALLOW();

    result = mysql_list_dbs(socket, wild);

    THREADS_DISALLOW();
  }
  if (!socket || !result) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    THREADS_ALLOW();

    result = mysql_list_dbs(socket, wild);

    THREADS_DISALLOW();
  }

  if (!(PIKE_MYSQL->last_result = result)) {
    error("mysql->list_dbs(): Cannot list databases: %s\n",
	  mysql_error(socket));
  }

  pop_n_elems(args);

  push_object(fp->current_object);
  fp->current_object->refs++;

  push_object(clone_object(mysql_result_program, 1));
}

/* object(mysql_res) list_tables(void|string wild) */
static void f_list_tables(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  MYSQL_RES *result;
  char *wild = NULL;

  if (args) {
    if (sp[-args].type != T_STRING) {
      error("Bad argument 1 to mysql->list_tables()\n");
    }
    if (sp[-args].u.string->len > 80) {
      error("Wildcard \"%s\" is too long (max 80 characters)\n",
	    sp[-args].u.string->str);
    }
    wild = sp[-args].u.string->str;
  }

  if (socket) {
    THREADS_ALLOW();

    result = mysql_list_tables(socket, wild);

    THREADS_DISALLOW();
  }
  if (!socket || !result) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    THREADS_ALLOW();

    result = mysql_list_tables(socket, wild);

    THREADS_DISALLOW();
  }

  if (!(PIKE_MYSQL->last_result = result)) {
    error("mysql->list_tables(): Cannot list databases: %s\n",
	  mysql_error(PIKE_MYSQL->socket));
  }

  pop_n_elems(args);

  push_object(fp->current_object);
  fp->current_object->refs++;

  push_object(clone_object(mysql_result_program, 1));
}

/* array(int|mapping(string:mixed)) list_fields(string table, void|string wild) */
static void f_list_fields(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  MYSQL_RES *result;
  MYSQL_FIELD *field;
  int i = 0;
  char *table;
  char *wild = NULL;

  if (!args) {
    error("Too few arguments to mysql->list_fields()\n");
  }
  
  if (sp[-args].type != T_STRING) {
    error("Bad argument 1 to mysql->list_fields()\n");
  }
  if (sp[-args].u.string->len > 125) {
    error("Table name \"%s\" is too long (max 125 characters)\n",
	  sp[-args].u.string->str);
  }
  table = sp[-args].u.string->str;
  if (args > 1) {
    if (sp[-args+1].type != T_STRING) {
      error("Bad argument 2 to mysql->list_fields()\n");
    }
    if (sp[-args+1].u.string->len + sp[-args].u.string->len > 125) {
      error("Wildcard \"%s\" + table name \"%s\" is too long "
	    "(max 125 characters)\n",
	    sp[-args+1].u.string->str, sp[-args].u.string->str);
    }
    wild = sp[-args+1].u.string->str;
  }

  if (socket) {
    THREADS_ALLOW();

    result = mysql_list_fields(socket, table, wild);

    THREADS_DISALLOW();
  }
  if (!socket || !result) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    THREADS_ALLOW();

    result = mysql_list_fields(socket, table, wild);

    THREADS_DISALLOW();
  }

  if (!result) {
    error("mysql->list_fields(): Cannot list databases: %s\n",
	  mysql_error(PIKE_MYSQL->socket));
  }

  pop_n_elems(args);

  while ((field = mysql_fetch_field(result))) {
    mysqlmod_parse_field(field, 1);
    i++;
  }
  f_aggregate(i);
}

/* object(mysql_res) list_processes() */
static void f_list_processes(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  MYSQL_RES *result;

  pop_n_elems(args);

  if (socket) {
    THREADS_ALLOW();

    result = mysql_list_processes(socket);

    THREADS_DISALLOW();
  }
  if (!socket || !result) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    THREADS_ALLOW();

    result = mysql_list_processes(socket);

    THREADS_DISALLOW();
  }

  if (!(PIKE_MYSQL->last_result = result)) {
    error("mysql->list_processes(): Cannot list databases: %s\n",
	  mysql_error(PIKE_MYSQL->socket));
  }

  push_object(fp->current_object);
  fp->current_object->refs++;

  push_object(clone_object(mysql_result_program, 1));
}

/*
 * Support for binary data in tables
 */
static void f_binary_data(INT32 args)
{
  pop_n_elems(args);
#ifdef HAVE_MYSQL_FETCH_LENGTHS
  push_int(1);
#else
  push_int(0);
#endif /* HAVE_MYSQL_FETCH_LENGTHS */
}

#endif /* HAVE_MYSQL */

/*
 * Module linkage
 */


void pike_module_init(void)
{
#ifdef HAVE_MYSQL
  /*
   * start_new_program();
   *
   * add_storage();
   *
   * add_function();
   * add_function();
   * ...
   *
   * set_init_callback();
   * set_exit_callback();
   *
   * program = end_c_program();
   * program->refs++;
   *
   */
 
  start_new_program();
  add_storage(sizeof(struct precompiled_mysql));

  add_function("error", f_error, "function(void:int|string)", ID_PUBLIC);
  add_function("create", f_create, "function(string|void, string|void, string|void, string|void:void)", ID_PUBLIC);
  add_function("affected_rows", f_affected_rows, "function(void:int)", ID_PUBLIC);
  add_function("insert_id", f_insert_id, "function(void:int)", ID_PUBLIC);
  add_function("select_db", f_select_db, "function(string:void)", ID_PUBLIC);
  add_function("big_query", f_big_query, "function(string:int|object)", ID_PUBLIC);
  add_function("create_db", f_create_db, "function(string:void)", ID_PUBLIC);
  add_function("drop_db", f_drop_db, "function(string:void)", ID_PUBLIC);
  add_function("shutdown", f_shutdown, "function(void:void)", ID_PUBLIC);
  add_function("reload", f_reload, "function(void:void)", ID_PUBLIC);
  add_function("statistics", f_statistics, "function(void:string)", ID_PUBLIC);
  add_function("server_info", f_server_info, "function(void:string)", ID_PUBLIC);
  add_function("host_info", f_host_info, "function(void:string)", ID_PUBLIC);
  add_function("protocol_info", f_protocol_info, "function(void:int)", ID_PUBLIC);
  add_function("list_dbs", f_list_dbs, "function(void|string:object)", ID_PUBLIC);
  add_function("list_tables", f_list_tables, "function(void|string:object)", ID_PUBLIC);
  add_function("list_fields", f_list_fields, "function(string, void|string:array(int|mapping(string:mixed)))", ID_PUBLIC);
  add_function("list_processes", f_list_processes, "function(void|string:object)", ID_PUBLIC);

  add_function("binary_data", f_binary_data, "function(void:int)", ID_PUBLIC);

  set_init_callback(init_mysql_struct);
  set_exit_callback(exit_mysql_struct);

  mysql_program = end_program();
  add_program_constant("mysql", mysql_program, 0);

  init_mysql_res_programs();
#endif /* HAVE_MYSQL */
}

void pike_module_exit(void)
{
#ifdef HAVE_MYSQL
  exit_mysql_res();

  if (mysql_program) {
    free_program(mysql_program);
    mysql_program = NULL;
  }
#endif /* HAVE_MYSQL */
}

