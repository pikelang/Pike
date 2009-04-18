//! The NULL Sql handler.
//!
//! This is an empty Sql handler typically used to test other
//! functionality of the Sql module.

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
//!     @member string "bindings"
//!       @expr{@[sprintf]("%O", @[bindings])@}.
//!     @member string "formatted_query"
//!       The formatted query.
//!   @endmapping
array(mapping(string:mixed)) query(string query,
				   mapping(string:mixed)|void bindings)
{
  return ({([
	     "query":query,
	     "bindings":sprintf("%O", bindings),
	     "formatted_query":
	     .sql_util.emulate_bindings(query, bindings, this),
	   ])});
}
