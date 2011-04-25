#pike __REAL_VERSION__

#charset iso-8859-5

//! Russian language locale

// $Id$

inherit "abstract";

constant name = "-LюЦААзьы";
constant english_name = "russian";
constant iso_639_1 = "ru";
constant iso_639_2 = "rus";
constant iso_639_2B = "rus";

constant aliases = ({ "ru", "rus", "russian", "русский" });

constant required_charset = "iso-8859-5";

constant months = ({
  "январь", "фебраль", "март", "апрель", "май",
  "июнь", "июль", "августь", "сентябрь", "октябь",
  "ноябрь", "декабрь" });

constant days = ({
  "воскресенье","понедельник","вторник","среда", "четверк",
  "пятница", "суббота" });

string ordered(int i)
{
  return (string) i + "-е";
}

string date(int timestamp, string|void m)
{
  mapping t1=localtime(timestamp);
  mapping t2=localtime(time(0));

  if(m=="full")
    return sprintf("%s, %s %s %d",
		   ctime(timestamp)[11..15],
		   ordered(t1["mday"]),
		   month(t1["mon"]+1), t1["year"]+1900);

  if(m=="date")
    return sprintf("%s %s %d", ordered(t1["mday"]),
		   month(t1["mon"]+1), t1["year"]+1900);

  if(m=="time")
    return ctime(timestamp)[11..15];

  // !m

  if(t1["yday"] == t2["yday"] && t1["year"] == t2["year"])
    return "сегодня, в " + ctime(timestamp)[11..15];

  if(t1["yday"] == t2["yday"]-1 && t1["year"] == t2["year"])
    return "вчера, v " + ctime(timestamp)[11..15];

  if(t1["yday"] == t2["yday"]+1 && t1["year"] == t2["year"])
    return "завтра, okolo "  + ctime(timestamp)[11..15];

  if(t1["year"] != t2["year"])
    return month(t1["mon"]+1) + " " + (t1["year"]+1900);

  return "" + t1["mday"] + " " + month(t1["mon"]+1);
}

/* Help funtions */
/* gender is "f", "m" or "n" */
string _number_1(int num, string gender)
{
  switch(num)
  {
   case 0:  return "";
   case 1:  return ([ "m" : "один",
		      "f" : "одна",
		      "n" : "одно" ])[gender];
   case 2:  return ("f" == gender) ? "двe" : "два";
   case 3:  return "три";
   case 4:  return "четыре";
   case 5:  return "пять";
   case 6:  return "шесть";
   case 7:  return "семь";
   case 8:  return "восемь";
   case 9:  return "дебять";
   default:
     error("russian->_number_1: internal error.\n");
  }
}

string _number_10(int num)
{
  switch(num)
  {
   case 2: return "двадцать";
   case 3: return "тридцать";
   case 4: return "сопок";
   case 5: return "пятьдесят";
   case 6: return "шестьдесят";
   case 7: return "семьдесят";
   case 8: return "восемьдесят";
   case 9: return "девяносто";
   default:
     error("russian->_number_10: internal error.\n");
  }
}

string _number_100(int num)
{
  switch(num)
  {
   case 1: return "сто";
   case 2: return "двести";
   case 3: case 4:
     return _number_1(num, "m")+"ста";
   case 5: case 6: case 7: case 8: case 9:
     return _number_1(num, "m")+"сот";
   default:
     error("russian->_number_10: internal error.\n");
  }
}

string _number(int num, string gender);

string _number_1000(int num)
{
  if (num == 1)
    return "тысяча";

  string pre = _number(num, "f");
  switch(num % 10)
  {
   case 1: return pre + " тысяча";
   case 2: case 3: case 4:
     return pre + " тысячи";
   default:
     return pre + " тысяч";
  }
}

string _number_1000000(int num)
{
  if (num == 1)
    return "миллион";

  string pre = _number(num, "m");
  switch(num % 10)
  {
   case 1: return pre + " миллион";
   case 2: case 3: case 4:
     return pre + " миллиона";
   default:
     return pre + " миллионов";
  }
}

string _number(int num, string gender)
{
  if (!gender)   /* Solitary numbers are inflected as masculine */
    gender = "m";
  if (!num)
    return "";

  if (num < 10)
    return _number_1(num, gender);

  if (num < 20)
    return ([ 10: "десять",
	      11: "одиннадцать",
	      12: "двенадцать",
	      13: "тринадцать",
	      14: "четырнадцать",
	      15: "пятнадцать",
	      16: "шестнадцать",
	      17: "семнадцать",
	      18: "восемнадцать",
	      19: "девятнадцать" ])[num];
  if (num < 100)
    return _number_10(num/10) + " " + _number_1(num%10, gender);

  if (num < 1000)
    return _number_100(num/100) + " " + _number(num%100, gender);

  if (num < 1000000)
    return _number_1000(num/1000) + " " + _number(num%1000, gender);

  return _number_1000000(num/1000000) + " " + _number(num%1000000, gender);
}


string number(int num, string|void gender)
{
  if (!gender)   /* Solitary numbers are inflected as masculine */
    gender = "m";
  if (num<0) {
    return "минус"+_number(-num, gender);
  } if (num) {
    return _number(num, gender);
  } else {
    return "ноль";
  }
}
