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

class _TimeUnit
{
   inherit Calendar._TimeUnit;

   program vYear=function_object(object_program(this_object()))->Year;
   program vDay=function_object(object_program(this_object()))->Day;
   program vMonth=function_object(object_program(this_object()))->Month;
   program vWeek=function_object(object_program(this_object()))->Week;
}

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
   inherit _TimeUnit;

//-- variables ------------------------------------------------------

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
	 object yp=vDay()->year();
	 y=yp->y;
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

   int leap_day()
   {
      return 31+24-1; // 24 Feb
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

   object month(object|string|int n)
   {
      if (objectp(n))
	 if (object_program(n)==vMonth)
	    n=n->number();

      if (stringp(n))
      {
	 if (!month_mapping)
	 {
	    month_mapping=
	       mkmapping(Array.map(month_names,lower_case),
			 indices(allocate(13))[1..]);
	 }
	 n=month_mapping[lower_case(n)];
	 if (!n) return 0;
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

   object week(object|int n)
   {
      if (objectp(n))
	 if (object_program(n)==vWeek)
	 {
	    n=n->number();
	    if (n>number_of_weeks()) return 0; /* no such week */
	 }

      if (n<0)
	 return vWeek(y,this->number_of_weeks()+n+1);
      else
	 return vWeek(y,n||1);
   }

   array(mixed) weeks()
   {
       return indices(allocate(this->number_of_weeks()+1))[1..];
   }

   object day(object|int n)
   {
      if (objectp(n))
	 if (object_program(n)==vDay)
	 {
	    object o=n->year();
	    n=n->year_day();
	    if (o->leap() && n==o->leap_day())
	       if (!this->leap()) return 0;
	       else n=this->leap_day();
	    else
	    {
	       if (o->leap() && n>o->leap_day()) n--;
	       if (this->leap() && n>=this->leap_day()) n++;
	    }
	    if (n>=number_of_days()) return 0; /* no such day */
	 }
	 else return 0; /* illegal object */

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
   inherit _TimeUnit;
//-- variables ------------------------------------------------------

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
	 object mp=vDay()->month();
	 y=mp->y;
	 m=mp->m;
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

   object day(int|object n)
   {
      if (objectp(n))
	 if (object_program(n)==vDay)
	 {
	    n=n->month_day();
	    if (n>number_of_days()) return 0; /* no such day */
	 }

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
   inherit _TimeUnit;
//-- variables ------------------------------------------------------

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

   object day(int|string|object n)
   {
      if (stringp(n))
      {
	 if (!week_day_mapping)
	    week_day_mapping=
	       mkmapping(Array.map(week_day_names,lower_case),
			 ({1,2,3,4,5,6,0}));
	 n=week_day_mapping[n];
      }
      else if (objectp(n))
	 if (object_program(n)==vDay)
	    n=n->week_day();
	 else return 0;

      if (n<0) n=7+n;
      n+=this->yday()-1;
      if (n<0) return vYear(y-1)->day(n);
      if (n>=this->year()->number_of_days()) 
 	 return vYear(y+1)->day(n-this->year()->number_of_days());
      return vDay(y,n);
   }

   array(mixed) days()
   {
       return ({0,1,2,3,4,5,6});
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
   inherit _TimeUnit;

//-- variables ------------------------------------------------------

   int y;
   int d;

//-- standard methods -----------------------------------------------

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

//-- nonstandard methods --------------------------------------------

   int julian_day()
   {
      return vYear(y)->julian_day(d);
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
      return (julian_day()+1)%7;
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
      n=(-({-1,-2,-3,-4,2,1,0})[this->year()->julian_day(0)%7]+d)/7+1;
      if (n>ye->number_of_weeks())
	 return ye->next()->week(1);
      else if (n<=0)
	 return ye->prev()->week(-1);
      return vWeek(y,n);
   }
};


