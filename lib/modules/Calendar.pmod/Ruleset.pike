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

//!
this_program set_timezone(string|.Rule.Timezone t)
{
   if (stringp(t)) {
     string name = t;
      t=.Timezone[t];
      if (!t) error("No timezone %O\n",name);
   }

   if (!t->is_timezone)
      error("Not a timezone: %O\n",t);

   this_program r=clone();
   r->timezone=t;
   return r;
}

//!
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

//! Sets the guess-mapping for timezones. Default is the mapping:
//!
//! @xml{<matrix>
//! <r><c><b>Abbreviation</b></c><c><b>Interpretation</b></c>
//!   <c><b>UTC</b></c></r>
//! <r><c>AMT</c><c>America/Manaus</c><c>UTC-4</c></r>
//! <r><c>AST</c><c>America/Curacao</c><c>UTC-4</c></r>
//! <r><c>CDT</c><c>America/Costa_Rica</c><c>UTC-5</c></r>
//! <r><c>CST</c><c>America/El Salvador</c><c>UTC-6</c></r>
//! <r><c>EST</c><c>America/Panama</c><c>UTC-5</c></r>
//! <r><c>GST</c><c>Asia/Dubai</c><c>UTC+4</c></r>
//! <r><c>IST</c><c>Asia/Jerusalem</c><c>UTC+2</c></r>
//! <r><c>WST</c><c>Australia/Perth</c><c>UTC+8</c></r>
//! </matrix>@}
//!
//! @seealso
//!   @[YMD.parse]
this_program set_abbr2zone(mapping(string:string) abbr2zone)
{
   this_program r=clone();
   r->abbr2zone=abbr2zone;
   return r;
}

//!
this_program set_rule(.Rule.Language|.Rule.Timezone rule)
{
   this_program r=clone();
   if (rule->is_timezone) r->timezone=rule;
   if (rule->is_language) r->language=rule;
   return r;
}

//!
this_program clone()
{
   this_program r=this_program();
   r->timezone=timezone;
   r->language=language;
   r->abbr2zone=abbr2zone;
   return r;
}

//!
int(0..1) `==(this_program other)
{
   if (!objectp(other)) return 0;
   return 
      other->timezone==timezone &&
      other->language==language;
}
