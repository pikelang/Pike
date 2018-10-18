/* -*- c -*-
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef PIKE_POSTGRES_H

#undef STDC_HEADERS

@TOP@
@BOTTOM@

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
#define HAVE_STRDUP 1
#define HAVE_RANDOM 1
#define HAVE_UNSETENV 1
#define HAVE_SRANDOM 1

/* Time to include stuff. */

#include "override.h"
#include <libpq-fe.h>

#endif

#endif /* !PIKE_POSTGRES_H */
