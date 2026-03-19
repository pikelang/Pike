/*
 * Glue for the Oracle-module
 */

#pike __REAL_VERSION__
#require constant(Oracle.oracle)

//! Sql interface class to connect to Oracle databases.

// Cannot dump this since the #require check may depend on the
// presence of system libs at runtime.
optional constant dont_dump_program = 1;

inherit Oracle.oracle;

string server_info()
{
  return "Oracle";
}
