#pike 8.1

inherit Protocols.HTTP;

string http_encode_string(string in)
//! This is a deprecated alias for @[uri_encode], for compatibility
//! with Pike 7.6 and earlier.
//!
//! In 7.6 this function didn't handle 8-bit and wider chars
//! correctly. It encoded 8-bit chars directly to @tt{%XX@} escapes,
//! and it used nonstandard @tt{%uXXXX@} escapes for 16-bit chars.
//!
//! That is considered a bug, and hence the function is changed. If
//! you need the old buggy encoding then use the 7.6 compatibility
//! version (@expr{#pike 7.6@}).
//!
//! @deprecated uri_encode
{
  return uri_encode (in);
}


//! This function used to claim that it encodes the specified string
//! according to the HTTP cookie standard. If fact it does not - it
//! applies URI-style (i.e. @expr{%XX@}) encoding on some of the
//! characters that cannot occur literally in cookie values. There
//! exist some web servers (read Roxen and forks) that usually perform
//! a corresponding nonstandard decoding of %-style escapes in cookie
//! values in received requests.
//!
//! This function is deprecated. The function @[quoted_string_encode]
//! performs encoding according to the standard, but it is not safe to
//! use with arbitrary chars. Thus URI-style encoding using
//! @[uri_encode] or @[percent_encode] is normally a good choice, if
//! you can use @[uri_decode]/@[percent_decode] at the decoding end.
//!
//! @deprecated
string http_encode_cookie(string f)
{
   return replace(
      f,
      ({ "\000", "\001", "\002", "\003", "\004", "\005", "\006", "\007",
	 "\010", "\011", "\012", "\013", "\014", "\015", "\016", "\017",
	 "\020", "\021", "\022", "\023", "\024", "\025", "\026", "\027",
	 "\030", "\031", "\032", "\033", "\034", "\035", "\036", "\037",
	 "\177",
	 "\200", "\201", "\202", "\203", "\204", "\205", "\206", "\207",
	 "\210", "\211", "\212", "\213", "\214", "\215", "\216", "\217",
	 "\220", "\221", "\222", "\223", "\224", "\225", "\226", "\227",
	 "\230", "\231", "\232", "\233", "\234", "\235", "\236", "\237",
	 " ", "%", "'", "\"", ",", ";", "=", ":" }),
      ({ 
	 "%00", "%01", "%02", "%03", "%04", "%05", "%06", "%07",
	 "%08", "%09", "%0a", "%0b", "%0c", "%0d", "%0e", "%0f",
	 "%10", "%11", "%12", "%13", "%14", "%15", "%16", "%17",
	 "%18", "%19", "%1a", "%1b", "%1c", "%1d", "%1e", "%1f",
	 "%7f",
	 "%80", "%81", "%82", "%83", "%84", "%85", "%86", "%87",
	 "%88", "%89", "%8a", "%8b", "%8c", "%8d", "%8e", "%8f",
	 "%90", "%91", "%92", "%93", "%94", "%95", "%96", "%97",
	 "%98", "%99", "%9a", "%9b", "%9c", "%9d", "%9e", "%9f",
	 "%20", "%25", "%27", "%22", "%2c", "%3b", "%3d", "%3a" }));
}

//! Helper function for replacing HTML entities with the corresponding
//! unicode characters.
//! @deprecated Parser.parse_html_entities
string unentity(string s)
{
  return Parser.parse_html_entities(s,1);
}
