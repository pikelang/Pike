#pike __REAL_VERSION__

import ".";
constant TimeRange=TimeRanges.TimeRange;

constant M_YD=({0,0,31,59,90,120,151,181,212,243,273,304,334});
constant M_ED=({({0,31,59,90,120,151,181,212,243,273,304,334,365}),
		({0,31,60,91,121,152,182,213,244,274,305,335,366}),
		({0,31,60,90,120,151,181,212,243,273,304,334,365}) });
constant M_NAME="---JanFebMarAprMayJunJulAugSepOctNovDec"/3;
constant WD_NAME="---MonTueWedThuFriSatSun"/3;

function(mixed...:TimeRange) std_day=master()->resolv("Calendar")["Day"];
function(mixed...:TimeRange) std_second=master()->resolv("Calendar")["Second"];

// ----------------------------------------------------------------
// base classes
// ----------------------------------------------------------------



//! 	@[Event] is a base class, defining the
//!	methods an Event need to have.
class Event
{
   string name;
   string id="?";

   constant is_event=1;

   //!	These functions calculate the next or previous occurance of the event,
   //!	from the given timerange's @b{start@},
   //!	including any event occuring at the start if the
   //!  @[including] flag is set.
   //!
   //!  @returns
   //!	  Zero is returned if there is no next event.
   //!
   //!  @note
   //!	  These methods are virtual in the base class.
   TimeRange next(void|TimeRange from,void|int(0..1) including);
   TimeRange previous(void|TimeRange from,void|int(0..1) including);

// what events in this period?
   //!	This calculates the eventual events that
   //!	are contained or overlapped by the given timerange.
   //!	
   //!	@example
   //!	  @code{Event.christmas_eve->scan(Year(2000))@}
   //!	  => @code{({ Day(Sun 24 Dec 2000) })@}.
   //!
   //!	@[scan()] uses @[next()] if not overloaded.
   //!
   //! @note
   //!	@[scan()] can return an array of overlapping timeranges.
   //!	
   //!	This method must use @tt{in->calendar_object->@i{type@}@}
   //!	to create the returned timeranges, and must keep the ruleset.
   array(TimeRange) scan(TimeRange in)
   {
      array res=({});
      TimeRange t=next(in,1);
      for (;;)
      {
	 if (!t || !t->overlaps(in)) return res;
	 res+=({t});
	 t=next(t);
      }
   }

   mapping(TimeRange:Event) scan_events(TimeRange in)
   {
      array r=scan(in);
      return mkmapping(r,allocate(sizeof(r),this_object()));
   }

   Event `|(Event ... with)
   {
      with-=({0});
      with|=({this_object()});
      if (sizeof(with)==1) return with[0];
      return SuperEvent(with);
   }
   Event ``|(Event with) { return `|(with); }

   string _sprintf(int t)
   {
      return (t!='O')?0:sprintf("Event(%s:%O)",id,name);
   }

   array(Event) cast(string to)
   {
      if (to[..4]=="array")
	 return ({this_object()});
      else
	 error("Can't cast to %O\n",to);
   }

   string describe()
   {
      return "Unknown event";
   }
}

class NullEvent
{
   inherit Event;

   constant is_nullevent=1;

   void create(string _id,string s,mixed ...args) 
   {
      id=_id;
      name=s;
   }
 
   TimeRange next(void|TimeRange from,void|int(0..1) including)
   {
      return 0;
   }

   TimeRange previous(void|TimeRange from,void|int(0..1) including)
   {
      return 0;
   }
}

//! module Calendar
//! submodule Event
//! subclass Day_Event
//! 	<ref>Day_Event</ref> is a base class, extending <ref>Event</ref>
//!	for events that are single days, using julian day numbers
//!	for the calculations.
//!
//! method array(TimeRange) scan(TimeRange in)
//! method TimeRange next(TimeRange from)
//! method TimeRange next(TimeRange from,int(0..1) including)
//!	These methods are implemented, using the
//!	virtual method <ref>scan_jd</ref>.
//! see also: Event
//!
//! method int scan_jd(Calendar realm,int jd,int(-1..-1)||int(1..1) direction)
//!	These methods has to be defined, and is what
//!	really does some work. It should return the next or previos
//!	julian day (&gt;<i>jd</i>) when the event occurs,
//!	or the constant <tt>NODAY</tt> if it doesn't.
//!
//!	<i>direction</I> <tt>1</tt> is forward (next), 
//!	<tt>-1</tt> is backward (previous).

class Day_Event
{
   inherit Event;

   constant is_day_event=1;
   constant NODAY=-1;

   int nd=1;

// find the next (or same) jd with the event
   int scan_jd(Calendar realm,int jd,int(1..1)|int(-1..-1) direction);

// find
   TimeRange next(void|TimeRange from,void|int(0..1) including)
   {
      if (!from) from=std_day();
      int jd;
      if (including) jd=(int)(from->julian_day());
      else jd=(int)(from->end()->julian_day());
      jd=scan_jd(from->calendar(),jd-nd+1,1);
      return (from->calendar()->Day)("julian_r",jd,from->ruleset())*nd;
   }

   TimeRange previous(void|TimeRange from,void|int(0..1) including)
   {
      if (!from) from=std_day();
      int jd;
      if (including) jd=(int)(from->end()->julian_day());
      else jd=(floatp(from->julian_day())
	       ?(int)floor(from->julian_day())
	       :(from->julian_day()-1));
      jd=scan_jd(from->calendar(),jd+nd-1,-1);
      if (jd==NODAY) return 0;
      return (from->calendar()->Day)("julian_r",jd,from->ruleset())*nd;
   }
}


//!	This is created by the @[Namedays] classes
//!	to represent an event for a name.
class Nameday
{
   inherit Day_Event;

   constant is_nameday=1;

   int jd;

   void create(string _name,int _jd)
   {
      name=_name;
      jd=_jd;
   }

   int scan_jd(Calendar realm,int sjd,int(1..1)|int(-1..-1) direction)
   {
      if (direction==1) return sjd<=jd?jd:NODAY;
      else return sjd>=jd?jd:NODAY;
   }

   string _sprintf(int t)
   {
      return t=='O'?sprintf("Nameday(%s:%O)",id,name):0;
   }
}

//!	This contains a ruleset about namedays. 
//! inherits Event
class Namedays
{
   inherit Event;
   constant is_namedays=1;

   int leapdayshift;
   int first_year=-1;
   int last_year=-1;

   array namelist;
   mapping lookup;

   void create(string _id,string _name,
	       array(array(string)) _names,
	       mapping(string:int|array(int)) _lookup,
	       void|int start,void|int stop,void|int _leapdayshift)
   {
      id=_id;
      name=_name;
      namelist=_names;
      first_year=start||-1;
      last_year=stop||-1;
      leapdayshift=_leapdayshift||2000;
      lookup=_lookup;
   }

   //!	Gives back an array of names that occur during
   //!	the time period, in no particular order.
   array(string) names(TimeRange t)
   {
// optimize this?
      return predef::`|(({}),@values(namedays(t)));
   }

   //!	Gives back an table of days with names that occur during
   //!	the time period. Note that days without names will not
   //!	appear in the returned mapping.
   mapping(TimeRange:array(string)) namedays(TimeRange t)
   {
      int jd=t->julian_day();
      mapping res=([]);
      function(mixed...:TimeRange) day=t->calendar()->Day;
      Ruleset rules=t->ruleset();
      [int y,int yjd,int leap]=gregorian_yjd(jd);
      if (first_year!=-1 && y<first_year)
	 [y,yjd,leap]=gregorian_year(first_year),jd=yjd;
      if (last_year!=-1 && y>last_year) return res;
      int ld;
      if (y<leapdayshift) ld=55-1; // 24 feb
      else ld=60-1;        // 29 feb

      for (;;)
      {
	 TimeRange td=day("julian_r",jd,rules);
	 if (!td->overlaps(t)) return res; 

	 if (jd>=yjd+365+leap)  // next year
	 {
	    [y,yjd,leap]=gregorian_yjd(jd);
	    if (last_year!=-1 && y>last_year) return res;
	    if (y<leapdayshift) ld=55-1; // 24 feb
	    else ld=60-1;        // 29 feb
	 }

	 array(string) n;
	 int d=jd-yjd;
	 if (leap)
	 { 
	    if (d<ld) n=namelist[d];
	    else if (d>ld) n=namelist[d-1];
	    else n=namelist[-1];
	 }
	 else
	    n=namelist[d];

	 if (n) res[td]=n;
	 jd++;
      }
   }

   mapping(TimeRange:Event) scan_events(TimeRange in)
   {
      mapping res=([]);
      foreach ((array)namedays(in),[TimeRange t,array(string) s])
	 res[t]=predef::`|(@map(s,Nameday,t->julian_day()));
      return res;
   }

   array(TimeRange) scan(TimeRange in)
   {
      return indices(namedays(in));
   }

   //! @decl TimeRange previous(TimeRange from,void|int(0..1) including)
   //! @decl TimeRange next(TimeRange from,void|int(0..1) including)

   static TimeRange _find(TimeRange t,int including,int direction)
   {
      int jd=(int)t->julian_day();

      jd+=direction*!including;

      [int y,int yjd,int leap]=gregorian_yjd(jd);
      int ld;
      if (y<leapdayshift) ld=55-1; // 24 feb
      else ld=60-1;        // 29 feb

      if (direction==-1 && last_year!=-1 && y>last_year) 
	 [y,yjd,leap]=gregorian_year(last_year+1),jd=yjd-1;
      if (direction==1 && first_year!=-1 && y<first_year)
	 [y,yjd,leap]=gregorian_year(first_year),jd=yjd;

      for (;;)
      {
	 if (jd>=yjd+365+leap || jd<yjd)  // year shift
	 {
	    [y,yjd,leap]=gregorian_yjd(jd);
	    if (y<leapdayshift) ld=55-1; // 24 feb
	    else ld=60-1;        // 29 feb
	    if (last_year!=-1 && y>last_year) return ([])[0];
	    if (first_year!=-1 && y<first_year) return ([])[0];
	 }

	 array(string) n;
	 int d=jd-yjd;
	 if (leap)
	 { 
	    if (d<ld) n=namelist[d];
	    else if (d>ld) n=namelist[d-1];
	    else n=namelist[-1];
	 }
	 else
	    n=namelist[d];

	 if (n) return t->calendar()->Day("julian_r",jd,t->ruleset());

	 jd+=direction;
      }
   }

   TimeRange next(TimeRange from,void|int(0..1) including)
   {
      return _find(from,including,1);
   }

   TimeRange previous(TimeRange from,void|int(0..1) including)
   {
      return _find(from,including,-1);
   }

   string _sprintf(int t)
   {
      return t=='O'?sprintf("Namedays(%s:%O)",id,name):0;
   }

   string describe()
   {
      return "Namedays";
   }

   SuperEvent|SuperNamedays|Namedays 
      `|(SuperEvent|Namedays|SuperNamedays e,
	 mixed ...extra)
   {
      object(SuperEvent)|object(SuperNamedays)|object(Namedays) res;
      if (e->is_nameday_wrapper && e->id==id && id!="?") 
	 res=SuperNamedays(e->namedays|({this_object()}),e->id);
      else
      {
	 array a=({e})|({this_object()});
	 if (!sizeof(a)) res=this_object();
	 else if (e->is_namedays && e->id==id) res=SuperNamedays(a,id);
	 else res=SuperEvent(a);
      }
      if (sizeof(extra)) return predef::`|(res,@extra);
      return res;
   }
}

class SuperNamedays
{
   inherit Event;
   constant is_namedays_wrapper=1;

// presumed non-overlapping namedays
   array(Nameday) namedayss;
   string id;
   
   void create(array(Nameday) _namedays,string _id)
   {
      name=id=_id;
      namedayss=_namedays;
   }

   string _sprintf(int t)
   {
      return t=='O'?sprintf("SuperNamedays(%s [%d])",id,sizeof(namedayss)):0;
   }

   string describe()
   {
      return "Namedays";
   }

   array(TimeRange) scan(TimeRange in)
   {
      return indices(namedays(in));
   }

   mapping(TimeRange:Event) scan_events(TimeRange in)
   {
      return predef::`|(@map(namedayss,"scan_events",in));
   }

   TimeRange next(TimeRange from,void|int(0..1) including)
   {
      array(TimeRange) a=map(namedayss,"next",from,including)-({0});
      switch (sizeof(a))
      {
	 case 0: return ([])[0];
	 case 1: return a[0];
	 default: return min(@a);
      }
   }

   TimeRange previous(TimeRange from,void|int(0..1) including)
   {
      array(TimeRange) a=map(namedayss,"previous",from,including)-({0});
      switch (sizeof(a))
      {
	 case 0: return ([])[0];
	 case 1: return a[0];
	 default: return max(@a);
      }
   }

   mapping(TimeRange:array(string)) namedays(TimeRange t)
   {
      return predef::`|(@map(namedayss,"namedays",t));
   }

   array(string) names(TimeRange t)
   {
      return predef::`|(@map(namedayss,"names",t));
   }

   SuperEvent|SuperNamedays|Namedays 
      `|(SuperEvent|Namedays|SuperNamedays e,
	 mixed ...extra)
   {
      if (e->is_namedays_wrapper)
	 return `|(this_object(),@e->namedayss,@extra);
      if (e->is_namedays && e->id==id)
	 return SuperNamedays(namedayss|({e}),id);
      return predef::`|(e,this_object(),@extra);
   }
}

// ----------------------------------------------------------------
// simple Gregorian date events
// ----------------------------------------------------------------

static array gregorian_yjd(int jd)
{
   int d=jd-1721426;

   int century=(4*d+3)/146097;
   int century_jd=(century*146097)/4;
   int century_day=d-century_jd;
   int century_year=(100*century_day+75)/36525;

   int y=century*100+century_year+1;

   return 
   ({
      y,
      1721426+century_year*365+century_year/4+century_jd,
      (!(((y)%4) || (!((y)%100) && ((y)%400))))
   });
}

static array gregorian_year(int y)
{
   return
   ({
      y,
      1721426+(y-1)*365+(y-1)/4-(y-1)/100+(y-1)/400,
      (!(((y)%4) || (!((y)%100) && ((y)%400))))
   });
}

static array julian_yjd(int jd)
{
   int d=jd-1721058;

   int quad=d/1461;
   int quad_year=max( (d%1461-1)/365, 0);

   int y=quad*4+quad_year;

   return 
   ({
      y,
      1721424+(y-1)*365+(y-1)/4,
      !(y%4),
   });
}

static array julian_year(int y)
{
   return
   ({
      y,
      1721424+(y-1)*365+(y-1)/4,
      !(y%4),
   });
}

// a set date of year, counting leap day in february -
// used for the gregorian fixed events in the events list
class Gregorian_Fixed
{
   inherit Day_Event;

   constant is_fixed=1;

   int md,mn;
   int yd;

   void create(string _id,string _name,
	       int(1..31) _md,int(1..12) _mn,int ... _n)
   {
      id=_id;
      name=_name;
      md=_md;
      mn=_mn;

      yd=M_YD[mn]+md;
      if (sizeof(_n)) nd=_n[0];
   }

   int scan_jd(Calendar realm,int jd,int(-1..1) direction)
   {
      [int y,int yjd,int leap]=gregorian_yjd(jd);
      int njd;

      for (;;)
      {
	 if (leap && yd>59) 
	    njd=yjd+yd;
	 else 
	    njd=yjd+yd-1; // yd start with 1

	 if (direction==1) 
	 {
	    if (njd>=jd) return njd;
	    [y,yjd,leap]=gregorian_year(y+1);
	 }
	 else
	 {
	    if (njd<=jd) return njd;
	    [y,yjd,leap]=gregorian_year(y-1);
	 }
      }
   }

   string describe()
   {
      return sprintf("%s %2d",M_NAME[mn],md);
   }
}

// same, but julian
class Julian_Fixed
{
   inherit Gregorian_Fixed;

   constant is_julian_fixed=1;

   int scan_jd(Calendar realm,int jd,int(-1..1) direction)
   {
      [int y,int yjd,int leap]=julian_yjd(jd);
      int njd;

      for (;;)
      {
	 if (leap && yd>59) 
	    njd=yjd+yd;
	 else 
	    njd=yjd+yd-1; // yd start with 1

	 if (direction==1) 
	 {
	    if (njd>=jd) return njd;
	    [y,yjd,leap]=julian_year(y+1);
	 }
	 else
	 {
	    if (njd<=jd) return njd;
	    [y,yjd,leap]=julian_year(y-1);
	 }
      }
   }

   string describe()
   {
      return sprintf("%s %2d julian",M_NAME[mn],md);
   }
}

//! 	This class represents the event of a given gregorian date.
//!	For instance, 
//!	<tt>Event.Date(12,10)->next(Day())</tt>
//!	finds the next 12 of October.
//!
class Date
{
   inherit Day_Event;

   int md,mn;
   
   int yd;

   //! @decl void create(int month_day, int month)
   //!	The event is created by a given month day and
   //!	a month number (1=January, 12=December).
   void create(int _md,int _mn)
   {
      md=_md;
      mn=_mn;
      name=M_NAME[mn]+" "+md;

      yd=M_YD[mn]+md;
   }

   int scan_jd(Calendar realm,int jd,int(-1..1) direction)
   {
      [int y,int yjd,int leap]=gregorian_yjd(jd);
      for (;;)
      {
	 int njd;

	 if (leap && yd>60) 
	    njd=yjd+yd;
	 else  
	    njd=yjd+yd-1; // yd start with 1
      
	 if (((direction==1)?njd>=jd:(njd<=jd)) && 
	     njd-yjd<M_ED[leap][mn]) return njd;

	 if (yd>M_ED[2][mn]) return NODAY;  // will never happen

	 if (direction==1) 
	    [y,yjd,leap]=gregorian_yjd(yjd+365+leap);
	 else
	    [y,yjd,leap]=gregorian_yjd(yjd-1);
      }
   }
}

//! 	This class represents the event that a given gregorian date appears
//!	a given weekday.
//!	For instance, 
//!	<tt>Event.Date_Weekday(12,10,5)->next(Day())</tt>
//!	finds the next 12 of October that is a friday.
//! 
class Date_Weekday
{
   inherit Day_Event;

   int md,mn;
   
   int yd;
   int jd_wd;

   //! @decl void create(int month_day,int month,int weekday)
   //!	The event is created by a given month day,
   //!	a month number (1=January, 12=December), and a 
   //!	weekday number (1=Monday, 7=Sunday).
   //!
   //! @note
   //!	The week day numbers used
   //!	are the same as the day of week in the @[ISO] calendar -
   //!	the @[Gregorian] calendar has 1=Sunday, 7=Saturday.
   void create(int _md,int _mn,int wd)
   {
      md=_md;
      mn=_mn;
      name=M_NAME[mn]+" "+md+" "+WD_NAME[wd];

      yd=M_YD[mn]+md;
      jd_wd=(wd+6)%7;
   }

   int scan_jd(Calendar realm,int jd,int(-1..1) direction)
   {
      if (md<1) return 0;
      [int y,int yjd,int leap]=gregorian_yjd(jd);
      for (;;)
      {
	 int njd;

	 if (leap && yd>60) 
	    njd=yjd+yd;
	 else 
	    njd=yjd+yd-1; // yd start with 1
      
	 if (jd_wd==njd%7 && 
	     ((direction==1)?njd>=jd:(njd<=jd)) &&
	     njd-yjd<M_ED[leap][mn]) return njd;

	 if (yd>M_ED[2][mn]) return NODAY;  // will never happen

	 if (direction==1) 
	    [y,yjd,leap]=gregorian_yjd(yjd+365+leap);
	 else
	    [y,yjd,leap]=gregorian_yjd(yjd-1);
      }
   }
}

//! 	This class represents the event that a given gregorian 
//!	day of month appears a given weekday.
//!	For instance, 
//!	@code{Event.Monthday_Weekday(13,5)->next(Day())@}
//!	finds the next friday the 13th.
//! 
class Monthday_Weekday
{
   inherit Day_Event;

   int md;
   int jd_wd;

   //! @decl void create(int month_day, int weekday)
   //!	The event is created by a given month day,
   //!	and a weekday number (1=Monday, 7=Sunday).
   //!
   //! @note
   //!	The week day numbers used
   //!	are the same as the day of week in the @[ISO] calendar -
   //!	the @[Gregorian] calendar has 1=Sunday, 7=Saturday.
   void create(int _md,int wd)
   {
      md=_md;
      name=md+","+WD_NAME[wd];
      jd_wd=(wd+6)%7;
   }

   int scan_jd(Calendar realm,int jd,int(-1..1) direction)
   {
      if (md>31 || md<1) return 0;
      [int y,int yjd,int leap]=gregorian_yjd(jd);
      for (;;)
      {
	 array z,w;

	 if (!leap) 
	    z=({0,31,59,90,120,151,181,212,243,273,304,334});
	 else 
	    z=({0,31,60,91,121,152,182,213,244,274,305,335});

	 z=map(z,`+,yjd,md,-1);
	 w=map(z,`%,7);

	 if (direction==1) 
	 {
	    foreach (enumerate(12),int i)
	       if (w[i]==jd_wd && z[i]>=jd &&
		   z[i]-yjd<M_ED[leap][i+1]) return z[i];
	    [y,yjd,leap]=gregorian_yjd(yjd+365+leap);
	 }
	 else
	 {
	    foreach (enumerate(12),int i)
	       if (w[i]==jd_wd && z[i]<=jd &&
		   z[i]-yjd<M_ED[leap][i+1]) return z[i];
	    [y,yjd,leap]=gregorian_yjd(yjd-1);
	 }
      }
   }
}

//! 	This class represents any given weekday.
//!	For instance, 
//!	<tt>Event.Weekday(5)->next(Day())</tt>
//!	finds the next friday.
//!
//!	These are also available as the pre-defined events
//!	"monday", "tuesday", "wednesday", "thursday", 
//!	"friday", "saturday" and "sunday".
//! 
class Weekday
{
   inherit Day_Event;
   constant is_weekday=1;

   int jd_wd;

   //! @decl void create(int weekday)
   //!	The event is created by a given 
   //!	weekday number (1=Monday, 7=Sunday).
   //!
   //! @note
   //!	The week day numbers used
   //!	are the same as the day of week in the @[ISO] calendar -
   //!	not the @[Gregorian] or @[Julian]
   //!	calendar that have 1=Sunday, 7=Saturday.
   void create(int wd,void|string _id)
   {
      jd_wd=(wd+6)%7; // convert to julian day numbering
      name=WD_NAME[wd];
      if (!id) id=name; else id=_id;
   }

   int scan_jd(Calendar realm,int jd,int(-1..1) direction)
   {
      if (direction==-1) return jd-(jd-jd_wd)%7;
      return jd+(7-(jd-jd_wd))%7;
   }
}


// Easter

class Easter
{
   inherit Day_Event;

// shift is the year to shift from old to new style

   int shift=1582;

   void create(void|int _shift)
   {
      if (_shift) shift=_shift;
   }

   int new_style(int y)
   {
      int century=y/100;
      int solar=century-century/4;
      int lunar=(century-15-(century-17)/25)/3;
      int epact=(13+11*(y%19)-solar+lunar)%30;
//  if (epact<0) epact+=30; // not neccesary for pike
      int new_moon=31-epact;
//        werror("epact: %O\n",epact);
//        werror("new_moon: %O\n",new_moon);
      if (new_moon<8)
	 if (epact==24 || epact==25 && (y%19)>10)
	    new_moon+=29;
	 else
	    new_moon+=30;
      int full_moon=new_moon+13;
      int week_day=(2+y+y/4-solar+full_moon)%7;
      return full_moon+7-week_day;
   }

   int old_style(int y)
   {
#if 1
      int new_moon=23-11*(y%19);
      while (new_moon<8) new_moon+=30;
      int full_moon=new_moon+13;
      int week_day=(y+y/4+full_moon)%7;
      return full_moon+7-week_day;
#else
      int g=y%19;
      int i=(19*g+15)%30;
      int j=(y+y/4+i)%7;
      int l=i-j;
      int m=3+(l+40)/44;
      int d=l+28-31*(m/4);
//        werror("y=%d m=%d d=%d l=%d\n",y,m,d,l);
      return l+28;
#endif
   }

   int easter_yd(int y,int yjd,int leap)
   {
      int z=(y<shift)?old_style(y):new_style(y);
      return `+(yjd,z,58,leap);
   }

   array(int) my_year(int y)
   {
      if (y<shift) return julian_year(y);
      return gregorian_year(y);
   }

   int scan_jd(Calendar realm,int jd,int(-1..1) direction)
   {
      int y,yjd,leap,ejd;

//        werror("scan %O %O\n",jd,direction);

      [y,yjd,leap]=gregorian_yjd(jd);
      if (y<shift)
	 [y,yjd,leap]=julian_yjd(jd);

      for (;;)
      {
	 ejd=easter_yd(y,yjd,leap);

//  	 werror("[%O %O %O] %O (%O)\n",y,yjd,leap,ejd,ejd-yjd+1);

	 if (direction==1) 
	 {
	    if (ejd>=jd) return ejd;
	    [y,yjd,leap]=my_year(y+1);
	 }
	 else
	 {
	    if (ejd<=jd) return ejd;
	    [y,yjd,leap]=my_year(y-1);
	 }
      }
   }
}

// Easter relative

class Easter_Relative
{
   inherit Easter;

   constant is_easter_relative=1;

   int offset;

   void create(string _id,string _name,void|int _offset)
   {
      id=_id;
      name=_name;
      offset=_offset;
   }

   int scan_jd(Calendar realm,int jd,int(-1..1) direction)
   {
      return offset+::scan_jd(realm,jd-direction*offset,direction);
   }

   string describe()
   {
      return sprintf("%seaster %+2d, %s",
		     shift>2000?"orthodox ":"",
		     offset,WD_NAME[(offset-1)%7+1]);
   }
}

// Orthodox Easter relative
// simple set shift year very high

class Orthodox_Easter_Relative
{
   inherit Easter_Relative;

   constant is_orthodox_easter_relative=1;

   int offset;

   void create(string _id,string _name,void|int _offset)
   {
      ::create(_id,_name,_offset);
      shift=9999999;
   }
}

// Monthday Weekday relative,
// n:th special weekday,
// "fourth sunday before 24 dec" => md=24,mn=12,wd=7,n=-4

class Monthday_Weekday_Relative
{
   inherit Gregorian_Fixed;

   constant is_fixed=0;
   constant is_monthday_weekday_relative=1;

   int offset;
   int wd;

   int n,inclusive;

   void create(string id,string name,int(1..31) md,int(1..12) mn,
	       int(1..7) _wd,int _n,void|int(0..1) _inclusive)
   {
      ::create(id,name,md,mn);

      n=_n;
      inclusive=_inclusive;

// offset is the offset to the last possible day
      if (n<0)
	 offset=(n+1)*7-!inclusive;
      else if (n>0)
	 offset=n*7-!!inclusive;
      else 
	 offset=3;

      wd=_wd;
   }

   int scan_jd(Calendar realm,int jd,int(-1..1) direction)
   {
      [int y,int yjd,int leap]=gregorian_yjd(jd-offset);
      for (;;)
      {
	 int njd;

	 int d=yd+offset;
	 if (leap && d>59) 
	    njd=(yjd+((d)-1)+leap-( (yjd+leap+((d)+(8-wd)-1)) % 7));
	 else 
	    njd=(yjd+((d)-1)-( (yjd+((d)+(8-wd)-1)) % 7));
      
	 if (direction==1) 
	 {
	    if (njd>=jd) return njd;
	    [y,yjd,leap]=gregorian_year(y+1);
	 }
	 else
	 {
	    if (njd<=jd) return njd;
	    [y,yjd,leap]=gregorian_year(y-1);
	 }
      }
   }

   string describe()
   {
      return sprintf("%s %2d %s %+2d %s [<=%+d]",
		     M_NAME[mn],md,WD_NAME[wd],n,
		     inclusive?"incl":"",offset);
   }
}

//!	This class holds any number of events,
//!	and adds the functionality of event flags.
//!
//! @note
//!	Scanning (scan_events,next,etc) will drop flag information.
//!	Dig out what you need with @tt{->@[holidays]@} et al first.
class SuperEvent
{
   inherit Event;
   
   constant is_superevent=1;
   string name="SuperEvent";
   
   mapping(Event:multiset(string)) flags=([]);

   array(Event) events=({});
   mapping(string:Event) id2event=([])[0];

   array(Event) day_events=({});
   array(Namedays) namedays=({});
   array(Event) other_events=({});


   static void create(array(Event) _events,
		      void|mapping(Event:multiset(string)) _flags,
		      void|string _id)
   {
      if (_id) id=_id;

      if (_flags) flags=_flags;

      foreach (_events,Event e)
	 if (e->is_superevent) 
	 {
	    events|=e->events;
	    if (flags[e] && flags[e]!=(<>))
	       foreach (e->events,Event e2)
		  flags[e2]=flags[e]|(e->flags[e2]||(<>));
	    else
	       flags|=e->flags;

	    m_delete(flags,e);
	 }
	 else events|=({e});

      foreach (events,Event e)
	 if (e->is_day_event) day_events+=({e});
	 else if (e->is_namedays) namedays+=({e});
	 else other_events+=({e});
   }

   //! @decl SuperEvent filter_flag(string flag)
   //! @decl SuperEvent holidays()
   //! @decl SuperEvent flagdays()
   //!	Filter out the events that has a certain flag set.
   //!	Holidays (flag "h") are the days that are marked
   //!	red in the calendar (non-working days),
   //!	Flagdays (flag "f") are the days that the flag
   //!	should be visible in (only some countries).

   SuperEvent filter_flag(string flag)
   {
      array res=({});
      multiset m;
      foreach (events,Event e)
	 if ((m=flags[e]) && m[flag]) res+=({e});
      return SuperEvent(res,flags&res,id+"!"+flag);
   }

   SuperEvent holidays() { return filter_flag("h"); }
   SuperEvent flagdays() { return filter_flag("f"); }

   mapping(TimeRange:Event) scan_events(TimeRange in)
   {
      mapping res=([]);
      foreach (events,Event e)
      {
	 mapping er=e->scan_events(in);
	 foreach ((array)er,[TimeRange t,Event e])
	    if (res[t]) res[t]|=e; // join 
	    else res[t]=e;
      }
      return res;
   }

   array(TimeRange) scan(TimeRange in)
   {
      return indices(scan_events(in));
   }

   TimeRange next(TimeRange from,void|int(0..1) including)
   {
      TimeRange best=0;
      foreach (events,Event e)
      {
	 TimeRange y=e->next(from,including);
	 if (!best || y->preceeds(best)) best=y;
	 else if (y->starts_with(best)) best|=y;
      }
      return best;
   }
    
   TimeRange previous(TimeRange from,void|int(0..1) including)
   {
      TimeRange best=0;
      foreach (events,Event e)
      {
	 TimeRange y=e->previous(from,including);
	 if (!best || best->preceeds(y)) best=y;
	 else if (y->ends_with(best)) best|=y;
      }
      return best;
   }

   Event `|(Event ... with)
   {
      with-=({0});
      return SuperEvent(events|with,flags,"?");
   }
   Event ``|(Event with) { return `|(with); }

   Event `-(Event ...subtract)
   {
      array(Event) res=events-subtract;
      if (res==events) return this_object();
      return SuperEvent(res,flags&res,"?");
   }

   array(Event) cast(string to)
   {
      if (to[..4]=="array")
	 return events;
      else
	 error("Can't cast to %O\n",to);
   }

   string _sprintf(int t)
   {
      return (t!='O')?0:
	 (sizeof(events)>5 
	  ? sprintf("SuperEvent(%s:%O,%O..%O [%d])",
		    id,events[0],events[1],events[-1],
		    sizeof(events))
	  : sprintf("SuperEvent(%s:%s)",
		    id,map(events,lambda(Event e) { return sprintf("%O",e); })*
		    ","));
   }

   function(string:Event) `-> = `[];
   Event `[](string s)
   {
      if (!id2event) id2event=mkmapping(events->id,events);
      return 
	 ::`[](s) ||
	 id2event[id+"/"+s] || id2event[s] ||
	 master()->resolv("Calendar")["Events"][id+"/"+s];
   }

   array(string) _indices()
   {
      if (!id2event) id2event=mkmapping(events->id,events);
      return indices(id2event);
   }

   array(Event) _values()
   {
      if (!id2event) id2event=mkmapping(events->id,events);
      return values(id2event);
   }
}

class TZShift_Event
{
   inherit Event;

   constant is_tzshift_event=1;

   Ruleset.Timezone timezone;

   void create(void|Ruleset.Timezone _tz)
   {
      timezone=_tz;
   }

   TimeRange next(void|TimeRange from,void|int(0..1) including)
   {
      if (!from) from=std_second();
      return scan_shift(timezone||from->timezone(),
			from,1,including);
   }
   TimeRange previous(void|TimeRange from,void|int(0..1) including)
   {
      if (!from) from=std_second();
      return scan_shift(timezone||from->timezone(),
			from,-1,including);
   }

   static TimeRange scan_shift(Ruleset.Timezone tz,
			       TimeRange from,
			       int direction,int including)
   {
      if (tz->whatrule)
	 return scan_history(tz,from,direction,including);
      return scan_rule(tz,from,direction,including);
   }

   static TimeRange scan_history(Ruleset.Timezone tz,
				 TimeRange from,
				 int direction,int(0..1) including)
   {
      int ux;

      if ((direction==1) ^ (!!including))
	 ux=(from=from->end())->unix_time();
      else
	 ux=from->unix_time();
      
      int nextshift=-1;
      if (direction==1)
      {
	 foreach (tz->shifts,int z)
	    if (z>=ux) { nextshift=z; break; }
      }
      else
	 foreach (reverse(tz->shifts),int z)
	    if (z<=ux) { nextshift=z; break; }

      TimeRange btr=0;
      if (nextshift!=-1)
	 btr=from->calendar()->Second("unix_r",nextshift,from->ruleset());
      
      TimeRange atr=from;
      for (;;)
      {
	 Ruleset.Timezone atz=tz->whatrule(ux);
	 atr=scan_rule(atz,atr,direction,including);
	 if (!atr) break;
	 if (direction==1)
	    { if (atr>=from) break; }
	 else
	    if (atr<=from) break;
      }

      if (!btr)	
	 return atr;
      if (!atr) 
	 return btr;
      if ( (direction==1)^(btr>atr) )
	 return btr; 
      else
	 return atr;
   }

   static TimeRange scan_rule(Ruleset.Timezone tz,
			      TimeRange from,
			      int direction,int including)
   {
      if (!tz->jd_year_periods)
	 return 0; // non-shifting timezone

      int jd;
      if ((direction==1) ^ (!!including))
	 jd=(int)(from=from->end())->julian_day();
      else
	 jd=(int)from->julian_day();
      [int y,int yjd,int leap_year]=gregorian_yjd(jd);

      for (;;)
      {
	 array(array) per=tz->jd_year_periods(yjd+1);
	 if (sizeof(per)==1) // no shifts this year
	 {
            // sanity check, are we leaving all shifts behind?
	    if (direction==-1 && y<tz->firstyear) return 0;
	    if (direction==1 && y>tz->lastyear) return 0;
	 }
	 if (direction==1)
	 {
	    foreach (per[1..],array shift)
	       if (shift[0]>=jd)
	       {
		  TimeRange atr=from->calendar()
		     ->Second("unix_r",(shift[0]-2440588)*86400+shift[1],
			      from->ruleset());
		  if (atr>=from) return atr;
	       }
	 }
	 else
	 {
	    foreach (reverse(per[1..]),array shift)
	       if (shift[0]<=jd)
	       {
		  TimeRange atr=from->calendar()
		     ->Second("unix_r",(shift[0]-2440588)*86400+shift[1],
			      from->ruleset());
		  if (atr<=from) return atr;
	       }
	 }
	 [y,yjd,leap_year]=gregorian_year(y+direction);
      }

      return 0;
   }
}
