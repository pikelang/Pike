/*
 * $Id: odbc.pike,v 1.7 2000/09/28 03:39:09 hubbe Exp $
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
  return ::big_query(.sql_util.emulate_bindings(q,bindings),this_object());
}

#else /* !constant(Odbc.odbc) */
#error "ODBC support not available.\n"
#endif /* constant(Odbc.odbc) */
