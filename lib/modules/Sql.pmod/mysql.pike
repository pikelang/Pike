/*
 * $Id: mysql.pike,v 1.8 2000/04/29 00:10:21 kinkie Exp $
 *
 * Glue for the Mysql-module
 */

//.
//. File:	mysql.pike
//. RCSID:	$Id: mysql.pike,v 1.8 2000/04/29 00:10:21 kinkie Exp $
//. Author:	Henrik Grubbström (grubba@idonex.se)
//.
//. Synopsis:	Implements the glue to the Mysql-module.
//.
//. +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//.
//. Implements the glue needed to access the Mysql-module from the generic
//. SQL module.
//.

#if constant(Mysql.mysql)

inherit Mysql.mysql;

//. - quote
//.   Quote a string so that it can safely be put in a query.
//. > s - String to quote.
string quote(string s)
{
  return(replace(s,
		 ({ "\\", "\"", "\0", "\'", "\n", "\r" }),
		 ({ "\\\\", "\\\"", "\\0", "\\\'", "\\n", "\\r" })));
}

// The following time conversion functions assumes the SQL server
// handles time in this local timezone. They map the special zero
// time/date spec to 0.

private constant timezone = localtime (0)->timezone;

//. - encode_time
//.   Converts a system time value to an appropriately formatted time
//.   spec for the database.
//. > time - Time to encode.
//. > date - If nonzero then time is taken as a "full" unix time spec
//.   (where the date part is ignored), otherwise it's converted as a
//.   seconds-since-midnight value.
string encode_time (int time, void|int date)
{
  if (date) {
    if (!time) return "000000";
    mapping(string:int) ct = localtime (time);
    return sprintf ("%02d%02d%02d", ct->hour, ct->min, ct->sec);
  }
  else return sprintf ("%02d%02d%02d", time / 3600 % 24, time / 60 % 60, time % 60);
}

//. - encode_date
//.   Converts a system time value to an appropriately formatted
//.   date-only spec for the database.
//. > time - Time to encode.
string encode_date (int time)
{
  if (!time) return "00000000";
  mapping(string:int) ct = localtime (time);
  return sprintf ("%04d%02d%02d", ct->year + 1900, ct->mon + 1, ct->mday);
}

//. - encode_datetime
//.   Converts a system time value to an appropriately formatted
//.   date and time spec for the database.
//. > time - Time to encode.
string encode_datetime (int time)
{
  if (!time) return "00000000000000";
  mapping(string:int) ct = localtime (time);
  return sprintf ("%04d%02d%02d%02d%02d%02d",
		  ct->year + 1900, ct->mon + 1, ct->mday,
		  ct->hour, ct->min, ct->sec);
}

//. - decode_time
//.   Converts a database time spec to a system time value.
//. > timestr - Time spec to decode.
//. > date - Take the date part from this system time value. If zero, a
//.   seconds-since-midnight value is returned.
int decode_time (string timestr, void|int date)
{
  int hour = 0, min = 0, sec = 0;
  if (sscanf (timestr, "%d:%d:%d", hour, min, sec) <= 1)
    sscanf (timestr, "%2d%2d%2d", hour, min, sec);
  if (date && (hour || min || sec)) {
    mapping(string:int) ct = localtime (date);
    return mktime (sec, min, hour, ct->mday, ct->mon, ct->year, ct->isdst, ct->timezone);
  }
  else return (hour * 60 + min) * 60 + sec;
}

//. - decode_date
//.   Converts a database date-only spec to a system time value.
//.   Assumes 4-digit years.
//. > datestr - Date spec to decode.
int decode_date (string datestr)
{
  int year = 0, mon = 0, mday = 0, n;
  n = sscanf (datestr, "%d-%d-%d", year, mon, mday);
  if (n <= 1) n = sscanf (datestr, "%4d%2d%2d", year, mon, mday);
  if (year || mon || mday)
    return mktime (0, 0, 0, n == 3 ? mday : 1, n >= 2 && mon - 1, year - 1900,
		   -1, timezone);
  else return 0;
}

//. - decode_datetime
//.   Converts a database date and time spec to a system time value.
//.   Can decode strings missing the time part.
//. > datestr - Date and time spec to decode.
int decode_datetime (string timestr)
{
  array(string) a = timestr / " ";
  if (sizeof (a) == 2)
    return decode_date (a[0]) + decode_time (a[1]);
  else {
    int n = sizeof (timestr);
    if (n >= 12)
      return decode_date (timestr[..n-7]) + decode_time (timestr[n-6..n-1]);
    else
      return decode_date (timestr);
  }
}

int|object big_query(string q, mapping(string|int:mixed)|void bindings)
{
  if (!bindings)
    return ::big_query(q);
  return ::big_query(.sql_util.emulate_bindings(q,bindings));
}

#else /* !constant(Mysql.mysql) */
#error "Mysql support not available.\n"
#endif /* constant(Mysql.mysql) */
