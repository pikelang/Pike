/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
||| for more information.
||| $Id: acconfig.h,v 1.6 2002/10/08 20:22:34 nilsson Exp $
\*/

/*
 * Config-file for the Pike ODBC-module.
 *
 * Henrik Grubbström
 */

#ifndef PIKE_ODBC_CONFIG_H
#define PIKE_ODBC_CONFIG_H

@TOP@

/* Define if you have SQLSMALLINT */
#undef HAVE_SQLSMALLINT

/* Define if you have SQLUSMALLINT */
#undef HAVE_SQLUSMALLINT

/* Define if you have SQLINTEGER */
#undef HAVE_SQLINTEGER

/* Define if you have SQLUINTEGER */
#undef HAVE_SQLUINTEGER

/* Define if you have SQLLEN */
#undef HAVE_SQLLEN

/* Define if you have SQLULEN */
#undef HAVE_SQLULEN

/* Define if you have ODBC */
#undef HAVE_ODBC

@BOTTOM@

#endif /* PIKE_ODBC_CONFIG_H */
