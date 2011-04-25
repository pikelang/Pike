#pike __REAL_VERSION__


//! English language locale.

// $Id$

inherit "abstract";

constant name = "english";
constant english_name = "english";
constant iso_639_1 = "en";
constant iso_639_2 = "eng";
constant iso_639_2B = "eng";

constant aliases = ({ "en", "eng", "english" });

constant months = ({
  "January", "February", "March", "April", "May",
  "June", "July", "August", "September", "October",
  "November", "December" });

constant days = ({
  "Sunday","Monday","Tuesday","Wednesday",
  "Thursday","Friday","Saturday" });

string ordered(int i)
{
  switch(i)
  {
   case 0:
    return "buggy";
   case 1:
    return "1st";
   case 2:
    return "2nd";
   case 3:
    return "3rd";
   default:
    if((i%100 > 10) && (i%100 < 14))
      return i+"th";
    if((i%10) == 1)
      return i+"st";
    if((i%10) == 2)
      return i+"nd";
    if((i%10) == 3)
      return i+"rd";
    return i+"th";
  }
}

string date(int timestamp, string|void m)
{
  mapping t1=localtime(timestamp);
  mapping t2=localtime(time(0));

  if(m=="full")
    return ctime(timestamp)[11..15]+", "+
           month(t1["mon"]+1) + " the "
           + ordered(t1["mday"]) + ", " +(t1["year"]+1900);

  if(m=="date")
    return month(t1["mon"]+1) + " the "  + ordered(t1["mday"])
      + " in the year of " +(t1["year"]+1900);

  if(m=="time")
    return ctime(timestamp)[11..15];

  // !m

  if(t1["yday"] == t2["yday"] && t1["year"] == t2["year"])
    return "today, "+ ctime(timestamp)[11..15];

  if(t1["yday"]+1 == t2["yday"] && t1["year"] == t2["year"])
    return "yesterday, "+ ctime(timestamp)[11..15];

  if(t1["yday"]-1 == t2["yday"] && t1["year"] == t2["year"])
    return "tomorrow, "+ ctime(timestamp)[11..15];

  if(t1["year"] != t2["year"])
    return (month(t1["mon"]+1) + " " + (t1["year"]+1900));

  return (month(t1["mon"]+1) + " " + ordered(t1["mday"]));
}


string number(int num)
{
  if(num<0)
    return "minus "+number(-num);
  switch(num)
  {
   case 0:  return "zero";
   case 1:  return "one";
   case 2:  return "two";
   case 3:  return "three";
   case 4:  return "four";
   case 5:  return "five";
   case 6:  return "six";
   case 7:  return "seven";
   case 8:  return "eight";
   case 9:  return "nine";
   case 10: return "ten";
   case 11: return "eleven";
   case 12: return "twelve";
   case 13: return "thirteen";
   case 14: return "fourteen";
   case 15: return "fifteen";
   case 16: return "sixteen";
   case 17: return "seventeen";
   case 18: return "eighteen";
   case 19: return "nineteen";
   case 20: return "twenty";
   case 30: return "thirty";
   case 40: return "forty";
   case 50: return "fifty";
   case 80: return "eighty";
   case 60: case 70: case 90:
     return number(num/10)+"ty";
   case 21..29: case 31..39: case 41..49:
   case 51..59: case 61..69: case 71..79:
   case 81..89: case 91..99:
     return number((num/10)*10)+number(num%10);
   case 100: case 200: case 300: case 400: case 500:
   case 600: case 700: case 800: case 900:
     return number(num/100)+" hundred";
   case 101..199: case 201..299: case 301..399: case 401..499:
   case 501..599: case 601..699: case 701..799: case 801..899:
   case 901..999:
     return number(num/100)+" hundred and "+number(num%100);
   case 1001..1099:
     return number(num/1000)+" thousand and "+number(num%1000);
   case 1000:
     return "one thousand";
   case 1100..999999:
     return number(num/1000)+" thousand "+number(num%1000);
   case 1000001..1000099:
     return number(num/1000000)+" million and "+number(num%1000000);
   case 1000000:
     return "one million";
   case 1000100..999999999:
     return number(num/1000000)+" million "+number(num%1000000);
   default:
    return "many";
  }
}
