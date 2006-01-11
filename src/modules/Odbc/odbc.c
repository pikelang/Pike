/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: odbc.c,v 1.35 2006/01/11 17:37:34 grubba Exp $
*/

/*
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

RCSID("$Id: odbc.c,v 1.35 2006/01/11 17:37:34 grubba Exp $");

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
#include "bignum.h"
#include "builtin_functions.h"

#include "precompiled_odbc.h"


#define sp Pike_sp
#define fp Pike_fp

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
#ifdef SQL_WCHAR
  SQLWCHAR errcode[256];
  SQLWCHAR errmsg[SQL_MAX_MESSAGE_LENGTH];
#else
  unsigned char errcode[256];
  unsigned char errmsg[SQL_MAX_MESSAGE_LENGTH];
#endif
  SWORD errmsg_len = 0;
  SDWORD native_error;

  _code =
#ifdef SQL_WCHAR
    SQLErrorW
#else
    SQLError
#endif
    (odbc_henv, odbc->hdbc, hstmt, errcode, &native_error,
		    errmsg, SQL_MAX_MESSAGE_LENGTH-1, &errmsg_len);
  errmsg[errmsg_len] = '\0';

  if (odbc) {
    if (odbc->last_error) {
      free_string(odbc->last_error);
    }
#ifdef SQL_WCHAR
    odbc->last_error = make_shared_binary_string1(errmsg, errmsg_len);
#else
    odbc->last_error = make_shared_binary_string((char *)errmsg, errmsg_len);
#endif
  }

  if (clean) {
    clean();
  }
  switch(_code) {
  case SQL_SUCCESS:
  case SQL_SUCCESS_WITH_INFO:
#ifdef SQL_WCHAR
    Pike_error("%s(): %s:\n"
	  "%d:%ws:%ws\n",
	  fun, msg, code, errcode, errmsg);
#else
    Pike_error("%s(): %s:\n"
	  "%d:%s:%s\n",
	  fun, msg, code, errcode, errmsg);
#endif
    break;
  case SQL_ERROR:
    Pike_error("%s(): %s:\n"
	  "SQLError failed (%d:SQL_ERROR)\n",
	  fun, msg, code);
    break;
  case SQL_NO_DATA_FOUND:
    Pike_error("%s(): %s:\n"
	  "SQLError failed (%d:SQL_NO_DATA_FOUND)\n",
	  fun, msg, code);
    break;
  case SQL_INVALID_HANDLE:
    Pike_error("%s(): %s:\n"
	  "SQLError failed (%d:SQL_INVALID_HANDLE)\n",
	  fun, msg, code);
    break;
  default:
    Pike_error("%s(): %s:\n"
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

  check_all_args("odbc->create", args,
		 BIT_STRING|BIT_INT|BIT_VOID, BIT_STRING|BIT_INT|BIT_VOID,
		 BIT_STRING|BIT_INT|BIT_VOID, BIT_STRING|BIT_VOID|BIT_INT, 0);

#define GET_ARG(VAR, ARG) do {					\
    if ((args > (ARG)) && (sp[(ARG)-args].type == T_STRING)) {	\
      VAR = sp[(ARG)-args].u.string;				\
      if (VAR->size_shift) {					\
	Pike_error("create(): Bad argument %d "			\
		   "(expected string(8bit))\n",			\
		   (ARG)+1);					\
      }								\
    }								\
  } while(0)

  GET_ARG(pwd, 3);
  GET_ARG(user, 2);
  GET_ARG(database, 1);
  GET_ARG(server, 0);

#undef GET_ARG

  /*
   * NOTE:
   *
   *   If no database has been specified, use the server argument.
   *   If neither has been specified, connect to the database named "default".
   */

  if (!database || !database->len) {
    if (!server || !server->len) {
      push_constant_text("default");
      database = sp[-1].u.string;
      args++;
    } else {
      database = server;
    }
  }
  if (!user) {
    push_constant_text("");
    user = sp[-1].u.string;
    args++;
  }
  if (!pwd) {
    push_constant_text("");
    pwd = sp[-1].u.string;
    args++;
  }
  if (PIKE_ODBC->flags & PIKE_ODBC_CONNECTED) {
    PIKE_ODBC->flags &= ~PIKE_ODBC_CONNECTED;
    /* Disconnect old hdbc */
    odbc_check_error("odbc->create", "Disconnecting HDBC",
		     SQLDisconnect(PIKE_ODBC->hdbc), NULL);
  }

  /* FIXME: Support wide strings. */

  odbc_check_error("odbc->create", "Connect failed",
		   SQLConnect(PIKE_ODBC->hdbc, (unsigned char *)database->str,
			      DO_NOT_WARN((SQLSMALLINT)database->len),
			      (unsigned char *)user->str,
			      DO_NOT_WARN((SQLSMALLINT)user->len),
			      (unsigned char *)pwd->str,
			      DO_NOT_WARN((SQLSMALLINT)pwd->len)),
		   NULL);
  PIKE_ODBC->flags |= PIKE_ODBC_CONNECTED;
  pop_n_elems(args);
}

static void f_affected_rows(INT32 args)
{
  pop_n_elems(args);
  push_int64(PIKE_ODBC->affected_rows);
}

static void f_select_db(INT32 args)
{
  /**********************************************/
}

/* Needed since free_string() can be a macro */
static void odbc_free_string(struct pike_string *s)
{
  if (s) {
    free_string(s);
  }
}

static void f_big_query(INT32 args)
{
  ONERROR ebuf;
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
    Pike_error("odbc->big_query(): Unexpected return value from "
	  "odbc_result->execute().\n");
  }

  if (!sp[-1].u.integer) {
    pop_n_elems(2);	/* Zap the result object too */

#ifdef ENABLE_IMPLICIT_COMMIT
    /* This breaks with Free TDS. */
    odbc_check_error("odbc->big_query", "Couldn't commit query",
		     SQLTransact(odbc_henv, PIKE_ODBC->hdbc, SQL_COMMIT),
		     NULL);
#endif /* ENABLE_IMPLICIT_COMMIT */

    push_int(0);
  } else {
    pop_stack();	/* Keep the result object */
  }
#ifdef PIKE_DEBUG
  if (sp != save_sp) {
    Pike_fatal("Stack error in odbc->big_query().\n");
  }
#endif /* PIKE_DEBUG */
}

static void f_list_tables(INT32 args)
{
#ifdef PIKE_DEBUG
  struct svalue *save_sp = sp + 1 - args;
#endif /* PIKE_DEBUG */
  ONERROR ebuf;
  struct pike_string *pattern = NULL;

  if (args) {
    if ((Pike_sp[-args].type != T_STRING) ||
	(Pike_sp[-args].u.string->size_shift)) {
      Pike_error("odbc->list_tables(): "
		 "Bad argument 1. Expected 8-bit string.\n");
    }
    copy_shared_string(pattern, Pike_sp[-args].u.string);
  }

  SET_ONERROR(ebuf, odbc_free_string, pattern);

  pop_n_elems(args);

  clean_last_error();

  /* Allocate the statement (result) object */
  ref_push_object(fp->current_object);
  push_object(clone_object(odbc_result_program, 1));

  UNSET_ONERROR(ebuf);

  /* Potential return value is now in place */

  PIKE_ODBC->affected_rows = 0;

  /* Do the actual query */
  if (pattern) {
    push_string(pattern);
    apply(sp[-1].u.object, "list_tables", 1);
  } else {
    apply(sp[-1].u.object, "list_tables", 0);
  }
  
  if (sp[-1].type != T_INT) {
    Pike_error("odbc->list_tables(): Unexpected return value from "
	       "odbc_result->list_tables().\n");
  }

  if (!sp[-1].u.integer) {
    pop_n_elems(2);	/* Zap the result object too */

    push_int(0);
  } else {
    pop_stack();	/* Keep the result object */
  }
#ifdef PIKE_DEBUG
  if (sp != save_sp) {
    Pike_fatal("Stack error in odbc->list_tables().\n");
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

static void f_list_dbs(INT32 args)
{
#ifdef SQL_WCHAR
  static SQLWCHAR buf[SQL_MAX_DSN_LENGTH+1];
  static SQLWCHAR descr[256];
#else
  static UCHAR buf[SQL_MAX_DSN_LENGTH+1];
  static UCHAR descr[256];
#endif
  SQLSMALLINT buf_len = 0;
  SQLSMALLINT descr_len = 0;
  int cnt = 0;
  RETCODE ret;

  pop_n_elems(args);

  ret =
#ifdef SQL_WCHAR
    SQLDataSourcesW
#else
    SQLDataSources
#endif
    (odbc_henv, SQL_FETCH_FIRST,
     buf, sizeof(buf), &buf_len,
     descr, sizeof(descr), &descr_len);
  while ((ret == SQL_SUCCESS) || (ret == SQL_SUCCESS_WITH_INFO)) {
#ifdef SQL_WCHAR
    push_string(make_shared_binary_string1(buf, buf_len));
#else
    push_string(make_shared_binary_string(buf, buf_len));
#endif
    cnt++;
    ret =
#ifdef SQL_WCHAR
      SQLDataSourcesW
#else
      SQLDataSources
#endif
      (odbc_henv, SQL_FETCH_NEXT,
       buf, sizeof(buf), &buf_len,
       descr, sizeof(descr), &descr_len);
  }
  f_aggregate(cnt);
}

#endif /* HAVE_ODBC */

/*
 * Module linkage
 */

PIKE_MODULE_INIT
{
#ifdef HAVE_ODBC
  RETCODE err = SQLAllocEnv(&odbc_henv);

  if (err != SQL_SUCCESS) {
    odbc_henv = SQL_NULL_HENV;
    return;
    /*    Pike_error("odbc_module_init(): SQLAllocEnv() failed with code %08x\n", err); */
  }

  start_new_program();
  ADD_STORAGE(struct precompiled_odbc);

  /* function(void:int|string) */
  ADD_FUNCTION("error", f_error,tFunc(tVoid,tOr(tInt,tStr)), ID_PUBLIC);
  /* function(string|void, string|void, string|void, string|void:void) */
  ADD_FUNCTION("create", f_create,tFunc(tOr(tStr,tVoid) tOr(tStr,tVoid) tOr(tStr,tVoid) tOr(tStr,tVoid),tVoid), ID_PUBLIC);

  /* function(string:void) */
  ADD_FUNCTION("select_db", f_select_db,tFunc(tStr,tVoid), ID_PUBLIC);
  /* function(string:int|object) */
  ADD_FUNCTION("big_query", f_big_query,tFunc(tStr,tOr(tInt,tObj)), ID_PUBLIC);
  /* function(void:int) */
  ADD_FUNCTION("affected_rows", f_affected_rows,tFunc(tVoid,tInt), ID_PUBLIC);
  /* function(void|string:object) */
  ADD_FUNCTION("list_tables", f_list_tables,tFunc(tOr(tVoid,tStr),tObj), ID_PUBLIC);
  /* NOOP's: */
  /* function(string:void) */
  ADD_FUNCTION("create_db", f_create_db,tFunc(tStr,tVoid), ID_PUBLIC);
  /* function(string:void) */
  ADD_FUNCTION("drop_db", f_drop_db,tFunc(tStr,tVoid), ID_PUBLIC);
  /* function(void:void) */
  ADD_FUNCTION("shutdown", f_shutdown,tFunc(tVoid,tVoid), ID_PUBLIC);
  /* function(void:void) */
  ADD_FUNCTION("reload", f_reload,tFunc(tVoid,tVoid), ID_PUBLIC);
#if 0
  /* function(void:int) */
  ADD_FUNCTION("insert_id", f_insert_id,tFunc(tVoid,tInt), ID_PUBLIC);
  /* function(void:string) */
  ADD_FUNCTION("statistics", f_statistics,tFunc(tVoid,tStr), ID_PUBLIC);
  /* function(void:string) */
  ADD_FUNCTION("server_info", f_server_info,tFunc(tVoid,tStr), ID_PUBLIC);
  /* function(void:string) */
  ADD_FUNCTION("host_info", f_host_info,tFunc(tVoid,tStr), ID_PUBLIC);
  /* function(void:int) */
  ADD_FUNCTION("protocol_info", f_protocol_info,tFunc(tVoid,tInt), ID_PUBLIC);
  /* function(string, void|string:array(int|mapping(string:mixed))) */
  ADD_FUNCTION("list_fields", f_list_fields,tFunc(tStr tOr(tVoid,tStr),tArr(tOr(tInt,tMap(tStr,tMix)))), ID_PUBLIC);
  /* function(void|string:object) */
  ADD_FUNCTION("list_processes", f_list_processes,tFunc(tOr(tVoid,tStr),tObj), ID_PUBLIC);
 
  /* function(void:int) */
  ADD_FUNCTION("binary_data", f_binary_data,tFunc(tVoid,tInt), ID_PUBLIC);
#endif /* 0 */
 
  set_init_callback(init_odbc_struct);
  set_exit_callback(exit_odbc_struct);
 
  odbc_program = end_program();
  add_program_constant("odbc", odbc_program, 0);
 
  /* function(void|string:array(string)) */
  ADD_FUNCTION("list_dbs", f_list_dbs,tFunc(tOr(tVoid,tStr),tArr(tStr)), ID_PUBLIC);

  init_odbc_res_programs();

#endif /* HAVE_ODBC */
}

PIKE_MODULE_EXIT
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
      Pike_error("odbc_module_exit(): SQLFreeEnv() failed with code %08x\n", err);
    }
  }
#endif /* HAVE_ODBC */
}
