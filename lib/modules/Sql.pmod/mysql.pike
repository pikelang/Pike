/*
 * $Id: mysql.pike,v 1.3 1998/03/19 19:55:47 grubba Exp $
 *
 * Glue for the Mysql-module
 */

inherit Mysql.mysql;

string quote(string s)
{
  return(replace(s,
		 ({ "\\", "\"", "\0", "\'", "\n", "\r" }),
		 ({ "\\\\", "\\\"", "\\0", "\\\'", "\\n", "\\r" })));
}
