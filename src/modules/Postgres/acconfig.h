/* -*- c -*-
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: acconfig.h,v 1.2 2003/04/28 09:50:33 grubba Exp $
*/

#undef STDC_HEADERS

@TOP@
@BOTTOM@

/* Define if we are running PostgreSQL 7.2 or newer */
#undef HAVE_PG72

/* If the PQsetnonblocking function is available, it means we're using
 * PostgreSQL 7.x or newer. This, in turn, means that the interface is
 * thread-safe on a per-connection level (as opposed to non-threadsafe).
 */
#ifdef HAVE_PQSETNONBLOCKING
# define PQ_THREADSAFE 1
#endif

/* End of autoconfigurable section */
#if defined(HAVE_LIBPQ) && \
    (defined(HAVE_POSTGRES_H) || \
     defined(HAVE_POSTGRES_FE_H) || \
     defined(HAVE_SERVER_POSTGRES_H) || \
     defined(HAVE_SERVER_POSTGRES_FE_H) || \
     defined(HAVE_PG72)) && \
    defined(HAVE_LIBPQ_FE_H)
#define HAVE_POSTGRES
#endif
