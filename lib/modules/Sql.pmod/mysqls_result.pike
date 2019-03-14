#pike __REAL_VERSION__
#require constant(Mysql.mysql.CLIENT_SSL)

// Cannot dump this since the #require check may depend on the
// presence of system libs at runtime.
constant dont_dump_program = 1;

inherit Sql.mysql_result;
