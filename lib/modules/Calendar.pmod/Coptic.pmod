#pike __REAL_VERSION__

//! This is the Coptic Orthodox Church calendar,
//! that starts the 11th or 12th September and has
//! 13 months. 
//!
//! @note
//! The (default) names of the months
//! are different then other the emacs calendar;
//! I do not know which ones are used - the difference
//! seem to be only the transcription of the phonetic sounds
//! (B <-> P, etc).
//!
//! I do not know for how long back the calendar is valid,
//! either. My sources claim that the calendar is synchronized
//! with the @[Gregorian] calendar, which is odd.

// inherit some rules from Gregorian, like week numbering
inherit .Gregorian:Gregorian;

string calendar_name() { return "Coptic"; }

private static mixed __initstuff=lambda()
{
   f_week_day_shortname_from_number="gregorian_week_day_shortname_from_number";
   f_week_day_name_from_number="gregorian_week_day_name_from_number";
   f_week_day_number_from_name="gregorian_week_day_number_from_name";

   f_year_name_from_number="coptic_year_name_from_number";
   f_month_name_from_number="coptic_month_name_from_number";
   f_month_shortname_from_number="coptic_month_shortname_from_number";
   f_month_number_from_name="coptic_month_number_from_name";
   f_week_name_from_number="week_name_from_number";
}();

static constant year_offset=-284;
static constant start=1720949;

static array year_from_julian_day(int jd)
{
   int d=jd-start;

   int century=(4*d+3)/146097;
   int century_jd=(century*146097)/4;
   int century_day=d-century_jd;
   int century_year=(100*century_day+75)/36525;

   return 
   ({
      century*100+century_year+year_offset,
      start+century_year*365+century_year/4+century_jd,
   });
}

static int julian_day_from_year(int y)
{
   y-=year_offset;
   return start+y*365+y/4-y/100+y/400;
}

static int year_leap_year(int y) 
{ 
   y-=year_offset;
   werror("%O\n",y);
   return (!(((y)%4) || (!((y)%100) && ((y)%400))));
}

static array(int) year_month_from_month(int y,int m)
{
// [y,m,ndays,myd]

   y+=(m-1)/13;
   m=1+(m-1)%13;

   return ({y,m,m==13?year_leap_year(y)+5:30,1+30*(m-1)});
}

static array(int) month_from_yday(int y,int yd)
{
// [month,day-of-month,ndays,month-year-day]
   int m=(yd-1)/30+1;
   int myd=1+30*(m-1);
   return ({m,1+yd-myd,m==13?year_leap_year(y)+5:30,myd});
}

class cYear
{
   inherit Gregorian::cYear;

   int number_of_months()
   {
      return 13*n;
   }
}
