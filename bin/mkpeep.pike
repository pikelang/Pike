#!/usr/local/bin/pike

#pragma strict_types

// $Id: mkpeep.pike,v 1.32 2003/04/01 00:52:02 nilsson Exp $

#define SKIPWHITE(X) sscanf(X, "%*[ \t\n]%s", X)

void err(string e, mixed ... a) {
  werror(e, @a);
  exit(1);
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

void dump(array(Rule) data, int ind)
{
  int i,maxv;
  string test;
  mixed cons, var;

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

    write("%*nswitch(%s)\n", ind, treat(test));
    write("%*n{\n", ind);

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

      write("%*ncase %s:\n", ind, cons);

      foreach(b[i], Rule d)
	d->from -= ({ a[i] });

      dump(b[i], ind+2);
      write("%*n  break;\n", ind);
      write("\n");
    }

    write("%*n}\n", ind);
  }

  /* Take care of whatever is left */
  if(sizeof(data))
  {
    foreach(data, Rule d)
    {
      write("%*n/* %s */\n", ind, d->line);

      if(sizeof(d->from))
      {
	string test;
	test=treat(d->from*" && ");
	write("%*nif(%s)\n", ind, test);
      }
      write("%*n{\n", ind);
      ind+=2;
      write("%*ndo_optimization(%d,\n", ind, d->opcodes);

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
	write("%*n                %d,%s,%{(%s), %}\n",
	      ind,
	      sizeof(args)+1,
	      fcode,
	      Array.map(args,treat));

      }
      write("%*n                0);\n",ind);

      write("%*ncontinue;\n", ind);
      ind-=2;
      write("%*n}\n", ind, test);
    }
  }
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

  write("  len=instrbuf.s.len/sizeof(p_instr);\n"
	"  instructions=(p_instr *)instrbuf.s.str;\n"
	"  instrbuf.s.str=0;\n"
	"  fifo_len=0;\n"
	"  init_bytecode();\n\n"
	"  for(eye=0;eye<len || fifo_len;)\n  {\n"
	"\n"
	"#ifdef PIKE_DEBUG\n"
	"    if(a_flag>6) {\n"
	"      int e;\n"
	"      fprintf(stderr, \"#%ld,%d:\",\n"
	"              DO_NOT_WARN((long)eye),\n"
	"              fifo_len);\n"
	"      for(e=0;e<4;e++) {\n"
	"        fprintf(stderr,\" \");\n"
	"        dump_instr(instr(e));\n"
	"      }\n"
	"      fprintf(stderr,\"\\n\");\n"
	"    }\n"
	"#endif\n\n");

  dump(data, 4);

  write("    advance();\n");
  write("  }\n");
  write("  for(eye=0;eye<len;eye++) free_string(instructions[eye].file);\n");
  write("  free((char *)instructions);\n");

  return 0;
}
