/* -*- mode: Pike; c-basic-offset: 3; -*- */

#pike __REAL_VERSION__

//! This is the Badi calendar,
//! used in the Baha'i religion.

inherit Calendar.YMD:YMD;

#include "constants.h"

string calendar_name() { return "Badi"; }

private protected mixed __initstuff=lambda()
{
   f_week_day_shortname_from_number="badi_week_day_shortname_from_number";
   f_week_day_name_from_number="badi_week_day_name_from_number";
   f_year_name_from_number="badi_year_name_from_number";
   f_week_day_number_from_name="badi_week_day_number_from_name";

   f_month_day_name_from_number="badi_month_day_name_from_number";
   f_month_name_from_number="badi_month_name_from_number";
   f_month_shortname_from_number="badi_month_shortname_from_number";
   f_month_number_from_name="badi_month_number_from_name";
   f_week_name_from_number="week_name_from_number";
   f_year_number_from_name="badi_year_number_from_name";
   dwim_year=([ "past_lower":0, "past_upper":0,
                "current_century":0, "past_century":0 ]);
}();

protected int year_leap_year(int y)
{
   // the beginning of the year is the day of the spring equinox
   // leap years are those where an extra day is needed to get up to the next
   // spring equinox.
   // for now this rule is not in effect and the start of the year is march
   // 21st. leap years are thus compatible with the gregorian calendar for the
   // time being.
   y+=1844;
   return (!(((y)%4) || (!((y)%100) && ((y)%400))));
}

// [y,yjd]
protected array year_from_julian_day(int jd)
{
   // in order to avoid coming up with a formula for leapyears in the bahai
   // calendar we add the necessary offsets to the data so we can use the take
   // the gregorian formula
   // gregorian year starts 286 days later march 21st -> january 1st
   int d=jd+286-1721426;

   int century=(4*d+3)/146097;
   int century_jd=(century*146097)/4;
   int century_day=d-century_jd;
   int century_year=(100*century_day+75)/36525;

   return
   ({
      (century*100+century_year+1)-1844, // but 1844 years earlier
      1721426-286+century_year*365+century_year/4+century_jd,
   });
}

protected int julian_day_from_year(int y)
{
   // FIXME: verify for leapyears (esp: 56 (1900) and 100, ...)
   // same as above
   y+=1843; // above we are adding days and subtracting years,
            // going back we need to subtract days and add years.
   return 1721426-286+y*365+y/4-y/100+y/400;
}

protected int compat_week_day(int n)
{
   // this is specific to the gregorian calendar.
   // we just stick with the value as it is
   return n;
}

protected array(int) year_month_from_month(int y,int m)
{
// [y,m,ndays,myd]

   y+=(m-1)/19;
   m=1+(m-1)%19;

   switch (m)
   {
      case 1..17: return ({y,m,19,(m-1)*19+1});
      case    18: return ({y,m,23+year_leap_year(y),(m-1)*19+1});
      case    19: return ({y,m,19,347+year_leap_year(y)});
   }

   error("Month out of range.\n");
}

protected array(int) month_from_yday(int y,int yd)
{
// [month,day-of-month,ndays,month-year-day]
   if (yd<1)
     return month_from_yday(--y, yd+365+year_leap_year(y));
   int ly = year_leap_year(y);
   switch (yd)
   {
      case 1..323: return ({(yd+18)/19, (yd+18)%19+1, 19, ((yd+18)/19-1)*19+1});
      case 324..342:   return ({18, (yd+18)%19+1, 23+ly, ((yd+18)/19-1)*19+1 });
      case 343..346:   return ({18, yd%19+19, 23+ly, ((yd)/19-1)*19+1 });
      case 347: if(ly) return ({18, yd%19+19, 23+ly, ((yd)/19-1)*19+1 });
      case 348..365:   return ({19, yd-346-ly, 19, 347+ly});
      case 366: if(ly) return ({19, yd-346-ly, 19, 347+ly});
   }
   error("yday out of range.\n");
}

protected array(int) week_from_julian_day(int jd)
{
// [year,week,day-of-week,ndays,week-julian-day]

   [int y,int yjd]=year_from_julian_day(jd);
   int yday=jd-yjd+1;
   int wjd=jd-(jd+2)%7;  // +2 is the offset from the julian day week
                         // starting on monday.

   int k=4+(yjd-2)%7;
   int w=(yday+k-1)/7;

   //werror("jd %d: y%O yjd%O yday%O, k%O w%O wjd%O, wd%O:%O\n",jd,y,yjd,yday,k,w,wjd, 1+(jd+2)%7, 1+(1+yjd+yday)%7);

   if (!w)
   {
// handle the case that the day is in the previous year;
// years previous to years staring on saturday,
//              ...  and leap years starting on sunday
      y--;
      w=52+( (k==4) || ( (k==5) && year_leap_year(y) ) );
   }
   else if (w==53 && k>=6-year_leap_year(y) && k<10-year_leap_year(y))
   {
// handle the case that the week is in the next year
      y++;
      w=1;
   }

   //werror("jd %d: y%O yjd%O yday%O, k%O w%O wjd%O, wd%O:%O\n",jd,y,yjd,yday,k,w,wjd, 1+(jd+2)%7, 1+(1+yjd+yday)%7);
   return ({y,w,1+(1+yjd+yday)%7,7,wjd});
}

protected array(int) week_from_week(int y,int w)
{
// [year,week,1 (wd),ndays,week-julian-day]

   int yjd=julian_day_from_year(y);
   int wjd=-6+yjd-(yjd+3)%7;
//   int wjd=-7+yjd-(yjd+2)%7;

   werror("%t: %O, %O, %O, %O\n", this, y, w, wjd, wjd+w*7);
   if (w<1 || w>52) // may or may not be out of this year
      return week_from_julian_day(wjd+w*7);

   return ({y,w,1,7,wjd+w*7});
//   fixme
}

// identical to gregorian
protected int year_remaining_days(int y,int yday)
{
   return 365+year_leap_year(y)-yday;
}

//! @decl int daystart_offset()
//!     Returns the offset to the start of the time range object
//!
int daystart_offset()
{
  return -21600; // 6 hours earlier
}

class cFraction
{
   inherit YMD::cFraction;

   TimeRange make_base()
   {
      base=Day("unix_r",ux-daystart_offset(),rules);
      if (len) base=base->range(Day("unix_r",ux-daystart_offset()+len,rules));
      return base;
   }

   protected void make_local()
   {
      ::make_local();
      ls+=daystart_offset();
   }
}

class cSecond
{
   inherit YMD::cSecond;

   TimeRange make_base()
   {
      base=Day("unix_r",ux-daystart_offset(),rules);
      if (len) base=base->range(Day("unix_r",ux-daystart_offset()+len,rules));
      return base;
   }

   protected void make_local()
   {
      ::make_local();
      ls+=daystart_offset();
   }
}

class cMinute
{
   inherit YMD::cMinute;

   TimeRange make_base()
   {
      base=Day("unix_r",ux-daystart_offset(),rules);
      if (len) base=base->range(Day("unix_r",ux-daystart_offset()+len,rules));
      return base;
   }

   protected void make_local()
   {
      ::make_local();
      ls+=daystart_offset();
   }

/*
   int unix_time()
   {
      int offset=daystart_offset();
      int minuteoffset=offset%60;
      if(houroffset>30)
        offset+=60-minuteoffset;
      else
        offset-=minuteoffset;
      return ::unix_time()+offset;
   }
*/
}

class cHour
{
   inherit YMD::cHour;

   TimeRange make_base()
   {
      base=Day("unix_r",ux-daystart_offset(),rules);
      if (len) base=base->range(Day("unix_r",ux-daystart_offset()+len,rules));
      return base;
   }

   protected void make_local()
   {
      ::make_local();
      ls+=daystart_offset();
   }

/*
   int unix_time()
   {
      int houroffset=daystart_offset()%3600;
      if(houroffset>1800)
        houroffset-=3600;
      int ux=::unix_time();
      werror("cHour:%d:%d:%d\n", daystart_offset(), houroffset, ux);
      return ux-houroffset;
   }
*/
}

class cDay
{
   inherit YMD::cDay;

   void create_unixtime_default(int unixtime)
   {
      ::create_unixtime_default(unixtime-daystart_offset());
   }

   string month_name()
   {
      if (md > 19)
        return rules->language[f_month_name_from_number](0);
      else return ::month_name();
   }

   string month_shortname()
   {
      if (md > 19)
        return rules->language[f_month_shortname_from_number](0);
      else return ::month_shortname();
   }

   int unix_time()
   {
      return ::unix_time()+daystart_offset();
   }

   int number_of_months()
   {
      if (n<=1) return 1;
      if (m==CALUNKNOWN) make_month();
      [int zy,int zyjd]=year_from_julian_day(jd+n-1);
      [int zm,int zmd,int znd,int zmyd]=month_from_yday(zy,jd+n-zyjd);
      return zm-m+1+(zy-y)*19;
   }

   string nice_print()
   {
      if (m==CALUNKNOWN) make_month();
      if (wd==CALUNKNOWN) make_week();
      return
         sprintf("%s %s %s(%d) %s",
                 week_day_shortname(),
                 month_day_name(),month_shortname(), month_no(),
                 year_name());
   }
}

class cWeek
{
   inherit YMD::cWeek;

   // identical to gregorian
   protected int weeks_to_week(int y2,int w2)
   {
      [int y3,int w3,int wd2,int nd2,int jd2]=week_from_week(y2,w2);
      werror("%t: %O, %O, %O, %O, %O\n", this, y2, w2, jd2, jd, (jd2-jd)/7);
      return (jd2-jd)/7;
   }

   int unix_time()
   {
      return ::unix_time()+daystart_offset();
   }

   int number_of_days()
   {
      return 7*n;
   }
}

class cMonth
{
   inherit YMD::cMonth;

   protected int months_to_month(int y2,int m2)
   {
      return (y2-y)*19+(m2-m);
   }

   int unix_time()
   {
      return ::unix_time()+daystart_offset();
   }

   string nice_print()
   {
      return sprintf("%s(%d) %s", month_name(), month_no(), year_name());
   }


   // identical to gregorian
   object(TimeRange)|zero place(TimeRange what,int|void force)
   {
      if (what->is_day)
      {
         int wmd=what->month_day();
         if (md==CALUNKNOWN) make_month();
         if (what->m==2 && m==2 && wmd>=24)
         {
            int l1=year_leap_year(what->y);
            int l2=year_leap_year(y);
            if (l1||l2)
            {
               if (l1 && wmd==24)
                  if (l2) wmd=24;
                  else { if (!force) return 0; }
               else
               {
                  if (l1 && wmd>24) wmd--;
                  if (l2 && wmd>24) wmd++;
               }
            }
         }
         if (!force && wmd>number_of_days()) return 0;
         return Day("ymd_yd",rules,y,yjd,jd+wmd-1,yd+wmd-1,what->n);
      }

      return ::place(what);
   }
}

class cYear
{
   inherit YMD::cYear;

   // identical to gregorian
   int number_of_days()
   {
      switch (n)
      {
         case 0: return 0;
         case 1: return 365+leap_year();
         default:
            return julian_day_from_year(y+n)-yjd;
      }
   }

   // identical to gregorian
   int number_of_weeks()
   {
      if (!n) return 1;
      if (n==1) return 53+(yjd%7==5 && leap_year());
      return
         Week("julian",jd)
         ->range(Week("julian",julian_day_from_year(y+n)-1))
         ->number_of_weeks();
   }

   int number_of_months()
   {
      return 19*n;
   }

   int unix_time()
   {
      return ::unix_time()+daystart_offset();
   }

   // identical to gregorian
   object(TimeRange)|zero place(TimeRange what,void|int force)
   {
      if (what->is_day)
      {
         int yd=what->yd;
         if (yd>=55)
            switch (year_leap_year(what->y)*10+year_leap_year(y))
            {
               case 00:
               case 11:
                  break;
               case 10: /* from leap to non-leap */
                  if (yd==55 && !force) return 0; // not this year
                  yd--;
                  break;
               case 01: /* from non-leap to leap */
                  yd++;
                  break;
            }
         return Day("ymd_yd",rules,y,yjd,yjd+yd-1,yd,what->n);
      }

      return ::place(what);
   }

   Vahid vahid(void|int m)
   {
      if (!m || (!n&&m==-1))
         return Vahid("ymd_yn",rules,y,1);

      if (m<0) m=number_of_vahids()+m;

      if (m<2) return Vahid("ymd_yn",rules,y,1);

      error("not in range (Vahid 0..%d exist)\n",
            number_of_vahids()-1);
   }

   int number_of_vahids()
   {
     return 0;
   }
}

//!
class Vahid
{
   inherit YMD;
   constant is_vahid=1;
   int v;   // vahid
   int vjd; // julian day of the first day of the vahid

   protected void create(mixed ...args)
   {
      if (!sizeof(args))
      {
         create_now();
         v=y/19+1;
         y=v*19-18;
         return;
      }
      else switch (args[0])
      {
         case "ymd_v":
            rules=args[1];
            v=args[2];
            n=args[3];
            jd=vjd=julian_day_from_vahid(v);
            m=md=w=wd=CALUNKNOWN;
            return;
         case "ymd_yn":
            rules=args[1];
            y=args[2];
            n=args[3];
            m=md=w=wd=CALUNKNOWN;
            yd=1;
            v=y/19+1;
            jd=vjd=julian_day_from_vahid(v);
            return;
     }
   }

   int julian_day_from_vahid(int v)
   {
     return julian_day_from_year(v*19-18);
   }

   void create_julian_day(int|float _jd)
   {
      [y,yjd]=year_from_julian_day((int)_jd);
      [v,vjd]=vahid_from_julian_day((int)_jd);
      jd=vjd;
      n=1;
      md=yd=m=1;
      wd=w=CALUNKNOWN; // unknown
   }

   array(int) vahid_from_julian_day(int _jd)
   {
      [int _y, int _yjd]=year_from_julian_day(_jd);
      return ({ _y/19+1, julian_day_from_year((_y/19+1)*19-18) });
   }

   int unix_time()
   {
      return ::unix_time()+daystart_offset();
   }

   string vahid_name()
   {
      return sprintf("v%d", v);
   }

   protected string _sprintf(int t)
   {
      switch (t)
      {
         case 'O':
//            if (n!=1)
//               return sprintf("Vahid(%s)",nice_print_period());
            return sprintf("Vahid(%s)",nice_print());
         case 't':
            return "Calendar."+calendar_name()+".Vahid";
         default:
            return ::_sprintf(t);
      }
   }

   string nice_print()
   {
      return vahid_name();
   }

//! @decl Year year()
//! @decl Year year(int n)
//! @decl Year year(string name)
//!     Return a year in the vahid by number or name:
//!
//!     @tt{vahid->year("Alif")@}
   cYear year(int|string ... yp)
   {
      int num;
      if (sizeof(yp) &&
          stringp(yp[0]))
      {
         num=((int)yp[0]) ||
            rules->language[f_year_number_from_name](yp[0]);
         if (!num)
            error("no such year %O in %O\n",yp[0],this);
      }
      else if(sizeof(yp))
         num=yp[0];

      if (!sizeof(yp) || (num==-1 && !n))
         return Year("ymd_y",rules,v*19-18,yd,1);

      if (num<0) num+=1+number_of_years();
      if(0 < num && num < 20)
        return Year("ymd_yn",rules,v*19-18+num-1,1);

      error("not in range; Year 1..%d exist in %O\n",
            number_of_years(),this);
   }

   array(cYear) years(int ...range)
   {
      int from=1,n=number_of_years(),to=n;

      if (sizeof(range))
         if (sizeof(range)<2)
            error("Illegal numbers of arguments to years()\n");
         else
         {
            [from,to]=range;
            if (from>n) return ({}); else if (from<1) from=1;
            if (to>n) to=n; else if (to<from) return ({});
         }

      return map(enumerate(1+to-from,1,from+m-1),
                 lambda(int x)
                 { return Year("ymd_yn",rules,v*19-18+x-1,1); });
   }

   int number_of_years()
   {
     return 19*n;
   }

   int number_of_days()
   {
     return (this+1)->julian_day()-julian_day();
   }

   TimeRange _move(int m,YMD step)
   {
      if (!step->n || !m)
         return this;

      if (step->is_vahid)
         return Vahid("ymd_v",rules,v+m*step->n,n)
            ->autopromote();

      if (step->is_year)
         return year()->add(m,step)->set_size(this);
/*
      if (step->is_month)
         return month()->add(m,step)->set_size(this);

//        if (step->is_week)
//       return week()->add(m,step)->set_size(this);
*/
      if (step->is_ymd)
         return Day("ymd_jd",rules,
                    vjd+m*step->number_of_days(),number_of_days())
            ->autopromote();

      error("_move: Incompatible type %O\n",step);
   }

   YMD autopromote() { return this; }
}
