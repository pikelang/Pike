#pike __REAL_VERSION__

//! Generic base classes for the Sql interfaces.

//! Wrapper to handle conversion of zero to NULL in
//! @[Connection()->handle_extraargs()].
//!
//! @seealso
//!   @[zero]
protected class ZeroWrapper
{
  //! @returns
  //!   Returns the following:
  //!   @string
  //!     @value "NULL"
  //!       If @[fmt] is @expr{'s'@}.
  //!     @value "ZeroWrapper()"
  //!       If @[fmt] is @expr{'O'@}.
  //!   @endstring
  //!   Otherwise it formats a @expr{0@} (zero).
  protected string _sprintf(int fmt, mapping(string:mixed) params)
  {
    if (fmt == 's') return "NULL";
    if (fmt == 'O') return "ZeroWrapper()";
    return sprintf(sprintf("%%*%c", fmt), params, 0);
  }
}

//! Instance of @[ZeroWrapper] used by @[Connection()->handle_extraargs()].
ZeroWrapper zero = ZeroWrapper();

protected class NullArg
{
  protected string _sprintf (int fmt)
    {return fmt == 'O' ? "NullArg()" : "NULL";}
}
NullArg null_arg = NullArg();

#define NANOSECONDS	1000000000
  // 2000/01/01 00:00:00 UTC
protected int year2000utc = mktime((["year":100, "mday":1, "timezone":0]));

private string isotimezone(int timezone) {
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

private string timetext(mapping(string:int) tm) {
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

//!
private class Timebase {

  //!
  int nsecs;	// Since 2000/01/01 00:00:00 UTC

  //!
  variant protected void create() {
  }
  variant protected void create(this_program copy) {
    nsecs = copy->nsecs;
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

  variant protected this_program `+(Interval what) {
    if (what->days || what->months)
      error("Adding days or months not supported yet\n");
    this_program n = this_program(this);
    n->nsecs += what->nsecs;
    return n;
  }
  variant protected this_program `+(int|float what) {
    this_program n = this_program();
    n->nsecs = (int)(nsecs + what * NANOSECONDS);
    return n;
  }

  variant protected this_program `-(Interval what) {
    if (what->days || what->months)
      error("Adding days or months not supported yet\n");
    this_program n = this_program(this);
    n->nsecs -= what->nsecs;
    return n;
  }
  variant protected this_program `-(int|float what) {
    this_program n = this_program();
    n->nsecs = (int)(nsecs - what * NANOSECONDS);
    return n;
  }

  variant protected int(0..1) `<(this_program what) {
    return nsecs < what->nsecs;
  }
  variant protected int(0..1) `<(int what) {
    return nsecs < what * NANOSECONDS;
  }
  variant protected int(0..1) `<(float what) {
    return nsecs < (int)(what * NANOSECONDS);
  }

  variant protected int(0..1) `>(int what) {
    return nsecs > what * NANOSECONDS;
  }
  variant protected int(0..1) `>(float what) {
    return nsecs > (int)(what * NANOSECONDS);
  }

  variant protected int(0..1) `==(this_program what) {
    return nsecs == what->nsecs;
  }
  variant protected int(0..1) `==(int what) {
    return nsecs == what * NANOSECONDS;
  }
  variant protected int(0..1) `==(float what) {
    return nsecs == (int)(what * NANOSECONDS);
  }

  //!
  public mapping(string:int) tm() {
    return (["nsec": nsecs % NANOSECONDS]);
  }

  //!
  protected mixed cast(string to) {
    switch (to) {
      case "string":
        return timetext(tm());
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

//! Object to be returned and passed for date values to and from the database
class Date {

  //!
  int days;	// Since 2000/01/01

  //!
  protected void create(int year, int month, int day) {
    create((["year":year - 1900, "mon":month - 1, "mday":day, "timezone":0]));
  }
  variant protected void create(this_program copy) {
    days = copy->days;
  }
  variant protected void create(mapping(string:int) tm) {
    create(mktime(tm + (["timezone":0])));
  }
  variant protected void create(int unix_time) {
    days = (unix_time - year2000utc) / (24 * 3600);
  }
  variant protected void create() {
  }

  variant protected this_program `+(Interval what) {
    if (what->nsecs % (24 * 3600 * NANOSECONDS) || what->months)
      error("Adding anything other than days not supported\n");
    this_program n = this_program(this);
    n->days += what->days + what->nsecs / (24 * 3600 * NANOSECONDS);
    return n;
  }
  variant protected this_program `+(int what) {
    this_program n = this_program(this);
    n->days += what;
    return n;
  }

  variant protected this_program `-(Interval what) {
    if (what->nsecs % (24 * 3600 * NANOSECONDS) || what->months)
      error("Adding anything other than days not supported\n");
    this_program n = this_program(this);
    n->days -= what->days + what->nsecs / (24 * 3600 * NANOSECONDS);
    return n;
  }
  variant protected this_program `-(int what) {
    return this + -what;
  }

  variant protected int(0..1) `<(this_program what) {
    return days < what->days;
  }

  variant protected int(0..1) `==(this_program what) {
    return days == what->days;
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

//! Object to be returned and passed for timestamp values to and from the database
class Timestamp {
  inherit Timebase;

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
          res += " " + timetext(t);
        return res + isotimezone(t->timezone);
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

//! Object to be returned and passed for time values to and from the database
class Time {
  inherit Timebase;

  //!
  variant
   protected void create(int hour, int min, void|int sec, void|int nsec) {
    nsecs = ((((hour * 60) + min) * 60 + sec) * NANOSECONDS) + nsec;
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

//! Object to be returned and passed for time values to and from the database
class TimeTZ {
  inherit Time;

  //!
  int timezone;

  variant protected void create(this_program copy) {
    ::create(copy);
    timezone = copy->timezone;
  }

  //!
  public mapping(string:int) tm() {
    return ::tm() + (["timezone":timezone]);
  }

  //!
  protected mixed cast(string to) {
    if (to == "string")
      return ::cast(to) + isotimezone(timezone);
    return ::cast(to);
  }

  protected string _sprintf(int fmt, mapping(string:mixed) params) {
    if (fmt == 'O')
      return sprintf("Sql.TimeTZ(%s)", (string)this);
    return ::_sprintf(fmt, params);
  }
}

//! Object to be returned and passed for interval values to and from the database
class Interval {
  inherit Time;

  //!
  int days;

  //!
  int months;

  variant protected void create(this_program copy) {
    ::create(copy);
    days = copy->days;
    months = copy->months;
  }

  variant protected this_program `*(int what) {
    this_program n = this_program(this);
    n->nsecs *= what;
    n->days *= what;
    n->months *= what;
    return n;
  }
  variant protected this_program `*(float what) {
    this_program n = this_program();
    n->nsecs = (int)(nsecs * what);
    n->days = (int)(days * what);
    n->months = (int)(months * what);
    if (days && n->days % days || months && n->months % months)
      error("Cannot create fractional days or months\n");
    return n;
  }

  variant protected this_program `/(int|float what) {
    this_program n = this_program(this);
    n->nsecs = (int)(nsecs / what);
    n->days = (int)(days / what);
    n->months = (int)(months / what);
    if (days && n->days % days || months && n->months % months)
      error("Cannot create fractional days or months\n");
    return n;
  }

  variant protected this_program `+(this_program what) {
    this_program n = this_program(this);
    n->nsecs += what->nsecs;
    n->days += what->days;
    n->months += what->months;
    return n;
  }

  variant protected this_program `-(this_program what) {
    this_program n = this_program(this);
    n->nsecs -= what->nsecs;
    n->days -= what->days;
    n->months -= what->months;
    return n;
  }

  variant protected int(0..1) `<(this_program what) {
    return
        months <= what->months && days <= what->days && nsecs  < what->nsecs
     || months <= what->months && days  < what->days && nsecs <= what->nsecs
     || months  < what->months && days <= what->days && nsecs <= what->nsecs;
  }

  variant protected int(0..1) `==(this_program what) {
    return months == what->months && days == what->days && nsecs == what->nsecs;
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

//!
class Inet {

  //!
  int address;

  //!
  int masklen;

  //!
  protected void create(string ip) {
    int ip0, ip1, ip2, ip3, m1, m2, m3;
    string mask;
    masklen = 32;
    switch (sscanf(ip, "%d.%d.%d.%d/%s", ip0, ip1, ip2, ip3, mask)) {
      default:
        masklen = 16*8;
        sscanf(ip, "%s/%d", ip, masklen);
        address = 0;
        foreach (Protocols.IPv6.parse_addr(ip); ; int val)
          address = address << 16 | val;
        break;
      case 5:
        switch (sscanf(mask, "%d.%d.%d.%d", masklen, m1, m2, m3)) {
          default:
            error("Unparseable IP %O\n", ip);
          case 4:
            m1 = ((masklen << 8 | m1) << 8 | m2) << 8 | m3;
            masklen = 32;
            while (!(m1 & 1) && --masklen)
              m1 >>= 1;
          case 1:
            break;
        }
      case 4:
        masklen += 12*8;
        address = ((ip0 << 8 | ip1) << 8 | ip2) << 8 | ip3;
    }
  }
  variant protected void create(int ip, void|int masklen) {
    address = ip;
    this::masklen = zero_type(masklen) ? 16*8 : 12*8 + masklen;
  }
  variant protected void create() {
  }

  variant protected this_program `+(int what) {
    this_program n = this_program();
    n->masklen = masklen;
    n->address = address + what;
    return n;
  }

  variant protected this_program `-(int what) {
    return this + -what;
  }

  variant protected this_program `~() {
    this_program n = this_program();
    n->masklen = masklen;
    n->address = ~address;
    return n;
  }

  variant protected this_program `|(this_program what) {
    this_program n = this_program();
    n->masklen = max(masklen, what->masklen);
    n->address = address | what->address;
    return n;
  }

  variant protected this_program `&(this_program what) {
    this_program n = this_program();
    n->masklen = min(masklen, what->masklen);
    n->address = address & what->address;
    return n;
  }

  variant protected int(0..1) `<(this_program what) {
    return address < what->address;
  }
  variant protected int(0..1) `<(int what) {
    return address < what;
  }

  variant protected int(0..1) `>(int what) {
    return address > what;
  }

  variant protected int(0..1) `==(this_program what) {
    return address == what->address && masklen == what->masklen;
  }
  variant protected int(0..1) `==(int what) {
    return address == what;
  }


  //!
  protected mixed cast(string to) {
    switch (to) {
      case "string": {
        array split = ({});
        int addr = address;
        do
          split += ({ addr & 0xffff });
        while ((addr >>= 16) && sizeof(split) < 16);
        if (sizeof(split) <= 2) {
          if (sizeof(split) == 1)
            split += ({0});
          return sprintf("%d.%d.%d.%d/%d", split[1] >> 8, split[1] & 255,
           split[0] >> 8, split[0] & 255, masklen - 12*8);
        } else {
          while (sizeof(split) < 8)
            split += ({0});
          split = reverse(split);
          return Protocols.IPv6.format_addr_short(split)
           + sprintf("/%d", masklen);
        }
      }
      case "int":
        return address;
      default:
        return UNDEFINED;
    }
  }

  protected string _sprintf(int fmt, mapping(string:mixed) params) {
    switch (fmt) {
      case 'O': return sprintf("Inet(%s)", (string)this);
        break;
      case 's': return (string)this;
        break;
      default: return sprintf(sprintf("%%*%c", fmt), params, 0);
    }
  }
}
