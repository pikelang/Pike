#if constant(Postgres.postgres_result)
inherit Postgres.postgres_result;
#else /* !constant(Postgres.postgres_result) */
void create() {
  destruct();
}
#endif /* constant(Postgres.postgres_result) */
