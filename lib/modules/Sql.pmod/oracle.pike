/*
 * $Id: oracle.pike,v 1.2 1998/10/17 02:59:23 grubba Exp $
 *
 * Glue for the Oracle-module
 */

#if constant(Oracle.oracle)
inherit Oracle.oracle;
#else /* !constant(Oracle.oracle) */
void create()
{
  destruct();
}
#endif /* constant(Oracle.oracle) */
