
#pike __REAL_VERSION__

#if constant(Msql.msql)
inherit Msql.msql;

//in c it's too hard, we're better off doing it in pike
mapping(string:mapping(string:mixed)) list_fields(string table, string|void wild) {
	mapping result = ::list_fields(table);
	if (!wild||!strlen(wild))
		return result;
	array a=glob(wild,indices(result));
	return mkmapping(a,Array.map(a,lambda(string s, mapping m) 
			{return m[s];},result));
}

array(mapping(string:mixed)) query(string q,
                                   mapping(string|int:mixed)|void bindings) {
  if (!bindings)
    return ::query(q);
  return ::query(.sql_util.emulate_bindings(q,bindings),this_object());
}

#else /* !constant(Msql.msql) */
#error "mSQL support not available.\n"
#endif /* constant(Msql.msql) */
