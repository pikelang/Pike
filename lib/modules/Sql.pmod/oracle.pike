/*
 * $Id: oracle.pike,v 1.7 2002/11/27 15:40:34 mast Exp $
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
