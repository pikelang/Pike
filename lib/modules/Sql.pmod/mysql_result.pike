/*
 * $Id$
 *
 * Glue for the Mysql-module
 */

#pike __REAL_VERSION__

// Cannot dump this since the #if constant(...) check below may depend
// on the presence of system libs at runtime.
constant dont_dump_program = 1;

#if constant(Mysql.mysql_result)
inherit Mysql.mysql_result;
#else /* !constant(Mysql.mysql_result) */
constant this_program_does_not_exist=1;
#endif /* constant(Mysql.mysql_result) */
