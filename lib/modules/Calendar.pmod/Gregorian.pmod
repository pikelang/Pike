
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
//!   
//! submodule Gregorian
//!	time units:
//!	<ref>Year</ref>, <ref>Month</ref>, <ref>Week</ref>, <ref>Day</ref>
//!
//! class 

array(string) month_names=
   ({"January","February","Mars","April","May","June","July","August",
     "September","October","November","December"});

array(string) week_day_names=
   ({"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"});

/* calculated on need */

mapping week_day_mapping,month_mapping;

//== Year ====================================================================

class Year
{
//! class Year
//! 	A <ref>Calendar.time_unit</ref>. 
//!
//!	Lesser units: <ref>Month</ref>, <ref>Week</ref>, <ref>Day</ref>
//!	Greater units: none
//!
//!     

//-- variables ------------------------------------------------------

   object this=this_object();
   program vYear=function_object(object_program(this))->Year;
   program vDay=function_object(object_program(this))->Day;
   program vMonth=function_object(object_program(this))->Month;
   program vWeek=function_object(object_program(this))->Week;

   int y;

   array days_per_month;
   array month_start_day;

//-- standard methods -----------------------------------------------

   array(string) lesser() 
   { 
      return ({"month","week","day"});
   }

   array(string) greater()
   {
      return ({});
   }

   void create(int ... arg)
   {
      if (!sizeof(arg))
      {
	 mapping t=localtime(time());
	 y=1900+t->year;
      }
      else
	 y=arg[0];

      int l=this->leap();
      days_per_month=
	 ({-17, 31,28+l,31,30,31,30,31,31,30,31,30,31});

      month_start_day=allocate(sizeof(days_per_month));

      for (int i=1,l=0; i<sizeof(days_per_month); i++)
	 month_start_day[i]=l,l+=days_per_month[i];
   }

   int `<(object x)
   {
      return y<(int)x;
   }

   int `==(object x)
   {
      return 
	 object_program(x)==object_program(this) &&
	 (int)x==y;
   }

   int `>(object x)
   {
      return y>(int)x;
   }

   object next()
   {
      return vYear(y+1);
   }

   object prev()
   {
      return vYear(y-1);
   }

   object `+(int n)
   {
      return vYear(y+n);
   }

   int|object `-(int|object n)
   {
      if (objectp(n) && object_program(n)==vYear)  return y-n->y;
      return this+-n;
   }

//-- nonstandard methods --------------------------------------------

   int julian_day(int d) // jd%7 gives weekday, mon=0, sun=6
   {
      int a;
      a = (y-1)/100;
      return 1721426 + d - a + (a/4) + (36525*(y-1))/100;
   }

   int leap()
   {
      if (!(y%400)) return 1;
      if (!(y%100)) return 0;
      return !(y%4);
   }

   int number_of_weeks()
   {
      int j=julian_day(0)%7;

      // years that starts on a thursday has 53 weeks
      if (j==3) return 53; 
      // leap years that starts on a wednesday has 53 weeks
      if (j==2 && this->leap()) return 53;
      // other years has 52 weeks
      return 52;
   }

   int number_of_days()
   {
      return 365+this->leap();
   }

   int number()
   {
      return y;
   }

   string name()
   {
      if (y>0) return (string)y+" AD";
      else return (string)(-y+1)+" BC";
   }

   mixed cast(string what)
   {
      switch (what)
      {
	 case "int": return this->number(); 
	 case "string": return this->name();
	 default:
	    throw(({"can't cast to "+what+"\n",backtrace()}));
      }
   }

//-- less -----------------------------------------------------------

   object month(string|int n)
   {
      if (stringp(n))
      {
	 if (!week_day_mapping)
	    month_mapping=
	       mkmapping(Array.map(month_names,lower_case),
			 indices(allocate(13))[1..]);
	 n=month_mapping[n];
      }

      if (n<0)
	 return vMonth(y,13+n);
      else
	 return vMonth(y,n||1);
   }

   array(mixed) months()
   {
       return ({1,2,3,4,5,6,7,8,9,10,11,12});
   }

   object week(int n)
   {
      if (n<0)
	 return vWeek(y,this->number_of_weeks()+n+1);
      else
	 return vWeek(y,n||1);
   }

   array(mixed) weeks()
   {
       return indices(allocate(this->number_of_weeks()+1))[1..];
   }

   object day(int n)
   {
      if (n<0)
	 return vDay(y,this->number_of_days()+n);
      else
	 return vDay(y,n);
   }

   array(mixed) days()
   {
       return indices(allocate(this->number_of_days()));
   }

//-- more -----------------------------------------------------------
     
   // none
};


//

//== Month ===================================================================

class Month
{
//-- variables ------------------------------------------------------

   object this=this_object();
   program vYear=function_object(object_program(this))->Year;
   program vDay=function_object(object_program(this))->Day;
   program vMonth=function_object(object_program(this))->Month;
   program vWeek=function_object(object_program(this))->Week;

   int y;
   int m;

//-- standard methods -----------------------------------------------

   array(string) lesser() 
   { 
      return ({"day"});
   }

   array(string) greater()
   {
      return ({"year"});
   }

   void create(int ... arg)
   {
      if (!sizeof(arg))
      {
	 mapping t=localtime(time());
	 y=1900+t->year;
	 m=t->mon+1;
      }
      else
      {
	 y=arg[0];
	 m=arg[1];
      }
   }
      
   int `<(object x)
   {
      return 
	 (object_program(x)==object_program(this) &&
	  (x->y==y && x->m<m) || (x->y<y));
   }

   int `==(object x)
   {
      return 
	 objectp(x) &&
	 object_program(x)==object_program(this) &&
	 x->y==y && x->m==m;
   }

   int `!=(object x)
   {
      write("foo\n");
      return !(this==x);
   }

   int `>(object x)
   {
      return 
	 (object_program(x)==object_program(this) &&
	  (x->y==y && x->m>m) || (x->y>y));
   }

   object `+(int n)
   {
      int m2=m;
      int y2=y;
      m2+=n;
      while (m2>12) { y2++; m2-=12; }
      while (m2<1) { y2--; m2+=12; }
      return vMonth(y2,m2);
   }

   int|object `-(object|int n)
   {
      if (objectp(n) && object_program(n)==vMonth) 
	 return m-n->m+(y-n->y)*12;
      return this+(-n);
   }

   object next()
   {
      return this+1;
   }

   object prev()
   {
      return this-1;
   }

//-- internal -------------------------------------------------------

   int yday()
   {
      return this->year()->month_start_day[m];
   }

//-- nonstandard methods --------------------------------------------

   int number_of_days()
   {
      return this->year()->days_per_month[m];
   }

   int number()
   {
      return m;
   }

   string name()
   {
      return month_names[this->number()-1];
   }

   mixed cast(string what)
   {
      switch (what)
      {
	 case "int": return this->number(); 
	 case "string":  return this->name();
	 default:
	    throw(({"can't cast to "+what+"\n",backtrace()}));
      }
   }

//-- less -----------------------------------------------------------

   object day(int n)
   {
      if (n<0)
	 return vDay(y,yday()+this->number_of_days()+n);
      else
	 return vDay(y,yday()+(n||1)-1);
   }

   array(mixed) days()
   {
       return indices(allocate(this->number_of_days()+1))[1..];
   }

//-- more -----------------------------------------------------------
     
   object year()
   {
      return vYear(y);
   }
};


//
//== Week ====================================================================

class Week
{
//-- variables ------------------------------------------------------

   object this=this_object();
   program vYear=function_object(object_program(this))->Year;
   program vDay=function_object(object_program(this))->Day;
   program vMonth=function_object(object_program(this))->Month;
   program vWeek=function_object(object_program(this))->Week;

   int y;
   int w;

//-- standard methods -----------------------------------------------

   array(string) lesser() 
   { 
      return ({"day"});
   }

   array(string) greater()
   {
      return ({"year"});
   }

   void create(int ... arg)
   {
      if (!sizeof(arg))
      {
	 object wp=vDay()->week();
	 y=wp->y;
	 w=wp->w;
      }
      else
      {
	 y=arg[0];
	 w=arg[1];
      }
   }

   int `<(object x)
   {
      return 
	 (object_program(x)==object_program(this) &&
	  (x->y==y && x->w<w) || (x->y<y));
   }

   int `==(object x)
   {
      return 
	 object_program(x)==object_program(this) &&
	 x->y==y && x->w==w;
   }

   int `>(object x)
   {
      return 
	 (object_program(x)==object_program(this) &&
	  (x->y==y && x->w>w) || (x->y>y));
   }

   object `+(int n)
   {
      int y2=y,nd;
      int w2=w;
      w2+=n;
      
      if (w2>0)
      {
	 nd=vYear(y)->number_of_weeks();
	 while (w2>nd) 
	 { 
	    w2-=nd;
	    y2++; 
	    nd=vYear(y2)->number_of_weeks();
	 }
      }
      else
	 while (w2<1) 
	 { 
	    y2--; 
	    w2+=vYear(y2)->number_of_weeks();
	 }
      return vWeek(y2,w2);
   }

   int|object `-(int|object n)
   {
      if (object_program(n)==vWeek && objectp(n)) 
	 return (this->day(1)-n->day(1))/7;
      return this+(-n);
   }

   object next()
   {
      return this+1;
   }

   object prev()
   {
      return this-1;
   }

//-- internal -------------------------------------------------------

   int yday()
   {
      return 
	 ({0,-1,-2,-3,3,2,1})[this->year()->julian_day(0)%7]
         +7*(w-1);
   }

//-- nonstandard methods --------------------------------------------

   int number_of_days()
   {
      return 7;
   }

   int number()
   {
      return w;
   }

   string name()
   {
      return "w"+(string)this->number();
   }

   mixed cast(string what)
   {
      switch (what)
      {
	 case "int": return this->number(); 
	 case "string": return this->name();
	 default:
	    throw(({"can't cast to "+what+"\n",backtrace()}));
      }
   }

//-- less -----------------------------------------------------------

   object day(int|string n)
   {
      if (stringp(n))
      {
	 if (!week_day_mapping)
	    week_day_mapping=
	       mkmapping(Array.map(week_day_names,lower_case),
			 indices(allocate(8))[1..]);
	 n=week_day_mapping[n];
      }

      if (n<0) n=8+n;
      else if (!n) n=1;
      n+=yday()-1;
      if (n<0) return vYear(y-1)->day(n);
      if (n>=this->year()->number_of_days()) 
 	 return vYear(y+1)->day(n-this->year()->number_of_days());
      return vDay(y,n);
   }

   array(mixed) days()
   {
       return ({1,2,3,4,5,6,7});
   }

//-- more -----------------------------------------------------------
     
   object year()
   {
      return vYear(y);
   }
};

//
//== Day =====================================================================

class Day
{
//-- variables ------------------------------------------------------

   object this=this_object();
   program vYear=function_object(object_program(this))->Year;
   program vDay=function_object(object_program(this))->Day;
   program vMonth=function_object(object_program(this))->Month;
   program vWeek=function_object(object_program(this))->Week;

   int y;
   int d;

//-- standard methods -----------------------------------------------

   array(string) lesser() 
   { 
      return ({});
   }

   array(string) greater()
   {
      return ({"year","month","week"});
   }

   void create(int ... arg)
   {
      if (!sizeof(arg))
      {
	 mapping t=localtime(time());
	 y=1900+t->year;
	 d=t->yday;
      }
      else
      {
	 y=arg[0];
	 d=arg[1];
      } 
   }

   int `<(object x)
   {
      return 
	 (object_program(x)==object_program(this) &&
	  (x->y==y && x->d<d) || (x->y<y));
   }

   int `==(object x)
   {
      return 
	 object_program(x)==object_program(this) &&
	 x->y==y && x->d==d;
   }

   int `>(object x)
   {
      return 
	 (object_program(x)==object_program(this) &&
	  (x->y==y && x->d>d) || (x->y>y));
   }

   object `+(int n)
   {
      int y2=y,nd;
      int d2=d;
      d2+=n;
      
      if (d2>0)
      {
	 nd=vYear(y)->number_of_days();
	 while (d2>=nd) 
	 { 
	    d2-=nd;
	    y2++; 
	    nd=vYear(y2)->number_of_days();
	 }
      }
      else
	 while (d2<0) 
	 { 
	    y2--; 
	    d2+=vYear(y2)->number_of_days();
	 }
      return vDay(y2,d2);
   }

   int|object `-(object|int n)
   {
      if (objectp(n) && object_program(n)==vDay) 
	 return (this->julian_day()-n->julian_day());

      return this+(-n);
   }

   object next()
   {
      return this+1;
   }

   object prev()
   {
      return this-1;
   }

//-- nonstandard methods --------------------------------------------

   int julian_day()
   {
      return vYear(y)->julian_day()+d;
   }

   int year_day()
   {
      return d;
   }

   int month_day()
   {
      int d=year_day();
      int pj=0;
      foreach (this->year()->month_start_day,int j)
	 if (d<j) return d-pj+1;
	 else pj=j;
      return d-pj+1;
   }

   int week_day()
   {
      return (julian_day()%7+1);
   }

   string week_day_name()
   {
      return week_day_names[(this->week_day()+6)%7];
   }

//-- less -----------------------------------------------------------

   // none

//-- more -----------------------------------------------------------
     
   object year()
   {
      return vYear(y);
   }

   object month()
   {
      int d=year_day();
      array a=year()->month_start_day;
      for (int i=2; i<sizeof(a); i++)
	 if (d<a[i]) return vMonth(y,i-1);
      return vMonth(y,12);
   }

   object week()
   {
      int n;
      object ye=this->year();
      n=(-({0,-1,-2,-3,3,2,1})[this->year()->julian_day(0)%7]+d)/7+1;
      if (n>ye->number_of_weeks())
	 return ye->next()->week(1);
      else if (n<=0)
	 return ye->prev()->week(-1);
      return vWeek(y,n);
   }
};


