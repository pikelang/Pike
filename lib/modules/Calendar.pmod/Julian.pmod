#pike __REAL_VERSION__

//! This is the Julian calendar, conjured up by
//! the old Romans when their calendar were just too
//! weird. It was used by the christians as so far
//! as the 18th century in some parts of the world.
//! (Especially the protestantic and orthodox parts.)
//!
//! @note
//! Don't confuse the @i{julian day@} with the Julian
//! calendar. The former is just a linear numbering of days,
//! used in the Calendar module as a common unit for
//! absolute time.

inherit .Gregorian:Gregorian;

string calendar_name() { return "Julian"; }

static int year_leap_year(int y) 
{ 
   return !((y)%4);
}

// [y,yjd]
static array year_from_julian_day(int jd)
{
   int d=jd-1721058;

   int quad=d/1461;
   int quad_year=max( (d%1461-1)/365, 0);

   return 
   ({
      quad*4+quad_year,
      1721058+1461*quad+365*quad_year+!!quad_year
   });
}

static int julian_day_from_year(int y)
{
   y--;
   return 1721424+y*365+y/4;
}
