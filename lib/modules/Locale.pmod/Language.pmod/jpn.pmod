#pike __REAL_VERSION__

#charset iso-2022

//! Japanese language locale.

// $Id: jpn.pmod,v 1.2 2004/05/16 10:06:36 nilsson Exp $

inherit "abstract";

constant name = "日本語";
constant english_name = "japanese";
constant iso_639_1 = "ja";
constant iso_639_2 = "jpn";
constant iso_639_2B = "jpn";

constant aliases =  ({ "ja", "jp", "jpn", "japanese", "nihongo",
			"日本語" });

constant required_charset = "iso-2022";

/* The following function is correct for -10**12 < n < 10**12 (I think...) */

string mknumber(int n)
{
  array(string) digit;
  string r;
  digit = ({ "", "一", "二", "三", "四", "五", "六", "七", "八", "九" });

  if(!n) return "ゼロ";

  if(n<0) return "負"+mknumber(-n);

  if(n>=200000000)
    return mknumber(n/100000000)+"億"+mknumber(n%100000000);
  else if(n>100000000)
    return "億"+mknumber(n%100000000);
  else if(n==100000000)
    return "億";

  if(n>=20000)
    return mknumber(n/10000)+"万"+mknumber(n%10000);
  else if(n>10000)
    return "万"+mknumber(n%10000);
  else if(n==10000)
    return "万";

  r = "";

  if(n>=2000)
    r += digit[n/1000]+"千";
  else if(n>=1000)
    r += "千";

  n %= 1000;
  if(n>=200)
    r += digit[n/100]+"百";
  else if(n>=100)
    r += "百";

  n %= 100;
  if(n>=20)
    r += digit[n/10]+"十";
  else if(n>=10)
    r += "十";

  return r + digit[n%10];
}


string ordered(int i)
{
  return mknumber(i)+"番";
}

string date(int timestamp, string|void m)
{
  mapping t1=localtime(timestamp);
  mapping t2=localtime(time(0));

  if(m=="full")
    return ctime(timestamp)[11..15]+
      mknumber(t1["year"]+1900)+"年"+mknumber(t1["mon"]+1)+
       "月"+mknumber(t1["mday"])+"日";

  if(m=="date")
    return mknumber(t1["year"]+1900)+"年"+mknumber(t1["mon"]+1)+
       "月"+mknumber(t1["mday"])+"日";

  if(m=="time")
    return ctime(timestamp)[11..15];

  // !m

  if(t1["yday"] == t2["yday"] && t1["year"] == t2["year"])
    return "今日" + ctime(timestamp)[11..15];

  if(t1["yday"]+1 == t2["yday"] && t1["year"] == t2["year"])
    return "昨日" + ctime(timestamp)[11..15];

  if(t1["yday"]-1 == t2["yday"] && t1["year"] == t2["year"])
    return "明日" + ctime(timestamp)[11..15];

  if(t1["year"] == t2["year"])
    return mknumber(t1["mon"]+1)+"月" + mknumber(t1["mday"])+"日";
  if(t1["year"]+1 == t2["year"])
    return "旧年" + mknumber(t1["mon"]+1)+"月" + mknumber(t1["mday"])+"日";
  if(t1["year"]-1 == t2["year"])
    return "次年" + mknumber(t1["mon"]+1)+"月" + mknumber(t1["mday"])+"日";
  return mknumber(t1["year"]+1900)+"年" + mknumber(t1["mon"]+1)+"月" +
    mknumber(t1["mday"])+"日";
}


string number(int num)
{
  return mknumber(num);
}

string month(int num)
{
  return mknumber(num)+"月";
}

string short_month(int num)
{
  return month(num);
}

string day(int num)
{
  return ({ "日", "月", "火", "水", "木", "金", "土" })[ num - 1 ]+
	    "曜日";
}

string short_day(int num)
{
  return ({ "日", "月", "火", "水", "木", "金", "土" })[ num - 1 ];
}
