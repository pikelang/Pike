/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: precompiled_odbc.h,v 1.22 2006/03/25 11:34:30 grubba Exp $
*/

/*
 * Pike interface to ODBC compliant databases.
 *
 * Henrik Grubbström
 */

#ifndef PIKE_PRECOMPILED_ODBC_H
#define PIKE_PRECOMPILED_ODBC_H

/*
 * Includes
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_ODBC
#ifdef HAVE_ISQL_H
#include <isql.h>
#else /* !HAVE_ISQL_H */
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#else /* !HAVE_WINDOWS_H */
#ifdef HAVE_QEODBC_H
#include <qeodbc.h>
#else
#undef HAVE_ODBC
#endif /* HAVE_QEODBC_H */
#endif /* HAVE_WINDOWS_H */
#endif /* HAVE_ISQL_H */
#endif /* HAVE_ODBC */

#ifdef HAVE_ODBC

#ifndef HAVE_ISQL_H
#ifdef HAVE_SQL_H
#include <sql.h>
#endif /* HAVE_SQL_H */
#endif /* !HAVE_ISQL_H */
#ifdef HAVE_ISQLEXT_H
#include <isqlext.h>
#else /* !HAVE_ISQLEXT_H */
#ifdef HAVE_SQLEXT_H
#include <sqlext.h>
#endif /* HAVE_SQLEXT_H */
#endif /* HAVE_ISQLEXT_H */

/*
 * Globals
 */

extern struct program *odbc_program;
extern struct program *odbc_result_program;

/*
 * Typedefs
 */

#ifndef HAVE_SQLSMALLINT
typedef SWORD	SQLSMALLINT;
#endif /* HAVE_SQLSMALLINT */

#ifndef HAVE_SQLUSMALLINT
typedef UWORD	SQLUSMALLINT;
#endif /* HAVE_SQLUSMALLINT */

#ifndef HAVE_SQLINTEGER
typedef SDWORD	SQLINTEGER;
#endif /* !HAVE_SQLINTEGER */

#ifndef HAVE_SQLUINTEGER
typedef UDWORD	SQLUINTEGER;
#endif /* !HAVE_SQLUINTEGER */

#ifndef HAVE_SQLLEN
typedef SQLINTEGER	SQLLEN;
#endif /* !HAVE_SQLLEN */

#ifndef HAVE_SQLULEN
typedef SQLUINTEGER	SQLULEN;
#endif /* !HAVE_SQLLEN */

#ifndef HAVE_SQLHENV
typedef HENV	SQLHENV;
#endif /* !HAVE_SQLENV */

#ifndef HAVE_SQLHDBC
typedef HDBC	SQLHDBC;
#endif /* !HAVE_SQLHDBC */

#ifndef HAVE_SQLHSTMT
typedef HSTMT	SQLHSTMT;
#endif /* !HAVE_SQLSTMT */

/*
 * Structures
 */

struct field_info {
  SWORD type;
};

struct precompiled_odbc {
  SQLHDBC hdbc;
  SQLLEN affected_rows;
  unsigned int flags;
  struct pike_string *last_error;
};

struct precompiled_odbc_result {
  struct object *obj;
  struct precompiled_odbc *odbc;
  SQLHSTMT hstmt;
  SWORD num_fields;
  SQLLEN num_rows;
  struct array *fields;
  struct field_info *field_info;
};

/*
 * Defines
 */

#define PIKE_ODBC	((struct precompiled_odbc *)(Pike_fp->current_storage))
#define PIKE_ODBC_RES	((struct precompiled_odbc_result *)(Pike_fp->current_storage))

/* Flags */
#define PIKE_ODBC_CONNECTED	1

/*
 * Prototypes
 */
#ifdef SQL_WCHAR
void push_sqlwchar(SQLWCHAR *str, size_t num_bytes);
#endif /* SQL_WCHAR */
void odbc_error(const char *fun, const char *msg,
		struct precompiled_odbc *odbc, SQLHSTMT hstmt,
		RETCODE code, void (*clean)(void *), void *clean_arg);

void init_odbc_res_programs(void);
void exit_odbc_res(void);

#endif /* HAVE_ODBC */
#endif /* PIKE_PRECOMPILED_ODBC_H */
