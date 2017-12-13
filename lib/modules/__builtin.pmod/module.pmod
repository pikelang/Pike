#pike __REAL_VERSION__
#pragma strict_types

inherit _static_modules.Builtin;

#define NANOSECONDS		1000000000

//! @param timezone
//!  Timezone in seconds @b{west@} from UTC
//!   (must include daylight-saving time adjustment).
//! @returns
//!   ISO formatted timezone @b{east@} from UTC.
//! @seealso
//!  @[localtime()]
final string iso_timezone(int timezone) {
  if (timezone) {
    string res;
    if (timezone < 0)
      timezone = -timezone, res = "+";
    else
      res = "-";
    res += sprintf("%02d", timezone / 3600);
    if (timezone %= 3600) {
      res += sprintf(":%02d", timezone / 60);
      if (timezone %= 60)
        res += sprintf(":%02d", timezone);
    }
    return res;
  }
  return "";
}

//! @param tm
//!   Standard tm mapping, optionally extended with @expr{nsec@} for
//!   nanoseconds.
//! @returns
//!   ISO formatted time.
//! @seealso
//!  @[mktime()]
string iso_time(mapping(string:int) tm) {
  string res = sprintf("%02d:%02d", tm->hour, tm->min);
  int usec = tm->nsec;
  if (tm->sec || usec) {
    res += sprintf(":%02d", tm->sec);
    if (usec) {
      int msec;
      res += (usec /= 1000) * 1000 != tm->nsec
        ? sprintf(".%09d", tm->nsec)
        : (msec = usec / 1000) * 1000 != usec
          ? sprintf(".%06d", usec)
          : sprintf(".%03d", msec);
    }
  }
  return res;
}

//!  Base class for the time related types.
//! @seealso
//!  @[Timestamp], @[Time]
class Timebase {

  //!  Nanoseconds since epoch (for absolute time classes,
  //!  epoch equals @expr{1970/01/01 00:00:00 UTC@}).
  int nsecs;

  //! @param sec
  //!   Seconds since epoch.
  //! @param nsec
  //!   Nanoseconds since epoch.
  variant protected void create() {
  }
  variant protected void create(object/*this_program*/ copy) {
    nsecs = [int]copy->nsecs;
  }
  variant protected void create(int|float sec, void|int nsec) {
    nsecs = (int)(sec * NANOSECONDS + nsec);
  }

  //! Microseconds since epoch; reflects the equivalent value as the basic
  //! member @[nsecs].
  int|float `usecs() {
    return nsecs % 1000 ? nsecs / 1000.0 : nsecs / 1000;
  }

  //! Microseconds since epoch; actually modifies the basic
  //! member @[nsecs].
  int `usecs=(int usec) {
    nsecs = 1000 * usec;
    return usec;
  }

  protected mixed `+(mixed that) {
    if (objectp(that) && ([object]that)->is_interval) {
      if (([object]that)->days || ([object]that)->months)
        error("Adding days or months not supported yet\n");
      this_program n = this_program(this);
      n->nsecs += [int]([object]that)->nsecs;
      return n;
    } else if (!intp(that) && !floatp(that))
      error("Cannot add %O\n", that);
    this_program n = this_program();
    n->nsecs = (int)(nsecs + [float|int]that * NANOSECONDS);
    return n;
  }

  protected mixed `-(mixed that) {
    if (objectp(that) && ([object]that)->is_interval) {
      if (([object]that)->days || ([object]that)->months)
        error("Adding days or months not supported yet\n");
      this_program n = this_program(this);
      n->nsecs -= [int]([object]that)->nsecs;
      return n;
    } else if (!intp(that) && !floatp(that))
      error("Cannot substract %O\n", that);
    this_program n = this_program();
    n->nsecs = (int)(nsecs - [float|int]that * NANOSECONDS);
    return n;
  }

  protected int(0..1) `<(mixed that) {
    return
       intp(that) ? nsecs < [int]that * NANOSECONDS
     : floatp(that) ? nsecs < [float]that * NANOSECONDS
     : objectp(that) && nsecs < [int]([object]that)->nsecs
        && !zero_type(([object]that)->nsecs);
  }

  protected int(0..1) `==(mixed that) {
    return objectp(that)
     ? nsecs == [int]([object]that)->nsecs && !zero_type(([object]that)->nsecs)
     : intp(that) ? nsecs == [int]that * NANOSECONDS
     : floatp(that) && (float)nsecs == [float]that * NANOSECONDS;
  }

  public mapping(string:int) tm() {
    return (["nsec": abs(nsecs) % NANOSECONDS ]);
  }

  //! Can be casted to string, float and int.
  //! Casting it to float and int return unix-time values.
  //!  @seealso
  //!    @[mktime()], @[gmtime()]
  protected mixed cast(string to) {
    switch (to) {
      case "string":
        return (nsecs < 0 ? "-" : "") + iso_time(tm());
      case "float":
        return nsecs / NANOSECONDS.0;
      case "int":
        return nsecs / NANOSECONDS;
      default:
        return UNDEFINED;
    }
  }

  protected string _sprintf(int fmt, mapping(string:mixed) params) {
    switch (fmt) {
      case 's': return (string)this;
      default: return sprintf(sprintf("%%*%c", fmt), params, 0);
    }
  }
}

//! Lightweight time type.  Stores absolute times in a day
//! with nanosecond resolution.  Normalised value range is between
//! @expr{00:00:00.000000000@} and @expr{23:59:59.999999999@}.
//! Values outside this range are accepted, and have arithmetically
//! sound results.
//! @seealso
//!  @[Interval], @[Date], @[TimeTZ], @[Range]
class Time {
  inherit Timebase;

  //! @param hour
  //!   Hours since epoch.
  //! @param min
  //!   Minutes since epoch.
  //! @param sec
  //!   Seconds since epoch.
  //! @param nsec
  //!   Nanoseconds since epoch.
  variant
   protected void create(int hour, int min, void|int sec, void|int nsec) {
    nsecs = ((((hour * 60) + min) * 60 + sec) * NANOSECONDS) + nsec;
  }
  variant protected void create(object/*this_program*/ copy) {
    ::create(copy);
  }
  variant protected void create(int sec) {
    nsecs = sec * NANOSECONDS;
  }

  //! @returns
  //!   A tm-like mapping which describes the time.  The mapping will
  //!   include an extra @expr{nsec@} component.
  //! @seealso
  //!  @[mktime()], @[gmtime()]
  public mapping(string:int) tm() {
    int hourleft;
    int secleft = (hourleft = abs(nsecs) / NANOSECONDS) % 60;
    int minleft = (hourleft /= 60) % 60;
    hourleft /= 60;
    return ::tm() + (["hour":hourleft, "min":minleft, "sec":secleft]);
  }

  protected string _sprintf(int fmt, mapping(string:mixed) params) {
    if (fmt == 'O')
      return sprintf("Time(%s)", (string)this);
    return ::_sprintf(fmt, params);
  }
}

//!  Lightweight time type with timezone.  Equivalent to @[Time], but
//!  stores a timezone offset as well.
//! @seealso
//!  @[Time], @[Date], @[Interval], @[Range]
class TimeTZ {
  inherit Time;

  //!   Timezone offset in seconds @b{west@} from UTC
  //!   (includes daylight-saving time adjustment).
  //! @note
  //!   ISO time format displays the timezone in seconds @b{east@} from UTC.
  //! @seealso
  //!  @[localtime()]
  int timezone;

  variant protected void create(object/*this_program*/ copy) {
    ::create(copy);
    timezone = [int]copy->timezone;
  }

  public mapping(string:int) tm() {
    return ::tm() + (["timezone":timezone]);
  }

  protected mixed cast(string to) {
    if (to == "string")
      return ::cast(to) + iso_timezone(timezone);
    return ::cast(to);
  }

  protected string _sprintf(int fmt, mapping(string:mixed) params) {
    if (fmt == 'O')
      return sprintf("TimeTZ(%s)", (string)this);
    return ::_sprintf(fmt, params);
  }
}

//! Lightweight time and date interval type.
//! It stores the interval in integers of nanoseconds, days and months.
//! @note
//!  Depending on daylight-saving time, a day may not equal 24 hours.
//! @note
//!  The number of days in a month depends on the the time of the year.
//! @seealso
//!  @[Timestamp], @[Date]
class Interval {
  inherit Time;

  constant is_interval = 1;

  //!  Number of days; may not be equal to 24 hours per day, depending
  //!  on daylight-saving time.
  int days;

  //!  Number of months; the number of days per month varies accordingly.
  int months;

  variant protected void create(Interval copy) {
    ::create(copy);
    days = copy->days;
    months = copy->months;
  }

  protected mixed `*(mixed that) {
    Interval n = Interval(this);
    if (intp(that)) {
      n->nsecs *= that;
      n->days *= that;
      n->months *= that;
    } else if (floatp(that)) {
      n->nsecs = (int)(nsecs * that);
      n->days = (int)(days * that);
      n->months = (int)(months * that);
      if (days && n->days % days || months && n->months % months)
        error("Cannot create fractional days or months\n");
    } else
      error("Cannot add %O\n", that);
    return n;
  }

  protected mixed `/(mixed that) {
    if (!intp(that) && !floatp(that))
      error("Cannot divide by %O\n", that);
    Interval n = Interval(this);
    n->nsecs = (int)(nsecs / that);
    n->days = (int)(days / that);
    n->months = (int)(months / that);
    if (days && n->days % days || months && n->months % months)
      error("Cannot create fractional days or months\n");
    return n;
  }

  protected mixed `+(mixed that) {
    if (!objectp(that) || !([object]that)->is_interval)
      error("Cannot add %O\n", that);
    Interval n = Interval(this);
    n->nsecs += ([object]that)->nsecs;
    n->days += ([object]that)->days;
    n->months += ([object]that)->months;
    return n;
  }

  protected mixed `-(mixed that) {
    if (!objectp(that) || !([object]that)->is_interval)
      error("Cannot substract %O\n", that);
    Interval n = Interval(this);
    n->nsecs -= ([object]that)->nsecs;
    n->days -= ([object]that)->days;
    n->months -= ([object]that)->months;
    return n;
  }

  protected int(0..1) `<(mixed that) {
    return objectp(that) && ([object]that)->is_interval
     &&
      (  months <= ([object]that)->months && days <= ([object]that)->days
         && nsecs  < ([object]that)->nsecs
      || months <= ([object]that)->months && days  < ([object]that)->days
         && nsecs <= ([object]that)->nsecs
      || months  < ([object]that)->months && days <= ([object]that)->days
         && nsecs <= ([object]that)->nsecs);
  }

  protected int(0..1) `==(mixed that) {
    return objectp(that) && ([object]that)->is_interval
     && months == [int]([object]that)->months
     && days == [int]([object]that)->days
     && nsecs == [int]([object]that)->nsecs;
  }

  //! @returns
  //!   A tm-like mapping which describes the interval.  The mapping will
  //!   include an extra @expr{nsec@} component, and optionally
  //!   @expr{isnegative = 1@} if the interval is a negative time interval.
  //! @seealso
  //!  @[mktime()], @[gmtime()]
  public mapping(string:int) tm() {
    return ::tm() + (["mon":months, "mday":days])
     + (nsecs < 0 ? (["isnegative":1]) : ([]));
  }

  //! Casting intervals with @expr{days@} or @expr{months@} to @expr{int@}
  //! or @expr{float@} is not possible since the units are not constant.
  //! Casting an interval to @expr{string@} will return a value which
  //! is SQL-compliant.
  protected mixed cast(string to) {
    switch (to) {
      case "string": {
        string res = months ? sprintf("%d MONTH ", months) : "";
        if (days)
          res += sprintf("%d DAY ", days);
        return res + ::cast(to);
      }
      case "float":
      case "int":
        if (months || days)
          error("Interval contains variable units and cannot be casted\n");
    }
    return ::cast(to);
  }

  protected string _sprintf(int fmt, mapping(string:mixed) params) {
    if (fmt == 'O')
      return sprintf("Interval(%s)", (string)this);
    return ::_sprintf(fmt, params);
  }
}

//! Lightweight time and date type.  The values point at absolute points
//! in time.  The values are stored internally with nanosecond resolution
//! since epoch (@expr{1970/01/01 00:00:00 UTC@}).
//! @seealso
//!  @[Interval], @[Date], @[Range], @[localtime()], @[mktime()]
class Timestamp {
  inherit Timebase;
  constant is_timestamp = 1;

  //! @param year
  //!   Absolute year (e.g. 1900 == 1900, 2000 = 2000, 2017 == 2017).
  //! @param month
  //!   Month of the year (January == 1 ... December == 12).
  //! @param day
  //!   Day of the month (typically between 1 and 31).
  //! @param hour
  //!   Hour of the day (typically between 0 and 23).
  //! @param min
  //!   Minutes (typically between 0 and 59).
  //! @param sec
  //!   Seconds (typically between 0 and 59).
  //! @param nsec
  //!   Nanoseconds (typically between 0 and 999999999).
  //! @note
  //!   If any of these values are offered in a denormalised range,
  //!   they will be normalised relative to the startdate offered.
  //!   I.e. it allows you to do year/month/day/hour/minute/second/nanosecond
  //!   arithmetic in a sane way.
  //! @seealso
  //!  @[localtime()], @[mktime()]
  variant protected void create(int year, int month, int day,
   void|int hour, void|int min, void|int sec, void|int nsec) {
    create((["year":year - 1900, "mon":month - 1, "mday":day, "hour":hour,
             "min":min, "sec":sec, "nsec":nsec
           ]));
  }
  variant protected void create(mapping(string:int) tm) {
    create(mktime(tm), tm->nsec);
  }
  variant protected void create(int unix_time, void|int nsec) {
    ::create(unix_time, nsec);
  }

  inline protected int(0..1) `<(mixed that) {
    return intp(that) ? (int)this < that : ::`<(that);
  }

  //! @returns
  //!  The same as @[localtime()], but augmented
  //!  by an extra member called @expr{nsec@}.
  //! @seealso
  //!  @[localtime()]
  public mapping(string:int) tm() {
    return localtime((int)this) + (["nsec": nsecs % NANOSECONDS ]);
  }

  //! When cast to string it returns an ISO formatted timestamp
  //! that includes daylight-saving and timezone corrections.
  protected mixed cast(string to) {
    switch (to) {
      case "string": {
        mapping(string:int) t = tm();
        string res = sprintf("%04d/%02d/%02d",
         t->year + 1900, t->mon+1, t->mday);
        if (t->hour || t->min || t->sec || t->nsec)
          res += " " + iso_time(t);
        return res + iso_timezone(t->timezone);
      }
    }
    return ::cast(to);
  }

  protected string _sprintf(int fmt, mapping(string:mixed) params) {
    if (fmt == 'O')
      return sprintf("Timestamp(%s)", (string)this);
    return ::_sprintf(fmt, params);
  }
}

//!  Lightweight date type.  Stores internally in days since epoch.
//! @seealso
//!  @[Interval], @[Timestamp], @[Time], @[TimeTZ], @[Range]
class Date {
  constant is_date = 1;

  //! Since 1970/01/01 (epoch).
  int days;

  //!
  variant protected void create(int year, int month, int day) {
    create((["year":year - 1900, "mon":month - 1, "mday":day, "timezone":0]));
  }
  variant protected void create(object/*this_program*/ copy) {
    days = [int]copy->days;
  }
  variant protected void create(Timestamp copy) {
    days = copy->nsecs / (24 * 3600 * NANOSECONDS) - (copy->nsecs < 0);
  }
  variant protected void create(mapping(string:int) tm) {
    create(mktime(tm + (["timezone":0])));
  }
  variant protected void create(int unix_time) {
    days = unix_time / (24 * 3600);
  }
  variant protected void create(float unix_time) {
    create((int)unix_time);
  }
  variant protected void create() {
  }

  protected mixed `+(mixed that) {
    this_program n = this_program(this);
    if (objectp(that)) {
      if (([object]that)->nsecs % (24 * 3600 * NANOSECONDS)
       || ([object]that)->months)
        error("Adding anything other than days not supported\n");
      n->days += ([object]that)->days
       + [int]([object]that)->nsecs / (24 * 3600 * NANOSECONDS);
    } else if (intp(that))
      n->days += that;
    else
      error("Cannot add %O\n", that);
    return n;
  }

  protected mixed `-(mixed that) {
    if (objectp(that)) {
      if (([object]that)->is_date) {
        Interval n = Interval();
        n->days = days - [int]([object]that)->days;
        return n;
      } else if (([object]that)->is_interval) {
        if (([object]that)->nsecs % (24 * 3600 * NANOSECONDS)
         || ([object]that)->months)
          error("Adding anything other than days not supported\n");
        this_program n = this_program(this);
        n->days -= ([object]that)->days
         + [int]([object]that)->nsecs / (24 * 3600 * NANOSECONDS);
        return n;
      }
    } else if (intp(that))
      return this + -that;
    error("Cannot substract %O\n", that);
  }

  inline protected int(0..1) `<(mixed that) {
    return
       intp(that) ? (int)this < that
     : floatp(that) ? (float)this < that
     : objectp(that)
      && (([object]that)->is_date ? days < ([object]that)->days
        : ([object]that)->is_timestamp
          && days * 24 * 3600 * NANOSECONDS < ([object]that)->nsecs);
  }

  inline protected int(0..1) `>(mixed that) {
    return
       intp(that) ? (int)this > that
     : floatp(that) ? (float)this > that
     : objectp(that)
      && (([object]that)->is_date ? days > ([object]that)->days
        : ([object]that)->is_timestamp
          && days * 24 * 3600 * NANOSECONDS > ([object]that)->nsecs);
  }

  protected int(0..1) `==(mixed that) {
    return
        objectp(that)
         && (days == [int]([object]that)->days
             && !zero_type(([object]that)->days)
          || !zero_type(([object]that)->nsecs)
           && days * 24 * 3600 * NANOSECONDS == [int]([object]that)->nsecs)
     || intp(that) && (int)this == [int]that
     || floatp(that) && (float)this == [float]that;
  }

  public mapping(string:int) tm() {
    return gmtime((int)this);
  }

  protected mixed cast(string to) {
    switch (to) {
      case "string": {
        mapping(string:int) t = tm();
        return sprintf("%04d/%02d/%02d", t->year + 1900, t->mon+1, t->mday);
      }
      case "float":
        return (float)(int)this;
      case "int":
        return days * 24 * 3600;
      default:
        return UNDEFINED;
    }
  }

  protected string _sprintf(int fmt, mapping(string:mixed) params) {
    switch (fmt) {
      case 'O': return sprintf("Date(%s)", (string)this);
      case 's': return (string)this;
    }
    return sprintf(sprintf("%%*%c", fmt), params, 0);
  }
}
