/*
 * $Id: precompiled_odbc.h,v 1.4 1998/05/31 17:17:14 grubba Exp $
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

#include <qeodbc.h>
#ifdef HAVE_SQL_H
#include <sql.h>
#endif /* HAVE_SQL_H */
#ifdef HAVE_SQLEXT_H
#include <sqlext.h>
#endif /* HAVE_SQLEXT_H */

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

#endif /* PIKE_PRECOMPILED_ODBC_H */
