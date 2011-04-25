#pike __REAL_VERSION__

#charset iso-8859-2

//! Czech language locale by Jan Petrous 16.10.1997,
//! based on Slovenian language module by Iztok Umek.

// $Id$

inherit "abstract";

constant name = "\33-B\415esky";
constant english_name = "czech";
constant iso_639_1 = "cs";
constant iso_639_2 = "ces";
constant iso_639_2B = "cze";

constant aliases = ({ "cs", "cz", "cze", "ces", "czech", "\33-B\415esky" });

constant required_charset = "iso-8859-2";

constant languages=([
  "cat":"katalánsky",
  "ces":"èesky",
  "nld":"holandsky",
  "fin":"finsky",
  "fra":"francouzsky",
  "deu":"nìmecky",
  "eng":"anglicky",
  "spa":"¹panìlsky",
  "hrv":"chorvatsky",
  "hun":"maïarsky",
  "ita":"italsky",
  "jpn":"japonsky",
  "mri":"maorsky",
  "nor":"norsky",
  "pol":"polsky",
  "por":"portugalsky",
  "rus":"rusky",
  "sin":"slovinsky",
  "slk":"slovensky",
  "srn":"srbsky",
  "swe":"¹védsky"
]);

constant months = ({
  "ledna", "února", "bøezna", "dubna", "kvìtna",
  "èervna", "èervence", "srpna", "záøí", "øíjna",
  "listopadu", "prosince" });

constant days = ({
  "nedìle","pondìlí","úterý","støeda",
  "ètvrtek","pátek","sobota" });

string ordered(int i)
{
  switch(i)
  {
   case 0:
    return ("buggy");
   default:
      return (i+".");
  }
}

string date(int timestamp, string|void m)
{
  mapping t1=localtime(timestamp);
  mapping t2=localtime(time(0));

  if(m=="full")
    return (ctime(timestamp)[11..15]+", "+
	   ordered(t1["mday"]) + " " +
           month(t1["mon"]+1) + " " +
           (t1["year"]+1900));

  if(m=="date")
    return (ordered(t1["mday"]) + " " + month(t1["mon"]+1) + " " +
       (t1["year"]+1900));

  if(m=="time")
    return (ctime(timestamp)[11..15]);

  // !m

  if(t1["yday"] == t2["yday"] && t1["year"] == t2["year"])
    return ("dnes, "+ ctime(timestamp)[11..15]);

  if(t1["yday"]+1 == t2["yday"] && t1["year"] == t2["year"])
    return ("vèera, "+ ctime(timestamp)[11..15]);

  if((t1["yday"]-1) == t2["yday"] && t1["year"] == t2["year"])
    return ("zítra, "+ ctime(timestamp)[11..15]);

  if(t1["year"] != t2["year"])
    return (month(t1["mon"]+1) + " " + (t1["year"]+1900));

  return (ordered(t1["mday"]) + " " + month(t1["mon"]+1));
}


string number(int num)
{
  if(num<0)
    return ("minus "+number(-num));
  switch(num)
  {
   case 0:  return ("");
   case 1:  return ("jedna");
   case 2:  return ("dvì");
   case 3:  return ("tøi");
   case 4:  return ("ètyøi");
   case 5:  return ("pìt");
   case 6:  return ("±est");
   case 7:  return ("sedm");
   case 8:  return ("osm");
   case 9:  return ("devìt");
   case 10: return ("deset");
   case 11: return ("jedenáct");
   case 12: return ("dvanáct");
   case 13: case 16..18: return (number(num-10)+"náct");
   case 14: return ("ètrnáct");
   case 15: return ("patnást");
   case 19: return ("devatenáct");
   case 20: return ("dvacet");
   case 30: return ("tøicet");
   case 40: return ("ètyøicet");
   case 50: return ("padesát");
   case 60: return ("±edesát");
   case 70: return ("sedmdesát");
   case 80: return ("osmdesát");
   case 90: return ("devadesát");
   case 21..29: case 31..39:
   case 51..59: case 61..69: case 71..79:
   case 81..89: case 91..99: case 41..49:
     return (number((num/10)*10)+number(num%10));
   case 100..199: return ("sto"+number(num%100));
   case 200..299: return ("dvìstì "+number(num%100));
   case 300..499: return (number(num/100)+"sta "+number(num%100));
   case 500..999: return (number(num/100)+"set "+number(num%100));
   case 1000..1999: return ("tisíc "+number(num%1000));
   case 2000..2999: return ("dva tisíce "+number(num%1000));
   case 3000..999999: return (number(num/1000)+" tisíc "+number(num%1000));
   case 1000000..999999999:
     return (number(num/1000000)+" milion "+number(num%1000000));
   default:
    return ("hodnì");
  }
}
