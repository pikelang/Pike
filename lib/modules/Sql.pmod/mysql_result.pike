/*
 * Glue for the Mysql-module
 */

#pike __REAL_VERSION__
#require constant(Mysql.mysql_result)

// Cannot dump this since the #require check may depend on the
// presence of system libs at runtime.
constant dont_dump_program = 1;

inherit Mysql.mysql_result;
