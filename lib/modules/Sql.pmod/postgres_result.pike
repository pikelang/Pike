#pike __REAL_VERSION__

//! Sql.postgres_result contains the result of a Postgres-query.
//! See @[Sql.postgres] for a description of this program's functions.

#if constant(Postgres)
inherit Postgres.postgres_result;
#else /* !constant(Postgres) */
constant this_program_does_not_exist=1;
#endif /* constant(Postgres) */
