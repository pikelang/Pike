/* -*- mode: Pike; c-basic-offset: 3; -*- */

//!
//! module Calendar
//! submodule YMD
//!
//! base for all Roman-kind of Calendars,
//! ie, one with years, months, weeks and days
//!
//! inherits Time

#charset iso-8859-1
#pike __REAL_VERSION__

//  #pragma strict_types

inherit Calendar.Time:Time;

#include "constants.h"

// ----------------
// virtual methods to tell how this calendar works
// ----------------

protected array(int) year_from_julian_day(int jd);
protected int julian_day_from_year(int year);
protected int year_remaining_days(int y,int yday);

protected array(int) year_month_from_month(int y,int m); // [y,m,ndays,myd]
protected array(int) month_from_yday(int y,int yday); // [m,day-of-month,ndays,myd]

protected array(int) week_from_week(int y,int w);   // [wy,w,wd,ndays,wjd]
protected array(int) week_from_julian_day(int jd);  // [wy,w,wd,ndays,wjd]

protected string f_month_name_from_number;
protected string f_month_shortname_from_number;
protected string f_month_number_from_name;
protected string f_month_day_name_from_number;
protected string f_week_name_from_number;
protected string f_week_day_number_from_name;
protected string f_week_day_shortname_from_number;
protected string f_week_day_name_from_number;
protected string f_year_name_from_number;
protected string f_year_number_from_name;


protected int(0..1) year_leap_year(int y);

protected int compat_week_day(int n);

// Polynomial terms for calculating the difference between TDT and UTC.
//
// The polynomials have been taken from NASA:
// @url{http://eclipse.gsfc.nasa.gov/SEhelp/deltatpoly2004.html@}
//
// Each entry is an @expr{array(float)@}:
// @array
//   @elem float 0
//     End year for the range.
//   @elem float 1
//     Year offset.
//   @elem float 2
//     Year divisor.
//   @elem float 3..
//     Polynomial factors with the highest exponent last.
// @endarray
protected constant deltat_polynomials = ({
  ({ -500.0, -1820.0, 100.0,
     -20.0, 0.0, 32.0, }),
  ({ 500.0, 0.0, 100.0,
     10583.6, -1014.41, 33.78311, -5.952053,
     -0.1798452, 0.022174192, 0.0090316521, }),
  ({ 1600.0, -1000.0, 100.0,
     1574.2, -556.01, 71.23472, 0.319781,
     -0.8503463, -0.005050998, 0.0083572073, }),
  ({ 1700.0, -1600.0, 1.0,
     120.0, -0.9808, -0.01532, 0.000140272128, }),
  ({ 1800.0, -1700.0, 1.0,
     8.83, 0.1603, -0.0059285, 0.00013336, -0.000000851788756, }),
  ({ 1860.0, -1800.0, 1.0,
     13.72, -0.332447, 0.0068612, 0.0041116, -0.00037436,
     0.0000121272, -0.0000001699, 0.000000000875, }),
  ({ 1900.0, -1860.0, 1.0,
     7.62, 0.5737, -0.251754, 0.01680668, -0.0004473624, 0.000004288643, }),
  ({ 1920.0, -1900.0, 1.0,
     -2.79, 1.494119, -0.0598939, 0.0061966, -0.000197, }),
  ({ 1941.0, -1920.0, 1.0,
     21.20, 0.84493, -0.076100, 0.0020936, }),
  ({ 1961.0, -1950.0, 1.0,
     29.07, 0.407, -0.0042918455, 0.0003926188, }),
  ({ 1986.0, -1975.0, 1.0,
     45.45, 1.067, -0.0038461538, -0.001392758, }),
  ({ 2005.0, -2000.0, 1.0,
     63.86, 0.3345, -0.060374, 0.0017275, 0.000651814, 0.00002373599, }),
  ({ 2050.0, -2000.0, 1.0,
     62.92, 0.32217, 0.005589, }),
  ({ 2150.0, -1820.0, 100.0,
     -20.0-185.724, 56.28, 32.0, }),
  ({ Math.inf, -1820.0, 100.0,
     -20.0, 0.0, 32.0, }),
});

//! method float deltat(int unadjusted_utc)
//! Terrestrial Dynamical Time difference from standard time.
//!
//! returns
//!   An approximation of the difference between TDT and UTC
//!   in fractional seconds at the specified time.
//!
//! The zero point is 1901-06-25T14:23:01 UTC
//! (unix time -2162281019), ie the accumulated number
//! of leap seconds since then is returned.
//!
//! note
//!   The function is based on polynomials provided by NASA,
//!   and the result may differ from actual for dates after 2004.
float deltat(int unadjusted_utc)
{
  // Approximation of the year. This ought to be good enough for
  // most purposes given the uncertainty in the table values.
  // 31556952 == 365.2425 * 24 * 60 * 60.
  float y = 1970.0 + unadjusted_utc/31556952.0;

  array(float) polynomial;
  int l, c, h = sizeof(deltat_polynomials);
  do {
    c = (l + h)/2;
    polynomial = deltat_polynomials[c];
    if (y < polynomial[0]) h = c;
    else l = c + 1;
  } while (l < h);
  polynomial = deltat_polynomials[l];

  float u = (y + polynomial[1])/polynomial[2];

  float deltat = 0.0;
  float p = 1.0;
  foreach(polynomial; int i; float factor) {
    if (i < 3) continue;
    deltat += factor * p;
    p = p * u;
  }
  return deltat;
}

//------------------------------------------------------------------------
//! class YMD
//! 	Base (virtual) time period of the Roman-kind of calendar.
//! inherits TimeRange
//------------------------------------------------------------------------

class YMD
{
   inherit TimeRange;

// --- generic for all YMD:

   // The correct year to use in context with yjd, yd, m and md is y.
   // The correct year to use in context with w is wy.
   // The correct year to use without context is year_no() (= either y or wy).

   int y;   // year
   int yjd; // julian day of the first day of the year

   int n;   // number of this in the period

   int jd;  // julian day of first day
   int yd;  // day of year (1..)
   int m;   // [*] month of year (1..12?), like
   int md;  // [*] day of month (1..)
   int wy;  // [*] week year
   int w;   // [*] week of week year (1..)
   int wd;  // [*] day of week (1..7?)

   int mnd=CALUNKNOWN;  // [*] days in current month
   int utco=CALUNKNOWN; // [*] distance to UTC
   string tzn=0;        // timezone name

   // [*]: might be uninitialized (CALUNKNOWN)

   Calendar.Ruleset rules;
   constant is_ymd=1;

// ----------------------------------------
// basic Y-M-D stuff
// ----------------------------------------

   void create_now()
   {
      rules=default_rules;
      create_unixtime_default(time());
   }

   void create_unixtime_default(int unixtime)
   {
// 1970-01-01 is julian day 2440588
      create_julian_day( 2440588+unixtime/86400 );
// we can't reuse this; it might not be start of day
      [int mutco,string mtzn]=rules->timezone->tz_ux(unixtime);
      int uxo=unixtime%86400-mutco;
      if (uxo<0)
	 create_julian_day( 2440588+unixtime/86400-1 );
      else if (uxo>=86400)
	 create_julian_day( 2440588+unixtime/86400+1 );
      else if (uxo==0)
	 utco=mutco,tzn=mtzn; // reuse, it *is* start of day
   }

   void make_month() // set m and md from y and yd
   {
      int myd;
      [m,md,mnd,myd]=month_from_yday(y,yd);
   }

   void make_week() // set wy, w and wd from jd
   {
      int wnd,wjd;
      [wy,w,wd,wnd,wjd]=week_from_julian_day(jd);
   }

   protected int __hash() { return jd; }

// --- query

//! method float fraction_no()
//! method int hour_no()
//! method int julian_day()
//! method int leap_year()
//! method int minute_no()
//! method int month_day()
//! method int month_days()
//! method int month_no()
//! method int second_no()
//! method int utc_offset()
//! method int week_day()
//! method int week_no()
//! method int year_day()
//! method int year_no()
//! method string month_name()
//! method string month_shortname()
//! method string month_day_name()
//! method string week_day_name()
//! method string week_day_shortname()
//! method string week_name()
//! method string year_name()
//! method string tzname()
//! method string tzname_iso()

   int julian_day()
   {
      return jd;
   }

//! method int unix_time()
//!     Returns the unix time integer corresponding to the start
//!	of the time range object. (An unix time integer is UTC.)

   int unix_time()
   {
// 1970-01-01 is julian day 2440588
      int ux=(jd-2440588)*86400;
      if (utco==CALUNKNOWN)
	 [utco,tzn]=rules->timezone->tz_jd(jd);
      return ux+utco;
   }

   int utc_offset()
   {
      if (utco==CALUNKNOWN)
	 [utco,tzn]=rules->timezone->tz_jd(jd);
      return utco;
   }

   string tzname()
   {
      if (!tzn)
	 [utco,tzn]=rules->timezone->tz_jd(jd);
      return tzn;
   }

   string tzname_iso()
   {
      int u=utc_offset();
      if (!(u%3600))
	 return sprintf("UTC%+d",-u/3600);
      if (!(u%60))
	 return
	    (u<0)
	    ?sprintf("UTC+%d:%02d",-u/3600,(-u/60)%60)
	    :sprintf("UTC-%d:%02d",u/3600,(u/60)%60);
      return
	 (u<0)
	 ?sprintf("UTC+%d:%02d:%02d",-u/3600,(-u/60)%60,(-u)%60)
	 :sprintf("UTC-%d:%02d:%02d",u/3600,(u/60)%60,u%60);
   }

   int year_no()
   {
      return y>0?y:-1+y;
   }

   int month_no()
   {
      if (m==CALUNKNOWN) make_month();
      return m;
   }

   int week_no()
   {
      if (w==CALUNKNOWN) make_week();
      return w;
   }

   int month_day()
   {
      if (md==CALUNKNOWN) make_month();
      return md;
   }

   int month_days()
   {
      if (mnd==CALUNKNOWN) make_month();
      return mnd;
   }

   int week_day()
   {
      if (wd==CALUNKNOWN) make_week();
      return wd;
   }

   int year_day()
   {
      return yd;
   }

   string year_name()
   {
      return rules->language[f_year_name_from_number](year_no());
   }

   string week_name()
   {
      if (w==CALUNKNOWN) make_week();
      return rules->language[f_week_name_from_number](w);
   }

   string month_name()
   {
      if (m==CALUNKNOWN) make_month();
      return rules->language[f_month_name_from_number](m);
   }

   string month_shortname()
   {
      if (m==CALUNKNOWN) make_month();
      return rules->language[f_month_shortname_from_number](m);
   }

   string month_day_name()
   {
      if (mnd==CALUNKNOWN) make_month();
      return rules->language[f_month_day_name_from_number](md,mnd);
   }

   string week_day_name()
   {
      if (wd==CALUNKNOWN) make_week();
      return rules->language[f_week_day_name_from_number](wd);
   }

   string week_day_shortname()
   {
      if (wd==CALUNKNOWN) make_week();
      return rules->language[f_week_day_shortname_from_number](wd);
   }

   int leap_year() { return year_leap_year(year_no()); }

   int hour_no() { return 0; }
   int minute_no() { return 0; }
   int second_no() { return 0; }
   float fraction_no() { return 0.0; }

//! method mapping datetime()
//!     This gives back a mapping with the relevant
//!	time information (representing the start of the period);
//!	<pre>
//!	 ([ "year":     int        // year number (2000 AD=2000, 1 BC==0)
//!	    "month":    int(1..)   // month of year
//!	    "day":      int(1..)   // day of month
//!	    "yearday":  int(0..)   // day of year
//!	    "week":	int(1..)   // week of year
//!	    "week_day": int(0..)   // day of week
//!	    "timezone": int        // offset to utc, including dst
//!
//!	    "unix":     int        // unix time
//!	    "julian":   int        // julian day
//!      // for compatibility:
//!	    "hour":     0          // hour of day, including dst
//!	    "minute":   0          // minute of hour
//!	    "second":   0          // second of minute
//!	    "fraction": 0.0        // fraction of second
//!	 ]);
//!	</pre>
//!
//! note:
//!	Day of week is compatible with old versions,
//!	ie, 0 is sunday, 6 is saturday, so it shouldn't be
//!	used to calculate the day of the week with the given
//!	week number. Year day is also backwards compatible,
//!	ie, one (1) less then from the year_day() function.
//!
//! note:
//!     If this function is called in a Week object that begins with
//!     the first week of a year, it returns the previous year if that
//!     is where the week starts. To keep the representation
//!     unambiguous, the returned week number is then one more than
//!     the number of weeks in that year.
//!
//!     E.g. Week(2008,1)->datetime() will return year 2007 and week
//!     53 since the first week of 2008 starts in 2007.

   mapping datetime(void|int skip_stuff)
   {
      if (m==CALUNKNOWN) make_month();
      if (w==CALUNKNOWN) make_week();

      int week;
      if (y == wy)
	week = w;
      else
	// w is the first week of the year and it begins in the
	// previous year, so we know y == wy - 1 and w == 1.
	week = week_from_week (y, 0)[1] + 1;

      if (skip_stuff) // called from timeofday
	 return ([ "year":     y,
		   "month":    m,
		   "day":      md,
		   "yearday":  yd-1,
		   "week":     week,
		   "week_day": compat_week_day(wd)
	 ]);
      else
      {
	 return ([ "year":     y,
		   "month":    m,
		   "day":      md,
		   "yearday":  yd-1,
		   "week":     week,
		   "week_day": compat_week_day(wd),
		   "timezone": utc_offset(),
		   "julian":   jd,
		   "unix":     unix_time(),
	     // for compatibility:
		   "hour":     0,
		   "minute":   0,
		   "second":   0,
		   "fraction": 0.0
	 ]);
      }
   }

// --- string format ----

//! method string format_iso_ymd();
//! method string format_ymd();
//! method string format_ymd_short();
//! method string format_ymd_xshort();
//! method string format_mdy();
//! method string format_iso_week();
//! method string format_iso_week_short();
//! method string format_week();
//! method string format_week_short();
//! method string format_month();
//! method string format_month_short();
//! method string format_iso_time();
//! method string format_time();
//! method string format_time_short();
//! method string format_time_xshort();
//! method string format_mtime();
//! method string format_xtime();
//! method string format_tod();
//! method string format_todz();
//! method string format_xtod();
//! method string format_mod();
//!	Format the object into nice strings;
//!	<pre>
//!	iso_ymd        "2000-06-02 (Jun) -W22-5 (Fri)" [2]
//!	ext_ymd        "Friday, 2 June 2000" [2]
//!     ymd            "2000-06-02"
//!     ymd_short      "20000602"
//!     ymd_xshort     "000602" [1]
//!	iso_week       "2000-W22"
//!	iso_week_short "2000W22"
//!	week           "2000-w22" [2]
//!	week_short     "2000w22" [2]
//!	month          "2000-06"
//!	month_short    "200006" [1]
//!	iso_time       "2000-06-02 (Jun) -W22-5 (Fri) 00:00:00 UTC+1" [2]
//!	ext_time       "Friday, 2 June 2000, 00:00:00" [2]
//!	ctime          "Fri Jun  2 00:00:00 2000\n" [2] [3]
//!	http           "Fri, 02 Jun 2000 00:00:00 GMT" [4]
//!     time           "2000-06-02 00:00:00"
//!	time_short     "20000602 00:00:00"
//!	time_xshort    "000602 00:00:00"
//!	iso_short      "2000-06-02T00:00:00"
//!     mtime          "2000-06-02 00:00"
//!     xtime          "2000-06-02 00:00:00.000000"
//!     tod            "00:00:00"
//!     tod_short      "000000"
//!     todz           "00:00:00 CET"
//!     todz_iso       "00:00:00 UTC+1"
//!     xtod           "00:00:00.000000"
//!     mod            "00:00"
//!	</pre>
//!	<tt>[1]</tt> note conflict (think 1 February 2003)
//!	<br><tt>[2]</tt> language dependent
//!	<br><tt>[3]</tt> as from the libc function ctime()
//!	<br><tt>[4]</tt> as specified by the HTTP standard;
//!	not language dependent.
//!
//!	The iso variants aim to be compliant with ISO-8601.

   string format_iso_ymd()
   {
      if (m==CALUNKNOWN) make_month();
      if (w==CALUNKNOWN) make_week();
      return sprintf("%04d-%02d-%02d (%s) -W%02d-%d (%s)",
		     y,m,md,
		     month_shortname(),
		     w,wd, // fixme - what weekday?
		     week_day_shortname());
   }

   string format_ext_ymd()
   {
      if (m==CALUNKNOWN) make_month();
      return sprintf("%s, %s %s %s",
		     week_day_name(),
		     month_day_name(),month_name(),year_name());
   }

   string format_ctime()
   {
      return sprintf("%s %s %2d 00:00:00 %s\n",
		     week_day_shortname(),
		     month_shortname(),
		     md,
		     year_name());
   }

   string format_http()
   {
      if (wd==CALUNKNOWN) make_week();
      if (md==CALUNKNOWN) make_month();

      return
        sprintf("%s, %02d %s %04d 00:00:00 GMT",
                ("SunMonTueWedThuFriSat"/3)[compat_week_day(wd)],
                md,
                ("zzzJanFebMarAprMayJunJulAugSepOctNovDec"/3)[m],
		y);
   }

   string format_ext_time_short()
   {
      if (wd==CALUNKNOWN) make_week();
      if (md==CALUNKNOWN) make_month();

      return
         sprintf("%s, %d %s %d 00:00:00 GMT",
                     week_day_shortname(),
		     month_day(),month_shortname(),y);
   }

   string format_ymd()
   {
      if (m==CALUNKNOWN) make_month();
      return sprintf("%04d-%02d-%02d",y,m,md);
   }

   string format_mdy()
   {
      if (m==CALUNKNOWN) make_month();
      return sprintf("%d/%d/%d",m,md,y%100);
   }

   string format_ymd_short()
   {
      if (m==CALUNKNOWN) make_month();
      return sprintf("%04d%02d%02d",y,m,md);
   }

   string format_ymd_xshort()
   {
      if (m==CALUNKNOWN) make_month();
      return sprintf("%02d%02d%02d",y%100,m,md);
   }

   string format_iso_week()
   {
      if (w==CALUNKNOWN) make_week();
      return sprintf("%04d-W%02d",wy,w);
   }

   string format_iso_week_short()
   {
      if (w==CALUNKNOWN) make_week();
      return sprintf("%04d%02d",wy,w);
   }

   string format_week()
   {
      if (w==CALUNKNOWN) make_week();
      return sprintf("%04d-%s",wy,week_name());
   }

   string format_week_short()
   {
      if (w==CALUNKNOWN) make_week();
      return sprintf("%04d%s",wy,week_name());
   }

   string format_month()
   {
      if (m==CALUNKNOWN) make_month();
      return sprintf("%04d-%02d",y,m);
   }

   string format_month_short()
   {
      if (m==CALUNKNOWN) make_month();
      return sprintf("%04d%02d",y,m);
   }

   string format_iso_time()
   {
      return format_iso_ymd()+" 00:00:00";
   }

   string format_ext_time()
   {
      return format_ext_ymd()+" 00:00:00";
   }

   string format_time()
   {
      return format_ymd()+" 00:00:00";
   }

   string format_time_short()
   {
      return format_ymd_short()+" 00:00:00";
   }

   string format_iso_short()
   {
      return format_ymd_short()+"T00:00:00";
   }

   string format_time_xshort()
   {
      return format_ymd_xshort()+" 00:00:00";
   }

   string format_mtime()
   {
      return format_ymd()+" 00:00";
   }

   string format_xtime()
   {
      return format_ymd()+" 00:00:00.000000";
   }

   string format_tod()
   {
      return "00:00:00";
   }

   string format_tod_short()
   {
      return "000000";
   }

   string format_todz()
   {
      return "00:00:00 "+tzname();
   }

   string format_todz_iso()
   {
      return "00:00:00 "+tzname_iso();
   }

   string format_mod()
   {
      return "00:00";
   }

   string format_xtod()
   {
      return "00:00:00.000000";
   }

   string format_nice();
   string format_nicez()
   {
      return format_nice()+" "+tzname();
   }

   string tzname_utc_offset()
   {
      int u=utc_offset();
      return sprintf("%+03d%02d", -u/3600, abs(u)/60%60);
   }

   string format_smtp()
   {
      if (m==CALUNKNOWN) make_month();
      return sprintf("%s, %s %s %s 00:00:00 %s",
		     week_day_shortname(),
		     month_day_name(),month_shortname(),year_name(),
		     tzname_utc_offset());
   }

   string format_commonlog()
   {
      if (m==CALUNKNOWN) make_month();
      return sprintf("%02d/%s/%d:%s %s",
		     month_day(), month_shortname(), year_no(), format_tod(),
		     tzname_utc_offset());
   }

   string format_elapsed()
   {
      return sprintf("%dd",number_of_days());
   }

// --- size and move ---

   protected TimeRange _set_size(int n,TimeRange t)
   {
      if (t->is_timeofday)
	 return second()->set_size(n,t);

      if (yd==1 && t->is_year)
	 return Year("ymd_y",rules,y,yjd,t->n*n)
	    ->autopromote();

// months are even on years
      if (t->is_year || t->is_month)
      {
	 if (md==CALUNKNOWN) make_month();
	 if (md==1)
	    return Month("ymd_yjmw",rules,y,yjd,jd,m,
			 t->number_of_months()*n,wd,w,wy)
	       ->autopromote();
      }

// weeks are not
      if (t->is_week)
      {
	 if (wd==CALUNKNOWN) make_week();
	 if (wd==1) return Week("ymd_yjwm",rules,y,yjd,jd,wy,w,t->n*n,md,m,mnd);
      }

// fallback on days
      if (t->is_ymd)
	 return Day("ymd_ydmw",rules,y,yjd,jd,yd,
		    n*t->number_of_days(),m,md,wy,w,wd,mnd);

      error("set_size: incompatible class %O\n",
	    object_program(t));
   }

   protected TimeRange _add(int _n,TimeRange step)
   {
      if (step->is_ymd)
	 return _move(_n,step);
      if (step->is_timeofday)
         if (n)
	    return second()->range(second(-1))->add(_n,step);
         else
            return second()->beginning()->add(_n,step);

      error("add: incompatible class %O\n",
	    object_program(step));
   }

   array(int(-1..1)) _compare(TimeRange with)
   {
      if (objectp(with))
	 if (with->is_timeofday)
	 {
      // wrap
	    array(int(-1..1)) cmp=with->_compare(this);

	    return ({-cmp[0],
		     -cmp[2],
		     -cmp[1],
		     -cmp[3]});
	 }
	 else if (with->is_ymd || with->julian_day)
	 {
#define CMP(A,B) ( ((A)<(B))?-1:((A)>(B))?1:0 )

	    int b1=julian_day();
	    int e1=b1+number_of_days();

	    int b2=with->julian_day();
	    int e2=b2+with->number_of_days();

	    return ({ CMP(b1,b2),CMP(b1,e2),CMP(e1,b2),CMP(e1,e2) });
	 }
      return ::_compare(with);
   }

// --- to other YMD

// years

   int number_of_years()
   {
      int m=number_of_days();
      if (m<=1 || m+yd-1<year()->number_of_days()) return 1;
      return 1+y-year_from_julian_day(jd+m-1)[0];
   }

   array(cYear) years(void|int from, void|int to)
   {
      int n=number_of_years();

      if (undefinedp (from)) {
	from = 1;
	to = n;
      }
      else
	 if (undefinedp (to))
	    error("Illegal numbers of arguments to years()\n");
	 else
	 {
	    if (from>n) return ({}); else if (from<1) from=1;
	    if (to>n) to=n; else if (to<from) return ({});
	 }

      return map(enumerate(1+to-from,1,y+from-1),
		 lambda(int x)
		 { return Year("ymd_yn",rules,x,1); });
   }

   cYear year(int m = 1)
   {
      if (!n&&m==-1)
	 return Year("ymd_y",rules,y,yjd,1);

      if (m<0) m += 1 + number_of_years();

      array(TimeRange) res=years(m,m);
      if (sizeof(res)==1) return res[0];
      error("Not in range (Year 1..%d exist)\n",
	    number_of_years());
   }


// days

//! method int number_of_days()
//!	Get the number of days in the current range.

   int number_of_days();

//! method array(Day) days(int|void from, int|void to)
//!	Get the days in the current range.

   array(cDay) days(void|int from, void|int to)
   {
      int n=number_of_days();

      if (undefinedp (from)) {
	from = 1;
	to = n;
      }
      else
	 if (undefinedp (to))
	    error("Illegal number of arguments to days()\n");
	 else
	 {
	    if (from>n) return ({}); else if (from<1) from=1;
	    if (to>n) to=n; else if (to<from) return ({});
	 }

      int zy=y;
      int zyd=yd+from-1;
      int zjd=jd+from-1;
      int zyjd=yjd;
      array(cDay) res=({});

      to-=from-1;

      for (;;)
      {
	 int rd=year_remaining_days(zy,zyd)+1;

	 if (rd>0)
	 {
	    if (rd>to) rd=to;
	    res+=map(enumerate(rd,1,zyd),
		     lambda(int x)
		     { return Day("ymd_yd",rules,zy,zyjd,zyjd+x-1,x,1); });
	    if (rd==to) break;
	    zjd+=rd;
	    to-=rd;
	 }

	 [zy,zyjd]=year_from_julian_day(zjd);
	 zyd=zjd-zyjd+1;
      }
      return res;
   }

//! method Day day()
//! method Day day(int n)
//!	Get day number n in the current range.
//!
//!     If n is negative, it is counted from the end of the range.

   cDay day(int m = 1, mixed... ignored)
   {
      if (!n)
	 return Day("ymd_yd",rules,y,yjd,jd,yd,1);

      if (m<0) m+=1+number_of_days();

      array(TimeRange) res=days(m,m);
      if (sizeof(res)==1) return res[0];
      if(number_of_days())
	error("Not in range (Day 1..%d exist).\n",
	      number_of_days());
      else
	error("No days in object.\n");
   }

// --- months

   int number_of_months();

   array(cMonth) months(int ...range)
   {
      int from=1,n=number_of_months(),to=n;

      if (sizeof(range))
	 if (sizeof(range)<2)
	    error("Illegal numbers of arguments to months()\n");
	 else
	 {
	    [from,to]=range;
	    if (from>n) return ({}); else if (from<1) from=1;
	    if (to>n) to=n; else if (to<from) return ({});
	 }

      if (md==CALUNKNOWN) make_month();

      return map(enumerate(1+to-from,1,from+m-1),
		 lambda(int x)
		 { return Month("ymd_ym",rules,y,x,1); });
   }

   cMonth month(int ... mp)
   {
      if (md==CALUNKNOWN) make_month();

      if (!sizeof(mp))
	 return Month("ymd_ym",rules,y,m,1);

      int num=mp[0];

      if (num==-1 && !n)
	 return Month("ymd_ym",rules,y,m,1);

      if (num<0) num+=1+number_of_months();

      array(TimeRange) res=months(num,num);
      if (sizeof(res)==1) return res[0];
      error("not in range; Month 1..%d exist in %O\n",
	    number_of_months(),this);
   }

//---- week

   int number_of_weeks();

   array(cWeek) weeks(int ...range)
   {
      int from=1,n=number_of_weeks(),to=n;

      if (sizeof(range))
	 if (sizeof(range)<2)
	    error("Illegal numbers of arguments to weeks()\n");
	 else
	 {
	    [from,to]=range;
	    if (from>n) return ({}); else if (from<1) from=1;
	    if (to>n) to=n; else if (to<from) return ({});
	 }

      if (wd==CALUNKNOWN) make_week();

      return map(enumerate(1+to-from,1,from+w-1),
		 lambda(int x)
		 { return Week("ymd_yw",rules,wy,x,1); });
   }

   cWeek week(int ... mp)
   {
      if (wd==CALUNKNOWN) make_week();

      if (!sizeof(mp))
	 return Week("ymd_yw",rules,wy,w,1);

      int num=mp[0];

      if (num==-1 && !n)
	 return Week("ymd_yw",rules,wy,w,1);

      if (num<0) num+=1+number_of_weeks();

      array(TimeRange) res=weeks(num,num);
      if (sizeof(res)==1) return res[0];
      error("not in range (Week 1..%d exist)\n",
	    number_of_weeks());
   }


// --- functions to conform to Time.*

   protected TimeRange get_unit(string unit,int m)
   {
      if (!n) return day()[unit]();
      if (m<0) m+=::`[]("number_of_"+unit+"s")();
      array(TimeRange) res=::`[](unit+"s")(m,m);
      if (sizeof(res)==1) return res[0];
      error("not in range ("+unit+" 0..%d exist)\n",
	    ::`[]("number_of_"+unit+"s")()-1);
   }

   protected array(TimeRange) get_timeofday(string unit,
					 int start,int step,program p,
					 int ... range)
   {
      int from=0,n=::`[]("number_of_"+unit)(),to=n-1;

      if (sizeof(range))
	 if (sizeof(range)<2)
	    error("Illegal numbers of arguments to "+unit+"()\n");
	 else
	 {
	    [from,to]=range;
	    if (from>=n) return ({}); else if (from<0) from=0;
	    if (to>=n) to=n-1; else if (to<from) return ({});
	 }

      from*=step;
      to*=step;

      to-=from-step;

      from+=unix_time();

      array z=
	 map(enumerate(to/step,step,from),
	     lambda(int x)
	     { return p("timeofday",rules,x,step); });

//        return z;

      if (sizeof(z)>1 &&
	  ((p==cHour && z[0]->utc_offset()%3600 != z[-1]->utc_offset()%3600) ||
	   (p==cMinute && z[0]->utc_offset()%60 != z[-1]->utc_offset()%60)))
      {
   // we're in a zone shifting, and we shift a non-hour (or non-minute)
	 cSecond sec=Second();
	 int i,uo=z[0]->utc_offset();
	 for (i=1; i<sizeof(z); i++)
	    if (z[i]->utc_offset()!=uo)
	    {
	       int uq=(z[i]->utc_offset()-uo);
	       if (uq<0)
	       {
		  if (uq<=-step) uq=-(-uq%step);
		  z= z[..i-1]+
		  ({z[i]->set_size(step+uq,sec)})+
		     map(z[i+1..],"add",uq,sec);
	       }
	       else
	       {
		  if (uq>=step) uq%=step;
		  z= z[..i-1]+
		  ({z[i]->set_size(uq,sec)})+
		     map(z[i..<1],"add",uq,sec);
		  i++;
	       }
	       uo=z[i]->utc_offset();
	    }
      }
      return z;
   }

//! method Second second()
//! method Second second(int n)
//! method Minute minute(int hour,int minute,int second)
//! method array(Second) seconds()
//! method array(Second) seconds(int first,int last)
//! method int number_of_seconds()
//! method Minute minute()
//! method Minute minute(int n)
//! method Minute minute(int hour,int minute)
//! method array(Minute) minutes()
//! method array(Minute) minutes(int first,int last)
//! method int number_of_minutes()
//! method Hour hour()
//! method Hour hour(int n)
//! method array(Hour) hours()
//! method array(Hour) hours(int first,int last)
//! method int number_of_hours()

   int number_of_hours() { return (number_of_seconds()+3599)/3600; }
   cHour hour(void|int n) { return get_unit("hour",n); }
   array(cHour) hours(int ...range)
   { return get_timeofday("hours",0,3600,Hour,@range); }

   int number_of_minutes() { return (number_of_seconds()+59)/60; }
   cMinute minute(void|int n,int ... time)
   {
      if (sizeof(time))
	 return minute(n*60+time[0]);
      return get_unit("minute",n);
   }
   array(cMinute) minutes(int ...range)
   { return get_timeofday("minutes",0,60,Minute,@range); }

   int number_of_seconds() { return end()->unix_time()-unix_time(); }
   cSecond second(void|int n,int ...time)
   {
      if (sizeof(time)==2)
      {
	 return second(n*3600+time[0]*60+time[1]);
//  	 return hour(n)->minute(time[0])->second(time[1]);
      }
      return get_unit("second",n);
   }
   array(cSecond) seconds(int ...range)
   { return get_timeofday("seconds",0,1,Second,@range); }

   float number_of_fractions() { return (float)number_of_seconds(); }
   cSecond fraction(void|float|int n)
   {
      return fractions()[0];
   }
   array(cSecond) fractions(int|float ...range)
   {
      float from,to,n=number_of_fractions();
      if (sizeof(range)==2)
	 from=(float)range[0],to=(float)range[1];
      else if (sizeof(range)==0)
	 from=0.0,to=n;
      else
	 error("Illegal arguments\n");
      if (from<0.0) from=0.0;
      if (to>n) to=n;
      return ({Fraction("timeofday_f",rules,unix_time(),0,
			(int)to,(int)(inano*(to-(int)to)))
	       ->autopromote()});
   }

   protected TimeRange `*(int|float n)
   {
      if(intp(n))
        return set_size(n,this);
      else
        return second()*(int)(how_many(Second)*n);
   }

   array(TimeRange) split(int|float n, function|TimeRange with = Second())
   {
      if (functionp(with))
        with=promote_program(with);

      int length=(int)(how_many(with)/n);
      int remains;
      if(length && intp(n))
        remains=(int)(how_many(with)%n);
      if(!length)
        length=1;

      TimeRange start=beginning();
      TimeRange end=end();
      array result=({});
      TimeRange next;
      while((next=start+with*(length+!!remains)) < end)
      {
        result += ({ start->distance(next) });
        start=next;
        if(remains)
          remains--;
      }
      result += ({ start->distance(end) });
      return result;
   }

// ----------------------------------------
// virtual functions needed
// ----------------------------------------

   string nice_print();
   protected string _sprintf(int t)
   {
      switch (t)
      {
	 case 't':
	    return "Calendar."+calendar_name()+".YMD";
	 default:
            return ::_sprintf(t);
      }
   }

   void create_julian_day(int|float jd);
   protected TimeRange _move(int n,YMD step);
   TimeRange place(TimeRange what,void|int force);

// not needed

   YMD autopromote() { return this; }
}

//------------------------------------------------------------------------
//! class Year
//! 	This is the time period of a year.
//! inherits TimeRange
//! inherits YMD
//------------------------------------------------------------------------

function(mixed...:cYear) Year=cYear;
class cYear
{
   inherit YMD;

   constant is_year=1;

// ---

//!
//! method void create("unix",int unix_time)
//! method void create("julian",int|float julian_day)
//! method void create(int year)
//! method void create(string year)
//! method void create(TimeRange range)
//!	It's possible to create the standard year
//!	by using three different methods; either the normal
//!	way - from standard unix time or the julian day,
//!	and also, for more practical use, from the year number.
//!

   protected void create(mixed ...args)
   {
      if (!sizeof(args))
      {
	 create_now();
	 return;
      }
      else switch (args[0])
      {
	 case "ymd_y":
	    rules=args[1];
	    y=args[2];
	    jd=yjd=args[3];
	    n=args[4];
	    m=md=wy=w=wd=CALUNKNOWN;
	    yd=1;
	    return;
	 case "ymd_yn":
	    rules=args[1];
	    y=args[2];
	    jd=yjd=julian_day_from_year(y);
	    n=args[3];
	    m=md=wy=w=wd=CALUNKNOWN;
	    yd=1;
	    return;
	 case "unix": case "unix_r": case "julian": case "julian_r":
	    // Handled by ::create.
	    break;
	 default:
	    if (intp(args[0]) && sizeof(args)==1)
	    {
	       rules=default_rules;
	       y=args[0];
	       jd=yjd=julian_day_from_year(y);
	       n=1;
	       m=md=wy=w=wd=CALUNKNOWN;
	       yd=1;
	       return;
	    }
	    else if (stringp(args[0]))
	    {
	       y=default_rules->language[f_year_number_from_name](args[0]);
	       rules=default_rules;
	       jd=yjd=julian_day_from_year(y);
	       n=1;
	       m=md=wy=w=wd=CALUNKNOWN;
	       yd=1;
	       return;
	    }
	    break;

      }
      rules=default_rules;
      ::create(@args);
   }

   void create_julian_day(int|float _jd)
   {
      if (floatp(_jd))
	 create_unixtime_default((int)((jd-2440588)*86400));
      else
      {
	 [y,yjd]=year_from_julian_day(_jd);
	 jd=yjd;
	 n=1;
	 md=yd=m=1;
	 wy=w=wd=CALUNKNOWN; // unknown
      }
   }

   TimeRange beginning()
   {
      return Year("ymd_y",rules,y,yjd,0);
   }

   TimeRange end()
   {
      return Year("ymd_yn",rules,y+n,0);
   }

// ----------------

   protected string _sprintf(int t)
   {
      switch (t)
      {
	 case 'O':
	    if (n!=1)
	       return sprintf("Year(%s)",nice_print_period());
	    return sprintf("Year(%s)",nice_print());
	 case 't':
	    return "Calendar."+calendar_name()+".Year";
	 default:
            return ::_sprintf(t);
      }
   }

   string nice_print_period()
   {
      if (!n) return nice_print()+" sharp";
      return sprintf("%s..%s",nice_print(),year(-1)->nice_print());
   }

   string nice_print()
   {
      return year_name();
   }

   string format_nice()
   {
      return year_name();
   }

// --- Year _move

   TimeRange _move(int m,YMD step)
   {
      if (!step->n || !m)
	 return this;

      if (step->is_year)
	 return Year("ymd_yn",rules,y+m*step->n,n)
	    ->autopromote();

      if (step->is_month)
	 return month()->add(m,step)->set_size(this);

//        if (step->is_week)
//  	 return week()->add(m,step)->set_size(this);

      if (step->is_ymd)
	 return Day("ymd_jd",rules,
		    yjd+m*step->number_of_days(),number_of_days())
	    ->autopromote();

      error("_move: Incompatible type %O\n",step);
   }

   protected void convert_from(TimeRange other)
   {
#if 0
      // The following is disabled since it leads to inconsistent
      // behavior with other time ranges when they are converted to
      // partly overlapping time ranges. If the user wants to convert
      // a week to the year it "unambiguously" belongs to, (s)he can
      // do Calendar.ISO.Year(week->year_no()).
      if (other->is_week) {
	// Weeks aren't even on years but they still unambiguously
	// belong to years. We therefore convert using the week year
	// instead of the default method that uses the julian day.
	create ("ymd_yn", other->rules, other->wy,
		other->n > 1 ?
		other->week(-1)->wy - other->wy + 1 :
		other->n);
	return;
      }
#endif

      ::convert_from(other);
      if (other->number_of_years)
	 n=other->number_of_years();
      else
	 n=0;
   }

   object(TimeRange)|zero place(TimeRange what,void|int force)
   {
      if (what->is_day)
      {
	 int yd=what->yd;
	 return Day("ymd_yd",rules,y,yjd,yjd+yd-1,yd,what->n);
      }

      if (what->is_week)
      {
	 cWeek week=Week("ymd_yw",rules,y,what->w,what->n);
	 if (!force && week->wy!=y) return 0; // not this year
	 return week;
      }

      if (what->is_year)
	 return Year("ymd_yn",rules,y,what->number_of_years());

      if (what->is_month)
	 return month(what->month_name());

      if (what->is_timeofday)
	 return place(what->day(),force)->place(what,force);

      error("place: Incompatible type %O\n",what);
   }

   TimeRange distance(TimeRange to)
   {
      if (to->is_timeofday)
      {
	 return hour()->distance(to);
      }
      if (to->is_ymd)
      {
	 if (to->is_year)
	 {
	    int y1=y;
	    int y2=to->y;
	    if (y2<y1)
	       error("distance: negative distance\n");
	    return Year("ymd_yn",rules,y,y2-y1)
	       ->autopromote();
	 }
	 if (to->is_month)
	    return month()->distance(to);
	 return day()->distance(to);
      }

      error("distance: Incompatible type %O\n",to);
   }

// ---

   int number_of_years()
   {
      return n;
   }

   int number_of_weeks();

//! method Month month()
//! method Month month(int n)
//! method Month month(string name)
//!	The Year type overloads the month() method,
//!	so it is possible to get a specified month
//!	by string:
//!
//!	<tt>year-&gt;month("April")</tt>
//!
//!	The integer and no argument behavior is inherited
//!	from <ref to=YMD.month>YMD</ref>().

   cMonth month(int|string ... mp)
   {
      if (sizeof(mp) &&
	  stringp(mp[0]))
      {
	 int num=((int)mp[0]) ||
	    rules->language[f_month_number_from_name](mp[0]);
	 if (!num)
	    error("no such month %O in %O\n",mp[0],this);

	 return ::month(num);
      }
      else
	 return ::month(@mp);
   }

//! method Week	week()
//! method Week	week(int n)
//! method Week	week(string name)
//!	The Year type overloads the week() method,
//!	so it is possible to get a specified week
//!	by name:
//!
//!	<tt>year-&gt;week("17")</tt>
//!	<tt>year-&gt;week("w17")</tt>
//!
//!	The integer and no argument behavior is inherited
//!	from <ref to=YMD.week>YMD</ref>().
//!
//!	This is useful, since the first week of a year
//!	not always (about half the years, in the ISO calendar)
//!	is numbered '1'.
//!

   cWeek week(int|string ... mp)
   {
      if (sizeof(mp) &&
	  stringp(mp[0]))
      {
	 int num;
	 sscanf(mp[0],"%d",num);
	 sscanf(mp[0],"w%d",num);

	 cWeek w=::week(num);
	 if (w->week_no()==num) return w;
	 return ::week(num-(w->week_no()-num));
      }
      else
	 return ::week(@mp);
   }

   cYear set_ruleset(Calendar.Ruleset r)
   {
      return Year("ymd_y",r,y,yjd,n);
   }
}


// ----------------------------------------------------------------
//! class Month
//! inherits YMD
// ----------------------------------------------------------------

function(mixed...:cMonth) Month=cMonth;
class cMonth
{
   inherit YMD;

   constant is_month=1;

   int nd; // number of days
   int nw; // number of weeks

   protected void create(mixed ...args)
   {
      if (!sizeof(args))
      {
	 rules=default_rules;
	 create_unixtime_default(time());
	 return;
      }
      else
	 switch (args[0])
	 {
	    case "ymd_ym":
	       rules=args[1];
	       y=args[2];
	       m=args[3];
	       n=args[4];
	       md=1;
	       wy=w=wd=CALUNKNOWN;
	       [y,m,nd,yd]=year_month_from_month(y,m);
	       yjd=julian_day_from_year(y);
	       jd=yjd+yd-1;
	       if (n!=1) nd=CALUNKNOWN;
	       nw=CALUNKNOWN;
	       return;
	    case "ymd_yjmw":
	       rules=args[1];
	       y=args[2];
	       yjd=args[3];
	       jd=args[4];
	       yd=1+jd-yjd;
	       m=args[5];
	       n=args[6];
	       wd=args[7];
	       w=args[8];
	       wy=args[9];
	       md=1;
	       nw=nd=CALUNKNOWN;
	       return;
	    case "ymd_jd":
	       rules=args[1];
	       create_julian_day(args[2]);
	       n=args[3];
	       return;
	    case "unix": case "unix_r": case "julian": case "julian_r":
	       // Handled by ::create.
	       break;
	    default:
	       if (intp(args[0]) && sizeof(args)==2)
	       {
		  create("ymd_ym",default_rules,args[0],args[1],1);
		  if (y!=args[0])
		     error("month %d doesn't exist in %d\n",args[1],args[0]);
		  return;
	       }
	       break;
	 }

      rules=default_rules;
      ::create(@args);
   }

   void create_julian_day(int|float _jd)
   {
      if (floatp(_jd))
	 create_unixtime_default((int)((jd-2440588)*86400));
      else
      {
	 int zmd;
	 [y,yjd]=year_from_julian_day(jd=_jd);
	 [m,zmd,nd,yd]=month_from_yday(y,1+jd-yjd);
	 jd=yd+yjd-1;

	 n=1;
	 md=1;
	 nw=wd=w=wy=CALUNKNOWN; // unknown
      }
   }

   protected string _sprintf(int t)
   {
//        return sprintf("month y=%d yjd=%d m=%d jd=%d yd=%d n=%d nd=%d",
//  		     y,yjd,m,jd,yd,n,number_of_days());
      switch (t)
      {
	 case 'O':
	    if (n!=1)
	       return sprintf("Month(%s)",nice_print_period());
	    return sprintf("Month(%s)",nice_print());
	 case 't':
	    return "Calendar."+calendar_name()+".Month";
	 default:
            return ::_sprintf(t);
      }
   }

   string nice_print()
   {
      return sprintf("%s %s", month_name(), year_name());
   }

   string format_nice()
   {
      return sprintf("%s %s", month_name(), year_name());
   }


   string nice_print_period()
   {
      if (!n) return day()->nice_print()+" "+minute()->nice_print()+" sharp";
      cMonth mo=month(-1);
      if (mo->y==y)
	 return sprintf("%s..%s %s",
			month_shortname(),
			mo->month_shortname(),
			year_name());
      return nice_print()+" .. "+mo->nice_print();
   }

   TimeRange beginning()
   {
      return Month("ymd_yjmw",rules,y,yjd,jd,m,0,wd,w,wy)
	 ->autopromote();
   }

   TimeRange end()
   {
      return Month("ymd_ym",rules,y,m+n,0)
	 ->autopromote();
   }

// --- month position and distance

   TimeRange distance(TimeRange to)
   {
      if (to->is_timeofday)
	 return hour()->distance(to);

      if (to->is_ymd)
      {
	 if (to->is_month || to->is_year)
	 {
	    int n1=months_to_month(to->y,to->is_year?1:to->m);
	    if (n1<0)
	       error("distance: negative distance (%d months)\n",n1);
	    return Month("ymd_yjmw",rules,y,yjd,jd,m,n1,wd,w,wy)
	       ->autopromote();
	 }

	 int d1=jd;
	 int d2=to->jd;
	 if (d2<d1)
	    error("distance: negative distance (%d days)\n",d2-d1);
	 return Day("ymd_ydmw",rules,y,yjd,jd,yd,d2-d1,m,1,wy,w,wd,mnd)
	    ->autopromote();
      }

      error("distance: Incompatible type %O\n",to);
   }

   protected void convert_from(TimeRange other)
   {
      ::convert_from(other);
      if (other->number_of_months)
	 n=other->number_of_months();
      else
	 n=0;
   }

   TimeRange _move(int x,YMD step)
   {
      if (step->is_year)
	 return Month("ymd_ym",rules,y+x*step->n,m,n)
	    ->autopromote();
      if (step->is_month)
	 return Month("ymd_ym",rules,y,m+x*step->n,n)
	    ->autopromote();

      return Day("ymd_jd",rules,jd+x*step->number_of_days(),number_of_days())
	 ->autopromote();
   }

   object(TimeRange)|zero place_day(int day,int day_n,void|int force)
   {
      if (day>number_of_days()) return 0; // doesn't exist
      return Day("ymd_jd",rules,jd+day-1,day_n)->autopromote();
   }

   TimeRange place(TimeRange what,void|int force)
   {
      if (what->is_year)
	 return year()->place(what,force); // just fallback

      if (what->is_day)
	 return place_day(what->month_day(),what->n,force);

      if (what->is_month)
	 return Month("ymd_ym",rules,y,m,what->number_of_months())
	    ->autopromote();

      if (what->is_week)
	 return place(what->day(),force)->week();

      if (what->is_timeofday)
	 return place(what->day(),force)->place(what,force);

      error("place: Incompatible type %O\n",what);
   }

// --- Month to other units

   int number_of_years()
   {
      if (n<=1) return 1;

      [int y2,int m2,int nd2,int yd2]=year_month_from_month(y,m+n);
      return 1+y2-y;
   }

   int number_of_days()
   {
      if (nd!=CALUNKNOWN) return nd;

      [int y2,int m2,int nd2,int yd2]=year_month_from_month(y,m+n);
      return nd=julian_day_from_year(y2)+yd2-jd-1;
   }

   int number_of_weeks()
   {
      if (nw!=CALUNKNOWN) return nw;

      [int y2,int m2,int nd2,int yd2]=year_month_from_month(y,m+n);

      return nw=
	 Week("julian_r",jd,rules)
	 ->range(Week("julian_r",julian_day_from_year(y2)+yd2-2,rules))
	 ->number_of_weeks();
   }

   int number_of_months()
   {
      return n;
   }

   cMonth set_ruleset(Calendar.Ruleset r)
   {
      return Month("ymd_yjmw",r,y,yjd,jd,m,n,wd,w,wy);
   }

// --- needs to be defined

   protected int months_to_month(int y,int m);
}

// ----------------------------------------------------------------
//! class Week
//!	The Calendar week represents a standard time period of
//!	a week. In the Gregorian calendar, the standard week
//!	starts on a sunday and ends on a saturday; in the ISO
//!	calendar, it starts on a monday and ends on a sunday.
//!
//!	The week are might not be aligned to the year, and thus
//!	the week may cross year borders and the year of
//!	the week might not be the same as the year of all the
//!	days in the week. The basic rule is that the week year
//!	is the year that has the most days in the week, but
//!	since week number only is specified in the ISO calendar
//!	- and derivates - the week number of most calendars
//!	is the week number of most of the days in the ISO
//!	calendar, which modifies this rule for the Gregorian calendar;
//!	the week number and year is the same as for the ISO calendar,
//!	except for the sundays.
//!
//!	When adding, moving and subtracting months
//!	to a week, it falls back to using days.
//!
//!	When adding, moving or subtracting years,
//!	if tries to place the moved week in the
//!	resulting year.
//!
//! inherits YMD
// ----------------------------------------------------------------

function(mixed...:cWeek) Week=cWeek;
class cWeek
{
   inherit YMD;

   // Note: wy, w and wd are never CALUNKNOWN in this class.

   constant is_week=1;

//!
//! method void create("unix",int unix_time)
//! method void create("julian",int|float julian_day)
//! method void create(int year,int week)
//!	It's possible to create the standard week
//!	by using three different methods; either the normal
//!	way - from standard unix time or the julian day,
//!	and also, for more practical use, from year and week
//!	number.
//!

   protected void create(mixed ...args)
   {
      if (!sizeof(args))
      {
	 rules=default_rules;
	 create_unixtime_default(time());
	 return;
      }
      else
	 switch (args[0])
	 {
	    case "ymd_yw":
	       rules=args[1];
	       wy=args[2];
	       w=args[3];
	       n=args[4];
	       m=md=CALUNKNOWN;
	       [wy,w,wd,int nd,jd]=week_from_week(wy,w);
	       int wyjd=julian_day_from_year(wy);
	       if (wyjd > jd)
		 y = wy - 1, yjd = julian_day_from_year (y);
	       else
		 y = wy, yjd = wyjd;
	       yd=1+jd-yjd;
	       if (n!=1) nd=CALUNKNOWN;
	       return;
	    case "ymd_yjwm":
	       rules=args[1];
	       y=args[2];
	       yjd=args[3];
	       jd=args[4];
	       yd=1+jd-yjd;
	       wy=args[5];
	       w=args[6];
	       n=args[7];
	       md=args[8];
	       m=args[9];
	       mnd=args[10];
	       wd=1;
	       nd=CALUNKNOWN;
	       return;
	    case "ymd_jd":
	       rules=args[1];
	       create_julian_day(args[2]);
	       n=args[3];
	       return;
	    case "unix": case "unix_r": case "julian": case "julian_r":
	       // Handled by ::create.
	       break;
	    default:
	       if (intp(args[0]) && sizeof(args)==2)
	       {
		  create("ymd_yw",default_rules,args[0],args[1],1);
		  if (wy!=args[0])
		    // FIXME: Allow weeks 0 and 53/54 if they contain
		    // at least one day in this year.
		     error("Week %d doesn't exist in %d\n",args[1],args[0]);
		  return;
	       }
	       break;
	 }

      rules=default_rules;
      ::create(@args);
   }

   void create_julian_day(int|float _jd)
   {
      if (floatp(_jd))
	 create_unixtime_default((int)((jd-2440588)*86400));
      else
      {
	 int zwd;
	 [wy,w,zwd,int nd,jd]=week_from_julian_day(_jd);
	 [y,yjd]=year_from_julian_day(_jd);
	 yd=1+jd-yjd;

	 n=1;
	 wd=1;
	 md=m=CALUNKNOWN; // unknown
      }
   }

   protected string _sprintf(int t)
   {
//        return sprintf("week y=%d yjd=%d w=%d jd=%d yd=%d n=%d nd=%d",
//  		     y,yjd,w,jd,yd,n,number_of_days());
      switch (t)
      {
	 case 'O':
	    if (n!=1)
	       return sprintf("Week(%s)",nice_print_period());
	    return sprintf("Week(%s)",nice_print());
	 case 't':
	    return "Calendar."+calendar_name()+".Week";
	 default:
            return ::_sprintf(t);
      }
   }

   int year_no()
   {
     return wy>0?wy:-1+wy;
   }

   int year_day()
   //! Can be less than 1 for the first week of the year if it begins
   //! in the previous year.
   {
     return y != wy ? 1 + jd - julian_day_from_year (wy) : yd;
   }

   string nice_print()
   {
      return
	 sprintf("%s %s",
		 week_name(),
		 year_name());
   }

   string format_nice()
   {
      return
	 sprintf("%s %s",
		 week_name(),
		 year_name());
   }


   string nice_print_period()
   {
      if (!n) return day()->nice_print()+" "+minute()->nice_print()+" sharp";
      cWeek wo=week(-1);
      if (wo->wy==wy)
	 return sprintf("%s..%s %s",
			week_name(),
			wo->week_name(),
			year_name());
      return nice_print()+" .. "+wo->nice_print();
   }

   TimeRange beginning()
   {
      return Week("ymd_yjwm",rules,y,yjd,jd,wy,w,0,md,m,mnd)
	 ->autopromote();
   }

   TimeRange end()
   {
      return Week("ymd_yw",rules,wy,w+n,0)
	 ->autopromote();
   }

// --- week position and distance

   TimeRange distance(TimeRange to)
   {
      if (to->is_timeofday)
	 return hour()->distance(to);

      if (to->is_week)
      {
	 int n1=weeks_to_week(to->wy,to->w);
	 if (n1<0)
	    error("distance: negative distance (%d weeks)\n",n1);
	 return Week("ymd_yjwm",rules,y,yjd,jd,wy,w,n1,md,m,mnd)
	    ->autopromote();
      }

      if (to->julian_day)
      {
	 int d1=jd;
	 int d2=to->julian_day();
	 if (d2<d1)
	    error("distance: negative distance (%d days)\n",d2-d1);
	 return Day("ymd_ydmw",rules,y,yjd,jd,yd,d2-d1,m,md,wy,w,1,mnd)
	    ->autopromote();
      }

      error("distance: Incompatible type %O\n",to);
   }

   protected void convert_from(TimeRange other)
   {
      ::convert_from(other);
      if (other->number_of_weeks)
	 n=other->number_of_weeks();
      else
	 n=0;
   }

   TimeRange _move(int x,YMD step)
   {
      if (step->is_week)
	 return Week("ymd_yw",rules,wy,w+x*step->n,n)
	    ->autopromote();

      if (step->is_year) {
	TimeRange stepped = year()->add(x,step);
	if (TimeRange placed = stepped->place(this,0))
	  return placed;
	// If we couldn't place our week in the target year it means
	// we're in week 53 and the target year got only 52 weeks. We
	// return the closest week of the target year, i.e. week 52.
	return Week ("ymd_yw", rules, stepped->y, 52, n);
      }

      if (step->number_of_days)
	 return Day("ymd_jd",rules,
		    jd+x*step->number_of_days(),number_of_days())
	    ->autopromote();

      error("add: Incompatible type %O\n",step);
   }

   object(TimeRange)|zero place_day(int day,int day_n,int force)
   {
      if (day>number_of_days())
	 if (!force)
	    return 0;
	 else
	    return Day("ymd_jd",rules,jd+day-1,max(0,day_n-1))->autopromote();
      return Day("ymd_jd",rules,jd+day-1,day_n)->autopromote();
   }

   TimeRange place(TimeRange what,void|int force)
   {
      if (what->is_supertimerange)
	 return what->mend_overlap(map(what->parts,place,force));
//  	 return `|(@map(what->parts,place,force));

      if (what->is_year)
	 return year()->place(what,force); // just fallback
      if (what->is_month)
	 return month()->place(what,force); // just fallback

      if (what->is_week)
	 return Week("ymd_yw",rules,wy,w,what->number_of_weeks());

      if (what->is_day)
	 return place_day(what->week_day(),what->n,force);

      if (what->is_timeofday)
	 return place(what->day(),force)->place(what,force);

      error("place: Incompatible type %O\n",what);
   }

// --- Week to other units

   int number_of_years()
   {
      if (n<=1 && y == wy) return 1;

      [int y2,int w2,int wd2,int nd2,int jd2]=week_from_week(wy,w+n);
      return 1+y2-y;
   }

   int number_of_months()
   {
      if (!n) return 1;

// cheat
      return Day("ymd_jd",rules,jd,number_of_days())
	 ->number_of_months();
   }

   int number_of_weeks()
   {
      return n;
   }

   int number_of_days();

//! method Day day()
//! method Day day(int n)
//! method Day day(string name)
//!	The Week type overloads the day() method,
//!	so it is possible to get a specified weekday
//!	by string:
//!
//!	<tt>week-&gt;day("sunday")</tt>
//!
//!	The integer and no argument behavior is inherited
//!	from <ref to=YMD.day>YMD</ref>().
//!
//! note:
//!	the weekday-from-string routine is language dependent.

   cDay day(int|string ... mp)
   {
      if (sizeof(mp) &&
	  stringp(mp[0]))
      {
	 int num=((int)mp[0]) ||
	    rules->language[f_week_day_number_from_name](mp[0]);
	 if (!num)
	    error("no such day %O in %O\n",mp[0],this);

	 return ::day(num);
      }
      else
	 return ::day(@mp);
   }

   cWeek set_ruleset(Calendar.Ruleset r)
   {
      return Week("ymd_yjwm",r,y,yjd,jd,wy,w,n,md,m,mnd);
   }

// --- needs to be defined

   protected int weeks_to_week(int y,int m);
}

// ----------------------------------------------------------------
//! class Day
//! inherits YMD
// ----------------------------------------------------------------

class cDay
{
   inherit YMD;

   constant is_day=1;
   int nw;

//!
//! method void create("unix",int unix_time)
//! method void create("julian",int|float julian_day)
//! method void create(int year,int month,int day)
//! method void create(int year,int year_day)
//! method void create(int julian_day)
//!	It's possible to create the day
//!	by using five different methods; either the normal
//!	way - from standard unix time or the julian day,
//!	and also, for more practical use, from year, month and day,
//!	from year and day of year, and from julian day
//!	without extra fuzz.

   protected void create(mixed ...args)
   {
      if (!sizeof(args))
      {
	 rules=default_rules;
	 create_unixtime_default(time());
	 return;
      }
      else
	 switch (args[0])
	 {
	    case "ymd_ydmw":
	       rules=args[1];
	       y=args[2];
	       yjd=args[3];
	       jd=args[4];
	       yd=args[5];
	       n=args[6];
	       m=args[7];
	       md=args[8];
	       wy=args[9];
	       w=args[10];
	       wd=args[11];
	       mnd=args[12];
	       nw=CALUNKNOWN;
	       return;
	    case "ymd_yd":
	       rules=args[1];
	       y=args[2];
	       yjd=args[3];
	       jd=args[4];
	       yd=args[5];
	       n=args[6];
	       wy=wd=nw=md=m=w=CALUNKNOWN;
	       return;
	    case "ymd_jd":
	       rules=args[1];
	       create_julian_day(args[2]);
	       n=args[3];
	       wy=wd=nw=md=m=w=CALUNKNOWN;
	       return;
	    case "unix_r":
	    case "julian_r":
	    case "unix":
	    case "julian":
	       // Handled by ::create.
	       break;
	    default:
	       rules=default_rules;
	       wy=wd=nw=md=m=w=CALUNKNOWN;
	       n=1;
	       switch (sizeof(args))
	       {
		  case 1:
		     if (intp(args[0]))
		     {
			create_julian_day(args[0]);
			return;
		     }
		     break;
		  case 2:
		     if (stringp(args[0]))
			y=default_rules->language[f_year_number_from_name]
			   (args[0]);
		     else if (intp(args[0]))
			y=args[0];
		     else
			break;
		     if (!intp(args[1]))
			break;
		     yd=args[1];
		     yjd=julian_day_from_year(y);
		     jd=yjd+yd-1;
		     return;
		  case 3:
		     if (stringp(args[0]))
			y=default_rules->language[f_year_number_from_name]
			   (args[0]);
		     else if (intp(args[0]))
			y=args[0];
		     else
			break;
		     if (!intp(args[1]) ||
			 !intp(args[2])) break;
		     md=args[2];
		     [y,m,int nmd,int myd]=
			year_month_from_month(y,args[1]);
		     if (m!=args[1] || y!=args[0])
			error("No such month (%d-%02d)\n",args[0],args[1]);
		     yjd=julian_day_from_year(y);
		     md=args[2];
		     jd=yjd+myd+md-2;
		     yd=jd-yjd+1;
		     if (md>nmd || md<1)
			error("No such day of month (%d-%02d-%02d)\n",
			      @args);
		     return;
	       }
	 }

      rules=default_rules;
      ::create(@args);
   }

   void create_julian_day(int|float _jd)
   {
      n=1;
      nw=md=m=wd=w=wy=CALUNKNOWN; // unknown

      if (floatp(_jd))
      {
	 create_unixtime_default((int)((jd-2440588)*86400));
      }
      else
      {
	 [y,yjd]=year_from_julian_day(jd=_jd);
	 yd=1+jd-yjd;
      }
   }

   protected string _sprintf(int t)
   {
      switch (t)
      {
	 case 'O':
	   catch {
	     if (n!=1)
	       return sprintf("Day(%s)",nice_print_period());
	     return sprintf("Day(%s)",nice_print());
	   };
	   return sprintf("Day(%d)", unix_time());
	 case 't':
	    return "Calendar."+calendar_name()+".Day";
	 default:
            return ::_sprintf(t);
      }
   }

   string nice_print()
   {
      if (m==CALUNKNOWN) make_month();
      if (wd==CALUNKNOWN) make_week();
      return
	 sprintf("%s %s %s %s",
		 week_day_shortname(),
		 month_day_name(),month_shortname(),
		 year_name());
   }

   string format_nice()
   {
      if (m==CALUNKNOWN) make_month();
      if (calendar()->Year()!=year())
	 return
	    sprintf("%s %s %s",
		    month_day_name(),month_shortname(),
		    year_name());
      else
	 return
	    sprintf("%s %s",
		    month_day_name(),month_shortname());
   }

   string nice_print_period()
   {
//        return nice_print()+" n="+n+"";
      if (!n) return nice_print()+" "+minute()->nice_print()+" sharp";
      return nice_print()+" .. "+day(-1)->nice_print();
   }

   TimeRange beginning()
   {
      return Day("ymd_ydmw",rules,y,yjd,jd,yd,0,m,md,wy,w,wd,mnd);
   }

   TimeRange end()
   {
      return Day("ymd_jd",rules,jd+n,0)
	 ->autopromote();
   }

   protected void convert_from(TimeRange other)
   {
      ::convert_from(other);
      if (other->number_of_days)
	 n=other->number_of_days();
      else
	 n=0;
   }

// --- Day _move

   protected TimeRange _move(int x,YMD step)
   {
      if (step->is_year) {
	TimeRange stepped = year()->add(x,step);
	if (TimeRange placed = stepped->place(this,0))
	  return placed;
	// If we couldn't place our day in the target year it means
	// we're on a leap day and the target year doesn't have any.
	// We return the closest day in the same month.
	TimeRange placed = stepped->place (month());
	if (md == CALUNKNOWN) make_month();
	return placed->day (md < placed->number_of_days() ? md : -1);
      }

      if (step->is_month) {
	TimeRange stepped = month()->add(x,step);
	if (TimeRange placed = stepped->place(this,0))
	  return placed;
	// The target month is shorter and our date doesn't exist in
	// it. We return the closest (i.e. last) day of the target
	// month.
	return stepped->day (-1);
      }

      if (step->is_week)
	 return week()->add(x,step)->place(this,1);

      if (step->is_day)
	 return Day("ymd_jd",rules,jd+x*step->n,n)
	    ->autopromote();

      error("_move: Incompatible type %O\n",step);
   }

   TimeRange place(TimeRange what,int|void force)
   {
      if (what->is_timeofday)
      {
	 int lux=
	    what->ux-
	    Day("unix_r",what->unix_time(),what->ruleset())
	    ->unix_time();
	 TimeRange res;

	 if (what->is_timeofday_f)
	    res=
	       Fraction("timeofday_f",rules,
			lux+unix_time(),what->ns,what->s_len,what->ns_len);
	 else
	    res=Second("timeofday",rules,unix_time()+lux,what->len);

	 if (what->rules->timezone->is_dst_timezone ||
	     rules->timezone->is_dst_timezone)
	 {
	    int u0=what->utc_offset()-what->day()->utc_offset();
	    int u1=res->utc_offset()-utc_offset();
//  	    werror("%O %O\n",u0,u1);
	    if (u1-u0)
	       res=res->add(u1-u0,Second);
	    else
	       res=res->autopromote();

	    if (!force)
	    {
	       if (res->hour_no()!=what->hour_no())
		  error("place: no such time of "
			"day (DST shift): %O\n", what);
	    }
	 }
	 else
	    res=res->autopromote();

	 return res;
      }

      if (what->is_year)
	 return year()->place(what,force); // just fallback
      if (what->is_month)
	 return month()->place(what,force); // just fallback
      if (what->is_week)
	 return week()->place(what,force); // just fallback

      if (what->is_day)
	 return Day("ymd_jd",rules,jd,what->number_of_days())
	    ->autopromote();

      error("place: Incompatible type %O\n",what);
   }

   TimeRange distance(TimeRange to)
   {
      if (to->is_timeofday)
	 return hour()->distance(to);
      if (to->is_ymd)
      {
	 int d1=jd;
	 int d2=to->jd;
	 if (d2<d1)
	    error("distance: negative distance (%d days)\n",d2-d1);
	 return Day("ymd_ydmw",rules,y,yjd,jd,yd,d2-d1,m,md,wy,w,wd,mnd)
	    ->autopromote();
      }

      error("distance: Incompatible type %O\n",to);
   }

// --- Day to other YMD

   int number_of_days()
   {
      return n;
   }

   int number_of_years()
   {
      if (n<=1) return 1;
      return 1+year_from_julian_day(jd+n-1)[0]-y;
   }

   int number_of_weeks()
   {
      if (nw!=CALUNKNOWN) return nw;

      if (n<=1) return nw=1;

      return nw=
	 Week("julian_r",jd,rules)
	 ->range(Week("julian_r",jd+n-1,rules))
	 ->number_of_weeks();
   }

   cDay set_ruleset(Calendar.Ruleset r)
   {
      return Day("ymd_ydmw",r,y,yjd,jd,yd,n,m,md,wy,w,wd,mnd);
   }

// backwards compatible with calendar I
   string iso_name() { return format_ymd(); }
   string iso_short_name() { return format_ymd_short(); }
}

function(mixed...:cDay) Day=cDay;

//------------------------------------------------------------------------
//- class YMD_Time
//------------------------------------------------------------------------

class YMD_Time
{
#define MKRBASE								     \
	do								     \
	{								     \
	   int n;							     \
	   if (!rbase)							     \
               rbase=Day("unix_r",this->ux,this->rules)->range(Day("unix_r",this->ux+this->len,this->rules)); \
	} while (0)

#define RBASE (this->base || this->make_base())

   cDay day(int ...n) { return RBASE->day(@n); }
   cDay number_of_days() { return RBASE->number_of_days(); }
   array(cDay) days(int ...r) { return RBASE->days(@r); }

   cMonth month(int ...n) { return RBASE->month(@n); }
   cMonth number_of_months() { return RBASE->number_of_months(); }
   array(cMonth) months(int ...r) { return RBASE->months(@r); }

   cWeek week(int ...n) { return RBASE->week(@n); }
   cWeek number_of_weeks() { return RBASE->number_of_weeks(); }
   array(cWeek) weeks(int ...r) { return RBASE->weeks(@r); }

   cYear year(int ...n) { return RBASE->year(@n); }
   cYear number_of_years() { return RBASE->number_of_years(); }
   array(cYear) years(int ...r) { return RBASE->years(@r); }

   int year_no() { return RBASE->year_no(); }
   int month_no() { return RBASE->month_no(); }
   int week_no() { return RBASE->week_no(); }
   int month_name() { return RBASE->month_name(); }
   string month_shortname() { return RBASE->month_shortname(); }
   int month_day() { return RBASE->month_day(); }
   string month_day_name() { return RBASE->month_day_name(); }
   int week_day() { return RBASE->week_day(); }
   int year_day() { return RBASE->year_day(); }
   int year_name() { return RBASE->year_name(); }
   string week_name() { return RBASE->week_name(); }
   string week_day_name() { return RBASE->week_day_name(); }
   string week_day_shortname() { return RBASE->week_day_shortname(); }
   int leap_year() { return RBASE->leap_year(); }

   string format_iso_ymd() { return RBASE->format_iso_ymd(); }
   string format_ext_ymd() { return RBASE->format_ext_ymd(); }
   string format_ymd() { return RBASE->format_ymd(); }
   string format_ymd_short() { return RBASE->format_ymd_short(); }
   string format_ymd_xshort() { return RBASE->format_ymd_xshort(); }
   string format_iso_week() { return RBASE->format_iso_week(); }
   string format_iso_week_short()
   { return RBASE->format_iso_week_short(); }
   string format_week() { return RBASE->format_week(); }
   string format_week_short() { return RBASE->format_week_short(); }
   string format_month() { return RBASE->format_month(); }
   string format_month_short() { return RBASE->format_month_short(); }

#undef RBASE
}

#define OVERLOAD_TIMEOFDAY						\
									\
   protected int(0..1) create_backtry(mixed ... args)			\
   {									\
      if (sizeof(args)>=5 &&						\
	  (intp(args[0])||stringp(args[0])) &&				\
	  intp(args[1]) &&						\
	  intp(args[2]))						\
      {									\
	 base=Day(@args[..2]);						\
	 return ::create_backtry(@args[3..]);				\
      }									\
      return ::create_backtry(@args);					\
   }


//------------------------------------------------------------------------
//! class Hour
//! inherits Time.Hour
//! inherits YMD
//------------------------------------------------------------------------

class cHour
{
   inherit Time::cHour;
   inherit YMD_Time;
   OVERLOAD_TIMEOFDAY;
}

//------------------------------------------------------------------------
//! class Minute
//! inherits Time.Minute
//! inherits YMD
//------------------------------------------------------------------------

class cMinute
{
   inherit Time::cMinute;
   inherit YMD_Time;
   OVERLOAD_TIMEOFDAY;
}

//------------------------------------------------------------------------
//! class Second
//! inherits Time.Second
//! inherits YMD
//------------------------------------------------------------------------

class cSecond
{
   inherit Time::cSecond;
   inherit YMD_Time;
   OVERLOAD_TIMEOFDAY;
}

//------------------------------------------------------------------------
//! class Fraction
//! inherits Time.Fraction
//! inherits YMD
//------------------------------------------------------------------------

class cFraction
{
   inherit Time::cFraction;
   inherit YMD_Time;
   OVERLOAD_TIMEOFDAY;
}

//------------------------------------------------------------------------
//! class SuperTimeRange
//! inherits Time.SuperTimeRange
//------------------------------------------------------------------------

class cSuperTimeRange
{
   inherit Time::cSuperTimeRange;

   array(cYear) years(int ...range) { return get_units("years",@range); }
   cYear year(void|int n) { return get_unit("years",n); }
   int number_of_years() { return num_units("years"); }

   array(cMonth) months(int ...range) { return get_units("months",@range); }
   cMonth month(void|int n) { return get_unit("months",n); }
   int number_of_months() { return num_units("months"); }

   array(cWeek) weeks(int ...range) { return get_units("weeks",@range); }
   cWeek week(void|int n) { return get_unit("weeks",n); }
   int number_of_weeks() { return num_units("weeks"); }

   array(cDay) days(int ...range) { return get_units("days",@range); }
   cDay day(void|int n) { return get_unit("days",n); }
   int number_of_days() { return num_units("days"); }

#define RBASE parts[0]

   int year_no() { return RBASE->year_no(); }
   int month_no() { return RBASE->month_no(); }
   int week_no() { return RBASE->week_no(); }
   int month_name() { return RBASE->month_name(); }
   string month_shortname() { return RBASE->month_shortname(); }
   int month_day() { return RBASE->month_day(); }
   string month_day_name() { return RBASE->month_day_name(); }
   int week_day() { return RBASE->week_day(); }
   int year_day() { return RBASE->year_day(); }
   string week_name() { return RBASE->week_name(); }
   string week_day_name() { return RBASE->week_day_name(); }
   string week_day_shortname() { return RBASE->week_day_shortname(); }
   int leap_year() { return RBASE->leap_year(); }

   string format_iso_ymd() { return RBASE->format_iso_ymd(); }
   string format_ext_ymd() { return RBASE->format_ext_ymd(); }
   string format_ymd() { return RBASE->format_ymd(); }
   string format_ymd_short() { return RBASE->format_ymd_short(); }
   string format_ymd_xshort() { return RBASE->format_ymd_xshort(); }
   string format_iso_week() { return RBASE->format_iso_week(); }
   string format_iso_week_short() { return RBASE->format_iso_week_short(); }
   string format_week() { return RBASE->format_week(); }
   string format_week_short() { return RBASE->format_week_short(); }
   string format_month() { return RBASE->format_month(); }
   string format_month_short() { return RBASE->format_month_short(); }

#undef RBASE
}

//------------------------------------------------------------------------
// Pop out doc-extractor context to the top-level scope.
//! module Calendar
//! submodule YMD
// global convenience functions
//------------------------------------------------------------------------

//! method TimeRange parse(string fmt,string arg)
//!	parse a date, create relevant object
//!	fmt is in the format "abc%xdef..."
//!	where abc and def is matched, and %x is
//!	one of those time units:
//!	<pre>
//!	%Y absolute year
//!	%y dwim year (70-99 is 1970-1999, 0-69 is 2000-2069)
//!	%M month (number, name or short name) (needs %y)
//!	%W week (needs %y)
//!	%D date (needs %y, %m)
//!	%d short date (20000304, 000304)
//!	%a day (needs %y)
//!	%e weekday (needs %y, %w)
//!	%h hour (needs %d, %D or %W)
//!	%m minute (needs %h)
//!	%s second (needs %m)
//!     %S seconds since the Epoch (only combines with %f)
//!     %f fraction of a second (needs %s or %S)
//!	%t short time (205314, 2053)
//!	%z zone
//!	%p "am" or "pm"
//!	%n empty string (to be put at the end of formats)
//!	</pre>
//!
//! returns 0 if format doesn't match data, or the appropriate time object.
//!
//! note:
//!	The zone will be a guess if it doesn't state an exact
//!	regional timezone (like "Europe/Stockholm") -
//!	most zone abbriviations (like "CET") are used by more
//!	then one region with it's own daylight saving rules.
//!	Also beware that for instance CST can be up to four different zones,
//!	central Australia or America being the most common.
//!
//!	<pre>
//! 	Abbreviation Interpretation
//!	AMT          America/Manaus       [UTC-4]
//!	AST	     America/Curacao      [UTC-4]
//!	CDT	     America/Costa_Rica   [UTC-6]
//!	CST	     America/El Salvador  [UTC-6]
//!	EST	     America/Panama       [UTC-5]
//!	GST          Asia/Dubai           [UTC+4]
//!	IST          Asia/Jerusalem       [UTC+2]
//!	WST          Australia/Perth      [UTC+8]
//!	</pre>
//!
//!	This mapping is modifiable in the ruleset, see
//!	<ref>Ruleset.set_abbr2zone</ref>.


// dwim time of day; needed to correct timezones
// this API may change without further notice
protected object(TimeRange)|zero dwim_tod(TimeRange origin,
                                          string whut,int h,int m,int s)
{
   TimeRange tr;
   if (catch {
      tr=origin[whut](h,m,s);
   }) {
      if (h==24 && m==0 && s==0) // special case
         return origin->end()->second();
      else {
	 object d=origin->day();
	 array(cHour) ha=origin->hours();
	 int n=search(ha->hour_no(),h);
	 if (n!=-1) tr=ha[n]->minute(m)->second(s);
	 else return 0; // no such hour
      }
   }

   if (tr->hour_no()!=h || tr->minute_no()!=m)
   {
//        werror("%O %O %O -> %O %O %O (%O)\n",
//  	     tr->hour_no(),tr->minute_no(),tr->second_no(),
//  	     h,m,s,tr);
      string tr_ymd = tr->format_ymd_short();
      string origin_ymd = origin->format_ymd_short();
      if (tr_ymd != origin_ymd) {
         // Timezone adjustment has moved tr to a different day.
         // This happens eg for the first 14 seconds of 1900-01-01,
         // which move back to 1899-12-31 in the timezone Europe/Stockholm.
         if (tr_ymd < origin_ymd) {
            tr = tr->add(1, Day);
         } else {
            tr = tr->add(-1, Day);
         }
      }
      if (tr->hour_no()!=h)
	 tr=tr->add(h-tr->hour_no(),Hour);
      if (tr->minute_no()!=m)
	 tr=tr->add(m-tr->minute_no(),Minute);
      if (tr->second_no()!=s)
	 tr=tr->add(s-tr->second_no(),Second);
      if (tr->hour_no()!=h || tr->minute_no()!=m ||
	  tr->second_no()!=s) return 0; // no such hour
   }
   return tr;
}

protected mapping abbr2zones;

// dwim timezone and call dwim time of day above
// this API may change without further notice
protected object(TimeRange)|zero dwim_zone(TimeRange origin,string zonename,
                                           string whut,int ...args)
{
   if (zonename=="") return 0;

   if (zonename[0]=='"') sscanf(zonename,"\"%s\"",zonename);
   sscanf(zonename,"%*[ \t]%s",zonename);

   if(sizeof(zonename)==4 && zonename[2]=='S')
     zonename = zonename[0..1] + zonename[3..3];
   else if(sizeof(zonename)>4 && has_suffix(zonename, "DST"))
     zonename = zonename[..<3];

   if (origin->rules->abbr2zone[zonename])
         zonename=origin->rules->abbr2zone[zonename];

   Calendar.Rule.Timezone zone = Calendar.Timezone[zonename];

   if (!zone)
   {
     if (sscanf(zonename,"%[^+-]%s",string a,string b)==2 && a!="" && b!="")
     {
       TimeRange tr=dwim_zone(origin,a,whut,@args);
       if (!tr) return 0;

       return
         dwim_tod(origin->set_timezone(
                                Calendar.Timezone.make_new_timezone(
                                    tr->timezone(),
                                    Calendar.Timezone.decode_timeskew(b))),
                            whut,@args);
     }
     if(!abbr2zones)
       abbr2zones = master()->resolv("Calendar")["TZnames"]["abbr2zones"];
     array pz=abbr2zones[zonename];
     if (!pz) return 0;
     foreach (pz,string zn)
     {
       TimeRange try=dwim_zone(origin,zn,whut,@args);
       if (try && try->tzname()==zonename) return try;
     }
     return 0;
   }

   return dwim_tod(origin->set_timezone(zone),whut,@args);
}

protected mapping(string:array) parse_format_cache=([]);

protected mapping dwim_year=([ "past_lower":70, "past_upper":100,
                            "current_century":2000, "past_century":1900 ]);

object(TimeRange)|zero parse(string fmt,string arg,void|TimeRange context)
{
   [string nfmt,array q]=(parse_format_cache[fmt]||({0,0}));

   if (!nfmt)
   {
//        nfmt=replace(fmt," %","%*[ \t]%"); // whitespace -> whitespace
#define ALNU "%[^ -,./:-?[-`{-�-]"
#define AMPM "%[ampAMP.]"
#define NUME "%[0-9]"
#define ZONE "%[-+0-9A-Za-z/]"
      nfmt=replace(fmt,
		   ({"%Y","%y","%M","%W","%D","%a","%e","%h","%m","%s","%p",
		     "%t","%f","%d","%z","%n","%S"}),
		   ({ALNU,ALNU,ALNU,"%d",NUME,"%d",ALNU,"%d","%d","%d",AMPM,
		     NUME,NUME,NUME,ZONE,"%s","%d"}));

#if 1
      q=array_sscanf(fmt,"%{%*[^%]%%%1s%}")*({})*({})-({"*","%"});
#else
// slower alternatives:
      array q=Array.map(replace(fmt,({"%*","%%"}),({"",""}))/"%",
			lambda(string s){ return s[..0];})-({""});

      array q=({});
      array pr=(array)fmt;
      int i=-1;
      while ((i=search(pr,'%',i+1))!=-1) q+=({sprintf("%c",pr[i+1])});
#endif
      if (sizeof(q)==0) error("format doesn't contain anything to parse\n");
      if (q[-1]=="z") nfmt=replace(nfmt,ZONE,"%s");
      parse_format_cache[fmt]=({nfmt,q});
   }

   array res=array_sscanf(arg,nfmt);

   int i=search(res,"");
   if (i!=-1 && i<sizeof(res)-1) return 0;

   if (sizeof(res)<sizeof(q))
      return 0; // parse error

   mapping m=mkmapping(q,res);
   if (i!=-1 && m->n!="") return 0;

//     werror("%O\n",m);

//     werror("bopa %O\n %O\n %O\n %O\n",fmt,arg,nfmt,m);


   TimeRange low;

   Calendar.Calendar cal=this;


//  #define NOCATCH
#ifndef NOCATCH
   if (catch {
#else
      werror("%O\n",m);
#endif
      if (m->n && m->n!="") return 0;

      if (m->Y)
	 m->Y=default_rules->language[f_year_number_from_name](m->Y);

      if (!undefinedp(m->Y) && m->D && (int)m->M)
	 low=m->day=cal->Day(m->Y,(int)m->M,(int)m->D);


      if (m->d)
      {
	 int y,mo,d;

	 if (sizeof(m->d)==6)
	 {
	    [y,mo,d]=(array(int))(m->d/2);
	    if (y<dwim_year->past_lower)
               y+=dwim_year->current_century;
            else
               y+=dwim_year->past_century;
	 }
	 else if (sizeof(m->d)==8)
	    [y,mo,d]=(array(int))array_sscanf(m->d,"%4s%2s%2s");
	 else return 0;

	 low=m->day=cal->Day(y,mo,d);
      }
      else
      {
	 if (!undefinedp(m->Y)) m->year=cal->Year(m->Y);
	 else if (m->y)
	 {
	    if (sizeof(m->y)<3)
	    {
	       m->y=(int)m->y;
               // FIXME? these should be adjustable for different calendars.
	       if (m->y<dwim_year->past_lower)
                  m->y+=dwim_year->current_century;
	       else if (m->y<dwim_year->past_upper)
                  m->y+=dwim_year->past_century;
	    }
	    low=m->year=cal->Year(m->y);
	 }
	 else low=m->year=context?context->year():cal->Year();

	 if (m->M)
	 {
	    m->month=low=m->year->month(m->M);
	 }
	 if (m->W)
	    m->week=low=m->year->week("w"+m->W);

	 if (!undefinedp(m->D))
	    m->day=low=(m->month||(context?context->month():cal->Month()))
	       ->day((int)m->D);
	 else if (!undefinedp(m->a))
	    m->day=low=(m->month || m->year)->day(m->a);
	 else if (!undefinedp(m->e))
	    m->day=low=(m->week||(context?context->week():cal->Week()))
	       ->day(m->e);
	 else
	    low=m->day=context?context->day():cal->Day();

	 if (m->day && undefinedp(m->Y) && undefinedp(m->y) && m->e)
	    if (m->month)
	    {
	 // scan for closest year that matches
	       cYear y2=m->day->year();
	       object d2;
	       int i;
	       for (i=0; i<20; i++)
	       {
		  d2=(y2+i)->place(m->day);
		  if (d2 && d2->week()->day(m->e)==d2) break;
		  d2=(y2-i)->place(m->day);
		  if (d2 && d2->week()->day(m->e)==d2) break;
	       }
	       if (i==20) return 0;
	       low=m->day=d2;
	    }
	    else
	    {
	 // scan for closest month that matches
	       cYear m2=m->day->month();
	       object d2;
	       int i;
	       for (i=0; i<20; i++)
	       {
		  d2=(m2+i)->place(m->day);
		  if (d2 && d2->week()->day(m->e)==d2) break;
		  d2=(m2-i)->place(m->day);
		  if (d2 && d2->week()->day(m->e)==d2) break;
	       }
	       if (i==20) return 0;
	       low=m->day=d2;
	    }
      }

      int h=0,mi=0,s=0;
      float sub_second;
      string|zero g=0;

      if (m->t)
      {
	 if (sizeof(m->t)==6)
	    [h,mi,s]=(array(int))(m->t/2),g="second";
	 else if (sizeof(m->t)==4)
	    [h,mi]=(array(int))(m->t/2),g="minute";
	 else return 0;
      }
      else
      {
	 if (!undefinedp(m->h)) h=m->h,g="hour";
	 if (!undefinedp(m->m)) mi=m->m,g="minute";
	 if (!undefinedp(m->s)) s=m->s,g="second";
	 if (!undefinedp(m->f)) sub_second=(float)("0."+m->f+"0"*9)[..10];
      }

      if (!undefinedp(m->p))
      {
	 switch (lower_case(m->p)-".")
	 {
	    case "am":
	       if (h==12) h=0;
	       break;
	    case "pm":
	       if (h!=12) h+=12;
	       break;
	    default:
	       return 0; // need "am" or "pm"
	 }
      }
      if (m->z) // zone
	 low = dwim_zone(low,m->z,g,h,mi,s);
      else if (g)
	 low = dwim_tod(low,g,h,mi,s);
      else if (!undefinedp(m->S))
	 low = Second(m->S);
      if (sub_second)
	 low = low->fraction(sub_second);
      return low;

#ifndef NOCATCH
   })
#endif
       return 0;
}

//! method Day dwim_day(string date)
//! method Day dwim_day(string date,TimeRange context)
//!	Tries a number of different formats on the given date (in order):
//!	<pre>
//!     <ref>parse</ref> format                  as in
//!	"%y-%M-%D (%M) -W%W-%e (%e)"  "2000-03-20 (Mar) -W12-1 (Mon)"
//!	"%y-%M-%D"		      "2000-03-20", "00-03-20"
//!	"%M%/%D/%y"	              "3/20/2000"
//!	"%D%*[ /]%M%*[ /-,]%y"	      "20/3/2000" "20 mar 2000" "20/3 -00"
//!	"%e%*[ ]%D%*[ /]%M%*[ /-,]%y" "Mon 20 Mar 2000" "Mon 20/3 2000"
//!	"-%y%*[ /]%D%*[ /]%M"	      "-00 20/3" "-00 20 mar"
//!	"-%y%*[ /]%M%*[ /]%D"	      "-00 3/20" "-00 march 20"
//!	"%y%*[ /]%D%*[ /]%M"	      "00 20 mar" "2000 20/3"
//!	"%y%*[ /]%M%*[ /]%D"	      "2000 march 20"
//!	"%D%.%M.%y"	              "20.3.2000"
//!	"%D%*[ -/]%M"                 "20/3" "20 mar" "20-03"
//!	"%M%*[ -/]%D"		      "3/20" "march 20"
//!     "%M-%D-%y"                    "03-20-2000"
//!     "%D-%M-%y"                    "20-03-2000"
//!     "%e%*[- /]%D%*[- /]%M"        "mon 20 march"
//!     "%e%*[- /]%M%*[- /]%D"        "mon/march/20"
//!	"%e%*[ -/wv]%W%*[ -/]%y"      "mon w12 -00" "1 w12 2000"
//!	"%e%*[ -/wv]%W"               "mon w12"
//!     "%d"                          "20000320", "000320"
//!     "today"                       "today"
//!	"last %e"                     "last monday"
//!	"next %e"                     "next monday"
//!	</pre>
//!
//! note:
//!	Casts exception if it fails to dwim out a day.
//!	"dwim" means do-what-i-mean.

/* tests:

Calendar.dwim_day("2000-03-20 (Mar) -W12-1 (Mon)");
Calendar.dwim_day("20/3/2000");
Calendar.dwim_day("20 mar 2000");
Calendar.dwim_day("20/3 -00");
Calendar.dwim_day("Mon 20 Mar 2000" );
Calendar.dwim_day("Mon 20/3 2000");
Calendar.dwim_day("2000-03-20");
Calendar.dwim_day("00-03-20");
Calendar.dwim_day("20000320");
Calendar.dwim_day("000320");
Calendar.dwim_day("-00 20/3" );
Calendar.dwim_day("-00 20 mar");
Calendar.dwim_day("-00 3/20" );
Calendar.dwim_day("-00 march 20");
Calendar.dwim_day("00 20 mar" );
Calendar.dwim_day("2000 20/3");
Calendar.dwim_day("2000 march 20");
Calendar.dwim_day("20/3" );
Calendar.dwim_day("20 mar" );
Calendar.dwim_day("20-03");
Calendar.dwim_day("3/20" );
Calendar.dwim_day("march 20");
Calendar.dwim_day("mon w12 -00" );
Calendar.dwim_day("1 w12 2000");
Calendar.dwim_day("mon w12");
Calendar.dwim_day("monday" );
Calendar.dwim_day("1");
Calendar.dwim_day("today");
Calendar.dwim_day("last monday");
Calendar.dwim_day("next monday");
Calendar.dwim_day("Sat Jun  2");

*/

protected constant dwim_day_strings=
({
  "%y-%M-%D (%*s) -W%W-%e (%e)",
  "%y-%M-%D",
  "%M/%D/%y",
  "%D%*[ /]%M%*[- /,]%y",
  "%M %D%*[- /,]%y",
  "%e%*[, ]%D%*[a-z:]%*[ /]%M%*[-/ ,]%y",
  "%e%*[, ]%M%*[ ,]%D%*[ ,]%y",
  "-%y%*[ /]%D%*[ /]%M",
  "-%y%*[ /]%M%*[ /]%D",
  "%y%*[ /]%M%*[ /]%D",
  "%y%*[ /]%D%*[ /]%M",
  "%D.%M.%y",
  "%D%*[- /]%M",
  "%M%*[- /]%D",
  "%M-%D-%y",
  "%D-%M-%y",
  "%e%*[- /]%D%*[- /]%M",
  "%e%*[- /]%M%*[- /]%D",
  "%e%*[- /wv]%W%*[ -/]%y",
  "%e%*[- /wv]%W",
  "%d"
});

cDay dwim_day(string day,void|TimeRange context)
{
   cDay d;

   foreach ( dwim_day_strings,
	     string dayformat)
      if ( (d=parse(dayformat+"%n",day,context)) )
	 return d;

   cDay t=context?context->day():Day();
   if ( (d=parse("%e",day,context)) )
   {
      if (d>=t) return d;
      else return (d->week()+1)->place(d);
   }

   if (sizeof(day)==4)
     catch {
       d = parse("%M/%D",day/2*"/",context);
       if(d) return d;
     };

   if (day=="today") return t;
   if (day=="tomorrow") return t+1;
   if (day=="yesterday") return t-1;
   if (sscanf(day,"last %s",day))
   {
      cDay d=dwim_day(day);
      return (d->week()-1)->place(d);
   }
   if (sscanf(day,"next %s",day))
   {
      cDay d=dwim_day(day);
      return (d->week()+1)->place(d);
   }

   error("Failed to dwim day from %O\n",day);
}

//! method Day dwim_time(string date_time)
//! method Day dwim_time(string date_time, TimeRange context)
//!	Tries a number of different formats on the given date_time.
//!
//! note:
//!	Casts exception if it fails to dwim out a time.
//!	"dwim" means do-what-i-mean.

TimeofDay dwim_time(string what,void|TimeRange cx)
{
   TimeofDay t;

// #define COLON "$*[ :]"
#define COLON ":"
#define SPACED(X) replace(X," ","%*[ ]")

   what = String.trim(what);

   if (sizeof(what)>22 &&
       (t=http_time(what,cx))) return t;
   if (sizeof(what)>12 &&
       (t=parse(SPACED("%e %M %D %h:%m:%s %Y"),what,cx))) return t; // ctime
   if (sizeof(what)>15 &&
       (t=parse(SPACED("%e %M %D %h:%m:%s %z %Y"),what,cx))) return t;
   if (sizeof(what)>19 &&
       (t=parse(SPACED("%e %M %D %h:%m:%s %z DST %Y"),what,cx))) return t;

   foreach ( dwim_day_strings +
	     ({""}),
	     string dayformat )
      foreach ( ({ "%t %z",
		   "T%t %z",
		   "T%t%z",
		   "T%t",
		   "%h"COLON"%m"COLON"%s %p %z",
		   "%h"COLON"%m"COLON"%s %p",
		   "%h"COLON"%m"COLON"%s %z",
		   "%h"COLON"%m"COLON"%s%z",
		   "%h"COLON"%m"COLON"%s",
		   "%h"COLON"%m %p %z",
		   "%h"COLON"%m %p",
		   "%h"COLON"%m %z",
		   "%h"COLON"%m%z",
		   "%h"COLON"%m",
		   "T%h"COLON"%m"COLON"%s %z",
		   "T%h"COLON"%m"COLON"%s%z",
		   "T%h"COLON"%m"COLON"%s",
		   "T%h"COLON"%m %z",
		   "T%h"COLON"%m%z",
		   "T%h"COLON"%m",
		   "%h%*[ ]%p",
		   "%*[a-zA-Z.] %h"COLON"%m"COLON"%s %p %z",
		   "%*[a-zA-Z.] %h"COLON"%m"COLON"%s %p",
		   "%*[a-zA-Z.] %h"COLON"%m"COLON"%s %z",
		   "%*[a-zA-Z.] %h"COLON"%m"COLON"%s%z",
		   "%*[a-zA-Z.] %h"COLON"%m"COLON"%s",
		   "%*[a-zA-Z.] %h"COLON"%m %p %z",
		   "%*[a-zA-Z.] %h"COLON"%m %p",
		   "%*[a-zA-Z.] %h"COLON"%m %z",
		   "%*[a-zA-Z.] %h"COLON"%m%z",
		   "%*[a-zA-Z.] %h"COLON"%m",
		   "%*[a-zA-Z.] %h%*[ ]%p",
		   "%t%z",
		   "%t",
		}),
		string todformat )
      {
//  	 werror("try: %O\n     %O\n",
//  		dayformat+"%*[ ,]"+todformat,
//  		todformat+"%*[ ,]"+dayformat);
	 if (dayformat=="")
	 {
	    if ( (t=parse(todformat+"%*[ ]%n",what,cx)) ) return t;
	 }
	 else
	 {
	    if ( (t=parse(dayformat+"%*[ ,:]"+todformat,what,cx)) ) return t;
	    if ( (t=parse(todformat+"%*[ ,:]"+dayformat,what,cx)) ) return t;
	 }
      }

   error("Failed to dwim time from %O\n",what);
}

// Parses time according to HTTP 1.1 (RFC 2616) HTTP-date token.
object(TimeofDay)|zero http_time(string what, void|TimeRange cx)
{
  TimeofDay t;

  constant date1 = "%D %M %Y"; // 2+1+3+1+4=11
  constant date2 = "%D-%M-%y"; // 2+1+3+1+2=9
  constant date3 = "%M %*[ ]%D"; // 2+1+2=5
  constant time = "%h:%m:%s"; // 2+1+2+1+2=8

  // 3+2+ 11 +1+ 8 +4 = 29
  // RFC 1123 (and RFC 822 which it bases its timestamp format on)
  // supports more variations than we support here.
  constant rfc1123_date = "%e, "+date1+" "+time+" %z";

  // 6+2+ 9 +1+ 8 +4 = 33
  constant rfc850_date = "%e, "+date2+" "+time+" %z";

  // 3+1+ 5 +1+ 8 +1+4 = 23
  constant asctime_date = "%e "+date3+" "+time+" %Y";

  if( sizeof(what)<23 ) return 0;

  if( (t=parse(rfc1123_date, what, cx)) ||
      (t=parse(rfc850_date, what, cx)) ||
      (t=parse(asctime_date+" %z", what+" GMT", cx)) )
    return t;

  return 0;
}

//-- auxillary functions------------------------------------------------

//!
//! function mapping(string:int) datetime(int|void unix_time)
//!     Replacement for localtime; gives back a mapping:
//!	<pre>
//!	 ([ "year":     int        // year number (2000 AD=2000, 1 BC==0)
//!	    "month":    int(1..)   // month of year
//!	    "day":      int(1..)   // day of month
//!	    "yearday":  int(1..)   // day of year
//!	    "week":	int(1..)   // week of year
//!	    "week_day": int(1..)   // day of week (depending on calendar)
//!	    "unix":     int        // unix time
//!	    "julian":   float      // julian day
//!	    "hour":     int(0..)   // hour of day, including dst
//!	    "minute":   int(0..59) // minute of hour
//!	    "second":   int(0..59) // second of minute
//!	    "fraction": float      // fraction of second
//!	    "timezone": int        // offset to utc, including dst
//!	 ]);
//!	</pre>
//!	This is the same as calling <ref>Second</ref>()-><ref to=Second.datetime>datetime</ref>().
//!
//! function string datetime_name(int|void unix_time)
//! function string datetime_short_name(int|void unix_time)
//!     Compat functions; same as <ref>format_iso</ref>
//!	and <ref>format_iso_short</ref>.
//!
//! function string format_iso(void|int unix_time)
//! function string format_iso_short(void|int unix_time)
//! function string format_iso_tod(void|int unix_time)
//! function string format_day_iso(void|int unix_time)
//! function string format_day_iso_short(void|int unix_time)
//!	Format the object into nice strings;
//!	<pre>
//!	iso    "2000-06-02 (Jun) -W22-5 (Fri) 11:57:18 CEST"
//!     iso_short   "2000-06-02 11:57:18"
//!	iso_tod     "11:57:18"
//!	</pre>

// Sane replacement for localtime().
mapping(string:int) datetime(int|void unix_time)
{
   return Second("unix",unix_time||time())->datetime();
}

string datetime_name(int|void unix_time)
{
   return Second("unix",unix_time||time())->format_iso();
}

string datetime_short_name(int|void unix_time)
{
   return Second("unix",unix_time||time())->format_iso_short();
}

string format_iso(int|void unix_time)
{
   return Second("unix",unix_time||time())->format_iso();
}

string format_iso_short(int|void unix_time)
{
   return Second("unix",unix_time||time())->format_iso_short();
}

string format_iso_tod(int|void unix_time)
{
   return Second("unix",unix_time||time())->format_iso_tod();
}

string format_day_iso(int|void unix_time)
{
   return Day("unix",unix_time||time())->format_iso();
}

string format_day_iso_short(int|void unix_time)
{
   return Day("unix",unix_time||time())->format_iso_short();
}
