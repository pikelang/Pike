/*
 * $Id: mysql.pike,v 1.4 1998/03/20 21:58:23 grubba Exp $
 *
 * Glue for the Mysql-module
 */

//.
//. File:	mysql.pike
//. RCSID:	$Id: mysql.pike,v 1.4 1998/03/20 21:58:23 grubba Exp $
//. Author:	Henrik Grubbström (grubba@idonex.se)
//.
//. Synopsis:	Implements the glue to the Mysql-module.
//.
//. +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//.
//. Implements the glue needed to access the Mysql-module from the generic
//. SQL module.
//.

inherit Mysql.mysql;

//. - quote
//.   Quote a string so that it can safely be put in a query.
//. > s - String to quote.
string quote(string s)
{
  return(replace(s,
		 ({ "\\", "\"", "\0", "\'", "\n", "\r" }),
		 ({ "\\\\", "\\\"", "\\0", "\\\'", "\\n", "\\r" })));
}
