/*
 * $Id: oracle.pike,v 1.6 2000/09/28 03:39:09 hubbe Exp $
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
#else /* !constant(Oracle.oracle) */
#error "Oracle support not available.\n"
#endif /* constant(Oracle.oracle) */
