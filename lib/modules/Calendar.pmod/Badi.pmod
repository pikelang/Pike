#pike __REAL_VERSION__

//! This is the Badi calendar,
//! used in the Baha'i religion.

inherit .Gregorian:Gregorian;

#include "constants.h"

string calendar_name() { return "Badi"; }

private static mixed __initstuff=lambda()
{
   f_week_day_shortname_from_number="badi_week_day_shortname_from_number";
   f_week_day_name_from_number="badi_week_day_name_from_number";
   f_year_name_from_number="badi_year_name_from_number";
   f_week_day_number_from_name="badi_week_day_number_from_name";

   f_month_day_name_from_number="badi_month_day_name_from_number";
   f_month_name_from_number="badi_month_name_from_number";
   f_month_shortname_from_number="badi_month_shortname_from_number";
   f_month_number_from_name="badi_month_number_from_name";
   f_week_name_from_number="week_name_from_number";
   f_year_number_from_name="badi_year_number_from_name";
   dwim_year=([ "past_lower":0, "past_upper":0,
                "current_century":0, "past_century":0 ]);

}();

static int year_leap_year(int y) 
{ 
   // the beginning of the year is the day of the spring equinox
   // leap years are those where an extra day is needed to get up to the next
   // spring equinox.
   // for now this rule is not in effect and the start of the year is march
   // 21st. leap years are thus compatible with the gregorian calendar for the
   // time being.
   y+=1844;
   return (!(((y)%4) || (!((y)%100) && ((y)%400))));
}

// [y,yjd]
static array year_from_julian_day(int jd)
{
   int d=jd-julian_day_from_year(1);

   int century=(4*d+3)/146097;
   int century_jd=(century*146097)/4;
   int century_day=d-century_jd;
   int century_year=(100*century_day+75)/36525;

   return 
   ({
      century*100+century_year+1,
      julian_day_from_year(1)+century_year*365+century_year/4+century_jd,
   });
}

static int julian_day_from_year(int y)
{
   // this would be march 21st, by taking the preceding year we can reuse the
   // leapyear calculations from the gregorian calendar
   y+=1843;
   return 1721426-286+y*365+y/4-y/100+y/400;
}

static int compat_week_day(int n)
{
   // week starts on saturday
   return n-2;
}

static array(int) year_month_from_month(int y,int m)
{
// [y,m,ndays,myd]

   y+=(m-1)/19;
   m=1+(m-1)%19;

   switch (m)
   {
      case 1..17: return ({y,m,19,(m-1)*19+1});
      case    18: return ({y,m,23+year_leap_year(y),(m-1)*19+1});
      case    19: return ({y,m,19,347+year_leap_year(y)});
   }

   error("Month out of range.\n");
}

static array(int) month_from_yday(int y,int yd)
{
// [month,day-of-month,ndays,month-year-day]
   if (yd<1) 
     return month_from_yday(--y, yd+365+year_leap_year(y));
   int ly = year_leap_year(y);
   switch (yd)
   {
      case 1..323: return ({(yd+18)/19, (yd+18)%19+1, 19, ((yd+18)/19-1)*19+1});
      case 324..342:   return ({18, (yd+18)%19+1, 23+ly, ((yd+18)/19-1)*19+1 });
      case 343..346:   return ({18, yd%19+19, 23+ly, ((yd)/19-1)*19+1 });
      case 347: if(ly) return ({18, yd%19+19, 23+ly, ((yd)/19-1)*19+1 });
      case 348..365:   return ({19, yd-346-ly, 19, 347+ly});
      case 366: if(ly) return ({19, yd-346-ly, 19, 347+ly});
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
   int wjd=jd-(jd+2)%7;  // i am assuming 2 is the offset from the julian day
                         // week starting on monday.

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
   inherit Gregorian::cYear;

   int number_of_months()
   {  
      return 19*n;
   }
}
