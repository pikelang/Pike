// $Id$

//!	time unit: TNGDate

#pike 7.0

//!	Implements ST:TNG stardates.
//!	Can be used as create argument to Day.
class TNGDate
{
   inherit Calendar._TimeUnit;

   // 40759.5  2363-10-05  2584405
   // 47391.2  2370-05-23  2586827

   // 50893.5  2373-11-23  2588107

   //  6631.7  ----------  2422 
   // 10134.0  ----------  3702
   //  1000.0  ----------  365.2425
   //     0.0      -  -    2569519
#define TNGSTARPERJULIAN (1000.0/365.2425)

//-- variables ------------------------------------------------------

   float jd;
   float tics;
   
//-- standard methods -----------------------------------------------

   void create(int|float|object ... day)
   {
      float jd;
      if (!sizeof(day))
	 day=({Calendar.Gregorian.Second()});
      else if (floatp(day[0]))
      {
	 from_stardate(day[0]);
	 return;
      }
      if (!intp(day[0]))
      {
	 object o=day[0];

	 if (o->julian_day || o->julian_day_f)
	    jd=(float)(o->julian_day_f||o->julian_day)();
	 else // dig
	    if (o->day) // larger
	    {
	       o=o->day(0);
	       if (o->julian_day_f)
		  jd=o->julian_day_f();
	       else if (o->julian_day)
		  jd=(float)o->julian_day();
	       else
		  ; // error, like
	    }
	    else // smaller
	    {
	       float z=1.0;
	       while (sizeof(o->greater()))
	       {
		  string name=o->is();
		  o=o[o->greater()[0]]();
		  z*=o["number_of_"+name+"s"]();
		  if (o->julian_day_f || o->julian_day) 
		  {
		     jd=(o->julian_day||o->julian_day_f)()/z;
		     break;
		  }
	       }
	    }
      }
      else 
	 jd=(float)day[0];
      from_julian_day(jd);
   }

   static void from_stardate(float f)
   {
      tics=f;
      jd=f/TNGSTARPERJULIAN+2569518.5;
   }

   static void from_julian_day(float f)
   {
      jd=f;
      tics=(f-2569518.5)*TNGSTARPERJULIAN;
   }

//-- nonstandard methods --------------------------------------------

   float number()
   {
      return tics;
   }
   
   int julian_day()
   {
      return (int)jd;
   }

   float julian_day_f()
   {
      return jd;
   }
}
