/* -*- c -*-
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef PIKE_POSTGRES_H

#undef STDC_HEADERS

@TOP@
@BOTTOM@

/* Define if we are running PostgreSQL 7.2 or newer */
#undef HAVE_PG72

/* Define if postgres passed sanity tests. */
#undef HAVE_WORKING_POSTGRES

/* If the PQsetnonblocking function is available, it means we're using
 * PostgreSQL 7.x or newer. This, in turn, means that the interface is
 * thread-safe on a per-connection level (as opposed to non-threadsafe).
 */
#ifdef HAVE_PQSETNONBLOCKING
# define PQ_THREADSAFE 1
#endif

/* End of autoconfigurable section */
#ifdef HAVE_WORKING_POSTGRES
#define HAVE_POSTGRES

/* This is needed to avoid broken <openssl/kssl.h> headerfiles. */
#define OPENSSL_NO_KRB5

/* This is needed to avoid broken prototypes for some builtin functions
 * (cf <server/port.h>). We don't care about the prototypes, since
 * we won't use those functions in this module anyway.
 *	/grubba 2008-03-13
 */
#define HAVE_CRYPT 1
#define HAVE_GETOPT 1
#define HAVE_ISINF 1
#define HAVE_RINT 1
#define HAVE_STRDUP 1
#define HAVE_RANDOM 1
#define HAVE_UNSETENV 1
#define HAVE_SRANDOM 1

/* Time to include stuff. */

/* postgres_fe.h should be used in preference to postgres.h in
 * client code.
 */
#include "override.h"
#ifdef HAVE_POSTGRESQL_SERVER_POSTGRES_FE_H
#include <server/postgres_fe.h>
#elif defined(HAVE_SERVER_POSTGRES_FE_H)
#include <server/postgres_fe.h>
#elif defined(HAVE_POSTGRES_FE_H)
#include <postgres_fe.h>
#elif defined(HAVE_POSTGRESQL_SERVER_POSTGRES_H)
#include <server/postgres.h>
#elif defined(HAVE_SERVER_POSTGRES_H)
#include <server/postgres.h>
#elif defined(HAVE_POSTGRES_H)
#include <postgres.h>
#endif /* HAVE_POSTGRES_FE_H */
#include <libpq-fe.h>

#endif

#endif /* !PIKE_POSTGRES_H */
