/*
 * $Id: mysql_result.pike,v 1.5 2004/04/14 20:20:35 nilsson Exp $
 *
 * Glue for the Mysql-module
 */

#pike __REAL_VERSION__

#if constant(Mysql)
inherit Mysql.mysql_result;
#else /* !constant(Mysql) */
constant this_program_does_not_exist=1;
#endif /* constant(Mysql) */

