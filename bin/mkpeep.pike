#! /usr/bin/env pike

#pragma strict_types

#define SKIPWHITE(X) sscanf(X, "%*[ \t\n]%s", X)

void err(string e, mixed ... a) {
  werror(e, @a);
  exit(1);
}

array(string) linebreak(array(string) tokens, int width) {
  array(string) out = ({});
  string line = "";
  foreach(tokens, string token)
    if(sizeof(line)+sizeof(token)<=width)
      line += token;
    else {
      out += ({ line });
      SKIPWHITE(token);
      line = token;
    }
  if(line!="")
    out += ({ line });
  return out;
}

string make_multiline(string prefix, array(string)|string tokens,
		      string suffix) {
  if(stringp(tokens))
    tokens = ((tokens/" ")/1)*({" "});
  tokens += ({ suffix });
  tokens = linebreak([array(string)]tokens, 79-sizeof(prefix));
  string ret = "";
  foreach(tokens; int r; string line)
    if(!r)
      ret += prefix + line + "\n";
    else
      ret += sprintf("%*n%s\n", sizeof(prefix), line);
  return ret;
}

// Find the matching parenthesis
int find_end(string s)
{
  int parlvl=1;

  for(int e=1; e<sizeof(s); e++)
  {
    switch(s[e])
    {
    case '(': case '{': case '[':
      parlvl++; break;
    case ')': case '}': case ']':
      parlvl--;
      if(!parlvl) return e;
      break;
    case '"':
      while(s[e]!='"') e+=1+(s[e]=='\\');
      break;
    }
  }

  err("Syntax error (1).\n");
}

array(string) explode_comma_expr(string s)
{
  int parlvl;
  array(string) ret=({});
  int begin=0;

  for(int e=0; e<sizeof(s); e++)
  {
    switch(s[e])
    {
    case '(': case '{': case '[':
      parlvl++; break;
    case ')': case '}': case ']':
      parlvl--; break;
    case '"':
      while(s[e]!='"') e+=1+(s[e]=='\\');
      break;

    case ',':
      if(!parlvl)
      {
	ret+=({ String.trim_all_whites(s[begin..e-1]) });
	begin=e+1;
      }
    }
  }

  // Ignore empty last arguments
  if(sizeof(String.trim_all_whites(s[begin..])))
    ret += ({ String.trim_all_whites(s[begin..]) });

  return ret;
}

array(string) tokenize(string line) {
  array(string) t = ({});

  while(sizeof(line))
  {
    switch(line[0])
    {
      // Source / Target separator
    case ':':
      t += ({":"});
      line = line[1..];
      break;

    case '!':
      t += ({"!"});
      line = line[1..];
      break;

      // Any identifier (i.e. not eof).
    case '?':
      t += ({"?"});
      line = line[1..];
      break;

      // Identifier
    case 'A'..'Z':
    case 'a'..'z':
    case '0'..'9':
    case '_':
      sscanf(line, "%[a-zA-Z0-9_]%s", string id, line);
      t += ({"F_"+id});
      break;

      // argument
    case '(':
      {
	int i=find_end(line);
	t += ({ line[0..i] });
	line = line[i+1..];
      }
      break;

      // condition
    case '[':
      {
	int i=find_end(line);
	t += ({ line[0..i] });
	line = line[i+1..];
      }
      break;

      // Error message
    case '"':
      int i;
      for (i = 1; (i < sizeof(line)); i++) {
	if (line[i] == '"') break;
	if (line[i] == '\\') i++;
      }
      t += ({ line[..i] });
      line = line[i+1..];
      break;

    case ' ': case '\t': case '\r': case '\n':
      break;

    default:
      werror("Unsupported character: '%c' (%d)\n",
	     line[0], line[0]);
      line = line[1..];
      break;
    }

    SKIPWHITE(line);
  }

  return t;
}

class Rule {
  array(string) from = ({});
  array(string) to = ({});
  int opcodes;
  string line = "";

  mapping(string:array(string)) varmap = ([]);

  protected void create(string _line) {
    line = _line;
    array(string) tokens = tokenize(line);

    // Section tokens into source and target.
    int div = search(tokens, ":");
    if(div==-1) err("Syntax error. No source/target delimiter.\n");
    from = tokens[..div-1];

    post_process();

    // Note: Can't reverse scan until we have counted the opcodes. */
    reverse_scan(to = tokens[div+1..]);
  }

  void reverse_scan(array(string) tokens)
  {
    /* Adjust to reverse scan. */
    foreach(tokens; int i; string t) {
      array(string) tmp = t/"$";
      if (sizeof(tmp) == 1) continue;
      foreach(tmp; int j; string expr) {
	if (!j) continue;
	sscanf(expr, "%d%s", int num, string rest);
	tmp[j] = sprintf("%d%s", opcodes - num, rest);
      }
      tokens[i] = tmp * "$";
    }
  }

  void post_process() {
    array(string) nt = ({});

    opcodes = 0;
    foreach(from, string t)
      switch(t[0]) {
      case '(':
	array(string) exprs = explode_comma_expr(t[1..sizeof(t)-2]);
	foreach(exprs; int x; string expr) {
	  string arg = sprintf("$%d%c", opcodes, 'a'+x);
	  if(arg != expr && sizeof(expr))
	    nt += ({ sprintf("(%s)==%s", expr, arg) });
	}
	break;

      case '[':
	nt += ({ t[1..sizeof(t)-2] });
	break;

      case 'F':
	opcodes++;
	nt += ({ t+"==$"+opcodes+"o" });
	break;

      case '?':
	opcodes++;
	nt += ({ "$"+opcodes+"o != -1" });
	break;

      default:
	nt += ({ t });
      }

    reverse_scan(nt);

    for(int e; e<sizeof(nt); e++)
      if(nt[e]=="!") {
	sscanf(nt[e+1], "%s==%s", string op, string arg);
	nt[e+1] = sprintf("%s != %s && %s !=-1", op, arg, arg);
	nt = nt[..e-1]+nt[e+1..];
	e--;
      }

    from = nt;
  }

  protected string _sprintf(int t, mapping(string:int)|void opts) {
    return t=='O' && ("Rule("+line+")");
  }
}

// Replace $[0-9]+(o|a|b) with something a C compiler can understand.
string treat(string expr)
{
  array(string) tmp = expr/"$";
  for(int e=1; e<sizeof(tmp); e++) {
    string rest = "";
    int num, type;
    if(sscanf(tmp[e], "%d%c%s", num, type, rest)!=3)
      err("Syntax error (3).\n");

    switch(type)
    {
    case 'a': tmp[e]="argument("+num+")"+rest; break;
    case 'b': tmp[e]="argument2("+num+")"+rest; break;
    case 'o': tmp[e]="opcode("+num+")"+rest; break;
    }
  }
  return tmp*"";
}

int function_serial;
string(8bit) functions = "";

class Switch(string test) {
  constant is_switch = 1;
  mapping(string:array(Switch|Breakable)) cases = ([]);

  void add_case(string c, array(Switch|Breakable) b) {
    cases[c] = b;
  }

  void make_child_fun() {
    foreach(sort(indices(cases)), string c)
      if(sizeof(cases[c])==1 && cases[c][0]->is_switch)
	([object(Switch)]cases[c][0])->make_fun();
      else
	foreach(cases[c], object(Switch)|object(Breakable) b)
	  if(b->is_switch)
	    ([object(Switch)]b)->make_child_fun();
  }

  void make_fun() {
    made_fun = ++function_serial;
    functions += "static int _asm_peep_"+made_fun+"(void)\n{\n";
    functions += make_switch(2);
    functions +=
      "  return 0;\n"
      "}\n"
      "\n";
  }

  int made_fun;
  string get_string(int(0..) ind) {
    if(made_fun) {
      string ret = "";
      ret += sprintf("%*nif (_asm_peep_%d())\n"
		     "%*n  return 1;\n",
		     ind, made_fun,
		     ind);
      return ret;
    }
    return make_switch(ind);
  }

  string make_switch(int(0..) ind) {
    string ret = "";
    ret += sprintf("%*nswitch(%s)\n", ind, test);
    ret += sprintf("%*n{\n", ind);

    mapping(string:array(string)) rev = ([]);
    foreach(sort(indices(cases)), string c)
    {
      if(`+(@cases[c]->is_switch)) continue;
      string code = cases[c]->get_string(0)*"";
      while( sscanf(code, "%s/*%*s*/%s", code, string tail)==3 )
        code = code+tail;
      rev[ code ] += ({ c });
    }
    mapping(string:array(string)) alias = ([]);
    foreach(rev;; array(string) cs)
    {
      if( sizeof(cs)>1 )
      {
        alias[cs[0]] = cs[1..];
        foreach(cs[1..], string c)
          m_delete(cases, c);
      }
    }

    foreach(sort(indices(cases)), string c) {
      ret += sprintf("%*ncase %s:\n", ind, c);
      if( alias[c] )
        foreach( alias[c], string c )
          ret += sprintf("%*ncase %s:\n", ind, c);

      foreach(cases[c], object(Switch)|object(Breakable) b)
	ret += b->get_string([int(0..)](ind+2));
      ret += sprintf("%*n  break;\n"
		     "\n",
		     ind);
    }

    ret += sprintf("%*n}\n", ind);
    return ret;
  }
}

class Breakable {
  array(string|array(string)) lines = ({});

  void add_line(string a, void|array(string)|string b, void|string c) {
    if(c)
      lines += ({ ({ a,b,c }) });
    else
      lines += ({ a });
  }

  string get_string(int(0..) ind) {
    string ret = "";
    foreach(lines, string|array(string) line)
      if(stringp(line)) {
	if(String.trim_all_whites([string]line)=="")
	  ret += [string]line;
	else
	  ret += sprintf("%*n%s\n", ind, [string]line);
      }
      else {
	array(string) line = [array(string)]line;
	ret += make_multiline(" "*ind+line[0], line[1], line[2]);
      }
    return ret;
  }
}

array(Switch|Breakable) make_switches(array(Rule) data)
{
  int i,maxv;
  string test = "", cons = "", var="";
  array(Switch|Breakable) ret = ({});

  while(1)
  {
    // meta variable : condition (from) : array(Rule)
    mapping(string:mapping(string:array(Rule))) foo = ([]);

    foreach(data, Rule d)
      foreach(d->from, string line)
      {
	if(sscanf(line,"F_%*[A-Z0-9_]==%s", var)==2 ||
	   sscanf(line,"(%*d)==%s", var)==2 ||
	   sscanf(line,"%*d==%s", var)==2)
	{
	  if(!foo[var]) foo[var]=([]);
	  if(!foo[var][line]) foo[var][line]=({});
	  foo[var][line] += ({d});
	}
      }

    // Check what variable has most values.
    // Sort to create deterministic result.
    maxv = 0;
    foreach(sort(indices(foo)), string ptest)
      if(sizeof(foo[ptest])>maxv) {
	test = ptest;
	maxv = sizeof(foo[ptest]);
      }

    // If zero, done
    if(maxv <= 1) break;

    Switch s = Switch(treat(test));

    // condition : array(Rule)
    mapping(string:array(Rule)) d = [mapping]foo[test];
    array(string) a = indices(d);
    array(array(Rule)) b = values(d);
    sort(a,b);

    /* test : variable
     * a[x] : condition
     * b[x] : rules
     */

    for(int i=0; i<sizeof(a); i++)
    {
      /* The lines b[i] are removed from data as they
       * will be treated below
       */
      data-=b[i];

      if(sscanf(a[i],"(%s)==%s",cons,var)!=2)
	sscanf(a[i],"%s==%s",cons,var);

      foreach(b[i], Rule d)
	d->from -= ({ a[i] });

      s->add_case(cons, make_switches(b[i]));
    }
    ret += ({ s });
  }

  // Take care of whatever is left
  if(sizeof(data))
  {
    Breakable buf = Breakable();
    int(0..) ind;
    foreach(data, Rule d)
    {
      buf->add_line(" "*ind+"/* ", d->line, " */");

      if(sizeof(d->from))
	buf->add_line(" "*ind+"if(", treat(d->from*" && "), ")");

      buf->add_line( sprintf("%*n{", ind) );
      ind += 2;
      array(string) opargs = ({ d->opcodes+", " });

      array(array(string)) ops = ({});

      for(int i=0; i<sizeof(d->to); i++)
      {
	array(string) args=({});
	string fcode=d->to[i];
	if(i+1<sizeof(d->to) && d->to[i+1][0]=='(')
	{
	  string tmp=d->to[i+1];
	  args=explode_comma_expr(tmp[1..sizeof(tmp)-2]);
	  i++;
	}
	if (fcode[0] == '"') {
	  buf->add_line(" "*ind+"my_yyerror(",
			({ fcode, @map(args,treat)[*]+"" }) * ", ",
			");");
	  continue;
	}
	ops += ({ ({ sizeof(args)+1+", ", fcode+", ",
		     @map(args,treat)[*]+", " }) });
      }
      opargs += ({ sizeof(ops) + ", " }) + reverse(ops) * ({});
      buf->add_line(" "*ind+"do_optimization(", opargs, "0);");

      buf->add_line( sprintf("%*nreturn 1;", ind) );
      ind -= 2;
      buf->add_line( sprintf("%*n}", ind) );
    }
    ret += ({ buf });
  }
  return ret;
}

void check_duplicates(array(Rule) data)
{
  int err;
  mapping(string:Rule) froms = ([]);
  foreach(data;; Rule r)
  {
    if(froms[r->from*","])
    {
      werror("Collision between\n  %O and\n  %O.\n",
             froms[r->from*","]->line, r->line);
      err=1;
    }
    froms[r->from*","] = r;
  }

  if(err)
    exit(1, "Found %d rule collisions.\n", err);
}

int main(int argc, array(string) argv)
{
  array(Rule) data=({});

  // Read input file
  string f=cpp(Stdio.read_file(argv[1]), argv[1]);
  foreach(f/"\n",f)
  {
    if(f=="" || f[0]=='#') continue;

    // Parse expressions
    foreach(f/";",f) {
      f = String.trim_all_whites(f);
      if(!sizeof(f)) continue;
      data += ({ Rule(f) });
    }
  }

  check_duplicates(data);

  write("\n\n/* Generated from %s by mkpeep.pike\n   %s\n*/\n\n",
	argv[1], Calendar.ISO.now()->format_time());

  array(Switch) a = [array(Switch)]make_switches(data);

  a->make_child_fun();

  write( functions );

  write("static int low_asm_opt(void) {\n");
  map(a->get_string(2), write);

  write("  return 0;\n"
	"}\n");

  return 0;
}
