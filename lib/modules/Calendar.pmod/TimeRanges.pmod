//! module Calendar

// $Id: TimeRanges.pmod,v 1.31 2006/01/13 15:57:20 grubba Exp $

#pike __REAL_VERSION__

#define zero int(0..0)

program SuperTimeRange=cSuperTimeRange;

string calendar_name() { return "TimeRanges"; }

object calendar_object=this;

string _sprintf(int t) { return (t=='O')?calendar_name():0; }

.Ruleset default_rules=Calendar.default_rules;

//------------------------------------------------------------------------
//! class TimeRange
//! 	This is the base class (usually implemented by e.g. Calendar
//! 	subclasses like Calendar.Second) for any time measurement and
//! 	calendrar information. It defines all the things you can do
//! 	with a time range or any time period.
//!
//!	A TimeRange doubles as both a fixed period in
//!	time, and an amount of time. For instance,
//!	a week plus a day moves the week-period one day
//!	ahead (unaligning it with the week period, 
//!	and thereby reducing it to just 7 days),
//!	no matter when in time the actual day were.
//!
//------------------------------------------------------------------------

class TimeRange
{
   constant is_timerange=1;

   .Ruleset rules;

//! method void create("unix",int unixtime)
//! method void create("unix",int unixtime,int seconds_len)
//!	Create the timerange from unix time (as given by
//!	<tt>time(2)</tt>), with eventually the size of
//!	the time range in the same unit, seconds.
//!
//! method void create("julian",int|float julian_day)
//!	Create the timerange from a julian day, the 
//!	standardized method of counting days. If
//!	the timerange is more then a day, it will 
//!	at least enclose the day.
//!
//! method void create(TimeRange from)
//!	Create the timerange from another timerange.
//!
//!	This is useful when converting objects from
//!	one calendar to another. Note that the ruleset will be
//!	transferred to the new object, so this method
//!	can't be used to convert between timezones 
//!	or languges - use <ref>set_timezone</ref>,
//!	<ref>set_language</ref> or <ref>set_ruleset</ref>
//!	to achieve this.
//!
//! note:
//!	The size of the new object may be inexact;
//!	a Month object can't comprehend seconds, for instance.

   static void create_unixtime(int unixtime,int len);
   static void create_unixtime_default(int unixtime);
   static void create_julian_day(int jd);

   void create(mixed ...args)
   {
      if (sizeof(args)) switch (args[0])
      {
	 case "unix":
	    if (sizeof(args)==2)
	       create_unixtime_default(args[1]);
	    else if (sizeof(args)>2)
	       create_unixtime(@args[1..]);
	    else break;
	    return;
	 case "unix_r":
	    if (sizeof(args)==3)
	    {
	       rules=args[2];
	       create_unixtime_default(args[1]);
	    }
	    else if (sizeof(args)>2)
	    {
	       rules=args[3];
	       create_unixtime(args[1],args[2]);
	    }
	    else break;
	    return;
	 case "julian":
	    if (sizeof(args)==2)
	    {
	       create_julian_day(args[1]);
	       return;
	    }
	    break;
	 case "julian_r":
	    if (sizeof(args)==3)
	    {
	       rules=args[2];
	       create_julian_day(args[1]);
	       return;
	    }
	    break;

	 default:
	    if (objectp(args[0]) && args[0]->is_timerange)
	    {
	       convert_from([object(TimeRange)]args[0]);
	       return;
	    }
	    break;
      }

      error("%O.%O: Illegal parameters %O,%O,%O...\n",
	    function_object(this_program),
	    this_program,@args,0,0,0);
   }

   static void convert_from(TimeRange other)
   {
// inheriting class must take care of size
      if (other->julian_day)
      {
	 int|float jd=other->julian_day();
	 if (floatp(jd) && other->unix_time)
	    create("unix_r",other->unix_time(),other->ruleset());
	 else
	    create("julian_r",jd,other->ruleset());
      }
      else if (other->unix_time)
	 create("unix_r",other->unix_time(),other->ruleset());
      else 
	 error("Can't convert %O->%s.%O\n",other,
	       calendar_name(), this_program);
   }

//! method TimeRange set_size(TimeRange size)
//! method TimeRange set_size(int n,TimeRange size)
//!	Gives back a new (or the same, if the size matches) 
//!	timerange with the new size. 
//!	If <i>n</i> are given, the resulting size
//!	will be <i>n</i> amounts of the given size.
//! note:	
//!	A negative size is not permitted; a zero one are.

// virtual
   static TimeRange _set_size(int n,TimeRange x);

   TimeRange set_size(function|TimeRange|int(0..0x7fffffff) a,
		      void|function|TimeRange b)
   {
      function|object(TimeRange) x;
      int(0..0x7fffffff) n;
      if (!b) 
	 if (intp(a)) 
	    x=this,n=[int(0..0x7fffffff)]a;
	 else
	    x=a,n=1;
      else
	 x=b,n=a;
      if (functionp(x)) x=promote_program(x);
      if (n<0)
	 error("Negative size (%d)\n",n);
      return _set_size(n,[object(TimeRange)]x);
   }

//! method TimeRange add(int n,void|TimeRange step)
//!     calculates the (promoted) time period n steps away;
//!	if no step is given, the step's length is of the
//!	same length as the called time period.
//!
//!	It is not recommended to loop by adding the increment
//!	time period to a shorter period; this can cause faults,
//!	if the shorter time period doesn't exist in the incremented 
//!	period. (Like week 53, day 31 of a month or the leap day of a year.)
//!
//!	Recommended use are like this:
//!	<pre>
//!	   // loop over the 5th of the next 10 months
//!	   TimeRange month=Month()+1;
//!	   TimeRange orig_day=month()->day(5);
//!	   for (int i=0; i&lt;10; i++)
//!	   {
//!	      month++;
//!	      TimeRange day=month->place(orig_day);
//!	      <i>...use day...</i>
//!	   }
//!	</pre>

// virtual
   static this_program _add(int n,this_program step);

   this_program add(function|this_program|int a,
		 void|function|this_program b)
   {
      function|object(this_program) x;
      int n;
      if (!b) 
	 if (intp(a)) 
	   x=this,n=[int]a;
	 else
	    x=a,n=1;
      else
	 x=b,n=a;
      if (functionp(x)) x=promote_program(x);
      return _add(n,[object(this_program)]x);
   }

//! method TimeRange place(TimeRange this)
//! method TimeRange place(TimeRange this,int(0..1) force)
//!	This will place the given timerange in this timerange,
//!	for instance, day 37 in the year - 
//!	<tt>Year(1934)->place(Day(1948 d37)) => Day(1934 d37)</tt>.
//!
//! note:
//!	The rules how to place things in different timeranges
//!	can be somewhat 'dwim'.

// virtual
   TimeRange place(TimeRange what,void|int force);

//! method TimeRange `+(int n)
//! method TimeRange `+(TimeRange offset)
//! method TimeRange `-(int m)
//! method TimeRange `-(TimeRange x)
//!	This calculates the (promoted) time period 
//!	either n step away or with a given offset.
//!	These functions does use <ref>add</ref> to really
//!	do the job:
//!	<pre>
//!	t+n         t->add(n)             t is a time period
//!	t-n         t->add(-n)            offset is a time period
//!	t+offset    t->add(1,offset)      n is an integer
//!	t-offset    t->add(-1,offset)
//!	n+t         t->add(n)
//!	n-t         illegal
//!	offset+t    offset->add(1,t)      | note this!
//!	offset-t    offset->add(-1,t)     | 
//!	</pre>
//!
//!	Mathematic rules:
//!	<pre>
//!	x+(t-x) == t    x is an integer or a time period
//!	(x+t)-x == t    t is a time period
//!	(t+x)-x == t
//!	o-(o-t) == t    o is a time period 
//!     t++ == t+1
//!     t-- == t-1
//!	</pre>
//!
//! note:
//!	a-b does not give the distance between the start of a and b.
//!	Use the <ref>distance</ref>() function to calculate that.
//!
//!	The integer used to `+, `- and add are the <i>number</i>
//!	of steps the motion will be. It does <i>never</i> represent
//!	any <i>fixed</i> amount of time, like seconds or days.

   TimeRange `+(program|this_program|int n)
   {
      if (objectp(n)) return add(1,n);
      return add(n);
   }

   TimeRange ``+(int n)
   {
      return add(n);
   }

   TimeRange `-(TimeRange|program|int n)
   {
      if (objectp(n)) return add(-1,n);
      return add(-n);
   }

//! method TimeRange next()
//! method TimeRange prev()
//!	Next and prev are compatible and convinience functions;
//!	<tt>a->next()</tt> is exactly the same as <tt>a+1</tt>;
//!	<tt>a=a->next()</tt> is <tt>a++</tt>.

   TimeRange next()
   {
      return this+1;
   }

   TimeRange prev()
   {
      return this-1;
   }

//! method TimeRange `*(int n)
//!	This changes the amount of time in 
//!	the time period. <tt>t*17</tt> is
//!	the same as doing <tt>t-><ref>set_size</ref>(t,17)</tt>.

   function ``* = `*;
   TimeRange `*(int|float n)
   {
      return set_size((int)n,this);
   }

//! method array(TimeRange) `/(int|float n)
//! method array(TimeRange) split(int|float n, void|TimeRange with)
//!	This divides the called timerange into
//!	n pieces. The returned timerange type
//!	is not neccesarily of the same type as the called one.
//!     If the optional timerange is specified then the resulting timeranges
//!     will be multiples of that range (except for the last one).
//!
//! known bugs:
//!	These are currently not defined for 
//!	<ref to=SuperTimeRange>supertimeranges</ref>.

//! method int `/(TimeRange with)
//! method int how_many(TimeRange with)
//!	This calculates how many instances of the given
//!	timerange has passed during the called timerange.
//!
//!	For instance, to figure out your age,
//!	create the timerange of your lifespan, and divide
//!	with the instance of a <ref to=YMD.Year>Year</ref>.

// virtual
   array(TimeRange) split(int|float n, void|function|TimeRange with);

   int how_many(function|TimeRange with)
   {
      if (functionp(with)) with=promote_program(with);
// default method; not optimized - guessing

      TimeRange start=beginning();
      TimeRange end=end();
      
// find low and high 2^n
      int nn=16;
      int low,high;

      TimeRange t=start+with*nn;
      if (t==end) return nn; // gotcha (luck)
      if (t>end) // scan less
      {
	 for (;;)
	 {
	    nn>>=1;
	    t=start+with*nn;
	    if (t==end) return nn; // gotcha (luck)
	    if (t<end) // ok, we're limited
	    {
	       low=nn;
	       high=nn*2-1;
	       break;
	    }
	    if (nn==1) return 0; // can't be less
	 }
      }
      else
      {
   // sanity check
	 TimeRange q=start+with*(nn+1);
	 if (q==t) error("Result is infinite - argument is zero range\n");
	 for (;;)
	 {
	    nn<<=1;
	    t=start+with*nn;
	    if (t==end) return nn; // gotcha (luck)
	    if (t>end) // ok, we're limited
	    {
	       low=(nn>>1);
	       high=nn-1;
	       break;
	    }
      // loop forever - pike can handle *big* numbers
	 }
      }

      for (;;)
      {
	 nn=(low+high)/2;

	 t=start+with*nn;
  	 if (t==end) return nn; 
	 if (t<end) low=nn+1;
	 else high=nn-1; 

	 if (low>high) return high; // can't go further
      }
   }

   array(TimeRange)|int `/(TimeRange|program|int|float x)
   {
      if (intp(x) || floatp(x)) return split(x);
      else return how_many(x);
   }

//! method int offset_to(TimeRange x)
//!	Calculates offset to x; this compares
//!	two timeranges and gives the integer offset
//!	between the two starting points.
//!	
//!	This is true for suitable a and b:
//!	<tt>a+a->offset_to(b)==b</tt>
//!
//!	By suitable means that a and b are of the same
//!	type and size. This is obviously true only
//!	if a+n has b as a possible result for any n.

   int offset_to(TimeRange x)
   {
      if (x==this) return 0;
      if (x<this)
	 return -(x->distance(this)/this);
      return this->distance(x)/this;
   }

//! method TimeRange beginning()
//! method TimeRange end()
//!	This gives back the zero-sized beginning
//!	or end of the called time period.
//!
//!	rule: 
//!	<tt>range(t->beginning(),t->end())==t</tt>

   TimeRange beginning();
   TimeRange end();

//! method TimeRange range(TimeRange other)
//! method TimeRange space(TimeRange other)
//! method TimeRange distance(TimeRange other)
//!	Derives different time periods in between
//!	the called timerange and the parameter timerange.
//!
//!     <pre>
//!    &gt;- the past          the future -&lt;
//!	|--called--|         |--other--|
//!     &gt;------------ range -----------&lt;
//!                &gt;--space--&lt;
//!     &gt;----- distance -----&lt;
//!	</pre>
//!
//!	See also: add, TimeRanges.range, TimeRanges.space, TimeRanges.distance

// virtual
   TimeRange distance(TimeRange to);

   TimeRange range(TimeRange with)
   {
      return distance(with->end());
   }

   TimeRange space(TimeRange to)
   {
      return end()->distance(to->beginning());
   }


//! method int(0..1) strictly_preceeds(TimeRange what);
//! method int(0..1) preceeds(TimeRange what);
//! method int(0..1) is_previous_to(TimeRange what);
//! method int(0..1) overlaps(TimeRange what);
//! method int(0..1) contains(TimeRange what);
//! method int(0..1) equals(TimeRange what);
//! method int(0..1) is_next_to(TimeRange what);
//! method int(0..1) succeeds(TimeRange what);
//! method int(0..1) strictly_succeeds(TimeRange what);
//!	These methods exists to compare two periods
//!	of time on the timeline.
//!
//! <pre>
//!           case            predicates 
//!
//!  &lt;-- past       future -&gt;
//!
//!  |----A----|              A strictly preceeds B,
//!               |----B----| A preceeds B
//!
//!  |----A----|              A strictly preceeds B, A preceeds B, 
//!            |----B----|    A is previous to B, A touches B
//!                           
//!      |----A----|          A preceeds B,
//!            |----B----|    A overlaps B, A touches B
//!
//!      |-------A-------|    A preceeds B, A ends with B
//!            |----B----|    A overlaps B, A contains B, A touches B,
//!
//!      |-------A-------|    A preceeds B,  A succeeds B,
//!          |---B---|        A overlaps B, A contains B, A touches B
//!
//!         |----A----|       A overlaps B, A touches B, A contains B
//!         |----B----|       A equals B, A starts with B, A ends with B
//!
//!      |-------A-------|    A succeeds B, A starts with B
//!      |----B----|          A overlaps B, A contains B, A touches B
//!
//!            |----A----|    A succeeds B, 
//!      |----B----|          A overlaps B, A touches B
//!
//!            |----A----|    A strictly succeeds B, A succeeds B
//!  |----B----|              A is next to B, A touches B
//!
//!              |----A----|  A strictly succeeds B, 
//!  |----B----|              A succeeds B
//!
//!  </pre>
//!
//! note:
//!	These methods only check the range of the first to the 
//!	last time in the period;
//!	use of combined time periods (<ref>SuperTimeRange</ref>s)
//!	might not give you the result you want. 
//!
//! see also: `&


//- internal method 
//- returns [-1,0,1] for comparison between
//- (in order) begin/begin,begin/end,end/begin and end/end

// virtual, default
   array(int(-1..1)) _compare(TimeRange what)
   {
      if (objectp(what) && what->is_supertimerange)
      {
	 array(int(-1..1)) cmp=what->_compare(this);

	 return ({-cmp[0],
		  -cmp[2],
		  -cmp[1],
		  -cmp[3]});
      }
      return ({-1,-1,-1,-1});
//        error("_compare: incompatible classes %O <-> %O\n",
//  	    this_program,object_program(what));
   }

   string _describe_compare(array(int(-1..1)) c,TimeRange a,TimeRange b)
   {
      mapping desc=([-1:"<",0:"=",1:">"]);
      return sprintf("%O start %s %O start\n"
		     "%O start %s %O end\n"
		     "%O end   %s %O start\n"
		     "%O end   %s %O end\n",
		     a,desc[c[0]],b,
		     a,desc[c[1]],b,
		     a,desc[c[2]],b,
		     a,desc[c[3]],b);
   }

#define BEGIN_BEGIN 0
#define BEGIN_END 1
#define END_BEGIN 2
#define END_END 3

   int(0..1) strictly_preceeds(TimeRange what)
   {
      array(int(-1..1)) a=_compare(what);
      return a[END_BEGIN]<0;
   }

   int(0..1) preceeds(TimeRange what)
   {
      array(int(-1..1)) a=_compare(what);
      return a[BEGIN_BEGIN]<0;
   }

   int(0..1) is_previous_to(TimeRange what)
   {
      array(int(-1..1)) a=_compare(what);
      return a[END_BEGIN]==0;
   }

   int(0..1) overlaps(TimeRange what)
   {
      array(int(-1..1)) a=_compare(what);
      return (a[END_BEGIN]>0 && a[BEGIN_END]<0);
   }

   int(0..1) contains(TimeRange what)
   {
      array(int(-1..1)) a=_compare(what);
      return a[BEGIN_BEGIN]<=0 && a[END_END]>=0;
   }

   int(0..1) equals(TimeRange what)
   {
      if (!objectp(what)) return 0;
      array(int(-1..1)) a=_compare(what);
      return a[BEGIN_BEGIN]==0 && a[END_END]==0;
   }

   int(0..1) is_next_to(TimeRange what)
   {
      array(int(-1..1)) a=_compare(what);
      return a[END_BEGIN]==0;
   }

   int(0..1) succeeds(TimeRange what)
   {
      array(int(-1..1)) a=_compare(what);
      return a[END_END]>0;
   }

   int(0..1) strictly_succeeds(TimeRange what)
   {
      array(int(-1..1)) a=_compare(what);
      return a[BEGIN_END]>0;
   }

   int(0..1) touches(TimeRange what)
   {
      array(int(-1..1)) a=_compare(what);
      return 
	 (a[BEGIN_END]<=0 && a[END_END]>=0) ||
	 (a[END_BEGIN]>=0 && a[BEGIN_BEGIN]<=0);
   }

   int (0..1) starts_with(TimeRange what)
   {
      array(int(-1..1)) a=_compare(what);
      return !a[BEGIN_BEGIN];
   }

   int (0..1) ends_with(TimeRange what)
   {
      array(int(-1..1)) a=_compare(what);
      return !a[END_END];
   }

//! method int(0..1) `<(TimeRange compared_to)
//! method int(0..1) `>(TimeRange compared_to)
//!	These operators sorts roughty on the 
//!	periods place in time. The major use
//!	might be to get multiset to work, 
//!	besides sorting events clearly defined in time.

   int(0..1) `<(TimeRange compared_to)
   {
      array(int(-1..1)) a=_compare(compared_to);
      if (a[0]<0) return 1;
      if (a[0]>0) return 0;
      if (a[3]<0) return 1;
      return 0;
   }

   int(0..1) `>(TimeRange compared_to)
   {
      array(int(-1..1)) a=_compare(compared_to);
      if (a[0]>0) return 1;
      if (a[0]<0) return 0;
      if (a[3]>0) return 1;
      return 0;
   }

//! method int(0..1) `==(TimeRange compared_to)
//! method int(0..1) _equal(TimeRange compared_to)
//!	These two overloads the operator <tt>`==</tt> and
//!	the result of the <tt>equal</tt> function.
//!
//!	a<tt>==</tt>b is considered true if the two
//!	timeranges are of the same type, have the same rules
//!	(language, timezone, etc) and are the same timerange.
//!
//!	<tt>equal(</tt>a<tt>,</tt>b<tt>)</tt> are considered
//!	true if a and b are the same timerange, exactly the same
//!	as the <ref>equals</ref> method.
//!
//!	The <tt>__hash</tt> method is also present, 
//!	to make timeranges possible to use as keys in mappings.
//!
//! known bugs:
//!	_equal is not currently possible to overload,
//!	due to weird bugs, so equal uses `== for now.

   int(0..1) `==(mixed what) 
   {
     return objectp(what) && functionp(what->ruleset) &&
       what->ruleset()==ruleset() && equals(what); 
   }

   int __hash();

//     int(0..1) _equal(TimeRange what) 
//     { 
//        return equals(what);
//     }

//! method TimeRange `&(TimeRange with)
//!	Gives the cut on the called time period
//!	with another time period. The result is
//!	zero if the two periods doesn't overlap.
//!     <pre>
//!    &gt;- the past          the future -&lt;
//!	|-------called-------|         
//!	     |-------other--------|
//!          &gt;----- cut -----&lt;
//!	</pre>

   function ``& = `&;
   TimeRange|zero `&(TimeRange with, mixed ...extra)
   {
      if (with->is_nulltimerange) 
	 return with;
      array(int(-1..1)) a=_compare(with);
      if (a[END_BEGIN]<0 || a[BEGIN_END]>0) 
	 return nulltimerange; // no overlap, no extra

      if (with->is_supertimerange)
	 return predef::`&(with,this,@extra); // let it handle that...

      TimeRange from,to;

// from the last beginning
      if (a[BEGIN_BEGIN]>0) from=beginning(); else from=with->beginning();

// to the first end
      if (a[END_END]<0) to=end(); else to=with->end();

// compute
      TimeRange res=from->distance(to);
      if (sizeof(extra)) return predef::`&(res,@extra);
      return res;
   }

//! method TimeRange `|(TimeRange with)
//!	Gives the union on the called time period
//!	and another time period.
//!     <pre>
//!    &gt;- the past          the future -&lt;
//!	|-------called-------|         
//!	     |-------other--------|
//!     &lt;----------union----------&gt;
//!	</pre>

   function ``| = `|;
   TimeRange `|(TimeRange with,mixed ...extra)
   {
      if (with->is_nulltimerange) 
	 return sizeof(extra)?`|(@extra):this;
      array(int(-1..1)) a=_compare(with);
      TimeRange from,to;

      if (a[END_BEGIN]<0 || a[BEGIN_END]>0)
	 from=SuperTimeRange( sort(({this,with})) ); // no overlap
      else
      {
	 if (with->is_supertimerange)   // let it handle that...
	    return predef::`|(with,this,@extra);

   // from the first beginning
	 if (a[BEGIN_BEGIN]<0) from=this; else from=with;

   // to the last end
	 if (a[END_END]>0) to=this; else to=with;
   // compute
	 from=from->range(to);
      }
      if (sizeof(extra)) return predef::`|(from,@extra);
      return from;
   }

//! method TimeRange `^(TimeRange with)
//!	Gives the exclusive-or on the called time period
//!	and another time period, ie the union without
//!	the cut. The result is zero if the 
//!	two periods were the same.
//!     <pre>
//!    &gt;- the past          the future -&lt;
//!	|-------called-------|         
//!	     |-------other--------|
//!     &lt;----|               |---->   - exclusive or
//!	</pre>

   function ``^ = `^;
   TimeRange `^(TimeRange with,mixed ... extra)
   {
      if (with->is_supertimerange)
	 return `^(with,this,@extra); // let it handle that...
      if (with->is_nulltimerange) 
	 return sizeof(extra)?`^(@extra):this;

      TimeRange res;

      array(int(-1..1)) a=_compare(with);
      
//        write(_describe_compare(a,this,with));

      TimeRange first,second;
      if (a[END_BEGIN]<0 || a[BEGIN_END]>0)
	 res=SuperTimeRange( sort(({this,with})) ); // no overlap
      else if (a[BEGIN_END]==0 || a[END_BEGIN]==0) // bordering
	 if (a[BEGIN_BEGIN]<0)
	    res=range(with); // A precedes B
	 else
	    res=with->range(this); // B precedes A
      else if (a[BEGIN_BEGIN]==0 && a[END_END]==0)
	 return sizeof(extra)?predef::`^(nulltimerange,@extra):nulltimerange;
      else
      {
   // from the first beginning to the second beginning
	 if (a[BEGIN_BEGIN]<0) 
	    first=distance(with); 
	 else 
	    first=with->distance(this);

// and from the first end to the last end
	 if (a[END_END]<0) 
	    second=end()->range(with); 
	 else 
	    second=with->end()->range(this);
	 res=first|second;
      }
   // done
      if (sizeof(extra)) return `^(res,@extra);
      return res;
   }

//! method TimeRange subtract(TimeRange what)
//!	This subtracts a period of time from another;
//!     <pre>
//!     &gt;- the past          the future -&lt;
//!	|-------called-------|         
//!	     |-------other--------|
//!     &lt;----&gt;  &lt;- called-&gt;subtract(other)
//!
//!	|-------called-------|         
//!	     |---third---|
//!     &lt;----&gt;           &lt;---> &lt;- called->subtract(third)
//!	</pre>

   TimeRange subtract(TimeRange what,mixed ... extra)
   {
      array(int(-1..1)) a=_compare(what);

      if (a[END_BEGIN]<=0 || a[BEGIN_END]>=0)
	 return sizeof(extra)?subtract(@extra):this; // no overlap

      if (what->is_supertimerange)
      {
	 array res=map(what->parts+extra,subtract)-({nulltimerange});
	 switch (sizeof(res))
	 {
	    case 0: return nulltimerange;
	    case 1: return res[0];
	    default: return predef::`&(@res);
	 }
      }

      TimeRange res;

//        write(_describe_compare(a,this,what));

      if (a[BEGIN_BEGIN]>=0)      // it preceeds us
	 if (a[END_END]<=0) 
	    return nulltimerange; // full overlap
	 else                     // half overlap at start
	    res=what->end()->range(this);
      else if (a[END_END]<=0)     // it succeeds us
	 res=distance(what);
      else 
      {
//  	 werror("%O..\n..%O\n%O..\n..%O\n",
//  		beginning(),what->beginning(),
//  		what->end(),end());
// it's inside us
	 res=predef::`|(distance(what),
			what->end()->range(this));
      }
      if (sizeof(extra)) return res->subtract(@extra);
      return res;
   }

//! method TimeRange set_ruleset(Ruleset r)
//! method TimeRange ruleset(Ruleset r)
//!	Set or get the current ruleset.
//! note: 
//!	this may include timezone shanges,
//!	and change the time of day.

   this_program set_ruleset(.Ruleset r);
   .Ruleset ruleset()
   {
      return rules;
   }

//! method TimeRange set_timezone(Timezone tz)
//! method TimeRange set_timezone(string tz)
//! method TimeZone timezone()
//!	Set or get the current timezone (including dst) rule.
//!
//! note: 
//!	The time-of-day may very well
//!	change when you change timezone.
//!
//!	To get the time of day for a specified timezone,
//!	select the timezone before getting the time of day:
//!
//!	<tt>Year(2003)-&gt;...-&gt;set_timezone(TimeZone.CET)-&gt;...-&gt;hour(14)-&gt;...</tt>
//!

   this_program set_timezone(string|.Rule.Timezone tz)
   {
      return set_ruleset(rules->set_timezone(tz));
   }

   .Rule.Timezone timezone()
   {
      return rules->timezone;
   }

//! method TimeRange set_language(Rule.Language lang)
//! method TimeRange set_language(string lang)
//! method Language language()
//!	Set or get the current language rule.

   this_program set_language(string|.Rule.Language lang)
   {
      return set_ruleset(rules->set_language(lang));
   }

   .Rule.Language language()
   {
      return rules->language;
   }

//! method Calendar calendar()
//!	Simply gives back the calendar in use, for instance
//!	Calendar.ISO or Calendar.Discordian.

   object calendar()
   {
      return calendar_object;
   }


   string _sprintf(int t,mapping m)
   {
      switch (t)
      {
	 case 'O':
	    return "TimeRange()";
	 case 't':
	    return "Calendar."+calendar_name()+".TimeRange";
	 default:
	    return 0;
      }
   }
}

// ----------------------------------------------------------------

//!
//! module Calendar
//! class SuperTimeRange
//!     This class handles the cases where you have a time
//!	period with holes. These can be created by the
//!	<tt>^</tt> or <tt>|</tt> operators on time ranges.
//! inherits TimeRange

class cSuperTimeRange
{
   inherit TimeRange;

   constant is_supertimerange=1;

   array parts;

//! method void create(array(TimeRange) parts)
//!	A SuperTimeRange must have at least two parts,
//!	two time ranges. Otherwise, it's either not
//!	a time period at all or a normal time period.

   void create(array(TimeRange) _parts)
   {
      if (sizeof(_parts->is_supertimerange-({0})))
	 error("one part is super\n%O\n",_parts);
      if (sizeof(_parts)<2)
	 error("SuperTimeRange: Too few time periods to constructor\n");
      parts=_parts;
   }

   TimeRange beginning()
   {
      return parts[0]->beginning();
   }

   TimeRange end()
   {
      return parts[-1]->end();
   }

   TimeRange distance(TimeRange to)
   {
      return beginning()->distance(to);
   }

   TimeRange mend_overlap(array parts)
   {
      switch (sizeof(parts))
      {
	 case 0: return nulltimerange;
	 case 1: return parts[0];
      }
      array res=({});
      TimeRange last=parts[0];
      foreach (parts[1..],TimeRange part)
      {
	 if (last->strictly_preceeds(part))
	 {
	    res+=({last});
	    last=part;
	 }
	 else
	    last=last|part;
      }
      if (!sizeof(res)) return last;
      return SuperTimeRange(res+({last}));
   }

   TimeRange `&(TimeRange with,mixed... extra)
   {
      array r=({});
      foreach (parts,TimeRange part)
      {
	 TimeRange tmp=predef::`&(part,with,@extra);
	 if (tmp)
	    if (tmp->is_supertimerange) r+=tmp->parts;
	    else if (!tmp->is_nulltimerange) r+=({tmp});
      }
      switch (sizeof(r))
      {
	 case 0: return nulltimerange;
	 case 1: return r[0]; 
	 default: return SuperTimeRange(r); 
      }
   }

   TimeRange `|(TimeRange with,mixed ...extra)
   {
      TimeRange res;
      if (with->is_supertimerange)
	 res=mend_overlap(sort(with->parts+parts));
      else if (with->is_nulltimerange)
	 return this;
      else
	 res=mend_overlap(sort( ({with})+parts ));
      if (sizeof(extra)) 
	 return predef::`|(res,@extra);
      return res;
   }

   TimeRange subtract(TimeRange with,mixed ...extra)
   {
      array r=({});
      foreach (parts,TimeRange part)
	 r+=({part->subtract(part,with,@extra)});
      return predef::`|(@r);
   }

   TimeRange `^(TimeRange with,mixed ...extra)
   {
//        werror("1 %O\n",with);
//        werror("2 %O\n",`|(with));
//        werror("3 %O\n",`&(with));
      TimeRange r=`|(with)->subtract(`&(with));
      if (sizeof(extra)) return predef::`^(r,@extra);
      return r;
   }

// == checks if equal 

   int `==(TimeRange with,mixed ...extra)
   {
      if (!with->is_supertimerange)
	 return 0; // it has to be
      if (sizeof(parts)!=sizeof(with->parts))
	 return 0; // it has to be
      for (int i=0; i<sizeof(parts); i++)
	 if (parts[i]!=with->parts[i]) return 0;
      return sizeof(extra)?predef::`==(with,@extra):1;
   }

// _compare wrapper
   array(int(-1..1)) _compare(TimeRange what)
   {
      array(int(-1..1)) a,b;
      a=parts[0]->_compare(what);
      b=parts[-1]->_compare(what);
      return ({a[0],a[1],b[2],b[3]});
   }

// `< and `> sort for multiset
// a little bit heavier for super-time-ranges

   int(0..1) `<(TimeRange with)
   {
      array(int(-1..1)) a=_compare(with);
      if (a[0]<0) return 1;
      if (a[0]>0) return 0;
      if (a[3]<0) return 1;
      if (a[3]>0) return 0;
      if (!with->is_supertimerange) return 1; // always
      if (sizeof(parts)>sizeof(with->parts)) return 1; 
      if (sizeof(parts)<sizeof(with->parts)) return 0; 
      for (int i=0; i<sizeof(parts); i++)
	 if (parts[i]<with->parts[i]) return 1;
      return 0;
   }

   int(0..1) `>(TimeRange with)
   {
      array(int(-1..1)) a=_compare(with);
      if (a[0]>0) return 1;
      if (a[0]<0) return 0;
      if (a[3]>0) return 1;
      if (a[3]<0) return 0;
      if (!with->is_supertimerange) return 0; // always
      if (sizeof(parts)<sizeof(with->parts)) return 1; 
      if (sizeof(parts)>sizeof(with->parts)) return 0; 
      for (int i=0; i<sizeof(parts); i++)
	 if (parts[i]>with->parts[i]) return 1;
      return 0;
   }

   int __hash()
   {
      return predef::`+(@map(parts,"__hash"));
   }

   string _sprintf(int t,mapping m)
   {
      switch (t)
      {
	 case 'O':
	    return "SuperTimeRange("+
	       map(parts,"_sprintf",'O')*", "+")";
	 case 't':
	    return "SuperTimeRange("+
	       map(parts,"_sprintf",'t')*", "+")";
      }
      return ::_sprintf(t,m);
   }

   TimeRange set_timezone(string|.Rule.Timezone tz)
   {
// fixme?
      return `|(@map(parts,"set_timezone",tz));
   }
}

//! module Calendar
//! constant TimeRange nulltimerange
//!	This represents the null time range, 
//!	which, to differ from the zero time range
//!	(the zero-length time range), isn't placed
//!	in time. This is the result of for instance
//!	<ref>`&</ref> between two strict non-overlapping
//!	timeranges - no time at all.
//!
//! 	It has a constant, <tt>is_nulltimerange</tt>, which
//!	is non-zero. <tt>`!</tt> on this timerange is true.


program NullTimeRange=cNullTimeRange;
static class cNullTimeRange
{
   inherit TimeRange;

   constant is_nulltimerange=1;

// overload
   void create()
   {
   }

   TimeRange set_size(TimeRange|int(0..0x7fffffff) a,void|TimeRange b)
   {
      return this;
   }

   TimeRange place(TimeRange what,void|int force)
   {
      return this;
   }

   array(TimeRange) split(int n)
   {
      return allocate(n,this);
   }

   TimeRange beginning() { return this; }
   TimeRange end() { return this; }

   TimeRange distance(TimeRange to)
   {
      if (to==this) return this;
      error("Can't distance/space/range with the null timerange\n");
   }

   array(int(-1..1)) _compare(TimeRange with)
   {
      if (with==this) return ({0,0,0,0});
      return ({-1,-1,-1,-1});
   }

   int(0..1) `<(TimeRange with)
   {
      return !(with==this);
   }

   int(0..1) `>(TimeRange with)
   {
      return 0;
   }

   int(0..1) `==(TimeRange with)
   {
      return objectp(with) && with->is_nulltimerange;
   }

   int(0..1) equals(TimeRange with)
   {
      return objectp(with) && with->is_nulltimerange;
   }

   TimeRange `&(TimeRange with, mixed ...extra)
   {
      return predef::`&(with,this,@extra);
   }

   TimeRange `|(TimeRange with, mixed ...extra)
   {
      return predef::`|(with,this,@extra);
   }

   TimeRange `^(TimeRange with, mixed ...extra)
   {
      return predef::`^(with,this,@extra);
   }

   this_program subtract(TimeRange with, mixed ...extra)
   {
      return this;
   }

   int(1..1) `!()
   {
      return 1;
   }

   string _sprintf(int t,mapping m)
   {
      switch (t)
      {
	 case 'O': return "NullTimeRange";
	 case 't': return "Calendar."+calendar_name()+".NullTimeRange";
	 default: return ::_sprintf(t,m);
      }
   }
}

cNullTimeRange nulltimerange=NullTimeRange();

// helper functions

static mapping(function:TimeRange) program2stuff=([]);

static TimeRange promote_program(function p)
{
   TimeRange x;
   if ( (x=program2stuff[p]) ) return x;
   x=[object(TimeRange)]p();
   if (!x->is_timerange)
      error("Not a timerange program: %O\n",p);
   return program2stuff[p]=x;
}
