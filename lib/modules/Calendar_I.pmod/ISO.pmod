#pike __REAL_VERSION__

// IS-8601, international standard

inherit Calendar_I.Gregorian:Gregorian;

class Year
{
   inherit Gregorian::Year;

   int leap_day()
   {
      if (y>1999) return 31+29-1; // 29 Feb
      return 31+24-1; // 24 Feb
   }

   string name()
   {
      return (string)y;
   }
}

class Month
{
   inherit Gregorian::Month;

   string iso_name()
   {
      return sprintf("%04d-%02d",
		     (int)this->year(),
		     (int)this);
   }
  
   string iso_short_name()
   {
      return name()-"-";
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
  
   string iso_name()
   {
      return sprintf("%04d-W%02d",
		     (int)this->year(),
		     (int)this);
   }
  
   string iso_short_name()
   {
      return name()-"-";
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
//       werror("week_days: %O\n",week_day_names);
      return week_day_names[(this->week_day()+6)%7];
   }

   string iso_name()
   {
      return sprintf("%04d-%02d-%02d",
		     (int)this->year(),
		     (int)this->month(),
		     (int)this->month_day());
   }

   string iso_short_name()
   {
      return iso_name()-"-";
   }

   string iso_name_by_week()
   {
      return sprintf("%04d-W%02d-%d",
		     (int)this->year(),
		     (int)this->week(),
		     (int)this->week_day());
   }
  
   string iso_name_by_yearday()
   {
      return sprintf("%04d-%03d",
		     (int)this->year(),
		     (int)this->year_day());
   }
  
   string iso_short_name_by_yearday()
   {
      return iso_name_by_yearday()-"-";
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

static private class _Day
{
   // FIXME: Kludge because the day object does not exist in
   // Minute and Second. This function will be shadowed in Hour.
   object day()
   {
      return this_object()->hour()->day();
   }
}

static private class Name
{
   object this = this_object();

   string iso_name()
   {
      return this->day()->iso_name()+this->_iso_name();
   }
  
   string iso_short_name()
   {
      return this->day()->iso_short_name()+this->_iso_short_name();
   }
  
   string iso_name_by_week()
   {
      return this->day()->iso_name_by_week()+this->_iso_name();
   }
  
   string iso_name_by_yearday()
   {
      return this->day()->iso_name_by_yearday()+this->_iso_name();
   }
  
   string iso_short_name_by_yearday()
   {
      return this->day()->iso_short_name_by_yearday()+this->_iso_short_name();
   }
}

class Hour
{
   inherit Gregorian::Hour;
   inherit Name;

   string _iso_name()
   {
      return sprintf("T%02d",
		     (int)this);
   }

   string _iso_short_name()
   {
      return _iso_name();
   }
}

class Minute
{
   inherit _Day;
   inherit Gregorian::Minute;
   inherit Name;

   string _iso_name()
   {
      return sprintf("T%02d:%02d",
		     (int)this->hour(),
		     (int)this);
   }

   string _iso_short_name()
   {
      return _iso_name()-":";
   }
}

class Second
{
   inherit _Day;
   inherit Gregorian::Second;
   inherit Name;

   string _iso_name()
   {
      return sprintf("T%02d:%02d:%02d",
		     (int)this->hour(),
		     (int)this->minute(),
		     (int)this);
   }

   string _iso_short_name()
   {
      return _iso_name()-":";
   }
}

