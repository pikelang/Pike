// $Id: Readline.pike,v 1.55 2004/02/26 21:51:17 agehall Exp $
#pike __REAL_VERSION__

//!
//! @fixme
//!   Ought to have support for charset conversion.
class OutputController
{
  static private .File outfd;
  static private .Terminfo.Termcap term;
  static private int xpos = 0, columns = 0;
  static private mapping oldattrs = 0;

#define BLINK     1
#define BOLD      2
#define DIM       4
#define REVERSE   8
#define ITALIC    16
#define STANDOUT  32
#define UNDERLINE 64

  static private int selected_attributes = 0, needed_attributes = 0;
  static private int active_attributes = 0;

  static int low_attribute_mask(array(string) atts)
  {
    return `|(@rows(([
      "blink":BLINK,
      "bold":BOLD,
      "dim":DIM,
      "reverse":REVERSE,
      "italic":ITALIC,
      "standout":STANDOUT,
      "underline":UNDERLINE
    ]), atts));
  }

  static void low_set_attributes(int mask, int val, int|void temp)
  {
    int remv = mask & selected_attributes & ~val;
    string s = "";

    if(remv & 15) {
      s += term->put("me")||"";
      needed_attributes |= selected_attributes;
      active_attributes = 0;
    }
    if(temp) {
      needed_attributes |= remv;
    } else {
      selected_attributes &= ~remv;
      needed_attributes &= ~remv;
    }
    active_attributes &= ~remv;
    for(int i=0; remv; i++)
      if(remv & (1<<i)) {
	string cap = ({0,0,0,0,"ZR","se","ue"})[i];
	if(cap && (cap = term->put(cap)))
	  s += cap;
	remv &= ~(1<<i);
      }
    if(sizeof(s))
      outfd->write(s);
    int add = mask & val & ~selected_attributes;
    selected_attributes |= add;
    needed_attributes |= add;
  }

  static void low_disable_attributes()
  {
    low_set_attributes(active_attributes, 0, 1);
  }

  static void low_enable_attributes()
  {
    int i, add = needed_attributes;
    string s = "";

    needed_attributes &= ~add;
    for(i=0; add; i++)
      if(add & (1<<i)) {
	string cap = ({"mb","md","mh","mr","ZH","so","us"})[i];
	if(cap && (cap = term->put(cap))) {
	  s += cap;
	  active_attributes |= i;
	}
	add &= ~(1<<i);
      }
    if(sizeof(s))
      outfd->write(s);
  }

  //! Set the provided attributes to on.
  void turn_on(string ... atts)
  {
    low_set_attributes(low_attribute_mask(atts), ~0);
  }

  //! Set the provided attributes to off.
  void turn_off(string ... atts)
  {
    low_set_attributes(low_attribute_mask(atts), 0);
  }

  //!
  void disable()
  {
    catch{
      if(oldattrs)
	outfd->tcsetattr((["OPOST":0,"ONLCR":0,"OCRNL":0,
			   "OLCUC":0,"OFILL":0,"OFDEL":0,
			   "ONLRET":0,"ONOCR":0])&oldattrs);
      else
	outfd->tcsetattr((["OPOST":1]));
    };
  }

  //!
  void enable()
  {
    if(term->put("cr") && term->put("do"))
      catch { outfd->tcsetattr((["OPOST":0])); };
    else
      catch { outfd->tcsetattr((["OPOST":1,"ONLCR":1,"OCRNL":0,"OLCUC":0,
				 "OFILL":1,"OFDEL":0,"ONLRET":0,"ONOCR":0]));};
  }

  static void destroy()
  {
    disable();
  }

  //! Check and return the terminal width.
  //!
  //! @note
  //!   In Pike 7.4 and earlier this function returned @expr{void@}.
  //!
  //! @seealso
  //!   @[get_number_of_columns]
  int check_columns()
  {
    catch {
      int c = outfd->tcgetattr()->columns;
      if(c)
	columns = c;
    };
    if(!columns)
      columns = term->tgetnum("co") || 80;
    return columns;
  }

  //! Returns the width of the terminal.
  //! @note
  //!   Does not check the width of the terminal.
  //! @seealso
  //!   @[check_columns]
  int get_number_of_columns()
  {
    return columns;
  }

  static string escapify(string s, void|int hide)
  {
#if 1
    s=replace(s,
	      ({
		"\000","\001","\002","\003","\004","\005","\006","\007",
		  "\010","\011","\012","\013","\014","\015","\016","\017",
		  "\020","\021","\022","\023","\024","\025","\026","\027",
		  "\030","\031","\032","\033","\034","\035","\036","\037",
		  "\177",
		  "\200","\201","\202","\203","\204","\205","\206","\207",
		  "\210","\211","\212","\213","\214","\215","\216","\217",
		  "\220","\221","\222","\223","\224","\225","\226","\227",
		  "\230","\231","\232","\233","\234","\235","\236","\237",
		  }),
	      ({
		"^@","^A","^B","^C","^D","^E","^F","^G",
		  "^H","^I","^J","^K","^L","^M","^N","^O",
		  "^P","^Q","^R","^S","^T","^U","^V","^W",
		  "^X","^Y","^Z","^[","^\\","^]","^^","^_",
		  "^?",
		  "~@","~A","~B","~C","~D","~E","~F","~G",
		  "~H","~I","~J","~K","~L","~M","~N","~O",
		  "~P","~Q","~R","~S","~T","~U","~V","~W",
		  "~X","~Y","~Z","~[","~\\","~]","~^","~_",
		  }));
    return hide ? "*"*sizeof(s) : s;
#else

  for(int i=0; i<sizeof(s); i++)
      if(s[i]<' ')
	s = s[..i-1]+sprintf("^%c", s[i]+'@')+s[i+1..];
      else if(s[i]==127)
	s = s[..i-1]+"^?"+s[i+1..];
      else if(s[i]>=128 && s[i]<160)
	s = s[..i-1]+sprintf("~%c", s[i]-128+'@')+s[i+1..];
    return s;
#endif
  }


  static int width(string s)
  {
    return sizeof(s);
  }

  static int escapified_width(string s)
  {
    return width(escapify(s));
  }

  //!
  void low_write(string s, void|int word_break)
  {
    int n = width(s);
    if(!n)
      return;

    if(needed_attributes)
      low_enable_attributes();

//    werror("low_write(%O)\n",s);

    if(word_break)
    {
      while(xpos+n>=columns)
      {
	int l = columns-xpos;
	string line=s[..l-1];
	int spos=search(reverse(line)," ");
	if(spos==-1)
	{
	  outfd->write(line);
	}else{
	  l=sizeof(line)-spos;
	  outfd->write(line[..l-2]);
	}
	s=s[l..];
	n-=l;
	xpos+=l;
	if(xpos<columns || !term->tgetflag("am"))
	  outfd->write((term->put("cr")||"")+(term->put("do")||"\n"));
	xpos = 0;
      }
    }else{
      while(xpos+n>=columns)
      {
	int l = columns-xpos;
	outfd->write(s[..l-1]);
	s=s[l..];
	n-=l;
	xpos = 0;
	if(!term->tgetflag("am"))
	  outfd->write((term->put("cr")||"")+(term->put("do")||"\n"));
      }
    }
    string le;
    if(n>0) {
      outfd->write(s);
      xpos += n;
    } else if(xpos==0 && term->tgetflag("am") && (le=term->put("le")))
      outfd->write(" "+le);
  }

  //!
  void write(string s,void|int word_break,void|int hide)
  {
    low_write(escapify(s,hide),word_break);
  }

  //!
  void low_move_downward(int n)
  {
    if(n<=0)
      return;
    if(active_attributes && !term->tgetflag("ms"))
      low_disable_attributes();
    outfd->write(term->put("DO", n) || (term->put("do")||"")*n);
  }

  //!
  void low_move_upward(int n)
  {
    if(n<=0)
      return;
    if(active_attributes && !term->tgetflag("ms"))
      low_disable_attributes();
    outfd->write(term->put("UP", n) || (term->put("up")||"")*n);
  }

  //!
  void low_move_forward(int n)
  {
    if(n<=0)
      return;
    if(active_attributes && !term->tgetflag("ms"))
      low_disable_attributes();
    if(xpos+n<columns) {
      outfd->write(term->put("RI", n) ||
		   (term->put("nd")||term->put("ri")||"")*n);
      xpos += n;
    } else {
      int l = (xpos+n)/columns;
      low_move_downward(l);
      n -= l*columns;
      if(n<0)
	low_move_backward(-n);
      else if(n>0)
	low_move_forward(n);
    }
  }

  //!
  void low_move_backward(int n)
  {
    if(n<=0)
      return;
    if(active_attributes && !term->tgetflag("ms"))
      low_disable_attributes();
    if(xpos-n>=0) {
      outfd->write(term->put("LE", n) || (term->put("le")||term->put("kb")||"\10")*n);
      xpos -= n;
    } else {
      int l = 1+(n-xpos-1)/columns;
      low_move_upward(l);
      n -= l*columns;
      if(n<0)
	low_move_forward(-n);
      else if(n>0)
	low_move_backward(n);
    }
  }

  //!
  void low_erase(int n)
  {
    string e = term->put("ec", n);
    if(active_attributes && !term->tgetflag("ms"))
      low_disable_attributes();
    if (e)
      outfd->write(e);
    else
    {
      low_write(" "*n);
      low_move_backward(n);
    }
  }

  //!
  void move_forward(string s)
  {
    low_move_forward(escapified_width(s));
  }

  //!
  void move_backward(string s)
  {
    low_move_backward(escapified_width(s));
  }

  //!
  void erase(string s)
  {
    low_erase(escapified_width(s));
  }

  //!
  void newline()
  {
    string cr = term->put("cr");
    // NOTE: Use "sf" in preference to "do" since "do" for "xterm" on HPUX
    //       is "\33[B", which doesn't scroll when at the last line.
    string down = term->put("sf") || term->put("do");
    if(active_attributes && !term->tgetflag("ms"))
      low_disable_attributes();
    if(cr && down)
      outfd->write(cr+down);
    else
      // In this case we have ONLCR (hopefully)
      outfd->write("\n");
    xpos = 0;
  }

  //!
  void bol()
  {
    if(active_attributes && !term->tgetflag("ms"))
      low_disable_attributes();
    outfd->write(term->put("cr")||"");
    xpos = 0;
  }

  //!
  void clear(int|void partial)
  {
    string s;
    if(active_attributes)
      low_disable_attributes();
    if(!partial && (s = term->put("cl"))) {
      outfd->write(s);
      xpos = 0;
      return;
    }
    if(!partial) {
      outfd->write(term->put("ho")||term->put("cm", 0, 0)||"");
      xpos = 0;
    }
    outfd->write(term->put("cd")||"");
  }

  //!
  void beep()
  {
    outfd->write(term->put("bl")||"");
  }

  //!
  void create(.File|void _outfd,
	      .Terminfo.Termcap|string|void _term)
  {
    outfd = _outfd || Stdio.File( "stdout", "w" );
    term = objectp(_term)? _term : .Terminfo.getTerm(_term);
    catch { oldattrs = outfd->tcgetattr(); };
    check_columns();
  }
}

//!
//! @fixme
//!   Ought to have support for charset conversion.
class InputController
{
  static private object infd, term;
  static private int enabled = -1;
  static private function(:int) close_callback = 0;
  static private string prefix="";
  static private mapping(int:function|mapping(string:function)) bindings=([]);
  static private function grab_binding = 0;
  static private mapping oldattrs = 0;

  int dumb=0;

  static void destroy()
  {
    catch{ infd->set_blocking(); };
    if(dumb)
      return;
    catch{ infd->tcsetattr((["ECHO":1,"ICANON":1])); };
    catch{ if(oldattrs) infd->tcsetattr((["ECHO":0,"ICANON":0,"VEOF":0,
					  "VEOL":0,"VLNEXT":0])&oldattrs); };
  }

  static private string process_input(string s)
  {
    int i;

    for (i=0; i<sizeof(s); i++)
    {
      if (!enabled)
	return s[i..];
      function|mapping(string:function) b = grab_binding || bindings[s[i]];
      grab_binding = 0;
      if (!b)
	/* do nothing */;
      else if(mappingp(b)) {
	int ml = 0, l = sizeof(s)-i;
	string m;
	foreach (indices(b), string k)
	{
	  if (sizeof(k)>l && k[..l-1]==s[i..])
	    /* Valid prefix, need more data */
	    return s[i..];
	  else if (sizeof(k) > ml && s[i..i+sizeof(k)-1] == k)
	  {
	    /* New longest match found */
	    ml = sizeof(m = k);
	  }
	}
	if (ml)
	{
	  i += ml-1;
	  b[m](m);
	}
      } else
	b(s[i..i]);
    }
    return "";
  }

  static private void read_cb(mixed _, string s)
  {
    if (!s || s=="")
      return;
    if (sizeof(prefix))
    {
      s = prefix+s;
      prefix = "";
    }
    prefix = process_input(s);
  }

  static private void close_cb()
  {
    if (close_callback && close_callback())
      return;
    destruct(this);
  }

  static private int set_enabled(int e)
  {
    if (e != enabled)
    {
      enabled = e;
      if (enabled)
      {
	string oldprefix = prefix;
	prefix = "";
	prefix = process_input(oldprefix);
	if ((!infd->set_read_callback || !infd->set_close_callback) && infd->set_nonblocking)
	  infd->set_nonblocking( read_cb, 0, close_cb );
	else {
	  infd->set_read_callback( read_cb );
	  infd->set_close_callback( close_cb );
	}
      }
      else
	infd->set_blocking();
      return !enabled;
    }
    else
      return enabled;
  }

  //!
  int isenabled()
  {
    return enabled;
  }

  //!
  int enable(int ... e)
  {
    if (sizeof(e))
      return set_enabled(!!e[0]);
    else
      return set_enabled(1);
  }

  //!
  int disable()
  {
    return set_enabled(0);
  }

  //!
  int run_blocking()
  {
    disable();
    string data = prefix;
    prefix = "";
    enabled = 1;
    for (;;)
    {
      prefix = process_input(data);
      if (!enabled)
	return 0;
      data = prefix+infd->read(1024, 1);
      prefix = "";
      if(!data || !sizeof(data)) {
	disable();
	return -1;
      }
    }
  }

  //!
  void set_close_callback(function (:int) ccb)
  {
    close_callback = ccb;
  }

  //! Clears the bindings.
  void nullbindings()
  {
    bindings = ([]);
  }

  //!
  void grabnextkey(function g)
  {
    if(functionp(g))
      grab_binding = g;
  }

  //!
  function bindstr(string str, function f)
  {
    function oldf = 0;
    if (mappingp(f))
      f = 0; // Paranoia
    switch (sizeof(str||""))
    {
    case 0:
      break;
    case 1:
      if (mappingp(bindings[str[0]]))
      {
	oldf = bindings[str[0]][str];
	if (f)
	  bindings[str[0]][str] = f;
	else
	  m_delete(bindings[str[0]], str);
      } else {
	oldf = bindings[str[0]];
	if (f)
	  bindings[str[0]] = f;
	else
	  m_delete(bindings, str[0]);
      }
      break;
    default:
      if (mappingp(bindings[str[0]]))
	oldf = bindings[str[0]][str];
      else
	bindings[str[0]] =
	  bindings[str[0]]? ([str[0..0]:bindings[str[0]]]) : ([]);
      if (f)
	bindings[str[0]][str] = f;
      else {
	m_delete(bindings[str[0]], str);
	if (!sizeof(bindings[str[0]]))
	  m_delete(bindings, str[0]);
	else if(sizeof(bindings[str[0]])==1 && bindings[str[0]][str[0..0]])
	  bindings[str[0]] = bindings[str[0]][str[0..0]];
      }
      break;
    }
    return oldf;
  }

  //!
  function unbindstr(string str)
  {
    return bindstr(str, 0);
  }

  //!
  function getbindingstr(string str)
  {
    switch (sizeof(str||""))
    {
    case 0:
      return 0;
    case 1:
      return mappingp(bindings[str[0]])?
	bindings[str[0]][str] : bindings[str[0]];
    default:
      return mappingp(bindings[str[0]]) && bindings[str[0]][str];
    }
  }

  //!
  function bindtc(string cap, function f)
  {
    return bindstr(term->tgetstr(cap), f);
  }

  //!
  function unbindtc(string cap)
  {
    return unbindstr(term->tgetstr(cap));
  }

  //!
  function getbindingtc(string cap)
  {
    return getbindingstr(term->tgetstr(cap));
  }

  //!
  string parsekey(string k)
  {
    if (k[..1]=="\\!")
      k = term->tgetstr(k[2..]);
    else for(int i=0; i<sizeof(k); i++)
      switch(k[i])
      {
      case '\\':
	if(i<sizeof(k)-1) 
	  switch(k[i+1]) {
	  case '0':
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	    int n, l;
	    if (2<sscanf(k[i+1..], "%o%n", n, l))
	    {
	      n = k[i+1]-'0';
	      l = 1;
	    }
	    k = k[..i-1]+sprintf("%c", n)+k[i+1+l..];
	    break;
	  case 'e':
	  case 'E':
	    k = k[..i-1]+"\033"+k[i+2..];
	    break;
	  case 'n':
	    k = k[..i-1]+"\n"+k[i+2..];
	    break;
	  case 'r':
	    k = k[..i-1]+"\r"+k[i+2..];
	    break;
	  case 't':
	    k = k[..i-1]+"\t"+k[i+2..];
	    break;
	  case 'b':
	    k = k[..i-1]+"\b"+k[i+2..];
	    break;
	  case 'f':
	    k = k[..i-1]+"\f"+k[i+2..];
	    break;
	  default:
	    k = k[..i-1]+k[i+1..];
	    break;
	  }
	break;
      case '^':
	if(i<sizeof(k)-1 && k[i+1]>='?' && k[i+1]<='_') 
	  k = k[..i-1]+(k[i+1]=='?'? "\177":sprintf("%c",k[i+1]-'@'))+k[i+2..];
	break;
      }
    return k;
  }

  //!
  function bind(string k, function f)
  {
    return bindstr(parsekey(k), f);
  }

  //!
  function unbind(string k)
  {
    return unbindstr(parsekey(k));
  }

  //!
  function getbinding(string k, string cap)
  {
    return getbindingstr(parsekey(k));
  }

  //!
  mapping(string:function) getbindings()
  {
    mapping(int:function) bb = bindings-Array.filter(bindings, mappingp);
    return `|(mkmapping(Array.map(indices(bb), lambda(int n) {
						 return sprintf("%c", n);
					       }), values(bb)),
	      @Array.filter(values(bindings), mappingp));
  }

  //!
  void create(object|void _infd, object|string|void _term)
  {
    infd = _infd || Stdio.File( "stdin", "r" );
    term = objectp(_term)? _term : .Terminfo.getTerm(_term);
    disable();
    if(search(term->aliases, "dumb")>=0) {
      // Dumb terminal.  Don't try anything fancy.
      dumb = 1;
      return;
    }
    catch { oldattrs = infd->tcgetattr(); };
    if(catch { infd->tcsetattr((["ECHO":0])); }) {
      // If echo can't be disabled, Readline won't work very well.
      // Go to dumb mode.
      dumb = 1;
      return;
    }
    catch { infd->tcsetattr((["ECHO":0,"ICANON":0,"VMIN":1,"VTIME":0,
			      "VLNEXT":0])); };
  }

}

//!
class DefaultEditKeys
{
  static private multiset word_break_chars =
    mkmultiset("\t \n\r/*?_-.[]~&;\!#$%^(){}<>\"'`"/"");
  static object _readline;

  //!
  void self_insert_command(string str)
  {
    _readline->insert(str, _readline->getcursorpos());
  }

  //!
  void quoted_insert()
  {
    _readline->get_input_controller()->grabnextkey(self_insert_command);
  }

  //!
  void newline()
  {
    _readline->newline();
  }

  //!
  void up_history()
  {
    _readline->delta_history(-1);
  }

  //!
  void down_history()
  {
    _readline->delta_history(1);
  }

  //!
  void backward_delete_char()
  {
    int p = _readline->getcursorpos();
    _readline->delete(p-1,p);
  }

  //!
  void delete_char_or_eof()
  {
    int p = _readline->getcursorpos();
    if (p<sizeof(_readline->gettext()))
      _readline->delete(p,p+1);
    else if(!sizeof(_readline->gettext()))
      _readline->eof();
  }

  //!
  void forward_char()
  {
    _readline->setcursorpos(_readline->getcursorpos()+1);
  }

  //!
  void backward_char()
  {
    _readline->setcursorpos(_readline->getcursorpos()-1);
  }

  //!
  void beginning_of_line()
  {
    _readline->setcursorpos(0);
  }

  //!
  void end_of_line()
  {
    _readline->setcursorpos(sizeof(_readline->gettext()));
  }

  //!
  void transpose_chars()
  {
    int p = _readline->getcursorpos();
    if (p<0 || p>=sizeof(_readline->gettext()))
      return;
    string c = _readline->gettext()[p-1..p];
    _readline->delete(p-1, p+1);
    _readline->insert(reverse(c), p-1);
  }

  static array find_word_to_manipulate()
  {
    int p = _readline->getcursorpos();
    int ep;
    string line = _readline->gettext();
    while(word_break_chars[ line[p..p] ] && p < sizeof(line))
      p++;
    if(p >= sizeof(line)) {
      _readline->setcursorpos(p);
      return ({ 0, 0 });
    }
    ep = forward_find_word();
    _readline->delete(p, ep);
    return ({ line[p..ep-1], p });
  }

  //!
  void capitalize_word()
  {
    [string word, string pos]= find_word_to_manipulate();
    if(word)
      _readline->insert(String.capitalize(lower_case(word)), pos);
  }

  //!
  void upcase_word()
  {
    [string word, string pos]= find_word_to_manipulate();
    if(word)
      _readline->insert(upper_case(word), pos);
  }

  //!
  void downcase_word()
  {
    [string word, string pos]= find_word_to_manipulate();
    if(word)
      _readline->insert(lower_case(word), pos);
  }

  static int forward_find_word()
  {
    int p, n;
    string line = _readline->gettext();
    for(p = _readline->getcursorpos(); p < sizeof(line); p++) {
      if(word_break_chars[ line[p..p] ]) {
	if(n) break;
      } else n = 1;
    }
    return p;
  }

  static int backward_find_word()
  {
    int p = _readline->getcursorpos()-1;
    string line = _readline->gettext();
    if(p >= sizeof(line)) p = sizeof(line) - 1;
    while(word_break_chars[ line[p..p] ] && p >= 0)
      // find first "non break char"
      p--;
    for(;p >= 0; p--) 
      if(word_break_chars[ line[p..p] ]) {
	p++; // We want to be one char before the break char.
	break;
      }
    return p;
  }

  //!
  void forward_word()
  {
    _readline->setcursorpos(forward_find_word());
  }

  //!
  void backward_word()
  {
    _readline->setcursorpos(backward_find_word());
  }

  //!
  void kill_word()
  {
    _readline->kill(_readline->getcursorpos(), forward_find_word());
  }
  
  //!
  void backward_kill_word()
  {
    int sp = backward_find_word();
    int ep = _readline->getcursorpos();
    if((ep - sp) == 0)
      sp--;
    _readline->kill(sp, ep);
  }

  //!
  void kill_line()
  {
    _readline->kill(_readline->getcursorpos(), sizeof(_readline->gettext()));
  }

  //!
  void kill_whole_line()
  {
    _readline->kill(0, sizeof(_readline->gettext()));
  }

  //!
  void yank()
  {
    _readline->setmark(_readline->getcursorpos());
    _readline->insert(_readline->kill_ring_yank(),_readline->getcursorpos());
  }

  //!
  void kill_ring_save()
  { 
    _readline->add_to_kill_ring(_readline->region());
  }

  //!
  void kill_region()
  { 
    _readline->kill(@_readline->pointmark());
  }

  //!
  void set_mark()
  {
    _readline->setmark(_readline->getcursorpos());
  }

  //!
  void swap_mark_and_point()
  {
    int p=_readline->getcursorpos();
    _readline->setcursorpos(_readline->getmark());
    _readline->setmark(p);
  }

  //!
  void redisplay()
  {
    _readline->redisplay(0);
  }

  //!
  void clear_screen()
  {
    _readline->redisplay(1);
  }

  static array(array(string|function)) default_bindings = ({
    ({ "^[[A", up_history }),
    ({ "^[[B", down_history }),
    ({ "^[[C", forward_char }),
    ({ "^[[D", backward_char }),
    ({ "^[C", capitalize_word }),
    ({ "^[c", capitalize_word }),
    ({ "^[U", upcase_word }),
    ({ "^[u", upcase_word }),
    ({ "^[L", downcase_word }),
    ({ "^[l", downcase_word }),
    ({ "^[D", kill_word }),
    ({ "^[^H", backward_kill_word }),
    ({ "^[^?", backward_kill_word }),
    ({ "^[d", kill_word }),
    ({ "^[F", forward_word }),
    ({ "^[B", backward_word }),
    ({ "^[f", forward_word }),
    ({ "^[b", backward_word }),
    ({ "^[w", kill_ring_save }),
    ({ "^[W", kill_ring_save }),
    ({ "^0", set_mark }),
    ({ "^A", beginning_of_line }),
    ({ "^B", backward_char }),
    ({ "^D", delete_char_or_eof }),
    ({ "^E", end_of_line }),
    ({ "^F", forward_char }),
    ({ "^H", backward_delete_char }),
    ({ "^J", newline }),
    ({ "^K", kill_line }),
    ({ "^L", clear_screen }),
    ({ "^M", newline }),
    ({ "^N", down_history }),
    ({ "^P", up_history }),
    ({ "^R", redisplay }),
    ({ "^T", transpose_chars }),
    ({ "^U", kill_whole_line }),
    ({ "^V", quoted_insert }),
    ({ "^W", kill_region }),
    ({ "^Y", yank }),
    ({ "^?", backward_delete_char }),
    ({ "\\!ku", up_history }),
    ({ "\\!kd", down_history }),
    ({ "\\!kr", forward_char }),
    ({ "\\!kl", backward_char }),
    ({ "^X^X", swap_mark_and_point }),
  });

  //!
  static void set_default_bindings()
  {
    object ic = _readline->get_input_controller();
    ic->nullbindings();
    for(int i=' '; i<'\177'; i++)
      ic->bindstr(sprintf("%c", i), self_insert_command);
    for(int i='\240'; i<='\377'; i++)
      ic->bindstr(sprintf("%c", i), self_insert_command);

    if(ic->dumb) {
      ic->bind("^J", newline);
      return;
    }
    
    foreach(default_bindings, array(string|function) b)
      ic->bind(@b);
  }

  //!
  void create(object readline)
  {
    _readline = readline;
    set_default_bindings();
  }

}

//!
class History
{
  static private array(string) historylist;
  static private mapping(int:string) historykeep=([]);
  static private int minhistory, maxhistory, historynum;

  //!
  string encode()
  {
    return historylist*"\n";
  }
  
  //!
  int get_history_num()
  {
    return historynum;
  }

  //!
  string history(int n, string text)
  {
    if (n<minhistory)
      n = minhistory;
    else if (n-minhistory>=sizeof(historylist))
      n = sizeof(historylist)+minhistory-1;
    if(text != historylist[historynum-minhistory]) {
      if(!historykeep[historynum])
	historykeep[historynum] = historylist[historynum-minhistory];
      historylist[historynum-minhistory]=text;
    }
    return historylist[(historynum=n)-minhistory];
  }

  //!
  void initline()
  {
    if (sizeof(historylist)==0 || historylist[-1]!="") {
      historylist += ({ "" });
      if (maxhistory && sizeof(historylist)>maxhistory)
      {
	int n = sizeof(historylist)-maxhistory;
	historylist = historylist[n..];
	minhistory += n;
      }
    }
    historynum = sizeof(historylist)-1+minhistory;
    historykeep = ([]);
  }

  //!
  void finishline(string text)
  {
    foreach(indices(historykeep), int n)
      historylist[n-minhistory]=historykeep[n];
    historykeep = ([]);
    historylist[-1] = text;
    if(sizeof(historylist)>1 && historylist[-2]==historylist[-1])
      historylist = historylist[..sizeof(historylist)-2];
  }

  //!
  void set_max_history(int maxhist)
  {
    maxhistory = maxhist;
  }

  //!
  void create(int maxhist, void|array(string) hist)
  {
    historylist = hist || ({ "" });
    minhistory = historynum = 0;
    maxhistory = maxhist;
  }
}


static private OutputController output_controller;
static private InputController input_controller;
static private string prompt="";
static private array(string) prompt_attrs=0;
static private string text="", readtext;
static private function(string:void) newline_func;
static private int cursorpos = 0;
static private int mark = 0;
/*static private */ History historyobj = 0;
static private int hide = 0;

static private array(string) kill_ring=({});
static private int kill_ring_size=30;

//!  get current output control object
//!  @returns
//!    Terminal output controller object
OutputController get_output_controller()
{
  return output_controller;
}

//!  get current input control object
//!  @returns
//!    Terminal input controller object
InputController get_input_controller()
{
  return input_controller;
}

//!   Return the current prompt string.
string get_prompt()
{
  return prompt;
}

//!   Set the prompt string.
//! @param newp
//!   New prompt string
//! @param newattrs
//!   Terminal attributes
string set_prompt(string newp, array(string)|void newattrs)
{
//  werror("READLINE: Set prompt: %O\n",newp);
  string oldp = prompt;
  if(newp!=prompt || !equal(prompt_attrs, newattrs))
  {
    prompt = newp;
    prompt_attrs = newattrs && copy_value(newattrs);
    if(newline_func) redisplay(0);

  }
  return oldp;
}

//!   Set text echo on or off.
//! @param onoff
//! 1 for echo, 0 for no echo.
void set_echo(int onoff)
{
  hide=!onoff;
}

//! @fixme
//!   Document this function
string gettext()
{
  return text;
}

//! @fixme
//!   Document this function
int getcursorpos()
{
  return cursorpos;
}

//! @fixme
//!   Document this function
int setcursorpos(int p)
{
  if (p<0)
    p = 0;
  if (p>sizeof(text))
    p = sizeof(text);
  if (p<cursorpos)
  {
    if(!input_controller->dumb)
      output_controller->move_backward(text[p..cursorpos-1]);
    cursorpos = p;
  }
  else if (p>cursorpos)
  {
    if(!input_controller->dumb)
      output_controller->move_forward(text[cursorpos..p-1]);
    cursorpos = p;
  }
  return cursorpos;
}

//! @fixme
//!   Document this function
int setmark(int p)
{
  if (p<0)
    p = 0;
  if (p>sizeof(text))
    p = sizeof(text);
  mark=p;
}

//! @fixme
//!   Document this function
int getmark()
{
  return mark;
}

//! @fixme
//!   Document this function
void insert(string s, int p)
{
  if (p<0)
    p = 0;
  if (p>sizeof(text))
    p = sizeof(text);
  setcursorpos(p);
  if(!input_controller->dumb)
    output_controller->write(s,0,hide);
  cursorpos += sizeof(s);
  string rest = text[p..];
  if (sizeof(rest) && !input_controller->dumb)
  {
    output_controller->write(rest,0,hide);
    output_controller->move_backward(rest);
  }
  text = text[..p-1]+s+rest;

  if (mark>p) mark+=sizeof(s);
}

//! @fixme
//!   Document this function
void delete(int p1, int p2)
{
  if (p1<0)
    p1 = 0;
  if (p2>sizeof(text))
    p2 = sizeof(text);
  setcursorpos(p1);
  if (p1>=p2)
    return;
  if(!input_controller->dumb) {
    output_controller->write(text[p2..],0,hide);
    output_controller->erase(text[p1..p2-1]);
  }
  text = text[..p1-1]+text[p2..];

  if (mark>p2) mark-=(p2-p1);
  else if (mark>p1) mark=p1;

  cursorpos = sizeof(text);
  setcursorpos(p1);
}

//! @fixme
//!   Document this function
array(int) pointmark() // returns point and mark in numeric order
{
   int p1,p2;
   p1=getcursorpos(),p2=getmark();   
   if (p1>p2) return ({p2,p1});
   return ({p1,p2});
}

//! @fixme
//!   Document this function
string region(int ... args) /* p1, p2 or point-mark */
{
   int p1,p2;
   if (sizeof(args)) [p1,p2]=args;
   else [p1,p2]=pointmark();
   return text[p1..p2-1];
}

//! @fixme
//!   Document this function
void kill(int p1, int p2)
{
  if (p1<0)
    p1 = 0;
  if (p2>sizeof(text))
    p2 = sizeof(text);
  if (p1>=p2)
    return;
  add_to_kill_ring(text[p1..p2-1]);
  delete(p1,p2);
}

//! @fixme
//!   Document this function
void add_to_kill_ring(string s)
{
   kill_ring+=({s});
   if (sizeof(kill_ring)>kill_ring_size) kill_ring=kill_ring[1..];
}

//! @fixme
//!   Document this function
string kill_ring_yank()
{
   if (!sizeof(kill_ring)) return "";
   return kill_ring[-1];
}

//! @fixme
//!   Document this function
void history(int n)
{
  if(historyobj) {
    string h = historyobj->history(n, text);
    delete(0, sizeof(text)+sizeof(prompt));
    insert(h, 0);
  }
}

//! Changes the line to a line from the history @[d] steps from the
//! current entry (0 being the current line, negative values older,
//! and positive values newer).
//! @note
//!   Only effective if you have a history object.
void delta_history(int d)
{
  if(historyobj)
    history(historyobj->get_history_num()+d);
}

//! @fixme
//!   Document this function
void redisplay(int clear, int|void nobackup)
{
  int p = cursorpos;
  if(clear)
    output_controller->clear();
  else if(!nobackup) {
    setcursorpos(0);
    output_controller->bol();
    output_controller->clear(1);
  }
  output_controller->check_columns();

  if(!input_controller->dumb) {
    if(newline_func) {
      if(prompt_attrs)
	output_controller->turn_on(@prompt_attrs);
      output_controller->write(prompt);
      if(prompt_attrs)
	output_controller->turn_off(@prompt_attrs);      
    }
    output_controller->write(text,0,hide);
  }
  cursorpos = sizeof(text);
  setcursorpos(p);
}

static private void initline()
{
  text = "";
  cursorpos = 0;
  if (historyobj)
    historyobj->initline();
}

//! @fixme
//!   Document this function
string newline()
{
  setcursorpos(sizeof(text));
  if(!input_controller->dumb)
    output_controller->newline();
  else
    output_controller->bol();
  string data = text;
  if (historyobj && !hide)
    historyobj->finishline(text);
  initline();
  if(newline_func)
    newline_func(data);
}

//! @fixme
//!   Document this function
void eof()
{
  if (historyobj)
    historyobj->finishline(text);
  initline();
  if(newline_func)
    newline_func(0);    
}

//! Print a message to the output device
void message(string msg)
{
  int p = cursorpos;
  setcursorpos(sizeof(text));
  output_controller->newline();
  foreach(msg/"\n", string l) {
    output_controller->write(l);
    output_controller->newline();
  }
  redisplay(0, 1);
  setcursorpos(p);
}

//! @fixme
//!   Document this function
void write(string msg,void|int word_wrap)
{
  int p = cursorpos;
  setcursorpos(0);
  if(!input_controller->dumb) {
    output_controller->bol();
    output_controller->clear(1);
  }
  array(string) tmp=msg/"\n";
  foreach(tmp[..sizeof(tmp)-2],string l)
  {
    output_controller->write(l,word_wrap);
    output_controller->newline();
  }
  output_controller->write(tmp[-1],word_wrap);

  cursorpos=sizeof(text);
  redisplay(0, 1);
  setcursorpos(p);
}

//! @fixme
//!   Document this function
void list_completions(array(string) c)
{
  message(sprintf("%-*#s",output_controller->get_number_of_columns(),
		  c*"\n"));
}

static private void read_newline(string s)
{
  input_controller->disable();
  readtext = s;
}

//! @fixme
//!   Document this function
void set_nonblocking(function f)
{
  int p=cursorpos;
  if (newline_func = f) {
    output_controller->enable();
    input_controller->enable();

    output_controller->bol();
    output_controller->clear(1);
    cursorpos=p;
    redisplay(0,1);
  } else {
    setcursorpos(0);
    output_controller->bol();
    output_controller->clear(1);
    cursorpos=p;

    input_controller->disable();
    output_controller->disable();
  }
}

//! @fixme
//!   Document this function
void set_blocking()
{
  set_nonblocking(0);
}

//! @fixme
//!   Document this function
string edit(string data, string|void local_prompt, array(string)|void attrs)
{
  if(data && sizeof(data) && input_controller->dumb)
  {
    string ret=edit("", (local_prompt || get_prompt()) +"["+data+"] ", attrs);
    return (!ret || !sizeof(ret))?data:ret;
  }
  string old_prompt;
  array(string) old_attrs;

  if(newline_func == read_newline)
    return 0;

  if(local_prompt)
  {
    old_prompt = get_prompt();
    old_attrs = prompt_attrs;
    set_prompt(local_prompt, attrs);
  }
  
  function oldnl = newline_func;
  if(attrs)
      output_controller->turn_on(@attrs);
  output_controller->write(local_prompt||prompt);
  if(attrs)
      output_controller->turn_off(@attrs);
  initline();
  newline_func = read_newline;
  readtext = "";
  output_controller->enable();
  insert(data, 0);
  int res = input_controller->run_blocking();


  set_nonblocking(oldnl);
  
  if(local_prompt)
    set_prompt(old_prompt, old_attrs);
  
  return (res>=0 || sizeof(readtext)) && readtext;
}

//! @fixme
//!   Document this function
string read(string|void prompt, array(string)|void attrs)
{
  return edit("", prompt, attrs);
}

//! @fixme
//!   Document this function
void enable_history(array(string)|History|int hist)
{
  if (objectp(hist))
    historyobj = hist;
  else if(arrayp(hist))
    historyobj = History(512,hist);
  else if(!hist)
    historyobj = 0;
  else if(historyobj)
    historyobj->set_max_history(hist);
  else
    historyobj = History(hist);
}

//! @fixme
//!   Document this function
History get_history()
{
  return historyobj;
}

static void destroy()
{
  if(input_controller)
    destruct(input_controller);
  if(output_controller)
    destruct(output_controller);
}

//! Creates a Readline object, that takes input from @[infd] and has output
//! on @[outfd].
//!
//! @param infd
//!   Defaults to @[Stdio.stdin].
//!
//! @param interm
//!   Defaults to @[Stdio.Terminfo.getTerm()].
//!
//! @param outfd
//!   Defaults to @[infd], unless @[infd] is 0, in which case
//!   @[outfd] defaults to @[Stdio.stdout].
//!
//! @param outterm
//!   Defaults to @[interm].
void create(object|void infd, object|string|void interm,
	    object|void outfd, object|string|void outterm)
{
  atexit(destroy);
  output_controller = OutputController(outfd || infd, outterm || interm);
  input_controller = InputController(infd, interm);
  DefaultEditKeys(this);
}
