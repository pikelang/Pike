/*
 * $Id: mysql.c,v 1.3 1996/12/31 00:51:17 grubba Exp $
 *
 * SQL database functionality for Pike
 *
 * Henrik Grubbström 1996-12-21
 */

#ifdef HAVE_MYSQL

/*
 * Includes
 */

/* From the mysql-dist */
#ifndef MYSQL_MYSQL_H
#define MYSQL_MYSQL_H
#include <mysql.h>
#endif

/* dynamic_buffer.h contains a conflicting typedef for string
 * we don't use any dynamic buffers, so we have this work-around
 */
#define DYNAMIC_BUFFER_H
typedef struct dynamic_buffer_s dynamic_buffer;

#endif /* HAVE_MYSQL */

/* From the Pike-dist */
#include <global.h>
#include <svalue.h>
#include <object.h>
#include <stralloc.h>
#include <interpret.h>
#include <port.h>
#include <error.h>
#include <las.h>

/* Local includes */
#include "precompiled_mysql.h"

/* System includes */
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#ifdef HAVE_MYSQL

/*
 * Globals
 */

RCSID("$Id: mysql.c,v 1.3 1996/12/31 00:51:17 grubba Exp $");

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
  if (PIKE_MYSQL->last_result) {
    mysql_free_result(PIKE_MYSQL->last_result);
    PIKE_MYSQL->last_result = NULL;
  }
  if (PIKE_MYSQL->socket) {
    mysql_close(PIKE_MYSQL->socket);
    PIKE_MYSQL->socket = NULL;
  }
}

/*
 * Methods
 */

/* void create(string|void host, string|void database, string|void password) */
static void f_create(INT32 args)
{
  char *host = NULL;
  char *database = NULL;
  char *password = NULL;

  if (args >= 1) {
    if (sp[-args].type != T_STRING) {
      error("Bad argument 1 to mysql()\n");
    }
    if (sp[-args].u.string->len) {
      host = sp[-args].u.string->str;
    }

    if (args >= 2) {
      if (sp[1-args].type != T_STRING) {
	error("Bad argument 2 to mysql()\n");
      }
      if (sp[-args].u.string->len) {
	database = sp[1-args].u.string->str;
      }

      if (args >= 3) {
	if (sp[2-args].type != T_STRING) {
	  error("Bad argument 3 to mysql()\n");
	}
	if (sp[-args].u.string->len) {
	  password = sp[2-args].u.string->str;
	}
      }
    }
  }

  if (!(PIKE_MYSQL->socket = mysql_connect(&PIKE_MYSQL->mysql, host,
					   0, password))) {
    error("mysql(): Couldn't connect to SQL-server\n");
  }
  if (database && (mysql_select_db(PIKE_MYSQL->socket, database) < 0)) {
    mysql_close(PIKE_MYSQL->socket);
    PIKE_MYSQL->socket = NULL;
    error("mysql(): Couldn't select database \"%s\"\n", database);
  }

  pop_n_elems(args);
}

/* int affected_rows() */
static void f_affected_rows(INT32 args)
{
  pop_n_elems(args);
  push_int(mysql_affected_rows(PIKE_MYSQL->socket));
}

/* int insert_id() */
static void f_insert_id(INT32 args)
{
  pop_n_elems(args);
  push_int(mysql_insert_id(PIKE_MYSQL->socket));
}

/* string error() */
static void f_error(INT32 args)
{
  pop_n_elems(args);
  push_text(mysql_error(PIKE_MYSQL->socket));
}

/* void select_db(string database) */
static void f_select_db(INT32 args)
{
  if (!args) {
    error("Too few arguments to mysql->select_db()\n");
  }
  if (sp[-args].type != T_STRING) {
    error("Bad argument 1 to mysql->select_db()\n");
  }

  if (mysql_select_db(PIKE_MYSQL->socket, sp[-args].u.string->str) < 0) {
    error("mysql->select_db(): Couldn't select database \"%s\"\n",
	  sp[-args].u.string->str);
  }

  pop_n_elems(args);
}

/* object(mysql_result) query(string q) */
static void f_query(INT32 args)
{
  if (!args) {
    error("Too few arguments to mysql->query()\n");
  }
  if (sp[-args].type != T_STRING) {
    error("Bad argument 1 to mysql->query()\n");
  }
  if (mysql_query(PIKE_MYSQL->socket, sp[-args].u.string->str) < 0) {
    error("mysql->query(): Query \"%s\" failed\n",
	  sp[-args].u.string->str);
  }

  if (!(PIKE_MYSQL->last_result = mysql_store_result(PIKE_MYSQL->socket))) {
    error("mysql->query(): Couldn't create result for query\n");
  }

  pop_n_elems(args);

  push_object(fp->current_object);
  fp->current_object->refs++;

  push_object(clone(mysql_result_program, 1));
}

/* void create_db(string database) */
static void f_create_db(INT32 args)
{
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
  if (mysql_create_db(PIKE_MYSQL->socket, sp[-args].u.string->str) < 0) {
    error("mysql->create_db(): Creation of database \"%s\" failed\n",
	  sp[-args].u.string->str);
  }

  pop_n_elems(args);
}

/* void drop_db(string database) */
static void f_drop_db(INT32 args)
{
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
  if (mysql_drop_db(PIKE_MYSQL->socket, sp[-args].u.string->str) < 0) {
    error("mysql->drop_db(): Drop of database \"%s\" failed\n",
	  sp[-args].u.string->str);
  }

  pop_n_elems(args);
}

/* void shutdown() */
static void f_shutdown(INT32 args)
{
  if (mysql_shutdown(PIKE_MYSQL->socket) < 0) {
    error("mysql->shutdown(): Shutdown failed\n");
  }

  pop_n_elems(args);
}

/* void reload() */
static void f_reload(INT32 args)
{
  if (mysql_reload(PIKE_MYSQL->socket) < 0) {
    error("mysql->reload(): Reload failed\n");
  }

  pop_n_elems(args);
}

/* string statistics() */
static void f_statistics(INT32 args)
{
  pop_n_elems(args);

  push_text(mysql_stat(PIKE_MYSQL->socket));
}

/* string server_info() */
static void f_server_info(INT32 args)
{
  pop_n_elems(args);

  push_text(mysql_get_server_info(PIKE_MYSQL->socket));
}

/* string host_info() */
static void f_host_info(INT32 args)
{
  pop_n_elems(args);

  push_text(mysql_get_host_info(PIKE_MYSQL->socket));
}

/* int protocol_info() */
static void f_protocol_info(INT32 args)
{
  pop_n_elems(args);

  push_int(mysql_get_proto_info(PIKE_MYSQL->socket));
}

/* object(mysql_res) list_dbs(void|string wild) */
static void f_list_dbs(INT32 args)
{
  char *wild = "*";

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
  if (!(PIKE_MYSQL->last_result = mysql_list_dbs(PIKE_MYSQL->socket, wild))) {
    error("mysql->list_dbs(): Cannot list databases: %s\n",
	  mysql_error(PIKE_MYSQL->socket));
  }

  pop_n_elems(args);

  push_object(fp->current_object);
  fp->current_object->refs++;

  push_object(clone(mysql_result_program, 1));
}

/* object(mysql_res) list_tables(void|string wild) */
static void f_list_tables(INT32 args)
{
  char *wild = "*";

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
  PIKE_MYSQL->last_result = mysql_list_tables(PIKE_MYSQL->socket, wild);

  pop_n_elems(args);

  push_object(fp->current_object);
  fp->current_object->refs++;

  push_object(clone(mysql_result_program, 1));
}

/* object(mysql_res) list_fields(string table, void|string wild) */
static void f_list_fields(INT32 args)
{
  char *table;
  char *wild = "*";

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
    if (sp[-args+1].u.string->len + sp[-args].u.string > 125) {
      error("Wildcard \"%s\" + table name \"%s\" is too long "
	    "(max 125 characters)\n",
	    sp[-args+1].u.string->str, sp[-args].u.string->str);
    }
    wild = sp[-args+1].u.string->str;
  }
  PIKE_MYSQL->last_result = mysql_list_fields(PIKE_MYSQL->socket, table, wild);

  pop_n_elems(args);

  push_object(fp->current_object);
  fp->current_object->refs++;

  push_object(clone(mysql_result_program, 1));
}

/* object(mysql_res) list_processes() */
static void f_list_processes(INT32 args)
{
  pop_n_elems(args);

  PIKE_MYSQL->last_result = mysql_list_processes(PIKE_MYSQL->socket);

  push_object(fp->current_object);
  fp->current_object->refs++;

  push_object(clone(mysql_result_program, 1));
}

#endif /* HAVE_MYSQL */

/*
 * Module linkage
 */

void init_mysql_efuns(void)
{
#ifdef HAVE_MYSQL
  init_mysql_res_efuns();
#endif /* HAVE_MYSQL */
}

void init_mysql_programs(void)
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

  add_function("error", f_error, "function(void:string)", OPT_EXTERNAL_DEPEND);
  add_function("create", f_create, "function(string|void, string|void, string|void:void)", OPT_SIDE_EFFECT);
  add_function("affected_rows", f_affected_rows, "function(void:int)", OPT_EXTERNAL_DEPEND);
  add_function("insert_id", f_insert_id, "function(void:int)", OPT_EXTERNAL_DEPEND);
  add_function("select_db", f_select_db, "function(string:void)", OPT_SIDE_EFFECT);
  add_function("query", f_query, "function(string:object)", OPT_EXTERNAL_DEPEND);
  add_function("create_db", f_create_db, "function(string:void)", OPT_SIDE_EFFECT);
  add_function("drop_db", f_drop_db, "function(string:void)", OPT_SIDE_EFFECT);
  add_function("shutdown", f_shutdown, "function(void:void)", OPT_SIDE_EFFECT);
  add_function("reload", f_reload, "function(void:void)", OPT_SIDE_EFFECT);
  add_function("statistics", f_statistics, "function(void:string)", OPT_EXTERNAL_DEPEND);
  add_function("server_info", f_server_info, "function(void:string)", OPT_EXTERNAL_DEPEND);
  add_function("host_info", f_host_info, "function(void:string)", OPT_EXTERNAL_DEPEND);
  add_function("protocol_info", f_protocol_info, "function(void:int)", OPT_EXTERNAL_DEPEND);
  add_function("list_dbs", f_list_dbs, "function(void|string:object)", OPT_EXTERNAL_DEPEND);
  add_function("list_tables", f_list_tables, "function(void|string:object)", OPT_EXTERNAL_DEPEND);
  add_function("list_fields", f_list_fields, "function(string, void|string:object)", OPT_EXTERNAL_DEPEND);
  add_function("list_processes", f_list_processes, "function(void|string:object)", OPT_EXTERNAL_DEPEND);

  set_init_callback(init_mysql_struct);
  set_exit_callback(exit_mysql_struct);

  mysql_program = end_c_program("/precompiled/mysql");
  mysql_program->refs++;

  init_mysql_res_programs();
#endif /* HAVE_MYSQL */
}

void exit_mysql(void)
{
#ifdef HAVE_MYSQL
  exit_mysql_res();

  if (mysql_program) {
    free_program(mysql_program);
    mysql_program = NULL;
  }
#endif /* HAVE_MYSQL */
}

