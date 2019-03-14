
#pike __REAL_VERSION__
#require constant(Msql.msql)

//! Implements the glue needed to access the Msql-module from the generic
//! SQL module.

// Cannot dump this since the #require check may depend on the
// presence of system libs at runtime.
constant dont_dump_program = 1;

inherit Msql.msql;

array(mapping(string:mixed)) query(string q,
                                   mapping(string|int:mixed)|void bindings) {
  if (!bindings)
    return ::query(q);
  return ::query(.sql_util.emulate_bindings(q,bindings),this);
}
