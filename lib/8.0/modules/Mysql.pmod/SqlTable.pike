#pike 9.0

//! Pike 8.0 API compatibility for Mysql.SqlTable.

//!
inherit Mysql.SqlTable;

//! Differs from @[::handle_argspec()] in that it does not have
//! the @tt{db_conn@} parameter.
local string handle_argspec(array argspec, mapping(string:mixed) bindings)
{
  return ::handle_argspec(UNDEFINED, argspec, bindings);
}
