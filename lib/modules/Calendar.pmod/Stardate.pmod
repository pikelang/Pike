//! module Calendar
//! submodule Stardate
//!	This implements TNG stardates. 

#pike __REAL_VERSION__

inherit .TimeRanges;

static constant TNGSTARPERJULIAN=1000.0/365.2425;
static constant TNGSTARPERSECOND=TNGSTARPERJULIAN/86400;
static constant TNG0JULIAN=2569518.5;
static constant TNG0UNIX=11139552000;

string calendar_name() { return "Stardate"; }

function(mixed...:cTick) Tick=cTick;
class cTick
{
   inherit TimeRange;

   constant is_stardate=1;

   float t;
   float len;

//! method void create(mixed ...)
//! method void create(int|float date)
//! method void create()
//!	Apart from the standard creation methods
//!	(julian day, etc), you can create a stardate
//!	from the stardate number. The length
//!	of the period will then be zero.
//!
//!	You can also omit any arguments to create now.
//!
//! known bugs:
//!	Since the precision is limited to the float type
//!	of pike you can get non-precise results:
//!
//!     <pre>
//!	> Calendar.Second(Calendar.Stardate.Day(Calendar.Year()));
//!	Result: Second(Fri 31 Dec 1999 23:59:18 CET - Sun 31 Dec 2000 23:59:18 CET)
//!	</pre>


   void create(mixed ...args)
   {
      switch (sizeof(args))
      {
	 case 4:
      // internal
	    if (args[0]=="stardate")
	    {
	       rules=args[1];
	       t=args[2];
	       len=args[3];
	       return;
	    }
	    break;
	 case 1:
	    if (intp(args[0]) || floatp(args[0]))
	    {
	       rules=default_rules;
	       t=(float)args[0];
	       len=0.0;
	       return;
	    }
	    break;
	 case 0:
	    rules=default_rules;
	    create_unixtime_default(time());
	    return;
      }
      rules=default_rules;
      ::create(@args);
   }

   static void create_unixtime(int unixtime,int seconds)
   {
      t=(unixtime-TNG0UNIX)*TNGSTARPERSECOND;
      len=seconds*TNGSTARPERSECOND;
   }

   static void create_unixtime_default(int unixtime)
   {
      t=(unixtime-TNG0UNIX)*TNGSTARPERSECOND;
      len=0.0;
   }

   static void create_julian_day(int|float jd)
   {
      t=(jd-TNG0JULIAN)*TNGSTARPERJULIAN;
      len=0.0;
   }

// method float tic()
//	This gives back the start of the stardate period,
//	as a float.

   float tic()
   {
      return t;
   }

//! method float tics()
//!	This gives back the number of stardate tics
//!	in the period.

   float tics()
   {
      return len;
   }

//! method int number_of_seconds()
//! method int number_of_days()
//!	This gives back the Gregorian/Earth/ISO number of seconds
//!	and number of days, for convinience and conversion to
//!	other calendars.

   int number_of_seconds()
   {
      return (int)(len/TNGSTARPERSECOND);
   }

   int number_of_days()
   {
      return (int)(len/TNGSTARPERJULIAN);
   }

   int unix_time()
   {
      return ((int)(t/TNGSTARPERSECOND))+TNG0UNIX;
   }

   float julian_day()
   {
      return ((int)(t/TNGSTARPERJULIAN))+TNG0JULIAN;
   }

   static TimeRange _add(int n,void|this_program step)
   {
      float x;
      if (!step) 
	 x=len;
      else 
      {
	 if (!step->is_stardate)
	    error("add: Incompatible type %O\n",step);
	 x=step->len;
      }


      if (n&&x)
	 return Tick("stardate",rules,t+x,len);
      return this_object();
   }

   static void convert_from(TimeRange other)
   {
      if (other->unix_time)
	 create_unixtime_default(other->unix_time());
      else
	 ::convert_from(other);
      if (other->is_stardate)
      {
	 t=other->t;
	 len=other->len;
      }
      else if (other->number_of_seconds)
	 len=TNGSTARPERSECOND*other->number_of_seconds();
      else if (other->number_of_days)
	 len=TNGSTARPERJULIAN*other->number_of_days();
      else
	 len=0.0;
   }

   static TimeRange _set_size(int n,TimeRange x)
   {
      if (!x->is_stardate)
	 error("distance: Incompatible type %O\n",x);
      return Tick("stardate",rules,t,x->len);
   }

   TimeRange place(TimeRange what,void|int force)
   {
// can't do this
      return this_object();
   }

   array(TimeRange) split(int n)
   {
      if (!n) return ({this_object()}); // foo

      float z=t;
      float l=len/n;
      array(TimeRange) res=({});

      while (n--)
	 res+=({Tick("stardate",rules,z,l)}),z+=l;

      return res;
   }

   TimeRange beginning()
   {
      if (!len) return this_object();
      return Tick("stardate",rules,t,0.0);
   }

   TimeRange end()
   {
      if (!len) return this_object();
      return Tick("stardate",rules,t+len,0.0);
   }

   TimeRange distance(TimeRange to)
   {
      if (!to->is_stardate)
	 error("distance: Incompatible type %O\n",to);
      if (to->t<t)
	 error("negative distance\n");
      return Tick("stardate",rules,t,to->t-t);
   }

   array(int(-1..1)) _compare(TimeRange with)
   {
      float b1=t;
      float e1=t+len;
      float b2,e2;
      if (with->is_stardate)
	 b2=with->t,e2=b2+with->len;
      else 
	 ::_compare(with);
#define CMP(A,B) ( ((A)<(B))?-1:((A)>(B))?1:0 )
      return ({ CMP(b1,b2),CMP(b1,e2),CMP(e1,b2),CMP(e1,e2) });
   }

   int __hash() { return (int)t; }

   TimeRange set_ruleset(.Ruleset r)
   {
      return Tick("stardate",r,t,len);
   }

   string _sprintf(int t, mapping m)
   {
      switch (t)
      {
	 case 'O':
	    if (len!=0.0) 
	       return sprintf("Tick(%s)",nice_print_period());
	    return sprintf("Tick(%s)",nice_print());
	 default:
	    return 0;
      }
   }

   string nice_print_period()
   {
      if (len>0.010)
	 return sprintf("%s..%s",nice_print(),end()->nice_print());
      else
	 return sprintf("%s..%+g",nice_print(),len);
   }

   string nice_print()
   {
      return sprintf("%.3f",t);
   }

//! string format_long(void|int precision)
//! string format_short(void|int precision)
//! string format_vshort(void|int precision)
//!	Format the stardate tick nicely. Precision is the number of decimals. Defaults to 3.
//!	<pre>
//!	   long    "-322537.312"
//!	   short   "77463.312"  (w/o >100000-component)
//!	   vshort  "7463.312"  (w/o >10000-component)
//!	</pre>

   string format_long(void|int p)
   {
      return sprintf("%.*f",p||3,t);
   }

   string format_short(void|int p)
   {
      return sprintf("%.*f",p||3,t-((int)t/100000)*100000);
   }

   string format_vshort(void|int p)
   {
      return sprintf("%.*f",p||3,t-((int)t/10000)*10000);
   }
}

// compat
function(mixed...:cTick) TNGDate=cTick;

// for events
function(mixed...:cTick) Day=cTick;

//------------------------------------------------------------------------
//  global convinience functions
//------------------------------------------------------------------------

//! method Time.TimeofDay now()
//!	Give the zero-length time period of the
//!	current time.

.Time.TimeofDay now()
{
   return Tick();
}
