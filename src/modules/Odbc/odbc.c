/*
 * $Id: odbc.c,v 1.3 1997/06/10 03:21:41 grubba Exp $
 *
 * Pike interface to ODBC compliant databases.
 *
 * Henrik Grubbström
 */

/*
 * Includes
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "global.h"
RCSID("$Id: odbc.c,v 1.3 1997/06/10 03:21:41 grubba Exp $");

#include "interpret.h"
#include "object.h"
#include "threads.h"
#include "svalue.h"
#include "stralloc.h"
#include "mapping.h"
#include "array.h"
#include "multiset.h"
#include "program.h"
#include "module_support.h"

#ifdef HAVE_ODBC

#include "precompiled_odbc.h"

/*
 * Globals
 */

struct program *odbc_program = NULL;

/*
 * Functions
 */

/*
 * Helper functions
 */

volatile void odbc_error(const char *fun, const char *msg,
			 struct precompiled_odbc *odbc, HSTMT hstmt,
			 RETCODE code, void (*clean)(void));

static INLINE volatile void odbc_check_error(const char *fun, const char *msg,
					     RETCODE code, void (*clean)(void))
{
  if ((code != SQL_SUCCESS) && (code != SQL_SUCCESS_WITH_INFO)) {
    odbc_error(fun, msg, PIKE_ODBC, PIKE_ODBC->hstmt, code, clean);
  }
}

volatile void odbc_error(const char *fun, const char *msg,
			 struct precompiled_odbc *odbc, HSTMT hstmt,
			 RETCODE code, void (*clean)(void))
{
  RETCODE _code;
  char errcode[256];
  char errmsg[SQL_MAX_MESSAGE_LENGTH];
  SWORD errmsg_len = 0;
  SDWORD native_error;

  _code = SQLError(odbc->henv, odbc->hdbc, hstmt, errcode, &native_error,
		   errmsg, SQL_MAX_MESSAGE_LENGTH-1, &errmsg_len);
  errmsg[errmsg_len] = '\0';

  if (odbc) {
    if (odbc->last_error) {
      free_string(odbc->last_error);
    }
    odbc->last_error = make_shared_binary_string(errmsg, errmsg_len);
  }

  if (clean) {
    clean();
  }
  switch(_code) {
  case SQL_SUCCESS:
  case SQL_SUCCESS_WITH_INFO:
    error("%s():%s: %d:%s:%s\n", fun, msg, code, errcode, errmsg);
    break;
  default:
    error("%s():%s: SQLError failed (%d:%d)\n", fun, msg, code, _code);
  }
}

/*
 * Clean-up functions
 */

static void clean_last_error(void)
{
  if (PIKE_ODBC->last_error) {
    free_string(PIKE_ODBC->last_error);
    PIKE_ODBC->last_error = NULL;
  }
}

static void clean_henv(void)
{
  HENV henv = PIKE_ODBC->henv;
  PIKE_ODBC->henv = SQL_NULL_HENV;
  odbc_check_error("odbc_error", "Freeing HENV",
		   SQLFreeEnv(henv), clean_last_error);
}

static void clean_hdbc(void)
{
  HDBC hdbc = PIKE_ODBC->hdbc;
  PIKE_ODBC->hdbc = SQL_NULL_HDBC;
  odbc_check_error("odbc_error", "Disconnecting HDBC",
		   SQLDisconnect(hdbc), clean_henv);
  odbc_check_error("odbc_error", "Freeing HDBC",
		   SQLFreeConnect(hdbc), clean_henv);
}

/*
 * Glue functions
 */

static void init_odbc_struct(struct object *o)
{
  RETCODE code;

  PIKE_ODBC->henv = SQL_NULL_HENV;
  PIKE_ODBC->hdbc = SQL_NULL_HDBC;
  PIKE_ODBC->hstmt = SQL_NULL_HSTMT;
  PIKE_ODBC->last_error = NULL;
  PIKE_ODBC->affected_rows = 0;

  odbc_check_error("init_odbc_struct", "ODBC initialization failed",
		   SQLAllocEnv(&(PIKE_ODBC->henv)), 0);

  odbc_check_error("init_odbc_struct", "ODBC initialization failed",
		   SQLAllocConnect(PIKE_ODBC->henv, &(PIKE_ODBC->hdbc)),
		   clean_henv);
}

static void exit_odbc_struct(struct object *o)
{
  RETCODE code;
  HENV henv = PIKE_ODBC->henv;
  HDBC hdbc = PIKE_ODBC->hdbc;

  PIKE_ODBC->hdbc = SQL_NULL_HDBC;
  odbc_check_error("exit_odbc_struct", "ODBC disconnect failed",
		   SQLDisconnect(hdbc), clean_henv);
  odbc_check_error("exit_odbc_struct", "Freeing ODBC connection failed",
		   SQLFreeConnect(hdbc), clean_henv);
  PIKE_ODBC->henv = SQL_NULL_HENV;
  odbc_check_error("exit_odbc_struct", "Freeing ODBC environment failed",
		   SQLFreeEnv(henv), clean_last_error);
}

/*
 * Pike functions
 */

static void f_error(INT32 args)
{
  pop_n_elems(args);

  if (PIKE_ODBC->last_error) {
    PIKE_ODBC->last_error->refs++;
    push_string(PIKE_ODBC->last_error);
  } else {
    push_int(0);
  }
}

static void f_create(INT32 args)
{
  struct pike_string *server = NULL;
  struct pike_string *database = NULL;
  struct pike_string *user = NULL;
  struct pike_string *pwd = NULL;

  get_args(sp-args, args, "%S%S%S%S", &server, &database, &user, &pwd);

  /*
   * NOTE:
   *
   * database argument is ignored
   */

  if (!pwd) {
    pwd = make_shared_string("");
  }
  if (!user) {
    user = make_shared_string("");
  }
  if (!server) {
    server = make_shared_string("");
  }
  odbc_check_error("odbc->create", "Connect failed",
		   SQLConnect(PIKE_ODBC->hdbc, server->str, server->len,
			      user->str, user->len, pwd->str, pwd->len), NULL);
  pop_n_elems(args);
}

static void f_affected_rows(INT32 args)
{
  pop_n_elems(args);
  push_int(PIKE_ODBC->affected_rows);
}

static void f_insert_id(INT32 args)
{
  /**********************************************/
}

static void f_select_db(INT32 args)
{
  /**********************************************/
}

static void f_big_query(INT32 args)
{
  HSTMT hstmt = SQL_NULL_HSTMT;
  struct pike_string *q = NULL;

  get_all_args("odbc->big_query", args, "%S", &q);

  odbc_check_error("odbc->big_query", "Statement allocation failed",
		   SQLAllocStmt(PIKE_ODBC->hdbc, &hstmt), NULL);
  odbc_check_error("odbc->big_query", "Query failed",
		   SQLExecDirect(hstmt, q->str, q->len), NULL);
  pop_n_elems(args);

  odbc_check_error("odbc->big_query", "Couldn't get the number of fields",
		   SQLNumResultCols(hstmt, &(PIKE_ODBC->num_fields)), NULL);

  odbc_check_error("odbc->big_query", "Couldn't get the number of rows",
		   SQLRowCount(hstmt, &(PIKE_ODBC->affected_rows)), NULL);

  pop_n_elems(args);

  if (PIKE_ODBC->num_fields) {
    PIKE_ODBC->hstmt=hstmt;
    push_object(fp->current_object);
    fp->current_object->refs++;

    push_object(clone_object(odbc_result_program, 1));
  } else {
    odbc_check_error("odbc->big_query", "Couldn't commit query",
		     SQLTransact(PIKE_ODBC->henv, PIKE_ODBC->hdbc,
				 SQL_COMMIT), NULL);
    push_int(0);
  }
}

static void f_create_db(INT32 args)
{
  /**************************************************/
}

static void f_drop_db(INT32 args)
{
  /**************************************************/
}

static void f_shutdown(INT32 args)
{
  /**************************************************/
}

static void f_reload(INT32 args)
{
  /**************************************************/
}


#endif /* HAVE_ODBC */

/*
 * Module linkage
 */

void pike_module_init(void)
{
#ifdef HAVE_ODBC
  start_new_program();
  add_storage(sizeof(struct precompiled_odbc));

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
#if 0
  add_function("statistics", f_statistics, "function(void:string)", ID_PUBLIC);
  add_function("server_info", f_server_info, "function(void:string)", ID_PUBLIC);
  add_function("host_info", f_host_info, "function(void:string)", ID_PUBLIC);
  add_function("protocol_info", f_protocol_info, "function(void:int)", ID_PUBLIC);
  add_function("list_dbs", f_list_dbs, "function(void|string:object)", ID_PUBLIC);
  add_function("list_tables", f_list_tables, "function(void|string:object)", ID_PUBLIC);
  add_function("list_fields", f_list_fields, "function(string, void|string:array(int|mapping(string:mixed)))", ID_PUBLIC);
  add_function("list_processes", f_list_processes, "function(void|string:object)", ID_PUBLIC);
 
  add_function("binary_data", f_binary_data, "function(void:int)", ID_PUBLIC);
#endif /* 0 */
 
  set_init_callback(init_odbc_struct);
  set_exit_callback(exit_odbc_struct);
 
  odbc_program = end_program();
  add_program_constant("odbc", odbc_program, 0);
 
  init_odbc_res_programs();

#endif /* HAVE_ODBC */
}

void pike_module_exit(void)
{
#ifdef HAVE_ODBC
  exit_odbc_res();
 
  if (odbc_program) {
    free_program(odbc_program);
    odbc_program = NULL;
  }
#endif /* HAVE_ODBC */
}
 
