/*
 * $Id: precompiled_odbc.h,v 1.6 1999/03/20 00:18:58 marcus Exp $
 *
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

#ifdef HAVE_ISQL_H
#ifndef FAR
#define FAR
#endif /* FAR */
#ifndef EXPORT
#define EXPORT
#endif /* EXPORT */
#ifndef CALLBACK
#define CALLBACK
#endif /* CALLBACK */
#include <isql.h>
#else /* !HAVE_ISQL_H */
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#else /* !HAVE_WINDOWS_H */
#ifdef HAVE_QEODBC_H
#include <qeodbc.h>
#else
#ifdef HAVE_ODBC
#undef HAVE_ODBC
#endif /* HAVE_ODBC */
#endif /* HAVE_QEODBC_H */
#endif /* HAVE_WINDOWS_H */
#endif /* HAVE_ISQL_H */

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
 * Structures
 */

struct field_info {
  SDWORD len;
  int size;		/* Size of buffer */
  void *buf;
};

struct precompiled_odbc {
  HDBC hdbc;
  int affected_rows;
  unsigned int flags;
  struct pike_string *last_error;
};

struct precompiled_odbc_result {
  struct object *obj;
  struct precompiled_odbc *odbc;
  HSTMT hstmt;
  SWORD num_fields;
  SDWORD num_rows;
  struct array *fields;
  struct field_info *field_info;
};

/*
 * Defines
 */

#define PIKE_ODBC	((struct precompiled_odbc *)(fp->current_storage))
#define PIKE_ODBC_RES	((struct precompiled_odbc_result *)(fp->current_storage))

/* Flags */
#define PIKE_ODBC_CONNECTED	1

/*
 * Prototypes
 */
void odbc_error(const char *fun, const char *msg,
		struct precompiled_odbc *odbc, HSTMT hstmt,
		RETCODE code, void (*clean)(void));

void init_odbc_res_programs(void);
void exit_odbc_res(void);

#endif /* HAVE_ODBC */
#endif /* PIKE_PRECOMPILED_ODBC_H */
