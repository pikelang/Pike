/*
 * $Id: odbc.pike,v 1.3 1999/06/14 23:08:38 grubba Exp $
 *
 * Glue for the ODBC-module
 */

#if constant(Odbc.odbc)
inherit Odbc.odbc;
#else /* !constant(Odbc.odbc) */
#error "ODBC support not available.\n"
#endif /* constant(Odbc.odbc) */
