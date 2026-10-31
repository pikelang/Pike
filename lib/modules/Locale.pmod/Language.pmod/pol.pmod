#pike __REAL_VERSION__

#charset utf-8

//! Polish language locale by Piotr Klaban <makler@@man.torun.pl>.

inherit "abstract";

constant name = "polski";
constant english_name = "polish";
constant iso_639_1 = "pl";
constant iso_639_2 = "pol";
constant iso_639_2B = "pol";

constant aliases = ({ "pl", "po", "pol", "polish" });

constant required_charset = "iso-8859-2";

constant languages=([
  "cat":"kataloński",
  "ces":"czeski",
  "nld":"holenderski",
  "fin":"fiński",
  "fra":"francuski",
  "deu":"niemiecki",
  "eng":"angielski",
  "spa":"hiszpański",
  "hrv":"chorwacki",
  "hun":"węgierski",
  "ita":"włoski",
  "jpn":"japoński",
  "mri":"maoryjski",
  "nor":"norweski",
  "pol":"polski",
  "por":"portugalski",
  "rus":"rosyjski",
  "slk":"słowacki",
  "srp":"serbski",
  "swe":"szwedzki"
]);

constant months = ({
  "Styczeń", "Luty", "Marzec", "Kwiecień", "Maj",
  "Czerwiec", "Lipiec", "Sierpień", "Wrzesien", "Październik",
  "Listopad", "Grudzień" });

constant dated_months = ({
  "Stycznia", "Lutego", "Marca", "Kwietnia", "Maja",
  "Czerwca", "Lipca", "Sierpnia", "Września", "Października",
  "Listopada", "Grudnia" });

constant days = ({
  "Niedziela","Poniedziałek","Wtorek","Środa",
  "Czwartek","Piątek","Sobota" });

string month(int(1..12) num, int|void dated)
{
  return (dated?dated_months:months)[ num - 1 ];
}

string ordered(int i)
{
  switch(i)
  {
   case 0:
    return "błąd";
   default:
    return i+".";
  }
}

string date(int timestamp, string|void m)
{
  mapping t1=localtime(timestamp);
  mapping t2=localtime(time(0));

  if(m=="full")
    return ctime(timestamp)[11..15]+", "+
	   ordered(t1["mday"]) + " " +
           month(t1["mon"]+1, 1) + " " +
           (t1["year"]+1900);

  if(m=="date")
    return (ordered(t1["mday"]) + " " + month(t1["mon"]+1, 1) + " " +
       (t1["year"]+1900));

  if(m=="time")
    return ctime(timestamp)[11..15];

  // !m

  if(t1["yday"] == t2["yday"] && t1["year"] == t2["year"])
    return "dziś, "+ ctime(timestamp)[11..15];

  if(t1["yday"]+1 == t2["yday"] && t1["year"] == t2["year"])
    return "wczoraj, "+ ctime(timestamp)[11..15];

  if(t1["yday"]-1 == t2["yday"] && t1["year"] == t2["year"])
    return "jutro, "+ ctime(timestamp)[11..15];

  if(t1["year"] != t2["year"])
    return (month(t1["mon"]+1) + " " + (t1["year"]+1900));

  return (ordered(t1["mday"]) + " " + month(t1["mon"]+1, 1));
}


string number(int num)
{
  int tmp;

  if(num<0)
    return "minus "+number(-num);
  switch(num)
  {
   case 0:  return "";
   case 1:  return "jeden";
   case 2:  return "dwa";
   case 3:  return "trzy";
   case 4:  return "cztery";
   case 5:  return "pięć";
   case 6:  return "sześć";
   case 7:  return "siedem";
   case 8:  return "osiem";
   case 9:  return "dziewięć";
   case 10: return "dziesięć";
   case 11: return "jedenaście";
   case 12: return "dwanaście";
   case 13: return "trzynaście";
   case 14: return "czternaście";
   case 15: return "piętnaście";
   case 16: return "szesnaście";
   case 17: return "siedemnaście";
   case 18: return "osiemnaście";
   case 19: return "dziewiętnaście";
   case 20: return "dwadzieścia";
   case 30: return "trzydzieści";
   case 40: return "czterdzieści";
   case 50: case 60: case 70: case 80: case 90:
     return number(num/10)+"dziesiąt";
   case 21..29: case 31..39:
   case 51..59: case 61..69: case 71..79:
   case 81..89: case 91..99: case 41..49:
     return number((num/10)*10)+" "+number(num%10);
   case 100: return "sto";
   case 200: return "dwieście";
   case 300: return "trzysta";
   case 400: return "czterysta";
   case 500: case 600: case 700: case 800: case 900:
     return number(num/100)+"set";
   case 101..199: case 201..299: case 301..399: case 401..499:
   case 501..599: case 601..699: case 701..799: case 801..899:
   case 901..999:
     return number(num-(num%100))+" "+number(num%100);
   case 1000..1999: return "tysiąc "+number(num%1000);
   case 2000..4999: return number(num/1000)+" tysiące "+number(num%1000);
   case 5000..999999: return number(num/1000)+" tysięcy "+number(num%1000);
   case 1000001..1999999:
     return number(num/1000000)+" milion "+number(num%1000000);
   case 2000000..999999999:
     tmp = (num/1000000) - ((num/10000000)*10000000);
     switch (tmp)
     {
	case 2: case 3: case 4:
	     return number(num/1000000)+" miliony "+number(num%1000000);
	default:
	     return number(num/1000000)+" milionów "+number(num%1000000);
     }
   default:
    return "wiele";
  }
}
