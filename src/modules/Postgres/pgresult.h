/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

extern struct program *Postgres_postgres_program;
extern struct program *Postgres_postgres_result_program;

struct object *make_postgres_result(struct object *postgres,
				    PGresult *result,
				    unsigned int flags);
void pgresult_init(void);
void pgresult_exit(void);

struct Postgres_postgres_struct;
PGconn *pike_get_pgconn(struct Postgres_postgres_struct *this);
#if defined(PIKE_THREADS) && defined(PQ_THREADSAFE)
PIKE_MUTEX_T *pike_get_pgmux(struct Postgres_postgres_struct *this);
#endif
void pike_pg_set_lastcommit(struct Postgres_postgres_struct *this);
