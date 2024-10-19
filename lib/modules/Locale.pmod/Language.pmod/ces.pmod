#pike __REAL_VERSION__

#charset iso-8859-2

//! Czech language locale by Jan Petrous 16.10.1997,
//! based on Slovenian language module by Iztok Umek.

inherit "abstract";

constant name = "�esky";
constant english_name = "czech";
constant iso_639_1 = "cs";
constant iso_639_2 = "ces";
constant iso_639_2B = "cze";

constant aliases = ({ "cs", "cz", "cze", "ces", "czech", "�esky" });

constant required_charset = "iso-8859-2";

constant languages=([
  "cat":"katal�nsky",
  "ces":"�esky",
  "nld":"holandsky",
  "fin":"finsky",
  "fra":"francouzsky",
  "deu":"n�mecky",
  "eng":"anglicky",
  "spa":"�pan�lsky",
  "hrv":"chorvatsky",
  "hun":"ma�arsky",
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
  "swe":"�v�dsky"
]);

constant months = ({
  "ledna", "�nora", "b�ezna", "dubna", "kv�tna",
  "�ervna", "�ervence", "srpna", "z���", "��jna",
  "listopadu", "prosince" });

constant days = ({
  "ned�le","pond�l�","�ter�","st�eda",
  "�tvrtek","p�tek","sobota" });

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
    return ("v�era, "+ ctime(timestamp)[11..15]);

  if((t1["yday"]-1) == t2["yday"] && t1["year"] == t2["year"])
    return ("z�tra, "+ ctime(timestamp)[11..15]);

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
   case 2:  return ("dv�");
   case 3:  return ("t�i");
   case 4:  return ("�ty�i");
   case 5:  return ("p�t");
   case 6:  return ("�est");
   case 7:  return ("sedm");
   case 8:  return ("osm");
   case 9:  return ("dev�t");
   case 10: return ("deset");
   case 11: return ("jeden�ct");
   case 12: return ("dvan�ct");
   case 13: case 16..18: return (number(num-10)+"n�ct");
   case 14: return ("�trn�ct");
   case 15: return ("patn�st");
   case 19: return ("devaten�ct");
   case 20: return ("dvacet");
   case 30: return ("t�icet");
   case 40: return ("�ty�icet");
   case 50: return ("pades�t");
   case 60: return ("�edes�t");
   case 70: return ("sedmdes�t");
   case 80: return ("osmdes�t");
   case 90: return ("devades�t");
   case 21..29: case 31..39:
   case 51..59: case 61..69: case 71..79:
   case 81..89: case 91..99: case 41..49:
     return (number((num/10)*10)+number(num%10));
   case 100..199: return ("sto"+number(num%100));
   case 200..299: return ("dv�st� "+number(num%100));
   case 300..499: return (number(num/100)+"sta "+number(num%100));
   case 500..999: return (number(num/100)+"set "+number(num%100));
   case 1000..1999: return ("tis�c "+number(num%1000));
   case 2000..2999: return ("dva tis�ce "+number(num%1000));
   case 3000..999999: return (number(num/1000)+" tis�c "+number(num%1000));
   case 1000000..999999999:
     return (number(num/1000000)+" milion "+number(num%1000000));
   default:
    return ("hodn�");
  }
}
