/*
 * $Id: mysql.c,v 1.15 1998/03/16 14:55:03 grubba Exp $
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

RCSID("$Id: mysql.c,v 1.15 1998/03/16 14:55:03 grubba Exp $");

struct program *mysql_program = NULL;

#ifdef HAVE_MYSQL_PORT
extern unsigned int mysql_port;
#ifdef _REENTRANT
static MUTEX_T stupid_port_lock;
#define STUPID_PORT_INIT()	mt_init(&stupid_port_lock)
#define STUPID_PORT_LOCK()	mt_lock(&stupid_port_lock)
#define STUPID_PORT_UNLOCK()	mt_unlock(&stupid_port_lock)
#define STUPID_PORT_DESTROY()	mt_destroy(&stupid_port_lock)
#else /* !_REENTRANT */
#define STUPID_PORT_INIT()
#define STUPID_PORT_LOCK()
#define STUPID_PORT_UNLOCK()
#define STUPID_PORT_DESTROY()
#endif /* _REENTRANT */
#endif /* HAVE_MYSQL_PORT */

#ifdef _REENTRANT
#define MYSQL_LOCK		(&(PIKE_MYSQL->lock))
#define INIT_MYSQL_LOCK()	mt_init(MYSQL_LOCK)
#define DESTROY_MYSQL_LOCK()	mt_destroy(MYSQL_LOCK)
#define MYSQL_ALLOW()		do { MUTEX_T *__l = MYSQL_LOCK; THREADS_ALLOW(); mt_lock(__l);
#define MYSQL_DISALLOW()	mt_unlock(__l); THREADS_DISALLOW(); } while(0)
#else /* !_REENTRANT */
#define INIT_MYSQL_LOCK()
#define DESTROY_MYSQL_LOCK()
#define MYSQL_ALLOW()
#define MYSQL_DISALLOW()
#endif /* _REENTRANT */


/*
 * Functions
 */

/*
 * State maintenance
 */

static void init_mysql_struct(struct object *o)
{
  MEMSET(PIKE_MYSQL, 0, sizeof(struct precompiled_mysql));
  INIT_MYSQL_LOCK();
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

  MYSQL_ALLOW();

  if (last_result) {
    mysql_free_result(last_result);
  }
  if (socket) {
    mysql_close(socket);
  }

  MYSQL_DISALLOW();

  DESTROY_MYSQL_LOCK();
}


static void pike_mysql_reconnect(void)
{
  MYSQL *mysql = &(PIKE_MYSQL->mysql);
  MYSQL *socket;
  char *host = NULL;
  char *database = NULL;
  char *user = NULL;
  char *password = NULL;
  char *portptr = NULL;
  unsigned int port = 0;
  unsigned int saved_port = 0;

  if (PIKE_MYSQL->host) {
    host = strdup(PIKE_MYSQL->host->str);
    if (!host) {
      error("Mysql.mysql(): Out of memory!\n");
    }
    if ((portptr = strchr(host, ':')) && (*portptr == ':')) {
      *portptr = 0;
      portptr++;
      port = (unsigned int) atoi(portptr);
    }
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

  MYSQL_ALLOW();

#ifdef HAVE_MYSQL_PORT
  STUPID_PORT_LOCK();
#endif /* HAVE_MYSQL_PORT */

  if (socket) {
    /* Disconnect the old connection */
    mysql_close(socket);
  }

#ifdef HAVE_MYSQL_PORT
  if (port) {
    saved_port = mysql_port;
    mysql_port = port;
  }
#endif /* HAVE_MYSQL_PORT */

  socket = mysql_connect(mysql, host, user, password);

#ifdef HAVE_MYSQL_PORT
  if (port) {
    mysql_port = saved_port;
  }

  STUPID_PORT_UNLOCK();
#endif /* HAVE_MYSQL_PORT */

  MYSQL_DISALLOW();

  if (host) {
    /* No longer needed */
    free(host);
  }
  
  if (!(PIKE_MYSQL->socket = socket)) {
    error("Mysql.mysql(): Couldn't reconnect to SQL-server\n"
	  "%s\n",
	  mysql_error(&PIKE_MYSQL->mysql));
  }
  if (database) {
    int tmp;

    MYSQL_ALLOW();

    tmp = mysql_select_db(socket, database);

    MYSQL_DISALLOW();

    if (tmp < 0) {
      PIKE_MYSQL->socket = NULL;
      MYSQL_ALLOW();

      mysql_close(socket);

      MYSQL_DISALLOW();
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
  MYSQL *socket;
  int count;

  if (!PIKE_MYSQL->socket) {
    pike_mysql_reconnect();
  }
  pop_n_elems(args);
  socket = PIKE_MYSQL->socket;

  MYSQL_ALLOW();
  count = mysql_affected_rows(socket);
  MYSQL_DISALLOW();

  push_int(count);
}

/* int insert_id() */
static void f_insert_id(INT32 args)
{
  MYSQL *socket;
  int id;

  if (!PIKE_MYSQL->socket) {
    pike_mysql_reconnect();
  }
  pop_n_elems(args);

  socket = PIKE_MYSQL->socket;

  MYSQL_ALLOW();
  id = mysql_insert_id(socket);
  MYSQL_DISALLOW();

  push_int(id);
}

/* int|string error() */
static void f_error(INT32 args)
{
  MYSQL *socket;
  char *error_msg;

  if (!PIKE_MYSQL->socket) {
    pike_mysql_reconnect();
  }

  socket = PIKE_MYSQL->socket;

  MYSQL_ALLOW();

  error_msg = mysql_error(socket);

  MYSQL_DISALLOW();

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
    MYSQL_ALLOW();

    tmp = mysql_select_db(socket, database);

    MYSQL_DISALLOW();
  }
  if (!socket || (tmp < 0)) {
    /* The connection might have been closed. */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();

    tmp = mysql_select_db(socket, database);

    MYSQL_DISALLOW();
  }

  if (tmp < 0) {
    char *err;

    MYSQL_ALLOW();
    err = mysql_error(socket);
    MYSQL_DISALLOW();

    error("mysql->select_db(): Couldn't select database \"%s\" (%s)\n",
	  sp[-args].u.string->str, err);
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
    MYSQL_ALLOW();

#ifdef HAVE_MYSQL_REAL_QUERY
    tmp = mysql_real_query(socket, query, qlen);
#else
    tmp = mysql_query(socket, query);
#endif /* HAVE_MYSQL_REAL_QUERY */

    MYSQL_DISALLOW();
  }
  if (!socket || (tmp < 0)) {
    /* The connection might have been closed. */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();

#ifdef HAVE_MYSQL_REAL_QUERY
    tmp = mysql_real_query(socket, query, qlen);
#else
    tmp = mysql_query(socket, query);
#endif /* HAVE_MYSQL_REAL_QUERY */

    MYSQL_DISALLOW();
  }

  if (tmp < 0) {
    char *err;

    MYSQL_ALLOW();
    err = mysql_error(socket);
    MYSQL_DISALLOW();

    if (sp[-args].u.string->len <= 512) {
      error("mysql->big_query(): Query \"%s\" failed (%s)\n",
	    sp[-args].u.string->str, err);
    } else {
      error("mysql->big_query(): Query failed (%s)\n", err);
    }
  }

  MYSQL_ALLOW();

  /* The same thing applies here */

  result = mysql_store_result(socket);

  MYSQL_DISALLOW();

  pop_n_elems(args);

  if (!(PIKE_MYSQL->last_result = result)) {
    int err;

    MYSQL_ALLOW();
    err = (mysql_num_fields(socket) && mysql_error(socket)[0]);
    MYSQL_DISALLOW();

    if (err) {
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
    MYSQL_ALLOW();

    tmp = mysql_create_db(socket, database);

    MYSQL_DISALLOW();
  }
  if (!socket || (tmp < 0)) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();

    tmp = mysql_create_db(socket, database);

    MYSQL_DISALLOW();
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
    MYSQL_ALLOW();

    tmp = mysql_drop_db(socket, database);

    MYSQL_DISALLOW();
  }
  if (!socket || (tmp < 0)) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();

    tmp = mysql_drop_db(socket, database);

    MYSQL_DISALLOW();
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
    MYSQL_ALLOW();
  
    tmp = mysql_shutdown(socket);

    MYSQL_DISALLOW();
  }
  if (!socket || (tmp < 0)) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();
  
    tmp = mysql_shutdown(socket);

    MYSQL_DISALLOW();
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
    MYSQL_ALLOW();

    tmp = mysql_reload(socket);

    MYSQL_DISALLOW();
  }
  if (!socket || (tmp < 0)) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();

    tmp = mysql_reload(socket);

    MYSQL_DISALLOW();
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

  MYSQL_ALLOW();

  stats = mysql_stat(socket);

  MYSQL_DISALLOW();

  push_text(stats);
}

/* string server_info() */
static void f_server_info(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  char *info;

  if (!socket) {
    pike_mysql_reconnect();
    socket = PIKE_MYSQL->socket;
  }

  pop_n_elems(args);

  push_text("mysql/");

  MYSQL_ALLOW();

  info = mysql_get_server_info(socket);

  MYSQL_DISALLOW();

  push_text(info);
  f_add(2);
}

/* string host_info() */
static void f_host_info(INT32 args)
{
  MYSQL *socket;
  char *info;

  if (!PIKE_MYSQL->socket) {
    pike_mysql_reconnect();
  }

  socket = PIKE_MYSQL->socket;

  pop_n_elems(args);

  MYSQL_ALLOW();

  info = mysql_get_host_info(socket);

  MYSQL_DISALLOW();

  push_text(info);
}

/* int protocol_info() */
static void f_protocol_info(INT32 args)
{
  MYSQL *socket;
  int prot;

  if (!PIKE_MYSQL->socket) {
    pike_mysql_reconnect();
  }

  pop_n_elems(args);

  socket = PIKE_MYSQL->socket;

  MYSQL_ALLOW();
  prot = mysql_get_proto_info(socket);
  MYSQL_DISALLOW();

  push_int(prot);
}

/* object(mysql_res) list_dbs(void|string wild) */
static void f_list_dbs(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  MYSQL_RES *result = NULL;
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
    MYSQL_ALLOW();

    result = mysql_list_dbs(socket, wild);

    MYSQL_DISALLOW();
  }
  if (!socket || !result) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();

    result = mysql_list_dbs(socket, wild);

    MYSQL_DISALLOW();
  }

  if (!(PIKE_MYSQL->last_result = result)) {
    char *err;

    MYSQL_ALLOW();

    err = mysql_error(socket);

    MYSQL_DISALLOW();

    error("mysql->list_dbs(): Cannot list databases: %s\n", err);
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
  MYSQL_RES *result = NULL;
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
    MYSQL_ALLOW();

    result = mysql_list_tables(socket, wild);

    MYSQL_DISALLOW();
  }
  if (!socket || !result) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();

    result = mysql_list_tables(socket, wild);

    MYSQL_DISALLOW();
  }

  if (!(PIKE_MYSQL->last_result = result)) {
    char *err;

    MYSQL_ALLOW();

    err =  mysql_error(socket);

    MYSQL_DISALLOW();

    error("mysql->list_tables(): Cannot list databases: %s\n", err);
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
  MYSQL_RES *result = NULL;
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
    MYSQL_ALLOW();

    result = mysql_list_fields(socket, table, wild);

    MYSQL_DISALLOW();
  }
  if (!socket || !result) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();

    result = mysql_list_fields(socket, table, wild);

    MYSQL_DISALLOW();
  }

  if (!result) {
    char *err;

    MYSQL_ALLOW();

    err = mysql_error(socket);

    MYSQL_DISALLOW();

    error("mysql->list_fields(): Cannot list databases: %s\n", err);
  }

  pop_n_elems(args);

  /* FIXME: Should have MYSQL_{DIS,}ALLOW() here */

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
  MYSQL_RES *result = NULL;

  pop_n_elems(args);

  if (socket) {
    MYSQL_ALLOW();

    result = mysql_list_processes(socket);

    MYSQL_DISALLOW();
  }
  if (!socket || !result) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();

    result = mysql_list_processes(socket);

    MYSQL_DISALLOW();
  }

  if (!(PIKE_MYSQL->last_result = result)) {
    char *err;

    MYSQL_ALLOW();

    err = mysql_error(socket);

    MYSQL_DISALLOW();

    error("mysql->list_processes(): Cannot list databases: %s\n", err);
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

#ifdef HAVE_MYSQL_PORT
  STUPID_PORT_INIT();
#endif /* HAVE_MYSQL_PORT */

  init_mysql_res_programs();
#endif /* HAVE_MYSQL */
}

void pike_module_exit(void)
{
#ifdef HAVE_MYSQL
  exit_mysql_res();

#ifdef HAVE_MYSQL_PORT
  STUPID_PORT_DESTROY();
#endif /* HAVE_MYSQL_PORT */

  if (mysql_program) {
    free_program(mysql_program);
    mysql_program = NULL;
  }
#endif /* HAVE_MYSQL */
}

