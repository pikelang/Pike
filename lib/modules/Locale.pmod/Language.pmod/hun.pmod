#charset iso-8859-2
#pike __REAL_VERSION__

//! Hungarian language locale by Zsolt Varga.

inherit "abstract";

constant name = ""; // FIXME
constant english_name = "hungarian";
constant iso_639_1 = "hu";
constant iso_639_2 = "hun";
constant iso_639_2B = "hun";

constant aliases = ({ "hu", "hun", "magyar", "hungarian" });

constant months = ({
  "janu�r", "febru�r", "m�rcius",
  "�prilis", "m�jus", "j�nius",
  "j�lius", "augusztus", "szeptember",
  "okt�ber", "november", "december" });

constant days = ({
  "vas�rnap", "h�tf�", "kedd",
  "szerda", "cs�t�rt�k", "p�ntek",
  "szombat" });

string ordered(int i)
{
    if(!i)
      return "�rtelmezhetetlen";
    return i+". ";
}

string date(int timestamp, string|void m)
{
  mapping t1=localtime(timestamp);
  mapping t2=localtime(time(0));

  if(m=="full")
    return ( (t1["year"]+1900) + ". " + month(t1["mon"]+1) + ". "+
           ordered(t1["mday"]) + ", " + ctime(timestamp)[11..15] );

  if(m=="date")
    return ( (t1["year"]+1900)+". "+month(t1["mon"]+1)+". "+
           ordered(t1["mday"]) );

  if(m=="time")
    return ctime(timestamp)[11..15];

  // !m

  if(t1["yday"] == t2["yday"] && t1["year"] == t2["year"])
    return "ma, "+ ctime(timestamp)[11..15];

  if(t1["yday"]+1 == t2["yday"] && t1["year"] == t2["year"])
    return "tegnap, "+ ctime(timestamp)[11..15];

  if(t1["yday"]-1 == t2["yday"] && t1["year"] == t2["year"])
    return "holnap, "+ ctime(timestamp)[11..15];

  if(t1["year"] != t2["year"])
    return ( (t1["year"]+1900) + ". " + month(t1["mon"]+1) );

  return (month(t1["mon"]+1) + ". " + ordered(t1["mday"]));
}

string number(int num)
{
  if(num<0)
    return "minusz "+number(-num);

  switch(num)
  {
   case 0:  return "";
   case 1:  return "egy";
   case 2:  return "kett�";
   case 3:  return "h�rom";
   case 4:  return "n�gy";
   case 5:  return "�t";
   case 6:  return "hat";
   case 7:  return "h�t";
   case 8:  return "nyolc";
   case 9:  return "kilenc";
   case 10: return "t�z";
   case 11..19: return "tizen"+number(num%10);
   case 20: return "h�sz";
   case 21..29: return "huszon"+number(num%10);

   case 30: return "harminc";
   case 40: return "negyven";
   case 50: return "�tven";
   case 60: return "hatvan";
   case 70: return "hetven";
   case 80: return "nyolcvan";
   case 90: return "kilencven";

   case 31..39: case 41..49: case 51..59:
   case 61..69: case 71..79: case 81..89:
   case 91..99:
     return number((num/10)*10)+number(num%10);

   case 100..999:
     return number(num/100)+"sz�z"+number(num%100);

   case 1000..999999:
     return number(num/1000)+"ezer"+number(num%1000);

   case 1000000..999999999:
     return number(num/1000000)+"milli�-"+number(num%1000000);

   default:
    return "sok";
  }
}
