/*
 * $Id: precompiled_mysql.h,v 1.1 1996/12/28 18:51:41 grubba Exp $
 *
 * SQL database connectivity for Pike
 *
 * Henrik Grubbström 1996-12-21
 */

#ifndef PRECOMPILED_MYSQL_H
#define PRECOMPILED_MYSQL_H

/*
 * Includes
 */

/* From the mysql-dist */
#ifndef MYSQL_MYSQL_H
#define MYSQL_MYSQL_H
#include <mysql.h>
#endif

/* From the Pike-dist */

/*
 * Structures
 */

struct precompiled_mysql {
  MYSQL		mysql, *socket;
  MYSQL_RES	*last_result;	/* UGLY way to pass arguments to create() */
};

struct precompiled_mysql_result {
  MYSQL_RES	*result;
};

/*
 * Defines
 */

#define PIKE_MYSQL	((struct precompiled_mysql *)(fp->current_storage))
#define PIKE_MYSQL_RES	((struct precompiled_mysql_result *)(fp->current_storage))

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

#endif /* PRECOMPILED_MYSQL_H */
