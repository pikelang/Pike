/* -*- mode: Pike; c-basic-offset: 3; -*- */

#pike __REAL_VERSION__

//! This is the standard western calendar, which is a derivate
//! of the Gregorian calendar, but with weeks that starts on
//! Monday instead of Sunday.

inherit Calendar.Gregorian:Gregorian;

#include "constants.h"

string calendar_name() { return "ISO"; }

#define WEEK_MAJORITY 4

private protected mixed __initstuff=lambda()
{
   f_week_day_shortname_from_number="week_day_shortname_from_number";
   f_week_day_name_from_number="week_day_name_from_number";
   f_year_name_from_number="year_name_from_number";
   f_week_day_number_from_name="week_day_number_from_name";
}();

protected int compat_week_day(int n)
{
   return n%7;
}

protected string year_name_from_number(int y)
{
   if (y>0) return ""+y;
   else return (1-y)+" BC";
}

protected array(int) week_from_julian_day(int jd)
{
// [week-year,week,day-of-week,ndays,week-julian-day]

   [int y,int yjd]=year_from_julian_day(jd);
   int yday=jd-yjd+1;
   int wjd=jd-jd%7;

#if 1
   int k=WEEK_MAJORITY+(yjd-WEEK_MAJORITY)%7;
   int w=(yday+k-1)/7;

//     werror("wjd %d: %O %O %O, %O %O %O\n",jd,y,yjd,yday,k,w,wjd);

   if (!w)
   {
// handle the case that the day is in the previous year;
// years previous to years staring on saturday,
//              ...  and leap years starting on sunday

      y--;
      w=52+( (k==WEEK_MAJORITY) ||
	     ( (k==WEEK_MAJORITY+1) && year_leap_year(y) ) );
   }
   else if (w==53 && k>=5-year_leap_year(y) && k<10-year_leap_year(y))
   {
// handle the case that the week is in the next year
      y++;
      w=1;
   }

#else

// Calendar FAQ algorithm (Stefan Potthast):
// not used, does not calculate the week-year

   int d4 = (jd+31741 - (jd %7))%146097 %36524 %1461;
   int L  = d4/1460;
   int d1 = ((d4-L) % 365) + L;

   int w = d1/7 + 1;

//     werror("wjd %d: %O %O %O, %O\n",jd,y,yjd,yday,wjd,w);



#endif

//     werror("wjd %d: = %d, %d, %d, %d, %d\n",
//  	  jd,@({y,w,1+(yjd+yday-1)%7,7,wjd}));

   return ({y,w,1+(yjd+yday-1)%7,7,wjd});
}

protected array(int) week_from_week(int y,int w)
{
// [week-year,week,1 (wd),ndays,week-julian-day]

   int yjd=julian_day_from_year(y);
   int wjd=-WEEK_MAJORITY+yjd-(yjd+WEEK_MAJORITY-1)%7;

//     werror("bip %O %O: %O %O\n",y,w,yjd,wjd);

   if (w<1 || w>52) // may or may not be out of this year
      return week_from_julian_day(wjd+w*7);

   return ({y,w,1,7,wjd+w*7});
//   fixme
}

class cYear
{
   inherit Gregorian::cYear;
   object(TimeRange)|zero place(TimeRange what,void|int force)
   {
      if (what->is_day)
      {
	 int wyd=what->yd;
	 if (md==CALUNKNOWN) make_month();
	 if (wyd>=55)
	 {
	    int l1=year_leap_year(what->y);
	    int l2=year_leap_year(y);
	    if (l1 != l2)
	    {
	       // (Apparently) the leap day was moved from February 24th to
	       // 29th on year 2000 in the ISO calendar.
	       //
	       // This code was overly clever and enforced that move by mapping
	       // the 24th in leap years before 2000 to 29th in 2000 and
	       // following leap years. That leads to nonlinear behavior, i.e.
	       //
	       //                 Day(y1,m1,d1) <= Day(y2,m2,d2)
	       //                              ===>
	       //      Day(y1,m1,d1) + n*Year() <= Day(y2,m2,d2) + n*Year()
	       //
	       // wasn't always true in the ISO calendar. That's bad when doing
	       // date arithmetic.
	       //
	       // That has now been fixed by simply mapping all days one-to-one
	       // between leap years, regardless whether they're before 2000 or
	       // not.
	       //
	       // The exact date for the leap day still has effect when
	       // converting between leap and non-leap years, though.
	       //
	       // /mast
	       if (l1) {
		 int ld1=(what->y<2000)?55:60; // 24th or 29th february
		 if (wyd>ld1) wyd--;
		 else if (wyd==ld1) {
		   if (force) wyd--; // Lossy case - prefer to keep the month.
		   else return 0;
		 }
	       }
	       else {
		  int ld2=(y<2000)?55:60; // 24th or 29th february
		  if (wyd>=ld2) wyd++;
	       }
	    }
	 }
	 if (!force && wyd>number_of_days()) return 0;

	 return Day("ymd_yd",rules,y,yjd,yjd+wyd-1,wyd,what->n);
      }

      return ::place(what, force);
   }
}

class cMonth
{
   inherit Gregorian::cMonth;

   object(TimeRange)|zero place(TimeRange what,int|void force)
   {
      if (what->is_day)
      {
	 int wmd=what->month_day();
	 if (md==CALUNKNOWN) make_month();
	 if (what->m==2 && m==2 && wmd>=24)
	 {
	    int l1=year_leap_year(what->y);
	    int l2=year_leap_year(y);
	    if (l1 != l2)
	    {
	       // See note above about leap day mapping.
	       if (l1) {
		 int ld1=(what->y<2000)?24:29; // 24th or 29th february
		 if (wmd>ld1) wmd--;
		 else if (wmd==ld1) {
		   if (force) wmd--; // Lossy case - prefer to keep the month.
		   else return 0;
		 }
	       }
	       else
	       {
		  int ld2=(y<2000)?24:29; // 24th or 29th february
		  if (wmd>=ld2) wmd++;
	       }
	    }
	 }
	 if (!force && wmd>number_of_days()) return 0;
	 return Day("ymd_yd",rules,y,yjd,jd+wmd-1,yd+wmd-1,what->n);
      }

      return ::place(what, force);
   }
}
