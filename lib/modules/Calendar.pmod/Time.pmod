//!
//! module Calendar
//! submodule Time
//!
//! 	Base for time of day in calendars, ie 
//! 	calendars with hours, minutes, seconds
//! 	
//! 	This module can't be used by itself, but
//! 	is inherited by other modules (<ref>ISO</ref> by <ref>YMD</ref>, 
//! 	for instance).

#pike __REAL_VERSION__

//  #pragma strict_types

//- these classes majorly works on seconds
//- an hour is 3600 seconds, a minute is 60 seconds

inherit .TimeRanges:TimeRanges;

#include "constants.h"

// from inherited module

function(mixed...:TimeRange) Day;

// sanity check

static private int __sanity_check=lambda()
{
   if (5000000000!=5*inano)
      error("Calendar.Time needs bignums (Gmp.mpz)\n");
   return 1;
}();

.Rule.Timezone Timezone_UTC=.Rule.Timezone(0,"UTC"); // needed for dumping

string calendar_name() { return "Time"; }

//------------------------------------------------------------------------
//! class TimeOfDay
//------------------------------------------------------------------------

class TimeofDay
{
//! inherits TimeRange
   inherit TimeRange;

   constant is_timeofday=1;

   TimeRange base; // base day
   int ux;         // unix time (in UTC, no timezone or DST thing)
   int len;        // size in seconds (n*60)
   
// may be unknown (CALUNKNOWN)
   int ls;         // local second (on this day, adjusted for dst and tz)
   int utco=CALUNKNOWN; // offset to utc, counting dst
   string tzn;     // name of timezone
   
//! method void create()
//! method void create(int unixtime)
//!	In addition to the wide range of construction arguments
//!	for a normal TimeRange (see <ref>TimeRange.create</ref>),
//!	a time of day can also be constructed with unixtime
//!	as single argument consisting of the unix time
//!	- as returned from <tt>time(2)</tt> - of the time unit start.
//!
//!	It can also be constructed without argument, which
//!	then means "now", as in "this minute".

//- for internal use:
//- method void create("timeofday",rules,unixtime,len)

   void create(mixed ...args)
   {
      if (!sizeof(args))
      {
	 rules=default_rules;
	 create_now();
	 return;
      }
      switch (args[0])
      {
	 case "timeofday":
	    rules=[object(.Ruleset)]args[1];
	    ux=[int]args[2];
	    len=[int]args[3];
	    ls=CALUNKNOWN;
	    return;

	 case "timeofday_sd":
	    rules=[object(.Ruleset)]args[1];
	    ux=[int]args[2];
	    len=[int]args[3];
	    ls=[int]args[4];
	    utco=[int]args[5];
	    return;
      
	 default:
	    if (intp(args[0]) && sizeof(args)==1)
	    {
	       rules=default_rules;
	       create_unixtime_default([int]args[0]);
	       return;
	    }
      }
      rules=default_rules;
      if (create_backtry(@args)) return;
      ::create(@args);
   }

   static int(0..1) create_backtry(mixed ...args) 
   {
      if (sizeof(args)>1 && objectp(args[0])) 
      {
	 base=args[0];
	 rules=base->ruleset();
	 args=args[1..];
      }
      if (base)
      {
	 switch (sizeof(args))
	 {
	    case 3:
	       if (intp(args[0]) && intp(args[1]) && intp(args[2]))
	       {
		  create_unixtime_default(base->second(@args)->unix_time());
		  return 1;
	       }
	       break;
	    case 2:
	       if (intp(args[0]) && intp(args[1]))
	       {
		  create_unixtime_default(base->second(@args,0)->unix_time());
		  return 1;
	       }
	       break;
	 }
      }
      return 0;
   }

   void create_unixtime(int unixtime,int _len)
   {
      ux=unixtime;
      len=_len;
      ls=CALUNKNOWN;
   }

   static void create_now();

   void create_julian_day(int|float jd)
   {
// 1970-01-01 is julian day 2440588
      jd-=2440588;
      float fjd=(jd-(int)jd)-0.5;
      ls=CALUNKNOWN;
      create_unixtime_default(((int)jd)*86400+(int)(fjd*86400));
   }

// make base
// needed in ymd
/*static*/ TimeRange make_base()
   {
      base=Day("unix_r",ux,rules);
      if (len) base=base->range(Day("unix_r",ux+len,rules));
//        werror("make base -> %O\n",base);
      return base;
   }

// make local second 
   static void make_local()
   {
      if (!base) make_base();

      ls=ux-base->unix_time();
      if (rules->timezone->is_dst_timezone)
      {
	 if (utco==CALUNKNOWN) 
	    [utco,tzn]=rules->timezone->tz_ux(ux);
	 if (utco!=base->utc_offset())
	    ls+=base->utc_offset()-utco;
      }
   }

// default autopromote
   TimeRange autopromote()
   {
      return this_object();
   }

   array(int(-1..1)) _compare(TimeRange with)
   {
#define CMP(A,B) ( ((A)<(B))?-1:((A)>(B))?1:0 )

      if (!objectp(with))
	 error("_compare: illegal argument 1, expected TimeRange: %O\n",with);

      if (with->is_timeofday_f)
      {
	 array(int(-1..1)) cmp=with->_compare(this_object());

	 return ({-cmp[0],
		  -cmp[2],
		  -cmp[1],
		  -cmp[3]});
      }
      else if (with->is_timeofday)
	 return ({ CMP(ux,with->ux),CMP(ux,with->ux+with->len),
		   CMP(ux+len,with->ux),CMP(ux+len,with->ux+with->len) });
      else if (with->is_supertimerange ||
	       with->is_nulltimerange ||
	       !with->unix_time)
	 return ::_compare(with);
      else
      {
	 int e2=with->end()->unix_time();
	 int b2=with->unix_time();

	 return ({ CMP(ux,b2),CMP(ux,e2),CMP(ux+len,b2),CMP(ux+len,e2) });
      }
   }
   
   cSecond beginning()
   {
      return Second("timeofday_sd",rules,ux,0,ls,utco)->autopromote();
   }

   cSecond end()
   {
      return Second("timeofday",rules,ux+len,0)->autopromote();
   }

   TimeRange distance(TimeRange to)
   {
      if (to->is_ymd)
	 return distance(to->hour());

      if (!to->is_timeofday) 
	 error("distance: incompatible class %O\n",
	       object_program(to));

      if (to->is_timeofday_f) 
	 return 
	    Fraction("timeofday_f",rules,ux,0,len,0)
	    ->distance(to);

      int m;
      if ( (m=to->unix_time()-unix_time())<0) 
	 error("Negative distance %O .. %O\n", this_object(),to);

      return 
	 Second("timeofday_sd",rules,ux,m,ls,utco)
	 ->autopromote();
   }

   TimeRange range(TimeRange to)
   {
      if (to->is_timeofday_f) 
      {
	 return distance(to->end());
      }
      if (to->is_timeofday) 
      {
	 int m;
	 if ( (m=to->unix_time()+to->len-unix_time())<0 )
	    error("Negative range\n");
	 return Second("timeofday_sd",rules,ux,m,ls,utco)
	    ->autopromote();
      }
      return ::range(to);
   }

   static void convert_from(TimeRange other)
   {
      ::convert_from(other);
      if (other->is_timeofday)
	 len=other->len;
      else if (other->number_of_seconds)
	 len=other->number_of_seconds();
      else if (other->number_of_days)
	 len=86400*other->number_of_days(); // chance
      else
	 len=0; // *shrug*
   }

   TimeRange _set_size(int n,TimeRange t)
   {
      if (!t->is_timeofday)
	 return distance(add(n,t));

      return Second("timeofday_sd",rules,ux,n*(t->len),ls,utco)->autopromote();
   }

   TimeRange _move(int n,int m);

   TimeRange _add(float|int n,TimeRange step)
   {
      if (step->is_timeofday_f)
	 return Fraction("timeofday_f",rules,ux,0,len,0)
	    ->add(n,step);
      if (step->is_timeofday)
	 return _move(n,step->number_of_seconds());

      if (!base) make_base();
      return base->add(n,step)->place(this_object(),1);
   }

//! method Hour hour()
//! method Hour hour(int n)
//! method array(Hour) hours()
//! method array(Hour) hours(int first,int last)
//! method int number_of_hours()
//!	<ref>hour</ref>() gives back the timerange representing the
//!	first or <i>n</i>th Hour of the called object.
//!	Note that hours normally starts to count at zero,
//!	so <tt>->hour(2)</tt> gives the third hour within
//!	the range.
//!
//!	An Hour is in the Calendar perspective as any other
//!	time range not only 60 minutes, but also one 
//!	of the (normally) 24 hours of the day, precisely.
//!
//!	<ref>hours</ref>() give back an array of all the hours
//!	containing the time periods called. With arguments,
//!	it will give back a range of those hours, in the
//!	same enumeration as the <i>n</i> to <ref>hour</ref>().
//!
//!	<ref>number_of_hours</ref>() simple counts the
//!	number of hours containing the called time period.
//!
//! Note:
//!	The called object doesn't have to
//!	*fill* all the hours it will send back, it's 
//!	enough if it exist in those hours:
//!
//! 	<pre>
//!	> object h=Calendar.Time.Hour();
//!	Result: Hour(265567)
//!	> h->hours();                    
//!	Result: ({ /* 1 element */
//!		    Hour(265567)
//!		})
//!	> h+=Calendar.Time.Minute();
//!	Result: Minute(265567:01+60m)
//!	> h->hours();
//!	Result: ({ /* 2 elements */
//!		    Hour(265567),
//!		    Hour(265568)
//!		})
//!	</pre>



   cHour hour(void|int n)
   {
// hours are on the day, adjust for timezone (non-full-hour timezones!)
      if (ls==CALUNKNOWN) make_local();
      int zx=ux-ls%3600;

      if (!n || (n==-1 && !len)) 
	 return Hour("timeofday",rules,zx,3600);
      if (n<0) n=number_of_hours()+n;
      if (n<0 || n*3600>=len+ux-zx) 
	 error("hour not in timerange (hour 0..%d exist)\n",(len-1)/3600);
      return Hour("timeofday",rules,zx+3600*n,3600)->autopromote();
   }

   int number_of_hours()
   {
      if (ls==CALUNKNOWN) make_local();
      int zx=ux-ls%3600;
      return ((ux-zx)+len+3599)/3600;
   }

   array(cSecond) hours(int ...range)
   { return get_timeofday("hours",0,3600,Hour,@range); }

//! method Minute minute()
//! method Minute minute(int n)
//! method array(Minute) minutes()
//! method array(Minute) minutes(int first,int last)
//! method int number_of_minutes()
//!	<ref>minute</ref>() gives back the timerange representing the
//!	first or <i>n</i>th Minute of the called object.
//!	Note that minutes normally starts to count at zero,
//!	so <tt>->minute(2)</tt> gives the third minute within
//!	the range.
//!
//!	An Minute is in the Calendar perspective as any other
//!	time range not only 60 seconds, but also one 
//!	of the (normally) 60 minutes of the <ref>Hour</ref>, precisely.
//!
//!	<ref>minutes</ref>() give back an array of all the minutes
//!	containing the time periods called. With arguments,
//!	it will give back a range of those minutes, in the
//!	same enumeration as the <i>n</i> to <ref>minute</ref>().
//!
//!	<ref>number_of_minutes</ref>() simple counts the
//!	number of minutes containing the called time period.
//!

   cMinute minute(void|int n)
   {
// minutes are on the day, adjust for timezone (non-full-minute timezones!)
      if (ls==CALUNKNOWN) make_local();
      int zx=ux-ls%60;

      if (!n || (n==-1 && !len)) 
	 return Minute("timeofday",rules,zx,60);
      if (n<0) n=number_of_minutes()+n;
      if (n<0 || n*60>=len+ux-zx) 
	 error("minute not in timerange (minute 0..%d exist)\n",(len-1)/60);
      return Minute("timeofday",rules,zx+60*n,60)->autopromote();
   }

   int number_of_minutes()
   {
      if (ls==CALUNKNOWN) make_local();
      int zx=ux-ls%60;
      return ((ux-zx)+len+59)/60;
   }

   array(cSecond) minutes(int ...range)
   { return get_timeofday("minutes",0,60,Minute,@range); }

//! method Second second()
//! method Second second(int n)
//! method array(Second) seconds()
//! method array(Second) seconds(int first,int last)
//! method int number_of_seconds()
//!	<ref>second</ref>() gives back the timerange representing the
//!	first or <i>n</i>th Second of the called object.
//!	Note that seconds normally starts to count at zero,
//!	so <tt>->second(2)</tt> gives the third second within
//!	the range.
//!
//!	<ref>seconds</ref>() give back an array of all the seconds
//!	containing the time periods called. With arguments,
//!	it will give back a range of those seconds, in the
//!	same enumeration as the <i>n</i> to <ref>second</ref>().
//!
//!	<ref>number_of_seconds</ref>() simple counts the
//!	number of seconds containing the called time period.

   cSecond second(void|int n)
   {
      int len=number_of_seconds();
      if (!n || (n==-1 && !len)) 
	 return Second("timeofday",rules,ux,1);
      if (n<0) n=len+n;
      if (n<0 || n>=len)
	 error("second not in timerange (second 0..%d exist)\n",len);
      return Second("timeofday",rules,ux+n,1)->autopromote();
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

      if (ls==CALUNKNOWN) make_local();
      from+=ux-ls%step;

      return
	 map(enumerate(to/step,step,from),
	     lambda(int x)
	     { return p("timeofday",rules,x,step); });
   }

   array(cSecond) seconds(int ...range)
   { return get_timeofday("seconds",0,1,Second,@range); }

   int number_of_seconds()
   {
      return len;
   }

// --------------------

   float number_of_fractions() { return (float)number_of_seconds(); }
   cSecond fraction(float|int ... n) 
   {
      if (sizeof(n))
      {
	 float m=n[0];
	 if (m<=-1.0) m=1+number_of_fractions()+m;
	 array q=fractions(m,m);
	 if (!sizeof(q)) 
	    error("fraction not in range (0..%.1f)\n",number_of_fractions());
	 return q[0];
      }
      else
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
      if (to<from)
	 return ({});
      to-=from;
      return ({Fraction("timeofday_f",rules,
			ux+(int)from,(int)(inano*(from-(int)from)),
			(int)to,(int)(inano*(to-(int)to)))
	       ->autopromote()});
   }

// --------------------

   string nice_print();

   string nice_print_period()
   {
      if (!base) make_base();

      if (!len) 
	 return sprintf("%s %s sharp",
			base->nice_print(),nice_print());

      string a=base->nice_print();
      object e=end();
      string b=e->day()->nice_print();
      string c=e->nice_print();
      if (a==b) 
	 return a+" "+nice_print()+" - "+end()->nice_print();
      if (c==b)
	 return a+" "+nice_print()+" - "+b+" 0:00";
      return a+" "+nice_print()+" - "+b+" "+end()->nice_print();
   }

//! method TimeRange set_size_seconds(int seconds)
//! method TimeRange set_size_ns(int nanoseconds)
//!	These two methods allows the time range
//!	to be edited by size of specific units.


   TimeRange set_size_seconds(int seconds)
   {
      if (seconds<0) error("Negative size\n");
      return Second("timeofday_sd",rules,ux,seconds,ls,utco)
	 ->autopromote();
   }

   TimeRange set_size_ns(int nanoseconds)
   {
      if (nanoseconds<0) error("Negative size\n");
      return Fraction("timeofday_f",rules,ux,0,0,nanoseconds)
	 ->autopromote();
   }

//! method TimeRange move_seconds(int seconds)
//! method TimeRange move_ns(int nanoseconds)
//!	These two methods gives back the 
//!	time range called moved the specified
//!	amount of time, with the length intact.
//!
//!	The motion is relative to the original position
//!	in time; 10 seconds ahead of 10:42:32 is 10:42:42, etc.

   TimeRange move_seconds(int seconds)
   {
      return _move(seconds,1);
   }

   TimeRange move_ns(int nanoseconds)
   {
      return Fraction("timeofday_f",rules,ux,nanoseconds,len,0)
	 ->autopromote();
   }

// --------

//! method int unix_time()
//!	This calculates the corresponding unix time,
//!	- as returned from <tt>time(2)</tt> - from the
//!	time range. Note that the calculated unix time
//!	is the beginning of the period.

   int unix_time() { return ux; }

//! method float julian_day()
//!	This calculates the corresponding julian day,
//!	from the time range. Note that the calculated day
//!	is the beginning of the period, and is a float -
//!	julian day standard says .00 is midday, 12:00 pm.
//!
//! note:
//!	Normal pike (ie, 32 bit) floats (without --with-double-precision)
//!	has a limit of about 7 digits, and since we are about
//!	julian day 2500000, the precision on time of day is very limited.

   float julian_day() 
   { 
      return 2440588+ux/86400.0-0.5;
   }

//! method int hour_no()
//! method int minute_no()
//! method int second_no()
//! method float fraction_no()
//!	This gives back the number of the time unit,
//!	on this day. Fraction is a float number, 0&lt;=fraction&lt;1.

   int hour_no()
   {
      if (ls==CALUNKNOWN) make_local();
      return ls/3600;
   }

   int minute_no()
   {
      if (ls==CALUNKNOWN) make_local();
      return (ls/60)%60;
   }

   int second_no()
   {
      if (ls==CALUNKNOWN) make_local();
      return ls%60;
   }

   float fraction_no()
   {
      return 0.0;
   }

//! function mapping datetime()
//!     This gives back a mapping with the relevant
//!	time information (representing the start of the period);
//!	<pre>
//!	 ([ "year":     int        // year number (2000 AD=2000, 1 BC==0)
//!	    "month":    int(1..)   // month of year
//!	    "day":      int(1..)   // day of month
//!	    "yearday":  int(1..)   // day of year
//!	    "week":	int(1..)   // week of year
//!	    "week_day": int(1..)   // day of week (depending on calendar)
//!
//!	    "hour":     int(0..)   // hour of day, including dst
//!	    "minute":   int(0..59) // minute of hour
//!	    "second":   int(0..59) // second of minute 
//!	    "fraction": float      // fraction of second
//!	    "timezone": int        // offset to utc, including dst
//!
//!	    "unix":     int        // unix time
//!	    "julian":   float      // julian day
//!	 ]);
//!	</pre>

   mapping datetime()
   {
      if (ls==CALUNKNOWN) make_local();
      if (!base) make_base();
      return base->datetime(1) |
      (["hour":ls/3600,
	"minute":(ls/60)%60,
	"second":ls%60,
	"fraction":0.0,
	"timezone":utc_offset(),
	"julian":julian_day(),
	"unix":ux
      ]);
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
//! method string format_iso_short();
//! method string format_time_xshort();
//! method string format_mtime();
//! method string format_xtime();
//! method string format_tod();
//! method string format_xtod();
//! method string format_mod();
//! method string format_nice();
//! method string format_nicez();
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
//!	iso_time       "2000-06-02 (Jun) -W22-5 (Fri) 20:53:14 UTC+1" [2]
//!	ext_time       "Friday, 2 June 2000, 20:53:14" [2]
//!	ctime          "Fri Jun  4 20:53:14 2000\n" [2] [3]
//!	http           "Fri, 02 Jun 2000 19:53:14 GMT" [4]
//!     time           "2000-06-02 20:53:14" 
//!	time_short     "20000602 20:53:14"
//!	time_xshort    "000602 20:53:14"
//!	iso_short      "20000602T20:53:14"
//!     mtime          "2000-06-02 20:53" 
//!     xtime          "2000-06-02 20:53:14.000000" 
//!     todz           "20:53:14 CET"
//!     todz_iso       "20:53:14 UTC+1"
//!     tod            "20:53:14"
//!     tod_short      "205314"
//!     xtod           "20:53:14.000000"
//!     mod            "20:53"
//!     nice           "2 Jun 20:53", "2 Jun 2000 20:53:14" [2][5]
//!     nicez          "2 Jun 20:53 CET" [2][5]
//!     smtp           "Fri, 2 Jun 2000 20:53:14 +0100" [6]
//!	</pre>
//!	<tt>[1]</tt> note conflict (think 1 February 2003)
//!	<br><tt>[2]</tt> language dependent
//!	<br><tt>[3]</tt> as from the libc function ctime()
//!	<br><tt>[4]</tt> as specified by the HTTP standard;
//!		this is always in GMT, ie, UTC. The timezone calculations
//!		needed will be executed implicitly. It is not language
//!		dependent.
//!	<br><tt>[5]</tt> adaptive to type and with special cases
//!	 	for yesterday, tomorrow and other years
//!	<br><tt>[6]</tt> as seen in Date: headers in mails

   string format_ext_ymd();
   string format_iso_ymd();
   string format_ymd();
   string format_ymd_short();
   string format_ymd_xshort();
   string format_iso_week();
   string format_iso_week_short();
   string format_week();
   string format_week_short();
   string format_month();
   string format_month_short();

   string format_nice();

   string format_iso_time()
   {
      return this_object()->format_iso_ymd()+" "+format_todz_iso();
   }

   string format_ext_time()
   {
      return this_object()->format_ext_ymd()+" "+format_tod();
   }

   string format_time()
   {
      return this_object()->format_ymd()+" "+format_tod();
   }

   string format_time_short()
   {
      return this_object()->format_ymd_short()+" "+format_tod();
   }

   string format_iso_short()
   {
      return this_object()->format_ymd_short()+"T"+format_tod();
   }

   string format_time_xshort()
   {
      return this_object()->format_ymd_xshort()+" "+format_tod();
   }

   string format_mtime()
   {
      return this_object()->format_ymd()+" "+format_mod();
   }

   string format_xtime()
   {
      return this_object()->format_ymd()+" "+format_xtod();
   }

   string format_ctime()
   {
      if (!base) make_base();
      return replace(base->format_ctime(),
		     "00:00:00",format_tod());
   }

   string format_http()
   {
      if (utc_offset())
	 return set_timezone(Timezone_UTC)->format_http();
      if (!base) make_base();

      return replace(base->format_http(),
		     "00:00:00",format_tod());
   }

   string format_tod()
   {
      if (ls==CALUNKNOWN) make_local();
      return sprintf("%02d:%02d:%02d", ls/3600, (ls/60)%60, ls%60);
   }

   string format_tod_short()
   {
      if (ls==CALUNKNOWN) make_local();
      return sprintf("%02d%02d%02d", ls/3600, (ls/60)%60, ls%60);
   }

   string format_todz()
   {
      if (ls==CALUNKNOWN) make_local();
      return sprintf("%02d:%02d:%02d %s",
		     ls/3600,
		     (ls/60)%60,
		     ls%60,
		     tzname());
   }

   string format_todz_iso()
   {
      if (ls==CALUNKNOWN) make_local();
      return sprintf("%02d:%02d:%02d %s",
		     ls/3600,
		     (ls/60)%60,
		     ls%60,
		     tzname_iso());
   }

   string format_mod()
   {
      if (ls==CALUNKNOWN) make_local();
      return sprintf("%02d:%02d",ls/3600,(ls/60)%60);
   }

   string format_xtod()
   {
      if (ls==CALUNKNOWN) make_local();
      return sprintf("%02d:%02d:%02d.%06d",
		     ls/3600,
		     (ls/60)%60,
		     ls%60,
		     0);
   }

   string format_nice();
   string format_nicez()
   {
      return format_nice()+" "+tzname();
   }

   string format_smtp()
   {
      if (ls==CALUNKNOWN) make_local();
      int u=utc_offset();
      return sprintf("%s, %s %s %s %d:%02d:%02d %+03d%02d",
		     base->week_day_shortname(),
		     base->month_day_name(),
		     base->month_shortname(),
		     base->year_name(),
		     ls/3600, (ls/60)%60, ls%60,
		     -u/3600,max(u,-u)/60%60);
   }

   string format_elapsed()
   {
      string res="";
      object left=this_object();
      int x;
      if ( (x=(this_object()/Day)) )
      {
	 res+=sprintf("%dd",x);
	 left=this_object()->add(x,Day)->range(this_object()->end());
      }
      return sprintf("%s%d:%02d:%02d",
		     res,left->len/3600,
		     (left->len/60)%60,
		     left->len%60);
   }

// --------

   TimeofDay set_ruleset(.Ruleset r)
   {
      return 
	 Second("timeofday",r,ux,len)
	 ->autopromote();
   }

   int utc_offset()
   {
      if (utco==CALUNKNOWN)
	 return [utco,tzn]=rules->timezone->tz_ux(ux),utco;
      else 
	 return utco;
   }

   string tzname()
   {
      if (tzn) return tzn;
      if (rules->timezone->is_dst_timezone)
	 return [utco,tzn]=rules->timezone->tz_ux(ux),tzn;
      return tzn=rules->timezone->name;      
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


   
// -----------------------------------------------------------------

   TimeRange place(TimeRange what,void|int force)
   {
      if (!base) make_base();
      if (what->is_ymd)
	 return base->place(what,force);

      error("place: Incompatible type %O\n",what);
   }

// --------

//  #define TIME_OPERATOR_DEBUG
#ifdef TIME_OPERATOR_DEBUG
#define DEBUG_OVERLOAD_OPERATOR(OP,NAME,IND)				\
   TimeRange OP(mixed ...args)						\
   {									\
      _ind+=IND;							\
      TimeRange x=::OP(@args);						\
      _ind=_ind[..sizeof(_ind)-sizeof(IND)-1];				\
      werror(_ind+"  %O\n",this_object());				\
      foreach (args,TimeRange t) werror(_ind+NAME+" %O\n",t);		\
      werror(_ind+"= %O\n",x);						\
      return x;								\
   }
   DEBUG_OVERLOAD_OPERATOR(`&,"&","|  ");
   DEBUG_OVERLOAD_OPERATOR(`^,"^","|  ");
   DEBUG_OVERLOAD_OPERATOR(`|,"|","|  ");
   DEBUG_OVERLOAD_OPERATOR(subtract,"-","|  ");
#endif
}

   string _ind="* ";

//-----------------------------------------------------------------
//! class SuperTimeRange
//! inherits TimeRanges.SuperTimeRange
//-----------------------------------------------------------------

class cSuperTimeRange
{
   inherit TimeRanges::cSuperTimeRange;

#ifdef TIME_OPERATOR_DEBUG
   DEBUG_OVERLOAD_OPERATOR(`&,"&","S  ");
   DEBUG_OVERLOAD_OPERATOR(`^,"^","S  ");
   DEBUG_OVERLOAD_OPERATOR(`|,"|","S  ");
   DEBUG_OVERLOAD_OPERATOR(subtract,"-","S  ");
#endif

//! method Second second()
//! method Second second(int n)
//! method array(Second) seconds()
//! method array(Second) seconds(int first,int last)
//! method int number_of_seconds()
//! method Minute minute()
//! method Minute minute(int n)
//! method array(Minute) minutes()
//! method array(Minute) minutes(int first,int last)
//! method int number_of_minutes()
//! method Hour hour()
//! method Hour hour(int n)
//! method array(Hour) hours()
//! method array(Hour) hours(int first,int last)
//! method int number_of_hours()
//!	Similar to <ref>TimeofDay</ref>, the Time::SuperTimeRange
//!	has a number of methods for digging out time parts of the
//!	range. Since a <ref>SuperTimeRange</ref> is a bit more
//!	complex - the major reason for its existance it that it 
//!	contains holes, this calculation is a bit more advanced too.
//!
//!	If a range contains the seconds, say, 1..2 and 4..5, 
//!	the third second (number 2, since we start from 0) 
//!	in the range would be number 4, like this:
//!
//!	<pre>
//!	no   means this second
//!	0    1        
//!	1    2        
//!	2    4      &lt;- second three is missing,
//!	3    5         as we don't have it in the example range
//!	</pre>
//!
//!	<ref>number_of_seconds</ref>() will in this example 
//!	therefore also report 4, not 5, even if the time from
//!	start of the range to the end of the range is 5 seconds.
//!

   array(cHour) hours(int ...range) { return get_units("hours",@range); }
   cHour hour(void|int n) { return get_unit("hours",n); }
   int number_of_hours() { return num_units("hours"); }

   array(cMinute) minutes(int ...range) { return get_units("minutes",@range); }
   cMinute minute(void|int n) { return get_unit("minutes",n); }
   int number_of_minutes() { return num_units("minutes"); }

   array(cSecond) seconds(int ...range) { return get_units("seconds",@range); }
   cSecond second(void|int n) { return get_unit("seconds",n); }
   int number_of_seconds() { return num_units("seconds"); }

// wrapper methods to calculate units in a supertimerange

   static array(TimeRange) get_units(string unit,int ... range)
   {
      int from=0,to=0x7fffffff,pos=0;
      array res=({});
      TimeRange last=0;
      string ums=unit[..sizeof(unit)-2]; // no 's'

      if (sizeof(range)==2)
	 [from,to]=range;
      else if (sizeof(range)==1)
	 error("Illegal numbers of arguments to "+unit+"()\n");

      foreach (parts,TimeRange part)
      {
	 if (pos>=from)
	 {
	    if (pos>to) break;
	    array tmp=part[unit](from-pos,to-pos);
	    pos+=sizeof(tmp);
	    if (tmp[0]==last) tmp=tmp[1..];
	    res+=tmp;
	    if (sizeof(tmp)) last=tmp[-1];
	 }
	 else
	 {
	    int n=part["number_of_"+unit]();
	    TimeRange l=part[ums]();
	    if (l==last) n--;
	    if (n+pos>from)
	       res=part[unit](from-pos,to-pos);
	    pos+=n;
	    last=l;
	 }
      }
      return res;
   }

   static int num_units(string unit)
   {
      int pos=0;
      TimeRange last=0;
      string ums=unit[..sizeof(unit)-2]; // no 's'

      foreach (parts,TimeRange part)
      {
	 int n=part["number_of_"+unit]();
	 TimeRange l=part[ums]();
	 if (l==last) n--;
	 pos+=n;
	 last=l;
      }
      return pos;
   }

   static TimeRange get_unit(string unit,int n)
   {
      array(TimeRange) res=get_units(unit,n,n);
      if (sizeof(res)==1) return res[0];
      error("not in range ("+unit+" 0..%d exist)\n",
	    num_units(unit)-1);
   }

#define RBASE parts[0]

   string format_ext_ymd();
   string format_iso_ymd();
   string format_ymd();
   string format_ymd_short();
   string format_ymd_xshort();
   string format_iso_week();
   string format_iso_week_short();
   string format_week();
   string format_week_short();
   string format_month();
   string format_month_short();

   string format_iso_time() { return RBASE->format_iso_time(); }
   string format_ext_time() { return RBASE->format_ext_time(); }
   string format_time() { return RBASE->format_time(); }
   string format_time_short() { return RBASE->format_time_short(); }
   string format_time_xshort() { return RBASE->format_time_xshort(); }
   string format_mtime() { return RBASE->format_mtime(); }
   string format_xtime() { return RBASE->format_xtime(); }
   string format_ctime() { return RBASE->format_ctime(); }
   string format_http() { return RBASE->format_http(); }
   string format_tod() { return RBASE->format_tod(); }
   string format_todz() { return RBASE->format_todz(); }
   string format_todz_iso() { return RBASE->format_todz_iso(); }
   string format_mod() { return RBASE->format_mod(); }
   string format_xtod() { return RBASE->format_xtod(); }

   int hour_no() { return RBASE->hour_no(); }
   int minute_no() { return RBASE->minute_no(); }
   int second_no() { return RBASE->second_no(); }

#undef RBASE

// count parts elapsed
   string format_elapsed() 
   {
      TimeRange z=parts[0];
      foreach (parts,TimeRange y) z+=y;
      return parts[0]->distance(z)->format_elapsed();
   }

   string _sprintf(int t,mapping m)
   {
      if (t=='t') 
	 return "Calendar."+calendar_name()+".TimeofDay";
      return ::_sprintf(t,m);
   }
}

class cNullTimeRange
{
   inherit TimeRanges::cNullTimeRange;

   array(cHour) hours(int ...range) { return ({}); }
   cHour hour(void|int n) { error("no hours in nulltimerange\n"); }
   int number_of_hours() { return 0; }

   array(cMinute) minutes(int ...range) { return ({}); }
   cMinute minute(void|int n) { error("no minutes in nulltimerange\n"); }
   int number_of_minutes() { return 0; }

   array(cSecond) seconds(int ...range) { return ({}); }
   cSecond second(void|int n) { error("no seconds in nulltimerange\n"); }
   int number_of_seconds() { return 0; }

   string format_elapsed() { return "0:00:00"; }
}

//------------------------------------------------------------------------
//! class Hour
//------------------------------------------------------------------------

function(mixed...:cHour) Hour=cHour; // inheritance purposes
class cHour
{
   constant is_hour=1;

//! inherits TimeofDay

   inherit TimeofDay;

   void create_unixtime_default(int unixtime)
   {
      create_unixtime(unixtime,3600);
   }

   static void create_now()
   {
      create_unixtime(time(),3600);
   }

   static void create_unixtime(int _ux,int _len)
   {
      ::create_unixtime(_ux,_len);
      if (ls==CALUNKNOWN) make_local();
      if (ls%3600) ux-=ls%3600,ls=CALUNKNOWN;
   }

   static void create_julian_day(int|float jd)
   {
      ::create_julian_day(jd);
      len=3600;
   }

   static void convert_from(TimeRange other)
   {
      ::convert_from(other);
      len-=len%3600;
   }

   TimeofDay _move(int n,int m)
   {
      if (m==0 || n==0) return this_object();
      if (m%3600) 
	 return Second("timeofday",rules,ux,len)->_move(n,m);
      return Hour("timeofday",rules,ux+n*m,len)->autopromote(); 
   }


   string _sprintf(int t,mapping m)
   {
      if (catch {
      switch (t)
      {
	 case 'O':
	    if (!base) make_base();
	    if (len!=3600)
	       return sprintf("Hour(%s)",
			      nice_print_period());
	    return sprintf("Hour(%s %s)",
			   base->nice_print(),
			   nice_print());
	 case 't':
	    return "Calendar."+calendar_name()+".Hour";
	 default:
	    return ::_sprintf(t,m);
      }
      })
	 return "error";
   }

   string nice_print()
   {
      if (ls==CALUNKNOWN) make_local();
      return sprintf("%d:00 %s",ls/3600,tzname());
   }

   string format_nice()
   {
      if (ls==CALUNKNOWN) make_local();
      if (!base) make_base();
      return base->format_nice()+" "+sprintf("%d:00",ls/3600);
   }

// -----------------------------------------------------------------

   TimeRange place(TimeRange what,void|int force)
   {
      if (what->is_hour)
	 return Hour("timeofday",rules,ux,what->len);
      if (what->is_minute)
	 return minute()+(what->hour()->distance(what))->number_of_minutes();
      if (what->is_fraction)
      {
	 TimeRange t=what->hour()->distance(what);
	 int s=t->len_s;
	 int ns=t->len_ns;
	 return 
	    Fraction("timeofday_f",rules,ux,0,what->len_s,what->len_s)
	    ->_move(1,s,ns);
      }
      if (what->is_second)
	 return second()+(what->hour()->distance(what))->number_of_seconds();

      ::place(what,force);
   }
}

//------------------------------------------------------------------------
//! class Minute
//------------------------------------------------------------------------

function(mixed...:cMinute) Minute=cMinute; // inheritance purposes
class cMinute
{
   constant is_minute=1;

//! inherits TimeofDay

   inherit TimeofDay;

   static void create_unixtime(int _ux,int _len)
   {
      ::create_unixtime(_ux,_len);
      if (ls==CALUNKNOWN) make_local();
      ux-=ls%60;
   }

   void create_unixtime_default(int unixtime)
   {
      create_unixtime(unixtime,60);
   }

   static void create_now()
   {
      create_unixtime(time()/60*60,60);
   }

   TimeofDay autopromote()
   {
      if (ls==CALUNKNOWN) make_local();
      if (!(ls%3600) && !(len%3600)) 
	 return Hour("timeofday_sd",rules,ux,len,ls,utco)->autopromote();
      return ::autopromote();
   }

   static void convert_from(TimeRange other)
   {
      ::convert_from(other);
      len-=len%60;
   }

   TimeofDay _move(int n,int m)
   {
      if (m==0 || n==0) return this_object();
      if (m%60) return Second("timeofday",rules,ux,len)->_move(n,m);
      return Minute("timeofday",rules,ux+n*m,len)->autopromote(); 
   }

   string _sprintf(int t,mapping m)
   {
      switch (t)
      {
	 case 'O':
	    if (!base) make_base();
	    if (len!=60)
	       return sprintf("Minute(%s)",
			      nice_print_period());
	    return sprintf("Minute(%s %s)",
			   base->nice_print(),
			   nice_print());
	 case 't':
	    return "Calendar."+calendar_name()+".Minute";
	 default:
	    return ::_sprintf(t,m);
      }
   }

   string nice_print()
   {
      if (ls==CALUNKNOWN) make_local();
      return sprintf("%d:%02d %s",ls/3600,(ls/60)%60,tzname());
   }

   string format_nice()
   {
      if (ls==CALUNKNOWN) make_local();
      if (!base) make_base();
      return base->format_nice()+" "+sprintf("%d:%02d",ls/3600,(ls/60)%60);
   }

   TimeRange place(TimeRange what,void|int force)
   {
      if (what->is_hour)
	 return hour()->place(what);
      if (what->is_minute)
	 return Minute("timeofday",rules,ux,what->len);
      if (what->is_fraction)
      {
	 TimeRange t=what->minute()->distance(what);
	 int s=t->len_s;
	 int ns=t->len_ns;
	 return 
	    Fraction("timeofday_f",rules,ux,0,what->len_s,what->len_s)
	    ->_move(1,s,ns);
      }
      if (what->is_second)
	 return second()+(what->minute()->distance(what))->number_of_seconds();

      ::place(what,force);
   }
}

//------------------------------------------------------------------------
//! class Second
//------------------------------------------------------------------------

function(mixed...:cSecond) Second=cSecond; // inheritance purposes
class cSecond
{
   constant is_second=1;

//! inherits TimeofDay

   inherit TimeofDay;

   void create_unixtime_default(int unixtime)
   {
      create_unixtime(unixtime,1);
   }

   static void create_now()
   {
      create_unixtime(time(),1);
   }

   static void create_julian_day(int|float jd)
   {
      ::create_julian_day(jd);
      len=1;
   }

   TimeofDay autopromote()
   {
      if (ls==CALUNKNOWN) make_local();
      if (!(ls%60) && !(len%60)) 
	 return Minute("timeofday",rules,ux,len)->autopromote();
      return ::autopromote();
   }

   TimeofDay _move(int n,int m)
   {
      if (m==0 || n==0) return this_object();
      return Second("timeofday",rules,ux+n*m,len)->autopromote(); 
   }

   string _sprintf(int t,mapping m)
   {
      switch (t)
      {
	 case 'O':
	    if (len!=1)
	       return sprintf("Second(%s)",
			      nice_print_period());
	    if (!base) make_base();
	    return sprintf("Second(%s %s)",
			   base->nice_print(),
			   nice_print());
	 case 't':
	    return "Calendar."+calendar_name()+".Second";
	 default:
	    return ::_sprintf(t,m);
      }
   }

   string nice_print()
   {
      if (ls==CALUNKNOWN) make_local();
      return sprintf("%d:%02d:%02d %s",ls/3600,ls/60%60,ls%60,tzname()||"?");
   }

   string format_nice()
   {
      if (ls==CALUNKNOWN) make_local();
      if (!base) make_base();
      return base->format_nice()+" "+
	 sprintf("%d:%02d:%02d",ls/3600,ls/60%60,ls%60);
   }

// backwards compatible with calendar I
   string iso_name()
   { return this_object()->format_ymd()+" T"+format_tod(); }
   string iso_short_name() 
   { return this_object()->format_ymd_short()+" T"+(format_tod()-":"); }


   TimeRange place(TimeRange what,void|int force)
   {
      if (what->is_hour)
	 return hour()->place(what);
      if (what->is_minute)
	 return minute()->place(what);
      if (what->is_fraction)
      {
	 TimeRange t=what->second()->distance(what);
	 int s=t->len_s;
	 int ns=t->len_ns;
	 return 
	    Fraction("timeofday_f",rules,ux,0,what->len_s,what->len_s)
	    ->_move(1,s,ns);
      }
      if (what->is_second)
	 return second()+(what->second()->distance(what))->number_of_seconds();

      ::place(what,force);
   }
}

//------------------------------------------------------------------------
//! class Fraction
//!	A Fraction is a part of a second, and/or a time period
//!	with higher resolution then a second. 
//!
//!	It contains everything that is possible to do with a 
//!	<ref>Second</ref>, and also some methods of grabbing
//!	the time period with higher resolution.
//!
//! note:
//!	Internally, the fraction time period is measured in
//!	nanoseconds. A shorter or more precise time period then 
//!	in nanoseconds is not possible within the current Fraction class.
//!
//! inherits TimeofDay
//------------------------------------------------------------------------

function(mixed...:cFraction) Fraction=cFraction;
class cFraction
{
   inherit cSecond;

   constant is_timeofday_f=1;
   constant is_fraction=1;

   int ns;     // nanoseconds add to s
   int len_ns; // nanoseconds length
   int len_s;  // seconds length

//! method void create()
//! method void create("unixtime",int|float unixtime)
//! method void create("unixtime",int|float unixtime,int|float len)
//!	It is possible to create a Fraction in two ways,
//!	either "now" with no arguments or
//!	from a unix time (as from <tt>time(2)</tt>).
//!
//!	If created from unix time, both the start of the period
//!	and the size of the period can be given in floats,
//!	both representing seconds. Note that the default
//!	float precision in pike is rather low (same as 'float' in C,
//!	the 32 bit floating point precision, normally about 7 digits),
//!	so beware that the resolution might bite you. (Internally
//!	in a Fraction, the representation is an integer.)
//!
//!	If created without explicit length, the fraction will always be
//!	of zero length.

   void create(mixed ...args)
   {
      if (!sizeof(args))
      {
	 rules=default_rules;
	 create_now();
	 return;
      }
      else if (args[0]=="timeofday_f")
      {
	 rules=[object(.Ruleset)]args[1];
	 ux=[int]args[2];
	 ns=[int]args[3];
	 len_s=[int]args[4];
	 len_ns=[int]args[5];
	 ls=CALUNKNOWN;

	 if (ns<0)
	    error("Can't create negative ns: %O\n",this_object());

	 if (!rules) error("no rules\n");

	 if (len_ns>=inano) len_s+=(len_ns/inano),len_ns%=inano;
	 if (ns>=inano) ux+=(ns/inano),ns%=inano;
	 len=len_s+(ns+len_ns+inano-1)/inano;

	 if (ns<0)
	    error("Can't create negative ns: %O\n",this_object());

	 return;
      }
      else if (intp(args[0]) && sizeof(args)==1)
      {
	 rules=default_rules;
	 create_unixtime(args[0]);
	 return;
      }
      else switch (args[0])
      {
	 case "unix":
	    rules=default_rules;
	    create_unixtime(@args[1..]);
	    return;
	 case "julian":
	    rules=default_rules;
	    create_julian_day(args[1]);
	    return;
	 case "unix_r":
	    rules=args[-1];
	    create_unixtime(@args[..sizeof(args)-2]);
	    return;
	 case "julian_r":
	    rules=args[2];
	    create_julian_day(args[1]);
	    return;
      }
      rules=default_rules;
      if (create_backtry(@args)) return;
      error("Fraction: illegal argument(s) %O,%O,%O,...\n",
	    @args,0,0,0);
   }

   int create_backtry(mixed ...args) 
   {
      if (sizeof(args)>1 && objectp(args[0]) && args[0]->is_day) 
      {
	 base=args[0];
	 rules=base->ruleset();
	 args=args[1..];
      }
      ls=CALUNKNOWN;
      if (base)
      {
	 switch (sizeof(args))
	 {
	    case 4:
	       if (intp(args[0]) && intp(args[1]) && intp(args[2]) &&
		   intp(args[3]))
	       {
		  ux=base->second(@args[..2])->unix_time();
		  ns=args[3];
		  return 1;
	       }
	       break;
	    case 3:
	       if (intp(args[0]) && intp(args[1]) && floatp(args[2]))
	       {
		  ux=base->second(args[0],args[1],(int)args[2])->unix_time();
		  ns=(int)(inano*(args[3]-(int)args[3]));
		  return 1;
	       }
	       break;
	 }
      }
      return ::create_backtry(@args);
   }

   static void create_now()
   {
      ux=time();
      ns=(int)(inano*time(ux));
      if (ns>=inano) ux+=(ns/inano),ns%=inano;
      len=len_s=len_ns=0;
      ls=CALUNKNOWN;
   }

   int unix_time() { return ux; }
   float f_unix_time() { return ux+ns*(1.0/inano); }

   static void create_unixtime(int|float unixtime,
			       void|int|float _len)
   {
      ux=(int)unixtime;
      ns=(int)(inano*(unixtime-(int)unixtime));
      len_s=(int)_len;
      len_ns=(int)(inano*(_len-(int)_len));
      ls=CALUNKNOWN;
   }

   static void create_unixtime_default(int|float unixtime)
   {
      create_unixtime(unixtime);
   }

   float julian_day()
   {
      return 2440588+(ux+ns*(1.0/inano))*(1/86400.0)+0.5;
   }

   static void create_julian_day(int|float jd)
   {
// 1970-01-01 is julian day 2440588
      jd-=2440588;
      float fjd=((jd-(int)jd)+0.5)*86400;
      ux=((int)jd)*86400+(int)(fjd);
      ns=(int)(inano*(fjd-(int)fjd));
      
      ls=CALUNKNOWN;
   }

   string _sprintf(int t,mapping m)
   {
      switch (t)
      {
	 case 'O':
	    if (!base) make_base();
	    if (len_ns || len_s)
	       return sprintf("Fraction(%s)",
			      nice_print_period());
	    return sprintf("Fraction(%s %s)",
			   base->nice_print(),
			   nice_print());
	 case 't':
	    return "Calendar."+calendar_name()+".Fraction";
	 default:
	    return ::_sprintf(t,m);
      }
   }

   int __hash() { return ux; }

   string nice_print()
   {
      if (ls==CALUNKNOWN) make_local();
      return sprintf("%d:%02d:%02d.%06d %s",
		     ls/3600,ls/60%60,ls%60,ns/1000,tzname());
   }

   string format_nice()
   {
      if (ls==CALUNKNOWN) make_local();
      if (!base) make_base();
      return base->format_nice()+" "+
	 sprintf("%d:%02d:%02d.%06d",
		 ls/3600,ls/60%60,ls%60,ns/1000);
   }

   TimeofDay autopromote()
   {
      if (!ns && !len_ns)
	 return Second("timeofday",rules,ux,len_s)->autopromote();

      return this_object();
   }

   TimeofDay _move(int n,int z_s,void|int z_ns)
   {
      if ((z_s==0 && z_ns==0) || n==0) return autopromote();
      int nns=ns+n*z_ns;
      int nux=ux+n*z_s;
      if (nns<0 || nns>=inano) nux+=nns/inano,nns%=inano;
      return Fraction("timeofday_f",rules,nux,nns,len_s,len_ns)
	 ->autopromote();
   }

   TimeRange _add(int n,TimeRange step)
   {
      if (step->is_timeofday_f)
	 return _move(n,step->len_s,step->len_ns);
      return ::_add(n,step);
   }

   static void convert_from(TimeRange other)
   {
      ::convert_from(other);
      if (other->is_timeofday_f)
      {
	 ns=other->ns;
	 len_s=other->len_s;
	 len_ns=other->len_ns;
      }
   }

   static TimeRange _set_size(int n,TimeRange t)
   {
      if (t->is_timeofday_f)
	 return Fraction("timeofday_f",rules,ux,ns,
			 n*t->len_s,n*t->len_ns)
	    ->autopromote();

      if (t->is_timeofday)
	 return Fraction("timeofday_f",rules,ux,ns,n*t->len,0)
	    ->autopromote();

      return distance(add(n,t));
   }

   TimeRange distance(TimeRange to)
   {
   // s is 0 in an object that doesn't have s

      int s1=ux;
      int s2;

      if (to->is_timeofday)
	 s2=to->ux;
      else 
	 s2=to->unix_time();

// ns is 0 in an object that doesn't have ns

      if (s2<s1 ||
	  (s2==s1 && to->ns<ns))
	 error("Negative distance %O .. %O\n", this_object(),to);

      return 
	 Fraction("timeofday_f",rules,ux,ns,s2-s1-1,to->ns-ns+inano)
	 ->autopromote();
   }

   TimeRange range(TimeRange to)
   {
      return distance(to->end());
   }

   array(int(-1..1)) _compare(TimeRange with)
   {
#define CMP2(AS,ANS,BS,BNS) 						\
      (((AS)<(BS))?-1:							\
       ((AS)>(BS))?1:							\
       ((ANS)<(BNS))?-1:						\
       ((ANS)>(BNS))?1:0)
       
      if (with->is_timeofday_f)
	 return ({ CMP2(ux,ns,with->ux,with->ns),
		   CMP2(ux,ns,with->ux+with->len_s,with->ns+with->len_ns),
		   CMP2(ux+len_s,ns+len_ns,with->ux,with->ns),
		   CMP2(ux+len_s,ns+len_ns,with->ux+with->len_s,
			with->ns+with->len_ns) });
      else if (with->is_timeofday)
	 return ({ CMP2(ux,ns,with->ux,0),
		   CMP2(ux,ns,with->ux+with->len,0),
		   CMP2(ux+len_s,ns+len_ns,with->ux,0),
		   CMP2(ux+len_s,ns+len_ns,with->ux+with->len, 0) });
      return ::_compare(with);
   }

   TimeofDay beginning()
   {
      if (len_s==0.0 && len_ns==0.0) return this_object();
      return Fraction("timeofday_f",rules,ux,ns,0,0)
	 ->autopromote();
   }

   TimeofDay end()
   {
      if (len_s==0 && len_ns==0) return this_object();
      object q=Fraction("timeofday_f",rules,ux+len_s,ns+len_ns,0,0)
	 ->autopromote();
      return q;
   }
   
   TimeRange set_size_seconds(int seconds)
   {
      if (seconds<0) error("Negative size\n");
      return Fraction("timeofday_f",rules,ux,ns,seconds,0)
	 ->autopromote();
   }

   TimeRange set_size_ns(int nanoseconds)
   {
      if (nanoseconds<0) error("Negative size\n");
      return Fraction("timeofday_f",rules,ux,ns,0,nanoseconds)
	 ->autopromote();
   }


   TimeRange move_seconds(int seconds)
   {
      return _move(1,seconds);
   }

   TimeRange move_ns(int nanoseconds)
   {
      return _move(1,0,nanoseconds);
   }

   int number_of_seconds()
   {
      if (len_ns || ns) return len_s+1;
      else return len_s||1;
   }

   float number_of_fractions() 
   { 
      return len_s+1e-9*len_ns;
   }

   array(cFraction) fractions(int|float ...range)
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
      if (to<from)
	 return ({});

      if (from==0.0 && to==n) return ({this_object()});

      to-=from;
      return ({Fraction("timeofday_f",rules,
			ux+(int)from,ns+(int)(inano*(from-(int)from)),
			(int)to,(int)(inano*(to-(int)to)))});
   }

   mapping datetime()
   {
      return ::datetime()|(["fraction":ns*(1.0/inano)]);
   }

   string format_xtod()
   {
      if (ls==CALUNKNOWN) make_local();
      return sprintf("%02d:%02d:%02d.%06d",
		     ls/3600,
		     (ls/60)%60,
		     ls%60,
		     ns/1000);
   }

   string format_elapsed()
   {
      int x;
      if ( (x=(this_object()/Day)) )
      {
	 object left=this_object()->add(x,Day)->range(this_object()->end());
	 return sprintf("%dd%d:%02d:%02d.%03d",
			x,left->len_s/3600,
			(left->len_s/60)%60,
			left->len_s%60,
			left->len_ns/1000000);
      }
      if (len_s>=3600)
	 return sprintf("%d:%02d:%02d.%03d",
			len_s/3600,(len_s/60)%60,
			len_s%60,len_ns/1000000);
      if (len_s>=60)
	 return sprintf("%d:%02d.%03d",
			len_s/60,len_s%60,len_ns/1000000);
      if (len_s || len_ns>inano/1000)
	 return sprintf("%d.%06d",
			len_s,len_ns/1000);
      return sprintf("0.%09d",len_ns);
   }
}

//------------------------------------------------------------------------
//  global convinience functions
//------------------------------------------------------------------------

//! method TimeofDay now()
//!	Give the zero-length time period of the
//!	current time.

TimeofDay now()
{
   return Fraction();
}

//! method Calendar set_timezone(Timezone tz)
//! method Calendar set_timezone(string|Timezone tz)
//! method TimeZone timezone()
//!	Set or get the current timezone (including dst) rule.
//!	<ref>set_timezone</ref> returns a new calendar object,
//!	as the called calendar but with another set of rules.
//!
//!	Example:
//!	<pre>
//!	&gt; Calendar.now();                                      
//!	Result: Fraction(Fri 2 Jun 2000 18:03:22.010300 CET)
//!	&gt; Calendar.set_timezone(Calendar.Timezone.UTC)->now();
//!	Result: Fraction(Fri 2 Jun 2000 16:03:02.323912 UTC)
//!	</pre>

this_program set_timezone(string|.Rule.Timezone tz)
{
   return set_ruleset(default_rules->set_timezone(tz));
}

.Rule.Timezone timezone()
{
   return default_rules->timezone;
}

this_program set_language(string|.Rule.Language lang)
{
   return set_ruleset(default_rules->set_language(lang));
}

.Rule.Language language()
{
   return default_rules->language;
}

//! method Calendar set_ruleset(Ruleset r)
//! method Ruleset ruleset()
//!	Set or read the ruleset for the calendar.
//!	<ref>set_ruleset</ref> returns a new calendar object,
//!	but with the new ruleset.

this_program set_ruleset(.Ruleset r)
{
   this_program c=this_program();
   c->default_rules=r;
   return c;
}

.Ruleset ruleset()
{
   return default_rules;
}
