/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: mysql.c,v 1.96 2005/11/16 16:20:19 grubba Exp $
*/

/*
 * SQL database functionality for Pike
 *
 * Henrik Grubbström 1996-12-21
 */

/* Master Pike headerfile */
#include "global.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_MYSQL

/*
 * Includes
 */

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#elif defined(HAVE_WINSOCK_H)
#include <winsock.h>
#endif

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
#ifndef DISABLE_BINARY
#error Need mysql.h header-file
#endif
#endif /* HAVE_MYSQL_MYSQL_H */
#endif /* HAVE_MYSQL_H */
#ifndef _mysql_h
#define _mysql_h
#endif
#endif

#ifdef HAVE_ERRMSG_H
#include <errmsg.h>
#else /* !HAVE_ERRMSG_H */
#ifdef HAVE_MYSQL_ERRMSG_H
#include <mysql/errmsg.h>
#endif /* HAVE_MYSQL_ERRMSG_H */
#endif /* HAVE_ERRMGS_H */


#endif /* HAVE_MYSQL */

/* From the Pike-dist */
#include "module.h"
#include "svalue.h"
#include "object.h"
#include "stralloc.h"
#include "interpret.h"
#include "port.h"
#include "pike_error.h"
#include "threads.h"
#include "program.h"
#include "operators.h"
#include "builtin_functions.h"
#include "fd_control.h"
#include "mapping.h"
#include "bignum.h"
#include "module_support.h"

/* System includes */
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#define sp Pike_sp

#ifdef HAVE_MYSQL

/* Local includes */
#include "precompiled_mysql.h"

/*
 * Globals
 */

/*! @module Mysql
 *!
 *! This module enables access to the Mysql database from within Pike.
 *! Use @[Sql.Sql] for general database access.
 *!
 *! Mysql is available from @url{http://www.mysql.com@}.
 *!
 *! @seealso
 *!  @[Mysql.mysql], @[Mysql.mysql_result], @[Sql.Sql]
 */

/*! @class mysql
 *!
 *! Interface to the Mysql database.
 *!
 *! This class enables access to the Mysql database from within Pike.
 *!
 *! Mysql is available from @url{http://www.mysql.com@}.
 *!
 *! @seealso
 *!   @[Mysql.mysql_result], @[Sql.Sql]
 */

struct program *mysql_program = NULL;

#if defined(HAVE_MYSQL_PORT) || defined(HAVE_MYSQL_UNIX_PORT)
#ifdef HAVE_MYSQL_PORT
extern unsigned int mysql_port;
#endif /* HAVE_MYSQL_PORT */
#ifdef HAVE_MYSQL_UNIX_PORT
extern char *mysql_unix_port;
#endif /* HAVE_MYSQL_UNIX_PORT */
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

#define CHECK_8BIT_NONBINARY_STRING(FUNC, ARG) do {			\
    if (sp[ARG-1-args].type != T_STRING ||				\
	sp[ARG-1-args].u.string->size_shift ||				\
        string_has_null(sp[ARG-1-args].u.string))			\
      SIMPLE_BAD_ARG_ERROR (FUNC, ARG, "string (nonbinary 8bit)");	\
  } while (0)

#define CHECK_8BIT_STRING(FUNC, ARG) do {				\
    if (sp[ARG-1-args].type != T_STRING ||				\
	sp[ARG-1-args].u.string->size_shift)				\
      SIMPLE_BAD_ARG_ERROR (FUNC, ARG, "string (8bit)");	\
  } while (0)


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
  PIKE_MYSQL->mysql = (MYSQL *)xalloc(sizeof(MYSQL));
#if defined(HAVE_MYSQL_REAL_CONNECT)
  mysql_init(PIKE_MYSQL->mysql);
#endif /* HAVE_MYSQL_REAL_CONNECT */
}

static void exit_mysql_struct(struct object *o)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  MYSQL *mysql = PIKE_MYSQL->mysql;

  PIKE_MYSQL->socket = NULL;
  PIKE_MYSQL->mysql = NULL;

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

  if (socket) {
    mysql_close(socket);
  }
  if (mysql) {
    free(mysql);
  }

  MYSQL_DISALLOW();

  DESTROY_MYSQL_LOCK();
}
void pike_mysql_set_ssl(struct mapping *options) {

#ifdef HAVE_MYSQL_SSL
    char *ssl_key = NULL;
    char *ssl_cert = NULL;
    char *ssl_ca = NULL;
    char *ssl_capath = NULL;
    char *ssl_cipher = NULL;
    struct svalue *val = NULL;

    if ((val = simple_mapping_string_lookup(options, "ssl_key")) &&
	(val->type == T_STRING) &&
	(!val->u.string->size_shift))
      ssl_key = val->u.string->str;

    if ((val = simple_mapping_string_lookup(options, "ssl_cert")) &&
	(val->type == T_STRING) &&
	(!val->u.string->size_shift))
      ssl_cert = val->u.string->str;

    if ((val = simple_mapping_string_lookup(options, "ssl_ca")) &&
	(val->type == T_STRING) &&
	(!val->u.string->size_shift))
      ssl_ca = val->u.string->str;

    if ((val = simple_mapping_string_lookup(options, "ssl_capath")) &&
	(val->type == T_STRING) &&
	(!val->u.string->size_shift))
      ssl_capath = val->u.string->str;

    if ((val = simple_mapping_string_lookup(options, "ssl_cipher")) &&
	(val->type == T_STRING) &&
	(!val->u.string->size_shift))
      ssl_cipher = val->u.string->str;

    if (ssl_key || ssl_cert || ssl_ca || ssl_capath || ssl_cipher)
      mysql_ssl_set(PIKE_MYSQL->mysql,
		    ssl_key,
		    ssl_cert,
		    ssl_ca,
		    ssl_capath,
		    ssl_cipher);
#endif /* HAVE_MYSQL_SSL */
}

static void pike_mysql_set_options(struct mapping *options)
{
  struct svalue *val;

#ifdef HAVE_MYSQL_OPTIONS
#ifdef HAVE_MYSQL_READ_DEFAULT_FILE
  if ((val = simple_mapping_string_lookup(options, "mysql_config_file")) &&
      (val->type == T_STRING) && (!val->u.string->size_shift)) {
    mysql_options(PIKE_MYSQL->mysql, MYSQL_READ_DEFAULT_FILE,
		  val->u.string->str);
  }
#endif /* MYSQL_READ_DEFAULT_FILE */
#ifdef HAVE_MYSQL_READ_DEFAULT_GROUP
  if ((val = simple_mapping_string_lookup(options, "mysql_group")) &&
      (val->type == T_STRING) && (!val->u.string->size_shift)) {
    mysql_options(PIKE_MYSQL->mysql, MYSQL_READ_DEFAULT_GROUP,
		  val->u.string->str);
  }
#endif /* MYSQL_READ_DEFAULT_GROUP */
#ifdef HAVE_MYSQL_INIT_COMMAND
  if ((val = simple_mapping_string_lookup(options, "init_command")) &&
      (val->type == T_STRING) && (!val->u.string->size_shift)) {
    mysql_options(PIKE_MYSQL->mysql, MYSQL_INIT_COMMAND,
		  val->u.string->str);
  }
#endif /* MYSQL_INIT_COMMAND */
#ifdef HAVE_MYSQL_OPT_NAMED_PIPE
  if ((val = simple_mapping_string_lookup(options, "mysql_named_pipe")) &&
      (val->type == T_INT) && (val->u.integer)) {
    mysql_options(PIKE_MYSQL->mysql, MYSQL_OPT_NAMED_PIPE, NULL);
  }
#endif /* MYSQL_OPT_NAMED_PIPE */
#ifdef HAVE_MYSQL_OPT_CONNECT_TIMEOUT
  if ((val = simple_mapping_string_lookup(options, "timeout")) &&
      (val->type == T_INT)) {
    unsigned int timeout = (unsigned int)val->u.integer;
    mysql_options(PIKE_MYSQL->mysql, MYSQL_OPT_CONNECT_TIMEOUT,
		  (char *)&timeout);
  }
#endif /* MYSQL_OPT_CONNECT_TIMEOUT */
#ifdef HAVE_MYSQL_OPT_COMPRESS
  if ((val = simple_mapping_string_lookup(options, "compress")) &&
      (val->type == T_INT) && (val->u.integer)) {
    mysql_options(PIKE_MYSQL->mysql, MYSQL_OPT_COMPRESS, NULL);
  }
#endif /* MYSQL_OPT_COMPRESS */
#ifdef HAVE_MYSQL_OPT_LOCAL_INFILE
  if ((val = simple_mapping_string_lookup(options, "mysql_local_infile")) &&
      (val->type == T_INT)) {
    unsigned int allowed = (unsigned int)val->u.integer;
    mysql_options(PIKE_MYSQL->mysql, MYSQL_OPT_LOCAL_INFILE,
		  (char *)&allowed);
  } else {
    /* Default to not allowed */
    unsigned int allowed = 0;
    mysql_options(PIKE_MYSQL->mysql, MYSQL_OPT_LOCAL_INFILE,
		  (char *)&allowed);    
  }
#endif /* MYSQL_OPT_LOCAL_INFILE */
#ifdef HAVE_MYSQL_SET_CHARSET_DIR
  if ((val = simple_mapping_string_lookup(options, "mysql_charset_dir")) &&
      (val->type == T_STRING) && (!val->u.string->size_shift)) {
    mysql_options(PIKE_MYSQL->mysql, MYSQL_SET_CHARSET_DIR,
		  val->u.string->str);
  }
#endif /* MYSQL_SET_CHARSET_DIR */
#ifdef HAVE_MYSQL_SET_CHARSET_NAME
  if ((val = simple_mapping_string_lookup(options, "mysql_charset_name")) &&
      (val->type == T_STRING) && (!val->u.string->size_shift)) {
    mysql_options(PIKE_MYSQL->mysql, MYSQL_SET_CHARSET_NAME,
		  val->u.string->str);
  }
#endif /* MYSQL_SET_CHARSET_NAME */
#endif /* HAVE_MYSQL_OPTIONS */
}

static void pike_mysql_reconnect(void)
{
  MYSQL *mysql = PIKE_MYSQL->mysql;
  MYSQL *socket;
  char *host = NULL;
  char *database = NULL;
  char *user = NULL;
  char *password = NULL;
  char *hostptr = NULL;
  char *portptr = NULL;
  char *saved_unix_port = NULL;
  unsigned int port = 0;
  unsigned int saved_port = 0;
  unsigned int options = 0;
  struct svalue *val;

  if (PIKE_MYSQL->host) {
    hostptr = strdup(PIKE_MYSQL->host->str);
    if (!hostptr) {
      Pike_error("Mysql.mysql(): Out of memory!\n");
    }
    if ((portptr = strchr(hostptr, ':')) && (*portptr == ':')) {
      *portptr = 0;
      portptr++;
      port = (unsigned int) atoi(portptr);
    }
    if (*hostptr) {
      host = hostptr;
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

  if ((val = simple_mapping_string_lookup(PIKE_MYSQL->options,
					  "connect_options")) &&
      (val->type == T_INT) && (val->u.integer)) {
    options = (unsigned int)val->u.integer;
  }

  MYSQL_ALLOW();

#if defined(HAVE_MYSQL_PORT) || defined(HAVE_MYSQL_UNIX_PORT)
  STUPID_PORT_LOCK();
#endif /* HAVE_MYSQL_PORT || HAVE_MYSQL_UNIX_PORT */

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
#ifdef HAVE_MYSQL_UNIX_PORT
  if (portptr) {
    saved_unix_port = mysql_unix_port;
    mysql_unix_port = portptr;
  }
#endif /* HAVE_MYSQL_UNIX_PORT */

#ifdef HAVE_MYSQL_REAL_CONNECT
  socket = mysql_real_connect(mysql, host, user, password,
                              NULL, port, portptr, options);
#else
  socket = mysql_connect(mysql, host, user, password);
#endif /* HAVE_MYSQL_REAL_CONNECT */

#ifdef HAVE_MYSQL_PORT
  if (port) {
    mysql_port = saved_port;
  }
#endif /* HAVE_MYSQL_PORT */
#ifdef HAVE_MYSQL_UNIX_PORT
  if (portptr) {
    mysql_unix_port = saved_unix_port;
  }
#endif /* HAVE_MYSQL_UNIX_PORT */
#if defined(HAVE_MYSQL_PORT) || defined(HAVE_MYSQL_UNIX_PORT)
  STUPID_PORT_UNLOCK();
#endif /* HAVE_MYSQL_PORT || MAVE_MYSQL_UNIX_PORT*/

  MYSQL_DISALLOW();

  if (hostptr) {
    /* No longer needed */
    free(hostptr);
  }
  
  if (!(PIKE_MYSQL->socket = socket)) {
    Pike_error("Mysql.mysql(): Couldn't reconnect to SQL-server\n"
	  "%s\n",
	  mysql_error(PIKE_MYSQL->mysql));
  }

  if (socket->net.fd >= 0) {
    /* Make sure the fd gets closed on exec. */
    set_close_on_exec(socket->net.fd, 1);
  }

  if (database) {
    int tmp;

    MYSQL_ALLOW();

    tmp = mysql_select_db(socket, database);

    MYSQL_DISALLOW();

    if (tmp) {
      PIKE_MYSQL->socket = NULL;
      MYSQL_ALLOW();

      mysql_close(socket);

      MYSQL_DISALLOW();
      if (strlen(database) < 1024) {
	Pike_error("Mysql.mysql(): Couldn't select database \"%s\"\n", database);
      } else {
	Pike_error("Mysql.mysql(): Couldn't select database\n");
      }
    }
  }
}

/*
 * Methods
 */

/*! @decl void create()
 *! @decl void create(string host)
 *! @decl void create(string host, string database)
 *! @decl void create(string host, string database, string user)
 *! @decl void create(string host, string database, string user, @
 *!                   string password)
 *! @decl void create(string host, string database, string user, @
 *!                   string password, mapping(string:string|int) options)
 *!
 *! Connect to a Mysql database.
 *!
 *! To access the Mysql database, you must first connect to it. This is
 *! done with this function.
 *!
 *! @param host
 *!   If you give no argument, or give @expr{""@} as @[host] it will connect
 *!   with a UNIX-domain socket, which can be a big performance gain.
 *!
 *! @param options
 *!   This optional mapping can contain zero or more of the following
 *!   parameters:
 *!
 *!   @mapping
 *!     @member string "init_command"
 *!       Command to execute on connect.
 *!
 *!     @member int "timeout"
 *!       Timeout in seconds.
 *!
 *!     @member int(0..1) "compress"
 *!       Enable compressed protocol.
 *!
 *!     @member string "mysql_config_file"
 *!       Change config file from @expr{"my.cnf"@}.
 *!
 *!     @member string "mysql_group"
 *!       Specify additional group to read from config file.
 *!
 *!     @member int(0..1) "mysql_named_pipe"
 *!       Use named pipe to connect to server.
 *!
 *!     @member int(0..1) "mysql_local_infile"
 *!       Enable use of LOCAL INFILE (security).
 *!
 *!     @member string "mysql_charset_dir"
 *!       Change charset directory.
 *!
 *!     @member string "mysql_charset_name"
 *!       Change charset name.
 *!
 *!     @member string "ssl_key"
 *!       Path to SSL-key for use in SSL-communication.
 *!
 *!     @member string "ssl_cert"
 *!       Path to SSL-cert for use in SSL-communication.
 *!
 *!     @member string "ssl_ca"
 *!       Path to SSL-CA for use in SSL-communication.
 *!
 *!     @member string "ssl_capath"
 *!       Path to SSL-CAPATH for use in SSL-communication.
 *!
 *!     @member string "ssl_cipher"
 *!       FIXME
 *!
 *!     @member int "connect_options"
 *!       Options used when connecting to the server. See mysql documentation
 *!       for more information.
 *!
 *!   @endmapping
 *!
 *! @note
 *!   Some options may not be implemented. Unimplemented options are
 *!   silently ignored.
 *!
 *!   To use SSL-connections, set the SSL-parameters correctly. They corespond
 *!   to the parameters given to the mysql-client with the same name so make
 *!   sure that the mysql-client works with SSL and set these parameters to
 *!   the same values and everything should work. If SSL-options are loaded
 *!   from a config-file, one may set the connect_options to include
 *!   CLIENT_SSL.
 */
static void f_create(INT32 args)
{
#if defined(HAVE_MYSQL_OPTIONS) && defined(HAVE_MYSQL_OPT_LOCAL_INFILE)
  /* Default to not allowed */
  unsigned int allowed = 0;
  mysql_options(PIKE_MYSQL->mysql, MYSQL_OPT_LOCAL_INFILE,
		(char *)&allowed);    
#endif /* HAVE_MYSQL_OPTIONS && HAVE_MYSQL_OPT_LOCAL_INFILE */

  if (args >= 1) {
    CHECK_8BIT_NONBINARY_STRING ("Mysql.mysql->create", 1);
    if (sp[-args].u.string->len) {
      add_ref(PIKE_MYSQL->host = sp[-args].u.string);
    }

    if (args >= 2) {
      CHECK_8BIT_NONBINARY_STRING ("Mysql.mysql->create", 2);
      if (sp[1-args].u.string->len) {
	add_ref(PIKE_MYSQL->database = sp[1-args].u.string);
      }
      
      if (args >= 3) {
	CHECK_8BIT_NONBINARY_STRING ("Mysql.mysql->create", 3);
	if (sp[2-args].u.string->len) {
	  add_ref(PIKE_MYSQL->user = sp[2-args].u.string);
	}
	
	if (args >= 4) {
	  CHECK_8BIT_NONBINARY_STRING ("Mysql.mysql->create", 4);
	  if (sp[3-args].u.string->len) {
	    add_ref(PIKE_MYSQL->password = sp[3-args].u.string);
	  }
	  if (args >= 5) {
	    if (sp[4-args].type != T_MAPPING) {
	      SIMPLE_BAD_ARG_ERROR ("Mysql.mysql->create", 5, "mapping(string:mixed)");
	    }
	    add_ref(PIKE_MYSQL->options = sp[4-args].u.mapping);
	    pike_mysql_set_options(sp[4-args].u.mapping);
	  }
	}
      }
    }
  }

  pop_n_elems(args);

  pike_mysql_set_ssl(PIKE_MYSQL->options);

  pike_mysql_reconnect();
}

/*! @decl string _sprintf(int type, void|mapping flags)
 */
static void mysql__sprintf(INT32 args)
{
  INT_TYPE type;
  struct mapping * flags;

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("_sprintf",2);
  if(sp[-args].type!=T_INT)
    SIMPLE_BAD_ARG_ERROR("_sprintf",0,"integer");

  type = sp[-args].u.integer;
  pop_n_elems(args);
  switch( type )
  {
    case 'O':
    {
      struct pike_string *res;
      MYSQL *socket;
      const char *info;

      if(!PIKE_MYSQL->socket)
	pike_mysql_reconnect();
      socket = PIKE_MYSQL->socket;

      MYSQL_ALLOW();
      info = mysql_get_host_info(socket);
      MYSQL_DISALLOW();

      push_text("mysql(/* %s %s*/)");
      push_text(info);
#ifdef HAVE_MYSQL_SSL
      if (PIKE_MYSQL->mysql->options.use_ssl) {
	push_text("using SSL");
      }
      else
	push_empty_string();
#else
      push_empty_string();
#endif /* HAVE_MYSQL_SSL */

      f_sprintf(3);

      res = Pike_sp[-1].u.string;
      Pike_sp--;
      push_string(res);
      return;
    }

    case 't':
    {
      struct pike_string * res = make_shared_binary_string("mysql", 5);
      push_string(res);
      return;
    }
  }
  push_int( 0 );
  Pike_sp[-1].subtype = 1;
}

/*! @decl int affected_rows()
 *!
 *! Returns the number of rows affected by the last query.
 */
static void f_affected_rows(INT32 args)
{
  MYSQL *socket;
  INT64 count;

  if (!PIKE_MYSQL->socket) {
    pike_mysql_reconnect();
  }
  pop_n_elems(args);
  socket = PIKE_MYSQL->socket;

  MYSQL_ALLOW();
  count = mysql_affected_rows(socket);
  MYSQL_DISALLOW();

  push_int64(count);
}

/*! @decl int insert_id()
 *!
 *! Returns the id of the last INSERT query into a table with
 *! an AUTO INCREMENT field.
 */
static void f_insert_id(INT32 args)
{
  MYSQL *socket;
  INT64 id;

  if (!PIKE_MYSQL->socket) {
    pike_mysql_reconnect();
  }
  pop_n_elems(args);

  socket = PIKE_MYSQL->socket;

  MYSQL_ALLOW();
  id = mysql_insert_id(socket);
  MYSQL_DISALLOW();

  push_int64(id);
}

/*! @decl string error()
 *!
 *! Returns a string describing the last error from the Mysql-server.
 *!
 *! Returns @expr{0@} (zero) if there was no error.
 */
static void f_error(INT32 args)
{
  MYSQL *socket;
  const char *error_msg;

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

/*! @decl void select_db(string database)
 *!
 *! Select database.
 *!
 *! The Mysql-server can hold several databases. You select which one
 *! you want to access with this function.
 *!
 *! @seealso
 *!   @[create()], @[create_db()], @[drop_db()]
 */
static void f_select_db(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  char *database;
  int tmp = -1;

  if (!args) {
    SIMPLE_TOO_FEW_ARGS_ERROR ("Mysql.mysql->select_db", 1);
  }
  CHECK_8BIT_NONBINARY_STRING ("Mysql.mysql->select_db", 1);

  database = sp[-args].u.string->str;

  if (socket) {
    MYSQL_ALLOW();

    tmp = mysql_select_db(socket, database);

    MYSQL_DISALLOW();
  }
  if (!socket || tmp) {
    /* The connection might have been closed. */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();

    tmp = mysql_select_db(socket, database);

    MYSQL_DISALLOW();
  }

  if (tmp) {
    const char *err;

    MYSQL_ALLOW();
    err = mysql_error(socket);
    MYSQL_DISALLOW();

    Pike_error("Mysql.mysql->select_db(): Couldn't select database \"%s\" (%s)\n",
	  sp[-args].u.string->str, err);
  }
  if (PIKE_MYSQL->database) {
    free_string(PIKE_MYSQL->database);
  }
  add_ref(PIKE_MYSQL->database = sp[-args].u.string);

  pop_n_elems(args);
}

#ifndef STDCALL
#define STDCALL
#endif

static void low_query(INT32 args, char *name,
		      MYSQL_RES* STDCALL (*mysql_func)(MYSQL*))
{
  MYSQL *socket = PIKE_MYSQL->socket;
  MYSQL_RES *result = NULL;
  char *query;
  int qlen;
  int tmp = -1;

  if (!args) {
    SIMPLE_TOO_FEW_ARGS_ERROR (name, 1);
  }
#ifdef HAVE_MYSQL_REAL_QUERY
  CHECK_8BIT_STRING (name, 1);
#else
  CHECK_8BIT_NONBINARY_STRING (name, 1);
#endif

  query = sp[-args].u.string->str;
  qlen = sp[-args].u.string->len;

  if (socket) {
    MYSQL_ALLOW();

#ifdef HAVE_MYSQL_REAL_QUERY
    tmp = mysql_real_query(socket, query, qlen);
#else
    tmp = mysql_query(socket, query);
#endif /* HAVE_MYSQL_REAL_QUERY */

    if (!tmp) {
      result = mysql_func(socket);
    }

    MYSQL_DISALLOW();
  }
  if (socket && tmp) {
    /* Check if we need to reconnect. */
#if defined(CR_SERVER_GONE_ERROR) && defined(CR_UNKNOWN_ERROR)
    int eno = mysql_errno(socket);
    if ((eno == CR_SERVER_GONE_ERROR) ||
	(eno == CR_UNKNOWN_ERROR)) {
      socket = NULL;
    }
#else /* !CR_SERVER_GONE_ERROR || !CR_UNKNOWN_ERROR */
    socket = NULL;
#endif /* CR_SERVER_GONE_ERROR && CR_UNKNOWN_ERROR */
  }
  if (!socket) {
    /* The connection might have been closed. */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();

#ifdef HAVE_MYSQL_REAL_QUERY
    tmp = mysql_real_query(socket, query, qlen);
#else
    tmp = mysql_query(socket, query);
#endif /* HAVE_MYSQL_REAL_QUERY */

    if (!tmp) {
      result = mysql_func(socket);
    }

    MYSQL_DISALLOW();
  }

  if (tmp) {
    const char *err;

    MYSQL_ALLOW();
    err = mysql_error(socket);
    MYSQL_DISALLOW();

    if (sp[-args].u.string->len <= 512) {
      Pike_error("%s(): Query \"%s\" failed (%s)\n",
		 name, query, err);
    } else {
      Pike_error("%s(): Query failed (%s)\n", name, err);
    }
  }

  pop_n_elems(args);

  if (!(result)) {
    int err;

    MYSQL_ALLOW();
    err = (
#ifdef mysql_field_count
	   mysql_field_count(socket)
#else /* !mysql_field_count */
#ifdef mysql_num_fields
	   mysql_num_fields(socket)
#else /* !mysql_num_fields */
	   socket->field_count
#endif /* mysql_num_fields */
#endif /* mysql_field_count */
	   && mysql_error(socket)[0]);
    MYSQL_DISALLOW();

    if (err) {
      Pike_error("%s(): Couldn't create result for query (%s)\n",
		 name, mysql_error(socket));
    }
    /* query was INSERT or similar - return 0 */

    push_int(0);
  } else {
    /* Return the result-object */
    struct precompiled_mysql_result *res;
    struct object *o;

    /* Create the object */
    ref_push_object(Pike_fp->current_object);
    push_object(o = clone_object(mysql_result_program, 1));

    /* Set the result. */
    if ((!(res = (struct precompiled_mysql_result *)
	   get_storage(o, mysql_result_program))) || res->result) {
      mysql_free_result(result);
      Pike_error("%s(): Bad mysql result object!\n", name);
    }
    res->result = result;
  }
}

/*! @decl Mysql.mysql_result big_query(string query)
 *!
 *! Make an SQL query.
 *!
 *! This function sends the SQL query @[query] to the Mysql-server. The result
 *! of the query is returned as a @[Mysql.mysql_result] object.
 *!
 *! Returns @expr{0@} (zero) if the query didn't return any result
 *! (e.g. @tt{INSERT@} or similar).
 *!
 *! @seealso
 *!   @[Mysql.mysql_result] @[streaming_query]
 */
static void f_big_query(INT32 args)
{
  low_query(args, "big_query", mysql_store_result);
}

/*! @decl Mysql.mysql_result big_query(string query)
 *!
 *! Makes a streaming SQL query.
 *!
 *! This function sends the SQL query @[query] to the Mysql-server.
 *! The result of the query is streamed through the returned
 *! @[Mysql.mysql_result] object. Note that the involved database
 *! tables are locked until all the results has been read.
 *!
 *! Returns @expr{0@} (zero) if the query didn't return any result
 *! (e.g. @tt{INSERT@} or similar).
 *!
 *! @seealso
 *!   @[Mysql.mysql_result]
 */
static void f_streaming_query(INT32 args)
{
  low_query(args, "streaming_query", mysql_use_result);
}


/*! @decl void create_db(string database)
 *!
 *! Create a new database
 *!
 *! This function creates a new database named @[database]
 *! in the Mysql-server.
 *!
 *! @seealso
 *!   @[select_db()], @[drop_db()]
 */
#ifdef USE_OLD_FUNCTIONS
static void f_create_db(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  char *database;
  int tmp = -1;

  if (!args) {
    SIMPLE_TOO_FEW_ARGS_ERROR ("Mysql.mysql->create_db", 1);
  }
  CHECK_8BIT_NONBINARY_STRING ("Mysql.mysql->create_db", 1);
  if (sp[-args].u.string->len > 127) {
    if (sp[-args].u.string->len < 1024) {
      Pike_error("Database name \"%s\" is too long (max 127 characters)\n",
	    sp[-args].u.string->str);
    } else {
      Pike_error("Database name (length %ld) is too long (max 127 characters)\n",
		 DO_NOT_WARN((long)sp[-args].u.string->len));
    }
  }
  database = sp[-args].u.string->str;

  if (socket) {
    MYSQL_ALLOW();

    tmp = mysql_create_db(socket, database);
    MYSQL_DISALLOW();
  }
  if (!socket || tmp) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();

    tmp = mysql_create_db(socket, database);

    MYSQL_DISALLOW();
  }

  if (tmp) {
    Pike_error("Mysql.mysql->create_db(): Creation of database \"%s\" failed\n",
	  sp[-args].u.string->str);
  }

  pop_n_elems(args);
}

/*! @decl void drop_db(string database)
 *!
 *! Drop a database
 *!
 *! This function drops the database named @[database] from the Mysql-server.
 *!
 *! @seealso
 *!  @[create_db()], @[select_db()]
 */
static void f_drop_db(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  char *database;
  int tmp = -1;

  if (!args) {
    SIMPLE_TOO_FEW_ARGS_ERROR ("Mysql.mysql->drop_db", 1);
  }
  CHECK_8BIT_NONBINARY_STRING ("Mysql.mysql->drop_db", 1);
  if (sp[-args].u.string->len > 127) {
    if (sp[-args].u.string->len < 1024) {
      Pike_error("Database name \"%s\" is too long (max 127 characters)\n",
	    sp[-args].u.string->str);
    } else {
      Pike_error("Database name (length %ld) is too long (max 127 characters)\n",
		 DO_NOT_WARN((long)sp[-args].u.string->len));
    }
  }
  database = sp[-args].u.string->str;

  if (socket) {
    MYSQL_ALLOW();

    tmp = mysql_drop_db(socket, database);

    MYSQL_DISALLOW();
  }
  if (!socket || tmp) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();

    tmp = mysql_drop_db(socket, database);

    MYSQL_DISALLOW();
  }    

  if (tmp) {
    Pike_error("Mysql.mysql->drop_db(): Drop of database \"%s\" failed\n",
	  sp[-args].u.string->str);
  }

  pop_n_elems(args);
}
#endif

/*! @decl void shutdown()
 *!
 *! Shutdown the Mysql-server
 *!
 *! This function shuts down a running Mysql-server.
 *!
 *! @seealso
 *!   @[reload()]
 */
static void f_shutdown(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  int tmp = -1;

  if (socket) {
    MYSQL_ALLOW();
  
#ifdef HAVE_SHUTDOWN_DEFAULT
    /* Mysql 4.1.3 added an extra shutdown_level argument. */
    tmp = mysql_shutdown(socket, SHUTDOWN_DEFAULT);
#else /* !HAVE_SHUTDOWN_DEFAULT */
    tmp = mysql_shutdown(socket);
#endif /* HAVE_SHUTDOWN_DEFAULT */

    MYSQL_DISALLOW();
  }
  if (!socket || tmp) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();
  
#ifdef HAVE_SHUTDOWN_DEFAULT
    /* Mysql 4.1.3 added an extra shutdown_level argument. */
    tmp = mysql_shutdown(socket, SHUTDOWN_DEFAULT);
#else /* !HAVE_SHUTDOWN_DEFAULT */
    tmp = mysql_shutdown(socket);
#endif /* HAVE_SHUTDOWN_DEFAULT */

    MYSQL_DISALLOW();
  }

  if (tmp) {
    Pike_error("Mysql.mysql->shutdown(): Shutdown failed\n");
  }

  pop_n_elems(args);
}

/*! @decl void reload()
 *!
 *! Reload security tables
 *!
 *! This function causes the Mysql-server to reload its access tables.
 *!
 *! @seealso
 *!   @[shutdown()]
 */
static void f_reload(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  int tmp = -1;

  if (socket) {
    MYSQL_ALLOW();

    tmp = mysql_reload(socket);

    MYSQL_DISALLOW();
  }
  if (!socket || tmp) {
    /* The connection might have been closed */
    pike_mysql_reconnect();

    socket = PIKE_MYSQL->socket;

    MYSQL_ALLOW();

    tmp = mysql_reload(socket);

    MYSQL_DISALLOW();
  }

  if (tmp) {
    Pike_error("Mysql.mysql->reload(): Reload failed\n");
  }

  pop_n_elems(args);
}

/*! @decl string statistics()
 *!
 *! Some Mysql-server statistics
 *!
 *! This function returns some server statistics.
 *!
 *! @seealso
 *!   @[server_info()], @[host_info()], @[protocol_info()]
 */
static void f_statistics(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  const char *stats;

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

/*! @decl string server_info()
 *!
 *! Get the version number of the Mysql-server.
 *!
 *! @seealso
 *!   @[statistics()], @[host_info()], @[protocol_info()]
 */
static void f_server_info(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  const char *info;

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

/*! @decl string host_info()
 *!
 *! Get information about the Mysql-server connection
 *!
 *! @seealso
 *!   @[statistics()], @[server_info()], @[protocol_info()]
 */
static void f_host_info(INT32 args)
{
  MYSQL *socket;
  const char *info;

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

/*! @decl int protocol_info()
 *!
 *! Give the Mysql protocol version
 *!
 *! This function returns the version number of the protocol the
 *! Mysql-server uses.
 *!
 *! @seealso
 *!   @[statistics()], @[server_info()], @[host_info()]
 */
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

/*! @decl object(Mysql.mysql_result) list_dbs()
 *! @decl object(Mysql.mysql_result) list_dbs(string wild)
 *!
 *! List databases
 *!
 *! Returns a table containing the names of all databases in the
 *! Mysql-server. If the argument @[wild] is specified, only those matching
 *! it will be returned.
 *!
 *! @seealso
 *!   @[list_tables()], @[list_fields()], @[list_processes()],
 *!   @[Mysql.mysql_result]
 */
static void f_list_dbs(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  MYSQL_RES *result = NULL;
  char *wild = NULL;

  if (args) {
    CHECK_8BIT_NONBINARY_STRING ("Mysql.mysql->list_dbs", 1);
    if (sp[-args].u.string->len > 80) {
      if (sp[-args].u.string->len < 1024) {
	Pike_error("Wildcard \"%s\" is too long (max 80 characters)\n",
	      sp[-args].u.string->str);
      } else {
	Pike_error("Wildcard (length %ld) is too long (max 80 characters)\n",
		   DO_NOT_WARN((long)sp[-args].u.string->len));
      }
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

  if (!result) {
    const char *err;

    MYSQL_ALLOW();

    err = mysql_error(socket);

    MYSQL_DISALLOW();

    Pike_error("Mysql.mysql->list_dbs(): Cannot list databases: %s\n", err);
  }

  pop_n_elems(args);

  {
    /* Return the result-object */
    struct precompiled_mysql_result *res;
    struct object *o;

    /* Create the object */
    ref_push_object(Pike_fp->current_object);
    push_object(o = clone_object(mysql_result_program, 1));

    /* Set the result. */
    if ((!(res = (struct precompiled_mysql_result *)
	   get_storage(o, mysql_result_program))) || res->result) {
      mysql_free_result(result);
      Pike_error("Mysql.mysql->list_dbs(): Bad mysql result object!\n");
    }
    res->result = result;
  }
}

/*! @decl object(Mysql.mysql_result) list_tables()
 *! @decl object(Mysql.mysql_result) list_tables(string wild)
 *!
 *! List tables in the current database
 *!
 *! Returns a table containing the names of all tables in the current
 *! database. If the argument @[wild] is given, only those matching it
 *! will be returned.
 *!
 *! @seealso
 *!   @[list_dbs()], @[list_fields()], @[list_processes()],
 *!   @[Mysql.mysql_result]
 */
static void f_list_tables(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  MYSQL_RES *result = NULL;
  char *wild = NULL;

  if (args) {
    CHECK_8BIT_NONBINARY_STRING ("Mysql.mysql->list_tables", 1);
    if (sp[-args].u.string->len > 80) {
      if (sp[-args].u.string->len < 1024) {
	Pike_error("Wildcard \"%s\" is too long (max 80 characters)\n",
	      sp[-args].u.string->str);
      } else {
	Pike_error("Wildcard (length %ld) is too long (max 80 characters)\n",
		   DO_NOT_WARN((long)sp[-args].u.string->len));
      }
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

  if (!result) {
    const char *err;

    MYSQL_ALLOW();

    err =  mysql_error(socket);

    MYSQL_DISALLOW();

    Pike_error("Mysql.mysql->list_tables(): Cannot list databases: %s\n", err);
  }

  pop_n_elems(args);

  {
    /* Return the result-object */
    struct precompiled_mysql_result *res;
    struct object *o;

    /* Create the object */
    ref_push_object(Pike_fp->current_object);
    push_object(o = clone_object(mysql_result_program, 1));

    /* Set the result. */
    if ((!(res = (struct precompiled_mysql_result *)
	   get_storage(o, mysql_result_program))) || res->result) {
      mysql_free_result(result);
      Pike_error("Mysql.mysql->list_tables(): Bad mysql result object!\n");
    }
    res->result = result;
  }
}

/*! @decl array(int|mapping(string:mixed)) list_fields(string table)
 *! @decl array(int|mapping(string:mixed)) list_fields(string table, @
 *!                                                    string wild)
 *!
 *! List all fields.
 *!
 *! Returns an array of mappings with information about the fields in the
 *! table named @[table]. If the argument @[wild] is given, only those
 *! fields matching it will be returned
 *!
 *! The mappings contain the following entries:
 *! @mapping
 *!   @member string "name"
 *!     The name of the field.
 *!   @member string "table"
 *!	The name of the table.
 *!   @member string "default"
 *!	The default value for the field.
 *!   @member string "type"
 *!	The SQL type of the field.
 *!   @member int "length"
 *!	The length of the field.
 *!   @member int "max_length"
 *!	The length of the longest element in this field.
 *!   @member multiset(string) "flags"
 *!	Some flags.
 *!   @member int decimals
 *!	The number of decimalplaces.
 *! @endmapping
 *!
 *! The type of the field can be any of:
 *!   @expr{"decimal"@}, @expr{"char"@}, @expr{"short"@}, @expr{"long"@},
 *!   @expr{"float"@}, @expr{"double"@}, @expr{"null"@}, @expr{"time"@},
 *!   @expr{"longlong"@}, @expr{"int24"@}, @expr{"tiny blob"@},
 *!   @expr{"medium blob"@},
 *!   @expr{"long blob"@}, @expr{"var string"@}, @expr{"string"@} or
 *!   @expr{"unknown"@}.
 *!
 *! The flags multiset can contain any of:
 *! @multiset
 *!   @index "primary_key"
 *!     This field is part of the primary key for this table.
 *!   @index "not_null"
 *!     This field may not be NULL.
 *!   @index "blob"
 *!     This field is a blob field.
 *! @endmultiset
 *!
 *! @note
 *!   Michael Widenius recomends use of the following query instead:
 *!   @tt{show fields in 'table' like "wild"@}.
 *!
 *! @seealso
 *!   @[list_dbs()], @[list_tables()], @[list_processes()],
 *!   @[Mysql.mysql_result.fetch_field()]
 */
static void f_list_fields(INT32 args)
{
  MYSQL *socket = PIKE_MYSQL->socket;
  MYSQL_RES *result = NULL;
  MYSQL_FIELD *field;
  int i = 0;
  char *table;
  char *wild = NULL;

  if (!args) {
    SIMPLE_TOO_FEW_ARGS_ERROR ("Mysql.mysql->list_fields", 1);
  }

  CHECK_8BIT_NONBINARY_STRING ("Mysql.mysql->list_fields", 1);
  if (sp[-args].u.string->len > 125) {
    if (sp[-args].u.string->len < 1024) {
      Pike_error("Table name \"%s\" is too long (max 125 characters)\n",
	    sp[-args].u.string->str);
    } else {
      Pike_error("Table name (length %ld) is too long (max 125 characters)\n",
		 DO_NOT_WARN((long)sp[-args].u.string->len));
    }
  }
  table = sp[-args].u.string->str;
  if (args > 1) {
    CHECK_8BIT_NONBINARY_STRING ("Mysql.mysql->list_fields", 2);
    if (sp[-args+1].u.string->len + sp[-args].u.string->len > 125) {
      /* The length of the table name has already been checked. */
      if (sp[-args+1].u.string->len < 1024) {
	Pike_error("Wildcard \"%s\" + table name \"%s\" is too long "
	      "(max 125 characters)\n",
	      sp[-args+1].u.string->str, sp[-args].u.string->str);
      } else {
	Pike_error("Wildcard (length %ld) + table name \"%s\" is too long "
		   "(max 125 characters)\n",
		   DO_NOT_WARN((long)sp[-args+1].u.string->len),
		   sp[-args].u.string->str);
      }
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
    const char *err;

    MYSQL_ALLOW();

    err = mysql_error(socket);

    MYSQL_DISALLOW();

    Pike_error("Mysql.mysql->list_fields(): Cannot list databases: %s\n", err);
  }

  pop_n_elems(args);

  /* FIXME: Should have MYSQL_{DIS,}ALLOW() here */

  while ((field = mysql_fetch_field(result))) {
    mysqlmod_parse_field(field, 1);
    i++;
  }
  f_aggregate(i);
}

/*! @decl object(Mysql.mysql_result) list_processes()
 *!
 *! List all processes in the Mysql-server
 *!
 *! Returns a table containing the names of all processes in the
 *! Mysql-server.
 *!
 *! @seealso
 *!   @[list_dbs()], @[list_tables()], @[list_fields()], @[Mysql.mysql_result]
 */
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

  if (!result) {
    const char *err;

    MYSQL_ALLOW();

    err = mysql_error(socket);

    MYSQL_DISALLOW();

    Pike_error("Mysql.mysql->list_processes(): Cannot list databases: %s\n", err);
  }

  {
    /* Return the result-object */
    struct precompiled_mysql_result *res;
    struct object *o;

    /* Create the object */
    ref_push_object(Pike_fp->current_object);
    push_object(o = clone_object(mysql_result_program, 1));

    /* Set the result. */
    if ((!(res = (struct precompiled_mysql_result *)
	   get_storage(o, mysql_result_program))) || res->result) {
      mysql_free_result(result);
      Pike_error("Mysql.mysql->list_processes(): Bad mysql result object!\n");
    }
    res->result = result;
  }
}

/*! @decl int binary_data()
 *!
 *! Inform if this version of @[Mysql.mysql] supports binary data
 *!
 *! This function returns non-zero if binary data can be reliably stored
 *! and retreived with this version of the mysql-module.
 *!
 *! Usually, there is no problem storing binary data in mysql-tables,
 *! but data containing @expr{'\0'@} (NUL) couldn't be fetched with old
 *! versions (prior to 3.20.5) of the mysql-library.
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

/*! @endclass
 */

/*! @endmodule
 */

#endif /* HAVE_MYSQL */

/*
 * Module linkage
 */


PIKE_MODULE_INIT
{
#ifdef HAVE_MYSQL
  start_new_program();
  ADD_STORAGE(struct precompiled_mysql);

  /* function(void:int|string) */
  ADD_FUNCTION("error", f_error,tFunc(tVoid,tOr(tInt,tStr)), ID_PUBLIC);
  /* function(string|void, string|void, string|void, string|void:void) */
  ADD_FUNCTION("create", f_create,tFunc(tOr(tStr,tVoid) tOr(tStr,tVoid) tOr(tStr,tVoid) tOr(tStr,tVoid) tOr(tMapping, tVoid),tVoid), ID_PUBLIC);
  /* function(int, void|mapping:string) */
  ADD_FUNCTION("_sprintf",mysql__sprintf,
	       tFunc(tInt tOr(tVoid,tMapping),tString),0);
  /* function(void:int) */
  ADD_FUNCTION("affected_rows", f_affected_rows,tFunc(tVoid,tInt), ID_PUBLIC);
  /* function(void:int) */
  ADD_FUNCTION("insert_id", f_insert_id,tFunc(tVoid,tInt), ID_PUBLIC);
  /* function(string:void) */
  ADD_FUNCTION("select_db", f_select_db,tFunc(tStr,tVoid), ID_PUBLIC);
  /* function(string:int|object) */
  ADD_FUNCTION("big_query", f_big_query,tFunc(tStr,tObj), ID_PUBLIC);
  /* function(string:int|object) */
  ADD_FUNCTION("streaming_query",
	       f_streaming_query,tFunc(tStr,tObj), ID_PUBLIC);
#ifdef USE_OLD_FUNCTIONS
  /* function(string:void) */
  ADD_FUNCTION("create_db", f_create_db,tFunc(tStr,tVoid), ID_PUBLIC);
  /* function(string:void) */
  ADD_FUNCTION("drop_db", f_drop_db,tFunc(tStr,tVoid), ID_PUBLIC);
#else
  add_integer_constant( "MYSQL_NO_ADD_DROP_DB", 1, 0 );
#endif
  /* function(void:void) */
  ADD_FUNCTION("shutdown", f_shutdown,tFunc(tVoid,tVoid), ID_PUBLIC);
  /* function(void:void) */
  ADD_FUNCTION("reload", f_reload,tFunc(tVoid,tVoid), ID_PUBLIC);
  /* function(void:string) */
  ADD_FUNCTION("statistics", f_statistics,tFunc(tVoid,tStr), ID_PUBLIC);
  /* function(void:string) */
  ADD_FUNCTION("server_info", f_server_info,tFunc(tVoid,tStr), ID_PUBLIC);
  /* function(void:string) */
  ADD_FUNCTION("host_info", f_host_info,tFunc(tVoid,tStr), ID_PUBLIC);
  /* function(void:int) */
  ADD_FUNCTION("protocol_info", f_protocol_info,tFunc(tVoid,tInt), ID_PUBLIC);
  /* function(void|string:object) */
  ADD_FUNCTION("list_dbs", f_list_dbs,tFunc(tOr(tVoid,tStr),tObj), ID_PUBLIC);
  /* function(void|string:object) */
  ADD_FUNCTION("list_tables", f_list_tables,tFunc(tOr(tVoid,tStr),tObj), ID_PUBLIC);
  /* function(string, void|string:array(int|mapping(string:mixed))) */
  ADD_FUNCTION("list_fields", f_list_fields,tFunc(tStr tOr(tVoid,tStr),tArr(tOr(tInt,tMap(tStr,tMix)))), ID_PUBLIC);
  /* function(void|string:object) */
  ADD_FUNCTION("list_processes", f_list_processes,tFunc(tOr(tVoid,tStr),tObj), ID_PUBLIC);

  /* function(void:int) */
  ADD_FUNCTION("binary_data", f_binary_data,tFunc(tVoid,tInt), ID_PUBLIC);

  add_integer_constant( "CLIENT_COMPRESS", CLIENT_COMPRESS, 0);
  add_integer_constant( "CLIENT_FOUND_ROWS", CLIENT_FOUND_ROWS, 0);
  add_integer_constant( "CLIENT_IGNORE_SPACE", CLIENT_IGNORE_SPACE, 0);
  add_integer_constant( "CLIENT_INTERACTIVE", CLIENT_INTERACTIVE, 0);
  add_integer_constant( "CLIENT_NO_SCHEMA", CLIENT_NO_SCHEMA, 0);
  add_integer_constant( "CLIENT_ODBC", CLIENT_ODBC, 0);

#ifdef HAVE_MYSQL_SSL
  add_integer_constant( "CLIENT_SSL", CLIENT_SSL, 0);
#endif

  set_init_callback(init_mysql_struct);
  set_exit_callback(exit_mysql_struct);

  mysql_program = end_program();
  add_program_constant("mysql", mysql_program, 0);

#ifdef HAVE_MYSQL_PORT
  STUPID_PORT_INIT();
#endif /* HAVE_MYSQL_PORT */

  init_mysql_res_programs();
#else
  if(!TEST_COMPAT(7,6))
    HIDE_MODULE();
#endif /* HAVE_MYSQL */
}

PIKE_MODULE_EXIT
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
