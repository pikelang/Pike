#pike __REAL_VERSION__

import ".";

string all_data=0;
mapping loaded_events=([]);
mapping loaded_regions=([]);

constant month2n=(["Jan":1,"Feb":2,"Mar":3,"Apr":4,"May":5,"Jun":6,
		   "Jul":7,"Aug":8,"Sep":9,"Oct":10,"Nov":11,"Dec":12]);
constant wd2n=(["Mon":1,"Tue":2,"Wed":3,"Thu":4,"Fri":5,"Sat":6,"Sun":7]);

void read_all_data()
{
   all_data=master()->master_read_file(
      combine_path(__FILE__,"../events/regional"));
}

Event.Event make_event(string source)
{
   string id;
   string flags;
   string s;
   string rule;

   if (sscanf(source, "%*[ \t]Event%*[ \t]"
	     "\"%[^\"]\"%*[ \t]%[-a-z]%*[ \t]"
	     "\"%s\"%*[ \t]%[^\n]",
	     id,flags,s,rule)!=9)
      error("Events: rule error; unknown format:\n%O\n",source);
   if (search(rule,"\"")!=-1) 
   {
      if (sscanf(source, "%*[ \t]Event%*[ \t]"
		 "\"%[^\"]\"%*[ \t]%[-a-z]%*[ \t]"
		 "\"%s\n",
		 id,flags,s)!=7)
	 error("Events: rule error; unknown format:\n%O\n",source);
      sscanf(replace(s,"\\\"","»»»1902318231»»»"),
	     "%s\"%*[ \t]%s",s,rule);
      s=replace(s,"»»»1902318231»»»","\"");
   }

   string ruleid=rule,mn,wd,a;
   int md,n,m;
   int days=1;
   sscanf(rule,"%[^ \t\n]",ruleid);
   switch (ruleid)
   {
      case "Fixed":
	 if (sscanf(rule,"Fixed%*[ \t]%s%*[ \t]%d%*[ \t]%d days",
		    mn,md,days)>=4 &&
	     month2n[mn])
	    return Event.Gregorian_Fixed(id,s,md,month2n[mn]);
	 error("Events: rule error; unknown rule format:\n%O\n",source);

      case "Julian_Fixed":
	 if (sscanf(rule,"Julian_Fixed%*[ \t]%s%*[ \t]%d%*[ \t]%d days",
		    mn,md,days)>=4 &&
	     month2n[mn])
	    return Event.Julian_Fixed(id,s,md,month2n[mn]);
	 error("Events: rule error; unknown rule format:\n%O\n",source);

      case "Islamic_Fixed":
      case "Coptic_Fixed":
   // not supported yet
	 return Event.NullEvent(id,s);

      case "WDRel":
   // WDRel May Fri 1st
	 if (sscanf(rule,
		    "WDRel%*[ \t]%s%*[ \t]%s%*[ \t]%d%*[a-z]%*[ \t]%d days",
		    mn,wd,n,days)>=5 &&
	     month2n[mn] && wd2n[wd] && n>0)
	 {
	    Event.Event e=
	       Event.Monthday_Weekday_Relative(
		  id,s,1,month2n[mn],wd2n[wd],n,1);
	    e->nd=days;
	    return e;
	 }
   // WDRel May Fri last
	 if (sscanf(rule,
		    "WDRel%*[ \t]%s%*[ \t]%s%*[ \t]%s%*[ \t]%d days",
		    mn,wd,a,days)>=5 && a=="last" &&
	     (m=month2n[mn]) && wd2n[wd])
	 {
	    m=(m%12)+1;
	    Event.Event e=
	       Event.Monthday_Weekday_Relative(
		  id,s,1,m,wd2n[wd],-1,0);
	    e->nd=days;
	    return e;
	 }
   // WDRel May 17 Fri +17 excl
	 days=a=0;
	 if (sscanf(rule,
		    "WDRel%*[ \t]%[A-z]%*[ \t]%d%*[ \t]%s%*[ \t]"
		    "%d%*[ \t]%[a-z]%*[ \t]%d days",
		    mn,md,wd,n,a,days)>=9 && a && a!="" &&
	     (m=month2n[mn]) && wd2n[wd])
	 {
	    if (!(<"incl","excl">)[a])
	       error("Events: rule error; expected incl or excl, not %O"
		     ":\n%O\n",a,source);

	    m=(m%12)+1;
	    Event.Event e=
	       Event.Monthday_Weekday_Relative(
		  id,s,md,month2n[mn],wd2n[wd],n,a!="excl");
	    e->nd=days;
	    return e;
	 }
   // WDRel May 17 Fri +17
	 if (sscanf(rule,
		    "WDRel%*[ \t]%[A-z]%*[ \t]%d%*[ \t]%[^ \t]%*[ \t]"
		    "%d%*[ \t]%d days",
		    mn,md,wd,n,days)>=7 && 
	     (m=month2n[mn]) && wd2n[wd])
	 {
	    m=(m%12)+1;
	    Event.Event e=
	       Event.Monthday_Weekday_Relative(
		  id,s,md,month2n[mn],wd2n[wd],n,1);
	    e->nd=days;
	    return e;
	 }
	 error("Events: rule error; unknown rule format:\n%O\n",source);

      case "Easter":
	 if (sscanf(rule,"Easter%*[ \t]%d%*[ \t]%d days",
		    n,days)>=2)
	    return Event.Easter_Relative(id,s,n);
	 error("Events: rule error; unknown rule format:\n%O\n",source);
	 
      case "Julian_Easter":
	 if (sscanf(rule,"Julian_Easter%*[ \t]%d%*[ \t]%d days",
		    n,days)>=2)
	    return Event.Easter_Relative(id,s,n);
	 error("Events: rule error; unknown rule format:\n%O\n",source);
	 
      case "Weekday":
	 if (sscanf(rule,"Weekday%*[ \t]%s%*[ \t]%d days",
		    wd,days)>=2 && (n=wd2n[wd]))
	    return Event.Weekday(n,id);
	 error("Events: rule error; unknown rule format:\n%O\n",source);

      case "Null":
	 return Event.NullEvent(id,s);
	 
      default:
	 error("Events: rule error; unknown rule id:\n%O\n",source);
   }
}

constant nd_m_yd=(["jan":0,"feb":31,"mar":59,"apr":90,
		"may":120,"jun":151,"jul":181,"aug":212,
		"sep":243,"oct":273,"nov":304,"dec":334]);
constant nd_m_nd=(["jan":31,"feb":28,"mar":31,"apr":30,
		"may":31,"jun":30,"jul":31,"aug":31,
		"sep":30,"oct":31,"nov":30,"dec":31]);

Event.Namedays new_namedays_object(string id,string name,
			   int start,int stop,int leapdayshift,
			   array(array(string)) names)
{
   return Event.Namedays(id,name,names,0,
			 start,stop,leapdayshift);
}

mapping made_namedays=([]);

string read_all_namedays()
{
   return master()->master_read_file(
      combine_path(__FILE__,"../events/namedays"));
}

Event.Namedays find_namedays(string region)
{
   string id="namedays/"+region;
   object res;
   if ( (res=made_namedays[region]) ) 
      return res;
   string all=read_all_namedays();

   int i=search(all,"\nRegion \""+region+"\"");
   if (i==-1) return UNDEFINED; // not found

   int i2=search(all,"\nRegion",i+1);
   if (i2==-1) i2=strlen(all)-1;

   array(array(string)) names=0;
   int start=-1,stop=-1;
   int leapdayshift=2000;
   string charset="iso-8859-1";
   function(string:string) decoder=0;

   foreach (all[i..i2]/"\n",string line)
   {
      string w="",s;
      sscanf(line,"%*[ \t]%[^ \t]%*[ \t]%s",w,s);
#if 1
      switch (w=lower_case(w))
      {
	 case "":
	 case "region": // ignore
	    break;
	 case "leapdayshift":
	    sscanf(s,"%d",leapdayshift);
	    break;
	 case "charset":
	    sscanf(s,"%s",charset);
	    object dec=Locale.Charset.decoder(charset);
	    decoder=lambda(string s)
		    {
		       return dec->feed(s)->drain();
		    };
	    break;
	 case "period":
	    if (names) 
	       if (res) 
		  res|=new_namedays_object(region,id,
					   start,stop,leapdayshift,names);
	       else
		  res=new_namedays_object(region,id,
					  start,stop,leapdayshift,names);
	    names=0;

	    sscanf(s,"%[0-9]-%[0-9]",string sstart,string sstop);
	    if (sstart=="") start=-1,stop=(int)sstop;
	    else if (sstop=="") start=(int)sstart,stop=-1;
	    else start=(int)sstart,stop=(int)sstop;
	    break;
	 case "jan": case "feb": case "mar": case "apr":
	 case "may": case "jun": case "jul": case "aug":
	 case "sep": case "oct": case "nov": case "dec":
  	    if (!names) names=allocate(366);

	    sscanf(s,"%d%*[ ]%{%*[, ]%[^,]%}",int mday,array name);
#if 1
	    if (mday<1 || mday>nd_m_nd[w])
	       error("Nameday date doesn't exists:\n%O\n",line);
#endif
	    if (sizeof(name))
	    {
	       name=`+(@name);
	       if (decoder) name=map(name,decoder);
	       names[nd_m_yd[w]+mday-1]=name;
	    }
	    break;
	 case "leapday":
	    sscanf(s,"%{%*[, ]%[^,]%}",name);
	    names[365]=`+(@name);
	    break;
	 default:
	    if (w[0]=='#') break;
	    error("Unknown namedays definition statement:\n%O\n",line);
      }
#endif
   }
   if (names) 
      if (res) 
	 res|=new_namedays_object(region,id||"?",
				  start,stop,leapdayshift,names);
      else
	 res=new_namedays_object(region,id||"?",start,stop,leapdayshift,names);
   
   return res;
}

Event.Event find_event(string s)
{
   Event.Event e=loaded_events[s];
   if (e) return e;

   if (!all_data) read_all_data();

   if (s[..8]=="namedays/")
      return find_namedays(s[9..]);

   int i=search(all_data,sprintf("Event %O",s));
   if (i==-1) return UNDEFINED;
   
   int j=search(all_data,"\n",i);
   if (j==-1) j=0x7fffffff;
   return make_event(all_data[i..j]);
}

Event.Event find_region(string c)
{
   Event.Event e=loaded_events[c];
   if (e) return e;

   if (!all_data) read_all_data();

   int i=search(all_data,sprintf("\nRegion %O",c));
   if (i==-1) return UNDEFINED;
   
   int j=search(all_data,"\nRegion \"",i+1);
   if (j==-1) j=0x7fffffff;

   array(Event) events=({});
   mapping(Event:multiset(string)) eflags=([]);
   mapping(string:multiset(string)) flagy=([]);

   foreach ( (all_data[i..j]/"\n")[2..], string line)
   {
      string whut,id="",flags="-";
      Event.Event e;
      if (sscanf(line,"%*[ \t]%[^ \t]%*[ \t]\"%s\"%*[ \t]%[-a-z]",
		 whut,id,flags)>=4 &&
	  whut!="" && whut[0]!='#')
	 switch (whut)
	 {
	    case "Add": 
	       e=find_event(id);
	       if (!e)
		  error("Events: Adding undefined event\n%O\n",line);
	       if (flags!="-")
		  eflags[e]=(flagy[flags]||
			     (flagy[flags]=mkmultiset( (flags-"-")/"" )));
	       events+=({e});
	       break;
	    case "Event": 
	       e=make_event(line);
	       if (flags!="-")
		  eflags[e]=(flagy[flags]||
			     (flagy[flags]=mkmultiset( (flags-"-")/"" )));
	       events+=({e});
	       break;
	    case "Include":
	       e=find_region(id);
	       if (!e)
		  error("Events: Including undefined region %O\n%O\n",id,line);
	       if (!e->is_superevent)
		  error("Events: Including non-region %O\n%O\n",id,line);
	       events|=e->events;
	       if (flags=="-")
		  eflags|=e->flags;
	       else
	       {
		  multiset f1=(flagy[flags]||
			       (flagy[flags]=mkmultiset( (flags-"-")/"" )));
		  foreach (e->events, Event.Event e2)
		     eflags[e2]=f1|(e->flags[e2] || (<>));
	       }
	       break;
	    case "Without":
	       e=find_event(id);
	       if (!e)
		  error("Events: Removing undefined event\n%O\n",line);
	       events-=({e});
	       m_delete(eflags,e);
	       break;
	    default:
	       error("Events: Unknown region directive:\n%O\n",line);
	 }
   }
   return loaded_regions[c]=Event.SuperEvent(events,eflags,c);
}

array all_regions()
{
   if (!all_data) read_all_data();
   return `+(@array_sscanf(all_data,"%{%*s\nRegion \"%s\"%}")[0]);
}

// -----------------------------------------------------------------------

program|Event.Event `[](string s)
{
   return ::`[](s) || magic_event(s);
}
function `-> = `[];

// Don't load Geogrphy.Countries unless we have to
object country_lookup=0;

Event.Event|Event.Namedays magic_event(string s)
{
   Event.Event e;
   if ( (e=loaded_events[s]) ) return e;

   s=replace(s,"_"," ");

   if (e=find_event(s)) return e;
   if (e=find_region(s)) return e;

   if (!country_lookup) 
      country_lookup=master()->resolv("Geography.Countries");
   object c=country_lookup->from_name(s);
   if (c && (e=find_region(lower_case(c->iso2)))) return e;
   
   if (s=="tzshift") 
      return loaded_events->tzshift=Event.TZShift_Event();

   return UNDEFINED;
}

Event.SuperEvent country(string s)
{
   s=replace(s,"_"," ");
   return find_region(s);
}

Event.SuperEvent event(string s)
{
   s=replace(s,"_"," ");
   return find_event(s);
}

mapping made_events=([]);

