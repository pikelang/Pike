//!
//! module Calendar
//! submodule YMD
//!
//! base for all Roman-kind of Calendars,
//! ie, one with years, months, weeks and days
//!

#pike __REAL_VERSION__

//  #pragma strict_types

import ".";
inherit Time:Time;

#include "constants.h"

#define this this_object()

// ----------------
// virtual methods to tell how this calendar works
// ----------------

static array(int) year_from_julian_day(int jd);
static int julian_day_from_year(int year);
static int year_remaining_days(int y,int yday);

static array(int) year_month_from_month(int y,int m); // [y,m,ndays,myd]
static array(int) month_from_yday(int y,int yday); // [m,day-of-month,ndays,myd]

static array(int) week_from_week(int y,int w);   // [y,w,wd,ndays,wjd]
static array(int) week_from_julian_day(int jd);  // [y,w,wd,ndays,wjd]

static string f_month_name_from_number;
static string f_month_shortname_from_number;
static string f_month_number_from_name;
static string f_month_day_name_from_number;
static string f_week_name_from_number;
static string f_week_day_number_from_name;
static string f_week_day_shortname_from_number;
static string f_week_day_name_from_number;
static string f_year_name_from_number;
static string f_year_number_from_name;


static int(0..1) year_leap_year(int y);

static int compat_week_day(int n);

//------------------------------------------------------------------------
//! class YMD
//! 	Base (virtual) time period of the Roman-kind of calendar.
//! inherits TimeRange
//------------------------------------------------------------------------

class YMD
{
   inherit TimeRange;

// --- generic for all YMD:

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

   int mnd=CALUNKNOWN;  // days in current month
   int utco=CALUNKNOWN; // [*] distance to UTC
   string tzn=0;        // timezone name

//          // ^^^ might be uninitialized (CALUNKNOWN)

   Ruleset rules;
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

   void make_week() // set w from y and yd
   {
      int wnd,wjd;
      [wy,w,wd,wnd,wjd]=week_from_julian_day(jd);
   }

   int __hash() { return jd; }

// --- query

//! method float fraction_no()
//! method int hour_no()
//! method int julian_day()
//! method int leap_year()
//! method int minute_no()
//! method int month_day()
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

//! function method int unix_time()
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
      return rules->language[f_year_name_from_number](y);
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

   int leap_year() { return year_leap_year(y); }

   int hour_no() { return 0; }
   int minute_no() { return 0; }
   int second_no() { return 0; }
   float fraction_no() { return 0.0; }
   
//! function method datetime()
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

   mapping datetime(void|int skip_stuff)
   {
      if (m==CALUNKNOWN) make_month();
      if (w==CALUNKNOWN) make_week();
      if (skip_stuff) // called from timeofday
	 return ([ "year":     y,
		   "month":    m,
		   "day":      md,
		   "yearday":  yd-1,
		   "week":     w,
		   "week_day": compat_week_day(wd)
	 ]);
      else
      {
	 return ([ "year":     y,
		   "month":    m,
		   "day":      md,
		   "yearday":  yd-1,
		   "week":     w,
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

   string format_iso_ymd()
   {
      if (m==CALUNKNOWN) make_month();
      if (w==CALUNKNOWN) make_week();
      return sprintf("%04d-%02d-%02d (%s) -W%02d-%d (%s)",
		     ((yd < 1)?y-1:y),m,md,
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
		 ((yd < 1)?y-1:y));
   }

   string format_ymd()
   {
      if (m==CALUNKNOWN) make_month();
      return sprintf("%04d-%02d-%02d",((yd < 1)?y-1:y),m,md);
   }

   string format_ymd_short()
   {
      if (m==CALUNKNOWN) make_month();
      return sprintf("%04d%02d%02d",((yd < 1)?y-1:y),m,md);
   }

   string format_ymd_xshort()
   {
      if (m==CALUNKNOWN) make_month();
      return sprintf("%02d%02d%02d",((yd < 1)?y-1:y)%100,m,md);
   }

   string format_iso_week()
   {
      if (w==CALUNKNOWN) make_week();
      return sprintf("%04d-W%02d",y,w);
   }

   string format_iso_week_short()
   {
      if (w==CALUNKNOWN) make_week();
      return sprintf("%04d%02d",y,w);
   }

   string format_week()
   {
      return sprintf("%04d-%s",y,week_name());
   }

   string format_week_short()
   {
      return sprintf("%04d%s",y,week_name());
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

   string format_smtp()
   {
      if (m==CALUNKNOWN) make_month();
      int u=utc_offset();
      return sprintf("%s, %s %s %s 00:00:00 %+03d%02d",
		     week_day_shortname(),
		     month_day_name(),month_shortname(),year_name(),
		     -u/3600,max(u,-u)/60%60);
   }

   string format_elapsed()
   {
      return sprintf("%dd",number_of_days());
   }

// --- size and move ---

   static TimeRange _set_size(int n,TimeRange t)
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
			 t->number_of_months()*n,wd,w)
	       ->autopromote();
      }

// weeks are not
      if (t->is_week)
      {
	 if (wd==CALUNKNOWN) make_week();
	 if (wd==1) return Week("ymd_yjwm",rules,y,yjd,jd,w,t->n*n,md,m,mnd);
      }

// fallback on days
      if (t->is_ymd)
	 return Day("ymd_ydmw",rules,y,yjd,jd,yd,
		    n*t->number_of_days(),m,md,w,wd,mnd);

      error("set_size: incompatible class %O\n",
	    object_program(t));
   }

   static TimeRange _add(int n,TimeRange step)
   {
      if (step->is_ymd)
	 return _move(n,step);
      if (step->is_timeofday)
	 return second()->range(second(-1))->add(n,step);

      error("add: incompatible class %O\n",
	    object_program(step));
   }

   array(int(-1..1)) _compare(TimeRange with)
   {
      if (objectp(with))
	 if (with->is_timeofday)
	 {
      // wrap
	    array(int(-1..1)) cmp=with->_compare(this_object());

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

   array(cYear) years(int ...range)
   {
      int from=1,n=number_of_years(),to=n;

      if (sizeof(range)) 
	 if (sizeof(range)<2)
	    error("Illegal numbers of arguments to days()\n");
	 else
	 {
	    [from,to]=range;
	    if (from>=n) return ({}); else if (from<0) from=0;
	    if (to>=n) to=n; else if (to<from) return ({});
	 }

      return map(enumerate(1+to-from,1,y+from),
		 lambda(int x) 
		 { return Year("ymd_yn",rules,x,1); });
   }

   cYear year(void|int m) 
   { 
      if (!m || (!n&&m==-1))
	 return Year("ymd_y",rules,y,yjd,1);

      if (m<0) m=number_of_years()+m;

      array(TimeRange) res=years(m,m);
      if (sizeof(res)==1) return res[0];
      error("not in range (Year 0..%d exist)\n",
	    number_of_years()-1);
   }
   

// days

   int number_of_days();

   array(cDay) days(int ...range)   
   {
      int from=1,n=number_of_days(),to=n;

      if (sizeof(range)) 
	 if (sizeof(range)<2)
	    error("Illegal numbers of arguments to days()\n");
	 else
	 {
	    [from,to]=range;
	    if (from>n) return ({}); else if (from<1) from=1;
	    if (to>n) to=n; else if (to<from) return ({});
	 }

      int zy=y;
      int zyd=yd+from-1;
      int zjd=jd+from-1;
      int zyjd=yjd;
      array(cDay) res=({});

      to-=from-1;

      if (zyd<1)
      {
	 [zy,zyjd]=year_from_julian_day(zjd);
	 zyd=zjd-zyjd+1;
      }

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

   cDay day(int ... mp) 
   {
      int m;
      if (sizeof(mp)) m=mp[0];
      else m=1;

      if (m==-1 && !n)
	 return Day("ymd_yd",rules,y,yjd,jd,yd,1);

      if (m<0) m+=1+number_of_days();

      array(TimeRange) res=days(m,m);
      if (sizeof(res)==1) return res[0];
      error("not in range (Day 1..%d exist)\n",
	    number_of_days());
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
	    number_of_months(),this_object());
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

   static TimeRange get_unit(string unit,int m)
   {
      if (!n) return day()[unit]();
      if (m<0) m+=::`[]("number_of_"+unit+"s")();
      array(TimeRange) res=::`[](unit+"s")(m,m);
      if (sizeof(res)==1) return res[0];
      error("not in range ("+unit+" 0..%d exist)\n",
	    ::`[]("number_of_"+unit+"s")()-1);
   }

   static array(TimeRange) get_timeofday(string unit,
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
		     map(z[i..sizeof(z)-2],"add",uq,sec);
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

// ----------------------------------------
// virtual functions needed
// ----------------------------------------

   string nice_print();
   string _sprintf(int t,mapping m)
   {
      switch (t)
      {
	 case 't':
	    return "Calendar."+calendar_name()+".YMD";
	 default:
	    return ::_sprintf(t,m);
      }
   }

   void create_julian_day(int|float jd);
   static TimeRange _move(int n,YMD step);
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
//!	It's possible to create the standard week
//!	by using three different methods; either the normal
//!	way - from standard unix time or the julian day,
//!	and also, for more practical use, from the year number.
//!

   void create(mixed ...args)
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
	    m=md=w=wd=CALUNKNOWN;
	    yd=1;
	    return;
	 case "ymd_yn":
	    rules=args[1];
	    y=args[2];
	    jd=yjd=julian_day_from_year(y);
	    n=args[3];
	    m=md=w=wd=CALUNKNOWN;
	    yd=1;
	    return;
	 default:
	    if (intp(args[0]) && sizeof(args)==1)
	    {
	       rules=default_rules;
	       y=args[0];
	       jd=yjd=julian_day_from_year(y);
	       n=1;
	       m=md=w=wd=CALUNKNOWN;
	       yd=1;
	       return;
	    }
	    else if (stringp(args[0]))
	    {
	       y=default_rules->language[f_year_number_from_name](args[0]);
	       rules=default_rules;
	       jd=yjd=julian_day_from_year(y);
	       n=1;
	       m=md=w=wd=CALUNKNOWN;
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
	 wd=w=CALUNKNOWN; // unknown
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

   string _sprintf(int t,mapping m)
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
	    return ::_sprintf(t,m);
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
	 return month()->add(m,step)->set_size(this_object());

//        if (step->is_week)
//  	 return week()->add(m,step)->set_size(this_object());

      if (step->is_ymd)
	 return Day("ymd_jd",rules,
		    yjd+m*step->number_of_days(),number_of_days())
	    ->autopromote();
      
      error("_move: Incompatible type %O\n",step);
   }

   static void convert_from(TimeRange other)
   {
      ::convert_from(other);
      if (other->number_of_years)
	 n=other->number_of_years();
      else
	 n=0;
   }

   TimeRange place(TimeRange what,void|int force)
   {
      if (what->is_day)
      {
	 int yd=what->yd;
	 return Day("ymd_yd",rules,y,yjd,yjd+yd-1,yd,what->n);
      }

      if (what->is_week)
      {
	 cWeek week=Week("ymd_yw",rules,y,what->w,what->n);
	 if (!force && week->y!=y) return 0; // not this year
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
	    error("no such month %O in %O\n",mp[0],this_object());

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

   cYear set_ruleset(Ruleset r)
   {
      return Year("ymd_y",r,y,yjd,n);
   }
}


// ----------------------------------------------------------------
//   Month
// ----------------------------------------------------------------

function(mixed...:cMonth) Month=cMonth;
class cMonth
{
   inherit YMD;

   constant is_month=1;

   int nd; // number of days
   int nw; // number of weeks

   void create(mixed ...args)
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
	       w=wd=CALUNKNOWN;
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
	       md=1;
	       nw=nd=CALUNKNOWN;
	       return;
	    case "ymd_jd":
	       rules=args[1];
	       create_julian_day(args[2]);
	       n=args[3];
	       return;
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
	 nw=wd=w=CALUNKNOWN; // unknown
      }
   }

   string _sprintf(int t,mapping m)
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
	    return ::_sprintf(t,m);
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
      if (!n) return day()->nice_print()+" 0:00 sharp";
      cMonth mo=month(-1);
      if (mo->y==y)
	 return sprintf("%s..%s %s",
			month_shortname(),
			mo->month_shortname(),
			year_name());
      return nice_print()+" .. "+month(-1)->nice_print();
   }

   cDay beginning()
   {
      return Month("ymd_yjmw",rules,y,yjd,jd,m,0,wd,w)
	 ->autopromote();
   }

   cDay end()
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
	    return Month("ymd_yjmw",rules,y,yjd,jd,m,n1,wd,w)
	       ->autopromote();
	 }

	 int d1=jd;
	 int d2=to->jd;
	 if (d2<d1)
	    error("distance: negative distance (%d days)\n",d2-d1);
	 return Day("ymd_ydmw",rules,y,yjd,jd,yd,d2-d1,m,1,w,wd,mnd)
	    ->autopromote();
      }

      error("distance: Incompatible type %O\n",to);
   }

   static void convert_from(TimeRange other)
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

   TimeRange place_day(int day,int day_n,void|int force)
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

   cMonth set_ruleset(Ruleset r)
   {
      return Month("ymd_yjmw",r,y,yjd,jd,m,n,wd,w);
   }

// --- needs to be defined

   static int months_to_month(int y,int m);
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
// ----------------------------------------------------------------

function(mixed...:cWeek) Week=cWeek;
class cWeek
{
   inherit YMD;

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

   void create(mixed ...args)
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
	       y=args[2];
	       w=args[3];
	       n=args[4];
	       m=md=CALUNKNOWN;
	       [y,w,wd,int nd,jd]=week_from_week(y,w);
	       yjd=julian_day_from_year(y);
	       yd=1+jd-yjd;
	       wy=y;
	       if (n!=1) nd=CALUNKNOWN;
	       return;
	    case "ymd_yjwm":
	       rules=args[1];
	       y=args[2];
	       yjd=args[3];
	       jd=args[4];
	       yd=1+jd-yjd;
	       w=args[5];
	       n=args[6];
	       md=args[7];
	       m=args[8];
	       mnd=args[9];
	       wd=1;
	       wy=y;
	       nd=CALUNKNOWN;
	       return;
	    case "ymd_jd":
	       rules=args[1];
	       create_julian_day(args[2]);
	       n=args[3];
	       return;
	    default:
	       if (intp(args[0]) && sizeof(args)==2)
	       {
		  create("ymd_yw",default_rules,args[0],args[1],1);
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
	 int zwd;
	 [y,w,zwd,int nd,jd]=week_from_julian_day(_jd);
	 yjd=julian_day_from_year(y);
	 yd=1+jd-yjd;

	 n=1;
	 wd=1;
	 wy=y;
	 md=m=CALUNKNOWN; // unknown
      }
   }

   string _sprintf(int t,mapping m)
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
	    return ::_sprintf(t,m);
      }
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
      if (!n) return day()->nice_print()+" 0:00 sharp";
      cWeek wo=week(-1);
      if (wo->y==y)
	 return sprintf("%s..%s %s",
			week_name(),
			wo->week_name(),
			year_name());
      return nice_print()+" .. "+week(-1)->nice_print();
   }

   cDay beginning()
   {
      return Week("ymd_yjwm",rules,y,yjd,jd,w,0,md,m,mnd)
	 ->autopromote();
   }

   cDay end()
   {
      return Week("ymd_yw",rules,y,w+n,0)
	 ->autopromote();
   }

// --- week position and distance

   TimeRange distance(TimeRange to)
   {
      if (to->is_timeofday)
	 return hour()->distance(to);
      
      if (to->is_week)
      {
	 int n1=weeks_to_week(to->y,to->w);
	 if (n1<0)
	    error("distance: negative distance (%d weeks)\n",n1);
	 return Week("ymd_yjwm",rules,y,yjd,jd,w,n1,md,m,mnd)
	    ->autopromote();
      }

      if (to->julian_day)
      {
	 int d1=jd;
	 int d2=to->julian_day();
	 if (d2<d1)
	    error("distance: negative distance (%d days)\n",d2-d1);
	 return Day("ymd_ydmw",rules,y,yjd,jd,yd,d2-d1,m,md,w,1,mnd)
	    ->autopromote();
      }

      error("distance: Incompatible type %O\n",to);
   }

   static void convert_from(TimeRange other)
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
	 return Week("ymd_yw",rules,y,w+x*step->n,n)
	    ->autopromote();

      if (step->is_year)
	 return year()->add(x,step)->place(this_object(),1);

      if (step->number_of_days)
	 return Day("ymd_jd",rules,
		    jd+x*step->number_of_days(),number_of_days())
	    ->autopromote();

      error("add: Incompatible type %O\n",step);
   }

   TimeRange place_day(int day,int day_n,int force)
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
	 return Week("ymd_yw",rules,y,w,what->number_of_weeks());

      if (what->is_day)
	 return place_day(what->week_day(),what->n,force);

      if (what->is_timeofday)
	 return place(what->day(),force)->place(what,force);

      error("place: Incompatible type %O\n",what);
   }

// --- Week to other units

   int number_of_years()
   {
      if (n<=1) return 1;

      [int y2,int w2,int wd2,int nd2,int jd2]=week_from_week(y,w+n);
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
	    error("no such day %O in %O\n",mp[0],this_object());

	 return ::day(num);
      }
      else
	 return ::day(@mp);
   }

   cWeek set_ruleset(Ruleset r)
   {
      return Week("ymd_yjwm",r,y,yjd,jd,w,n,md,m,mnd);
   }

// --- needs to be defined

   static int weeks_to_week(int y,int m);
}

// ----------------------------------------------------------------
//   Day
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

   void create(mixed ...args)
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
	       w=args[9];
	       wd=args[10];
	       mnd=args[11];
	       nw=CALUNKNOWN;
	       return;
	    case "ymd_yd":
	       rules=args[1];
	       y=args[2];
	       yjd=args[3];
	       jd=args[4];
	       yd=args[5];
	       n=args[6];
	       wd=nw=md=m=w=CALUNKNOWN;
	       return;
	    case "ymd_jd":
	       rules=args[1];
	       create_julian_day(args[2]);
	       n=args[3];
	       wd=nw=md=m=w=CALUNKNOWN;
	       return;
	    case "unix_r":
	    case "julian_r":
	    case "unix":
	    case "julian":
	       break;
	    default:
	       rules=default_rules;
	       wd=nw=md=m=w=CALUNKNOWN;
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
      nw=md=m=wd=w=CALUNKNOWN; // unknown

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

   string _sprintf(int t,mapping m)
   {
      switch (t)
      {
	 case 'O':
	    if (n!=1) 
	       return sprintf("Day(%s)",nice_print_period());
	    return sprintf("Day(%s)",nice_print());
	 case 't':
	    return "Calendar."+calendar_name()+".Day";
	 default:
	    return ::_sprintf(t,m);
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
      if (!n) return nice_print()+" 0:00 sharp";
      return nice_print()+" .. "+day(-1)->nice_print();
   }

   cDay beginning()
   {
      return Day("ymd_ydmw",rules,y,yjd,jd,yd,0,m,md,w,wd,mnd);
   }

   cDay end()
   {
      return Day("ymd_jd",rules,jd+n,0)
	 ->autopromote();
   }

   static void convert_from(TimeRange other)
   {
      ::convert_from(other);
      if (other->number_of_days)
	 n=other->number_of_days();
      else
	 n=0;
   }

// --- Day _move

   static TimeRange _move(int x,YMD step)
   {
      if (step->is_year)
	 return year()->add(x,step)->place(this_object(),1);

      if (step->is_month)
	 return month()->add(x,step)->place(this_object(),1);

      if (step->is_week)
	 return week()->add(x,step)->place(this_object(),1);

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
			"day (DST shift)\n",what,this_object());
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
	 return Day("ymd_ydmw",rules,y,yjd,jd,yd,d2-d1,m,md,w,wd,mnd)
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

   cDay set_ruleset(Ruleset r)
   {
      return Day("ymd_ydmw",r,y,yjd,jd,yd,n,m,md,w,wd,mnd);
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
   int month_shortname() { return RBASE->month_shortname(); }
   int month_day() { return RBASE->month_day(); }
   int month_day_name() { return RBASE->month_day_name(); }
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
   static int(0..1) create_backtry(mixed ... args)			\
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

class cMinute
{
   inherit Time::cMinute;
   inherit YMD_Time;
   OVERLOAD_TIMEOFDAY;
}

class cSecond
{
   inherit Time::cSecond;
   inherit YMD_Time;
   OVERLOAD_TIMEOFDAY;
}

class cFraction
{
   inherit Time::cFraction;
   inherit YMD_Time;
   OVERLOAD_TIMEOFDAY;
}

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
   int month_shortname() { return RBASE->month_shortname(); }
   int month_day() { return RBASE->month_day(); }
   int month_day_name() { return RBASE->month_day_name(); }
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
//! global convinience functions
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
//!     %f fraction of a second (needs %s)
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
static TimeRange dwim_tod(TimeRange origin,string whut,int h,int m,int s)
{
   TimeRange tr;
   if (catch {
      tr=origin[whut](h,m,s);
   } && h==24 && m==0 && s==0) // special case
      return origin->end()->second();

   if (tr->hour_no()!=h || tr->minute_no()!=m)
   {
//        werror("%O %O %O -> %O %O %O (%O)\n",
//  	     tr->hour_no(),tr->minute_no(),tr->second_no(),
//  	     h,m,s,tr);
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

// dwim timezone and call dwim time of day above
// this API may change without further notice
static TimeRange dwim_zone(TimeRange origin,string zonename,
			   string whut,int ...args)
{
   if (zonename=="") return 0;

   if (zonename[0]=='"') sscanf(zonename,"\"%s\"",zonename);
   sscanf(zonename,"%*[ \t]%s",zonename);

   // Ugly fix for synonyms. This suppport should of course be
   // added in a lower layer when the next refactoring occurs.
   zonename = ([ "MEST":"CET", "MESZ":"CET" ])[zonename] || zonename;

   if (origin->rules->abbr2zone[zonename])
      zonename=origin->rules->abbr2zone[zonename];

   Ruleset.Timezone zone=Timezone[zonename];
   if (!zone)
   {
      if (sscanf(zonename,"%[^-+]%s",string a,string b)==2 && a!="" && b!="")
      {
	 TimeRange tr=dwim_zone(origin,a,whut,@args);
	 if (!tr) return 0;

	 return 
	    dwim_tod(origin->set_timezone(
	       Timezone.make_new_timezone(
		  tr->timezone(),
		  Timezone.decode_timeskew(b))),
	       whut,@args);
      }

      array pz=TZnames.abbr2zones[zonename];
      if (!pz) return 0;
      foreach (pz,string zn)
      {
	 TimeRange try=dwim_zone(origin,zn,whut,@args);
	 if (try && try->tzname()==zonename) return try;
      }
      return 0;
   }
   else
      return dwim_tod(origin->set_timezone(zone),whut,@args);
}

static mapping(string:array) parse_format_cache=([]);

TimeRange parse(string fmt,string arg,void|TimeRange context)
{
   [string nfmt,array q]=(parse_format_cache[fmt]||({0,0}));

   if (!nfmt)
   {
//        nfmt=replace(fmt," %","%*[ \t]%"); // whitespace -> whitespace
#define ALNU "%[^- -,./:-?[-`{-¿]"
#define AMPM "%[ampAMP.]"
#define NUME "%[0-9]"
#define ZONE "%[-+0-9A-Za-z/]"
      nfmt=replace(fmt,
		   ({"%Y","%y","%M","%W","%D","%a","%e","%h","%m","%s","%p",
		     "%t","%f","%d","%z","%n"}),
		   ({ALNU,ALNU,ALNU,"%d",NUME,"%d",ALNU,"%d","%d","%d",AMPM,
		     NUME,NUME,NUME,ZONE,"%s"}));

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

   Calendar cal=this_object();


//  #define NOCATCH
#ifndef NOCATCH
   if (catch {
#else
      werror("%O\n",m);
#endif
      if (m->n && m->n!="") return 0;

      string x;
      if (m->Y) 
	 m->Y=default_rules->language[f_year_number_from_name](m->Y);

      if (!zero_type(m->Y) && m->D && (int)m->M)
	 low=m->day=cal->Day(m->Y,(int)m->M,(int)m->D);


      if (m->d)
      {
	 int y,mo,d;
      
	 if (sizeof(m->d)==6)
	 {
	    [y,mo,d]=(array(int))(m->d/2);
	    if (y<70) y+=2000; else y+=1900;
	 }
	 else if (sizeof(m->d)==8)
	    [y,mo,d]=(array(int))array_sscanf(m->d,"%4s%2s%2s");
	 else return 0;

	 low=m->day=cal->Day(y,mo,d);
      }
      else
      {
	 if (!zero_type(m->Y)) m->year=cal->Year(m->Y);
	 else if (m->y)
	 {
	    if (sizeof(m->y)<3) 
	    {
	       m->y=(int)m->y;
	       if (m->y<70) m->y+=2000;
	       else if (m->y<100) m->y+=1900;
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

	 if (!zero_type(m->D))
	    m->day=low=(m->month||(context?context->month():cal->Month()))
	       ->day((int)m->D);
	 else if (!zero_type(m->a))
	    m->day=low=m->year->day(m->a);
	 else if (!zero_type(m->e))
	    m->day=low=(m->week||(context?context->week():cal->Week()))
	       ->day(m->e);
	 else
	    low=m->day=context?context->day():cal->Day();

	 if (m->day && zero_type(m->Y) && zero_type(m->y) && m->e)
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
      string g=0;
      
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
	 if (!zero_type(m->h)) h=m->h,g="hour";
	 if (!zero_type(m->m)) mi=m->m,g="minute";
	 if (!zero_type(m->s)) s=m->s,g="second";
      }

      if (!zero_type(m->p))
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
	 return dwim_zone(low,m->z,g,h,mi,s);
      else if (g)
	 return dwim_tod(low,g,h,mi,s);
      else
	 return low;

#ifndef NOCATCH
   })
#endif
       return 0;
}

//! function Day dwim_day(string date)
//! function Day dwim_day(string date,TimeRange context)
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

static constant dwim_day_strings=
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

   if (sizeof(day)==4) catch { return parse("%M/%D",day/2*"/",context); };

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

TimeofDay dwim_time(string what,void|TimeRange cx)
{
   string a,h,m,s;
   TimeofDay t;

// #define COLON "$*[ :]" 
#define COLON ":" 
#define SPACED(X) replace(X," ","%*[ ]")

   sscanf(what,"%*[ \t]%s",what);

   if (t=parse(SPACED("%e %M %D %h:%m:%s %Y"),what,cx)) return t; // ctime
   if (t=parse(SPACED("%e %M %D %h:%m:%s %z %Y"),what,cx)) return t;

   foreach ( dwim_day_strings +
	     ({""}),
	     string dayformat )
      foreach ( ({ "%t %z",
		   "T%t %z",
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
 		   "%*[a-zA-Z.] %h%*[ ]%p", }),
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

//-- auxillary functions------------------------------------------------

//!
//! function datetime(int|void unix_time)
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
//! function datetime_name(int|void unix_time)
//! function datetime_short_name(int|void unix_time)
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

