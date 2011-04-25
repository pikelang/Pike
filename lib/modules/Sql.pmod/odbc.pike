/*
 * $Id$
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

#endif /* constant(Odbc.odbc) */
