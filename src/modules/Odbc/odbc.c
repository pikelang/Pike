/*
 * $Id: odbc.c,v 1.12 1998/11/22 11:04:48 hubbe Exp $
 *
 * Pike interface to ODBC compliant databases.
 *
 * Henrik Grubbström
 */

/*
 * Includes
 */

#include "global.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

RCSID("$Id: odbc.c,v 1.12 1998/11/22 11:04:48 hubbe Exp $");

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

#include "precompiled_odbc.h"

#ifdef HAVE_ODBC

/*
 * Globals
 */

struct program *odbc_program = NULL;

HENV odbc_henv = SQL_NULL_HENV;

/*
 * Functions
 */

/*
 * Helper functions
 */

void odbc_error(const char *fun, const char *msg,
		struct precompiled_odbc *odbc, HSTMT hstmt,
		RETCODE code, void (*clean)(void));

static INLINE void odbc_check_error(const char *fun, const char *msg,
				    RETCODE code, void (*clean)(void))
{
  if ((code != SQL_SUCCESS) && (code != SQL_SUCCESS_WITH_INFO)) {
    odbc_error(fun, msg, PIKE_ODBC, SQL_NULL_HSTMT, code, clean);
  }
}

void odbc_error(const char *fun, const char *msg,
		struct precompiled_odbc *odbc, HSTMT hstmt,
		RETCODE code, void (*clean)(void))
{
  RETCODE _code;
  unsigned char errcode[256];
  unsigned char errmsg[SQL_MAX_MESSAGE_LENGTH];
  SWORD errmsg_len = 0;
  SDWORD native_error;

  _code = SQLError(odbc_henv, odbc->hdbc, hstmt, errcode, &native_error,
		   errmsg, SQL_MAX_MESSAGE_LENGTH-1, &errmsg_len);
  errmsg[errmsg_len] = '\0';

  if (odbc) {
    if (odbc->last_error) {
      free_string(odbc->last_error);
    }
    odbc->last_error = make_shared_binary_string((char *)errmsg, errmsg_len);
  }

  if (clean) {
    clean();
  }
  switch(_code) {
  case SQL_SUCCESS:
  case SQL_SUCCESS_WITH_INFO:
    error("%s(): %s:\n"
	  "%d:%s:%s\n",
	  fun, msg, code, errcode, errmsg);
    break;
  case SQL_ERROR:
    error("%s(): %s:\n"
	  "SQLError failed (%d:SQL_ERROR)\n",
	  fun, msg, code);
    break;
  case SQL_NO_DATA_FOUND:
    error("%s(): %s:\n"
	  "SQLError failed (%d:SQL_NO_DATA_FOUND)\n",
	  fun, msg, code);
    break;
  case SQL_INVALID_HANDLE:
    error("%s(): %s:\n"
	  "SQLError failed (%d:SQL_INVALID_HANDLE)\n",
	  fun, msg, code);
    break;
  default:
    error("%s(): %s:\n"
	  "SQLError failed (%d:%d)\n",
	  fun, msg, code, _code);
    break;
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

/*
 * Glue functions
 */

static void init_odbc_struct(struct object *o)
{
  RETCODE code;

  PIKE_ODBC->hdbc = SQL_NULL_HDBC;
  PIKE_ODBC->affected_rows = 0;
  PIKE_ODBC->flags = 0;
  PIKE_ODBC->last_error = NULL;

  odbc_check_error("init_odbc_struct", "ODBC initialization failed",
		   SQLAllocConnect(odbc_henv, &(PIKE_ODBC->hdbc)), NULL);
}

static void exit_odbc_struct(struct object *o)
{
  HDBC hdbc = PIKE_ODBC->hdbc;

  if (hdbc != SQL_NULL_HDBC) {
    if (PIKE_ODBC->flags & PIKE_ODBC_CONNECTED) {
      PIKE_ODBC->flags &= ~PIKE_ODBC_CONNECTED;
      odbc_check_error("odbc_error", "Disconnecting HDBC",
		       SQLDisconnect(hdbc), (void (*)(void))exit_odbc_struct);
      /* NOTE: Potential recursion above! */
    }
    PIKE_ODBC->hdbc = SQL_NULL_HDBC;
    odbc_check_error("odbc_error", "Freeing HDBC",
		     SQLFreeConnect(hdbc), clean_last_error);
  }
  clean_last_error();
}

/*
 * Pike functions
 */

static void f_error(INT32 args)
{
  pop_n_elems(args);

  if (PIKE_ODBC->last_error) {
    ref_push_string(PIKE_ODBC->last_error);
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
  if (PIKE_ODBC->flags & PIKE_ODBC_CONNECTED) {
    PIKE_ODBC->flags &= ~PIKE_ODBC_CONNECTED;
    /* Disconnect old hdbc */
    odbc_check_error("odbc->create", "Disconnecting HDBC",
		     SQLDisconnect(PIKE_ODBC->hdbc), NULL);
  }
  odbc_check_error("odbc->create", "Connect failed",
		   SQLConnect(PIKE_ODBC->hdbc, (unsigned char *)server->str,
			      server->len, (unsigned char *)user->str,
			      user->len, (unsigned char *)pwd->str,
			      pwd->len), NULL);
  PIKE_ODBC->flags |= PIKE_ODBC_CONNECTED;
  pop_n_elems(args);
}

static void f_affected_rows(INT32 args)
{
  pop_n_elems(args);
  push_int(PIKE_ODBC->affected_rows);
}

static void f_select_db(INT32 args)
{
  /**********************************************/
}

/* Needed since free_string() can be a macro */
static void odbc_free_string(struct pike_string *s)
{
  free_string(s);
}

static void f_big_query(INT32 args)
{
  ONERROR ebuf;
  HSTMT hstmt = SQL_NULL_HSTMT;
  struct pike_string *q = NULL;
#ifdef PIKE_DEBUG
  struct svalue *save_sp = sp + 1 - args;
#endif /* PIKE_DEBUG */

  get_all_args("odbc->big_query", args, "%S", &q);

  add_ref(q);
  SET_ONERROR(ebuf, odbc_free_string, q);

  pop_n_elems(args);

  clean_last_error();

  /* Allocate the statement (result) object */
  ref_push_object(fp->current_object);
  push_object(clone_object(odbc_result_program, 1));

  UNSET_ONERROR(ebuf);

  /* Potential return value is now in place */

  PIKE_ODBC->affected_rows = 0;

  /* Do the actual query */
  push_string(q);
  apply(sp[-2].u.object, "execute", 1);
  
  if (sp[-1].type != T_INT) {
    error("odbc->big_query(): Unexpected return value from "
	  "odbc_result->execute().\n");
  }

  if (!sp[-1].u.integer) {
    pop_n_elems(2);	/* Zap the result object too */

    odbc_check_error("odbc->big_query", "Couldn't commit query",
		     SQLTransact(odbc_henv, PIKE_ODBC->hdbc, SQL_COMMIT),
		     NULL);

    push_int(0);
  } else {
    pop_stack();	/* Keep the result object */
  }
#ifdef PIKE_DEBUG
  if (sp != save_sp) {
    fatal("Stack error in odbc->big_query().\n");
  }
#endif /* PIKE_DEBUG */
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
  RETCODE err = SQLAllocEnv(&odbc_henv);

  if (err != SQL_SUCCESS) {
    error("odbc_module_init(): SQLAllocEnv() failed with code %08x\n", err);
  }

  start_new_program();
  add_storage(sizeof(struct precompiled_odbc));

  add_function("error", f_error, "function(void:int|string)", ID_PUBLIC);
  add_function("create", f_create, "function(string|void, string|void, string|void, string|void:void)", ID_PUBLIC);

  add_function("select_db", f_select_db, "function(string:void)", ID_PUBLIC);
  add_function("big_query", f_big_query, "function(string:int|object)", ID_PUBLIC);
  add_function("affected_rows", f_affected_rows, "function(void:int)", ID_PUBLIC);
  /* NOOP's: */
  add_function("create_db", f_create_db, "function(string:void)", ID_PUBLIC);
  add_function("drop_db", f_drop_db, "function(string:void)", ID_PUBLIC);
  add_function("shutdown", f_shutdown, "function(void:void)", ID_PUBLIC);
  add_function("reload", f_reload, "function(void:void)", ID_PUBLIC);
#if 0
  add_function("insert_id", f_insert_id, "function(void:int)", ID_PUBLIC);
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

  if (odbc_henv != SQL_NULL_HENV) {
    RETCODE err = SQLFreeEnv(odbc_henv);
    odbc_henv = SQL_NULL_HENV;

    if ((err != SQL_SUCCESS) && (err != SQL_SUCCESS_WITH_INFO)) {
      error("odbc_module_exit(): SQLFreeEnv() failed with code %08x\n", err);
    }
  }
#endif /* HAVE_ODBC */
}
 
