/*
 * $Id: odbc.pike,v 1.13 2004/04/14 20:20:35 nilsson Exp $
 *
 * Glue for the ODBC-module
 */

#pike __REAL_VERSION__

#if constant(Odbc)
inherit Odbc.odbc;

int|object big_query(object|string q, mapping(string|int:mixed)|void bindings)
{  
  if (!bindings)
    return ::big_query(q);
  return ::big_query(.sql_util.emulate_bindings(q, bindings, this));
}

constant list_dbs = Odbc.list_dbs;

#else
constant this_program_does_not_exist=1;
#endif /* constant(Odbc) */
