
//! module Calendar
//!    
//!   This module implements calendar calculations, and base classes
//!   for time units. 
//!
//!   example program:
//!   <pre>
//!	void write_month(object m)
//!	{
//!	   object w;
//!	   object today;
//!	
//!	   today=function_object(object_program(m))->Day();
//!	
//!	   write(sprintf("    %|28s\n",
//!			 Simulate.capitalize(m->name()+" ")
//!                      +m->year()->name()));
//!	   
//!	   w=m->day(1)->week();
//!	
//!	   write("    ");
//!	   foreach (Array.map(w->days(),w->day)->week_day_name(),string n)
//!	      write(sprintf("%3s ",n[0..2]));
//!	   write("\n");
//!	
//!	   do
//!	   {
//!	      array a;
//!	      object d;
//!	      a=Array.map(Array.map(w->days(),w->day),
//!			  lambda(object d,object m) 
//!			  { if (d->month()!=m) return 0; else return d; },m);
//!	
//!	      write(sprintf("%3s ",w->name()));
//!	      foreach (a,d)
//!		 if (d)
//!		    if (d!=today) write(sprintf(" %2d ",d->month_day()));
//!		    else write(sprintf(">%2d<",d->month_day()));
//!		 else write("    ");
//!	
//!	      write("\n");
//!	      w++;
//!	   }
//!	   while (w->day(0)->month()==m);
//!	}
//!    </pre>
//!	call with, for example, 
//!	<tt>write_month(Calendar.Swedish.Month());</tt>.
//!
//! class time_unit
//!
//! method array(string) lesser()
//!	Gives a list of methods to get lesser (shorter) time units.
//!	ie, for a month, this gives back <tt>({"day"})</tt>
//!	and the method <tt>day(mixed n)</tt> gives back that 
//!	day object. The method <tt>days()</tt> gives back a
//!	list of possible argument values to the method <tt>day</tt>.
//!	Concurrently, <tt>Array.map(o->days(),o->day)</tt> gives
//!	a list of day objects in the object <tt>o</tt>.
//!
//!	
//!	Ie:<pre>
//!	array(string) lesser()    - gives back a list of possible xxx's.
//!	object xxxs()     	  - gives back a list of possible n's.
//!	object xxx(mixed n)       - gives back xxx n
//!	object xxx(object(Xxx) o) - gives back the corresponing xxx 
//!	</pre>
//!
//!	The list of n's (as returned from xxxs) are always in order.
//!
//!	There are two n's with special meaning, 0 and -1.
//!	0 always gives the first xxx, equal to 
//!	my_obj->xxx(my_obj->xxxs()[0]), and -1 gives the last,
//!	equal to my_obj->xxx(my_obj->xxxs()[-1]).
//!
//!	To get all xxxs in the object, do something like
//!	<tt>Array.map(my_obj->xxxs(),my_obj->xxx)</tt>.
//!
//!	xxx(object) may return zero, if there was no correspondning xxx.
//!
//! method array(string) greater()
//!	Gives a list of methods to get greater (longer) time units 
//!	from this object. For a month, this gives back <tt>({"year"})</tt>,
//!	thus the method <tt>month->year()</tt> gives the year object.
//!
//! method object next()
//! method object prev()
//! method object `+(int n)
//! method object `-(int n)
//! method object `-(object x)
//!	next and prev gives the logical next and previous object.
//!	The <tt>+</tt> operator gives that logical relative object,
//!	ie <tt>my_day+14</tt> gives 14 days ahead.
//!     <tt>-</tt> works the same way, but can also take an object
//!	of the same type and give the difference as an integer.

#define error(X) throw(({(X),backtrace()}))

class _TimeUnit
{
   object this=this_object();

   array(string) greater() { return ({}); }
   array(string) lesser() { return ({}); }

   int `<(object x) { error("'< undefined\n"); }
   int `>(object x) { error("'> undefined\n"); }
   int `==(object x) { error("'== undefined\n"); }

   object `+(int n) { error("`+ undefined\n"); }
   object|int `-(int n) { error("`- undefined\n"); }

   object next() { return this+1; }
   object prev() { return this+(-1); }
}




//!
//! function datetime(int|void unix_time)
//!          Replacement for localtime.
//!
//! function datetime_name(int|void unix_time)
//!          Replacement for ctime.
//!
//! function datetime_short_name(int|void unix_time)
//!          Replacement for ctime.
//!

//
// Maybe these auxiliary routines should be somewhere else? Maybe even
// written in C?
//
//
static private int ymd2jd(int y, int m, int d)
{
  if(m<3) {
    y--;
    m+=12;
  }
  return 1720995+d+2- (y/100) + (y/400) + (36525*y)/100 + (306001*(m+1))/10000;
}

static private int y2jd(int y)
{
  y--;
  return 1721426 - (y/100) + (y/400) + (36525*y)/100;
}

static private int leapyear(int year)
{
  if((year%400) == 0)
    return 1;
  if((year%100) == 0)
    return 0;
  if((year%4) == 0)
    return 1;
  return 0;
}

static private int week(int year, int month, int day)
{
  int delta_day, first_time, yday, first_day, first_next_day;

  first_time = y2jd(year);
  yday = ymd2jd(year, month, day)-first_time;
  first_day = first_time%7;
  first_next_day = y2jd(year+1)%7;

  if((yday > (364+leapyear(year)-first_next_day)) &&
     (first_next_day <= 3))
    return 1;
  
  if(first_day <= 3) {
    /* Week begins on Thursday or erlier, means week one here. */
    delta_day = first_day;
  } else {
    /* Week begins on Friday or later, means same week as last year. */
    if(yday + first_day <= 6) {
      /* Return the last week previous year. */
      return week(year-1, 12, 28);
    }
    delta_day = first_day-7;
  }
  
  /* Calculate week. Begin with week 1. */
  return (yday + delta_day)/7 + 1;
}

// Sane replacement for localtime().
mapping(string:int) datetime(int|void unix_time)
{
   mapping t = localtime(unix_time || time());
   return ([ "year":t->year+1900,
	     "month":t->mon+1,
	     "day":t->mday,
	     "hour":t->hour,
	     "minute":t->min,
	     "second":t->sec,
	     "week":week(t->year+1900, t->mon+1, t->mday),
	     // ISO weekdays begin with Monday as 1 --- Sunday as 7.
	     "weekday":((t->wday+6)%7)+1,
	     "yearday":t->yday,
	     "timezone":t->timezone,
	     // Dayligt-saving time.
	     "DST":t->isdst ]);
}

// Sane replacement for ctime().
string datetime_name(int|void unix_time)
{
  array(string) day_names = ({ "Mon", "Tue", "Wed", "Thu",
			       "Fri", "Sat", "Sun" });
  array(string) month_names = ({ "Jan", "Feb", "Mar", "Apr",
				 "May", "Jun", "Jul", "Aug",
				 "Sep", "Oct", "Nov", "Dec" });
  
  mapping t = datetime(unix_time);
  return sprintf("%04d-%02d-%02d (%s) -W%02d-%d (%s) %02d:%02d:%02d",
		 t->year, t->month, t->day, month_names[t->month-1],
		 t->week, t->weekday, day_names[t->weekday-1],
		 t->hour, t->minute, t->second);
}

// Sane replacement for ctime().
string datetime_short_name(int|void unix_time)
{
  mapping t = datetime(unix_time);
  return sprintf("%04d-%02d-%02d %02d:%02d:%02d",
		 t->year, t->month, t->day,
		 t->hour, t->minute, t->second);
}
