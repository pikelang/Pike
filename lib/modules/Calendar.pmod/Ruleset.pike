//! This is the container class for rules.


#pike __REAL_VERSION__

class Timezone
{
   constant is_timezone=1;

// seconds to utc, not counting DST
   static int offset_to_utc;  
   
// timezone name
   string name;

   static void create(int offset,string _name) 
   { 
      offset_to_utc=offset; 
      name=_name;
   }

   // seconds to UTC, counting DST

   array(int) tz_ux(int unixtime)
   {
      return ({offset_to_utc,name}); 
   }

   array(int) tz_jd(int julian_day)
   {
      return ({offset_to_utc,name}); 
   }

   string _sprintf(int t) { return (t=='O')?"Timezone("+name+")":0; }

   int raw_utc_offset() { return offset_to_utc; }
}

Timezone timezone;

class Language
{
   constant is_language=1;
   
   string month_name_from_number(int n);
   string month_shortname_from_number(int n);
   int month_number_from_name(string name);

   string week_day_name_from_number(int n);
   string week_day_shortname_from_number(int n);
   int week_day_number_from_name(string name);

   string gregoiran_week_day_name_from_number(int n);
   string gregorian_week_day_shortname_from_number(int n);
   int gregorian_week_day_number_from_name(string name);

   string week_name_from_number(int n);
   int week_number_from_name(string s);
   string year_name_from_number(int y);
}

Language language;

mapping(string:string) abbr2zone=
([
   "AMT":"America/Manaus",
   "AST":"America/Curacao",
   "CDT":"America/Costa_Rica",
   "CST":"America/El_Salvador",
   "EST":"America/Panama",
   "GST":"Asia/Dubai",
   "IST":"Asia/Jerusalem",
   "WST":"Australia/Perth",

// written out names -> standard abbreviation
   "Pacific Daylight Time":"PDT",
   "Pacific Standard Time":"PST",
   "Central Daylight Time":"CDT",
   "Central Standard Time":"CST",
   "Eastern Daylight Time":"EDT",
   "Eastern Standard Time":"EST",

// local languages

   "MEZ":"CET",
   "MESZ":"CEST",
]);

this_program set_timezone(string|Timezone t)
{
   this_program r=clone();
   if (stringp(t))
   {
      t=master()->resolv("Calendar")["Timezone"][t];
      if (!t) error("no timezone %O\n",t);
   }

   if (!t->is_timezone)
      error("Not a timezone: %O\n",t);
   r->timezone=t;
   return r;
}

this_program set_language(string|Language lang)
{
   this_program r=clone();
   if (stringp(lang))
   {
      lang=master()->resolv("Calendar")["Language"][lang];
      if (!lang) lang=master()->resolv("Calendar")["Language"]["ISO"];
   }
   if (!lang->is_language)
      error("Not a language: %O\n",lang);
   r->language=lang;
   return r;
}

//! Sets the guess-mapping for timezones. 
//! Default is the mapping
//!
//! @pre{
//! 	Abbreviation Interpretation
//!	AMT          America/Manaus       [UTC-4]
//!	AST	     America/Curacao      [UTC-4]
//!	CDT	     America/Costa_Rica   [UTC-5]
//!	CST	     America/El Salvador  [UTC-6]
//!	EST	     America/Panama       [UTC-5]
//!	GST          Asia/Dubai           [UTC+4]
//!	IST          Asia/Jerusalem       [UTC+2]
//!	WST          Australia/Perth      [UTC+8]
//! @}
//!
//! @seealso
//!   @[YMD.parse()]
this_program set_abbr2zone(mapping(string:string) m)
{
   this_program r=clone();
   r->abbr2zone=m;
   return r;
}

this_program set_rule(Language|Timezone rule)
{
   this_program r=clone();
   if (rule->is_timezone) r->timezone=rule;
   if (rule->is_language) r->language=rule;
   return r;
}

this_program clone()
{
   this_program r=object_program(this_object())();   
   r->timezone=timezone;
   r->language=language;
   return r;
}

int(0..1) `==(this_program other)
{
   if (!objectp(other)) return 0;
   return 
      other->timezone==timezone &&
      other->language==language;
}
