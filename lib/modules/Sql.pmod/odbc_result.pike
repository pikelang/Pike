/*
 * $Id: odbc_result.pike,v 1.3 2000/09/26 19:00:09 hubbe Exp $
 *
 * Glue for the ODBC-module
 */

#pike __VERSION__

#if constant(Odbc.odbc_result)
inherit Odbc.odbc_result;
#else /* !constant(Odbc.odbc_result) */
void create()
{
  destruct();
}
#endif /* constant(Odbc.odbc_result) */
