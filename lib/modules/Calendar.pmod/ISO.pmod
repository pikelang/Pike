//!
//! module Calendar
//! submodule ISO
//!
//! 	This is the standard western calendar,
//!	which is a derivate of the Gregorian calendar,
//!	but with weeks that starts on monday 
//!	instead of sunday.
//!
//! inherits Gregorian
//!

#pike __REAL_VERSION__

import ".";
inherit Gregorian:Gregorian;

#include "constants.h"

string calendar_name() { return "ISO"; }

private static mixed __initstuff=lambda()
{
   f_week_day_shortname_from_number="week_day_shortname_from_number";
   f_week_day_name_from_number="week_day_name_from_number";
   f_year_name_from_number="year_name_from_number";
   f_week_day_number_from_name="week_day_number_from_name";
}();

static int compat_week_day(int n)
{
   return n%7;
}

static string year_name_from_number(int y)
{
   if (y>0) return ""+y;
   else return (1-y)+" BC";
}

static array(int) week_from_julian_day(int jd)
{
// [year,week,day-of-week,ndays,week-julian-day]

   [int y,int yjd]=year_from_julian_day(jd);
   int yday=jd-yjd+1;

   int k=3+(yjd-3)%7;
   int w=(yday+k-1)/7;
   int wjd=jd-jd%7;

   if (!w) 
   {
// handle the case that the day is in the previous year;
// years previous to years staring on saturday,
//              ...  and leap years starting on sunday
      y--;
      w=52+( (k==3) || ( (k==4) && year_leap_year(y) ) );
   }
   else if (w==53 && k>=5-year_leap_year(y) && k<9-year_leap_year(y))
   {
// handle the case that the week is in the next year
      y++;
      w=1;
   }

   return ({y,w,1+(yjd+yday-1)%7,7,wjd});
}

static array(int) week_from_week(int y,int w)
{
// [year,week,1 (wd),ndays,week-julian-day]

   int yjd=julian_day_from_year(y);
   int wjd=-3+yjd-(yjd+4)%7;

   if (w<1 || w>52) // may or may not be out of this year
      return week_from_julian_day(wjd+w*7);

   return ({y,w,1,7,wjd+w*7});
//   fixme
}

class cYear
{
   inherit Gregorian::cYear;
   TimeRange place(TimeRange what,void|int force)
   {
      if (what->is_day)
      {
	 int wyd=what->yd;
	 if (md==CALUNKNOWN) make_month();
	 if (wyd>=55)
	 {
	    int l1=year_leap_year(what->y);
	    int l2=year_leap_year(y);
	    if (l1||l2)
	    {
	       int ld1=(what->y<2000)?55:60; // 24th or 29th february
	       int ld2=(y<2000)?55:60; // 24th or 29th february

	       if (l1 && wyd==ld1) 
		  if (l2) wyd=ld2;
		  else { if (!force) return 0; }
	       else
	       {
		  if (l1 && wyd>ld1) wyd--;
		  if (l2 && wyd>=ld2) wyd++;
	       }
	    }
	 }
	 if (!force && wyd>number_of_days()) return 0;

	 return Day("ymd_yd",rules,y,yjd,yjd+wyd-1,wyd,what->n);
      }

      return ::place(what);
   }
}

class cMonth
{
   inherit Gregorian::cMonth;

   TimeRange place(TimeRange what,int|void force)
   {
      if (what->is_day)
      {
	 int wmd=what->month_day();
	 if (md==CALUNKNOWN) make_month();
	 if (what->m==2 && m==2 && wmd>=24)
	 {
	    int l1=year_leap_year(what->y);
	    int l2=year_leap_year(y);
	    if (l1||l2)
	    {
	       int ld1=(what->y<2000)?24:29; // 24th or 29th february
	       int ld2=(y<2000)?24:29; // 24th or 29th february

	       if (l1 && wmd==ld1) 
		  if (l2) wmd=ld2;
		  else { if (!force) return 0; }
	       else
	       {
		  if (l1 && wmd>ld1) wmd--;
		  if (l2 && wmd>=ld2) wmd++;
	       }
	    }
	 }
	 if (!force && wmd>number_of_days()) return 0;
	 return Day("ymd_yd",rules,y,yjd,jd+wmd-1,yd+wmd-1,what->n);
      }

      return ::place(what);
   }
}
