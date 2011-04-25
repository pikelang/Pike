#pike __REAL_VERSION__


//! Norwegian language locale

// $Id$

inherit "abstract";

constant name = "norsk";
constant english_name = "norwegian";
constant iso_639_1 = "no";
constant iso_639_2 = "nor";
constant iso_639_2B = "nor";

constant aliases = ({ "no", "nor", "norwegian", "norsk" });

constant months = ({
  "januar", "februar", "mars", "april", "mai",
  "juni", "juli", "august", "september", "oktober",
  "november", "december" });

constant days = ({
  "søndag","mandag","tisdag","onsdag",
  "torsdag","fredag","lørdag" });

string ordered(int i)
{
    return "" + i + ".";
}

string date(int timestamp, string|void m)
{
  mapping t1=localtime(timestamp);
  mapping t2=localtime(time(0));

  if(m=="full")
    return sprintf("%s, den %s %s %d",
                   ctime(timestamp)[11..15],
                   ordered(t1["mday"]),
                   month(t1["mon"]+1), t1["year"]+1900);

  if(m=="date")
    return sprintf("den %s %s %d", ordered(t1["mday"]),
                   month(t1["mon"]+1), t1["year"]+1900);

  if(m=="time")
    return ctime(timestamp)[11..15];

  // !m

  if(t1["yday"] == t2["yday"] && t1["year"] == t2["year"])
    return "i dag, klokken " + ctime(timestamp)[11..15];
  
  if(t1["yday"] == t2["yday"]-1 && t1["year"] == t2["year"])
    return "i går, klokken " + ctime(timestamp)[11..15];
  
  if(t1["yday"] == t2["yday"]+1 && t1["year"] == t2["year"])
    return "i morgen, ved "  + ctime(timestamp)[11..15];
  
  if(t1["year"] != t2["year"])
    return month(t1["mon"]+1) + " " + (t1["year"]+1900);

  return "den " + t1["mday"] + " " + month(t1["mon"]+1);
}

string number(int num)
{
  if(!num)
    return "null";
  if(num<0)
    return "minus "+number(-num);
  switch(num)
  {
   case 0:  return "";
   case 1:  return "en";
   case 2:  return "to";
   case 3:  return "tre";
   case 4:  return "fire";
   case 5:  return "fem";
   case 6:  return "seks";
   case 7:  return "sju";
   case 8:  return "åtte";
   case 9:  return "ni";
   case 10: return "ti";
   case 11: return "elleve";
   case 12: return "tolv";
   case 13: return "tretten";
   case 14: return "fjorten";
   case 15: return "femten";
   case 16: return "seksten";
   case 17: return "sytten";
   case 18: return "atten";
   case 19: return "nitten";
   case 20: return "tjue";
   case 30: return "tretti";
   case 40: return "forti";
   case 50: return "femti";
   case 60: return "seksti";
   case 70: return "sytti";
   case 80: return "åtti";
   case 90: return "nitti";
   case 21..29: case 31..39: case 41..49:
   case 51..59: case 61..69: case 71..79:
   case 81..89: case 91..99:
     return number((num/10)*10)+number(num%10);
   case 100..999: return number(num/100)+"hundre"+number(num%100);
   case 1000..999999: return number(num/1000)+"tusen"+number(num%1000);
   case 1000000..999999999:
     return number(num/1000000)+"millioner"+number(num%1000000);
   default:
    return "ekstremt mange";
  }
}
