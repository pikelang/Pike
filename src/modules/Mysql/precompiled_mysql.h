/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: precompiled_mysql.h,v 1.17 2005/04/12 00:36:13 nilsson Exp $
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

/* From the Pike-dist */

/*
 * Structures
 */

struct precompiled_mysql {
#ifdef PIKE_THREADS
  DEFINE_MUTEX(lock);
#endif /* PIKE_THREADS */

  MYSQL		*mysql, *socket;
  struct pike_string	*host, *database, *user, *password;	/* Reconnect */
  struct mapping   *options;
};

struct precompiled_mysql_result {
  struct object *connection;
  MYSQL_RES	*result;
  int eof;
};

/*
 * Defines
 */

#define PIKE_MYSQL	((struct precompiled_mysql *)(Pike_fp->current_storage))
#define PIKE_MYSQL_RES	((struct precompiled_mysql_result *)(Pike_fp->current_storage))

/*
 * Globals
 */

extern struct program *mysql_program;
extern struct program *mysql_result_program;

/*
 * Prototypes
 */

/* From result.c */

void init_mysql_res_efuns(void);
void init_mysql_res_programs(void);
void exit_mysql_res(void);
void mysqlmod_parse_field(MYSQL_FIELD *field, int support_default);

#endif /* PRECOMPILED_MYSQL_H */
