#pike __REAL_VERSION__

//! This is the container class for rules.

.Rule.Timezone timezone;
.Rule.Language language;

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

this_program set_timezone(string|.Rule.Timezone t)
{
   if (stringp(t)) {
      t=.Timezone[t];
      if (!t) error("No timezone %O\n",t);
   }

   if (!t->is_timezone)
      error("Not a timezone: %O\n",t);

   this_program r=clone();
   r->timezone=t;
   return r;
}

this_program set_language(string|.Rule.Language lang)
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

//! method Ruleset set_abbr2zone(mapping(string:string) abbr2zone)
//!	Sets the guess-mapping for timezones. 
//!	Default is the mapping
//!
//!	<pre>
//! 	Abbreviation Interpretation
//!	AMT          America/Manaus       [UTC-4]
//!	AST	     America/Curacao      [UTC-4]
//!	CDT	     America/Costa_Rica   [UTC-5]
//!	CST	     America/El Salvador  [UTC-6]
//!	EST	     America/Panama       [UTC-5]
//!	GST          Asia/Dubai           [UTC+4]
//!	IST          Asia/Jerusalem       [UTC+2]
//!	WST          Australia/Perth      [UTC+8]
//!	</pre>
//!
//! see also: YMD.parse


this_program set_abbr2zone(mapping(string:string) m)
{
   this_program r=clone();
   r->abbr2zone=m;
   return r;
}

this_program set_rule(.Rule.Language|.Rule.Timezone rule)
{
   this_program r=clone();
   if (rule->is_timezone) r->timezone=rule;
   if (rule->is_language) r->language=rule;
   return r;
}

this_program clone()
{
   this_program r=this_program();
   r->timezone=timezone;
   r->language=language;
   r->abbr2zone=abbr2zone;
   return r;
}

int(0..1) `==(this_program other)
{
   if (!objectp(other)) return 0;
   return 
      other->timezone==timezone &&
      other->language==language;
}
