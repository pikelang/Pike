/*
 * $Id: mysql.pike,v 1.23 2006/08/10 14:24:04 grubba Exp $
 *
 * Glue for the Mysql-module
 */

//! Implements the glue needed to access the Mysql-module from the generic
//! SQL module.

#pike __REAL_VERSION__

#if constant(Mysql.mysql)

inherit Mysql.mysql;

#define UTF8_DECODE_QUERY	1
#define UTF8_ENCODE_QUERY	2

//! Set to the above if the connection is in utf8-mode.
static int utf8_mode;

//! Enter unicode encode/decode mode.
//!
//! After this has been enabled, query-strings may be provided
//! as wide (Unicode) strings, and any non-binary data will be
//! decoded automatically according to UTF8.
//!
//! @returns
//!   Returns @expr{1@} on success.
//!
//! @note
//!   All strings not prefixed by the keyword @tt{BINARY@}
//!   will be encoded according to UTF8.
//!
//! @seealso
//!   @[enter_unicode_decode_mode()]
int(0..1) enter_unicode_mode()
{
  if (!utf8_mode) {
    if (catch {
	big_query("SET NAMES 'utf8'");
      }) {
      return 0;
    }
  }
  utf8_mode = UTF8_DECODE_QUERY|UTF8_ENCODE_QUERY;
  return 1;
}

//! Enter unicode decode mode.
//!
//! After this has been enabled, non-binary data from the database
//! will be decoded according to UTF8.
//!
//! @returns
//!   Returns @expr{1@} on success.
//!
//! @note
//!   Any query encoding will need to be done by hand.
//!
//! @seealso
//!   @[enter_unicode_mode()]
int(0..1) enter_unicode_decode_mode()
{
  if (!utf8_mode) {
    if (catch {
	big_query("SET NAMES 'utf8'");
      }) {
      return 0;
    }
  }
  utf8_mode = UTF8_DECODE_QUERY;
  return 1;
}

// FIXME: Add a latin1 mode?

#if constant( Mysql.mysql.MYSQL_NO_ADD_DROP_DB )
// Documented in the C-file.
void create_db( string db )
{
  ::big_query( "CREATE DATABASE "+db );
}

void drop_db( string db )
{
  ::big_query( "DROP DATABASE "+db );
}
#endif

//! Quote a string so that it can safely be put in a query.
//!
//! @param s
//!   String to quote.
string quote(string s)
{
  return replace(s,
		 ({ "\\", "\"", "\0", "\'", "\n", "\r" }),
		 ({ "\\\\", "\\\"", "\\0", "\\\'", "\\n", "\\r" }));
}

//! Encode the apropriate sections of the query according to UTF8.
//! ie Those sections that are not strings prefixed by BINARY.
string utf8_encode_query(string q)
{
  string uq = upper_case(q);
  if (!has_value(uq, "BINARY")) return string_to_utf8(q);
  if ((q & ("\x7f" * sizeof(q))) == q) return q;

  // We need to find the segments that shouldn't be encoded.
  string e = "";
  while(has_value(uq, "BINARY")) {
    string prefix = "";
    string suffix;
    sscanf(q, "%[^\'\"]%s", prefix, suffix);
    e += string_to_utf8(prefix);
    if (!suffix || !sizeof(suffix)) {
      q = uq = "";
      break;
    }

    string quote = suffix[..0];
    int start = 1;
    int end;
    while ((end = search(suffix, quote, start)) >= 0) {
      if (suffix[end-1] == '\\') {
	// Count the number of preceeding back-slashes.
	// if odd, continue searching after the quote.
	int i;
	for (i = 2; i < end; i++) {
	  if (suffix[end - i] != '\\') break;
	}
	if (!(i & 1)) {
	  start = end+1;
	  continue;
	}
      }
      if (sizeof(suffix) == end+1) break;
      if (suffix[end+1] == quote[0]) {
	// Quote quoted by doubling.
	start = end+2;
	continue;
      }
      break;
    }

    string uprefix = uq[..sizeof(prefix)-1];
    int is_binary;
    // Common cases.
    if (has_suffix(uprefix, "BINARY") || has_suffix(uprefix, "BINARY ")) {
      // Binary string.
      is_binary = 1;
    } else {
      // Find the white-space suffix of the prefix.
      int i = sizeof(uprefix);
      while (i--) {
	if (!(< ' ', '\n', '\r', '\t' >)[uprefix[i]]) break;
      }
      is_binary = has_suffix(uprefix, "BINARY" + uprefix[i+1..]);
    }
    if (is_binary) {
      e += suffix[..end];
    } else {
      e += string_to_utf8(suffix[..end]);
    }
    q = suffix[end+1..];
    uq = uq[sizeof(uq)-sizeof(q)..];
  }
  // Encode the trailer.
  e += string_to_utf8(q);
  return e;
}

// The following time conversion functions assumes the SQL server
// handles time in this local timezone. They map the special zero
// time/date spec to 0.

private constant timezone = localtime (0)->timezone;

//! Converts a system time value to an appropriately formatted time
//! spec for the database.
//!
//! @param time
//!   Time to encode.
//!
//! @param date
//!   If nonzero then time is taken as a "full" unix time spec
//!   (where the date part is ignored), otherwise it's converted as a
//!   seconds-since-midnight value.
string encode_time (int time, void|int date)
{
  if (date) {
    if (!time) return "000000";
    mapping(string:int) ct = localtime (time);
    return sprintf ("%02d%02d%02d", ct->hour, ct->min, ct->sec);
  }
  else return sprintf ("%02d%02d%02d", time / 3600 % 24, time / 60 % 60, time % 60);
}

//! Converts a system time value to an appropriately formatted
//! date-only spec for the database.
//!
//! @param time
//!   Time to encode.
string encode_date (int time)
{
  if (!time) return "00000000";
  mapping(string:int) ct = localtime (time);
  return sprintf ("%04d%02d%02d", ct->year + 1900, ct->mon + 1, ct->mday);
}

//! Converts a system time value to an appropriately formatted
//! date and time spec for the database.
//!
//! @param time
//!   Time to encode.
string encode_datetime (int time)
{
  if (!time) return "00000000000000";
  mapping(string:int) ct = localtime (time);
  return sprintf ("%04d%02d%02d%02d%02d%02d",
		  ct->year + 1900, ct->mon + 1, ct->mday,
		  ct->hour, ct->min, ct->sec);
}

//! Converts a database time spec to a system time value.
//!
//! @param timestr
//!   Time spec to decode.
//!
//! @param date
//!   Take the date part from this system time value. If zero, a
//!   seconds-since-midnight value is returned.
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

//! Converts a database date-only spec to a system time value.
//! Assumes 4-digit years.
//!
//! @param datestr
//!   Date spec to decode.
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

//! Converts a database date and time spec to a system time value.
//! Can decode strings missing the time part.
//!
//! @param datestr
//!   Date and time spec to decode.
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

//!
int|object big_query(string q, mapping(string|int:mixed)|void bindings)
{
  if (bindings)
    q = .sql_util.emulate_bindings(q,bindings,this);
  if (utf8_mode & UTF8_ENCODE_QUERY) {
    // Mysql's line protocol is stupid; we need to detect
    // the binary strings in the query.
    q = utf8_encode_query(q);
  }

  int|object res = ::big_query(q);
  if (!objectp(res)) return res;

  if (utf8_mode & UTF8_DECODE_QUERY) {
    return .sql_util.UnicodeWrapper(res);
  }
  return ::big_query(q);
}


int(0..1) is_keyword( string name )
//! Return 1 if the argument @[name] is a mysql keyword.
{
  return (<
      "action", "add", "aggregate", "all", "alter", "after", "and", "as",
      "asc", "avg", "avg_row_length", "auto_increment", "between", "bigint",
      "bit", "binary", "blob", "bool", "both", "by", "cascade", "case",
      "char", "character", "change", "check", "checksum", "column",
      "columns", "comment", "constraint", "create", "cross", "current_date",
      "current_time", "current_timestamp", "data", "database", "databases",
      "date", "datetime", "day", "day_hour", "day_minute", "day_second",
      "dayofmonth", "dayofweek", "dayofyear", "dec", "decimal", "default",
      "delayed", "delay_key_write", "delete", "desc", "describe", "distinct",
      "distinctrow", "double", "drop", "end", "else", "escape", "escaped",
      "enclosed", "enum", "explain", "exists", "fields", "file", "first",
      "float", "float4", "float8", "flush", "foreign", "from", "for", "full",
      "function", "global", "grant", "grants", "group", "having", "heap",
      "high_priority", "hour", "hour_minute", "hour_second", "hosts",
      "identified", "ignore", "in", "index", "infile", "inner", "insert",
      "insert_id", "int", "integer", "interval", "int1", "int2", "int3",
      "int4", "int8", "into", "if", "is", "isam", "join", "key", "keys",
      "kill", "last_insert_id", "leading", "left", "length", "like",
      "lines", "limit", "load", "local", "lock", "logs", "long", "longblob",
      "longtext", "low_priority", "max", "max_rows", "match", "mediumblob",
      "mediumtext", "mediumint", "middleint", "min_rows", "minute",
      "minute_second", "modify", "month", "monthname", "myisam", "natural",
      "numeric", "no", "not", "null", "on", "optimize", "option",
      "optionally", "or", "order", "outer", "outfile", "pack_keys",
      "partial", "password", "precision", "primary", "procedure", "process",
      "processlist", "privileges", "read", "real", "references", "reload",
      "regexp", "rename", "replace", "restrict", "returns", "revoke",
      "rlike", "row", "rows", "second", "select", "set", "show", "shutdown",
      "smallint", "soname", "sql_big_tables", "sql_big_selects",
      "sql_low_priority_updates", "sql_log_off", "sql_log_update",
      "sql_select_limit", "sql_small_result", "sql_big_result",
      "sql_warnings", "straight_join", "starting", "status", "string",
      "table", "tables", "temporary", "terminated", "text", "then", "time",
      "timestamp", "tinyblob", "tinytext", "tinyint", "trailing", "to",
      "type", "use", "using", "unique", "unlock", "unsigned", "update",
      "usage", "values", "varchar", "variables", "varying", "varbinary",
      "with", "write", "when", "where", "year", "year_month", "zerofill",      
  >)[ lower_case(name) ];
}

static void create(string|void host, string|void database,
		   string|void user, string|void password,
		   mapping(string:string|int)|void options)
{
  if (options) {
    ::create(host||"", database||"", user||"", password||"", options);
  } else {
    ::create(host||"", database||"", user||"", password||"");
  }
  enter_unicode_mode();
}

#else
constant this_program_does_not_exist=1;
#endif /* constant(Mysql.mysql) */
