#pike __REAL_VERSION__
#require constant(Postgres.postgres_result)

//! Sql.postgres_result contains the result of a Postgres-query.
//! See @[Sql.postgres] for a description of this program's functions.

// Cannot dump this since the #require check may depend on the
// presence of system libs at runtime.
constant dont_dump_program = 1;

inherit Postgres.postgres_result;
