#pike __REAL_VERSION__

//! Abstract language locale class, inherited by all the language locale classes.

// $Id$

//! Array(string) with the months of the year, beginning with January.
constant months = ({ "", "", "", "", "", "", "", "", "", "", "", "" });

//! Array(string) with the days of the week, beginning with Sunday.
constant days = ({ "", "", "", "", "", "", "" });

// Array(string) with all the language's identifiers
constant aliases = ({});

//! String with the language code in ISO-639-1 (two character code). Note that
//! all languages does not have a ISO-639-1 code.
optional constant iso_639_1 = 0;

//! String with the language code in ISO-639-2/T (three character code).
constant iso_639_2 = 0;

//! String with the language code in ISO-639-2/B (three character code). This
//! is usually the same as the ISO-639-2/T code (@[iso_639_2]).
constant iso_639_2B = 0;

//! The name of the language in english.
constant english_name = "Unknown";

//! The name of the langauge. E.g. "svenska" for Swedish.
constant name = "Unknown";

//! Mapping(string:string) that maps an ISO-639-2/T id code to the
//! name of that language.
constant languages = ([]);

//! Returns the name of month number @[num].
string month(int(1..12) num)
{
  return months[ num - 1 ];
}

//! Returns the name of weekday number @[num].
string day(int(1..7) num)
{
  return days[ num - 1 ];
}

//! Returns the number @[i] as a string.
string number(int i)
{
  return (string)i;
}

//! Returns the ordered number @[i] as a string.
string ordered(int i)
{
  return (string)i;
}

//! Returns the date for posix time @[timestamp] as a textual string.
//! @param mode
//!   Determines what kind of textual string should be produced.
//!   @string
//!     @value "time"
//!       I.e. "06:36"
//!     @value "date"
//!       I.e. "January the 17th in the year of 2002"
//!     @value "full"
//!       I.e. "06:37, January the 17th, 2002"
//!   @endstring
//! @example
//! > Locale.Language.eng()->date(time());
//! Result: "today, 06:36"
string date(int timestamp, string|void mode)
{
  mapping lt=localtime(timestamp);
  return sprintf("%4d-%02d-%02d", lt->year+1900, lt->mon+1, lt->mday);
}
