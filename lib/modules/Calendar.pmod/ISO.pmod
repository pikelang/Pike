// IS-8601, international standard

inherit Calendar.Gregorian:Gregorian;

class Year
{
   inherit Gregorian::Year;

   int leap_day()
   {
      if (y>1999) return 31+29-1; // 29 Feb
      return 31+24-1; // 24 Feb
   }
}

class Week
{
   inherit Gregorian::Week;

   int yday()
   {
      return 
	 ({0,-1,-2,-3,3,2,1})[this->year()->julian_day(0)%7]
         +7*(w-1);
   }

   array(mixed) days()
   {
       return ({1,2,3,4,5,6,7});
   }

   object day(int|string|object n)
   {
      if (stringp(n)) 
      {
	 if (!week_day_mapping)
	    week_day_mapping=
	       mkmapping(Array.map(week_day_names,lower_case),
			 ({1,2,3,4,5,6,7}));
	 n=week_day_mapping[n];
      }
      else if (objectp(n))
	 if (object_program(n)==vDay)
	    n=n->week_day();
	 else return 0;

      if (n<0) n=8+n;
      else if (!n) n=1;
      n+=this->yday()-1;
      if (n<0) return vYear(y-1)->day(n);
      if (n>=this->year()->number_of_days()) 
 	 return vYear(y+1)->day(n-this->year()->number_of_days());
      return vDay(y,n);
   }
}

class Day
{
   inherit Gregorian::Day;

   int week_day()
   {
      return julian_day()%7+1;
   }

   string week_day_name()
   {
      return week_day_names[(this->week_day()+6)%7];
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
}
