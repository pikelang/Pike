inherit Calendar.Gregorian : christ;

void create()
{
   month_names=
      ({"januari","februari","mars","april","maj","juni","juli","augusti",
	"september","oktober","november","december"});

   week_day_names=
      ({"måndag","tisdag","onsdag","torsdag",
	"fredag","lördag","söndag"});
}

class Week
{
   inherit Calendar.Gregorian.Week;

   string name()
   {
      return "v"+(string)this->number();
   }
}

class Year
{
   inherit Calendar.Gregorian.Year;

   string name()
   {
      if (this->number()<=0) 
	 return (string)(1-this->number())+" fk";
      return (string)this->number();
   }

   int leap_day()
   {
      if (y>1999) return 31+29-1; // 29 Feb
      return 31+24-1; // 24 Feb
   }
}
