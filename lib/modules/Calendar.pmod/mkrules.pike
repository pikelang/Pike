// this is a script to generate rules
// from timezone data files;
// ftp://elsie.nci.nih.gov/pub/ 
// (timezone mailing list: tz@elsie.nci.nih.gov)
//
// source datafile are usually found somewhere around zic(8),
// if they exist in the system.
//
// Most systems only have compiled files, just like pike,
// and zic(8) is the usual compiler.

// pike mkrules.pike ../data/{africa,antarctica,asia,australasia,backward,etcetera,europe,northamerica,pacificnew,southamerica,systemv}
// $Id$

#pike __REAL_VERSION__

object cal=Calendar.ISO->set_timezone("UTC");
function Year=cal->Year;
object nleapy=Year(1999);

object tzrules; // needed to make timezones, compiled below

mapping rules=([]);
mapping zones=([]);
mapping links=([]);
array arules=({});
array azones=({});

#define FIXED(D)   (yjd+((D)-1))
#define FIX_L(D)   (yjd+leap+((D)-1))
#define LDAY(D,W)  (yjd+((D)-1)-( (yjd+((D)+(8-W)-1)) % 7))
#define LDAYL(D,W) (yjd+((D)-1)+leap-( (yjd+leap+((D)+(8-W)-1)) % 7))

#define FIXID(id) replace(id,"/-+"/1,"__p"/1)

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
      if (!s) return this;
      if (!arrayp(s)) s=({s});
      return s+({this});
   }

   int ldayl_is_fix_l(int d1,int wd,int d2,int yn1,int yn2)
   {
      object y1=Year(yn1);
      object y2=Year(yn2);
      int yjd,leap;

      yjd=y1->julian_day();
      leap=y1->leap_year();
      d1=LDAYL(d1,wd);

      yjd=y2->julian_day();
      leap=y2->leap_year();
      d2=FIX_L(d2);

      return d1==d2;
   }

   Shift try_promote(Shift t,int y0,int y1)
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
      int l=Year(z[0])->leap_year();
      foreach (z[1..],int y) if (Year(y)->leap_year()!=l) { l=2; break; }
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

class MyRule
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

      for (int y=min(@indices(rules));y<=INF_YEAR; y++)
	 [r2[y],last]=mkperiods(rules[y],last,first);

      res+=("class "+
	    FIXID(id)+"\n"
	    "{\n"
	    "   inherit TZRules;\n"
	    "   static array(array(string|int)) jd_year_periods(int jd)\n"
	    "   {\n"
	    "      [int y,int yjd,int leap]=gregorian_yjd(jd);\n"
	    "      switch (y)\n"
	    "      {\n");

      string s="",t;

      int mn=min(@indices(rules-(<NUL_YEAR>)));

      for (int y=INF_YEAR;sizeof(r2);y--)
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
	    "   }\n"
	    "}\n\n");

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
	       if (!tzrules) return 0; // can't do that now
	       object|program rules=tzrules[FIXID(a[1])];
	       if (!rules)
	       {
		  werror("ERROR: Missing rule %O (used in Zone %O)\n",a[1],id);
		  return "[err]";
	       }
	       rules=rules(-a[0],"-");

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
      string cid=FIXID(id);
      string res="";

      if (!sizeof(rules))
      {
	 res+=("// skipped %O due to errors\n",id);
	 return res;
      }
	       
      if (sizeof(rules)==1) // simple zone
      {
	 res+=("Rule.Timezone "+cid+"="+
	       rules[0][4]+";\n");
	 return res;
      }

      mapping rname=([]);
      int n=1;
      
      foreach (rules,array a)
	 if (rname[a[4]]) a[6]=rname[a[4]];
	 else a[6]=rname[a[4]]="tz"+n++;
      
      res+=("class "+cid+"\n"
	    "{\n"
	    "   inherit TZHistory;\n"
	    "   Rule.Timezone "+
	    sort(values(rname))*","+";\n"
	    "   Rule.Timezone whatrule(int ux)\n"
	    "   {\n"
	    );

      foreach (rules,array a)
      {
	 if (!a[5]) a[5]=rule_shift(a);

	 string s="";
	 sscanf(a[3],"%s#%*[ \t]%s",a[3],s);
	 a[3]="from "+reverse(array_sscanf(reverse(a[3]),"%*[ \t]%s")[0]);
	 a[7]=s;
      }

      array last=rules[-1];
      n=sizeof(rules);
      foreach (reverse(rules)[1..],array a)
      {
	 res+=sprintf("      if (ux>=%s) // %s %s\n"
		      "         return %s || (%s=%s);\n",
		      a[5],a[3],last[7],last[6],last[6],last[4]);
	 n--;
	 last=a;
      }
      if (last[7]!="")
	 res+=sprintf("      // %s\n",last[7]);
      res+=sprintf("      return %s || (%s=%s);\n",
		   last[6],last[6],last[4]);

      res+=("   }\n"
	    "}\n");
      
      return res;
   }
}

void complain(string fmt,mixed ... args)
{
   throw( sprintf(fmt,@args) );
}

void collect_rules(string file)
{
   int n=0;
   werror("reading %O...\n",file);
   string s=Stdio.read_bytes(file),t;
   if (!s) 
   {
      werror("%s:-: Failed to open file: %s\n",file,strerror(errno()));
      return;
   }

   Zone lastz;
   MyRule lastr;

   foreach (s/"\n",string line)
   {
      n++;
      mixed err=catch 
      {
	 if (line[..0]!="#") 
	    if (sscanf(line,"Zone%*[ \t]%[^ \t]%*[ \t]%s",s,t)==4)
	    {
	       if (zones[s]) lastz=zones[s]->add(t);
	       else (lastz=zones[s]=Zone(s))->add(t),azones+=({lastz});
	    }
	    else if (sscanf(line,"Rule%*[ \t]%[^ \t]%*[ \t]%s",s,t)==4)
	    {
	       if (rules[s]) rules[s]->add(t);
	       else (lastr=rules[s]=MyRule(s))->add(t),arules+=({lastr});
	       lastz=0;
	    }
	    else if (sscanf(line,"Link%*[ \t]%[^ \t]%*[ \t]%[^ \t]",s,t)==4)
	    {
	 // link t to s
	       if (links[s]) links[s]+=({t});
	       else links[s]=({t});
	    }
	    else if (sscanf(line,"%*[ \t]%[-0-9]%s",s,t)==3 && sizeof(s))
	    {
	       if (!lastz) complain("implicit zone line w/o zone\n");
	       lastz->add(s+t);
	    }
	    else if ((t="",sscanf(line,"%[ \t]",t),t==line))
	       ;
	    else if (sscanf(line,"%*[ \t]#%s",t,s)==2)
	       ;
	    else
	       complain("unknown keyword %O...\n",line[..10]);
      };
      if (err)
	 if (stringp(err))
	    werror("%s:%d: %s",file,n,err);
	 else
	    throw(err);
   }
}

int main(int ac,array(string) am)
{
   if (sizeof(am)<1)
   {
      werror("USAGE: %s datafile [datafile ...]\n",am[0]);
      return 1;
   }
   map(am[1..],collect_rules);

   write("thinking...\n");

   string t=TZrules_base;

   foreach (arules,MyRule r)
      t+=r->dump();

   tzrules=compile_string(t)();

   mv("TZrules.pmod","TZrules.pmod~");
   werror("writing TZrules.pmod (%d bytes)...",sizeof(t));
   Stdio.File("TZrules.pmod","wtc")->write(t);
   werror("\n");

   t="// ----------------------------------------------------------------\n"
      "// Timezones\n"
      "//\n"
      "// NOTE: this file is generated by mkrules.pike;\n"
      "//       please do not edit manually /Mirar\n"
      "// ----------------------------------------------------------------\n"
      "\n"
      "import \".\";\n\n";

   t+=("// "+"-"*70+"\n"
       "// Timezones\n"
       "// "+"-"*70+"\n\n");

   mixed err=catch {
      foreach (azones,Zone z)
         if (sizeof(z->rules)==1) 
	 {
	    t+=z->dump();
	    if (links[z->id])
	       foreach(links[z->id],string s)
		  t+="Rule.Timezone "+FIXID(s)+"="+
		     FIXID(z->id)+";\n";
	 }
   };
   if (err) if (stringp(err)) error(err); else throw(err);

   t+=("\n"
       "// "+"-"*70+"\n"
       "// Timezones with an attitude\n"
       "// "+"-"*70+"\n"
       "\n");

   err=catch {
      foreach (azones,Zone z)
	 if (sizeof(z->rules)!=1) 
	 {
	    t+=z->dump();
	    if (links[z->id])
	       foreach(links[z->id],string s)
		  t+="constant "+FIXID(s)+"="+
		     FIXID(z->id)+";\n";
	    t+="\n";
	 }
   };
   if (err) if (stringp(err)) error(err); else throw(err);

   t+=("\n"
       "// "+"-"*70+"\n");

   mv("TZs.pike","TZs.pike~");
   werror("writing TZs.h (%d bytes)...",sizeof(t));
   Stdio.File("TZs.h","wtc")->write(t);
   werror("\n");

   mapping zs=([]);
   foreach (azones,Zone z)
      if (sscanf(z->id,"%s/%s",string s,string t)==2)
	 zs[s]=(zs[s]||({}))+({t});

   t="// ----------------------------------------------------------------\n"
      "// Timezone names\n"
      "//\n"
      "// NOTE: this file is generated by mkrules.pike;\n"
      "//       please do not edit manually /Mirar\n"
      "// ----------------------------------------------------------------\n"
      "\n"
      "mapping _module_value=\n"
      "([\n";
   foreach (indices(zs)-({"SystemV","Etc"}),string co)
      t+=(replace(
	 sprintf("   %-13s({%-=63s\n",
		 sprintf("%O:",co),
		 map(zs[co],lambda(string s) { return sprintf("%O",s); })
		 *", "+"}),"),({",                 ",", "}),",,"/1));
   t+="]);\n\n";
   
   mv("TZnames.pike","TZnames.pike~");
   werror("writing TZnames.pmod (%d bytes)...",sizeof(t));
   Stdio.File("TZnames.pmod","wtc")->write(t);
   werror("\n");

   return 0;
}


string TZrules_base=
#"// ----------------------------------------------------------------
// Daylight savings and war time rules
//
// NOTE: this file is generated by mkrules.pike;
//       please do not edit manually /Mirar
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// all rules are based on the gregorian calendar, so 
// this is the gregorian rule:
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

// ----------------------------------------------------------------
// Base \"Timezone with rules\" class
// ----------------------------------------------------------------

class TZRules
{
   constant is_timezone=1;
   constant is_dst_timezone=1;
   static int offset_to_utc;  
   string name;

   static function(string:string) tzformat;
   static array names;

   static void create(int offset,string _name) 
   { 
      offset_to_utc=offset; 
      name=_name;
      if (has_value(name, \"/\"))
      {
	 names=name/\"/\";
	 tzformat=lambda(string s)
		  {
		     if (s==\"\") return names[0]; else return names[1];
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

   string _sprintf(int t) { return (t=='O')?\"Timezone(\"+name+\")\":0; }

   int raw_utc_offset() { return offset_to_utc; }
}

// ----------------------------------------------------------------------
// DST Rules
// ----------------------------------------------------------------------

// useful macros
#define FIXED(D)   (yjd+((D)-1))
#define FIX_L(D)   (yjd+leap+((D)-1))
#define LDAY(D,W)  (yjd+((D)-1)-( (yjd+((D)+(8-W)-1)) % 7))
#define LDAYL(D,W) (yjd+((D)-1)+leap-( (yjd+leap+((D)+(8-W)-1)) % 7))
#define UO offset_to_utc

// ----------------------------------------------------------------------
";
