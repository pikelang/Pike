/*
 * $Id: odbc.pike,v 1.2 1998/10/17 03:00:34 grubba Exp $
 *
 * Glue for the ODBC-module
 */

#if constant(Odbc.odbc)
inherit Odbc.odbc;
#else /* !constant(Odbc.odbc) */
void create()
{
  destruct();
}
#endif /* constant(Odbc.odbc) */
