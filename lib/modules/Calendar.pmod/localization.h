// This file is for the user or installer of pike
// to modify after the local setup.

// Note that you should modify this file
// in pike's source location 
// (.../lib/modules/Calendar.pmod/localization.pmod)
// and reinstall, so the byte-compiling gets correct.



// This should be the default calendar. Americans
// use the Gregorian calendar as default - the big
// difference is that the Gregorian calendar starts
// the weeks on sundays, ISO on mondays.
// Programs will probably get confused if this isn't
// a year-month-day calendar, so don't put "Stardate" here.

string default_calendar="ISO";
// string default_calendar="Gregorian";


// The timezone name is strings like "Europe/Stockholm" or "UTC".
// There are two magic timezones, "locale", which
// tries to do the best of the situation, and
// "localtime" which uses localtime for rules (slow!).
//
// Check the TZs.h or TZnames.pmod for correct timezone names.
//
// Also note that "CET" or stuff like that shouldn't be
// used unless you mean it - that summer time ruleset
// might not be what you want.

string default_timezone="locale"; 
// string default_timezone="Europe/Stockholm";



// The default language to use. This should probably
// not be modified, since more then one program will
// assume the language is english (=ISO).

string default_language="ISO"; 






