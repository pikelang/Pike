/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: dummy.c,v 1.9 2003/03/28 21:33:38 marcus Exp $
*/

/*
 * Glue needed on Solaris if libgcc.a isn't compiled with -fpic.
 *
 * Henrik Grubbström 1997-03-06
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "global.h"

#ifdef HAVE_MYSQL

typedef INT64 _ll_t;
typedef unsigned INT64 _ull_t;

static _ll_t mysql_dummy(_ull_t a, _ull_t b, _ll_t c, _ll_t d) {
  return(a%b+(c%d)+(c/d)+(a/b));
}

#else
static int place_holder;	/* Keep the compiler happy */
#endif /* HAVE_MYSQL */
