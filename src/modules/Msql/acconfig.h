/* -*- c -*-
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#undef STDC_HEADERS

@TOP@
@BOTTOM@

/* End of autoconfigurable section */
#undef HAVE_MSQL

#ifdef HAVE_MSQL_H
#ifdef HAVE_LIBMSQL
#define HAVE_MSQL
#endif
#endif

#ifdef HAVE_MSQLLISTINDEX
#define MSQL_VERSION_2
#endif
