/*
 * $Id: mysql.pike,v 1.2 1998/03/19 19:50:05 grubba Exp $
 *
 * Glue for the Mysql-module
 */

inherit Mysql.mysql;

string sqlquote(string s)
{
  return(replace(s,
		 ({ "\\", "\"", "\0", "\'", "\n", "\r" }),
		 ({ "\\\\", "\\\"", "\\0", "\\\'", "\\n", "\\r" })));
}
