#pike __REAL_VERSION__

static private int stage=0;
static private int booted=0;
static private object defcal;
static private object iso_utc;
static private object default_rules;
static constant magic= // magic + indices(Calendar.ISO) without YMD
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
   "Second", "Fraction", "now" >);

#include "localization.h"

#if 1
static mixed `[](string what)
{
   if (!booted)
   {
      booted++;
      stage++;
// bootstrap in the right order
      master()->resolv("Calendar")["Timezone"];
      master()->resolv("Calendar")["TimeRanges"];
      master()->resolv("Calendar")["Time"];
      master()->resolv("Calendar")["YMD"];
      master()->resolv("Calendar")["Gregorian"];

// load ISO
// it can crash here if you're loading from compiled modules
// that is updated without all of the calendar module is updated
      iso_utc=master()->resolv("Calendar")["ISO"];
      iso_utc=iso_utc->set_timezone("UTC");
      stage--;
      object tz=
	 master()->resolv("Calendar")["Timezone"][default_timezone];
      if (!tz) 
	 error("Failed to make default timezone %O\n",default_timezone);
      else
	 default_rules->timezone=tz; // destructive!
   }
   if ( !magic[what] || (stage && what!="default_rules")) return UNDEFINED;
   switch (what)
   {
      case "ISO_UTC":
	 if (!iso_utc)
	    error("ERROR\n");
	 return iso_utc;
      case "II":
	 return 1;
      case "default_rules":
	 if (!default_rules)
	 {
	    default_rules=master()->resolv("Calendar")["Ruleset"]();
	    default_rules=default_rules->set_language(default_language);
	 }
   // load ISO_UTC and set timezone there
//  	 if (!iso_utc) `[]("ISO_UTC");
   // timezone will be set on the way out, through boot above
	 return default_rules;
   }
   if (!defcal)
   {
      if (!iso_utc) `[]("ISO_UTC");
      stage++;
      defcal=master()->resolv("Calendar")[default_calendar];
      stage--;
   }
   return defcal[what];
}
static mixed `-> = `[];
#endif
