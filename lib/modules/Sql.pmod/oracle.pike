/*
 * $Id: oracle.pike,v 1.3 1999/06/14 23:08:39 grubba Exp $
 *
 * Glue for the Oracle-module
 */

#if constant(Oracle.oracle)
inherit Oracle.oracle;
#else /* !constant(Oracle.oracle) */
#error "Oracle support not available.\n"
#endif /* constant(Oracle.oracle) */
