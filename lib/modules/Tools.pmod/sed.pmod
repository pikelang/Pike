#pike __REAL_VERSION__

//! edit commands supported:
//! @pre{
//! <firstline>,<lastline><edit command>
//!    ^^ numeral (17) ^^
//!       or relative (+17, -17)
//!       or a search regexp (/regexp/)
//!       or multiple (17/regexp//regexp/+2)
//! @}
//!
//! @xml{
//! <matrix>
//! <r><c><b>Command</b></c><c><b>Action</b></c></r>
//! <r><c>D</c><c>Delete first line in space</c></r>
//! <r><c>G</c><c>Insert hold space</c></r>
//! <r><c>H</c><c>Append current space to hold space</c></r>
//! <r><c>P</c><c>Print current data</c></r>
//! <r><c>a&lt;string&gt;</c><c>Insert</c></r>
//! <r><c>c&lt;string&gt;</c><c>Change current space</c></r>
//! <r><c>d</c><c>Delete current space</c></r>
//! <r><c>h</c><c>Copy current space to hold space</c></r>
//! <r><c>i&lt;string&gt;</c><c>Print string</c></r>
//! <r><c>l</c><c>Print current space</c></r>
//! <r><c>p</c><c>Print first line in data</c></r>
//! <r><c>q</c><c>Quit evaluating</c></r>
//! <r><c>s/regexp/with/x</c><c>Replace</c></r>
//! <r><c>y/chars/chars/</c><c>Replace chars</c></r>
//! </matrix>
//! @}
//!
//! where line is numeral, first 'line'==0

static array sedreplace(string s,object re,string with,
			array whatin,int first,int lastmod,
			multiset flags)
{
   array a;
   string w=0;
   array pr=({});

   if (!(a=re->split(s)))
      return 0;

   if (first)
   {
      array wa;
      wa=sedreplace(a[0],re,with,whatin,first,lastmod,flags);
      if (wa)
	 if (!flags["g"])
	    return ({wa[0],wa[1]+s[strlen(a[0])..]});
	 else
	    pr=wa[0],w=wa[1];
      else
	 w=a[0];
   }
   
   string t=
      replace(with,whatin[..sizeof(a)-first+lastmod-1],
	      a[first..sizeof(a)+lastmod-1]);

   if (flags["p"]) pr+=({t});

   s=(w||"")+t;
   if (flags["g"])
   {
      if (lastmod) 
      {
	 array wa;
	 wa=sedreplace(a[-1],re,with,whatin,first,lastmod,flags);
	 if (wa) 
	 {
	    pr+=wa[0];
	    s+=wa[1];
	 }
	 else
	    s+=a[-1];
      }
   }
   else
      s+=a[-1];

   return ({pr,s});
};

static array scan_for_linenumber(string cmd,
				 array(string) in,
				 int n)
{
   int x;
   string what;
   object re;

   while (cmd!="" && ((cmd[0]>='0' && cmd[0]<='9') 
		      || cmd[0]=='/' || cmd[0]=='+' || cmd[0]=='-'))
   {
      if (cmd[0]>='0' && cmd[0]<='9')
      {
	 sscanf(cmd,"%d%s",n,cmd);
	 // n--; if first line==1
      }
      else if (cmd[0]=='+')
      {
	 sscanf(cmd,"+%d%s",x,cmd);
	 n+=x; 
      }
      else if (cmd[0]=='-')
      {
	 sscanf(cmd,"-%d%s",x,cmd);
	 n-=x; 
      }
      else if (sscanf(cmd,"/%s/%s",what,cmd)==2)
      {
	 re=Regexp(what);
	 while (n<sizeof(in))
	 {
	    if (re->match(in[n])) break;
	    n++;
	 }
      }
      else break;
   }
   if (n<0) n=0; else if (n>=sizeof(in)) n=sizeof(in)-1;
   return ({n,cmd});
}

//!
string|array `()(string|array(string) commands,
		 string|array(string) data,
		 void|int suppress)
{
   int start,stop;
   string div,what,with,inflags;
   multiset flags;
   array whatin=({"\\1","\\2","\\3","\\4","\\5","\\6","\\7","\\8","\\9"});
   array print=({});
   array hold=({});
   object re;
   array a1,a2;
   array in,e;

   if (arrayp(data)) in=copy_value(data);
   else in=data/"\n";

   if (arrayp(commands)) e=commands;
   else e=commands/"\n";

   start=0; 
   stop=sizeof(in)-1;

   foreach (e, string cmd)
   {
      if (cmd!="" && ((cmd[0]>='0' && cmd[0]<='9') 
		      || cmd[0]=='/' || cmd[0]=='+' || cmd[0]=='-'))
      {
	 a1=scan_for_linenumber(cmd,in,start);
	 stop=start=a1[0]; 
	 cmd=a1[1];
      }

      if (cmd[0..1]==",$") { cmd=cmd[2..]; stop=sizeof(in)-1; }
      else if (sscanf(cmd,",%s",cmd))
      {
	 a1=scan_for_linenumber(cmd,in,start);
	 stop=a1[0]; 
	 cmd=a1[1];
      }

      if (stop>sizeof(in)-1) stop=sizeof(in)-1;
      if (start<0) start=0;
      
      if (cmd=="") continue;
      switch (cmd[0])
      {
	 case 's':
	    div=cmd[1..1]; 
	    if (div=="%") div="%%";
	    inflags="";
	    if (sscanf(cmd,"%*c"+div+"%s"+div+"%s"+div+"%s",
		       what,with,inflags)<3) continue;
	    flags=aggregate_multiset(@(inflags/""));
	    
	    int first=0,lastmod=0;
	    if (what!="") // fix the regexp for working split
	    {
	       if (what[0]!='^') what="^(.*)"+what,first=1;
	       if (what[-1]!='$') what=what+"(.*)$",lastmod=-1;
	    }
	    re=Regexp(what);

	    while (start<=stop)
	    {
	       array sa=sedreplace(in[start],re,with,whatin,
				   first,lastmod,flags);

	       if (sa)
	       {
		  in[start]=sa[1];
		  print+=sa[0];
		  if (!flags["g"]) break;
	       }
	       start++;
	    }
	    
	    break;

	 case 'y':
	    div=cmd[1..1]; 
	    if (div=="%") div="%%";
	    inflags="";
	    if (sscanf(cmd,"%*c"+div+"%s"+div+"%s"+div+"%s",
		       what,with,inflags)<3) continue;
	    if (strlen(what)!=strlen(with))
	    {
	       what=what[0..strlen(with)-1];
	       with=with[0..strlen(what)-1];
	    }
	    
	    a1=what/"",a2=with/"";

	    while (start<=stop)
	    {
	       in[start]=replace(in[start],a1,a2);
	       start++;
	    }
	    break;

	 case 'G': // insert hold space
	    in=in[..start-1]+hold+in[start..];
	    if (stop>=start) stop+=sizeof(hold);
	    break;

	 case 'a': // insert line
	    in=in[..start-1]+({cmd[1..]})+in[start..];
	    if (stop>=start) stop++;
	    break;

	 case 'c': // change 
	    in=in[..start-1]+({cmd[1..]})+in[stop+1..];
	    stop=start;
	    break;

	 case 'd': // delete 
	    in=in[..start-1]+in[stop+1..];
	    stop=start;
	    break;

	 case 'D': // delete first line
	    in=in[..start-1]+in[start+1..];
	    stop=start;
	    break;

	 case 'h': // copy
	    hold=in[start..stop];
	    break;

	 case 'H': // appending copy
	    hold+=in[start..stop];
	    break;
	    
	 case 'i': // print text
	    print+=({cmd[1..]});
	    break;

	 case 'l': // print space
	    print+=in[start..stop];
	    break;

	 case 'P': // print all
	    print+=in;
	    break;

	 case 'p': // print first
	    print+=in[..0];
	    break;

	 case 'q': // quit
	    if (!suppress) 
	       return (arrayp(data)?(print+in):((print+in)*"\n"));
	    return (arrayp(data)?(print):(print*"\n"));
   
	 default:
	    // error? just ignore for now
      }
   }
   if (!suppress) 
      return (arrayp(data)?(print+in):(print+in)*"\n");
   return (arrayp(data)?(print):(print*"\n"));
}
