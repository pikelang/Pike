#pike __REAL_VERSION__

//! This is the standard conservative christian calendar,
//! used regularly in some countries - USA, for instance - and 
//! which derivate - the @[ISO] calendar - is used in most of
//! Europe.

inherit .YMD:YMD;

#include "constants.h"

string calendar_name() { return "Gregorian"; }

private static mixed __initstuff=lambda()
{
   f_week_day_shortname_from_number="gregorian_week_day_shortname_from_number";
   f_week_day_name_from_number="gregorian_week_day_name_from_number";
   f_year_name_from_number="gregorian_year_name_from_number";
   f_week_day_number_from_name="gregorian_week_day_number_from_name";

   f_month_day_name_from_number="month_day_name_from_number";
   f_month_name_from_number="month_name_from_number";
   f_month_shortname_from_number="month_shortname_from_number";
   f_month_number_from_name="month_number_from_name";
   f_week_name_from_number="week_name_from_number";
   f_year_number_from_name="year_number_from_name";
}();

static int year_leap_year(int y) 
{ 
   return (!(((y)%4) || (!((y)%100) && ((y)%400))));
}

// [y,yjd]
static array year_from_julian_day(int jd)
{
   int d=jd-1721426;

   int century=(4*d+3)/146097;
   int century_jd=(century*146097)/4;
   int century_day=d-century_jd;
   int century_year=(100*century_day+75)/36525;

   return 
   ({
      century*100+century_year+1,
      1721426+century_year*365+century_year/4+century_jd,
   });
}

static int julian_day_from_year(int y)
{
   y--;
   return 1721426+y*365+y/4-y/100+y/400;
}

static int compat_week_day(int n)
{
   return n-1;
}

static array(int) year_month_from_month(int y,int m)
{
// [y,m,ndays,myd]

   y+=(m-1)/12;
   m=1+(m-1)%12;

   switch (m)
   {
      case  1: return ({y,m,31,1});
      case  2: return ({y,m,28+year_leap_year(y),32});
      case  3: return ({y,m,31,60+year_leap_year(y)});
      case  4: return ({y,m,30,91+year_leap_year(y)});
      case  5: return ({y,m,31,121+year_leap_year(y)});
      case  6: return ({y,m,30,152+year_leap_year(y)});
      case  7: return ({y,m,31,182+year_leap_year(y)});
      case  8: return ({y,m,31,213+year_leap_year(y)});
      case  9: return ({y,m,30,244+year_leap_year(y)});
      case 10: return ({y,m,31,274+year_leap_year(y)});
      case 11: return ({y,m,30,305+year_leap_year(y)});
      case 12: return ({y,m,31,335+year_leap_year(y)});
   }

   error("Month out of range.\n");
}

static array(int) month_from_yday(int y,int yd)
{
// [month,day-of-month,ndays,month-year-day]
   if (yd<1) return ({12,31+yd,32,335+year_leap_year(y-1)});
   int l=year_leap_year(y);
   if (yd<32) return ({1,yd,31,1});
   yd-=l;
   switch (yd)
   {
      case 0..59: return ({2,yd-31+l,28+l,32});
      case 60..90: return ({3,yd-59,31    ,60+year_leap_year(y)}); 
      case 91..120: return ({4,yd-90,30	  ,91+year_leap_year(y)}); 
      case 121..151: return ({5,yd-120,31 ,121+year_leap_year(y)});
      case 152..181: return ({6,yd-151,30 ,152+year_leap_year(y)});
      case 182..212: return ({7,yd-181,31 ,182+year_leap_year(y)});
      case 213..243: return ({8,yd-212,31 ,213+year_leap_year(y)});
      case 244..273: return ({9,yd-243,30 ,244+year_leap_year(y)});
      case 274..304: return ({10,yd-273,31,274+year_leap_year(y)});
      case 305..334: return ({11,yd-304,30,305+year_leap_year(y)});
      case 335..365: return ({12,yd-334,31,335+year_leap_year(y)});
   }
   error("yday out of range.\n");
}

static array(int) week_from_julian_day(int jd)
{
// [year,week,day-of-week,ndays,week-julian-day]

   [int y,int yjd]=year_from_julian_day(jd);
   int yday=jd-yjd+1;

   int k=4+(yjd-4)%7;
   int w=(yday+k)/7;
   int wjd=jd-(jd+1)%7;

   if (!w) 
   {
// handle the case that the day is in the previous year;
// years previous to years staring on saturday,
//              ...  and leap years starting on sunday
      y--;
      w=52+( (k==4) || ( (k==5) && year_leap_year(y) ) );
   }
   else if (w==53 && k>=6-year_leap_year(y) && k<10-year_leap_year(y))
   {
// handle the case that the week is in the next year
      y++;
      w=1;
   }

   return ({y,w,1+(yjd+yday)%7,7,wjd});
}

static array(int) week_from_week(int y,int w)
{
// [year,week,1 (wd),ndays,week-julian-day]

   int yjd=julian_day_from_year(y);
   int wjd=-5+yjd-(yjd+3)%7;

   if (w<1 || w>52) // may or may not be out of this year
      return week_from_julian_day(wjd+w*7);

   return ({y,w,1,7,wjd+w*7});
//   fixme
}

static int year_remaining_days(int y,int yday)
{
   return 365+year_leap_year(y)-yday;
}

class cYear
{
   inherit YMD::cYear;

   int number_of_days()
   {
      switch (n)
      {
	 case 0: return 0;
	 case 1: return 365+leap_year();
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
      if (n==1) return 53+(yjd%7==5 && leap_year());
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
	 if (yd>=55)
	    switch (year_leap_year(what->y)*10+year_leap_year(y))
	    {
	       case 00:
	       case 11:
		  break;
	       case 10: /* from leap to non-leap */
		  if (yd==55 && !force) return 0; // not this year
		  yd--;
		  break;
	       case 01: /* from non-leap to leap */
		  yd++;
		  break;
	    }
	 return Day("ymd_yd",rules,y,yjd,yjd+yd-1,yd,what->n);
      }

      return ::place(what);
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

// a Gregorian Month can autopromote to a year

   static int months_to_month(int y2,int m2)
   {
      return (y2-y)*12+(m2-m);
   }

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
	       if (l1 && wmd==24) 
		  if (l2) wmd=24;
		  else { if (!force) return 0; }
	       else
	       {
		  if (l1 && wmd>24) wmd--;
		  if (l2 && wmd>24) wmd++;
	       }
	    }
	 }
	 if (!force && wmd>number_of_days()) return 0;
	 return Day("ymd_yd",rules,y,yjd,jd+wmd-1,yd+wmd-1,what->n);
      }

      return ::place(what);
   }
}

class cWeek
{
   inherit YMD::cWeek;

   string nice_print()
   {
      mixed err=catch
      {
	 return 
	    sprintf("%s %s",
		    week_name(),
		    year_name());
      };
      return "error";
   }

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
