/*
 * $Id$
 *
 * Glue for the Oracle-module
 */

#pike __REAL_VERSION__

#if constant(Oracle.oracle)
inherit Oracle.oracle;

string server_info()
{
  return "Oracle";
}
#endif /* constant(Oracle.oracle) */
