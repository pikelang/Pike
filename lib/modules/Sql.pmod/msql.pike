// Msql module support stuff, (C) 1997 Francesco Chemolli <kinkie@comedia.it>

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
