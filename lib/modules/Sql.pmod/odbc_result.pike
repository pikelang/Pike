/*
 * $Id: odbc_result.pike,v 1.2 1998/10/17 03:01:59 grubba Exp $
 *
 * Glue for the ODBC-module
 */

#if constant(Odbc.odbc_result)
inherit Odbc.odbc_result;
#else /* !constant(Odbc.odbc_result) */
void create()
{
  destruct();
}
#endif /* constant(Odbc.odbc_result) */
