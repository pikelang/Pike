/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: acconfig.h,v 1.11 2003/03/28 21:33:31 marcus Exp $
*/

/*
 * Config-file for the Pike mySQL-module.
 *
 * Henrik Grubbström 1997-01-30
 */

#ifndef PIKE_MYSQL_CONFIG_H
#define PIKE_MYSQL_CONFIG_H

@TOP@
@BOTTOM@

/* Define if you have mySQL */
#undef HAVE_MYSQL

/* Return type of mysql_fetch_lengths(). Usually unsigned long. */
#undef FETCH_LENGTHS_TYPE

/* Define if you have mysql_fetch_lengths(). */
#undef HAVE_MYSQL_FETCH_LENGTHS

/* Define if you have mysql_real_query(). */
#undef HAVE_MYSQL_REAL_QUERY

/* Define if you have mysql_options(). */
#undef HAVE_MYSQL_OPTIONS

/* Define if you have the mysql_port variable. */
#undef HAVE_MYSQL_PORT

/* Define if you have the mysql_unix_port variable. */
#undef HAVE_MYSQL_UNIX_PORT

#endif /* PIKE_MYSQL_CONFIG_H */
