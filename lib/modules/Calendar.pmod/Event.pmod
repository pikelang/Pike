/* -*- mode: Pike; c-basic-offset: 3; -*- */
#pike __REAL_VERSION__

protected constant M_YD=({0,0,31,59,90,120,151,181,212,243,273,304,334});
protected constant M_ED=({({0,31,59,90,120,151,181,212,243,273,304,334,365}),
		       ({0,31,60,91,121,152,182,213,244,274,305,335,366}),
		       ({0,31,60,90,120,151,181,212,243,273,304,334,365}) });
protected constant M_NAME="---JanFebMarAprMayJunJulAugSepOctNovDec"/3;
protected constant WD_NAME="---MonTueWedThuFriSatSun"/3;

protected function(mixed...:Calendar.TimeRanges.TimeRange) std_day=
  Calendar.Day;
protected function(mixed...:Calendar.TimeRanges.TimeRange) std_second=
  Calendar.Second;

// ----------------------------------------------------------------
// base classes
// ----------------------------------------------------------------


//! Event is an abstract class, defining what methods an Event
//! need to have.
class Event
{
   string name;
   string id="?";

  //! This constant may be used to identify an event object.
   constant is_event=1;

  //! This calculates the next or previous occurance of the event,
  //! from the given timerange's start, including any event occuring
  //! at the start if that flag is set.
  //!
  //! It returns zero if there is no next event.
  //!
  //! These methods are virtual in the base class.
  Calendar.TimeRanges.TimeRange next(void|Calendar.TimeRanges.TimeRange from,
				     void|int(0..1) including);
  Calendar.TimeRanges.TimeRange previous(void|Calendar.TimeRanges.TimeRange from,
					 void|int(0..1) including);

  //! This calculates the eventual events that is contained or
  //! overlapped by the given timerange. @[scan] uses @[next], if not
  //! overloaded.
  //!
  //! @example
  //!   Calendar.Event.Easter()->scan(Calendar.Year(2000))
  //!     => ({ Day(Sun 23 Apr 2000) })
  //!
  //! @note
  //!   @[scan] can return an array of overlapping timeranges.
  //!
  //!   This method must use @tt{in->calendar_object->@}@i{type@}
  //!   to create the returned timeranges, and must keep the ruleset.
  array(Calendar.TimeRanges.TimeRange) scan(Calendar.TimeRanges.TimeRange in)
  {
      array res=({});
      Calendar.TimeRanges.TimeRange t=next(in,1);
      for (;;)
      {
	 if (!t || !t->overlaps(in)) return res;
	 res+=({t});
	 t=next(t);
      }
  }

  //! Returns a mapping with time ranges mapped to events.
  mapping(Calendar.TimeRanges.TimeRange:Event)
    scan_events(Calendar.TimeRanges.TimeRange in)
  {
    array r=scan(in);
    return mkmapping(r,allocate(sizeof(r),this));
  }

  //! Joins several events into one @[SuperEvent].
   protected SuperEvent `|(Event ... with)
   {
      with-=({0});
      with|=({this});
      if (sizeof(with)==1) return with[0];
      return SuperEvent(with);
   }
   protected SuperEvent ``|(Event with) { return `|(with); }

   protected string _sprintf(int t)
   {
      return (t!='O')?0:sprintf("Event(%s:%O)",id,name);
   }

   protected array(Event) cast(string to)
   {
     if (to=="array")
       return ({this});
     return UNDEFINED;
   }

  //! Returns a description of the event.
   string describe()
   {
      return "Unknown event";
   }
}

//! A non-event.
class NullEvent
{
   inherit Event;

  //! This constant may be used to identify a NullEvent.
   constant is_nullevent=1;

   protected void create(string _id,string s,mixed ...args)
   {
      id=_id;
      name=s;
   }

    object(Calendar.TimeRanges.TimeRange)|zero
      next(void|Calendar.TimeRanges.TimeRange from,
           void|int(0..1) including)
   {
      return 0;
   }

   object(Calendar.TimeRanges.TimeRange)|zero
     previous(void|Calendar.TimeRanges.TimeRange from,
              void|int(0..1) including)
   {
      return 0;
   }
}

//! @[Day_Event] is an abstract class, extending @[Event] for events
//! that are single days, using julian day numbers for the calculations.
class Day_Event
{
   inherit Event;

  //! This constant may be used to identify @[Day_Event] objects.
   constant is_day_event=1;

  //! Returned from @[scan_jd] if the even searched for did not
  //! exist.
   constant NODAY=-1;

   int nd=1;

  //! @decl int scan_jd(Calendar.Calendar realm, int jd,@
  //!                   int(-1..-1)|int(1..1) direction)
  //! This method has to be defined, and is what
  //! really does some work.
  //!
  //! @param direction
  //!   @int
  //!     @value 1
  //!       Forward (next),
  //!     @value -1
  //!	    Backward (previous).
  //!   @endint
  //!
  //! @returns
  //!   It should return the next or previous
  //!   julian day (>@i{jd@}) when the event occurs,
  //!   or the constant @[NODAY] if it doesn't.

   int scan_jd(Calendar.Calendar realm,int jd,int(1..1)|int(-1..-1) direction);

  //! Uses the virtual method @[scan_jd].
  //! @seealso
  //!   @[Event.next]
   Calendar.TimeRanges.TimeRange
       next(Calendar.TimeRanges.TimeRange from = std_day(),
            void|int(0..1) including)
   {
      int jd;
      if (including) jd=(int)(from->julian_day());
      else jd=(int)(from->end()->julian_day());
      jd=scan_jd(from->calendar(),jd-nd+1,1);
      return (from->calendar()->Day)("julian_r",jd,from->ruleset())*nd;
   }

  //! Uses the virtual method @[scan_jd].
  //! @seealso
  //!   @[Event.previous]
   object(Calendar.TimeRanges.TimeRange)|zero
       previous(Calendar.TimeRanges.TimeRange from = std_day(),
                void|int(0..1) including)
   {
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


//! This is created by the @[Namedays] classes
//! to represent an event for a name.
class Nameday
{
   inherit Day_Event;

  //! This constant may be used to identify @[Nameday] objects.
   constant is_nameday=1;

   int jd;

   protected void create(string _name,int _jd)
   {
      name=_name;
      jd=_jd;
   }

   int scan_jd(Calendar.Calendar realm,int sjd,int(1..1)|int(-1..-1) direction)
   {
      if (direction==1) return sjd<=jd?jd:NODAY;
      else return sjd>=jd?jd:NODAY;
   }

   protected string _sprintf(int t)
   {
      return t=='O'?sprintf("Nameday(%s:%O)",id,name):0;
   }
}

//! This contains a ruleset about namedays.
class Namedays
{
   inherit Event;

  //! This constant may be used to identify @[Namedays].
   constant is_namedays=1;

   int leapdayshift;
   int first_year=-1;
   int last_year=-1;

   array namelist;
   mapping|zero lookup;	// Unused???

   protected void create(string _id,string _name,
			 array(array(string)) _names,
			 mapping(string:int|array(int))|zero _lookup,
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

  //! Gives back an array of names that occur during
  //! the time period, in no particular order.
   array(string) names(Calendar.TimeRanges.TimeRange t)
   {
     // optimize this?
      return predef::`|(({}),@values(namedays(t)));
   }

  //! Gives back an table of days with names that occur during
  //! the time period. Note that days without names will not
  //! appear in the returned mapping.
   mapping(Calendar.TimeRanges.TimeRange:array(string))
     namedays(Calendar.TimeRanges.TimeRange t)
   {
      int jd=t->julian_day();
      mapping res=([]);
      function(mixed...:Calendar.TimeRanges.TimeRange) day=t->calendar()->Day;
      Calendar.Ruleset rules=t->ruleset();
      [int y,int yjd,int leap]=gregorian_yjd(jd);
      if (first_year!=-1 && y<first_year)
	 [y,yjd,leap]=gregorian_year(first_year),jd=yjd;
      if (last_year!=-1 && y>last_year) return res;
      int ld;
      if (y<leapdayshift) ld=55-1; // 24 feb
      else ld=60-1;        // 29 feb

      for (;;)
      {
	 Calendar.TimeRanges.TimeRange td=day("julian_r",jd,rules);
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

   mapping(Calendar.TimeRanges.TimeRange:Event)
     scan_events(Calendar.TimeRanges.TimeRange in)
   {
      mapping res=([]);
      foreach ((array)namedays(in),
	       [Calendar.TimeRanges.TimeRange t,array(string) s])
	 res[t]=predef::`|(@map(s,Nameday,t->julian_day()));
      return res;
   }

   array(Calendar.TimeRanges.TimeRange) scan(Calendar.TimeRanges.TimeRange in)
   {
      return indices(namedays(in));
   }

   protected Calendar.TimeRanges.TimeRange _find(Calendar.TimeRanges.TimeRange t,
					      int including, int direction)
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
	    if (last_year!=-1 && y>last_year) return UNDEFINED;
	    if (first_year!=-1 && y<first_year) return UNDEFINED;
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

   Calendar.TimeRanges.TimeRange next(Calendar.TimeRanges.TimeRange from,
				      void|int(0..1) including)
   {
      return _find(from,including,1);
   }

   Calendar.TimeRanges.TimeRange previous(Calendar.TimeRanges.TimeRange from,
					  void|int(0..1) including)
   {
      return _find(from,including,-1);
   }

   protected string _sprintf(int t)
   {
      return t=='O'?sprintf("Namedays(%s:%O)",id,name):0;
   }

   string describe()
   {
      return "Namedays";
   }

   protected SuperEvent|SuperNamedays|Namedays
      `|(SuperEvent|Namedays|SuperNamedays e,
	 mixed ...extra)
   {
      object(SuperEvent)|object(SuperNamedays)|object(Namedays) res;
      if (e->is_nameday_wrapper && e->id==id && id!="?")
	 res=SuperNamedays(e->namedays|({this}),e->id);
      else
      {
	 array a=({e})|({this});
	 if (!sizeof(a)) res=this;
	 else if (e->is_namedays && e->id==id) res=SuperNamedays(a,id);
	 else res=SuperEvent(a);
      }
      if (sizeof(extra)) return predef::`|(res,@extra);
      return res;
   }
}

//! Container for merged @[Namedays] objects. Presumes non-overlapping
//! namedays
class SuperNamedays (array(Nameday) namedayss, string id)
{
   inherit Event;
   constant is_namedays_wrapper=1;

   protected string _sprintf(int t)
   {
      return t=='O' && sprintf("SuperNamedays(%s [%d])",id,sizeof(namedayss));
   }

   string describe()
   {
      return "Namedays";
   }

   array(Calendar.TimeRanges.TimeRange) scan(Calendar.TimeRanges.TimeRange in)
   {
      return indices(namedays(in));
   }

   mapping(Calendar.TimeRanges.TimeRange:Event)
     scan_events(Calendar.TimeRanges.TimeRange in)
   {
      return predef::`|(@map(namedayss,"scan_events",in));
   }

   Calendar.TimeRanges.TimeRange next(Calendar.TimeRanges.TimeRange from,
				      void|int(0..1) including)
   {
      array(Calendar.TimeRanges.TimeRange) a=map(namedayss,"next",
						 from,including)-({0});
      switch (sizeof(a))
      {
	 case 0: return UNDEFINED;
	 case 1: return a[0];
	 default: return min(@a);
      }
   }

   Calendar.TimeRanges.TimeRange previous(Calendar.TimeRanges.TimeRange from,
					  void|int(0..1) including)
   {
      array(Calendar.TimeRanges.TimeRange) a=map(namedayss,"previous",
					 from,including)-({0});
      switch (sizeof(a))
      {
	 case 0: return UNDEFINED;
	 case 1: return a[0];
	 default: return max(@a);
      }
   }

   mapping(Calendar.TimeRanges.TimeRange:array(string))
     namedays(Calendar.TimeRanges.TimeRange t)
   {
      return predef::`|(@map(namedayss,"namedays",t));
   }

   array(string) names(Calendar.TimeRanges.TimeRange t)
   {
      return predef::`|(@map(namedayss,"names",t));
   }

   protected SuperEvent|SuperNamedays|Namedays
      `|(SuperEvent|Namedays|SuperNamedays e,
	 mixed ...extra)
   {
      if (e->is_namedays_wrapper)
	 return `|(this,@e->namedayss,@extra);
      if (e->is_namedays && e->id==id)
	 return SuperNamedays(namedayss|({e}),id);
      return predef::`|(e,this,@extra);
   }
}

// ----------------------------------------------------------------
// simple Gregorian date events
// ----------------------------------------------------------------

protected array gregorian_yjd(int jd)
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

protected array gregorian_year(int y)
{
   return
   ({
      y,
      1721426+(y-1)*365+(y-1)/4-(y-1)/100+(y-1)/400,
      (!(((y)%4) || (!((y)%100) && ((y)%400))))
   });
}

protected array julian_yjd(int jd)
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

protected array julian_year(int y)
{
   return
   ({
      y,
      1721424+(y-1)*365+(y-1)/4,
      !(y%4),
   });
}

//! A set date of year, counting leap day in February,
//! used for the Gregorian fixed events in the events list.
//! @seealso
//!   @[Julian_Fixed]
class Gregorian_Fixed
{
   inherit Day_Event;

  //! This constant may be used to identify @[Gregorian_Fixed] objects.
   constant is_fixed=1;

   int md,mn;
   int yd;

  //! @decl void create(string id, string name, int(1..31) month_day,@
  //!                   int(1..12) month, int extra)
   protected void create(string id, string name,
                         int(1..31) month_day, int(1..12) month,
                         int ... extra)
   {
     this::id=id;
     this::name=name;
     md=month_day;
     mn=month;

      yd=M_YD[mn]+md;
      if (sizeof(extra)) nd=extra[0];
   }

   int scan_jd(Calendar.Calendar realm,int jd,int(-1..1) direction)
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

//! A set date of year, counting leap day in February,
//! used for the Gregorian fixed events in the events list.
//! @seealso
//!   @[Gregorian_Fixed]
class Julian_Fixed
{
   inherit Gregorian_Fixed;

  //! This constant may be used to identify @[Julian_Fixed] objects.
   constant is_julian_fixed=1;

   int scan_jd(Calendar.Calendar realm,int jd,int(-1..1) direction)
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

//! This class represents the event of a given gregorian date. For instance,
//! @tt{Event.Date(12,10)->next(Day())@} finds the next 12 of October.
class Date
{
   inherit Day_Event;

   int md,mn;

   int yd;

  //! @decl void create(int(1..31) month_day, int(1..12) month)
  //! The event is created by a given month day and a month number
  //! (1=January, 12=December).
   protected void create(int _md,int _mn)
   {
     md=_md;
      mn=_mn;
      name=M_NAME[mn]+" "+md;

      yd=M_YD[mn]+md;
   }

   int scan_jd(Calendar.Calendar realm,int jd,int(-1..1) direction)
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

//! This class represents the event that a given gregorian date appears
//! a given weekday. For instance,
//! @tt{Event.Date_Weekday(12,10,5)->next(Day())@} finds the next 12 of
//! October that is a friday.
class Date_Weekday
{
   inherit Day_Event;

   int md,mn;

   int yd;
   int jd_wd;

  //! @decl void create(int month_day, int month, int weekday)
  //! The event is created by a given month day,
  //! a month number (1=January, 12=December), and a
  //! weekday number (1=Monday, 7=Sunday).
  //!
  //! @note
  //! The week day numbers used are the same as the day of week in
  //! the @[ISO] calendar - the @[Gregorian] calendar has 1=Sunday,
  //! 7=Saturday.
   protected void create(int _md,int _mn,int wd)
   {
      md=_md;
      mn=_mn;
      name=M_NAME[mn]+" "+md+" "+WD_NAME[wd];

      yd=M_YD[mn]+md;
      jd_wd=(wd+6)%7;
   }

   int scan_jd(Calendar.Calendar realm,int jd,int(-1..1) direction)
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

//! This class represents the event that a given gregorian
//! day of month appears a given weekday. For instance,
//! @tt{Event.Monthday_Weekday(13,5)->next(Day())@} finds the next
//! friday the 13th.
class Monthday_Weekday
{
   inherit Day_Event;

   int md;
   int jd_wd;

  //! @decl void create(int month_day,int weekday)
  //! The event is created by a given month day,
  //! and a weekday number (1=Monday, 7=Sunday).
  //!
  //! @note
  //! The week day numbers used are the same as the day of week in
  //! the @[ISO] calendar - the @[Gregorian] calendar has 1=Sunday,
  //! 7=Saturday.
   protected void create(int _md,int wd)
   {
      md=_md;
      name=md+","+WD_NAME[wd];
      jd_wd=(wd+6)%7;
   }

   int scan_jd(Calendar.Calendar realm,int jd,int(-1..1) direction)
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

//! This class represents any given weekday. For instance,
//! @tt{Event.Weekday(5)->next(Day())@}	finds the next friday.
//!
//! These are also available as the pre-defined events @[Events.monday],
//! @[Events.tuesday], @[Events.wednesday], @[Events.thursday],
//! @[Events.friday], @[Events.saturday] and @[Events.sunday].
class Weekday
{
   inherit Day_Event;
   constant is_weekday=1;

   int jd_wd;

  //! @decl void create(int weekday, void|string id)
  //! The event is created by a given weekday number (1=Monday, 7=Sunday).
  //!
  //! @note
  //! The week day numbers used are the same as the day of week in
  //! the @[ISO] calendar - the @[Gregorian] calendar has 1=Sunday,
  //! 7=Saturday.
   protected void create(int wd,void|string _id)
   {
      jd_wd=(wd+6)%7; // convert to julian day numbering
      name=WD_NAME[wd];
      if (!id) id=name; else id=_id;
   }

   int scan_jd(Calendar.Calendar realm,int jd,int(-1..1) direction)
   {
      if (direction==-1) return jd-(jd-jd_wd)%7;
      return jd+(7-(jd-jd_wd))%7;
   }
}

//! This class represents a solar event as observed from Earth.
//!
//! The @[event_type] is one of
//! @int
//!   @value 0
//!     Northern hemisphere spring equinox.
//!   @value 1
//!     Northern hemisphere summer solstice.
//!   @value 2
//!     Northern hemisphere autumn equinox.
//!   @value 3
//!     Northern hemisphere winter solstice.
//! @endint
class Solar(int|void event_type)
{
  inherit Day_Event;

  //! @array
  //!   @elem array(float) 0..
  //!     @array
  //!       @elem float 0
  //!         Amplitude in days.
  //!       @elem float 1
  //!         Cosine phase in radians in year 2000.
  //!       @elem float 2
  //!         Period in radians/year.
  //!     @endarray
  //! @endarray
  protected constant periodic_table = ({
    ({ 0.00485, 5.67162,   33.757041, }),
    ({ 0.00203, 5.88577,  575.338485, }),
    ({ 0.00199, 5.97042,    0.352312, }),
    ({ 0.00182, 0.48607, 7771.377155, }),
    ({ 0.00156, 1.27653,  786.041946, }),
    ({ 0.00136, 2.99359,  393.020973, }),
    ({ 0.00077, 3.8841, 1150.67697, }),
    ({ 0.00074, 5.1787,   52.96910, }),
    ({ 0.00070, 4.2513,  157.73436, }),
    ({ 0.00058, 2.0911,  588.49268, }),
    ({ 0.00052, 5.1866,    2.62983, }),
    ({ 0.00050, 0.3669,   39.81490, }),
    ({ 0.00045, 4.3204,  522.36940, }),
    ({ 0.00044, 5.6749,  550.75533, }),
    ({ 0.00029, 1.0634,   77.55226, }),
    ({ 0.00018, 2.7074, 1179.06290, }),	// NB: Some have amplitude 28 here.
    ({ 0.00017, 5.0403,   79.62981, }),
    ({ 0.00016, 3.4565, 1097.70789, }),
    ({ 0.00014, 3.4865,  548.67778, }),
    ({ 0.00012, 1.6649,  254.43145, }),
    ({ 0.00012, 5.0110,  557.31428, }),
    ({ 0.00012, 5.5992,  606.97767, }),
    ({ 0.00009, 3.9746,   21.32991, }),
    ({ 0.00008, 0.2697,  294.24635, }),
  });

  //! Calculate the next event.
  //!
  //! Based on Meeus Astronomical Algorithms Chapter 27.
  array(int|float) solar_event(int y)
  {
    int jd;
    float offset;

    // First calculate an initial guess for the Julian day.
    if (y < 1000) {
      float yy = y/1000.0;
      float y2 = yy*yy;
      float y3 = y2*yy;
      float y4 = y3*yy;

      // 4th degree polynomial around year 2BC.
      switch (event_type) {
      case 0:
	offset = 365242 * yy;
	jd = 1721139 + (int)floor(offset);
	offset -= floor(offset);
	offset += 0.29189 + 0.13740 * yy + 0.06134 * y2 -
	  0.00111 * y3 - 0.00071 * y4;
	break;
      case 1:
	offset = 365241 * yy;
	jd = 1721233 + (int)floor(offset);
	offset -= floor(offset);
	offset += 0.25401 + 0.72562 * yy + 0.05323 * y2 -
	  0.00907 * y3 - 0.00025 * y4;
	break;
      case 2:
	offset = 365242 * yy;
	jd = 1721325 + (int)floor(offset);
	offset -= floor(offset);
	offset += 0.70455 + 0.49558 * yy + 0.11677 * y2 -
	  0.00297 * y3 - 0.00074 * y4;
	break;
      case 3:
	offset = 365242 * yy;
	jd = 1721414 + (int)floor(offset);
	offset -= floor(offset);
        offset += 0.39987 + 0.88257 * yy + 0.00769 * y2 -
	  0.00933 * y3 - 0.00006 * y4;
	break;
      }
    } else {
      float yy = (y - 2000)/1000.0;
      float y2 = yy*yy;
      float y3 = y2*yy;
      float y4 = y3*yy;

      // 4th degree polynomial around year 2000.
      switch (event_type) {
      case 0:
	offset = 365242 * yy;
	jd = 2451623 + (int)floor(offset);
	offset -= floor(offset);
	offset += 0.80984 + 0.37404 * yy + 0.05169 * y2 -
	  0.00411 * y3 - 0.00057 * y4;
	break;
      case 1:
	offset = 365241 * yy;
	jd = 2451716 + (int)floor(offset);
	offset -= floor(offset);
	offset += 0.56767 + 0.62603 * yy + 0.00325 * y2 -
	  0.00888 * y3 - 0.00030 * y4;
	break;
      case 2:
	offset = 365242 * yy;
	jd = 2451810 + (int)floor(offset);
	offset -= floor(offset);
	offset += 0.21715 + 0.01767 * yy + 0.11575 * y2 -
	  0.00337 * y3 - 0.00078 * y4;
	break;
      case 3:
	offset = 365242 * yy;
	jd = 2451900 + (int)floor(offset);
	offset -= floor(offset);
	offset += 0.05952 + 0.74049 * yy + 0.06223 * y2 -
	  0.00823 * y3 - 0.00032 * y4;
	break;
      }
    }

    float delta_y = ((jd - 2451545) + offset) / 36525.0;

    // Omega is in radians.
    float omega = 628.30759 * delta_y - 0.0431096;
    float l = 1.0 + 0.0334 * cos(omega) + 0.0007 * cos(2.0 * omega);

    // Adjusted to radians.
    float S = 0.0;
    foreach(periodic_table; int i; array(float) fun) {
      S += fun[0] * cos(fun[1] + fun[2] * delta_y);
    }

    offset += S / l;

    // Adjust for Meeus starting julian days at 12:00 UTC.
    offset += 0.5;

    jd += (int)floor(offset);
    offset -= (int)floor(offset);
    return ({ jd, offset });
  }

  //! @note
  //!   Returns unixtime in UTC to avoid losing the decimals!
  int scan_jd(Calendar.Calendar realm, int jd, int(1..1)|int(-1..-1) direction)
  {
    [int y, int yjd, int leap] = gregorian_yjd(jd);

    [int new_jd, float offset] = solar_event(y);

    if ((direction > 0) && (new_jd < jd)) {
      [new_jd, offset] = solar_event(y + 1);
    } else if ((direction < 0) && (new_jd >= jd)) {
      [new_jd, offset] = solar_event(y - 1);
    }

    // Convert into an UTC timestamp.
    int utc = (new_jd - 2440588)*86400 + (int)(offset * 86400.0);
    return utc - (int)round(.ISO.deltat(utc));
  }

   Calendar.TimeRanges.TimeRange
      next(Calendar.TimeRanges.TimeRange from = std_day(),
           void|int(0..1) including)
   {
      int jd;
      if (including) jd=(int)(from->julian_day());
      else jd=(int)(from->end()->julian_day());
      int utc = scan_jd(from->calendar(),jd-nd+1,1);
      return (from->calendar()->Day)("unix_r",utc,from->ruleset())*nd;
   }

   //! Uses the virtual method @[scan_jd].
   //! @seealso
   //!   @[Event.previous]
   Calendar.TimeRanges.TimeRange
      previous(Calendar.TimeRanges.TimeRange from = std_day(),
               void|int(0..1) including)
   {
      int jd;
      if (including) jd=(int)(from->end()->julian_day());
      else jd=(floatp(from->julian_day())
	       ?(int)floor(from->julian_day())
	       :(from->julian_day()-1));
      int utc = scan_jd(from->calendar(),jd+nd-1,-1);
      return (from->calendar()->Day)("unix_r",utc,from->ruleset())*nd;
   }
}

//! This class represents an easter.
class Easter
{
   inherit Day_Event;

   int shift=1582;

  //! @decl void create(void|int shift)
  //! @[shift] is the year to shift from old to new style easter
  //! calculation. Default is 1582.
   protected void create(void|int _shift)
   {
      if (_shift) shift=_shift;
   }

   protected int new_style(int y)
   {
      int century=y/100;
      int solar=century-century/4;
      int lunar=(century-15-(century-17)/25)/3;
      int epact=(13+11*(y%19)-solar+lunar)%30;
//  if (epact<0) epact+=30; // not necessary for pike
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

   protected int old_style(int y)
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

  //! Calculates the year day for the easter.
   int easter_yd(int y,int yjd,int leap)
   {
      int z=(y<shift)?old_style(y):new_style(y);
      return `+(yjd,z,58,leap);
   }

   protected array(int) my_year(int y)
   {
      if (y<shift) return julian_year(y);
      return gregorian_year(y);
   }

   int scan_jd(Calendar.Calendar realm,int jd,int(-1..1) direction)
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

//! This class represents an easter relative event.
class Easter_Relative
{
   inherit Easter;

   constant is_easter_relative=1;

   int offset;

  //! @decl void create(string id, string name, int offset)
   protected void create(string _id,string _name,void|int _offset)
   {
      id=_id;
      name=_name;
      offset=_offset;
   }

   int scan_jd(Calendar.Calendar realm,int jd,int(-1..1) direction)
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

//! This class represents an orthodox easter relative event.
class Orthodox_Easter_Relative
{
   inherit Easter_Relative;

   constant is_orthodox_easter_relative=1;

   int offset;

  //! @decl void create(string id, string name, int offset)
   protected void create(string _id,string _name,void|int _offset)
   {
      ::create(_id,_name,_offset);
      shift=9999999;
   }
}

//! This class represents a monthday weekday relative event or
//! n:th special weekday event, e.g.
//! "fourth sunday before 24 dec" => md=24,mn=12,wd=7,n=-4
class Monthday_Weekday_Relative
{
   inherit Gregorian_Fixed;

   constant is_fixed=0;
   constant is_monthday_weekday_relative=1;

   int offset;
   int wd;

   int n,inclusive;

  //!
   protected void create(string id,string name,int(1..31) md,int(1..12) mn,
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

   int scan_jd(Calendar.Calendar realm,int jd,int(-1..1) direction)
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

//! This class holds any number of events,
//! and adds the functionality of event flags.
//!
//! @note
//! Scanning (scan_events,next,etc) will drop flag information.
//! Dig out what you need with @[holidays] et al first.
class SuperEvent
{
   inherit Event;

   constant is_superevent=1;
   string name="SuperEvent";

   mapping(Event:multiset(string)) flags=([]);

   array(Event) events=({});
   mapping(string:Event)|zero id2event = UNDEFINED;

   array(Event) day_events=({});
   array(Namedays) namedays=({});
   array(Event) other_events=({});


   protected void create(array(Event) _events,
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

   mapping(Calendar.TimeRanges.TimeRange:Event)
     scan_events(Calendar.TimeRanges.TimeRange in)
   {
      mapping res=([]);
      foreach (events,Event e)
      {
	 mapping er=e->scan_events(in);
	 foreach ((array)er,[Calendar.TimeRanges.TimeRange t,Event e])
	    if (res[t]) res[t]|=e; // join
	    else res[t]=e;
      }
      return res;
   }

   array(Calendar.TimeRanges.TimeRange) scan(Calendar.TimeRanges.TimeRange in)
   {
      return indices(scan_events(in));
   }

   object(Calendar.TimeRanges.TimeRange)|zero
      next(Calendar.TimeRanges.TimeRange from, void|int(0..1) including)
   {
      object(Calendar.TimeRanges.TimeRange)|zero best = 0;
      foreach (events,Event e)
      {
	 Calendar.TimeRanges.TimeRange y=e->next(from,including);
	 if (!best || y->preceeds(best)) best=y;
	 else if (y->starts_with(best)) best|=y;
      }
      return best;
   }

   object(Calendar.TimeRanges.TimeRange)|zero
      previous(Calendar.TimeRanges.TimeRange from, void|int(0..1) including)
   {
      object(Calendar.TimeRanges.TimeRange)|zero best = 0;
      foreach (events,Event e)
      {
	 Calendar.TimeRanges.TimeRange y=e->previous(from,including);
	 if (!best || best->preceeds(y)) best=y;
	 else if (y->ends_with(best)) best|=y;
      }
      return best;
   }

   protected Event `|(Event|SuperEvent ... with)
   {
      with-=({0});
      return SuperEvent(events|with,flags,"?");
   }
   protected Event ``|(Event|SuperEvent with) { return `|(with); }

   protected Event `-(Event|SuperEvent ...subtract)
   {
      array(Event) res=events-subtract;
      if (res==events) return this;
      return SuperEvent(res,flags&res,"?");
   }

   protected array(Event) cast(string to)
   {
     if (to=="array")
       return events;
     return UNDEFINED;
   }

   protected string _sprintf(int t)
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

   protected Event `-> (string s) {return `[] (s);}
   protected Event `[](string s)
   {
      if (!id2event) id2event=mkmapping(events->id,events);
      return
	 ::`[](s) ||
	 id2event[id+"/"+s] || id2event[s] ||
	 master()->resolv("Calendar")["Events"][id+"/"+s];
   }

   protected array(string) _indices()
   {
      if (!id2event) id2event=mkmapping(events->id,events);
      return indices(id2event);
   }

   protected array(Event) _values()
   {
      if (!id2event) id2event=mkmapping(events->id,events);
      return values(id2event);
   }
}

//! Event containing information about when a timezone is changed.
class TZShift_Event
{
   inherit Event;

   constant is_tzshift_event=1;

   Calendar.Rule.Timezone timezone;

   protected void create(void|Calendar.Rule.Timezone _tz)
   {
      timezone=_tz;
   }

   Calendar.TimeRanges.TimeRange
      next(Calendar.TimeRanges.TimeRange from = std_second(),
           void|int(0..1) including)
   {
      return scan_shift(timezone||from->timezone(),
			from,1,including);
   }
   Calendar.TimeRanges.TimeRange
      previous(Calendar.TimeRanges.TimeRange from = std_second(),
               void|int(0..1) including)
   {
      return scan_shift(timezone||from->timezone(),
			from,-1,including);
   }

  //!
   protected Calendar.TimeRanges.TimeRange
     scan_shift(Calendar.Rule.Timezone tz,
		Calendar.TimeRanges.TimeRange from,
		int direction,int including)
   {
      if (tz->whatrule)
	 return scan_history(tz,from,direction,including);
      return scan_rule(tz,from,direction,including);
   }

  //!
   protected Calendar.TimeRanges.TimeRange
     scan_history(Calendar.Rule.Timezone tz,
		  Calendar.TimeRanges.TimeRange from,
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

      object(Calendar.TimeRanges.TimeRange)|zero btr = 0;
      if (nextshift!=-1)
	 btr=from->calendar()->Second("unix_r",nextshift,from->ruleset());

      Calendar.TimeRanges.TimeRange atr=from;
      for (;;)
      {
	 Calendar.Rule.Timezone atz=tz->whatrule(ux);
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

  //!
   protected object(Calendar.TimeRanges.TimeRange)|zero
     scan_rule(Calendar.Rule.Timezone tz,
	       Calendar.TimeRanges.TimeRange from,
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
		  Calendar.TimeRanges.TimeRange atr=from->calendar()
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
		  Calendar.TimeRanges.TimeRange atr=from->calendar()
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
