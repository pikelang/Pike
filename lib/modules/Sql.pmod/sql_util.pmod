/*
 * $Id: sql_util.pmod,v 1.2 1999/07/01 20:25:38 grubba Exp $
 *
 * Some SQL utility functions.
 * They are kept here to avoid circular references.
 *
 * Henrik Grubbström 1999-07-01
 */

//.
//. File:	sql_util.pmod
//. RCSID:	$Id: sql_util.pmod,v 1.2 1999/07/01 20:25:38 grubba Exp $
//. Author:	Henrik Grubbström (grubba@idonex.se)
//.
//. Synopsis:	Some SQL utility functions
//.
//. +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//.
//. These functions are kept here mainly to avoid circular references.
//.

//. - quote
//.   Quote a string so that it can safely be put in a query.
//. > s - String to quote.
string quote(string s)
{
  return(replace(s, "\'", "\'\'"));
}

//. - fallback
//.   Throw an error in case an unimplemented function is called.
void fallback()
{
  throw(({ "Function not supported in this database.", backtrace() }));
}
