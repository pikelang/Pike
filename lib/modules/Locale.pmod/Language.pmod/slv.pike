#pike __REAL_VERSION__


//! Slovenian language locale by Iztok Umek 7. 8. 1997

inherit "abstract";

constant name = ""; // FIXME
constant english_name = "slovenian";
constant iso_639_1 = "sl";
constant iso_639_2 = "slv";
constant iso_639_2B = "slv";

constant aliases = ({ "sl", "si", "svn", "slv", "slovenian" });

constant months = ({
  "Januar", "Februar", "Marec", "April", "Maj",
  "Junij", "Julij", "Avgust", "September", "Oktober",
  "November", "December" });

constant days = ({
  "Nedelja","Ponedeljek","Torek","Sreda",
  "Èetrtek","Petek","Sobota" });

string number(int num)
{
  if(!num)
    return ""; //FIXME!
  if(num<0)
    return "minus "+number(-num);
  switch(num)
  {
   case 0:  return "";
   case 1:  return "ena";
   case 2:  return "dva";
   case 3:  return "tri";
   case 4:  return "¹tiri";
   case 5:  return "pet";
   case 6:  return "¹est";
   case 7:  return "sedem";
   case 8:  return "osem";
   case 9:  return "devet";
   case 10: return "deset";
   case 11..19: return number(num%10)+"najst";
   case 20: return "dvajset";
   case 30: case 40: case 50: case 60: case 70: case 80: case 90:
     return number(num/10)+"deset";
   case 21..29: case 31..39: case 41..49: case 51..59: case 61..69:
   case 71..79: case 81..89: case 91..99:
     return number(num%10)+"in"+number((num/10)*10);
   case 100: return "sto";
   case 101..199: return "sto "+number(num%100);
   case 200..299: return "dvesto "+number(num%100);
   case 300..999: return number(num/100)+"sto "+number(num%100);
   case 1000: return "tisoè";
   case 1001..1999: return "tisoè "+number(num%1000);
   case 2000..999999: return number(num/1000)+" tisoè "+number(num%1000);
   case 1000000..1999999:
     return "milijon "+number(num%1000000);
   case 2000000..2999999: 
     return number(num/1000000)+" milijona"+number(num%1000000);
   case 3000000..4999999:
     return number(num/1000000)+" milijone"+number(num%1000000);
   case 5000000..999999999:
     return number(num/1000000)+" milijonov"+number(num%1000000);
     if ( ((num%10000000)/1000000)==1 ) return number(num/1000000)+" milijon "+number(num%1000000);
   default:
     return "veliko";
  }
}

mapping(int:string) small_orders = ([ 1: "prvi", 2: "drugi", 3: "tretji",
                                     4: "èetrti", 7: "sedmi", 8: "osmi" ]);

string ordered(int i)
{
  int rest2 = i%1000000;
  int rest1 = i%1000;
  int rest = i%100;
  int base = i-rest;
  if (!i) {
    return("napacen");
  }
  if (!rest2) {
    return replace(number(i)+"ti"," ","");
  }
  if (!rest1) {
    return replace(number(i)+"i"," ","");
  }
  if (!rest) {
    return replace(number(i)+"ti"," ","");
  }
  if (small_orders[rest])
    return replace((base ? (number(base)+" ") : "")+small_orders[rest]," ","");
  else
    return replace(number(i)+"i"," ","");
}


string date(int timestamp, string|void m)
{
  mapping t1=localtime(timestamp);
  mapping t2=localtime(time(0));

  if(m=="full")
    return ctime(timestamp)[11..15]+", "+
           (t1["mday"]) + ". "
           + month(t1["mon"]+1) + " " +(t1["year"]+1900);

  if(m=="date")
    return (t1["mday"]) + ". " + month(t1["mon"]+1)
      + " " + (t1["year"]+1900);

  if(m=="time")
    return ctime(timestamp)[11..15];

  // !m

  if(t1["yday"] == t2["yday"] && t1["year"] == t2["year"])
    return "danes, "+ ctime(timestamp)[11..15];
  
  if(t1["yday"]+1 == t2["yday"] && t1["year"] == t2["year"])
    return "vèeraj, "+ ctime(timestamp)[11..15];
  
  if(t1["yday"]-1 == t2["yday"] && t1["year"] == t2["year"])
    return "danes, "+ ctime(timestamp)[11..15];
  
  if(t1["year"] != t2["year"])
    return (month(t1["mon"]+1) + " " + (t1["year"]+1900));

  return (t1["mday"]+1 + ". " + month(t1["mon"]+1));
}
