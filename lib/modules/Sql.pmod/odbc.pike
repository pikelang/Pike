/*
 * $Id: odbc.pike,v 1.14 2004/04/16 12:12:46 grubba Exp $
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
  return ::big_query(.sql_util.emulate_bindings(q, bindings, this));
}

constant list_dbs = Odbc.list_dbs;

#else
constant this_program_does_not_exist=1;
#endif /* constant(Odbc.odbc) */
