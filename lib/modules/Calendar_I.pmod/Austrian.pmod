#pike __REAL_VERSION__

// by Martin Baehr <mbaehr@email.archlab.tuwien.ac.at>

inherit Calendar_I.ISO:ISO;

void create()
{
   month_names=
      ({"jänner","feber","märz","april","mai","juni","juli","august",
        "september","oktober","november","dezember"});

   week_day_names=
      ({"montag","dienstag","mittwoch","donnerstag",
        "freitag","samstag","sonntag"});
}

class Week
{
   inherit ISO::Week;

   string name()
   {
      return "w"+(string)this->number();
   }
}

class Year
{
   inherit ISO::Year;

   string name()
   {
      if (this->number()<=0)
         return (string)(1-this->number())+" v. Chr.";
      return (string)this->number();
   }
}
