//! 	This is the islamic calendar. Due to some sources,
//!	they decide the first day of the new months on a 
//!	month-to-month basis (sightings of the new moon),
//!	so it's probably not @i{that@} accurate. If
//!	someone can confirm (or deny) accuracy better than that, 
//!	please contact me so I can change this statement.
//!
//! 	It's vaugely based on rules presented in algorithms by
//!	Dershowitz, Reingold and Clamen, 'Calendrical Calculations'.
//!	It is the same that's used in Emacs calendar mode.
//!
//! @bugs
//!	I have currently no idea how the arabic countries
//!	count the week. The same rules as ISO are followed
//!	for now... The time is also suspicious; the *day*
//!	really starts at sunrise (sunset?) and not midnight,
//!	the hours of the day is not correct. Also I don't know
//!	what to call years before 1 - go for "BH"; positive
//!	years are "AH", anno Hegirac.
//!	

#pike __REAL_VERSION__

import ".";
inherit YMD:YMD;

#include "constants.h"

string calendar_name() { return "Islamic"; }

private static mixed __initstuff=lambda()
{
   f_week_day_shortname_from_number="islamic_week_day_shortname_from_number";
   f_week_day_name_from_number="islamic_week_day_name_from_number";
   f_year_name_from_number="islamic_year_name_from_number";

   f_month_day_name_from_number="month_day_name_from_number";
   f_month_name_from_number="islamic_month_name_from_number";
   f_month_shortname_from_number="islamic_month_shortname_from_number";
   f_month_number_from_name="islamic_month_number_from_name";

   f_week_day_number_from_name="gregorian_week_day_number_from_name";
   f_week_name_from_number="week_name_from_number";
}();

int year_leap_year(int y)
{
   return (14+11*y)%30<11;
}

int julian_day_from_year(int y)
{
   return (y-1)*354 + (11*y+3)/30 + 1948440;
}

array(int) year_from_julian_day(int jd)
{
// [y,yjd]
   jd-=1948440;
   int tr=jd/10631; 
   int td=jd-tr*10631;
   int y=(14+td*30)/10631+tr*30+1;

   return ({y,
	    (y-1)*354 + (11*y+3)/30 + 1948440});
}

static array(int) year_month_from_month(int y,int m)
{
// [y,m,ndays,myd]

   y+=(m-1)/12;
   m=1+(m-1)%12;

   switch (m)
   {
      case  1: return ({y,m,30,  1});
      case  2: return ({y,m,29, 31});
      case  3: return ({y,m,30, 60});
      case  4: return ({y,m,29, 90});
      case  5: return ({y,m,30,119});
      case  6: return ({y,m,29,149});
      case  7: return ({y,m,30,178});
      case  8: return ({y,m,29,208});
      case  9: return ({y,m,30,237});
      case 10: return ({y,m,29,267});
      case 11: return ({y,m,30,296});
      case 12: return ({y,m,29+year_leap_year(y),326});
   }			       

   error("month out of range\n");
}

static array(int) month_from_yday(int y,int yd)
{
// [month,day-of-month,ndays,month-year-day]
   int l=year_leap_year(y);

   switch (yd)
   {
      case   1.. 30: return ({ 1,yd,    30,  1});
      case  31.. 59: return ({ 2,yd- 30,29, 31});
      case  60.. 89: return ({ 3,yd- 59,30, 60});
      case  90..118: return ({ 4,yd- 89,29, 90});
      case 119..148: return ({ 5,yd-118,30,119});
      case 149..177: return ({ 6,yd-148,29,149});
      case 178..207: return ({ 7,yd-177,30,178});
      case 208..236: return ({ 8,yd-207,29,208});
      case 237..266: return ({ 9,yd-236,30,237});
      case 267..295: return ({10,yd-266,29,267});
      case 296..325: return ({11,yd-295,30,296});
      case 326..:    return ({12,yd-326,29+year_leap_year(y),326});
   }			       

   error("yday out of range\n");
}

static array(int) week_from_julian_day(int jd)
{
// [year,week,day-of-week,ndays,week-julian-day]

   [int y,int yjd]=year_from_julian_day(jd);
   int yday=jd-yjd+1;

   int k=4+(yjd-4)%7;
   int w=(yday+k)/7;
   int wjd=jd-(jd+1)%7;

// fixme: week number

   return ({y,w,1+(yjd+yday)%7,7,wjd});
}

static array(int) week_from_week(int y,int w)
{
// [year,week,1 (wd),ndays,week-julian-day]

   int yjd=julian_day_from_year(y);
   int wjd=-5+yjd-(yjd+3)%7;

//   fixme: week number

   return ({y,w,1,7,wjd+w*7});
}

static int compat_week_day(int n)
{
   return n%7;
}

static int year_remaining_days(int y,int yday)
{
   return 354+year_leap_year(y)-yday;
}

class cYear
{
   inherit YMD::cYear;

   int number_of_days()
   {
      switch (n)
      {
	 case 0: return 0;
	 case 1: return 354+leap_year();
	 default: 
	    return julian_day_from_year(y+n)-yjd;
      }
   }

   int number_of_months()
   {
      return 12*n;
   }

   int number_of_weeks()
   {
      if (!n) return 1;
//        if (n==1) return 51;
      return 
	 Week("julian",jd)
	 ->range(Week("julian",julian_day_from_year(y+n)-1))
	 ->number_of_weeks();
   }

   TimeRange place(TimeRange what,void|int force)
   {
      if (what->is_day)
      {
	 int yd=what->yd;
	 if (yd==355 && !year_leap_year(y))
	    if (!force) return 0; // can't place
	    else return Day("julian_r",julian_day_from_year(y+1),rules);
	 return Day("ymd_yd",rules,y,yjd,yjd+yd-1,yd,what->n);
      }

      return ::place(what,force);
   }
}

class cDay
{
   inherit YMD::cDay;

   int number_of_months()
   {
      if (n<=1) return 1;
      if (m==CALUNKNOWN) make_month();
      [int zy,int zyjd]=year_from_julian_day(jd+n-1);
      [int zm,int zmd,int znd,int zmyd]=month_from_yday(zy,jd+n-zyjd);
      return zm-m+1+(zy-y)*12;
   }
}

class cMonth
{
   inherit YMD::cMonth;

   static int months_to_month(int y2,int m2)
   {
      return (y2-y)*12+(m2-m);
   }
}

class cWeek
{
   inherit YMD::cWeek;

   static int weeks_to_week(int y2,int w2)
   {
      [int y3,int w3,int wd2,int nd2,int jd2]=week_from_week(y2,w2);
      return (jd2-jd)/7;
   }

   int number_of_days()
   {
      return 7*n;
   }
}
