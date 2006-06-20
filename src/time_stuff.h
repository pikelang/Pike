/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: time_stuff.h,v 1.15 2006/06/20 17:53:04 mast Exp $
*/

#ifndef TIME_STUFF_H
#define TIME_STUFF_H

#ifndef CONFIGURE_TEST
#include "machine.h"
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  if HAVE_TIME_H
#   include <time.h>
#  endif
# endif
#endif

#ifdef HAVE_WINSOCK2_H
# include <winsock2.h>
#elif defined(HAVE_WINSOCK_H)
# include <winsock.h>
#endif

#undef HAVE_SYS_TIME_H
#undef HAVE_TIME_H
#undef TIME_WITH_SYS_TIME

#define my_timercmp(tvp, cmp, uvp) \
  ( (tvp)->tv_sec == (uvp)->tv_sec ? \
    (tvp)->tv_usec cmp (uvp)->tv_usec : \
    (tvp)->tv_sec cmp (uvp)->tv_sec )

#define my_subtract_timeval(X, Y)		\
  do {						\
    struct timeval *_a=(X), *_b=(Y);		\
    _a->tv_sec -= _b->tv_sec;			\
    _a->tv_usec -= _b->tv_usec;			\
    if(_a->tv_usec < 0) {			\
      _a->tv_sec--;				\
      _a->tv_usec+=1000000;			\
    }						\
  } while(0)

#define my_add_timeval(X, Y)			\
  do {						\
    struct timeval *_a=(X), *_b=(Y);		\
    _a->tv_sec += _b->tv_sec;			\
    _a->tv_usec += _b->tv_usec;			\
    if(_a->tv_usec >= 1000000) {		\
      _a->tv_sec++;				\
      _a->tv_usec-=1000000;			\
    }						\
  } while(0)

/* Note: fdlib.c contains a function local_time_to_utc that converts a
 * time_t containing local time to a real one (i.e. UTC). It might be
 * generally useful but is so far slightly MS specific, which is why
 * it still resides in fdlib.c. (It just needs some more of the usual
 * HAVE_XYZ configure test hoopla.) */

#ifndef STRUCT_TIMEVAL_DECLARED
#define STRUCT_TIMEVAL_DECLARED
#endif

#ifndef HAVE_STRUCT_TIMEVAL
struct timeval
{
  long tv_sec;
  long tv_usec;
};
#endif


#endif
