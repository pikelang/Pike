/*
 * $Id: odbc.pike,v 1.10 2002/06/11 08:45:56 grubba Exp $
 *
 * Glue for the ODBC-module
 */

#pike __REAL_VERSION__

#if constant(Odbc.odbc)
inherit Odbc.odbc;

int|object big_query(object|string q, mapping(string|int:mixed)|void bindings)
{  
  if (!bindings)
    return ::big_query(q);
  return ::big_query(.sql_util.emulate_bindings(q, bindings, this_object()));
}

constant list_dbs = Odbc.list_dbs;

#else /* !constant(Odbc.odbc) */
#error "ODBC support not available.\n"
#endif /* constant(Odbc.odbc) */
