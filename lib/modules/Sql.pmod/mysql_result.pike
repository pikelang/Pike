/*
 * $Id: mysql_result.pike,v 1.2 1998/10/17 03:07:12 grubba Exp $
 *
 * Glue for the Mysql-module
 */

#if constant(Mysql.mysql_result)
inherit Mysql.mysql_result;
#else /* !constant(Mysql.mysql_result) */
void create()
{
  destruct();
}
#endif /* constant(Mysql.mysql_result) */

