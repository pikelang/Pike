//! module Calendar
//! submodule Timezone
//!
//! 	This module contains all the predefined timezones. 	
//!	Index it with whatever timezone you want to use.
//!
//!	Example:
//!	<tt>Calendar.Calendar my_cal=
//!	    Calendar.ISO->set_timezone(Calendar.Timezone["Europe/Stockholm"]);
//!	</tt>
//!
//!	A simpler way of selecting timezones might be 
//!	to just give the string to
//!	<ref to=Ruleset.set_timezone>set_timezone</ref>;
//!	<ref to=Ruleset.set_timezone>it</ref> indexes by itself:
//!
//!	<tt>Calendar.Calendar my_cal=
//!	    Calendar.ISO->set_timezone("Europe/Stockholm");
//!	</tt>
//!
//! note
//!	Do not confuse this module with <ref>Ruleset.Timezone</ref>,
//!	which is the base class of a timezone object.
//!
//!	<tt>"CET"</tt> and some other standard abbreviations work too,
//!	but not all of them (due to more then one country using them).
//!
//!	Do not call <ref to=Calendar.Time.set_timezone>set_timezone</ref>
//!	too often, but remember the result if possible. It might take
//!	some time to initialize a timezone object.
//!
//!	There are about 504 timezones with 127 different daylight
//!	saving rules. Most of them historic.
//!
//!	The timezone information comes from 
//!	<a href=ftp://elsie.nci.nih.gov/pub/>ftp://elsie.nci.nih.gov/pub/</a>
//!	and are not made up from scratch. Timezone bugs may be reported
//!	to the timezone mailing list, 
//!	<a href=mailto:tz@elsie.nci.nih.gov>tz@elsie.nci.nih.gov</a>,
//!	preferable with a <tt>cc</tt> to 
//!	<a href=mailto:mirar@mirar.org>mirar@mirar.org</a>. /Mirar
//!
//! see also: TZnames, Ruleset.Timezone

//! constant Ruleset.Timezone locale
//!	This contains the local timezone, found from
//!	various parts of the system, if possible.

//! constant Ruleset.Timezone localtime
//!	This is a special timezone, that uses <ref>localtime</ref>()
//!	and <ref>tzname</ref>
//!	to find out what current offset and timezone string to use.
//!	
//!	<ref>locale</ref> uses this if there is no other
//!	way of finding a better timezone to use.
//!
//!	This timezone is limited by <ref>localtime</ref> and 
//!	libc to the range of <tt>time_t</tt>, 
//!	which is a MAXINT on most systems - 13 Dec 1901 20:45:52
//!	to 19 Jan 2038 3:14:07, UTC.

//! module Calendar
//! submodule TZnames
//!	This module is a mapping of the names of 
//!	all the geographical (political) 
//!	based timezones. It looks mainly like
//!	<pre>
//!	(["Europe":({"Stockholm","Paris",...}),
//!       "America":({"Chicago","Panama",...}),
//!	  ...
//!     ])
//!     </pre>
//!
//!	It is mainly there for easy and reliable ways
//!	of making user interfaces to select timezone.
//!
//!	The Posix and standard timezones (like CET, PST8PDT, etc)
//!	are not listed.


import ".";

// ----------------------------------------------------------------

static class TZHistory
{
   constant is_timezone=1;
   constant is_dst_timezone=1;

// figure out what timezone to use
   Ruleset.Timezone whatrule(int ux);

   string name=sprintf("%O",object_program(this_object()));

   array(int) tz_ux(int ux)
   {
//        werror("tz_ux %O\n",ux);
//        object z=whatrule(ux);
//        werror("%O %O\n",z->offset_to_utc,z->name);
//        return z->tz_ux(ux);
      return whatrule(ux)->tz_ux(ux);
   }

   array(int) tz_jd(int jd)
   {
//        werror("tz_jd %O\n",jd);
//        object z=whatrule((jd-2440588)*86400-86400/2);
//        werror("%O %O\n",z->offset_to_utc,z->name);
//        return z->tz_jd(jd);
      return whatrule((jd-2440588)*86400-86400/2)->tz_jd(jd);
   }

   string _sprintf(int t) { return (t=='O')?"Timezone("+name+")":0; }
   
   int raw_utc_offset();
}

#include "TZs.h";

// ----------------------------------------------------------------
// from the system

Ruleset.Timezone locale=_locale();

static function _locale()
{
   Ruleset.Timezone tz;

// try to get the real local time settings

#if 1
   string s;

   if ( (s=getenv("TZ")) )
   {
      tz=`[](s);
      if (tz) return tz;
   }

// Linux RedHat
   if ( (s=Stdio.read_bytes("/etc/sysconfig/clock")) )
   {
      sscanf(s,"%*sZONE=\"%s\"",s);
      tz=`[](s);
//        werror("=>%O\n",tz);
      if (tz) return tz;
   }

#if constant(tzname)
   mapping l=predef::localtime(time()); 
   array(string) tzn=tzname();

   tz=::`[](tzn[0]);
   if (tz && l->timezone==tz->raw_utc_offset()) return tz;
#endif
#endif

   return localtime(); // default - use localtime
};

class localtime
{
   constant is_timezone=1;
   constant is_dst_timezone=1;

#if constant(tzname)
   static array(string) names=tzname();
#endif

   string name="local";

// is (midnight) this julian day dst?
   array tz_jd(int jd)
   {
      return tz_ux((jd-2440588)*86400);
   }

// is this unixtime (utc) dst?
   array tz_ux(int ux)
   {
      if (ux<-0x80000000 || ux>0x7fffffff)
	 error("Time is out of range for Timezone.localtime()\n");

      int z0=ux%86400;
      mapping ll=predef::localtime(ux);
      int zl=ll->hour*3600+ll->min*60+ll->sec;
      int tz=z0-zl;
      if (tz>86400/2) tz-=86400;
      else if (tz<-86400/2) tz+=86400;
#if constant(tzname)
      return ({tz,names[ll->isdst]});
#else
      return ({tz,"local"});
#endif
   }

   string _sprintf(int t) { return (t=='O')?"Timezone.localtime()":0; }

   int raw_utc_offset(); // N/A but needed for interface
}

// ----------------------------------------------------------------
// magic timezones

static private Ruleset.Timezone _make_new_timezone(string tz,float plusminus)
{
   object(Ruleset.Timezone) z=`[](tz);
   if (!z) return ([])[0];
   if (plusminus>14.0 || plusminus<-14.0)
      error("difference out of range -14..14 h\n");
   if (plusminus==0.0)
      return z;
   return 
      object_program(z)(z->offset_to_utc-((int)(3600*plusminus)),
			sprintf("%s%+g",z->name||"",plusminus));
}

static private constant _military_tz=
([ "Y":"UTC-12", "X":"UTC-11", "W":"UTC-10", "V":"UTC-9", "U":"UTC-8", 
   "T":"UTC-7", "S":"UTC-6", "R":"UTC-5", "Q":"UTC-4", "P":"UTC-3", 
   "O":"UTC-2", "N":"UTC-1", "Z":"UTC", "A":"UTC+1", "B":"UTC+2", 
   "C":"UTC+3", "D":"UTC+4", "E":"UTC+5", "F":"UTC+6", "G":"UTC+7", 
   "H":"UTC+8", "I":"UTC+9", "K":"UTC+10", "L":"UTC+11", "M":"UTC+12",
   "J":"locale" ]);

static private Ruleset.Timezone _magic_timezone(string tz)
{
   float d;
   string z;
   if (sscanf(tz,"%s+%f",z,d)==2)
      return _make_new_timezone(z,d);
   if (sscanf(tz,"%s-%f",z,d)==2)
      return _make_new_timezone(z,-d);
   if ((z=_military_tz[tz])) return `[](z);
   return ::`[](replace(tz,"-/+"/1,"__p"/1));
}

Ruleset.Timezone `[](string tz)
{
   mixed p=::`[](tz);
   if (!p) p=_magic_timezone(tz);
   if (programp(p) || functionp(p)) return p();
   return p;
}
