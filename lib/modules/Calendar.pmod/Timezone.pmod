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
// static

Ruleset.Timezone UTC=Ruleset.Timezone(0,"UTC");

// ----------------------------------------------------------------
// from the system

Ruleset.Timezone locale=0;

static function(:Ruleset.Timezone) _locale()
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

object runtime_timezone_compiler=0;

static private Ruleset.Timezone _magic_timezone(string tz)
{
   float d;
   string z;

   if (!runtime_timezone_compiler)
      runtime_timezone_compiler=Runtime_timezone_compiler();

   object p=runtime_timezone_compiler->find_zone(tz);
   if (p) return p;

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
   if (!p && tz=="locale") return locale=_locale();
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

      if (t=="0") return 0;

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

   array parse_tod(string t)
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

      void create(array a)
      {
	 switch (sizeof(a))
	 {
	    case 5:
	       dayrule=think_day(a[0],a[1]);
	       comment=a[0]+" "+a[1];
	       [time,timetype]=parse_tod(a[2]);
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

      string _sprintf(int t) 
      { 
	 return (t=='O')?
	    sprintf("Shift(%s,%d%s,%+d,%O)",
		    dayrule,time,timetype,offset,s):
	    0;
      }

      int `==(Shift other)
      {
	 return ( dayrule==other->dayrule &&
		  time==other->time &&
		  timetype==other->timetype &&
		  offset==other->offset &&
		  s==other->s );
      }
      function(Shift:int) __equal=`==;

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

      Shift|array ``+(array|Shift s)
      {
	 if (!s) return this_object();
	 if (!arrayp(s)) s=({s});
	 return s+({this_object()});
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

      Shift try_promote(Shift t,int y0,int y1)
      {
   // this is year y0
   // t is year y1
	 if (t==this_object()) return t; // same!
	 if (t->time!=time ||
	     t->timetype!=timetype ||
	     t->offset!=offset ||
	     t->s!=s) return 0; // no chance

	 int a,b,c;
	 if (sscanf(dayrule,"LDAYL(%d,%d)",a,b)==2 &&
	     sscanf(t->dayrule,"FIX_L(%d)",c)==1)
	    if (ldayl_is_fix_l(a,b,c,y0,y1)) 
	       return this_object(); // ldayl
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

   class Rule
   {
      string id;

      mapping rules=([]);

      int amt=0;

      void create(string _id) { id=_id; }
   
      void add(string line)
      {
	 array a= array_sscanf(line, replace("%s %s %s %s %s %s %s %[^\t ]",
					     " ","%*[ \t]"));

	 if (sizeof(a)<8) complain("illegal rule line format\n");

	 if (!(int)a[0] && a[0]!="min")
	    complain("unknown year %O\n",a[0]);

   // ---

#define INF_YEAR 2050
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
	 string res="";

	 if (!r2[NUL_YEAR]) r2[NUL_YEAR]=({last});

	 int y=min(@indices(rules));
	 for (;y<=INF_YEAR; y++)
	    [r2[y],last]=mkperiods(rules[y],last,first);

	 res+=
	    TZrules_init+
	    "import __Calendar_mkzone;\n"
	    "   inherit TZRules;\n"
	    "   static array(array(string|int)) jd_year_periods(int jd)\n"
	    "   {\n"
	    "      [int y,int yjd,int leap]=gregorian_yjd(jd);\n"
	    "      switch (y)\n"
	    "      {\n";

	 string s="",t;

	 int y,mn=min(@indices(rules-(<NUL_YEAR>)));

	 for (y=INF_YEAR;sizeof(r2);y--)
	    if (r2[y])
	    {
	       array z=r2[y];
	       multiset my=(<y>);

	       foreach (indices(r2),int y2)
		  if (join_periods(z,r2[y2],y,y2))
		     my[y2]=1;

	       foreach ((array)my,int y2) m_delete(r2,y2);

	       string t="";

	       int y0=min(@(array)my);
	       int y2=max(@(array)my);
	       for (; y0<=y2; y0++)
		  if (my[y0])
		  {
		     int y1=y0;
		     while (my[++y1]);

		     y1--;
		  
		     if (y0==NUL_YEAR)
		     {
			if (my[INF_YEAR])
			   t+="         default: // .."+max(y1,mn-1)+
			      " and ½½½..\n";
			else
			   t+="         default: // .."+max(y1,mn-1)+":\n";
		     }
		     else if (y0==y1)
			t+="         case "+y0+":\n";
		     else if (y1==2050)
		     { 
			if (!my[NUL_YEAR]) t+="         case "+y0+"..:\n"; 
			else t=replace(t,"½½½",(string)y0); 
		     }
		     else 		  
			t+="         case "+y0+".."+y1+":\n";

		     y0=y1;
		  }

	       int lastoffset=0;
	       string res=" "*12+"return ({";
	       foreach (z,Shift s)
	       {
		  res+=s->dump(lastoffset,my)+("\n"+" "*21);
		  lastoffset=s->offset;
	       }
	       array resa=res/"\n";
	       resa[-2]=replace(resa[-2],",  ","});");
	 
	       t+=resa[..sizeof(resa)-2]*"\n"+"\n";
	       s=t+s;
	    }
	 res+=(s+
	       "      }\n"
	       "   }\n");

	 return res;
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
   }

   class Zone
   {
      string id;

      array rules=({});

      void create(string _id) { id=_id; }

      void add(string line)
      {
	 array a= array_sscanf(line, replace("%s %s %s %s",
					     " ","%*[ \t]"));
	 if (sizeof(a)<4)
	    complain("parse error\n");
      
	 array a=({parse_offset(a[0]), // offset
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
	       "Ruleset.Timezone(%d,%O)",
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
	 string res="";

	 if (!sizeof(rules))
	 {
	    res+=("// skipped %O due to errors\n",id);
	    return res;
	 }
	       
	 res+="import __Calendar_mkzone;\n";

	 if (sizeof(rules)==1) // simple zone
	 {
	    res+=("object thezone="+rules[0][4]+";\n");
	    return res;
	 }

	 mapping rname=([]);
	 int n=1;
      
	 foreach (rules,array a)
	    if (rname[a[4]]) a[6]=rname[a[4]];
	    else a[6]=rname[a[4]]="tz"+n++;
      
	 res+=( "inherit TZHistory;\n"
		"Ruleset.Timezone "+
		sort(values(rname))*","+";\n"
		"Ruleset.Timezone whatrule(int ux)\n"
		"{\n" );

	 foreach (rules,array a)
	 {
	    if (!a[5]) a[5]=rule_shift(a);

	    string s="";
	    sscanf(a[3],"%s#%*[ \t]%s",a[3],s);
	    a[3]="from "+reverse(array_sscanf(reverse(a[3]),"%*[ \t]%s")[0]);
	    a[7]=s;
	 }

	 array last=rules[-1];
	 int n=sizeof(rules);
	 foreach (reverse(rules)[1..],array a)
	 {
	    res+=sprintf("   if (ux>=%s) // %s %s\n"
			 "      return %s || (%s=%s);\n",
			 a[5],a[3],last[7],last[6],last[6],last[4]);
	    n--;
	    last=a;
	 }
	 if (last[7]!="")
	    res+=sprintf("   // %s\n",last[7]);
	 res+=sprintf("   return %s || (%s=%s);\n",
		      last[6],last[6],last[4]);

	 res+=("}\n");
      
	 return res;
      }
   }

   string base_path=combine_path(__FILE__,"../tzdata/");
   array files=
   ({
      "africa",
      "antarctica",
      "asia",
      "australasia",
      "backward",
      "etcetera",
      "europe",
      "northamerica",
      "pacificnew",
      "systemv",
   });

   mapping zone_cache=([]);
   mapping rule_cache=([]);
   string all_rules=0;

   string get_all_rules()
   {
      return 
	 map(files,
	     lambda(string fn)
	     {
		return Stdio.read_bytes(base_path+fn) ||
		   error("Failed to open file %O\n",base_path+fn);
	     })*"\n";
   }

   class Dummymodule
   {
      function(string:mixed) f;
      mixed `[](string s) { return f(s); }
      void create(function(string:mixed) _f) { f=_f; }
   }

   mapping mkzonemod=
   (["TZrules":Dummymodule(find_rule),
     "TZRules":TZRules,
     "TZHistory":TZHistory,
     "Ruleset":Ruleset]);

   object find_zone(string s)
   {
#ifdef RTTZC_DEBUG
      werror("Searching for zone %O\n",s);
#endif
      if (zone_cache[s]) return zone_cache[s];

      if (!all_rules) all_rules=get_all_rules();

      Zone z=Zone(s);   
      int n=0;
      for (;;)
      {
	 n=search(all_rules,s,n);
#ifdef RTTZC_DEBUG
	 werror("hit at: %O\n",n);
#endif
	 if (n==-1) 
	    return ([])[0];
	 int i=max(n-100,0)-1,j;
	 do i=search(all_rules,"\nZone",(j=i)+1); while (i<n && i!=-1);

	 if (j<n && i!=-1 &&
	     sscanf(all_rules[j..j+8000],"\nZone%*[ \t]%[^ \t]%*[ \t]%s\n%s", 
		    string a,string b,string q)==5 && 
	     a==s)
	 {
	    z->add(b);
	    foreach (q/"\n",string line)
	       if (sscanf(line,"%*[ \t]%[-0-9]%s",a,b)==3 && strlen(a))
		  z->add(a+b);
	       else 
		  break; // end of zone
	    break;
	 }
	 i=max(n-100,0)-1;
	 do i=search(all_rules,"\nLink",(j=i)+1); while (i<n && i!=-1);
	 if (j<n && i!=-1 &&
	     sscanf(all_rules[j..j+100],"\nLink%*[ \t]%[^ \t]%*[ \t]%[^ \t\n]", 
		    string a,string b)==4 &&
	     b==s)
	    return find_zone(a);
	 n++;
      }
      string c=z->dump();

#ifdef RTTZC_DEBUG
      werror("%s\n",c);
#endif
      
      add_constant("__Calendar_mkzone",mkzonemod);

      program p=compile_string(c);
      object zo=p();
      if (zo->thezone) zo=zo->thezone;

      return zone_cache[s]=zo;
   }

   program find_rule(string s)
   {
      s=UNFIXID(s);
      if (rule_cache[s]) return rule_cache[s];
#ifdef RTTZC_DEBUG
      werror("Searching for rule %O\n",s);
#endif

      if (!all_rules) all_rules=get_all_rules();

      Rule r=Rule(s);   
      int n=0;
      for (;;)
      {
	 n=search(all_rules,s,n);
#ifdef RTTZC_DEBUG
	 werror("hit at: %O\n",n);
#endif
	 if (n==-1) 
	    return ([])[0];
	 string t=all_rules[n-20..n+8000]; // dummy limit to speed up

	 if (sscanf(t,"%*s\nRule%*[ \t]%[^ \t]%*[ \t]%s\n%s",
		    string a,string b,t)==6 && a==s)
	 {
	    r->add(b);
	    foreach (t/"\n",string line)
	       if (sscanf(line,"Rule%*[ \t]%[^ \t]%*[ \t]%s",a,b)==4 &&
		   a==s)
		  r->add(b);
	       else 
		  break; // end of zone
	    break;
	 }
	 n++;
      }
      string c=r->dump();

#ifdef RTTZC_DEBUG
      werror("%s\n",c);
#endif
      
      add_constant("__Calendar_mkzone",mkzonemod);

      program p=compile_string(c);
      return rule_cache[s]=p;
   }


   int main(int ac,array(string) am)
   {
      map(am[1..],find_zone);
      return 0;
   }

// ----------------------------------------------------------------
// Base class for daylight savings and war time rules
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// Base "Timezone with rules" class
// ----------------------------------------------------------------

   class TZRules
   {
      constant is_timezone=1;
      constant is_dst_timezone=1;
      static int offset_to_utc;  
      string name;

      static function(string:string) tzformat;
      static array names;


// ----------------------------------------------------------------
// all rules are based on the gregorian calendar, so 
// this is the needed gregorian rule:
// ----------------------------------------------------------------

      static array gregorian_yjd(int jd)
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

      static void create(int offset,string _name) 
      { 
	 offset_to_utc=offset; 
	 name=_name;
	 if (search(name,"/")!=-1)
	 {
	    names=name/"/";
	    tzformat=lambda(string s)
		     {
			if (s=="") return names[0]; else return names[1];
		     };
	 }
	 else
	    tzformat=lambda(string s) { return sprintf(name,s); };
      }

// the Rule:
// which julian day does dst start and end this year?
      static array(array(string|int)) jd_year_periods(int jd);

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

      string _sprintf(int t) { return (t=='O')?"Timezone("+name+")":0; }

      int raw_utc_offset() { return offset_to_utc; }
   }

   class TZHistory
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
