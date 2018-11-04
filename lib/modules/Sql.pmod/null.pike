//! The NULL Sql handler.
//!
//! This is an empty Sql handler typically used to test other
//! functionality of the Sql module.

#pike __REAL_VERSION__

inherit __builtin.Sql.Connection;

//! @returns
//!   @expr{@[sprintf]("quote(%q)", s)@}.
string quote(string s)
{
  return sprintf("quote(%q)", s);
}

//! @returns
//!   Returns an array with a single element:
//!   @mapping
//!     @member string "query"
//!       The query string before formating.
//!     @member string "bindings_query"
//!       The query string before bindings having been inserted.
//!     @member string "formatted_query"
//!       The formatted query.
//!   @endmapping
variant Sql.Result big_query(string query)
{
  return Sql.sql_array_result(({([
				  "query":query,
				  "bindings_query":query,
				  "formatted_query":query,
				])}));
}

//! @returns
//!   Returns an array with a single element:
//!   @mapping
//!     @member string "query"
//!       The query string before formating.
//!     @member string "bindings_query"
//!       The query string before bindings having been inserted.
//!     @member string "bindings"
//!       @expr{@[sprintf]("%O", @[bindings])@}.
//!     @member string "formatted_query"
//!       The formatted query.
//!   @endmapping
variant Sql.Result big_query(string query, mapping bindings)
{
  Sql.Result res = ::big_query(query, bindings);

  res->_values[0]->bindings = sprintf("%O", bindings);
  res->_values[0]->query = query;
  res->_values[0]->bindings_query = query;
  return res;
}

//! @returns
//!   Returns an array with a single element:
//!   @mapping
//!     @member string "query"
//!       The query string before formating.
//!     @member string "bindings_query"
//!       The query string before bindings having been inserted.
//!     @member string "bindings"
//!       @expr{@[sprintf]("%O", @[bindings])@}.
//!     @member string "formatted_query"
//!       The formatted query.
//!     @member string "args"
//!       @expr{@[sprintf]("%O", ({@[extraarg]}) + extraargs)@}.
//!   @endmapping
variant Sql.Result big_query(string query,
			     string|int|float|object extraarg,
			     string|int|float|object ... extraargs)
{
  Sql.Result res = ::big_query(query, extraarg, @extraargs);

  res->_values[0]->query = query;
  res->_values[0]->args = sprintf("%O", ({ extraarg }) + extraargs);
  return res;
}
