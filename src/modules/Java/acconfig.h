/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: acconfig.h,v 1.7 2004/12/19 19:24:04 grubba Exp $
*/

/*
 * Config-file for the Pike Java module.
 *
 * Marcus Comstedt
 */

#ifndef PIKE_JAVA_CONFIG_H
#define PIKE_JAVA_CONFIG_H

@TOP@
@BOTTOM@

/* Define if you have Java */
#undef HAVE_JAVA

/* Define to home of Java */
#undef JAVA_HOME

/* Define to the library path for Java libraries */
#undef JAVA_LIBPATH

/* Define if you have a Sparc CPU */
#undef HAVE_SPARC_CPU

/* Define if you have an x86 CPU */
#undef HAVE_X86_CPU

/* Define if you have an x86_64 CPU */
#undef HAVE_X86_64_CPU

/* Define if you have a PowerPC CPU */
#undef HAVE_PPC_CPU

/* Define if you have an alpha CPU */
#undef HAVE_ALPHA_CPU

/* Define for debug if you java has ibmFindDLL(). */
#undef HAVE_IBMFINDDLL

#endif /* PIKE_JAVA_CONFIG_H */
