/*
 * $Id: mysql_result.pike,v 1.3 2000/09/26 19:00:09 hubbe Exp $
 *
 * Glue for the Mysql-module
 */

#pike __VERSION__

#if constant(Mysql.mysql_result)
inherit Mysql.mysql_result;
#else /* !constant(Mysql.mysql_result) */
void create()
{
  destruct();
}
#endif /* constant(Mysql.mysql_result) */

