/*
 * $Id: mysql.pike,v 1.39 2007/05/26 13:53:26 mast Exp $
 *
 * Glue for the Mysql-module
 */

//! Implements the glue needed to access the Mysql-module from the generic
//! SQL module.

#pike __REAL_VERSION__

#if constant(Mysql.mysql)

inherit Mysql.mysql;

#define UNICODE_DECODE_MODE	1 // Unicode decode mode
#define LATIN1_UNICODE_ENCODE_MODE 2 // Unicode encode mode with latin1 charset
#define UTF8_UNICODE_ENCODE_MODE 4 // Unicode encode mode with utf8 charset

#ifdef MYSQL_CHARSET_DEBUG
#define CH_DEBUG(X...)	werror("Sql.mysql: " + X)
#else
#define CH_DEBUG(X...)
#endif

#if !constant (Mysql.mysql.HAVE_MYSQL_FIELD_CHARSETNR)
// Recognition constant to tell that the unicode decode mode would use
// the buggy MySQLBrokenUnicodeWrapper if it would be enabled through
// any of the undocumented methods.
constant unicode_decode_mode_is_broken = 1;
#endif

// Set to the above if the connection is requested to be in one of the
// unicode modes. latin1 unicode encode mode is enabled by default; it
// should be compatible with earlier pike versions.
static int utf8_mode;

// The charset, either "latin1" or "utf8", currently assigned to
// character_set_client when unicode encode mode is enabled. Zero when
// the connection charset has been set to something else than "latin1"
// or "unicode".
static string send_charset;

static void update_unicode_encode_mode_from_charset (string charset)
{
  switch (charset) {		// Lowercase assumed.
    case "latin1":
      utf8_mode |= LATIN1_UNICODE_ENCODE_MODE;
      utf8_mode &= ~UTF8_UNICODE_ENCODE_MODE;
      send_charset = "latin1";
      CH_DEBUG ("Entering latin1 encode mode.\n");
      break;
    case "unicode":
      utf8_mode |= UTF8_UNICODE_ENCODE_MODE;
      utf8_mode &= ~LATIN1_UNICODE_ENCODE_MODE;
      send_charset = "utf8";
      CH_DEBUG ("Entering unicode encode mode.\n");
      break;
    default:
      // Wrong charset - the mode can't be used.
      utf8_mode |= LATIN1_UNICODE_ENCODE_MODE|UTF8_UNICODE_ENCODE_MODE;
      send_charset = 0;
      CH_DEBUG ("Not entering latin1/unicode encode mode "
		"due to incompatible charset %O.\n", charset);
      break;
  }
}

int(0..1) set_unicode_encode_mode (int enable)
//! Enables or disables unicode encode mode.
//!
//! In this mode, if the server supports UTF-8 and the connection
//! charset is @expr{latin1@} (the default) or @expr{unicode@} then
//! @[big_query] handles wide unicode queries. Enabled by default.
//!
//! Unicode encode mode works as follows: Eight bit strings are sent
//! as @expr{latin1@} and wide strings are sent using @expr{utf8@}.
//! @[big_query] sends @expr{SET character_set_client@} statements as
//! necessary to update the charset on the server side. If the server
//! doesn't support that then it fails, but the wide string query
//! would fail anyway.
//!
//! To make this transparent, string literals with introducers (e.g.
//! @expr{_binary 'foo'@}) are excluded from the UTF-8 encoding. This
//! means that @[big_query] needs to do some superficial parsing of
//! the query when it is a wide string.
//!
//! @returns
//!   @int
//!     @value 1
//!       Unicode encode mode is enabled.
//!     @value 0
//!       Unicode encode mode couldn't be enabled because an
//!       incompatible connection charset is set. You need to do
//!       @expr{@[set_charset]("latin1")@} or
//!       @expr{@[set_charset]("unicode")@} to enable it.
//!   @endint
//!
//! @note
//!   Note that this mode doesn't affect the MySQL system variable
//!   @expr{character_set_connection@}, i.e. it will still be set to
//!   @expr{latin1@} by default which means server functions like
//!   @expr{UPPER()@} won't handle non-@expr{latin1@} characters
//!   correctly in all cases.
//!
//!   To fix that, do @expr{@[set_charset]("unicode")@}. That will
//!   allow unicode encode mode to work while @expr{utf8@} is fully
//!   enabled at the server side.
//!
//!   Tip: If you enable @expr{utf8@} on the server side, you need to
//!   send raw binary strings as @expr{_binary'...'@}. Otherwise they
//!   will get UTF-8 encoded by the server.
//!
//! @note
//!   When unicode encode mode is enabled and the connection charset
//!   is @expr{latin1@}, the charset accepted by @[big_query] is not
//!   quite Unicode since @expr{latin1@} is based on @expr{cp1252@}.
//!   The differences are in the range @expr{0x80..0x9f@} where
//!   Unicode has control chars.
//!
//!   This small discrepancy is not present when the connection
//!   charset is @expr{unicode@}.
//!
//! @seealso
//!   @[set_unicode_decode_mode], @[set_charset]
{
  if (enable)
    update_unicode_encode_mode_from_charset (lower_case (get_charset()));
  else {
    utf8_mode &= ~(LATIN1_UNICODE_ENCODE_MODE|UTF8_UNICODE_ENCODE_MODE);
    send_charset = 0;
    CH_DEBUG("Disabling unicode encode mode.\n");
  }
  return !!send_charset;
}

int get_unicode_encode_mode()
//! Returns nonzero if unicode encode mode is enabled, zero otherwise.
//!
//! @seealso
//!   @[set_unicode_encode_mode]
{
  return !!send_charset;
}

void set_unicode_decode_mode (int enable)
//! Enable or disable unicode decode mode.
//!
//! In this mode, if the server supports UTF-8 then non-binary text
//! strings in results are automatically decoded to (possibly wide)
//! unicode strings. Not enabled by default.
//!
//! The statement "@expr{SET character_set_results = utf8@}" is sent
//! to the server to enable the mode. When the mode is disabled,
//! "@expr{SET character_set_results = xxx@}" is sent, where
//! @expr{xxx@} is the connection charset that @[get_charset] returns.
//!
//! @param enable
//!   Nonzero enables this feature, zero disables it.
//!
//! @throws
//!   Throws an exception if the server doesn't support this, i.e. if
//!   the statement above fails. The MySQL system variable
//!   @expr{character_set_results@} was added in MySQL 4.1.1.
//!
//!   An error is also thrown if Pike has been compiled with a MySQL
//!   client library older than 4.1.0, which lack the necessary
//!   support for this.
//!
//! @seealso
//!   @[set_unicode_encode_mode]
{
#if !constant (Mysql.mysql.HAVE_MYSQL_FIELD_CHARSETNR)
  // Undocumented feature for old mysql libs. See
  // MySQLBrokenUnicodeWrapper for details.
  if (!(<0, -1>)[enable] && !getenv("PIKE_BROKEN_MYSQL_UNICODE_MODE")) {
    predef::error ("Unicode decode mode not supported - "
		   "compiled with MySQL client library < 4.1.0.\n");
  }
#endif

  if (enable) {
    CH_DEBUG("Enabling unicode decode mode.\n");
    ::big_query ("SET character_set_results = utf8");
    utf8_mode |= UNICODE_DECODE_MODE;
  }
  else {
    CH_DEBUG("Disabling unicode decode mode.\n");
    ::big_query ("SET character_set_results = " + get_charset());
    utf8_mode &= ~UNICODE_DECODE_MODE;
  }
}


int get_unicode_decode_mode()
//! Returns nonzero if unicode decode mode is enabled, zero otherwise.
//!
//! @seealso
//!   @[set_unicode_decode_mode]
{
  return utf8_mode & UNICODE_DECODE_MODE;
}

void set_charset (string charset)
//! Changes the connection charset. Works similar to sending the query
//! @expr{SET NAMES @[charset]@} but also records the charset on the
//! client side so that various client functions work correctly.
//!
//! @[charset] is a MySQL charset name or the special value
//! @expr{"unicode"@} (see below). You can use @expr{SHOW CHARACTER
//! SET@} to get a list of valid charsets.
//!
//! Specifying @expr{"unicode"@} as charset is the same as
//! @expr{"utf8"@} except that unicode encode and decode modes are
//! enabled too. Briefly, this means that you can send queries as
//! unencoded unicode strings and will get back non-binary text
//! results as unencoded unicode strings. See
//! @[set_unicode_encode_mode] and @[set_unicode_decode_mode] for
//! further details.
//!
//! @throws
//!   Throws an exception if the server doesn't support this, i.e. if
//!   the statement @expr{SET NAMES@} fails. Support for it was added
//!   in MySQL 4.1.0.
//!
//! @note
//!   If @[charset] is @expr{"latin1"@} and unicode encode mode is
//!   enabled (the default) then @[big_query] can send wide unicode
//!   queries transparently if the server supports UTF-8. See
//!   @[set_unicode_encode_mode].
//!
//! @note
//!   If unicode decode mode is already enabled (see
//!   @[set_unicode_decode_mode]) then this function won't affect the
//!   result charset (i.e. the MySQL system variable
//!   @expr{character_set_results@}).
//!
//!   Actually, a query @expr{SET character_set_results = utf8@} will
//!   be sent immediately after setting the charset as above if
//!   unicode decode mode is enabled and @[charset] isn't
//!   @expr{"utf8"@}.
//!
//! @note
//!   You should always use either this function or the
//!   @expr{"mysql_charset_name"@} option to @[create] to set the
//!   connection charset, or more specifically the charset that the
//!   server expects queries to have (i.e. the MySQL system variable
//!   @expr{character_set_client@}). Otherwise @[big_query] might not
//!   work correctly.
//!
//!   Afterwards you may change the system variable
//!   @expr{character_set_connection@}, and also
//!   @expr{character_set_results@} if unicode decode mode isn't
//!   enabled.
//!
//! @note
//!   The MySQL @expr{latin1@} charset is close to Windows
//!   @expr{cp1252@}. The difference from ISO-8859-1 is a bunch of
//!   printable chars in the range @expr{0x80..0x9f@} (which contains
//!   control chars in ISO-8859-1). For instance, the euro currency
//!   sign is @expr{0x80@}.
//!
//!   You can use the @expr{mysql-latin1@} encoding in the
//!   @[Locale.Charset] module to do conversions, or just use the
//!   special @expr{"unicode"@} charset instead.
//!
//! @seealso
//!   @[get_charset], @[set_unicode_encode_mode], @[set_unicode_decode_mode]
{
  charset = lower_case (charset);

  CH_DEBUG("Setting charset to %O.\n", charset);

  int broken_unicode = charset == "broken-unicode";
  if (broken_unicode) charset = "unicode";

  ::set_charset (charset == "unicode" ? "utf8" : charset);

  if (charset == "unicode" ||
      utf8_mode & (LATIN1_UNICODE_ENCODE_MODE|UTF8_UNICODE_ENCODE_MODE))
    update_unicode_encode_mode_from_charset (charset);

  if (charset == "unicode") {
#if constant (Mysql.mysql.HAVE_MYSQL_FIELD_CHARSETNR)
    utf8_mode |= UNICODE_DECODE_MODE;
#else
    if (broken_unicode || getenv ("PIKE_BROKEN_MYSQL_UNICODE_MODE"))
      // Undocumented feature for old mysql libs. See
      // MySQLBrokenUnicodeWrapper for details.
      utf8_mode |= UNICODE_DECODE_MODE;
    else
      predef::error ("Unicode decode mode not supported - "
		     "compiled with MySQL client library < 4.1.0.\n");
#endif
  }
  else if (utf8_mode & UNICODE_DECODE_MODE && charset != "utf8")
    // This setting has been overridden by ::set_charset, so we need
    // to reinstate it.
    ::big_query ("SET character_set_results = utf8");
}

string get_charset()
//! Returns the MySQL name for the current connection charset.
//!
//! Returns @expr{"unicode"@} if unicode encode mode is enabled and
//! UTF-8 is used on the server side (i.e. in
//! @expr{character_set_connection@}).
//!
//! @note
//!   In servers with full charset support (i.e. MySQL 4.1.0 or
//!   later), this corresponds to the MySQL system variable
//!   @expr{character_set_client@} (with one exception - see next
//!   note) and thus controls the charset in which queries are sent.
//!   The charset used for text strings in results might be something
//!   else (and typically is if unicode decode mode is enabled; see
//!   @[set_unicode_decode_mode]).
//!
//! @note
//!   If the returned charset is @expr{latin1@} or @expr{unicode@} and
//!   unicode encode mode is enabled (the default) then
//!   @expr{character_set_client@} in the server might be either
//!   @expr{latin1@} or @expr{utf8@}, depending on the last sent
//!   query. See @[set_unicode_encode_mode] for more info.
//!
//! @seealso
//!   @[set_charset]
{
  if (utf8_mode & UTF8_UNICODE_ENCODE_MODE && send_charset)
    // We don't try to be symmetric with set_charset when the
    // broken-unicode kludge is in use. That since this reflects the
    // setting on the encode side only.
    return "unicode";
  return ::get_charset();
}

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

string latin1_to_utf8 (string s)
//! Converts a string in MySQL @expr{latin1@} format to UTF-8.
{
  return string_to_utf8 (replace (s, ([
    "\x80": "\u20AC", /*"\x81": "\u0081",*/ "\x82": "\u201A", "\x83": "\u0192",
    "\x84": "\u201E", "\x85": "\u2026", "\x86": "\u2020", "\x87": "\u2021",
    "\x88": "\u02C6", "\x89": "\u2030", "\x8a": "\u0160", "\x8b": "\u2039",
    "\x8c": "\u0152", /*"\x8d": "\u008D",*/ "\x8e": "\u017D", /*"\x8f": "\u008F",*/
    /*"\x90": "\u0090",*/ "\x91": "\u2018", "\x92": "\u2019", "\x93": "\u201C",
    "\x94": "\u201D", "\x95": "\u2022", "\x96": "\u2013", "\x97": "\u2014",
    "\x98": "\u02DC", "\x99": "\u2122", "\x9a": "\u0161", "\x9b": "\u203A",
    "\x9c": "\u0153", /*"\x9d": "\u009D",*/ "\x9e": "\u017E", "\x9f": "\u0178",
  ])));
}

string utf8_encode_query (string q, function(string:string) encode_fn)
//! Encodes the appropriate sections of the query with @[encode_fn].
//! Everything except strings prefixed by an introducer (i.e.
//! @expr{_something@} or @expr{N@}) is encoded.
{
  // We need to find the segments that shouldn't be encoded.
  string e = "";
  while (1) {
    sscanf(q, "%[^\'\"]%s", string prefix, string suffix);
    e += encode_fn (prefix);

    if (suffix == "") break;

    string quote = suffix[..0];
    int start = 1;
    int end;
    while ((end = search(suffix, quote, start)) >= 0) {
      if (suffix[end-1] == '\\') {
	// Count the number of preceding back-slashes.
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

#define IS_IDENTIFIER_CHAR(chr) (Unicode.is_wordchar (chr) ||		\
				 (<'_', '$'>)[chr])

    int intpos = -1;

    // Optimize the use of _binary.
    if (has_suffix (prefix, "_binary"))
      intpos = sizeof (prefix) - sizeof ("_binary");
    else if (has_suffix (prefix, "_binary "))
      intpos = sizeof (prefix) - sizeof ("_binary ");

    else {
      // Find the white-space suffix of the prefix.
      int i = sizeof(prefix);
      while (i--) {
	if (!(< ' ', '\n', '\r', '\t' >)[prefix[i]]) break;
      }

      if (i >= 0) {
	if ((<'n', 'N'>)[prefix[i]])
	  // Probably got a national charset string.
	  intpos = i;
	else {
	  // The following assumes all possible charset names contain
	  // only [a-zA-Z0-9_$] and are max 32 chars (from
	  // MY_CS_NAME_SIZE in m_ctype.h).
	  sscanf (reverse (prefix[i - 33..i]), "%[a-zA-Z0-9_$]%s",
		  string rev_intro, string rest);
	  if (sizeof (rev_intro) && rev_intro[-1] == '_' && sizeof (rest))
	    intpos = i - sizeof (rev_intro) + 1;
	}
      }
    }

    int got_introducer;
    if (intpos == 0)
      // The prefix begins with the introducer.
      got_introducer = 1;
    else if (intpos > 0) {
      // Check that the introducer sequence we found isn't a suffix of
      // some longer keyword or identifier.
      int prechar = prefix[intpos - 1];
      if (!IS_IDENTIFIER_CHAR (prechar))
	got_introducer = 1;
    }

    if (got_introducer) {
      string s = suffix[..end];
      if (String.width (s) > 8) {
	string encoding = prefix[intpos..];
	if (has_prefix (encoding, "_"))
	  sscanf (encoding[1..], "%[a-zA-Z0-9]", encoding);
	else
	  encoding = "utf8";	// Gotta be "N".
	s = s[1..<1];
	if (sizeof (s) > 40) s = sprintf ("%O...", s[..37]);
	else s = sprintf ("%O", s);
	predef::error ("A string in the query should be %s encoded "
		       "but it is wide: %s\n", encoding, s);
      }
      e += s;
    } else {
      e += encode_fn (suffix[..end]);
    }

    q = suffix[end+1..];
  }
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

#if constant (Mysql.mysql.HAVE_MYSQL_FIELD_CHARSETNR)
#define HAVE_MYSQL_FIELD_CHARSETNR_IFELSE(TRUE, FALSE) TRUE
#else
#define HAVE_MYSQL_FIELD_CHARSETNR_IFELSE(TRUE, FALSE) FALSE
#endif

#define QUERY_BODY(do_query)						\
  if (bindings)								\
    query = .sql_util.emulate_bindings(query,bindings,this);		\
									\
  string restore_charset;						\
  if (charset) {							\
    restore_charset = send_charset || get_charset();			\
    if (charset != restore_charset) {					\
      CH_DEBUG ("Switching charset from %O to %O (due to charset arg).\n", \
		restore_charset, charset);				\
      ::big_query ("SET character_set_client=" + charset);		\
      /* Can't be changed automatically - has side effects. /mast */	\
      /* ::big_query("SET character_set_connection=" + charset); */	\
    } else								\
      restore_charset = 0;						\
  }									\
									\
  else if (send_charset) {						\
    string new_send_charset;						\
									\
    if (utf8_mode & LATIN1_UNICODE_ENCODE_MODE) {			\
      if (String.width (query) == 8)					\
	new_send_charset = "latin1";					\
      else {								\
	CH_DEBUG ("Converting (mysql-)latin1 query to utf8.\n");	\
	query = utf8_encode_query (query, latin1_to_utf8);		\
	new_send_charset = "utf8";					\
      }									\
    }									\
									\
    else {  /* utf8_mode & UTF8_UNICODE_ENCODE_MODE */			\
      if (_can_send_as_latin1 (query))					\
	new_send_charset = "latin1";					\
      else {								\
	CH_DEBUG ("Converting query to utf8.\n");			\
	query = utf8_encode_query (query, string_to_utf8);		\
	new_send_charset = "utf8";					\
      }									\
    }									\
									\
    if (new_send_charset != send_charset) {				\
      CH_DEBUG ("Switching charset from %O to %O.\n",			\
		send_charset, new_send_charset);			\
      if (mixed err = catch {						\
	  ::big_query ("SET character_set_client=" + new_send_charset);	\
	  /* Can't be changed automatically - has side effects. /mast */ \
	  /* ::big_query("SET character_set_connection=" +		\
	     new_send_charset); */					\
	  }) {								\
	if (new_send_charset == "utf8")					\
	  predef::error ("The query is a wide string "			\
			 "and the MySQL server doesn't support UTF-8: %s\n", \
			 describe_error (err));				\
	else								\
	  throw (err);							\
      }									\
      send_charset = new_send_charset;					\
    }									\
  }									\
									\
  CH_DEBUG ("Sending query with charset %O: %O.\n",			\
	    charset || send_charset, query);				\
									\
  int|object res = ::do_query(query);					\
									\
  if (restore_charset) {						\
    if (send_charset && (<"latin1", "utf8">)[charset])			\
      send_charset = charset;						\
    else {								\
      CH_DEBUG ("Restoring charset %O.\n", restore_charset);		\
      ::big_query ("SET character_set_client=" + restore_charset);	\
      /* Can't be changed automatically - has side effects. /mast */	\
      /* ::big_query("SET character_set_connection=" + restore_charset); */ \
    }									\
  }									\
									\
  if (!objectp(res)) return res;					\
									\
  if (utf8_mode & UNICODE_DECODE_MODE) {				\
    CH_DEBUG ("Using unicode wrapper for result.\n");			\
    return								\
      HAVE_MYSQL_FIELD_CHARSETNR_IFELSE (				\
	.sql_util.MySQLUnicodeWrapper(res),				\
	.sql_util.MySQLBrokenUnicodeWrapper (res));			\
  }									\
  return res;

Mysql.mysql_result big_query (string query,
			      mapping(string|int:mixed)|void bindings,
			      void|string charset)
//! Sends a query to the server.
//!
//! @param query
//!   The SQL query.
//!
//! @param bindings
//!   An optional bindings mapping. See @[Sql.query] for details about
//!   this.
//!
//! @param charset
//!   An optional charset that will be used temporarily while sending
//!   @[query] to the server. If necessary, a query
//!   @code
//!     SET character_set_client=@[charset]
//!   @endcode
//!   is sent to the server first, then @[query] is sent as-is, and then
//!   the connection charset is restored again (if necessary).
//!
//!   Primarily useful with @[charset] set to @expr{"latin1"@} if
//!   unicode encode mode (see @[set_unicode_encode_mode]) is enabled
//!   (the default) and you have some large queries (typically blob
//!   inserts) where you want to avoid the query parsing overhead.
//!
//! @returns
//!   A @[Mysql.mysql_result] object is returned if the query is of a
//!   kind that returns a result. Zero is returned otherwise.
//!
//! @seealso
//!   @[Sql.big_query]
{
  QUERY_BODY (big_query);
}

Mysql.mysql_result streaming_query (string query,
				    mapping(string|int:mixed)|void bindings,
				    void|string charset)
//! Makes a streaming SQL query.
//!
//! This function sends the SQL query @[query] to the Mysql-server.
//! The result of the query is streamed through the returned
//! @[Mysql.mysql_result] object. Note that the involved database
//! tables are locked until all the results has been read.
//!
//! In all other respects, it behaves like @[big_query].
{
  QUERY_BODY (streaming_query);
}

int(0..1) is_keyword( string name )
//! Return 1 if the argument @[name] is a mysql keyword that needs to
//! be quoted in a query. The list is currently up-to-date with MySQL
//! 5.1.
{
  return ([
    "accessible": 1, "add": 1, "all": 1, "alter": 1, "analyze": 1, "and": 1,
    "as": 1, "asc": 1, "asensitive": 1, "before": 1, "between": 1, "bigint": 1,
    "binary": 1, "blob": 1, "both": 1, "by": 1, "call": 1, "cascade": 1,
    "case": 1, "change": 1, "char": 1, "character": 1, "check": 1, "collate": 1,
    "column": 1, "condition": 1, "constraint": 1, "continue": 1, "convert": 1,
    "create": 1, "cross": 1, "current_date": 1, "current_time": 1,
    "current_timestamp": 1, "current_user": 1, "cursor": 1, "database": 1,
    "databases": 1, "day_hour": 1, "day_microsecond": 1, "day_minute": 1,
    "day_second": 1, "dec": 1, "decimal": 1, "declare": 1, "default": 1,
    "delayed": 1, "delete": 1, "desc": 1, "describe": 1, "deterministic": 1,
    "distinct": 1, "distinctrow": 1, "div": 1, "double": 1, "drop": 1,
    "dual": 1, "each": 1, "else": 1, "elseif": 1, "enclosed": 1, "escaped": 1,
    "exists": 1, "exit": 1, "explain": 1, "false": 1, "fetch": 1, "float": 1,
    "float4": 1, "float8": 1, "for": 1, "force": 1, "foreign": 1, "from": 1,
    "fulltext": 1, "grant": 1, "group": 1, "having": 1, "high_priority": 1,
    "hour_microsecond": 1, "hour_minute": 1, "hour_second": 1, "if": 1,
    "ignore": 1, "in": 1, "index": 1, "infile": 1, "inner": 1, "inout": 1,
    "insensitive": 1, "insert": 1, "int": 1, "int1": 1, "int2": 1, "int3": 1,
    "int4": 1, "int8": 1, "integer": 1, "interval": 1, "into": 1, "is": 1,
    "iterate": 1, "join": 1, "key": 1, "keys": 1, "kill": 1, "leading": 1,
    "leave": 1, "left": 1, "like": 1, "limit": 1, "linear": 1, "lines": 1,
    "load": 1, "localtime": 1, "localtimestamp": 1, "lock": 1, "long": 1,
    "longblob": 1, "longtext": 1, "loop": 1, "low_priority": 1,
    "master_ssl_verify_server_cert": 1, "match": 1, "mediumblob": 1,
    "mediumint": 1, "mediumtext": 1, "middleint": 1, "minute_microsecond": 1,
    "minute_second": 1, "mod": 1, "modifies": 1, "natural": 1, "not": 1,
    "no_write_to_binlog": 1, "null": 1, "numeric": 1, "on": 1, "optimize": 1,
    "option": 1, "optionally": 1, "or": 1, "order": 1, "out": 1, "outer": 1,
    "outfile": 1, "precision": 1, "primary": 1, "procedure": 1, "purge": 1,
    "range": 1, "read": 1, "reads": 1, "read_only": 1, "read_write": 1,
    "real": 1, "references": 1, "regexp": 1, "release": 1, "rename": 1,
    "repeat": 1, "replace": 1, "require": 1, "restrict": 1, "return": 1,
    "revoke": 1, "right": 1, "rlike": 1, "schema": 1, "schemas": 1,
    "second_microsecond": 1, "select": 1, "sensitive": 1, "separator": 1,
    "set": 1, "show": 1, "smallint": 1, "spatial": 1, "specific": 1, "sql": 1,
    "sqlexception": 1, "sqlstate": 1, "sqlwarning": 1, "sql_big_result": 1,
    "sql_calc_found_rows": 1, "sql_small_result": 1, "ssl": 1, "starting": 1,
    "straight_join": 1, "table": 1, "terminated": 1, "then": 1, "tinyblob": 1,
    "tinyint": 1, "tinytext": 1, "to": 1, "trailing": 1, "trigger": 1,
    "true": 1, "undo": 1, "union": 1, "unique": 1, "unlock": 1, "unsigned": 1,
    "update": 1, "usage": 1, "use": 1, "using": 1, "utc_date": 1, "utc_time": 1,
    "utc_timestamp": 1, "values": 1, "varbinary": 1, "varchar": 1,
    "varcharacter": 1, "varying": 1, "when": 1, "where": 1, "while": 1,
    "with": 1, "write": 1, "x509": 1, "xor": 1, "year_month": 1, "zerofill": 1,
    // The following keywords were in the old list, but according to MySQL
    // docs they don't need to be quoted:
    // "action", "after", "aggregate", "auto_increment", "avg",
    // "avg_row_length", "bit", "bool", "change", "checksum", "columns",
    // "comment", "data", "date", "datetime", "day", "dayofmonth", "dayofweek",
    // "dayofyear", "delay_key_write", "end", "enum", "escape", "escaped",
    // "explain", "fields", "file", "first", "flush", "for", "full", "function",
    // "global", "grants", "heap", "hosts", "hour", "identified", "if",
    // "insert_id", "integer", "interval", "isam", "last_insert_id", "length",
    // "lines", "local", "logs", "max", "max_rows", "mediumtext", "min_rows",
    // "minute", "modify", "month", "monthname", "myisam", "no", "numeric",
    // "pack_keys", "partial", "password", "privileges", "process",
    // "processlist", "reload", "returns", "row", "rows", "second", "shutdown",
    // "soname", "sql_big_selects", "sql_big_tables", "sql_log_off",
    // "sql_log_update", "sql_low_priority_updates", "sql_select_limit",
    // "sql_small_result", "sql_warnings", "status", "straight_join", "string",
    // "tables", "temporary", "text", "time", "timestamp", "tinytext",
    // "trailing", "type", "use", "using", "varbinary", "variables", "with",
    // "write", "year"
  ])[ lower_case(name) ];
}

static void create(string|void host, string|void database,
		   string|void user, string|void password,
		   mapping(string:string|int)|void options)
{
  if (options) {
    string charset = options->mysql_charset_name ?
      lower_case (options->mysql_charset_name) : "latin1";

    int broken_unicode = charset == "broken-unicode";
    if (broken_unicode) charset = "unicode";

    if (charset == "unicode")
      options->mysql_charset_name = "utf8";

    ::create(host||"", database||"", user||"", password||"", options);

    update_unicode_encode_mode_from_charset (lower_case (charset));

#if !constant (Mysql.mysql.HAVE_MYSQL_FIELD_CHARSETNR)
    // Undocumented feature for old mysql libs. See
    // MySQLBrokenUnicodeWrapper for details.
    if (broken_unicode || getenv ("PIKE_BROKEN_MYSQL_UNICODE_MODE")) {
#endif
      if (charset == "unicode")
	utf8_mode |= UNICODE_DECODE_MODE;
      else if (options->unicode_decode_mode)
	set_unicode_decode_mode (1);
#if !constant (Mysql.mysql.HAVE_MYSQL_FIELD_CHARSETNR)
    }
    else
      if (charset == "unicode" || options->unicode_decode_mode)
	predef::error ("Unicode decode mode not supported - "
		       "compiled with MySQL client library < 4.1.0.\n");
#endif

  } else {
    ::create(host||"", database||"", user||"", password||"");

    update_unicode_encode_mode_from_charset ("latin1");
  }
}

#else
constant this_program_does_not_exist=1;
#endif /* constant(Mysql.mysql) */
