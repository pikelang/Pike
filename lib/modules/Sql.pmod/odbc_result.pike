/*
 * $Id: odbc_result.pike,v 1.4 2000/09/28 03:39:09 hubbe Exp $
 *
 * Glue for the ODBC-module
 */

#pike __REAL_VERSION__

#if constant(Odbc.odbc_result)
inherit Odbc.odbc_result;
#else /* !constant(Odbc.odbc_result) */
void create()
{
  destruct();
}
#endif /* constant(Odbc.odbc_result) */
