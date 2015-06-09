/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * SQL database connectivity for Pike
 *
 * Henrik Grubbström 1996-12-21
 */

#ifndef PRECOMPILED_MYSQL_H
#define PRECOMPILED_MYSQL_H

/*
 * Includes
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_WINSOCK_H
#include <winsock.h>
#endif

/* From the mysql-dist */
/* Workaround for versions prior to 3.20.0 not beeing protected against
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

/*
 * Macros
 */

#define PIKE_MYSQL_FLAG_STORE_RESULT	1
#define PIKE_MYSQL_FLAG_TYPED_RESULT	2

/*
 * Prototypes
 */

/* From result.c */

struct object *make_mysql_result(struct object *mysql,
				 MYSQL_RES *result,
				 int flags);
void init_mysql_res_efuns(void);
void init_mysql_res_programs(void);
void exit_mysql_res(void);
void mysqlmod_parse_field(MYSQL_FIELD *field, int support_default);

#endif /* PRECOMPILED_MYSQL_H */
