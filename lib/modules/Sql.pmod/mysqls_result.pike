#pike __REAL_VERSION__

// Cannot dump this since the #if constant(...) check below may depend
// on the presence of system libs at runtime.
constant dont_dump_program = 1;

#if constant(Mysql.mysql.CLIENT_SSL)

inherit Sql.mysql_result;

#else
constant this_program_does_not_exist = 1;
#endif
