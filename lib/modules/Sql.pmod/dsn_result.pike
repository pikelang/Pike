/*
 * $Id: dsn_result.pike,v 1.1 2005/01/26 18:57:45 grubba Exp $
 *
 * Glue for the ODBC-module
 */

#pike __REAL_VERSION__

#if constant(Odbc.odbc_result)
inherit Odbc.odbc_result;
#else /* !constant(Odbc.odbc_result) */
constant this_program_does_not_exist=1;
#endif /* constant(Odbc.odbc_result) */
