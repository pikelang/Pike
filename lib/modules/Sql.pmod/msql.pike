// Msql module support stuff, (C) 1997 Francesco Chemolli <kinkie@kame.usr.dsi.unimi.it>

#pike __REAL_VERSION__

#if constant(Msql.msql)
inherit Msql.msql;

array(mapping(string:mixed)) query(string q,
                                   mapping(string|int:mixed)|void bindings) {
  if (!bindings)
    return ::query(q);
  return ::query(.sql_util.emulate_bindings(q,bindings),this_object());
}

#else /* !constant(Msql.msql) */
#error "mSQL support not available.\n"
#endif /* constant(Msql.msql) */
