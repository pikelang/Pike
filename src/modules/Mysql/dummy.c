/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: dummy.c,v 1.14 2004/04/12 15:47:16 grubba Exp $
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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <stdio.h>

typedef INT64 _ll_t;
typedef unsigned INT64 _ull_t;

_ll_t mysql_dummy_dum_dum(_ull_t a, _ull_t b, _ll_t c, _ll_t d) {
#ifdef HAVE_LDIV
  ldiv(10, 3);
#endif
#ifdef HAVE_OPEN
  open(0, 0);
#endif
#ifdef HAVE_SOPEN
  sopen(0, 0, 0);
#endif
#ifdef HAVE_CLOSE
  close(0);
#endif
#ifdef HAVE_READ
  read(0, 0, 0);
#endif
#ifdef HAVE_FILENO
  /* This is a macro on AIX 4.3, so we need a proper argument. */
  fileno(stderr);
#endif
#ifdef HAVE_PUTS
  puts(0);
#endif
#ifdef HAVE_FGETS
  fgets(0, 0, 0);
#endif
#ifdef HAVE_MESSAGEBOXA
  MessageBoxA(0, 0, 0, 0);
#endif

  return(a%b+(c%d)+(c/d)+(a/b));
}

#else
static int place_holder;	/* Keep the compiler happy */
#endif /* HAVE_MYSQL */
