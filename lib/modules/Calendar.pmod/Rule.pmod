/* -*- mode: Pike; c-basic-offset: 3; -*- */

#pike __REAL_VERSION__

//! Contains a time zone.
class Timezone
{
   constant is_timezone=1;

  // seconds to utc, not counting DST
  protected int offset_to_utc;

  // timezone name
  string name = "-";
  string `zoneid() { return name; }

  //! @param offset
  //!   Offset to UTC, not counting DST.
  //! @param name
  //!   The name of the time zone.
  protected void create(int offset, string name)
  {
    offset_to_utc=offset;
    this::name=name;
  }

   // seconds to UTC, counting DST

  //! @fixme
  //! This method takes one integer argument, ignores it and
  //! returns an array with the UTC offset and the timezone name.
  array(int) tz_ux(int unixtime) {
    return ({offset_to_utc,name});
  }

  //! @fixme
  //! This method takes one integer argument, ignores it and
  //! returns an array with the UTC offset and the timezone name.
  array(int) tz_jd(int julian_day) {
    return ({offset_to_utc,name});
  }

  protected string _sprintf(int t)
  {
    return t=='O' &&
       sprintf("Calendar.Rule.Timezone(%O, %O)", offset_to_utc, name);
  }

  //! Returns the offset to UTC, not counting DST.
  int raw_utc_offset() { return offset_to_utc; }
}

class Language
{
   constant is_language=1;

   string month_name_from_number(int n);
   string month_shortname_from_number(int n);
   int month_number_from_name(string name);

   string week_day_name_from_number(int n);
   string week_day_shortname_from_number(int n);
   int week_day_number_from_name(string name);

   string gregorian_week_day_name_from_number(int n);
   string gregorian_week_day_shortname_from_number(int n);
   int gregorian_week_day_number_from_name(string name);

   string week_name_from_number(int n);
   int week_number_from_name(string s);
   string year_name_from_number(int y);
}
