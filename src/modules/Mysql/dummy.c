/*
 * Glue needed on Solaris if libgcc.a isn't compiled with -fpic.
 *
 * Henrik Grubbström 1997-03-06
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_MYSQL

typedef long long _ll_t;
typedef unsigned long long _ull_t;

static mysql_dummy(_ull_t a, _ull_t b, _ll_t c, _ll_t d) {
  return(a%b+(c%d)+(c/d)+(a/b));
}

#endif /* HAVE_MYSQL */
