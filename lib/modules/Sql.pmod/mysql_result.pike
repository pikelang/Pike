/*
 * $Id: mysql_result.pike,v 1.4 2000/09/28 03:39:08 hubbe Exp $
 *
 * Glue for the Mysql-module
 */

#pike __REAL_VERSION__

#if constant(Mysql.mysql_result)
inherit Mysql.mysql_result;
#else /* !constant(Mysql.mysql_result) */
void create()
{
  destruct();
}
#endif /* constant(Mysql.mysql_result) */

