#! /usr/bin/env pike

#pragma strict_types

// $Id: mkpeep.pike,v 1.40 2004/06/05 17:23:29 nilsson Exp $

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
    }

    SKIPWHITE(line);
  }

  return t;
}

class Rule {
  array(string) from;
  array(string) to;
  int opcodes;
  string line;

  mapping(string:array(string)) varmap = ([]);

  void create(string _line) {
    line = _line;
    array(string) tokens = tokenize(line);

    // Section tokens into source and target.
    int div = search(tokens, ":");
    if(div==-1) err("Syntax error. No source/target delimiter.\n");
    from = tokens[..div-1];
    to = tokens[div+1..];

    // Count opcodes
    foreach(from, string ops)
      if( (<'F', '?'>)[ops[0]] )
	opcodes++;

    post_process();
  }

  void post_process() {
    array(string) nt = ({});

    int i;
    foreach(from, string t)
      switch(t[0]) {
      case '(':
	array(string) exprs = explode_comma_expr(t[1..sizeof(t)-2]);
	foreach(exprs; int x; string expr) {
	  string arg = sprintf("$%d%c", i, 'a'+x);
	  if(arg != expr && sizeof(expr))
	    nt += ({ sprintf("(%s)==%s", expr, arg) });
	}
	break;

      case '[':
	nt += ({ t[1..sizeof(t)-2] });
	break;

      case 'F':
	i++;
	nt += ({ t+"==$"+i+"o" });
	break;

      case '?':
	i++;
	nt += ({ "$"+i+"o != -1" });
	break;

      default:
	nt += ({ t });
      }

    for(int e; e<sizeof(nt); e++)
      if(nt[e]=="!") {
	sscanf(nt[e+1], "%s==%s", string op, string arg);
	nt[e+1] = sprintf("%s != %s && %s !=-1", op, arg, arg);
	nt = nt[..e-1]+nt[e+1..];
	e--;
      }

    from = nt;
  }

  string _sprintf(int t) {
    return t=='O' ? ("Rule("+line+")") : 0;
  }
}

// Replace $[0-9]+(o|a|b) with something a C compiler can understand.
string treat(string expr)
{
  array(string) tmp = expr/"$";
  for(int e=1; e<sizeof(tmp); e++) {
    string rest;
    int num, type;
    if(sscanf(tmp[e], "%d%c%s", num, type, rest)!=3)
      err("Syntax error (3).\n");

    num--;
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
string functions = "";

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
    functions += "INLINE static void _asm_peep_"+made_fun+"(void)\n{\n";
    functions += make_switch(2);
    functions += "}\n\n";
  }

  int made_fun;
  string get_string(int ind) {
    if(made_fun) {
      string ret = "";
      ret += sprintf("%*n_asm_peep_%d();\n", ind, made_fun);
      ret += sprintf("%*nreturn;\n", ind);
      return ret;
    }
    return make_switch(ind);
  }

  string make_switch(int ind) {
    string ret = "";
    ret += sprintf("%*nswitch(%s)\n", ind, test);
    ret += sprintf("%*n{\n", ind);

    foreach(sort(indices(cases)), string c) {
      ret += sprintf("%*ncase %s:\n", ind, c);
      foreach(cases[c], object(Switch)|object(Breakable) b)
	  ret += b->get_string(ind+2);
      ret += sprintf("%*n  break;\n", ind);
      ret += sprintf("\n");
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

  string get_string(int ind) {
    string ret = "";
    foreach(lines, string|array(string) line)
      if(stringp(line)) {
	if(String.trim_all_whites([string]line)=="")
	  ret += line;
	else
	  ret += sprintf("%*n%s\n", ind, line);
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
  string test,cons,var;
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
    mapping(string:array(Rule)) d = foo[test];
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
    int ind;
    foreach(data, Rule d)
    {
      buf->add_line(" "*ind+"/* ", d->line, " */");

      if(sizeof(d->from))
	buf->add_line(" "*ind+"if(", treat(d->from*" && "), ")");

      buf->add_line( sprintf("%*n{", ind) );
      ind += 2;
      array(string) opargs = ({ d->opcodes+", " });

      for(int i=0; i<sizeof(d->to); i++)
      {
	array args=({});
	string fcode=d->to[i];
	if(i+1<sizeof(d->to) && d->to[i+1][0]=='(')
	{
	  string tmp=d->to[i+1];
	  args=explode_comma_expr(tmp[1..sizeof(tmp)-2]);
	  i++;
	}
	opargs += ({ sizeof(args)+1+", ", fcode+", " });
	opargs += map(args,treat)[*]+", ";
      }
      buf->add_line(" "*ind+"do_optimization(", opargs, "0);");

      buf->add_line( sprintf("%*nreturn;", ind) );
      ind -= 2;
      buf->add_line( sprintf("%*n}", ind, test) );
    }
    ret += ({ buf });
  }
  return ret;
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

  write("\n\n/* Generated from %s by mkpeep.pike\n   %s\n*/\n\n",
	argv[1], Calendar.ISO.now()->format_time());

  array(Switch) a = [array(Switch)]make_switches(data);
  if(sizeof(a)!=1 || !a[0]->is_switch) error("Expected one top switch.\n");

  a[0]->make_child_fun();
  write( functions );

  write("INLINE static void low_asm_opt(void) {\n");
  write( a[0]->get_string(2) );

  write("}\n");

  return 0;
}
