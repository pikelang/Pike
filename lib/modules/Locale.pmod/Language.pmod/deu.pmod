#pike __REAL_VERSION__


//! German language locale by Tvns Böker.

// $Id$

inherit "abstract";

constant name = "deutsch";
constant english_name = "german";
constant iso_639_1 = "de";
constant iso_639_2 = "deu";
constant iso_639_2B = "ger";

constant aliases = ({ "de", "deu", "ger", "deutsch", "german" });

constant months = ({
  "Januar", "Februar", "März", "April", "Mai",
  "Juni", "Juli", "August", "September", "Oktober",
  "November", "Dezember" });


constant days = ({
  "Sonntag","Montag","Dienstag","Mittwoch",
  "Donnerstag","Freitag","Samstag" });

string ordered(int i)
{
  return i+".";
}

string date(int timestamp, string|void m)
{
  mapping t1=localtime(timestamp);
  mapping t2=localtime(time(0));

  if(m=="full")
    return ctime(timestamp)[11..15]+", "+
           ordered(t1["mday"]) +" "+
           month(t1["mon"]+1) +" "+ (t1["year"]+1900);

  if(m=="date")
    return ordered(t1["mday"])+" "+month(t1["mon"]+1)
      + " im Jahre des Herrn " +(t1["year"]+1900);

  if(m=="time")
    return ctime(timestamp)[11..15];

  // !m

  if(t1["yday"] == t2["yday"] && t1["year"] == t2["year"])
    return "heute, "+ ctime(timestamp)[11..15];
  
  if(t1["yday"]+1 == t2["yday"] && t1["year"] == t2["year"])
    return "gestern, "+ ctime(timestamp)[11..15];
  
  if(t1["yday"]-1 == t2["yday"] && t1["year"] == t2["year"])
    return "morgen, "+ ctime(timestamp)[11..15];
  
  if(t1["year"] != t2["year"])
    return (month(t1["mon"]+1) + " " + (t1["year"]+1900));

  return ordered(t1["mday"]) + " " + (month(t1["mon"]+1));
}

string number(int num)
{
  if(num<0)
    return "minus "+number(-num);
  switch(num)
  {
   case 0:  return "";
   case 1:  return "eins";
   case 2:  return "zwei";
   case 3:  return "drei";
   case 4:  return "vier";
   case 5:  return "fünf";
   case 6:  return "sechs";
   case 7:  return "sieben";
   case 8:  return "acht";
   case 9:  return "neun";
   case 10: return "zehn";
   case 11: return "elf";
   case 12: return "zwölf";
   case 13..15:
   case 18..19: return number(num%10)+"zehn";
   case 16: return "sechzehn";
   case 17: return "siebzehn";
   case 20: return "zwanzig";
   case 30: return "dreißig";
   case 70: return "siebzig";
   case 40: case 50: case 60: case 80: case 90:
     return number(num/10)+"zig";

   case 21: case 31: case 41: case 51: case 61: case 71: case 81: case 91:
     return "einund"+number((num/10)*10);
   case 22..29: case 32..39: case 42..49:
   case 52..59: case 62..69: case 72..79: 
   case 82..89: case 92..99:
     return number(num%10)+"und"+number((num/10)*10);
   case 100..199: return "einhundert"+number(num%100);
   case 200..999: return number(num/100)+"hundert"+number(num%100);
   case 1000..1999: return "eintausend"+number(num%1000);
   case 2000..999999: return number(num/1000)+"tausend"+number(num%1000);
   case 1000000..1999999:
     return "eine Million "+number(num%1000000);
   case 2000000..999999999: 
     return number(num/1000000)+" Millionen "+number(num%1000000);
   default:
    return "verdammt viele";
  }
}
