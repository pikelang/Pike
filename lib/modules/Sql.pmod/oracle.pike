/*
 * $Id: oracle.pike,v 1.5 2000/09/26 19:00:10 hubbe Exp $
 *
 * Glue for the Oracle-module
 */

#pike __VERSION__

#if constant(Oracle.oracle)
inherit Oracle.oracle;

string server_info()
{
  return "Oracle";
}
#else /* !constant(Oracle.oracle) */
#error "Oracle support not available.\n"
#endif /* constant(Oracle.oracle) */
