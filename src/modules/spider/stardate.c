/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: stardate.c,v 1.19 2004/10/07 22:49:58 nilsson Exp $
*/

#include "global.h"

#include "config.h"

#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "builtin_functions.h"
#include "pike_error.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
#define	FIT(x,y) while((x)<0)(x)+=(y);while((x)>(y))(x)-=(y)



#define MAXPRECISION       7
#define OUTPUTFORMAT       "%%%03d.%df"

/*---------------------------------------------------------------------*
 *TAG (                           julian_day                           );
 *---------------------------------------------------------------------*
 * Returns the New Epoch Julian Day for the given Gregorian calendar
 * day.
 */
double julian_day (int month, int day, int year)
{
  int loc_month;
  int loc_year;
  int a, b, c, d;
  
  loc_month = month;
  loc_year = year;
  
  /* correct for March-based B.C. year */
  if (year < 0) loc_year ++;
  if (month < 3) {
    loc_month = month + 12;
    loc_year --;
  }
  
  /* set up A and B for Julian to Gregorian calendar change */
  if (year < 1582) b = 0;
  else if (year == 1582) {
    if (month < 10) b = 0;
    else if (month == 10) {
      if (day < 15) b = 0;
      else {
	a = loc_year / 100;
	b = 2 - a + (a/4);
      }
    } else {
      a = loc_year / 100;
      b = 2 - a + (a/4);
    }
  } else {
    a = loc_year / 100;
    b = 2 - a + (a/4);		/* century is set up */
  }
  
  /* fudge for year and month */
  c = DO_NOT_WARN((int) (365.25 * loc_year)) - 694025;
  d = DO_NOT_WARN((int) (30.6001 * (loc_month + 1)));
  
  return (double) (b + c + d + day) - .5;
}

/*---------------------------------------------------------------------*
 *TAG (                            sidereal                            );
 *---------------------------------------------------------------------*
 * Returns Greenwich Mean Sidereal Time for the given Greenwich Mean
 * Solar Time on the given NE Julian Day.
 */
double sidereal (double gmt, double jd, int gyear)
{
  register double dayfudge, yrfudge, jhund;
  double initjd, hourfudge, time0, lst;
  
  /* find Julian century at start of year */
  initjd = julian_day (1, 0, gyear);
  jhund = initjd / 36525.0;
  
  /* apply polynomial correction */
  dayfudge = 6.6460656 + (0.051262 + (jhund * 0.00002581)) * jhund;
  yrfudge = 2400.0 * (jhund - ( (double) (gyear - 1900) / 100));
  hourfudge = 24.0 - dayfudge - yrfudge;
  time0 = (jd - initjd) * 0.0657098 - hourfudge;
  
  /* adjust to modulo 24 hours */
  lst = (gmt * 1.002737908) + time0;
  FIT (lst, 24.0);
  return lst;
}

/*! @module spider
 */

/*! @decl string stardate(int time, int precision)
 */
void f_stardate (INT32 args)
{
  int jd, precis=0;
  double gmt, gmst;
  time_t t;
  struct tm *tm;
  char buf[16];
  char fmt[16];

  if (args < 2) 
    Pike_error("Wrong number of arguments to stardate(int, int)\n");
  precis=Pike_sp[-args+1].u.integer;
  t=Pike_sp[-args].u.integer;

  if (precis < 1) precis = 1;
  if (precis > MAXPRECISION) precis = MAXPRECISION;
  tm = gmtime (&t);
  jd = DO_NOT_WARN((int)julian_day(tm->tm_mon + 1, tm->tm_mday,
				   tm->tm_year + 1900));

  gmt = (double) tm->tm_hour +
    tm->tm_min / 60.0 +
    tm->tm_sec / 3600.0;
  gmst = sidereal (gmt, (double) jd, tm->tm_year);
  sprintf (fmt, OUTPUTFORMAT, precis + 6, precis);
  sprintf (buf, fmt, (double) jd + gmst / 24.0);
  pop_n_elems(args);
  push_text(buf);
}

/*! @endmodule
 */
