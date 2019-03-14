#pike __REAL_VERSION__

inherit Calendar_I.Gregorian;

class Year
{
  inherit Calendar_I.Gregorian.Year;

  int julian_day(int d) // jd%7 gives weekday, mon=0, sun=6
  {
    return 1721424 + d + (36525*(y-1))/100;
  }

  int leap()
  {
    return !(y%4);
  }

}

class Day
{
  inherit Calendar_I.Gregorian.Day;

  void create(int ... arg)
  {
    if (!sizeof(arg))
    {
      int jd = Calendar_I.Gregorian.Day()->julian_day()-1721424;
      y = jd*100/36525+1;
      d = jd-(y-1)*36525/100;
    }
    else
      ::create(@arg);
  }

}
