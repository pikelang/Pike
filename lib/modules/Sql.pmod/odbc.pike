/*
 * $Id$
 *
 * Glue for the ODBC-module
 */

#pike __REAL_VERSION__

// Cannot dump this since the #if constant(...) check below may depend
// on the presence of system libs at runtime.
constant dont_dump_program = 1;

#if constant(Odbc.odbc)
inherit Odbc.odbc;

int|object big_query(object|string q, mapping(string|int:mixed)|void bindings)
{  
  if (!bindings)
    return ::big_query(q);
  return ::big_query(.sql_util.emulate_bindings(q, bindings, this));
}

int|object big_typed_query(object|string q,
			   mapping(string|int:mixed)|void bindings)
{
  if (!bindings)
    return ::big_typed_query(q);
  return ::big_typed_query(.sql_util.emulate_bindings(q, bindings, this));
}

constant list_dbs = Odbc.list_dbs;

//!
class typed_result
{
  inherit odbc::typed_result;

  //! Value to use to represent NULL.
  mixed _null_value = Val.null;
}

#else
constant this_program_does_not_exist=1;
#endif /* constant(Odbc.odbc) */
