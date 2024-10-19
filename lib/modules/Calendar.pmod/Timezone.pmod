/* -*- mode: Pike; c-basic-offset: 3; -*- */

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
//!	<a href=mailto:mirar+pike@mirar.org>mirar+pike@mirar.org</a>. /Mirar
//!
//! see also: TZnames

//! constant Rule.Timezone locale
//!	This contains the local timezone, found from
//!	various parts of the system, if possible.

//! constant Rule.Timezone localtime
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

#charset iso-8859-1
#pike __REAL_VERSION__

// ----------------------------------------------------------------
// protected

Calendar.Rule.Timezone UTC=Calendar.Rule.Timezone(0,"UTC");

// ----------------------------------------------------------------
// from the system

Calendar.Rule.Timezone locale=0;

protected Calendar.Rule.Timezone _locale()
{
   Calendar.Rule.Timezone tz;

// try to get the real local time settings

#if 1
   string s;

   if ( (s=getenv("TZ")) )
   {
      tz=`[](s);
      if (tz) return tz;
   }

   foreach(({ "/etc/timezone",		// Linux
	      "/etc/TIMEZONE",		// Solaris
	      "/etc/sysconfig/clock",	// Linux Gentoo
	      "/etc/conf.d/clock",	// Linux RedHat
	      "/etc/localtime",		// Linux & BSDs (binary)
	   }), string fname) {
     // Mapping from file name to variable name.
     string var_name =
       ([ "/etc/timezone":"",
	  "/etc/TIMEZONE":"TZ",
	  "/etc/sysconfig/clock":"ZONE",
	  "/etc/conf.d/clock":"TIMEZONE",
	  "/etc/localtime":0,
       ])[fname];
     catch {
#if constant(readlink)
       if (!var_name && Stdio.is_link(fname)) {
	 s = readlink(fname);
	 if (s) {
	   array(string) a = s/"/";
	   // Modern zones usually have two levels.
	   // (Eg Europe/Paris)
           tz = this[a[<1..]*"/"] ||
	     // NB: There are some zones that have three levels.
	     //     (Eg America/Indiana/Indianapolis)
             this[a[<2..]*"/"] ||
	     // Old-style zones have a single level.
             this[a[-1]];
	   if (tz) return tz;
	 }
       }
#endif
       if (Stdio.is_file(fname) && (s = Stdio.read_bytes(fname))) {
	 if (!var_name) {
	   if (tz = tz_from_tzfile(s)) return tz;
	 } else {
	   foreach(s/"\n", string line) {
	     line = (line/"#")[0];	// Strip comments.
	     if (sscanf(line, "%*s" + var_name + "=%s", s) == 2) {
	       sscanf(s, "\"%s\"", s);	// Strip quotes (if any).
	       if (tz = `[](s))
	       {
		 // werror("=>%O\n",tz);
		 return tz;
	       }
	     } else if ((var_name == "") && sizeof(line)) {
	       if (tz = `[](line))
	       {
		 // werror("=>%O\n",tz);
		 return tz;
	       }
	     }
	   }
	 }
       }
     };
   }

#ifdef __NT__
   // FIXME: Consider getting timezone info from the registry.
   //      HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet
   //        \\Control\\TimeZoneInformation:
   //	ActiveTimeBias	REG_DWORD	0xffffffc4
   //	Bias		REG_DWORD	0xffffffc4
   //	DaylightBias	REG_DWORD	0xffffffc4
   //	DaylightName	REG_SZ		"W. Europe Daylight Time"
   //	DaylightStart	REG_BINARY	00 00 03 00 05 00 02 00
   //					00 00 00 00 00 00 00 00
   //	StandardBias	REG_DWORD	0x00000000
   //	StandardName	REG_SZ		"W. Europe Standard Time"
   //	StandardStart	REG_BINARY	00 00 0a 00 05 00 03 00
   //					00 00 00 00 00 00 00 00
#endif /* __NT__ */

#if constant(tzname)
   mapping l=predef::localtime(time());
   array(string) tzn=tzname();

   tz=::`[](tzn[0]);
   if (tz && l->timezone==tz->raw_utc_offset()) return tz;
#endif
#endif

   // run an expert system try on the localtime() rules,
   // default to localtime()
   return expert(localtime());
};

object(Calendar.Rule.Timezone)|zero tz_from_tzfile(string tzfile)
{
  /* TZfile format (network byte-order):
   *
   * time_t is int32
   *
   * off   type            a[x] name            Typical value
   * ---------------------------------------------------------------------------
   * 0x00  char[4]	     0	magic		"TZif"
   * 0x04  char[1]	     1	version		"\0" or "2"
   * 0x05  char[15]	     1	reserved	"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
   * 0x14  int32	     2	isgmtcnt
   * 0x18  int32	     3	isstdcnt
   * 0x1c  int32	     4	leapcnt
   * 0x20  int32	     5	timecnt
   * 0x24  int32	     6	typecnt
   * 0x28  int32	     7	charcnt
   * 0x2c  time_t[timecnt]	transition_times
   * -     char[timecnt]	transition_time_types
   * -     ttinfo[typecnt]	transition_info
   * -     char[charcnt]	abbreviations
   * -     leapinfo[leapcnt]	leapseconds
   * -     char[isstdcnt]	utc_or_local_time
   * -     char[isgmtcnt]	utc_or_local_time
   *
   * struct ttinfo {
   *   time_t gmtoff
   *   char isdst
   *   char abbrind
   * }
   * struct leapinfo {
   *   time_t	leap_time
   *   int32	num_leapseconds
   * }
   *
   * In version 2 the above is followed by a second header
   * where time_t is int64.
   *
   * The second header is followed by a "\n"-enclosed string
   * describing the active rule after the last transition.
   * (Eg "CET-1CEST,M3.5.0,M10.5.0/3")
   *
   */
   array header = array_sscanf(tzfile, "%4s%16s%4c%4c%4c%4c%4c%4c");
   if( sizeof(header)<8 ) return 0;
   array zoneabbr = tzfile[44+header[5]*4+header[5]+header[6]*6..44+header[5]*4+header[5]+header[6]*6+header[7]-1]/"\0";
   if(!expert_tzn)
      expert_tzn=master()->resolv("Calendar")["TZnames"];

   array validzones = `&(@values(expert_tzn->abbr2zones & mkmultiset(zoneabbr)));
   if(sizeof(validzones) == 1)
     return `[](validzones[0]);
}


// ----------------------------------------------------------------
// expert system to pick out the correct timezone

protected Calendar.Rule.Timezone timezone_expert_rec(Calendar.Rule.Timezone try,
						  mapping|array|string tree,
						  object cal)
{
   int t=tree->test,uo;
   if (t<0)
   {
      if (catch { uo=cal->Second(t)->set_timezone(try)
		     ->utc_offset(); })
	 return timezone_select(try,timezone_collect(tree),cal);
   }
   else
      uo=cal->Second(t)->set_timezone(try)->utc_offset();

   if (!(tree=tree[uo]))
      return try;

   if (mappingp(tree))
      return timezone_expert_rec(try,tree,cal);

   if (arrayp(tree))
      return timezone_select(try,tree,cal);

// stringp
   return `[](tree);
}

protected Calendar.Rule.Timezone timezone_select(Calendar.Rule.Timezone try,
					      array tree,
					      object cal)
{
#if constant(tzname)
   array res=({});
   multiset names=mkmultiset(tzname());
   function f=cal->Second(970416317)->set_timezone;
   foreach (tree,string tzn)
      if (names[f(tzn)->tzname()]) res+=({tzn});
   if (!sizeof(res)) return try;
   tree=res;
#endif
// pick one
   return `[](tree[0]);
}

protected array timezone_collect(string|mapping|array tree)
{
   if (arrayp(tree)) return tree;
   else if (stringp(tree)) return ({tree});
   else return `+(@map(values(tree-({"test"})),timezone_collect));
}

protected object expert_cal, expert_tzn;

Calendar.Rule.Timezone expert(Calendar.Rule.Timezone try)
{
   if(!expert_cal)
     expert_cal=master()->resolv("Calendar")["ISO_UTC"];
   if(!expert_tzn)
     expert_tzn=master()->resolv("Calendar")["TZnames"];
   return timezone_expert_rec(try, expert_tzn->timezone_expert_tree,
			      expert_cal);
}

// ----------------------------------------------------------------

class localtime
{
   @Pike.Annotations.Implements(Calendar.Rule.Timezone);

   constant is_timezone=1;
   constant is_dst_timezone=1;

#if constant(tzname)
   protected array(string) names=tzname();
#endif

   string name="local";
   string `zoneid() { return name; }

// is (midnight) this julian day dst?
   array(int) tz_jd(int jd)
   {
      return tz_ux((jd-2440588)*86400);
   }

   // Workaround for predef::localtime() on WIN32 and others
   // throwing errors for times before 1970. This interferes
   // with the timezone expert system.
   protected mapping(string:int) paranoia_localtime(int ux)
   {
      if (ux<-0x80000000 || ux>0x7fffffff)
	 error("Time is out of range for Timezone.localtime()\n");
      mixed err = catch { return predef::localtime(ux); };
      if ((ux < 0) && (ux > -86400*2)) {
	// Try post-adjustment...
	// The code below actually handles up to 30 days, but...
	mapping(string:int) res = predef::localtime(0);
	if ((res->sec += ux) < 0) {
	  int delta = res->sec/60;
	  res->sec -= delta*60;
	  if ((res->min += delta) < 0) {
	    delta = res->min/60;
	    res->min -= delta*60;
	    if ((res->hour += delta) < 0) {
	      delta = res->hour/24;
	      res->hour -= delta*24;
	      if ((res->mday += delta) < 1) {
		// FIXME: Assertions?
		res->yday = 364 + res->mday;
		res->wday = (3 + res->mday) % 7;
		res->mday += 31;
		res->mon = 11;
		res->year -= 1;
	      }
	    }
	  }
	}
	return res;
      }
      throw(err);
   }

// is this unixtime (utc) dst?
   array(int) tz_ux(int ux)
   {
      if (ux<-0x80000000 || ux>0x7fffffff)
	 error("Time is out of range for Timezone.localtime()\n");

      int z0=ux%86400;
      mapping ll = paranoia_localtime(ux);
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

   protected string _sprintf(int t)
   {
      return t=='O' && "Timezone.localtime()";
   }

   int raw_utc_offset(); // N/A but needed for interface
}

// ----------------------------------------------------------------
// magic timezones

class Timezone_Encapsule
{
   Calendar.Rule.Timezone what;

   constant is_timezone=1;
   constant is_dst_timezone=1; // ask me

   protected string extra_name;
   protected int extra_offset;
   string name;

   protected void create(Calendar.Rule.Timezone enc,string name,int off)
   {
      what=enc;
      extra_name=name;
      extra_offset=off;
      name=enc->name+extra_name;
   }

   array(int) tz_ux(int unixtime)
   {
      array z=what->tz_ux(unixtime);
      return ({z[0]+extra_offset,z[1]+extra_name});
   }

   array(int) tz_jd(int julian_day)
   {
      array z=what->tz_jd(julian_day);
      return ({z[0]+extra_offset,z[1]+extra_name});
   }

   protected string _sprintf(int t)
   {
      return t=='O' && sprintf("%O%s",what,extra_name || "");
   }

   int raw_utc_offset() { return what->raw_utc_offset()+extra_offset; }
}

protected private Calendar.Rule.Timezone _make_new_timezone_i(string tz,
							   int plusminus)
{
   Calendar.Rule.Timezone z=`[](tz);
   if (!z) return UNDEFINED;
   return make_new_timezone(z,plusminus);
}

// internal, don't use this outside calendar module
Calendar.Rule.Timezone make_new_timezone(Calendar.Rule.Timezone z,int plusminus)
{
   if (plusminus>14*3600 || plusminus<-14*3600)
      error("difference out of range -14..14 h\n");
   if (plusminus==0)
      return z;
   string s;
   if (plusminus%60)
      s=sprintf("%+d:%02d:%02d",plusminus/3600,plusminus/60%60,plusminus%60);
   else if (plusminus/60%60)
      s=sprintf("%+d:%02d",plusminus/3600,plusminus/60%60);
   else
      s=sprintf("%+d",plusminus/3600);

   return Timezone_Encapsule(z,s,-plusminus);
}

protected private constant _military_tz=
([ "Y":"UTC-12", "X":"UTC-11", "W":"UTC-10", "V":"UTC-9", "U":"UTC-8",
   "T":"UTC-7", "S":"UTC-6", "R":"UTC-5", "Q":"UTC-4", "P":"UTC-3",
   "O":"UTC-2", "N":"UTC-1", "Z":"UTC", "A":"UTC+1", "B":"UTC+2",
   "C":"UTC+3", "D":"UTC+4", "E":"UTC+5", "F":"UTC+6", "G":"UTC+7",
   "H":"UTC+8", "I":"UTC+9", "K":"UTC+10", "L":"UTC+11", "M":"UTC+12",
   "J":"locale" ]);

protected object runtime_timezone_compiler=0;

// internal, don't use this outside calendar module
int decode_timeskew(string w)
{
   float f;
   int a,b,c;
   string s="";
   int neg=1;

   if (sscanf(w,"-%s",w)) neg=-1;

   if (sscanf(w,"%d:%d:%d",a,b,c)==3)
      return neg*(a*3600+b*60+c);
   else if (sscanf(w,"%d:%d",a,b)==2)
      return neg*(a*3600+b*60);
   sscanf(w,"%d%s",a,s);
   if (s!="") { sscanf(w,"%f",f); if (f!=(float)a) return neg*(int)(f*3600); }
   return neg*a*3600; // ignore litter
}

protected private Calendar.Rule.Timezone _magic_timezone(string tz)
{
   string z,w;

   if (!runtime_timezone_compiler)
      runtime_timezone_compiler=Runtime_timezone_compiler();

//     int t=time(1);
//     float t1=time(t);
//     runtime_timezone_compiler->find_rule("EU");
   object p=runtime_timezone_compiler->find_zone(tz);
//     float t2=time(t);
//     werror("%O\n",t2-t1);
   if (p) return p;

   if (sscanf(tz,"%s+%s",z,w)==2 && z!="")
      return _make_new_timezone_i(z,decode_timeskew(w));
   if (sscanf(tz,"%s-%s",z,w)==2 && z!="" && z!="+")
      return _make_new_timezone_i(z,-decode_timeskew(w));
   if ((z=_military_tz[tz])) return `[](z);

   if (sscanf(tz,"%[-+]%[-+0-9]",string a,string b)==2)
      if ((<"-","+">)[a])
      {
	 switch (sizeof(b))
	 {
	    case 2: return _magic_timezone("UTC"+a+b[..1]);
	    case 4: return _magic_timezone("UTC"+a+b[..1]+":"+b[2..]);
	    case 6:
	       return _magic_timezone("UTC"+a+b[..1]+":"+b[2..3]+":"+b[4..]);
	 }
      }
      else if (a=="+-") return _magic_timezone("-0"+b);

   return ::`[](replace(tz,"-/+"/1,"__p"/1));
}

protected Calendar.Rule.Timezone `[](string tz)
{
  mixed p=::`[](tz);
  if (!p && tz=="locale") return locale=_locale();

  if ((<"make_new_timezone","decode_timeskew">)[tz]) return p;

  if (!p) p=_magic_timezone(tz);
  if (programp(p) || functionp(p)) return p();
  return p;
}

// ================================================================
// this is to runtime-compile timezones
// it's not very nice; based on the one-time-compilation
// utility I wrote first - but that method was too slow :/
// ================================================================

class Runtime_timezone_compiler
{
   object cal=master()->resolv("Calendar")["ISO_UTC"];
   function Year=cal->Year;
   object nleapy=Year(1999);

   int is_leap_year(int y)
   { return (!(((y)%4) || (!((y)%100) && ((y)%400)))); }

#define FIXED(D)   (yjd+((D)-1))
#define FIX_L(D)   (yjd+leap+((D)-1))
#define LDAY(D,W)  (yjd+((D)-1)-( (yjd+((D)+(8-W)-1)) % 7))
#define LDAYL(D,W) (yjd+((D)-1)+leap-( (yjd+leap+((D)+(8-W)-1)) % 7))

#define complain error

#define FIXID(s) replace(s,"-+/"/1,"_minus_,_plus_,_slash_"/",")
#define UNFIXID(s) replace(s,"_minus_,_plus_,_slash_"/",","-+/"/1)

   int parse_offset(string t)
   {
      int h,m,s;
      string res;

      if (t=="0" || t=="-") return 0;

      res="";
      if (sscanf(t,"-%d:%d:%d%s",h,m,s,res)&&res=="")
	 return -(h*3600+m*60+s);
      res="";
      if (sscanf(t,"-%d:%d%s",h,m,res)&&res=="")
	 return -(h*3600+m*60);
      res="";
      if (sscanf(t,"%d:%d:%d%s",h,m,s,res)&&res=="")
	 return h*3600+m*60+s;
      res="";
      if (sscanf(t,"-%d:%d%s",h,m,res)&&res=="")
	 return h*3600+m*60;

      complain("failed to parse offset %O\n",t);
   }

   array|zero parse_tod(string t)
   {
      int h,m,s;
      string res;

      if (t=="0") return 0;

      if (sscanf(t,"%d:%d:%d%s",h,m,s,res)==4)
	 return ({h*3600+m*60+s,res});
      res="";
      if (sscanf(t,"%d:%d%s",h,m,res)==3)
	 return ({h*3600+m*60,res});
      if (sscanf(t,"%d%s",h,res)==2)
	 return ({h*3600,res});

      complain("failed to parse time of day %O\n",t);
   }

   class Shift
   {
      string dayrule;
      int time;
      string timetype;
      int offset;
      string s;
      string comment;

      protected void create(array a)
      {
	 switch (sizeof(a))
	 {
	    case 5:
	       dayrule=think_day(a[0],a[1]);
	       comment=a[0]+" "+a[1];
	       // NB: The Morocco rule for 2011-07-31 has 0 as AT,
	       //     while all others have 0:00.
	       [time,timetype] = parse_tod(a[2]) || ({ 0, "" });
	       switch (timetype)
	       {
		  case "": timetype="w"; break;
		  case "s": case "u": case "w": break;
		  default: complain("unknown time of day type %O\n",timetype);
	       }
	       offset=parse_offset(a[3]);
	       s=(a[4]=="-")?"":a[4];
	       break;
	    case 6:
	       [dayrule,comment,time,timetype,offset,s]=a;
	       break;
	    default:
	       error("illegal size of a\n");
	 }
      }

      protected string _sprintf(int t)
      {
	 return t=='O' &&
	    sprintf("Shift(%s,%d%s,%+d,%O)",
		    dayrule || "<unset>", time,
		    timetype || "<unset>", offset, s);
      }

      protected int `==(Shift other)
      {
	 return ( dayrule==other->dayrule &&
		  time==other->time &&
		  timetype==other->timetype &&
		  offset==other->offset &&
		  s==other->s );
      }
      int __equal (Shift other) {return `== (other);}

      constant wday=(["Mon":1,"Tue":2,"Wed":3,"Thu":4,"Fri":5,"Sat":6,"Sun":7]);
      constant vmonth=(<"Jan","Feb","Mar","Apr","May","Jun",
			"Jul","Aug","Sep","Nov","Dec">);

      string think_day(string mon,string rule)
      {
	 int d;
	 string ds;

	 if (mon=="") return "0";
	 if (rule==(string)(d=(int)rule))
	 {
	    if (mon=="Feb") return "FIXED("+(31+d)+")";
	    if (mon=="Jan") return "FIXED("+(d)+")";
	    return "FIX_L("+nleapy->month(mon)->day(d)->year_day()+")";
	 }
	 else if (sscanf(rule,"last%s",ds))
	 {
	    int wd=wday[ds];
	    if (!wd) complain("unknown weekday %O (last%s)\n",ds,ds);

	    if (mon=="Jan")
	       return "LDAY ("+31+","+wd+")";

	    return "LDAYL("+nleapy->month(mon)->day(-1)->year_day()+
	       ","+wd+")";
	 }
	 else if (sscanf(rule,"%s>=%d",ds,d))
	 {
	    int wd=wday[ds];
	    if (!wd) complain("unknown weekday %O (last%s)\n",ds,ds);

	    if (d>24 && mon=="Feb")
	       complain("can't handle Feb %d in a >= rule\n",d);

	    if (mon=="Jan")
	       return "LDAY ("+(nleapy->month(mon)->day(d)->year_day()+6)+
		  ","+wd+")";

	    return "LDAYL("+(nleapy->month(mon)->day(d)->year_day()+6)+
	       ","+wd+")";
	 }
	 else if (sscanf(rule,"%s<=%d",ds,d))
	 {
	    int wd=wday[ds];
	    if (!wd) complain("unknown weekday %O (last%s)\n",ds,ds);

	    if (d>24 && mon=="Feb")
	       complain("can't handle Feb %d in a <= rule\n",d);

	    if (mon=="Jan" || mon=="Feb")
	       return "LDAY ("+(nleapy->month(mon)->day(d)->year_day())+
		  ","+wd+")";

	    return "LDAYL("+(nleapy->month(mon)->day(d)->year_day())+
	       ","+wd+")";
	 }
	 else
	    complain("unknown rule method %O\n",rule);
      }

      protected Shift|array ``+(array|Shift s)
      {
	 if (!s) return this;
	 if (!arrayp(s)) s=({s});
	 return ([array]s) + ({this});
      }

      int ldayl_is_fix_l(int d1,int wd,int d2,int yn1,int yn2)
      {
	 return 0;
   //        object y1=Year(yn1);
   //        object y2=Year(yn2);
   //        int yjd,leap;

   //        yjd=y1->julian_day();
   //        leap=y1->leap_year();
   //        int d1=LDAYL(d1,wd);

   //        yjd=y2->julian_day();
   //        leap=y2->leap_year();
   //        int d2=FIX_L(d2);

   //        return d1==d2;
      }

      object(Shift)|zero try_promote(Shift t,int y0,int y1)
      {
   // this is year y0
   // t is year y1
	 if (t==this) return t; // same!
	 if (t->time!=time ||
	     t->timetype!=timetype ||
	     t->offset!=offset ||
	     t->s!=s) return 0; // no chance

	 int a,b,c;
	 if (sscanf(dayrule,"LDAYL(%d,%d)",a,b)==2 &&
	     sscanf(t->dayrule,"FIX_L(%d)",c)==1)
	    if (ldayl_is_fix_l(a,b,c,y0,y1))
	       return this; // ldayl
	    else
	       return 0; // no
	 if (sscanf(t->dayrule,"LDAYL(%d,%d)",a,b)==2 &&
	     sscanf(dayrule,"FIX_L(%d)",c)==1)
	    if (ldayl_is_fix_l(a,b,c,y1,y0))
	       return t; // ldayl
	    else
	       return 0; // no

	 return 0;
      }

      string dump(int lastoffset,multiset ys)
      {
	 string t;
	 array z=(array)ys;
	 int l=is_leap_year(z[0]);

	 if (dayrule=="0" && time==0 && offset==0 && s=="")
	    return sprintf("%-40s,  // %s", "  ZEROSHIFT",comment);

	 foreach (z[1..],int y) if (is_leap_year(y)!=l) { l=2; break; }
	 switch (timetype)
	 {
	    case "s": t=sprintf("UO%+d",time); break;
	    case "u": t=""+time; break;
	    case "w": t=sprintf("UO%+d",(time-lastoffset)); break;
	    default: error("illegal state\n");
	 }
	 string r=dayrule;
	 if (l!=2)
	 {
	    int d,w;
	    if (sscanf(r,"FIX_L(%d)",d)) r=sprintf("FIXED(%d)",d+l);
	    else if (sscanf(r,"LDAYL(%d,%d)",d,w)==2)
	       r=sprintf("LDAY (%d,%d)",d+l,w);
	 }
	 return sprintf("({%-12s,%-10s,%-5d,%-6O}),  %s",
			r,t,offset,s,comment!=""?"// "+comment:"");
      }
   }

  object compile_handler = class {
    mapping(string:mixed) get_default_module() {
      return constants;
    }

    mapping constants = all_constants() +
    (["TZrules":Dummymodule(find_rule),
      "TZRules":TZRules,
      "TZHistory":TZHistory,
      "Rule":Calendar.Rule,
      "ZEROSHIFT":({0,0,0,""})
    ]);

  }();

   class Rule(string id)
   {
      array(string) lines = ({});

      void add_line(string line)
      {
	 lines += ({ line });
      }
   };

   class RuleCompiler(string id, array(string) lines)
   {
      mapping rules=([]);

      int amt=0;

      void add(string line)
      {
	 array a= array_sscanf(line, replace("%s %s %s %s %s %s %s %[^\t ]",
					     " ","%*[ \t]"));

	 if (sizeof(a)<8) complain("illegal rule line format\n");

	 if (!(int)a[0] && a[0]!="min")
	    complain("unknown year %O\n",a[0]);

   // ---

#define INF_YEAR 2100
#define NUL_YEAR 1850

	 int y1=(int)a[0] || NUL_YEAR;
	 int y2;
	 if (a[1]=="max") y2=INF_YEAR;
	 else if (a[1]=="only") y2=y1;
	 else if (!(y2=(int)a[1]))
	    complain("unknown year %O\n",a[1]);
	 else if (y2>=INF_YEAR)
	    complain("big year\n");

	 Shift sh=Shift(a[3..]);

	 switch (a[2])
	 {
	    case "-": for (;y1<=y2;y1++) rules[y1]+=sh; break;
	    case "odd":
	       if (!(y1&1)) y1++;
	       for (;y1<=y2;y1+=2) rules[y1]+=sh;
	       break;
	    case "even":
	       if ((y1&1)) y1++;
	       for (;y1<=y2;y1+=2) rules[y1]+=sh;
	       break;
	    default:
	       complain("unknown year type %O\n",a[2]);
	 }
      }

      string dump()
      {
	 mapping r2=([]);
	 Shift last=Shift(({"0","?",0,"u",0,"?"}));
	 Shift first=last;
	 array res=({});

	 if (!r2[NUL_YEAR]) r2[NUL_YEAR]=({last});

	 int y=min(@indices(rules));
	 for (;y<=INF_YEAR; y++)
	    [r2[y],last]=mkperiods(rules[y],last,first);

	 res+=
	    ({TZrules_init+
	      "   inherit TZRules;\n"
	      "   array(array(string|int)) jd_year_periods(int jd)\n"
	      "   {\n"
	      "      [int y,int yjd,int leap]=gregorian_yjd(jd);\n"
	      "      switch (y)\n"
	      "      {\n"});

	 array sr=({});

	 int mn=min(@indices(rules)-({NUL_YEAR}));
	 int firstyear=99999,lastyear=0;

	 for (y=INF_YEAR;sizeof(r2);y--)
	    if (r2[y])
	    {
	       array z=r2[y];
	       multiset my=(<y>);

	       foreach (indices(r2),int y2)
		  if (join_periods(z,r2[y2],y,y2))
		     my[y2]=1;

	       foreach ((array)my,int y2) m_delete(r2,y2);

	       array tr=({});

	       int y0=min(@(array)my);
	       int y2=max(@(array)my);
	       if (y0!=NUL_YEAR && y0<firstyear) firstyear=y0;
	       if (y2!=INF_YEAR && y2>lastyear) lastyear=y2;
	       for (; y0<=y2; y0++)
		  if (my[y0])
		  {
		     int y1=y0;
		     while (my[++y1]);

		     y1--;

	       // figure first and last interesting year
		     if (y0==NUL_YEAR)
		     {
			if (y1!=NUL_YEAR && y1<firstyear) firstyear=y1;
			if (my[INF_YEAR])
			   tr+=({"         default: // .."+max(y1,mn-1),
				  " and ���..\n"});
			else
			   tr+=({"         default: // .."+max(y1,mn-1),
				  ":\n"});
		     }
		     else if (y0==y1)
			tr+=({"         case "+y0+":\n"});
		     else if (y1==INF_YEAR)
		     {
			if (y0>lastyear) lastyear=y0;

			if (!my[NUL_YEAR])
			   tr+=({"         case "+y0,"..:\n"});
			else
			   tr[0]=replace(tr[0],"���",(string)y0);
		     }
		     else
			tr+=({"         case "+y0,".."+y1+":\n"});

		     y0=y1;
		  }

	       int lastoffset=0;
	       tr+=({" "*12,"return ({"});
	       foreach (z,Shift s)
	       {
		  tr+=({s->dump(lastoffset,my),"\n"," "*21});
		  lastoffset=s->offset;
	       }
	       tr[-3]=replace(tr[-3],",  ","});");
	       sr=tr[..<1]+sr;
	    }
	 res+=sr+
	 ({"      }\n"
	   "   }\n"
	   "\n"
	   "constant firstyear="+firstyear+";\n"
	   "constant lastyear="+lastyear+";\n",
	   "array(string) rule_s=\n"});

	 multiset tzname=(<>);
	 foreach (values(rules),array(Shift)|object(Shift) s)
	    if (arrayp(s)) foreach (s,Shift s) tzname[s->s]=1;
	    else  tzname[s->s]=1;

	 res+=({"({",map((array)tzname,
			 lambda(string s) { return sprintf("%O",s); })*",",
		"});\n",
		sprintf("string name=%O;\n",id)});

	 return res*"";
      }

      int join_periods(array s,array t,int y0,int y1)
      {
	 if (equal(s,t))
	    return 1;
	 if (sizeof(s)!=sizeof(t)) return 0;
	 if (s[0]!=t[0]) return 0;
   // try promote
	 array q=s[..0];
	 int i;
	 for (i=1; i<sizeof(s); i++)
	 {
	    Shift u=s[i]->try_promote(t[i],y0,y1);
	    if (!u) return 0;
	    q+=({u});
	 }
	 for (i=1; i<sizeof(s); i++)
	    s[i]=q[i]; // destructive
	 return 1;
      }

      array(array(Shift)|Shift) mkperiods(array|Shift s,Shift last,Shift first)
      {
	 if (!s) s=({});
	 if (!arrayp(s)) s=({s});

	 sort(map(s,lambda(Shift s)
		    {
		       return array_sscanf(s->dayrule,"%*[^(](%d")[0];
		    }),s);

	 if (first->s=="?")
	    foreach (s,Shift d) if (!d->offset) first->s=d->s;

	 s=({last,@s});

	 last=Shift( ({"0","",0,"u",
		       s[-1]->offset,s[-1]->s}) );

	 return ({s, last});
      }


     program compile()
     {
#ifdef RTTZC_TIMING
       float t1=time(t);
#endif
#ifdef RTTZC_DEBUG
       werror("Compiling rule %s\n", id);
#endif

       foreach(lines, string line) add(line);

       string c = dump();

#ifdef RTTZC_TIMING
       float td=time(t);
       werror("dump %O: %O\n", id, td-t1);
#endif

#ifdef RTTZC_DEBUG
       werror("%s\n",c);
#endif

       program p;
       mixed err=catch { p=compile_string(c, UNDEFINED, compile_handler); };
       if (err)
       {
	 int i=0;
	 werror("Failed to compile timezone rule %O.\n", id);
	 foreach (c/"\n",string line) werror("%2d: %s\n",++i,line);
	 error(err);
       }
#ifdef RTTZC_TIMING
       float t3=time(t);
       werror("compile %O: %O\n", id, t3-td);
#endif
       return rule_cache[id] = p;
     }

   }

   class Zone
   {
      string id;

      array(string) lines = ({});

      array(string) aliases = ({});

      protected void create(string _id) {
	id=_id;
	aliases = ({ id });
      }

      void add_alias(string zone_alias)
      {
	 aliases += ({ zone_alias });
      }

      void add_line(string line)
      {
	 lines += ({ line });
      }
   };

   class ZoneCompiler(string id, array(string) lines, array(string) aliases)
   {
      array rules=({});

      void add(string line)
      {
	 array a= array_sscanf(line, replace("%s %s %s %s",
					     " ","%*[ \t]"));
	 if (sizeof(a)<4)
	    complain("parse error\n");

	 a=({parse_offset(a[0]), // offset
		   a[1], // rule or added offset
		   a[2], // string
		   a[3],
		   0, 0, "tz", 0}); // until
	 a[5]=rule_shift(a);
	 a[4]=clone_rule(a);

	 rules+=({a});
      }

      string clone_rule(array a)
      {
	 int h,m,s,roff=-17;
	 if (a[1]=="-") roff=0;
	 else if (sscanf(a[1],"-%d:%d:%d",h,m,s)==3) roff=h*3600+m*60+s;
	 else if (sscanf(a[1],"%d:%d:%d",h,m,s)==3) roff=h*3600+m*60+s;
	 else if (sscanf(a[1],"-%d:%d",h,m)==2) roff=h*3600+m*60;
	 else if (sscanf(a[1],"%d:%d",h,m)==2) roff=h*3600+m*60;

	 if (roff==-17) // based on DST rule
	    return sprintf(
	       "TZrules.%s(%d,%O)",
	       FIXID(a[1]),-a[0],a[2]);
	 else // simple timezone
	    return sprintf(
	       "Rule.Timezone(%d,%O)",
	       -(roff+a[0]),a[2]);
      }

      string rule_shift(array a)
      {
	 if (a[3]=="" || a[3][0]=='#') return "forever";

	 string in=a[3];
	 sscanf(in,"until %s",in);
	 sscanf(in,"%s#",in);

   //        werror("%O\n",in);

	 int y,d=1,t=0;
	 string mn="Jan",ty="w";
	 if (sscanf(in,"%d%*[ \t]%s%*[ \t]%d%*[ \t]%[^# \t]",
		    y,mn,d,string tod)==7 &&
	     tod!="")
	    [t,ty]=parse_tod(tod);
	 else if (!(sscanf(in,"%d%*[ \t]%[A-Za-z]%*[ \t]%d",y,mn,d)==5 ||
		    (sscanf(in,"%d%*[ \t]%[A-Za-z]",y,mn)==3 && mn!="") ||
		    (mn="Jan",sscanf(in,"%d",y))))
	 {
      //  	 werror("%O\n",array_sscanf(in,"%d%*[ \t]%[A-Za-z]"));
	    complain("failed to understand UNTIL %O\n",in);
	    y=2000;
	 }

	 int utc0=Year(y)->month(mn)->day(d)->unix_time();
	 switch (ty)
	 {
	    case "u":     // utc time
	 //  	    a[3]=sprintf("[%d+%d=%d] %s\n",utc0,t,utc0+t,a[3]);
	       return (string)(utc0+t); break;
	    case "s":     // local standard time
	 //  	    a[3]=sprintf("[%d+%d-%d=%d] %s\n",utc0,t,a[0],utc0+t-a[0],a[3]);
	       return (string)(utc0+t-a[0]); break;
	    case "w": case "":       // with rule; check rule
	       int h,m,s,roff=-17;
	       if (a[1]=="-") roff=0;
	       else if (sscanf(a[1],"-%d:%d:%d",h,m,s)==3) roff=h*3600+m*60+s;
	       else if (sscanf(a[1],"%d:%d:%d",h,m,s)==3) roff=h*3600+m*60+s;
	       else if (sscanf(a[1],"-%d:%d",h,m)==2) roff=h*3600+m*60;
	       else if (sscanf(a[1],"%d:%d",h,m)==2) roff=h*3600+m*60;

	       if (roff==-17) // based on DST rule
	       {
		  program rulesp=find_rule(a[1]);
		  if (!rulesp)
		     error("ERROR: Missing rule %O (used in Zone %O)\n",a[1],id);

		  object rules=rulesp(-a[0],"-");

		  roff=rules->tz_jd(Year(y)->month(mn)->day(d)->julian_day())[0];

	    //  	       werror("Using %O:%O\n",rules,roff);

		  return (string)(utc0+t-roff);
	       }
	       return (string)(utc0+t-a[0]-roff);

	    default:
	       complain("unknown time of day modifier %O\n",ty);
	 }
      }

      string dump()
      {
	 array(string) res=({});

	 if (!sizeof(rules))
	    error("no rules for %O\n",id);

	 if (sizeof(rules)==1) // simple zone
	 {
	    res+=({"object thezone=",rules[0][4],";\n"});
	    return res*"";
	 }

	 mapping rname=([]);
	 int n=1;

	 foreach (rules,array a)
	    if (rname[a[4]]) a[6]=rname[a[4]];
	    else a[6]=rname[a[4]]="tz"+n++;

	 res+=({ "inherit TZHistory;\n"
                 "@Pike.Annotations.Implements(Calendar.Rule.Timezone);\n"
                 "protected Rule.Timezone ",
		 sort(values(rname))*",",";\n"
		 "Rule.Timezone whatrule(int ux)\n"
		 "{\n" });

	 foreach (rules,array a)
	 {
	    if (!a[5]) a[5]=rule_shift(a);

	    string s="";
	    sscanf(a[3],"%s#%*[ \t]%s",a[3],s);
	    a[3]="from "+reverse(array_sscanf(reverse(a[3]),"%*[ \t]%s")[0]);
	    a[7]=s;
	 }

	 array last=rules[-1];
	 foreach (reverse(rules)[1..],array a)
	 {
	    res+=({sprintf("   if (ux>=%s) // %s %s\n"
			   "      return %s || (%s=%s);\n",
			   a[5],a[3],last[7],last[6],last[6],last[4])});
	    last=a;
	 }
	 if (last[7]!="")
	    res+=({sprintf("   // %s\n",last[7])});
	 res+=({sprintf("   return %s || (%s=%s);\n",
			last[6],last[6],last[4])+
		"}\n"});

	 multiset tzname=(<>);
	 foreach (rules,array a)
	    if (!has_value(a[2], "%s"))
	       tzname[a[2]]=1;
	    else
	    {
	       program r=find_rule(a[1]);
	       foreach (r(0,a[2])->rule_s,string s)
		  tzname[sprintf(a[2],s)]=1;
	    }

	 res+=({"array(string) zone_s=({"+
		map((array)tzname,
		    lambda(string s) { return sprintf("%O",s); })*",",
		"});\n",
		"array(int) shifts=({"});
	 foreach (rules[..<1],array a)
	    res+=({a[5]+","});
	 res+=({"});\n",
		sprintf(
		   "protected string _sprintf(int t) { return t=='O' &&"
		   "%O; }\n"
                   "constant zoneid=%O;\n",
                   sprintf("Calendar.Timezone[%O]", id), id)});

	 return res*"";
      }

     object compile()
     {
       string zone_name = id;
       // werror("Compiling zone %O...\n", zone_name);

#ifdef RTTZC_TIMING
       float t1=time(t);
#endif
#ifdef RTTZC_DEBUG
       werror("Compiling zone %s\n", zone_name);
#endif

       rules = ({});
       foreach(lines, string line) add(line);

       string c=dump();

#ifdef RTTZC_TIMING
       float td=time(t);
       werror("dump %O: %O\n",zone_name,td-t1);
       float td=time(t);
#endif

#ifdef RTTZC_DEBUG
       werror("%s\n",c);
#endif

       program p;
       mixed err=catch { p=compile_string(c, "Calendar.Timezone." + zone_name,
                                          compile_handler); };
       if (err)
       {
	 int i=0;
	 werror("Failed to compile timezone zone %O.\n", id);
	 foreach (c/"\n",string line) werror("%2d: %s\n",++i,line);
	 throw(err);
       }
       object zo=p();
       if (zo->thezone) zo=zo->thezone;

#ifdef RTTZC_TIMING
       float t3=time(t);
       werror("compile %O: %O\n",zone_name,t3-td);
#endif
       return zo;
     }

   }

   string base_path=combine_path(__DIR__,"tzdata/");
   array files=
   ({
      "africa",
      "antarctica",
      "asia",
      "australasia",
      "backward",
      "backzone",
      "etcetera",
      "europe",
      "northamerica",
      "southamerica",
#if 0
      "pacificnew",
      "systemv",
#endif
   });

   mapping zone_cache;
   mapping rule_cache;
   string all_rules;
   protected mapping(string:Zone) zones = ([]);
   protected mapping(string:Rule) rules = ([]);

   string get_all_rules()
   {
     return
       map(files,
	   lambda(string fn)
	   {
	     return (master()->master_read_file(base_path+fn) ||
		     (error("Failed to open file %O\n",base_path+fn), "")) - "\r";
	   })*"\n";
   }

   class Dummymodule (function(string:mixed) f)
   {
      protected mixed `[](string s) { return f(s); }
   }

//  #define RTTZC_DEBUG
//  #define RTTZC_TIMING

   void parse_all_rules()
   {
      if (!all_rules) all_rules=get_all_rules();
#ifdef RTTZC_TIMING
      float t1=time(t);
#endif

      mapping(string:Zone) new_zones = ([]);
      mapping(string:Rule) new_rules = ([]);

      mapping(string:string) zone_aliases = ([]);

      Zone current_zone;
      foreach(all_rules/"\n", string line) {
	line = (line/"#")[0];
	if ((line == "") || (line == " ")) continue;

	if (has_prefix(line, "Rule")) {
	  current_zone = 0;

	  string rule_name, interval;
	  if (sscanf(line, "Rule%*[ \t]%[^ \t]%*[ \t]%s",
		     rule_name, interval) == 4) {
	    Rule r = new_rules[rule_name];
	    if (!r) {
	      r = new_rules[rule_name] = Rule(rule_name);
	    }
	    r->add_line(interval);
	  } else {
	    werror("Failed to parse directive %O.\n", line);
	  }
	} else if (has_prefix(line, "Zone")) {
	  string zone_name, zone_info;
	  if (sscanf(line, "Zone%*[ \t]%[^ \t]%*[ \t]%s",
		     zone_name, zone_info) == 4) {
	    // werror("Creating zone %O.\n", zone_name);
            if (!new_zones[zone_name]) {
              // Some zones appear in multiple places
              // eg in backzone and europe.
              new_zones[zone_name] = Zone(zone_name);
            }
            Zone z = current_zone = new_zones[zone_name];
	    z->add_line(zone_info);
	  } else {
	    werror("Failed to parse directive %O.\n", line);
	  }
	} else if (has_prefix(line, "Link")) {
	  string zone_name, zone_alias;
	  if (sscanf(line, "Link%*[ \t]%[^ \t]%*[ \t]%[^ \t]",
		     zone_name, zone_alias) == 4) {
            // werror("Deferred alias: %O ==> %O.\n", zone_alias, zone_name);
            zone_aliases[zone_alias] = zone_name;
	  } else {
	    werror("Failed to parse directive %O.\n", line);
	  }
	} else if (current_zone) {
	  string prefix, suffix;
	  if ((sscanf(line, "%*[ \t]%[-0-9]%s", prefix, suffix) == 3) &&
	      sizeof(prefix)) {
	    current_zone->add_line(prefix + suffix);
	  }
	} else {
	  // werror("Skipping line %O.\n", line);
	}
      }
#ifdef RTTZC_TIMING
      float td=time(t);
      werror("parsing: %O\n",td-t1);
#endif

      // Fixup the zone aliases.
      foreach(zone_aliases; string zone_alias; string zone_name) {
	Zone z;
        // NB: Do not zap zones that have proper definitions
        //     in eg backzone.
        if (z = new_zones[zone_name]) {
          if (!new_zones[zone_alias]) {
            z->add_alias(zone_alias);
            new_zones[zone_alias] = z;
          }
	} else {
	  werror("Zone %O is a link to a nonexistant zone %O.\n",
		 zone_alias, zone_name);
	}
      }

      zones = new_zones;
      rules = new_rules;

      rule_cache = ([]);
      zone_cache = ([]);
   }

   object find_zone(string s)
   {
#ifdef RTTZC_DEBUG
      werror("Searching for zone %O\n",s);
#endif
      if (!zone_cache) parse_all_rules();
      object ret = zone_cache[s];
      if (!undefinedp(ret)) {
#ifdef RTTZC_DEBUG
        werror("Found in cache: %O\n", ret);
#endif
        return ret || UNDEFINED;
      }
      Zone z;
      if (!(z = zones[s])) {
	// Check if it's a known alias.
	// Note: TZnames.abbr2zones is sorted with
	//       the most recent user first.
	array(string) a = .TZnames.abbr2zones[s];
        if (a && sizeof(a)) {
#ifdef RTTZC_DEBUG
          werror("Found alias: %O\n", a[0]);
#endif
	  return zone_cache[s] = find_zone(a[0]);
        }
#ifdef RTTZC_DEBUG
        werror("Not found!\n");
#endif
	return UNDEFINED;
      }
      ret = ZoneCompiler(z->id, z->lines, z->aliases)->compile();
#ifdef RTTZC_DEBUG
      werror("Compiled zone: %O\n", ret);
#endif
      foreach(z->aliases, string zone_alias) {
        if (!zone_cache[zone_alias]) {
#ifdef RTTZC_DEBUG
          werror("Updating aliases: %O\n", zone_alias);
#endif
          zone_cache[zone_alias] = ret;
          m_delete(zones, zone_alias);
        }
      }
      m_delete(zones, s);
      return ret;
   }

   program find_rule(string s)
   {
      if (!rule_cache) parse_all_rules();
      s = UNFIXID(s);
      program ret = rule_cache[s];
      if (ret) return ret;
      Rule r;
      if (!(r = rules[s])) return UNDEFINED;
      ret = rule_cache[s] = RuleCompiler(s, r->lines)->compile();
      m_delete(rules, s);
      return ret;
   }

   int main(int ac,array(string) am)
   {
      map(am[1..],find_zone);
      return 0;
   }

// ----------------------------------------------------------------
// Base class for daylight-saving and war time rules
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// Base "Timezone with rules" class
// ----------------------------------------------------------------

   class TZRules
   {
      constant is_timezone=1;
      constant is_dst_timezone=1;
      protected int offset_to_utc;
      string name;

      protected array names;


// ----------------------------------------------------------------
// all rules are based on the gregorian calendar, so
// this is the needed gregorian rule:
// ----------------------------------------------------------------

      protected array gregorian_yjd(int jd)
      {
	 int d=jd-1721426;

	 int century=(4*d+3)/146097;
	 int century_jd=(century*146097)/4;
	 int century_day=d-century_jd;
	 int century_year=(100*century_day+75)/36525;

	 int y=century*100+century_year+1;

	 return
	    ({
	       y,
	       1721426+century_year*365+century_year/4+century_jd,
	       (!(((y)%4) || (!((y)%100) && ((y)%400))))
	    });
      }

      protected string tzformat (string s)
      {
	 if (names)
	 {
	    if (s=="") return names[0]; else return names[1];
	 }
	 else
	    return sprintf(name,s);
      }

      protected void create(int offset,string name)
      {
	 offset_to_utc=offset;
	 this::name=name;
	 if (has_value(name, "/"))
	 {
	    names=name/"/";
	 }
      }

// the Rule:
// which julian day does dst start and end this year?
      protected array(array(string|int)) jd_year_periods(int jd);

// is (midnight) this julian day dst?
      array tz_jd(int jd)
      {
	 array(array(string|int)) a=jd_year_periods(jd);

	 int i=0,n=sizeof(a)-1;
	 while (i<n)
	 {
	    array b=a[i+1];
	    if (jd<b[0]) break;
	    if (jd==b[0] && -offset_to_utc+b[1]>=0) break;
	    i++;
	 }

	 return ({offset_to_utc-a[i][2],tzformat(a[i][3])});
      }

// is this unixtime (utc) dst?
      array tz_ux(int ux)
      {
	 int jd=2440588+ux/86400;
	 array(array(string|int)) a=jd_year_periods(jd);

	 int i=0,n=sizeof(a)-1;
	 while (i<n)
	 {
	    array b=a[i+1];
	    if (jd<b[0]-1) break;
	    if (jd<b[0]+1 &&
		ux<(b[0]-2440588)*86400+b[1]) break;
	    i++;
	 }

	 return ({offset_to_utc-a[i][2],tzformat(a[i][3])});
      }

      protected string _sprintf(int t)
      {
        return t=='O' && "Rule.Timezone("+name+")";
      }

      int raw_utc_offset() { return offset_to_utc; }
   }

   class TZHistory
   {
      @Pike.Annotations.Implements(Calendar.Rule.Timezone);

      constant is_timezone=1;
      constant is_dst_timezone=1;

// figure out what timezone to use
      Calendar.Rule.Timezone whatrule(int ux);

      constant zoneid = "-";
      string name = zoneid;

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

      int raw_utc_offset();
   }

// ----------------------------------------------------------------------

   string TZrules_init=
#"
// useful macros
#define FIXED(D)   (yjd+((D)-1))
#define FIX_L(D)   (yjd+leap+((D)-1))
#define LDAY(D,W)  (yjd+((D)-1)-( (yjd+((D)+(8-W)-1)) % 7))
#define LDAYL(D,W) (yjd+((D)-1)+leap-( (yjd+leap+((D)+(8-W)-1)) % 7))
#define UO offset_to_utc
";

// ----------------------------------------------------------------------

}
