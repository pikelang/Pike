#pike __REAL_VERSION__

//! Sql.postgres_result contains the result of a Postgres-query.
//! See @[Sql.postgres] for a description of this program's functions.

// Cannot dump this since the #if constant(...) check below may depend
// on the presence of system libs at runtime.
constant dont_dump_program = 1;

#if constant(Postgres.postgres_result)
inherit Postgres.postgres_result;
#else /* !constant(Postgres.postgres_result) */
constant this_program_does_not_exist=1;
#endif /* constant(Postgres.postgres_result) */
