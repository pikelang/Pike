/*
 * $Id: odbc_result.pike,v 1.6 2004/04/16 12:12:46 grubba Exp $
 *
 * Glue for the ODBC-module
 */

#pike __REAL_VERSION__

#if constant(Odbc.odbc_result)
inherit Odbc.odbc_result;
#else /* !constant(Odbc.odbc_result) */
constant this_program_does_not_exist=1;
#endif /* constant(Odbc.odbc_result) */
