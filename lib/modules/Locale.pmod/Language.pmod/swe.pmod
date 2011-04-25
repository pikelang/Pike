#pike __REAL_VERSION__


//! Swedish language locale

// $Id$

inherit "abstract";

constant name = "svenska";
constant english_name = "swedish";
constant iso_639_1 = "sv";
constant iso_639_2 = "swe";
constant iso_639_2B = "swe";

constant aliases = ({ "sv", "se", "sve", "swe", "swedish", "svenska" });

constant months = ({
  "januari", "februari", "mars", "april", "maj",
  "juni", "juli", "augusti", "september", "oktober",
  "november", "december" });

constant days = ({
  "söndag","måndag","tisdag","onsdag", "torsdag","fredag",
  "lördag" });

constant languages=([
  "cat":"katalanska",
  "ces":"tjeckiska",
  "nld":"nederländska",
  "fin":"finska",
  "fra":"franska",
  "deu":"tyska",
  "eng":"engelska",
  "spa":"spanska",
  "hrv":"kroatiska",
  "hun":"ungerska",
  "ita":"italienska",
  "jpn":"japanska",
  "mri":"maori",
  "nor":"norska",
  "pol":"polska",
  "por":"portugisiska",
  "rus":"ryska",
  "sin":"slovenska",
  "slk":"slovakiska",
  "srp":"serbiska",
  "swe":"svenska"
]);

string ordered(int i)
{
  if (((< 1,2 >)[i%10]) && (!(< 11,12 >)[i%100])) {
    return i + ":a";
  }
  return i + ":e";
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
    return "i dag, klockan " + ctime(timestamp)[11..15];

  if(t1["yday"] == t2["yday"]-1 && t1["year"] == t2["year"])
    return "i går, klockan " + ctime(timestamp)[11..15];

  if(t1["yday"] == t2["yday"]+1 && t1["year"] == t2["year"])
    return "i morgon, vid "  + ctime(timestamp)[11..15];

  if(t1["year"] != t2["year"])
    return month(t1["mon"]+1) + " " + (t1["year"]+1900);

  return "den " + t1["mday"] + " " + month(t1["mon"]+1);
}

string _number(int num)
{
  switch(num)
  {
   case 0:  return "";
   case 1:  return "ett";
   case 2:  return "två";
   case 3:  return "tre";
   case 4:  return "fyra";
   case 5:  return "fem";
   case 6:  return "sex";
   case 7:  return "sju";
   case 8:  return "åtta";
   case 9:  return "nio";
   case 10: return "tio";
   case 11: return "elva";
   case 12: return "tolv";
   case 13: return "tretton";
   case 14: return "fjorton";
   case 15: return "femton";
   case 16: return "sexton";
   case 17: return "sjutton";
   case 18: return "arton";
   case 19: return "nitton";
   case 20: return "tjugo";
   case 30: return "trettio";
   case 40: return "fyrtio";
   case 50: return "femtio";
   case 60: return "sextio";
   case 70: return "sjuttio";
   case 80: return "åttio";
   case 90: return "nittio";
   case 21..29: case 31..39: case 41..49:
   case 51..59: case 61..69: case 71..79:
   case 81..89: case 91..99:
    return _number((num/10)*10)+_number(num%10);
   case 100..999: return _number(num/100)+"hundra"+_number(num%100);
   case 1000..1999:
    return _number(num/1000)+"tusen"+_number(num%1000);
   case 2000..999999: return _number(num/1000)+"tusen"+_number(num%1000);
   case 1000000..1999999:
    return "en miljon"+_number(num%1000000);
   case 2000000..999999999:
     return _number(num/1000000)+"miljoner"+_number(num%1000000);
   default:
    return "många";
  }
}

string number(int num)
{
  if (num<0) {
    return("minus "+_number(-num));
  } if (num) {
    return(_number(num));
  } else {
    return("noll");
  }
}
