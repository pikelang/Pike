//!	The Discordian calendar, as described on page 34
//!	in the fourth edition of Principia Discordia.
//!
//!	Chaotic enough, it's quite simpler then the Gregorian calendar;
//!	weeks are 5 days, and evens up on a year. Months are 73 days.
//!
//!	The leap day is inserted at the 60th day of the first month 
//!	(Chaos), giving the first month 74 days. The description of
//!	the calendar is a "perpetual date converter from the gregorian
//!	to the POEE calendar", so the leap years are the same as
//!	the gregorians.
//!
//!	The Principia calls months "seasons", but for simplicity I 
//!	call them months in this calendar.
//!
//!	If anyone knows more about how to treat the leap day - now it is 
//!	inserted in the month and week where it lands, rather then being
//!	separated from month and weeks, I'm interested to know.
//!
//!	- Mirar, Pope of POEE.

// the discordian calendar follows the gregorian years, very practical ;)

#pike __REAL_VERSION__

import ".";
inherit Gregorian:Gregorian;

#include "constants.h"

string calendar_name() { return "Discordian"; }

private static mixed __initstuff=lambda()
{
// language setup
   f_week_day_shortname_from_number=
      "discordian_week_day_shortname_from_number";
   f_week_day_name_from_number="discordian_week_day_name_from_number";
   f_year_name_from_number="discordian_year_name_from_number";
   f_month_name_from_number="discordian_month_name_from_number";
   f_month_shortname_from_number="discordian_month_shortname_from_number";
   f_month_number_from_name="discordian_month_number_from_name";
   f_week_name_from_number="discordian_week_name_from_number";
   f_week_day_number_from_name="discordian_week_day_number_from_name";
}();

static int compat_week_day(int n)
{
   return n; // N/A
}

// almost as gregorian
static array year_from_julian_day(int jd)
{
   array a=::year_from_julian_day(jd);
   return ({a[0]+1166,a[1]});
}

static int julian_day_from_year(int y)
{
   return ::julian_day_from_year(y-1166);
}

static int year_leap_year(int y) 
{ 
   return ::year_leap_year(y-1166);
}

static array(int) year_month_from_month(int y,int m)
{
// [y,m,ndays,myd]

   y+=(m-1)/5;
   m=1+(m-1)%5;

   switch (m)
   {
      case 1: return ({y,m,73+year_leap_year(y),1});
      case 2..5: return ({y,m,73,1+73*(m-1)+year_leap_year(y)});
   }

   error("month out of range\n");
}

static array(int) month_from_yday(int y,int yd)
{
// [month,day-of-month,ndays,month-year-day]
   int l=year_leap_year(y);

   if (yd<=73+l) return ({1,yd,73+l,1});
   yd-=l;
   int m=(yd+72)/73;
   return ({m,yd-(m-1)*73,73,(m-1)*73+l+1});
}

static array(int) week_from_julian_day(int jd)
{
// [year,week,day-of-week,ndays,week-julian-day]

   [int y,int yjd]=year_from_julian_day(jd);
   int yday=jd-yjd+1;
   int l=year_leap_year(y);

   if (l)
      if (yday==60)
	 return ({y,12,6,6,yjd+55});
      else if (yday>55 && yday<62)
	 return ({y,12,(yday==61)?5:yday-55,6,yjd+55});
      else if (yday>60) 
	 yday--;
      else
	 l=0;
   
   int w=(yday+4)/5;
   return ({y,w,(yday-1)%5+1,5,yjd+(w-1)*5+l});
}

static array(int) week_from_week(int y,int w)
{
// [year,week,1 (wd),ndays,week-julian-day]
   y+=(w-1)/73;
   w=1+(w-1)%73;
   int yjd=julian_day_from_year(y);

   int l=year_leap_year(y);
   if (!l||w<12) return ({y,w,1,5,yjd+(w-1)*5});
   return week_from_julian_day(yjd+(w-1)*5+(l&&w>12));
}

static int year_remaining_days(int y,int yday)
{
   return 365+year_leap_year(y)-yday;
}

class cYear
{
   inherit Gregorian::cYear;

   int number_of_weeks()
   {
      return 73*n;
   }

   TimeRange place(TimeRange what)
   {
      if (what->is_day)
      {
	 int yd=what->yd;
	 if (yd>=59)
	    switch (year_leap_year(what->y)*10+year_leap_year(y))
	    {
	       case 00:
	       case 11:
		  break;
	       case 10: /* from leap to non-leap */
		  if (yd==59) return 0; // not this year
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

   int year_no() { return y+1166; }
}

class cDay
{
   inherit Gregorian::cDay;

   string nice_print()
   {
      mixed err=catch
      {
	 if (m==CALUNKNOWN) make_month();
	 if (wd==CALUNKNOWN) make_week();
	 return 
	    sprintf("%s %d of %s %s",
		    week_day_shortname(),
		    md,month_shortname(),
		    year_name());
      };
      return "error";
   }

   int year_no() { return y+1166; }
}

class cMonth
{
   inherit Gregorian::cMonth;

   void create(mixed ...args)
   {
      ::create(@args);
      if (yjd+yd-1!=jd) error("yjd=%O yday=%O jd=%O != %O\n",
				yjd,yd,jd,jd+yd-1);

   }

   TimeRange place(TimeRange what,int|void force)
   {
      if (what->is_day)
      {
	 int wmd=what->month_day();
	 if (md==CALUNKNOWN) make_month();
	 if (what->m==1 && m==1 && wmd>=60)
	 {
	    int l1=year_leap_year(what->y);
	    int l2=year_leap_year(y);
	    if (l1||l2)
	    {
	       if (l1 && wmd==60) 
		  if (l2) wmd=60;
		  else { if (!force) return 0; }
	       else
	       {
		  if (l1 && wmd>60) wmd--;
		  if (l2 && wmd>60) wmd++;
	       }
	    }
	 }
	 if (!force && wmd>number_of_days()) return 0;
	 return Day("ymd_yd",rules,y,yjd,jd+wmd-1,yd+wmd-1,what->n);
      }

      return ::place(what);
   }

   int year_no() { return y+1166; }
}

class cWeek
{
   inherit Gregorian::cWeek;

   static int weeks_to_week(int y2,int w2)
   {
      return (y2-y)*73+w2-w;
   }

   int number_of_days()
   {
      [int y2,int w2,int wd2,int nd2,int jd2]=week_from_week(y,w+n);
      return jd2-jd;
   }

   int year_no() { return y+1166; }
}
