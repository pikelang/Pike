/* $Id: pg_types.h,v 1.3 1998/03/28 14:41:17 grubba Exp $ */
#ifndef _PG_TYPES_H_
#define _PG_TYPES_H_

#include "program.h"
#include "svalue.h"

struct postgres_result_object_data {
  PGresult * result;
  int cursor;
};

struct pgres_object_data {
	PGconn *dblink;
	struct pike_string *last_error;
	PGresult * last_result;
	struct svalue * notify_callback;
};

/* The header name could be deceiving, but who cares? */
extern struct program *postgres_program, *pgresult_program;

#endif
