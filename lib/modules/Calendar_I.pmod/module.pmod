
//!   This module exist only for backwards compatibility issues with
//!   earlier Pike releases. Use the Calendar module instead.
//!
//!   This code can be used to simulate the old calendar
//!   for now (it might be removed in later Pike's):
//!
//!    This module has been totally rewritten in Pike 7.1+. 
//!    To be forward compatible the lazy way, you can do
//!    something like this, though:
//!
//!   @code{
//!   #if constant(Calendar.II)
//!   #define Calendar Calendar_I
//!   #endif
//!   @i{...@} import Calendar @i{or whatever ...@}
//!   @}
//!
//!   This module implements calendar calculations, and base classes
//!   for time units. 
//!

#define error(X) throw(({(X),backtrace()}))

//! class time_unit
//!
class _TimeUnit
{
   object this=this_object();

   //!	Gives a list of methods to get lesser (shorter) time units.
   //!	ie, for a month, this gives back @code{({"day"})@}
   //!	and the method @code{day(mixed n)@} gives back that 
   //!	day object. The method @code{days()@} gives back a
   //!	list of possible argument values to the method @tt{day@}.
   //!	Concurrently, @code{Array.map(o->days(),o->day)@} gives
   //!	a list of day objects in the object @tt{o@}.
   //!
   //!	
   //!	Ie:@tt{
   //!	  array(string) lesser()    - gives back a list of possible xxx's.
   //!	  object xxxs()             - gives back a list of possible n's.
   //!	  object xxx(mixed n)       - gives back xxx n
   //!	  object xxx(object(Xxx) o) - gives back the corresponing xxx 
   //!	@}
   //!
   //!	The list of n's (as returned from xxxs) are always in order.
   //!
   //!	There are two n's with special meaning, 0 and -1.
   //!	0 always gives the first xxx, equal to 
   //!	@code{my_obj->xxx(my_obj->xxxs()[0])@}, and -1 gives the last,
   //!	equal to @code{my_obj->xxx(my_obj->xxxs()[-1])@}.
   //!
   //!	To get all xxxs in the object, do something like
   //!	@code{Array.map(my_obj->xxxs(),my_obj->xxx)@}.
   //!
   //!	xxx(object) may return zero, if there was no correspondning xxx.
   //!
   array(string) lesser() { return ({}); }

   //!	Gives a list of methods to get greater (longer) time units 
   //!	from this object. For a month, this gives back @code{({"year"})@},
   //!	thus the method @code{month->year()@} gives the year object.
   //!
   array(string) greater() { return ({}); }

   int `<(object x) { error("'< undefined\n"); }
   int `>(object x) { error("'> undefined\n"); }
   int `==(object x) { error("'== undefined\n"); }

   //! @decl object next()
   //! @decl object prev()
   //! @decl object `+(int n)
   //! @decl object `-(int n)
   //! @decl object `-(object x)
   //!	@[next()] and @[prev()] give the logical next and previous object.
   //!	The @[`+()] operator gives that logical relative object,
   //!	ie @code{my_day+14@} gives 14 days ahead.
   //!     @[`-()] works the same way, but can also take an object
   //!	of the same type and give the difference as an integer.

   object `+(int n) { error("`+ undefined\n"); }
   object|int `-(int n) { error("`- undefined\n"); }

   object next() { return this+1; }
   object prev() { return this+(-1); }
}





string print_month(void|object month,void|mapping options)
{
   object w;
   object today;
   string res="";

   if (!month)  // resolv thing here is to avoid compile-time resolve
      month=master()->resolv("Calendar_I")["Gregorian"]["Month"]();

   options=(["mark_today":1,
	     "week_space":3,
	     "weeks":1,
	     "days":1,
	     "title":1,
	     "notes":([])]) | (options || ([]));
   options=(["date_space":(options->mark_today?3:2)])|options;


   today=function_object(object_program(month))->Day();

   w=month->day(1)->week();

   res="";
   if (options->weeks) res+=" "+" "*options->week_space;
   if (options->title)
      res+=sprintf("%|*s\n",
		   (1+options->date_space)*sizeof(w->days())-1,
		   intp(options->title)
		   ?(String.capitalize(month->name()+" ")
		     +month->year()->name())
		   :options->title);

   if (options->days)
   {
      if (options->weeks) res+=" "+" "*options->week_space;
      foreach (Array.map(w->days(),w->day)->week_day_name(),string n)
	 res+=sprintf("%*s ",options->date_space,n[0..options->date_space-1]);
      res+="\n";
   }

   string daynotes="";

   if (sizeof(Array.filter(
      Array.map(month->days(),month->day),
      lambda(object d) { return !!options->notes[d]; })))
      daynotes="\n%-|"+options->date_space+"s";

   do
   {
      array a;
      array b;
      object d;
      string format="";

      a=Array.map(Array.map(w->days(),w->day),
		  lambda(object d) 
		  { if (d->month()!=month) return 0; else return d; });

      if (options->weeks)
	 res+=sprintf("%*s ",options->week_space,w->name());
      foreach (a,d)
	 if (d)
	    if (!options->mark_today || d!=today) 
	       res+=sprintf("%* |d ",
			    options->date_space,
			    d->month_day());
	    else 
	       res+=sprintf(">%* |d<",
			    options->date_space-1,
			    d->month_day());
	 else res+=" "+" "*options->date_space;

      if (options->notes[w])
	 res+=sprintf("%-=*s",
		      options->week_note_column_width,
		      options->notes[w]);

      res+="\n";

      if (daynotes)
      {
	 
      }

      w++;
   }
   while (w->day(0)->month()==month);

   return res;
}
