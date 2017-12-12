#pike __REAL_VERSION__
#pragma strict_types

inherit _static_modules.Builtin;

#define NANOSECONDS	1000000000
  // 2000/01/01 00:00:00 UTC
protected int year2000utc = mktime((["year":100, "mday":1, "timezone":0]));

//! @param timezone
//!  Timezone in seconds relative to UTC.
//! @returns
//!   ISO formatted timezone.
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

//! Base class for the time related types.
//! @seealso
//!  [Timestamp]
class Timebase {

  //!
  int nsecs;	// Since 2000/01/01 00:00:00 UTC

  //!
  variant protected void create() {
  }
  variant protected void create(object/*this_program*/ copy) {
    nsecs = [int]copy->nsecs;
  }
  variant protected void create(int|float sec, void|int nsec) {
    nsecs = (int)(sec * NANOSECONDS + nsec);
  }

  //!
  int|float `usecs() {
    return nsecs % 1000 ? nsecs / 1000.0 : nsecs / 1000;
  }

  //!
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

  //!
  public mapping(string:int) tm() {
    return (["nsec": nsecs % NANOSECONDS]);
  }

  //!
  protected mixed cast(string to) {
    switch (to) {
      case "string":
        return iso_time(tm());
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

//! Lightweight time type.
class Time {
  inherit Timebase;

  //!
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

  //!
  public mapping(string:int) tm() {
    int hourleft;
    int secleft = (hourleft = nsecs / NANOSECONDS) % 60;
    int minleft = (hourleft /= 60) % 60;
    hourleft /= 60;
    return ::tm() + (["hour":hourleft, "min":minleft, "sec":secleft]);
  }

  protected string _sprintf(int fmt, mapping(string:mixed) params) {
    if (fmt == 'O')
      return sprintf("Sql.Time(%s)", (string)this);
    return ::_sprintf(fmt, params);
  }
}

//! Lightweight time type with timezone.
class TimeTZ {
  inherit Time;

  //!
  int timezone;

  variant protected void create(object/*this_program*/ copy) {
    ::create(copy);
    timezone = [int]copy->timezone;
  }

  //!
  public mapping(string:int) tm() {
    return ::tm() + (["timezone":timezone]);
  }

  //!
  protected mixed cast(string to) {
    if (to == "string")
      return ::cast(to) + iso_timezone(timezone);
    return ::cast(to);
  }

  protected string _sprintf(int fmt, mapping(string:mixed) params) {
    if (fmt == 'O')
      return sprintf("Sql.TimeTZ(%s)", (string)this);
    return ::_sprintf(fmt, params);
  }
}

//! Lightweight time and date interval type.
class Interval {
  inherit Time;

  constant is_interval = 1;

  //!
  int days;

  //!
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

  //!
  public mapping(string:int) tm() {
    return ::tm() + (["mon":months, "mday":days]);
  }

  //!
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
      return sprintf("Sql.Interval(%s)", (string)this);
    return ::_sprintf(fmt, params);
  }
}

//! Lightweight time and date type.  The values are stored internally
//! relative to UTC.
class Timestamp {
  inherit Timebase;
  constant is_timestamp = 1;

  //! Init using local time
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
    ::create(unix_time - year2000utc, nsec);
  }

  inline protected int(0..1) `<(mixed that) {
    return intp(that) ? (int)this < that : ::`<(that);
  }

  //!
  public mapping(string:int) tm() {
    return localtime((int)this) + ::tm();
  }

  //!
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
      case "float":
      case "int":
        return ::cast(to) + year2000utc;
    }
    return ::cast(to);
  }

  protected string _sprintf(int fmt, mapping(string:mixed) params) {
    if (fmt == 'O')
      return sprintf("Sql.TimeStamp(%s)", (string)this);
    return ::_sprintf(fmt, params);
  }
}

//! Lightweight date type.
class Date {
  constant is_date = 1;

  //!
  int days;	// Since 2000/01/01

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
    days = (unix_time - year2000utc) / (24 * 3600);
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

  //!
  public mapping(string:int) tm() {
    return gmtime((int)this);
  }

  //!
  protected mixed cast(string to) {
    switch (to) {
      case "string": {
        mapping(string:int) t = tm();
        return sprintf("%04d/%02d/%02d", t->year + 1900, t->mon+1, t->mday);
      }
      case "float":
        return (float)(int)this;
      case "int":
        return days * (24 * 3600) + year2000utc;
      default:
        return UNDEFINED;
    }
  }

  protected string _sprintf(int fmt, mapping(string:mixed) params) {
    switch (fmt) {
      case 'O': return sprintf("Sql.Date(%s)", (string)this);
      case 's': return (string)this;
    }
    return sprintf(sprintf("%%*%c", fmt), params, 0);
  }
}
