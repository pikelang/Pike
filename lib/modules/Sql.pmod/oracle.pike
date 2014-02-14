/*
 * Glue for the Oracle-module
 */

#pike __REAL_VERSION__
#require constant(Oracle.oracle)

// Cannot dump this since the #require check may depend on the
// presence of system libs at runtime.
constant dont_dump_program = 1;

inherit Oracle.oracle;

string server_info()
{
  return "Oracle";
}
