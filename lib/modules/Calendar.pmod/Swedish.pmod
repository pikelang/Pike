inherit Calendar.Gregorian : christ;

void create()
{
   month_names=
      ({"Januari","Februari","Mars","April","Maj","Juni","Juli","Augusti",
	"September","Oktober","November","December"});

   week_day_names=
      ({"Måndag","Tisdag","Onsdag","Torsdag",
	"Fredag","Lördag","Söndag"});
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
}
