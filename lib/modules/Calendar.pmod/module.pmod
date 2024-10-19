/* -*- mode: Pike; c-basic-offset: 3; -*- */

#pike __REAL_VERSION__

protected private int stage=0;
protected private int booted=0;
protected private object defcal;
protected private object iso_utc;
protected private object default_rules;
protected constant magic= // magic + indices(Calendar.ISO) without YMD
(<
   "ISO_UTC","II", "default_rules",
   "_sprintf", "set_timezone", "language", "Day", "Year", "Week",
   "Month", "Hour", "Minute", "datetime", "format_iso",
   "format_iso_short", "format_iso_tod", "YMD_Time", "parse", "dwim_day",
   "dwim_time", "datetime_name", "datetime_short_name", "format_day_iso",
   "format_day_iso_short", "SuperTimeRange",
   "calendar_name", "calendar_object", "TimeRange",
   "nulltimerange", "ruleset", "set_ruleset", "inano", "timezone",
   "set_language", "default_rules", "TimeofDay",
   "Second", "Fraction", "now", "Bahai" >);

protected array _indices()
{
  return (array)magic;
}

//! Time and day system
//!
//!@code
//!Q:  I need to parse some date in a non-strict format, like
//!    the one in the HTTP or mail protocol, or from a user web
//!    form.
//!
//!A:  Calendar.dwim_day, or Calendar.dwim_time, should solve
//!    your problem.
//!
//!      > Calendar.dwim_day("1/2/3");
//!      Result: Day(Thu 2 Jan 2003)
//!      > Calendar.dwim_day("1 aug 2001");
//!      Result: Day(Wed 1 Aug 2001)
//!
//!      > Calendar.dwim_time("1 aug 2001 23:14 EDT");
//!      Result: Minute(Wed 1 Aug 2001 23:14 EDT)
//!      > Calendar.dwim_time("2001 2 3 23:14:23 UTC+9");
//!      Result: Second(Sat 3 Feb 2001 23:14:23 UTC+9)
//!
//!    If it doesn't, and it should, report the problem to me
//!    and I'll see what I can do. Note that the timezones
//!    are rather unpredictable - if it doesn't get it, you
//!    will get the default (local) timezone.
//!
//!-------------------------------------------------------------------------
//!
//!Q:  The dwim_* functions are too slow.
//!
//!A:  They are not written to be fast, but to do good guessing.
//!
//!    If you know the format, you should use the Calendar.parse
//!    function:
//!
//!      > Calendar.parse("%Y-%M-%D %h:%m","2040-11-08 2:46");
//!      Result: Minute(Thu 8 Nov 2040 2:46 CET)
//!      > Calendar.parse("%Y w%W %e %h:%m %p %z","1913 w4 monday 2:14 pm CET");
//!      Result: Minute(Mon 20 Jan 1913 14:14 CET)
//!
//!    These are the format characters:
//!     %Y absolute year
//!     %y dwim year (70-99 is 1970-1999, 0-69 is 2000-2069)
//!     %M month (number, name or short name) (needs %y)
//!     %W week (needs %y)
//!     %D date (needs %y, %m)
//!     %d short date (20000304, 000304)
//!     %a day (needs %y)
//!     %e weekday (needs %y, %w)
//!     %h hour (needs %d, %D or %W)
//!     %m minute (needs %h)
//!     %s second (needs %m)
//!     %f fraction of a second (needs %s)
//!     %t short time (205314, 2053)
//!     %z zone
//!     %p "am" or "pm"
//!     %n empty string (to be put at the end of formats)
//!
//!    and you can also use "%*[....]" to skip some characters,
//!    as in sscanf().
//!
//!    If this is too slow, there is currently no solution in Pike
//!    to do this faster, except possibly sscanf and manual calculations/
//!    time object creation.
//!
//!-------------------------------------------------------------------------
//!
//!Q:  How do I get from unix time (time(2)) to a unit and back?
//!
//!A:  Calendar.Unit("unix",time())
//!    unit->unix_time()
//!
//!      > Calendar.Day("unix",987654321);
//!      Result: Day(Thu 19 Apr 2001)
//!      > Calendar.Second("unix",987654321);
//!      Result: Second(Thu 19 Apr 2001 6:25:21 CEST)
//!
//!      > Calendar.Day()->unix_time();
//!      Result: 979081200
//!
//!    Note that you will get the time for the start of the unit.
//!    Unix time is timezone independant.
//!
//!    The day-of-time units (seconds, hours, etc) uses this
//!    as internal representation of time.
//!
//!-------------------------------------------------------------------------
//!
//!Q:  I'm a mad astronomer, how do I do the same conversions with
//!    julian day numbers?
//!
//!A:  Julian day numbers are used as the internal representation
//!    for the day, and for most other bigger-than-time-of-day calculations.
//!
//!      > Calendar.Day("julian",2454545);
//!      Result: Day(Wed 19 Mar 2008)
//!      > Calendar.Second("julian",2430122.0);
//!      Result: Second(Tue 6 May 1941 13:00:00 CET)
//!
//!    Julian day numbers from day units and bigger are integers,
//!    representing the new julian number on that day. Julian day
//!    numbers from time of day units are represented in floats.
//!
//!      > Calendar.Day()->julian_day();
//!      Result: 2451920
//!      > Calendar.Second()->julian_day();
//!      Result: 2451919.949595
//!
//!    Watch out for the float precision, though. If you haven't
//!    compiled your Pike with --with-double-precision, this gives
//!    you awkwardly low precision - 6 hours.
//!
//!-------------------------------------------------------------------------
//!
//!Q:  How do I convert a "Second(Sat 3 Feb 2001 23:14:23 UTC+9)" object
//!    to my timezone?
//!
//!A:  ->set_timezone(your timezone)
//!
//!      > Calendar.dwim_time("2001 2 3 23:14:23 PST")
//!      	  ->set_timezone("Europe/Stockholm");
//!      Result: Second(Sun 4 Feb 2001 8:14:23 CET)
//!
//!      > Calendar.dwim_time("2001 2 3 23:14:23 PST")
//!      	  ->set_timezone("locale");
//!      Result: Second(Sun 4 Feb 2001 8:14:23 CET)
//!
//!-------------------------------------------------------------------------
//!
//!Q:  How do I print my time object?
//!
//!A:  ->format_xxx();
//!
//!    You can either print it unit-sensitive,
//!
//!      > Calendar.dwim_time("2001 2 3 23:14:23 PST")->format_nice();
//!      Result: "3 Feb 2001 23:14:23"
//!      > Calendar.Week()->format_nice();
//!      Result: "w2 2001"
//!      > Calendar.now()->format_nicez();
//!      Result: "10 Jan 10:51:15.489603 CET"
//!
//!    or in a format not depending on the unit,
//!
//!      > Calendar.Week()->format_ymd();
//!      Result: "2001-01-08"
//!      > Calendar.Day()->format_time();
//!      Result: "2001-01-10 00:00:00"
//!
//!    This is all the formats:
//!
//!    format_ext_time       "Wednesday, 10 January 2001 10:49:57"
//!    format_ext_time_short "Wed, 10 Jan 2001 10:49:57 CET"
//!    format_ext_ymd        "Wednesday, 10 January 2001"
//!    format_iso_time       "2001-01-10 (Jan) -W02-3 (Wed) 10:49:57 UTC+1"
//!    format_iso_ymd        "2001-01-10 (Jan) -W02-3 (Wed)"
//!    format_mod            "10:49"
//!    format_month          "2001-01"
//!    format_month_short    "200101"
//!    format_mtime          "2001-01-10 10:49"
//!    format_time           "2001-01-10 10:49:57"
//!    format_time_short     "20010110 10:49:57"
//!    format_time_xshort    "010110 10:49:57"
//!    format_tod            "10:49:57"
//!    format_tod_short      "104957"
//!    format_todz           "10:49:57 CET"
//!    format_todz_iso       "10:49:57 UTC+1"
//!    format_week           "2001-w2"
//!    format_week_short     "2001w2"
//!    format_iso_week       "2001-W02"
//!    format_iso_week_short "200102"
//!    format_xtime          "2001-01-10 10:49:57.539198"
//!    format_xtod           "10:49:57.539658"
//!    format_ymd            "2001-01-10"
//!    format_ymd_short      "20010110"
//!    format_ymd_xshort     "010110"
//!
//!    format_ctime          "Wed Jan 10 10:49:57 2001\n"
//!    format_smtp           "Wed, 10 Jan 2001 10:49:57 +0100"
//!    format_http           "Wed, 10 Jan 2001 09:49:57 GMT"
//!
//!-------------------------------------------------------------------------
//!
//!Q:  How old am I?
//!
//!A:  First, you need to create the time period representing your age.
//!
//!      > object t=Calendar.dwim_time("1638 dec 23 7:02 pm")
//!      	  ->distance(Calendar.now());
//!      Result: Fraction(Thu 23 Dec 1638 19:02:00.000000 LMT -
//!      		       Wed 10 Jan 2001 10:53:33.032856 CET)
//!
//!   Now, you can ask for instance how many years this is:
//!
//!      > t->how_many(Calendar.Year);
//!      Result: 362
//!
//!   Or how many 17 seconds it is:
//!
//!      > t->how_many(Calendar.Second()*17);
//!      Result: 672068344
//!
//!   A note here is to use ->distance, and not ->range, since that
//!   will include the destination unit too:
//!
//!     > Calendar.dwim_day("00-01-02")->range(Calendar.Week(2000,2))
//!        ->how_many(Calendar.Day());
//!     Result: 15
//!     > Calendar.dwim_day("00-01-02")->distance(Calendar.Week(2000,2))
//!        ->how_many(Calendar.Day());
//!     Result: 8
//!
//!-------------------------------------------------------------------------
//!
//!Q:  In 983112378 days, what weekday will it be?
//!
//!A:  (this weekday + 983112378) % 7   ;)
//!
//!    or take this day, add the number, and ask the object:
//!
//!      > (Calendar.Day()+983112378)->week_day_name();
//!      Result: "Saturday"
//!
//!    "+int" will add this number of the unit to the unit;
//!    this means that Calendar.Year()+2 will move two years
//!    forward, but Calendar.now()+2 will not move at all
//!    - since now has zero size.
//!
//!    To add a number of another time unit, simply do that:
//!
//!      > Calendar.Day()+3*Calendar.Year();
//!      Result: Day(Sat 10 Jan 2004)
//!      > Calendar.Day()+3*Calendar.Minute()*134;
//!      Result: Minute(Wed 10 Jan 2001 6:42 CET - Thu 11 Jan 2001 6:42 CET)
//!
//!    The last result here is because the resulting time still will
//!    be as long as the first.
//!
//!-------------------------------------------------------------------------
//!
//!Q:  Are there other calendars?
//!
//!A:  Yes.
//!
//!    Calendar.Day is really a shortcut to Calendar.ISO.Day.
//!
//!    There is currently:
//!
//!    Gregorian
//!	This is the base module for Julian style calendars;
//!	despite the name. Most calendars of today are in sync
//!	with the Gregorian calendar.
//!    ISO
//!	This inherits the Gregorian calendar to tweak it to
//!	conform to the ISO standards. Most affected are weeks,
//!	which starts on Monday in the ISO calendar.
//!	This is also the default calendar.
//!    Discordian
//!	The Discordian calendar as described in Principia Discordia
//!	is in sync with the Gregorian calendar (although some claim
//!	that it should be the Julian - I go with what I can read
//!	from my Principia Discordia). The module inherits and
//!	tweaks the Gregorian module.
//!    Coptic
//!	The Coptic calendar is by some sources ("St. Marks'
//!	Coptic Orthodox Church" web pages) is for now on in sync with
//!	the Gregorian Calendar, so this module too inherits
//!	and tweaks the Gregorian module. It needs to be
//!	adjusted for historical use.
//!    Julian
//!	This is the Julian calendar, with the small changes
//!	to the Gregorian calendar (leap years).
//!    Badi (Baha'i)
//!        The Badi calendar used by the Baha'i religion is based on the
//!        solar year. For the time being it is in sync with the Gregorian
//!        calendar.
//!
//!    Islamic
//!	This is the Islamic calendar, using the 'Calendrical
//!	Calculations' rules for new moon. It is based
//!	directly on the YMD module.
//!    Stardate
//!	This is the (TNG) Stardate calendar, which consists
//!	of one time unit only, the Tick (1000 Tick is one earth year).
//!	It is based directly on TimeRanges.
//!
//!-------------------------------------------------------------------------
//!
//!Q:  How do I convert between the calendars?
//!
//!A:  You give the unit to be converted to the constructor of
//!    the unit you want it to be.
//!
//!    > Calendar.Coptic.Day(Calendar.dwim_day("14 feb 1983"));
//!    Result: Day(Mon 7 Ams 1699)
//!    > Calendar.Islamic.Minute(Calendar.dwim_day("14 feb 1983"));
//!    Result: Minute(aha 29 Rebîul-âchir 1403 AH 13:00 CET -
//!    		   ith 1 Djumâda'l-ûla 1403 AH 13:00 CET)
//!    > Calendar.Day(Calendar.Stardate.Tick(4711));
//!    Result: Day(Sat 17 Sep 2327 0:00 sharp)
//!
//!-------------------------------------------------------------------------
//!
//!Q:  Isn't there a <my country> calendar?
//!
//!A:  <your country> uses the ISO calendar, with just different
//!    names for the months. Language is a parameter to the
//!    calendar units, as well as the timezone.
//!
//!    You set the language by using ->set_language(yourlanguage).
//!
//!      > t->set_language("pt")->format_ext_ymd();
//!      Result: "Quarta-feira, 10 Janeiro 2001"
//!      > t->set_language("roman")->format_ext_ymd();
//!      Result: "Mercurii dies, X Ianuarius MMDCCLIII ab urbe condita"
//!
//!    Note that all languages aren't supported. If you miss your
//!    favourite language or I got it all wrong (or have some time over
//!    to help me out), look in the Language.pmod file and send me an
//!    update.
//!
//!    Or send me a list of the weekdays and month names
//!    (please start with Monday and January).
//!
//!    Currently, these languages are supported:
//!
//!      name        code
//!      -------------------------------
//!      ISO                 (default, aka English)
//!
//!      Afrikaans   af afr   (South Africa),
//!      Austrian    de_AT
//!      Basque      eu eus   (Spain)
//!      Catalan     ca cat   (Catalonia)
//!      Croatian    hr hrv
//!      Danish      da dan
//!      Dutch       nl nld
//!      English     en eng
//!      Estonian    et est
//!      Faroese     fo fao
//!      Finnish     fi fin
//!      French      fr fra
//!      Galician    gl glg   (Spain)
//!      German      de deu
//!      Greenlandic kl kal
//!      Hungarian   hu hun
//!      Icelandic   is isl
//!      Irish       ga gle   (Gaelic)
//!      Italian     it ita
//!      Latvian     lv lav
//!      Lithuanian  lt lit
//!      Norwegian   no nor
//!      Persian     fa fas   (Iran)
//!      Polish      pl pol
//!      Portugese   pt por
//!      Romanian    ro ron
//!      Serbian     sr srp   (Yugoslavia)
//!      Slovenian   sl slv
//!      Spanish     es spa
//!      Swedish     sv swe
//!      Turkish     tr
//!      Welsh       cy cym
//!
//!      Latin       la lat
//!      Roman              (Roman Latin)
//!
//!-------------------------------------------------------------------------
//!
//!Q:  Isn't there a <whatever> calendar?
//!
//!A:  Not if it isn't listed above. I'll appreciate any
//!    implementation help if you happen to have the time over
//!    to implement some calendar.
//!
//!    I know I miss these:
//!
//!      Chinese
//!      Jewish or Hebreic
//!      Maya
//!
//!    Of these, the two first are based on astronomical events,
//!    which I haven't had the time to look into yet, but the
//!    last - Maya - is totally numeric.
//!
//!-------------------------------------------------------------------------
//!
//!Q:  I don't like that weeks starts on Mondays.
//!    Every school kids knows that weeks start on Sundays.
//!
//!A:  According to the ISO 8601 standard, weeks start on mondays.
//!
//!    If you don't like it, use Calendar.Gregorian.Day, etc.
//!
//!-------------------------------------------------------------------------
//!
//!Q:  How do I find out which days are red in a specific region?
//!
//!A:  Events.<region>
//!
//!    - contains the events for the region, as a SuperEvent.
//!    You can ask this object to filter out the holidays,
//!
//!       Events.se->holidays();
//!
//!    which will be a superevent containing only holidays.
//!
//!    To use this information, you can for instance use ->scan,
//!    here in an example to see what red days there are in Sweden
//!    the current month:
//!
//!      > Calendar.Events.se->filter_flag("h")->scan(Calendar.Month());
//!      Result: ({ /* 6 elements */
//!     		   Day(Sun 7 Jan 2001),
//!     		   Day(Sun 14 Jan 2001),
//!     		   Day(Sun 21 Jan 2001),
//!     		   Day(Sun 28 Jan 2001),
//!     		   Day(Sat 6 Jan 2001),
//!     		   Day(Mon 1 Jan 2001)
//!     	       })
//!
//!-------------------------------------------------------------------------
//!
//!Q:  How accurate are the events information?
//!
//!A:  For some regions, very. For most regions, not very.
//!
//!    The first reason is lack of information of this kind on
//!    the web, especially sorted into useful rules (like "the
//!    third monday after 23 dec", not "8 jan").
//!
//!    The second reason is lack of time and interest to do
//!    research, which is a rather tedious job.
//!
//!    If you want to help, the check your region in the
//!    events/regions file and send us <pike@@roxen.com> a patch.
//!
//!    Don't send me "the x region is all wrong!" mails without
//!    telling me what it should look like.
//!
//!-------------------------------------------------------------------------
//!
//!Q:  My timezone says it's DST. It's wrong.
//!
//!A:  No it isn't.  But:
//!
//!    o The local timezone detector failed to find your timezone by
//!      itself, or found the wrong timezone.
//!
//!    o or you use the wrong timezone.
//!
//!    To make sure the right timezone is used, use the standard
//!    timezone names. Those aren't "CET" or "PST", but
//!    "Europe/Amsterdam" or "America/Dawson".
//!
//!    OR this may be in the future and you have a changed DST
//!    rule and uses an old Pike. Then you can either download
//!    a new version or download new timezone data files from
//!    the ftp address below (if the internet still is there).
//!@endcode
//!
//! @fixme
//!
//! This needs to be reformatted as documentation.

//! @module ISO_UTC
//!   @[Calendar.ISO] with the timezone set to @expr{"UTC"@}.

//! @decl inherit ISO

//! @endmodule

//! @decl inherit ISO_UTC
//!
//! Symbol lookups directly in @[Calendar] default to
//! looking up the same symbol in @[Calendar.ISO_UTC].

//! @module Bahai
//!   This is an alias for @[Calendar.Badi].

//! @decl inherit Badi

//! @endmodule

constant II = 1;
//! Recongnition constant for Calendar module API 2.

//! @module default_rules
//!   This is the default ruleset (which is ISO).

//! @decl inherit Ruleset

//! @endmodule

#if 1
protected mixed `[](string what)
{
   if (!booted)
   {
      if (what == "_module_value") return UNDEFINED;
      booted++;
      stage++;

// load ISO
// it can crash here if you're loading from compiled modules
// that is updated without all of the calendar module is updated
      iso_utc=master()->resolv("Calendar")["ISO"];
      iso_utc=iso_utc->set_timezone("UTC");
      object Time = master()->resolv("Calendar")["Time"];
      Time->Day = iso_utc->cDay;
      stage--;

      // Timezone dynamically loads ISO_UTC. It is however the only
      // part of Calendar that uses ISO_UTC.
      object tz=
         master()->resolv("Calendar")["Timezone"]["locale"];
      default_rules->timezone=tz; // destructive!
   }

   // If it isn't magic, use the master resolver.
   if ( !magic[what] || (stage && what!="default_rules")) return UNDEFINED;

   switch (what)
   {
      case "Bahai":
         return master()->resolv("Calendar")["Badi"];
      case "ISO_UTC":
         return iso_utc;
      case "II":
	 return 1;
      case "default_rules":
         if (!default_rules)
	 {
	    default_rules=master()->resolv("Calendar")["Ruleset"]();
            default_rules=default_rules->set_language("ISO");
	 }
         return default_rules;
   }
   if (!defcal)
   {
      stage++;
      defcal=master()->resolv("Calendar")["ISO"];
      stage--;
   }
   return defcal[what];
}

protected mixed `-> (string what)
{
  // This becomes an alias.
  return `[] (what);
}
#endif
