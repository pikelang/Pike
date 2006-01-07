// $Id: Terminfo.pmod,v 1.22 2006/01/07 17:05:41 nilsson Exp $
#pike __REAL_VERSION__


#if constant(thread_create)
#define LOCK object m_key = mutex->lock()
#define UNLOCK destruct(m_key)
#define MUTEX static private object(Thread.Mutex) mutex = Thread.Mutex();
#else
#define LOCK
#define UNLOCK
#define MUTEX
#endif

MUTEX

static private array ctrlcharsfrom =
   map(indices(allocate(32)),
       lambda(int z) { return sprintf("^%c",z+64); })+
   map(indices(allocate(32)),
       lambda(int z) { return sprintf("^%c",z+96); });
static private array ctrlcharsto =
   map(indices(allocate(32)),
       lambda(int z) { return sprintf("%c",z); })+
   map(indices(allocate(32)),
       lambda(int z) { return sprintf("%c",z); });


static private class TermMachine {

  mapping(string:string|int) map = ([]);

  int tgetflag(string id)
  {
    return map[id]==1;
  }

  int tgetnum(string id)
  {
    return intp(map[id]) && [int]map[id];
  }
  
  string tgetstr(string id)
  {
    return stringp(map[id]) && [string]map[id];
  }

  string tparam(string f, mixed ... args)
  {
    array(string) fmt=f/"%";
    string res=fmt[0];
    string tmp;
    int z;
    mapping var=([]);
    array args0=args;
    
#define POP (z=[int]args[0],args=args[1..],z)
#define PUSH(x) (args=({x})+args)
    
    while ( (fmt=fmt[1..])!=({}) )
      if (fmt[0]=="") res+="%";
      else
      {
	switch (fmt[0][0])
	{
	case 'd': res+=sprintf("%d%s",POP,fmt[0][1..]); break;
	case 'x': res+=sprintf("%x%s",POP,fmt[0][1..]); break;
	case '0': case '2': case '3':
	  sscanf(fmt[0],"%[0-9]%s",tmp,fmt[0]);
	  res+=sprintf("%"+tmp+fmt[0][..0]+"%s",POP,fmt[0][1..]);
	  break;
	case 'c': res+=sprintf("%c%s",POP,fmt[0][1..]); break;
	case 's': res+=sprintf("%s%s",POP,fmt[0][1..]); break;
	  
	case '\'': 
	  sscanf(fmt[0],"'%s'%s",tmp,fmt[0]);
	  if (tmp=="") tmp="\0";
	  if (tmp[0]=='\\') tmp=sprintf("%c",(int)("0"+tmp[1..]));
	  PUSH(tmp[0]);
	  res+=fmt[0];
	  break;
	case '{': 
	  sscanf(fmt[0],"{%d}%s",z,fmt[0]);
	  res+=fmt[0];
	  PUSH(z);
	  break;
	case 'p':
	  PUSH(args0[fmt[0][1]-'1']);
	  res+=fmt[0][2..];
	  break;
	case 'P':
	  var[fmt[0][1]]=POP;
	  res+=fmt[0][2..];
	  break;
	case 'g':
	  PUSH(var[fmt[0][1]]);
	  res+=fmt[0][2..];
	  break;
	case 'i':
	  args[0]+=1;
	  args[1]+=1;
	  break;
	case '+': PUSH(POP+POP); res+=fmt[0][1..]; break;
	case '-': PUSH(POP-POP); res+=fmt[0][1..]; break;
	case '*': PUSH(POP*POP); res+=fmt[0][1..]; break;
	case '/': PUSH(POP/POP); res+=fmt[0][1..]; break;
	case 'm': PUSH(POP%POP); res+=fmt[0][1..]; break;
	case '&': PUSH(POP&POP); res+=fmt[0][1..]; break;
	case '|': PUSH(POP|POP); res+=fmt[0][1..]; break;
	case '^': PUSH(POP^POP); res+=fmt[0][1..]; break;
	case '=': PUSH(POP==POP); res+=fmt[0][1..]; break;
	case '>': PUSH(POP>POP); res+=fmt[0][1..]; break;
	case '<': PUSH(POP<POP); res+=fmt[0][1..]; break;
	case 'A': PUSH(POP && POP); res+=fmt[0][1..]; break;
	case 'O': PUSH(POP || POP); res+=fmt[0][1..]; break;
	case '!': PUSH(!POP); res+=fmt[0][1..]; break;
	case '~': PUSH(~POP); res+=fmt[0][1..]; break;
	case '?':
	  error("Sorry, Terminal can't handle if-else's\n");
	default:
	  error("Unknown opcode: %%%s\n",fmt[0][..0]);
	}
      }
    return res;
  }

  string tgoto(string cap, int col, int row)
  {
    return tparam(cap, col, row);
  }

  string tputs(string s)
  {
    return s;
  }

  string put(string cap, mixed ... args)
  {
    string str = tgetstr(cap);
    string tstr = str && tparam(str, @args);
    return tstr && tputs(tstr);
  }

}

//! Termcap terminal description object.
class Termcap {

  inherit TermMachine;

  //!
  array(string) aliases;

  static Termcap parent;

  //! Put termcap string
  string tputs(string s)
  {
    // Delay stuff completely ignored...
    sscanf(s, "%*d%s", s);
    return s;
  }

  private static multiset(string) load_cap(string en)
  {
    string br=":";
    int i=search(en,":");
    int j=search(en,",");
    multiset(string) clears = (<>);

    if (i==-1) { i=j; br=","; }
    else if (j!=-1) { i=min(i,j); if (i==j) br=","; }
    if (i<1)
      error("Termcap: Unparsable entry\n");
    aliases=en[..i-1]/"|";
    en=en[i..];
    
    while (en!="")
    {
      string name;
      string data;
      if(sscanf(en,"%*[ \t]%[a-zA-Z_0-9&]%s"+br+"%s",name,data,en) < 4)
      {	
	sscanf(en,"%*[ \t]%[a-zA-Z_0-9&]%s",name,data);
	en="";
      }
      
      if (data=="") // boolean
      {
	if (name!="") map[name]=1;
      }
      else if (data[0]=='@') // boolean off
      {
	clears[name]=1;
      }
      else if (data[0]=='#') // number
      {
	int z;
	sscanf(data,"#%d",z);
	map[name]=z;
      }
      else if (data[0]=='=') // string
      {
	data=data[1..];
	while (sizeof(data) && data[-1]=='\\')
	{
	  string add;
	  if (sscanf(en,"%s"+br+"%s",add,en)<2) break;
	  data+="\\"+add;
	}
	
	data=replace(data,"\\^","\\*");
	
	if (has_value(data, "^"))
	  data=replace(data,ctrlcharsfrom,ctrlcharsto);
	
	data = replace(data,
		  ({"\\E","\\e","\\n","\\r","\\t","\\b","\\f",
		    "\\*","\\\\","\\,","\\:","#",
		    "\\0","\\1","\\2","\\3","\\4","\\5","\\6","\\7"}),
		  ({"\033","\033","\n","\r","\t","\b","\f",
		    "^","\\",",",":","#!",
		    "#0","#1","#2","#3","#4","#5","#6","#7"}));

	array(string) parts = data/"#";
	data = parts[0];
	foreach (parts[1..], string p)
	  if (sizeof(p) && p[0]=='!')
	    data += "#"+p[1..];
	  else
	  {
	    int n;
	    string x;
	    if(2==sscanf(p[..2], "%o%s", n, x))
	      data += sprintf("%c%s%s", n, x, p[3..]);
	    else
	      data += p;
	  }

	map[name]=data;
	
      }
      else // weird
      {
	// ignore
      }
    }

    return clears;
  }

  //!
  void create(string cap, TermcapDB|void tcdb, int|void maxrecurse)
  {
    int i=0;
    while((i=search(cap, "\\\n", i))>=0) {
      string capr;
      if(2!=sscanf(cap[i..], "\\\n%*[ \t\r]%s", capr))
	break;
      cap = cap[..i-1]+capr;
    }
    multiset(string) clears = load_cap(cap);
    if(map->tc) {
      if(maxrecurse==1)
	error("Termcap: maximum inheritance depth exceeded (loop?)\n");
      parent = (tcdb||defaultTermcapDB())->
	load(map->tc, maxrecurse? (maxrecurse-1):25);
      if(!parent)
	error("Termcap: can't find parent terminal \"%s\"\n", map->tc);
      map = parent->map | map;
    }
    map |= mkmapping(indices(clears), allocate(sizeof(clears)));
  }
}


//! Terminfo terminal description object
class Terminfo {

  inherit TermMachine;

  //!
  array(string) aliases;

  static private constant boolnames =
  ({ "bw","am","xb","xs","xn","eo","gn","hc","km","hs","in","da","db","mi",
     "ms","os","es","xt","hz","ul","xo","nx","5i","HC","NR","NP","ND","cc",
     "ut","hl","YA","YB","YC","YD","YE","YF","YG" });
  static private constant numnames =
  ({ "co","it","li","lm","sg","pb","vt","ws","Nl","lh","lw","ma","MW","Co",
     "pa","NC","Ya","Yb","Yc","Yd","Ye","Yf","Yg","Yh","Yi","Yj","Yk","Yl",
     "Ym","Yn","BT","Yo","Yp" });
  static private constant strnames =
  ({ "bt","bl","cr","cs","ct","cl","ce","cd","ch","CC","cm","do","ho","vi",
     "le","CM","ve","nd","ll","up","vs","dc","dl","ds","hd","as","mb","md",
     "ti","dm","mh","im","mk","mp","mr","so","us","ec","ae","me","te","ed",
     "ei","se","ue","vb","ff","fs","i1","is","i3","if","ic","al","ip","kb",
     "ka","kC","kt","kD","kL","kd","kM","kE","kS","k0","k1","k;","k2","k3",
     "k4","k5","k6","k7","k8","k9","kh","kI","kA","kl","kH","kN","kP","kr",
     "kF","kR","kT","ku","ke","ks","l0","l1","la","l2","l3","l4","l5","l6",
     "l7","l8","l9","mo","mm","nw","pc","DC","DL","DO","IC","SF","AL","LE",
     "RI","SR","UP","pk","pl","px","ps","pf","po","rp","r1","r2","r3","rf",
     "rc","cv","sc","sf","sr","sa","st","wi","ta","ts","uc","hu","iP","K1",
     "K3","K2","K4","K5","pO","rP","ac","pn","kB","SX","RX","SA","RA","XN",
     "XF","eA","LO","LF","@1","@2","@3","@4","@5","@6","@7","@8","@9","@0",
     "%1","%2","%3","%4","%5","%6","%7","%8","%9","%0","&1","&2","&3","&4",
     "&5","&6","&7","&8","&9","&0","*1","*2","*3","*4","*5","*6","*7","*8",
     "*9","*0","#1","#2","#3","#4","%a","%b","%c","%d","%e","%f","%g","%h",
     "%i","%j","!1","!2","!3","RF","F1","F2","F3","F4","F5","F6","F7","F8",
     "F9","FA","FB","FC","FD","FE","FF","FG","FH","FI","FJ","FK","FL","FM",
     "FN","FO","FP","FQ","FR","FS","FT","FU","FV","FW","FX","FY","FZ","Fa",
     "Fb","Fc","Fd","Fe","Ff","Fg","Fh","Fi","Fj","Fk","Fl","Fm","Fn","Fo",
     "Fp","Fq","Fr","cb","MC","ML","MR","Lf","SC","DK","RC","CW","WG","HU",
     "DI","QD","TO","PU","fh","PA","WA","u0","u1","u2","u3","u4","u5","u6",
     "u7","u8","u9","op","oc","Ic","Ip","sp","Sf","Sb","ZA","ZB","ZC","ZD",
     "ZE","ZF","ZG","ZH","ZI","ZJ","ZK","ZL","ZM","ZN","ZO","ZP","ZQ","ZR",
     "ZS","ZT","ZU","ZV","ZW","ZX","ZY","ZZ","Za","Zb","Zc","Zd","Ze","Zf",
     "Zg","Zh","Zi","Zj","Zk","Zl","Zm","Zn","Zo","Zp","Zq","Zr","Zs","Zt",
     "Zu","Zv","Zw","Zx","Zy","Km","Mi","RQ","Gm","AF","AB","xl","dv","ci",
     "s0","s1","s2","s3","ML","MT","Xy","Zz","Yv","Yw","Yx","Yy","Yz","YZ",
     "S1","S2","S3","S4","S5","S6","S7","S8","Xh","Xl","Xo","Xr","Xt","Xv",
     "sA","sL" });

  //! @fixme
  //!  Document this function
  string tputs(string s)
  {
    // Delay stuff completely ignored...
    string pre, post;
    while (3==sscanf(s, "%s$<%*[0-9.]>%s", pre, post))
      s = pre+post;
    return s;
  }

  static private string swab(string s)
  {
    return predef::map(s/2, reverse)*"";
  }

  static private int load_cap(.File f, int|void bug_compat)
  {
    int magic, sname, nbool, nnum, nstr, sstr;

    if (6!=sscanf(swab(f->read(12)), "%2c%2c%2c%2c%2c%2c",
		  magic, sname, nbool, nnum, nstr, sstr) ||
	magic != 0432)
      return 0;
    aliases = (f->read(sname)-"\0")/"|";
    {
      int blen = nbool;
      if((nbool&1) && !bug_compat)
	blen++;
      array(int) bools = values(f->read(blen)[..nbool-1]);
      if (sizeof(bools)>sizeof(boolnames))
	bools = bools[..sizeof(boolnames)-1];
      map = mkmapping(boolnames[..sizeof(bools)-1], bools);
    }
    {
      array(int) nums = [array(int)]
	array_sscanf(swab(f->read(nnum*2)), "%2c"*nnum);
      if (sizeof(nums)>sizeof(numnames))
	nums = nums[..sizeof(numnames)-1];
      mapping(string:int) tmp = mkmapping(numnames[..sizeof(nums)-1], nums);
      foreach (numnames[..sizeof(nums)-1], string name)
	if (tmp[name]>=0xfffe)
	  m_delete(tmp, name);
      map += tmp;
    }
    {
      string stroffs = swab(f->read(nstr*2));
      string strbuf = f->read(sstr);
      if(sizeof(strbuf)==sstr-1 && !bug_compat && (nbool&1)) {
	// Ugh.  Someone didn't pad their bool array properly (one suspects).
	f->seek(0);
	return load_cap(f, 1);
      }
      if(sizeof(strbuf)!=sstr)
	return 0;
      array(string) strarr = predef::map(array_sscanf(stroffs, "%2c"*nstr),
                                         lambda(int offs, string buf) {
                                           return offs<0xfffe &&
                                             offs<sizeof(buf) &&
                                             buf[offs..
                                                 search(buf, "\0", offs)-1];
                                         }, strbuf+"\0");
      if (sizeof(strarr)>sizeof(strnames))
	strarr = strarr[..sizeof(strnames)-1];
      mapping(string:string) tmp = mkmapping(strnames[..sizeof(strarr)-1],
					     strarr);
      foreach (strnames[..sizeof(strarr)-1], string name)
	if (!tmp[name])
	  m_delete(tmp, name);
      map += tmp;
    }
    return 1;
  }

  //!
  void create(string filename)
  {
    .File f = .File();
    if (!f->open(filename, "r"))
	error("Terminfo: unable to open terminfo file \"%s\"\n", filename);
    int r = load_cap(f);
    f->close();
    if (!r)
      error("Terminfo: unparsable terminfo file \"%s\"\n", filename);
  }
}

class TermcapDB {

  MUTEX

  static private inherit .File;

  static private string buf="";
  static private mapping(string:int|Termcap) cache=([]);
  static private int complete_index=0;

  void create(string|void filename)
  {
    if (!filename) {
      string tce = [string]getenv("TERMCAP");
      if (tce && sizeof(tce) && tce[0]=='/')
	filename = tce;
      else if ((getenv("OSTYPE") == "msys") &&
	       (filename = getenv("SHELL"))) {
	// MinGW
	// Usually something like "C:/msys/1.0/bin/sh"
	// Termcap is in "C:/msys/1.0/etc/termcap"
	filename = combine_path(filename, "../../etc/termcap");
      }
      else
	filename = "/etc/termcap";
    }
    if (!::open(filename, "r")) {
      error("failed to open termcap file %O\n", filename);
    }
  }

  static private void rewind(int|void pos)
  {
    ::seek(pos);
    buf="";
  }

  static private int more_data()
  {
    string q;
    q=::read(8192);
    if (q=="" || !q) return 0;
    buf+=q; 
    return 1;
  }

  static private array(string) get_names(string cap)
  {
    sscanf(cap, "%s:", cap);
    sscanf(cap, "%s,", cap);
    return cap/"|";
  }

  static private string read()
  {
    int i, st;
    string res="";
    for (;;)
    {
      if (buf=="" && !more_data()) return 0; // eof
      
      sscanf(buf,"%*[ \t\r\n]%s",buf);
      if (buf=="") continue;
      
      if (buf[0]=='#') // comment, scan to newline
      {
	while ((i=search(buf,"\n"))<0)
	{
	  // The rest of the buffer is comment, toss it and read more
	  buf="";
	  if(!more_data()) return 0; // eof
	}
	buf=buf[i+1..];
	continue;
      }

      break;
    }

    st = ::tell()-sizeof(buf);

    while ((i=search(buf, "\n"))<0)
    {
      if (!more_data()) return 0; // eof
    }
    
    while (buf[i-1]=='\\')
    {
      res+=buf[..i-2];
      buf=buf[i+1..];
      while (sscanf(buf,"%*[ \t\r]%s",buf)<2 || !sizeof(buf))
	if (!more_data()) {
	  buf = "";
	  return res; // eof, or illegal... weird
	}
      while ((i=search(buf, "\n"))<0)
      {
	if (!more_data()) return 0; // eof
      }
    }
    
    res+=buf[..i-1];
    buf=buf[i+1..];

    foreach(get_names(res), string name)
      if(!objectp(cache[name]))
	cache[name]=st;

    return res;
  }

  static private string readat(int pos)
  {
    rewind(pos);
    return read();
  }

  array(string) _indices()
  {
    LOCK;
    if(!complete_index) {
      rewind();
      while(read());
      complete_index = 1;
    }
    UNLOCK;
    return sort(indices(cache));
  }

  array(Termcap) _values()
  {
    array(object|int) res = ({});
    mapping(int:string) extra = ([]);
    LOCK;
    if (complete_index)
      res = predef::map(sort(indices(cache)),
                        [function(string,mapping(int:string):Termcap)]
                        lambda(string name, mapping(int:string) extra) {
                          if (!objectp(cache[name]) && !extra[cache[name]])
                            extra[cache[name]] = readat(cache[name]);
                          return cache[name];
                        }, extra);
    else {
      array(string) resi = ({});
      string cap;
      int i = 1;
      rewind();
      while ((cap = read())) {
	array(string) names = get_names(cap);
	object|int o = objectp(cache[names[0]]) && cache[names[0]];
	if (!o)
	{
	  o = i++;
	  extra[o] = cap;
	}
	res += ({ o }) * sizeof(names);
	resi += names;
      }
      sort(resi, res);
      complete_index = 1;
    }
    UNLOCK;
    return [array(Termcap)]
      predef::map(res,
                  lambda(int|Termcap x, mapping(int:Termcap) y) {
                    return objectp(x)? x : y[x];
                  },
                  mkmapping(indices(extra),
                            predef::map(values(extra),
                                        Termcap, this)));
  }

  static private string read_next(string find) // quick search
  {
    for (;;)
    {
      int i, j;

      if (buf=="" && !more_data()) return 0; // eof
      
      i=search(buf,find);
      if (i!=-1)
      {
	int j=i;
	while (j>=0 && buf[j]!='\n') j--; // find backwards
	
	if (buf!="" && buf[j+1]!='#')  // skip comments
	{
	  buf=buf[j+1..];
	  return read();
	}

	while ((i=search(buf,"\n",j+1))<0)
	  if (!more_data()) return 0; // eof

	buf = buf[i+1..];
	
	continue;
      }
      for(j=-1; (i=search(buf,"\n",j+1))>=0; j=i);
      buf = buf[j+1..];
      if (!more_data()) return 0; // eof
    }
  }

  Termcap load(string term, int|void maxrecurse)
  {
    int|string|Termcap cap;

    LOCK;
    if (zero_type(cache[term]))
    {
      if (!complete_index)
      {
	rewind();
	do
	  cap = read_next(term);
	while(cap && search(get_names(cap), term)<0);
      }
    }
    else if (intp(cap=cache[term])) {
      rewind(cap);
      cap = read();
    }
    UNLOCK;
    if (stringp(cap))
    {
      array(string) names = get_names(cap);
      if ((cap = Termcap(cap, this, maxrecurse)))
      {
	LOCK;
	foreach(names, string name)
	  cache[name] = [object(Termcap)]cap;
	UNLOCK;
      }
    }
    return objectp(cap) && [object(Termcap)]cap;
  }

  Termcap `[](string name)
  {
    return load(name);
  }
}


class TerminfoDB {

  MUTEX

  static private string dir;
  static private mapping(string:Terminfo) cache = ([]);
  static private int complete_index=0;

  void create(string|void dirname)
  {
    if (!dirname)
    {
      foreach (({"/usr/share/lib/terminfo", "/usr/share/terminfo",
		 "/usr/share/termcap",
		 "/usr/lib/terminfo", "/usr/share/misc/terminfo"}), string dn)
      {
	.Stat s = file_stat(dn);
	if (s && s->type=="dir")
	{
	  dirname = dn;
	  break;
	}
      }
      if (!dirname) {
	destruct(this);
	return;
      }
    }

    if(sizeof(dirname)<1 || dirname[-1]!='/')
      dirname += "/";

    if (!get_dir(dir = dirname))
      error("failed to read terminfo dir %O\n", dirname);
  }

  array(string) _indices()
  {
    LOCK;
    if (!complete_index) {
      array(string) files;
      foreach (get_dir(dir), string a)
	if (sizeof(a) == 1)
	  foreach (get_dir(dir+a), string b)
	    if(zero_type(cache[b]))
	      cache[b] = 0;
      complete_index = 1;
    }
    UNLOCK;
    return sort(indices(cache));
  }

  array(object) _values()
  {
    return predef::map(_indices(),
                       [function(string:object(Terminfo))]
                       lambda(string name) {
                         return cache[name] ||
                           Terminfo(dir+name[..0]+"/"+name);
                       });
  }

  Terminfo load(string term)
  {
    Terminfo ti;

    if (!sizeof(term))
      return 0;
    LOCK;
    if (!(ti = cache[term]))
    {
      if (file_stat(dir+term[..0]+"/"+term)) {
	// Traditional Terminfo layout.
	ti = Terminfo(dir+term[..0]+"/"+term);
      } else if (file_stat(sprintf("%s%02x/%s", dir, term[0], term))) {
	// MacOS X Terminfo layout.
	ti = Terminfo(sprintf("%s%02x/%s", dir, term[0], term));
      }
      if (ti)
	cache[term] = ti;
    }
    UNLOCK;
    return ti;
  }

  Terminfo `[](string name)
  {
    return load(name);
  }
}

static private Termcap defterm;
static private TermcapDB deftermcap;
static private TerminfoDB defterminfo;

TermcapDB defaultTermcapDB()
{
  TermcapDB tcdb;
  LOCK;
  catch { tcdb = deftermcap || (deftermcap = TermcapDB()); };
  UNLOCK;
  return tcdb;
}

TerminfoDB defaultTerminfoDB()
{
  TerminfoDB tidb;
  LOCK;
  catch { tidb = defterminfo || (defterminfo = TerminfoDB()); };
  UNLOCK;
  return tidb;
}

//! Returns the terminal description object for @[term] from the
//! systems termcap database. Returns 0 if not found.
//!
//! @seealso
//!  Stdio.Terminfo.getTerm, Stdio.Terminfo.getTerminfo
Termcap getTermcap(string term)
{
  TermcapDB tcdb = defaultTermcapDB();
  return tcdb && tcdb[term];
}

//! Returns the terminal description object for @[term] from the
//! systems terminfo database. Returns 0 if not found.
//!
//! @seealso
//!  Stdio.Terminfo.getTerm, Stdio.Terminfo.getTermcap
Terminfo getTerminfo(string term)
{
  TerminfoDB tidb = defaultTerminfoDB();
  return tidb && tidb[term];
}

//! Returns an object describing the terminal term. If term is not specified, it will
//! default to @[getenv("TERM")] or if that fails to "dumb".
//!
//! Lookup of terminal information will first be done in the systems terminfo
//! database, and if that fails in the termcap database. If neither database exists, a
//! hardcoded entry for "dumb" will be used.
//!
//! @seealso
//!  Stdio.Terminfo.getTerminfo, Stdio.Terminfo.getTermcap, Stdio.getFallbackTerm
Termcap getTerm(string|void term)
{
  if (!term) {
    Termcap t = defterm;
    if (!t)
    {
      string tc = [string]getenv("TERMCAP");
      if (mixed err = catch {
	t = tc && sizeof(tc) && tc[0] != '/' && Termcap(tc);
      })
	werror("%s", describe_backtrace(err));
      if (!t)
	t = getTerm(getenv("TERM") || "dumb");
      LOCK;
      if (!defterm)
	defterm = t;
      UNLOCK;
    }
    return t;
  }
  return getTerminfo(term) || getTermcap(term) || getFallbackTerm(term);
}

//! Returns an object describing the fallback terminal for the terminal
//! @[term]. This is usually equvivalent to @[Stdio.Terminfo.getTerm("dumb")].
//!
//! @seealso
//!  Stdio.Terminfo.getTerm
static Termcap getFallbackTerm(string term)
{
  return (term=="dumb"? Termcap("dumb:\\\n\t:am:co#80:do=^J:") :
	  getTerm("dumb"));
}

static int is_tty_cache;

int is_tty()
//! Returns 1 if @[Stdio.stdin] is connected to an interactive
//! terminal that can handle backspacing, carriage return without
//! linefeed, and the like.
{
  if(!is_tty_cache)
  {
#ifdef __NT__
    is_tty_cache=1;
#else
    is_tty_cache=!!.stdin->tcgetattr();
#endif
    if(!is_tty_cache)
    {
      is_tty_cache=-1;
    }else{
      switch(getenv("TERM"))
      {
        case "dumb":
        case "emacs":
          is_tty_cache=-1;
      }
    }
  }
  return is_tty_cache>0;
}
