
#pike __REAL_VERSION__

//! Implements the glue needed to access the Msql-module from the generic
//! SQL module.

#if constant(Msql.msql)
inherit Msql.msql;

array(mapping(string:mixed)) query(string q,
                                   mapping(string|int:mixed)|void bindings) {
  if (!bindings)
    return ::query(q);
  return ::query(.sql_util.emulate_bindings(q,bindings),this);
}

#else
constant this_program_does_not_exist=1;
#endif /* constant(Msql.msql) */
