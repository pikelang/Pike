#pike __REAL_VERSION__

class Evaluator
{
/* Incremental Pike evaluator */

import Stdio;
import Array;
import String;
import Getopt;

/* todo:
 *  return (void)1; will give me problems.
 *  strstr(string,string *) -> return first occurance of first string...
 *  inherit doesn't work
 *  preprocessor stuff
 *  for simple evaluations, re-use (and inherit) compiled program.
 */

#!define catch(X) ((X),0)

  // #pragma all_inline

  // #define DEBUG

  mapping variables=([]);
  mapping(string:mixed) constants=([]);
  array(string) functions=({});
  array(string) function_names=({});
  array(string) imports_and_inherits=({});

  constant keywords = (< "array", "break", "case", "catch", "class",
			 "constant", "continue", "default", "do", "else",
			 "extern", "final", "float", "for", "foreach",
			 "function", "gauge", "if", "import", "int",
			 "inherit", "inline", "lambda", "local", "mapping",
			 "mixed", "multiset", "nomask", "object", "optional",
			 "program", "predef", "private", "protected", "public",
			 "return", "sscanf", "string", "static", "switch",
			 "type", "typeof", "variant", "void", "while", >);

  mapping query_variables() { return variables; }
/* do nothing */
  
  function write;

  object compile_handler = class
    {
      void compile_error(string file,int line,string err)
      {
	write(sprintf("%s:%s:%s\n",master()->trim_file_name(file),
		      line?(string)line:"-",err));
      }
    }();

  object eval(string f)
  {
    string prog,file;
    object o;
    mixed err;
    prog=("#pragma unpragma_strict_types\n"+ // "#pragma all_inline\n"+

	  imports_and_inherits*""+

	  map(indices(constants),lambda(string f)
	      { return constants[f]&&sprintf("constant %s=___hilfe.%s;",f,f); })*"\n"+
	      map(indices(variables),lambda(string f)
		  { return sprintf("mixed %s;",f,f); })*"\n"+
		  "\nmapping query_variables() { return ([\n"+
		  map(indices(variables),lambda(string f)
		      { return sprintf("    \"%s\":%s,",f,f); })*"\n"+
		      "\n  ]);\n}\n"+
		      functions*"\n"+
		      "\n# 1\n"+
		      f+"\n");
    
#ifdef DEBUG
    write("program:"+prog);
#endif
    program p;

    mixed oldwrite=all_constants()->write;
    add_constant("write",write);
    add_constant("___hilfe",constants);
    err=catch(p=compile_string(prog, "-", compile_handler));
    add_constant("___hilfe");
    add_constant("write",oldwrite);

    if(err)
    {
#ifdef DEBUG
      write(describe_backtrace(err));
      write("Couldn't compile expression.\n");
#endif
      return 0;
    }
    if(err=catch(o=clone(p)))
    {
      trace(0);
      write(describe_backtrace(err));
      return 0;
    }
    foreach(indices(variables), string f) o[f]=variables[f];
    return o;
  }
  
  mixed do_evaluate(string a, int show_result)
  {
    mixed foo, c;
    if(foo=eval(a))
    {
      if(c=catch(a=sprintf("%O",foo->___Foo4711())))
      {
	trace(0);
	if(objectp(c) && c->is_generic_error)
	  catch { c = ({ c[0], c[1] }); };
	if(arrayp(c) && sizeof(c)==2 && arrayp(c[1]))
	{
	  c[1]=c[1][sizeof(backtrace())..];
	  write(describe_backtrace(c));
	}else{
	  write(sprintf("Error in evaluation: %O\n",c));
	}
      }else{
	if(show_result)
	   write("Result: "+replace(a, "\n", "\n        ")+"\n");
	else
	   write("Ok.\n");
	variables=foo->query_variables();
      }
    }
  }

  string input="";
  
  string skipwhite(string f)
  {
    while(sscanf(f,"%*[ \r\n\t]%s",f) && sscanf(f,"/*%*s*/%s",f));
    return f;
  }
  
  int find_next_comma(string s)
  {
    int e, p, q;
    
    for(e=0;e<strlen(s);e++)
    {
      switch(s[e])
      {
	case '"':
	  for(e++;s[e]!='"';e++)
	  {
	    switch(s[e])
	    {
	      case 0: return 0;
	      case '\\': e++; break;
	    }
	  }
	  break;

	case '\'':
	  for(e++;s[e]!='\'';e++)
	  {
	    switch(s[e])
	    {
	      case 0: return 0;
	      case '\\': e++; break;
	    }
	  }
	  break;
	  
	case '{':
	case '(':
	case '[':
	  p++;
	  break;
	  
	case ',':
	  if(!p) return e;
	  break;
	  
	case '/':
	  if(s[e+1]=='*')
	    while(s[e-1..e]!="*/" && e<strlen(s)) e++;
	  break;
	  
	case '}':
	case ')':
	case ']':
	  p--;
	  if(p<0)
	  {
	    write("Syntax errror.\n");
	    input="";
	    return 0;
	  }
	  break;
	  
      }
    }
    return 0;
  }
  
  array(string) get_name(string f)
  {
    int e,d;
    string rest;
    
    f=skipwhite(f);
    if(sscanf(f,"*%s",f)) f=skipwhite(f);
    sscanf(f,"%[a-zA-Z0-9_]%s",f,rest);
    rest=skipwhite(rest);
    return ({f,rest});
  }

  string first_word=0;
  int pos=0;
  int parenthese_level=0;
  int in_comment=0;
  int in_string=0;
  int in_quote=0;
  int in_char=0;
  int eq_pos=-1;
  
  void set_buffer(string s,int parsing)
  {
    input=s;
    first_word=0;
    pos=-1;
    parenthese_level=0;
    in_comment=0;
    in_quote=0;
    in_char=0;
    in_string=0;
    eq_pos=-1;
    if(!parsing) do_parse();
  }
  
  void clean_buffer() { set_buffer("",0);  }

  void add_buffer(string s)
  {
    input+=s;
    do_parse();
    input=skipwhite(input);
  }

  void cut_buffer(int where)
  {
    int old,new;
    old=strlen(input);
    input=skipwhite(input[where..old-1]);
    new=strlen(input);
    if(where>1) first_word=0;
    pos-=old-new;
    if(pos<0) {
#ifdef DEBUG
      write("negative pos: %O\n", pos);
#endif /* DEBUG */
      // Note: -1 since the loop will increase it to 0.
      pos=-1;
    }
    eq_pos-=old-new;
    if(eq_pos<0) eq_pos=-1;
  }
  
  void print_version()
  {
    write(version()+
	  " running Hilfe v2.0 (Incremental Pike Frontend)\n");
  }
  
  
  int do_parse()
  {
    string tmp;
#ifdef DEBUG
    write("do_parse(): input = %O  pos = %o\n", input, pos);
#endif /* DEBUG */
    if(pos<0) pos=0;
    for(;pos<strlen(input);pos++)
    {
      if(in_quote) { in_quote=0; continue; }
//    trace(99);
      if(!first_word)
      {
	int d;
	if(!strlen(input) && pos)
	{
	  werror("Error in optimizer.\n");
	  exit(1);
	}
	
	d=input[pos];
	if((d<=' ' || d==';') && !pos)
	{
	  cut_buffer(1);
	  continue;
	}
	if((d<'a' || d>'z') && (d<'A' || d>'Z') && (d<'0' || d>'9') && d!='_')
	{
	  first_word=input[0..pos-1];
#ifdef DEBUG
	  write("First = %O  pos = %O\n", first_word, pos);
	  write("input = %O\n", input);
#endif
	  switch(first_word)
	  {
	    case "quit":
	      write("Exiting.\n");
	      exit(0);
	    case ".":
	      clean_buffer();
	      write("Input buffer flushed.\n");
	      continue;
	      
	    case "new":
	      __INIT();
	      continue;
	      
	    case "dump":
	      sum_arrays(lambda(string var,mixed foo)
			 {
			   write(sprintf("%-15s:%s\n",var,sprintf("%O",foo)));
			 },
			   indices(variables),
			   values(variables));
	      cut_buffer(4);
	      continue;
	      
	    case "help":
	      print_version();
	      write("Hilfe is a tool to evaluate Pike interactively and incrementally.\n"
		    "Any Pike function, expression or variable declaration can be entered\n"
		    "at the command line. There are also a few extra commands:\n"
		    " help       - show this text\n"
		    " quit       - exit this program\n"
		    " .          - abort current input batch\n"
		    " dump       - dump variables\n"
		    " new        - clear all function and variables\n"
		    "See the Pike reference manual for more information.\n"
		);
	      cut_buffer(4);
	      continue;
	      
	  }
	}
      }
      
      switch(input[pos])
      {
	case '\\':
	  in_quote=1;
	  break;

	case '\'':
	  if(in_quote || in_comment || in_string) break;
	  in_char=!in_char;
	  break;
	  
	case '=':
	  if(in_char || in_string || in_comment  || parenthese_level ||
	     eq_pos!=-1) break;
	  eq_pos=pos;
	  break;
	  
	case '"':
	  if(in_char || in_quote || in_comment) break;
	  in_string=!in_string;
	  break;
	  
	case '{':
	case '(':
	case '[':
	  if(in_char || in_string || in_comment) break;
	  parenthese_level++;
	  break;
	  
	case '}':
	case ')':
	case ']':
	  if(in_char || in_string || in_comment) break;
	  if(--parenthese_level<0)
	  {
	    write("Syntax error.\n");
	    clean_buffer();
	    return 0;
	  }
	  if(!parenthese_level && input[pos]=='}')
	  {
	    if(tmp=parse_function(input[0..pos]))
	    {
	      cut_buffer(pos+1);
	      if(stringp(tmp))
		set_buffer(tmp+input,1);
	    }
	  }
	  break;
	  
	case ';':
	  if(in_char || in_string || in_quote || in_comment || parenthese_level) break;
	  if(tmp=parse_statement(input[0..pos]))
	  {
	    cut_buffer(pos+1);
	    if(stringp(tmp))
	      set_buffer(tmp+input,1);
	  }
	  break;
	  
	case '*':
	  if(in_char || in_string || in_comment) break;
	  if(input[pos-1]=='/') in_comment=1;
	  break;
	  
	case '/':
	  if(in_char || in_string) break;
	  if(input[pos-1]=='*') in_comment=0;
	  break;
      }
#ifdef DEBUG
      write("End of loop: input = %O  pos = %O\n", input, pos);
#endif /* DEBUG */
    }
    if(pos>strlen(input)) pos=strlen(input);
    return -1;
  }


  mixed parse_function(string fun)
  {
    string name,a,b;
    object foo;
    mixed c;
#ifdef DEBUG
    write("Parsing block ("+first_word+")\n");
#endif
    
    switch(first_word)
    {
      case "if":
      case "for":
      case "do":
      case "while":
      case "foreach":
	/* parse loop */
	do_evaluate("mixed ___Foo4711() { "+fun+" ; }\n",0);
	return 1;
	
      case "int":
      case "void":
      case "object":
      case "array":
      case "mapping":
      case "string":
      case "multiset":
      case "float":
      case "mixed":
      case "program":
      case "function":
      case "class":
	/* parse function */
	if(eq_pos!=-1) break;  /* it's a variable */
	sscanf(fun,first_word+"%s",name);
	
	c=get_name(name);
	name=c[0];
	c=c[1];
	
	int i;
	if((i=search(function_names,name))!=-1)
	{
	  b=functions[i];
	  functions[i]=fun;
	  if(!eval(""))  functions[i]=b;
	}else{
	  if(eval(fun))
	  {
	    functions+=({fun});
	    function_names+=({name});
	  }
	}
	return 1;
    }
  }

  mixed parse_statement(string ex)
  {
    string a,b,name;
    mixed c;
    object foo;
    int e;
#ifdef DEBUG
    write("Parsing statement ("+first_word+")\n");
#endif
    switch(first_word)
    {
      // Things to implement:
      // import and inherit

      case "if":
      case "for":
      case "do":
      case "while":
      case "foreach":
	/* parse loop */
	do_evaluate("mixed ___Foo4711() { "+ex+" ; }\n",0);
	return 1;


      case "inherit":
      {
	/* parse variable def. */
	sscanf(ex,first_word+"%s",b);
	b=skipwhite(b);

	imports_and_inherits+=({"inherit "+b+";\n"});
	if(!eval(""))
	{
	  imports_and_inherits=imports_and_inherits[..sizeof(imports_and_inherits)-2];
	}
	return 1;
      }

      case "import":
      {
	sscanf(ex,first_word+"%s",b);
	b=skipwhite(b);
	if(object o=eval("mixed ___Foo4711() { return "+b+"; }\n"))
	{
	  mixed const_value=o->___Foo4711();
//	  werror("%O\n",const_value);
	  string name="___import"+sizeof(imports_and_inherits);
	  constants[name]=const_value;
	  imports_and_inherits+=({"import ___hilfe."+name+";\n"});
	  if(!eval(""))
	  {
	    m_delete(constants,name);
	    imports_and_inherits=imports_and_inherits[..sizeof(imports_and_inherits)-2];
	  }
	}
	return 1;
      }
	
      case "constant":
      {
	/* parse variable def. */
	sscanf(ex,first_word+"%s",b);
	b=skipwhite(b);
	c=get_name(b);
	name=c[0];
	sscanf(c[1],"=%s",c[1]);
	mixed old_value=constants[name];
	if(object o=eval("mixed ___Foo4711() { return "+c[1]+"; }\n"))
	{
	  mixed const_value=o->___Foo4711();
	  constants[name]=const_value;
	  if(!eval(""))
	    constants[name]=old_value;
	}
	return 1;
      }
	
      case "int":
      case "void":
      case "object":
      case "array":
      case "mapping":
      case "string":
      case "multiset":
      case "float":
      case "mixed":
      case "program":
      case "function":
	/* parse variable def. */
	sscanf(ex,first_word+"%s",b);
	b=skipwhite(b);
	c=get_name(b);
	name=c[0];
	c=c[1];
	
#ifdef DEBUG
	write("Variable def.\n");
#endif
	if(name=="") 
	{
	  return 1;
	}else{
	  string f;

	  if (keywords[name]) {
	    werror(sprintf("%s is a reserved word\n", name));
	    return 1;
	  }

	  variables[name]=0;
	  
	  if(sscanf(c,"=%s",c))
	  {
#ifdef DEBUG
	    write("Variable def. with assign. ("+name+")\n");
#endif
	    if(e=find_next_comma(c))
	    {
	      return name+"="+c[0..e-1]+";\n"+
		first_word+" "+c[e+1..strlen(c)-1];
	    }else{
	      return name+"="+c;
	    }
#ifdef DEBUG
	    write("Input buffer = '"+input+"'\n");
#endif
	    
	  }else{
	    sscanf(c,",%s",c);
	    return first_word+" "+c;
	  }
	}
	
	return 1;
	
      default:
	if(ex==";") return 1;
	/* parse expressions */
	do_evaluate("mixed ___Foo4711() { return (mixed)("+ex[0..strlen(ex)-2]+"); }\n",1);
	return 1;
    }
  }
  
  void add_input_line(string s)
  {
    string a,b,c,f,name;
    array(string) tmp;
    int e,d;
    object foo;
    
    s=skipwhite(s);
    
    if(s[0..1]==".\n")
    {
      clean_buffer();
      write("Input buffer flushed.\n");
      s=s[2..strlen(s)-1];
    }
    add_buffer(s);
//  if(!strlen(input))  write("> ");
  }

  void create()
  {
    print_version();
  }
}

class StdinHilfe
{
  inherit Evaluator;
  
  object(Stdio.Readline) readline;

  void save_history()
  {
    catch {
      if(string home=getenv("HOME")||getenv("USERPROFILE"))
      {
	rm(home+"/.hilfe_history~");
	if(object f=Stdio.File(home+"/.hilfe_history~","wct"))
	{
	  f->write(readline->get_history()->encode());
	  f->close();
	}
	rm(home+"/.hilfe_history");
	mv(home+"/.hilfe_history~",home+"/.hilfe_history");
#if constant(chmod)	
	chmod(home+"/.hilfe_history", 0600);
#endif	
      }
    };
  }
  
  void signal_trap(int s)
  {
    clean_buffer();
    save_history();
    exit(1);
  }
  
  void create()
  {
    write=predef::write;
    ::create();
    
    if(string home=getenv("HOME")||getenv("USERPROFILE"))
    {
      if(string s=Stdio.read_file(home+"/.hilferc"))
      {
	add_buffer(s);
      }
    }
    
    readline = Stdio.Readline();
    array(string) hist;
    catch{
      if(string home=getenv("HOME")||getenv("USERPROFILE"))
      {
	if(object f=Stdio.File(home+"/.hilfe_history","r"))
	{
	  string s=f->read()||"";
	  hist=s/"\n";
	  readline->enable_history(hist);
	}
      }
    };
    if(!hist)
      readline->enable_history(512);
    signal(signum("SIGINT"),signal_trap);
    
    for(;;)
    {
      readline->set_prompt(strlen(input) ? ">> " : "> ");
      string s=readline->read();
      if(!s)
	break;
//       signal(signum("SIGINT"),signal_trap);
      add_input_line(s+"\n");
//       signal(signum("SIGINT"));
    }
    save_history();
    destruct(readline);
    write("Terminal closed.\n");
  }
}

class GenericHilfe
{
  inherit Evaluator;
  
  void create(object(Stdio.FILE) in, object(Stdio.File) out)
  {
    write=out->write;
    ::create();

    while(1)
    {
      write(strlen(input) ? ">> " : "> ");
      if(string s=in->gets())
      {
	add_input_line(s+"\n");
      }else{
	write("Terminal closed.\n");
	return;
      }
    }
  }
}


class GenericAsyncHilfe
{
  inherit Evaluator;
  object(Stdio.File) infile, outfile;

  string outbuffer="";
  void write_callback()
  {
    int i=outfile->write(outbuffer);
    outbuffer=outbuffer[i..];
  }
  void send_output(string s, mixed ... args)
  {
    outbuffer+=sprintf(s,@args);
    write_callback();
  }

  string inbuffer="";
  void read_callback(mixed id, string s)
  {
    inbuffer+=s;
    foreach(inbuffer/"\n",string s)
      {
	add_input_line(s);
	write(strlen(input) ? ">> " : "> ");
      }
  }

  void close_callback()
  {
    write("Terminal closed.\n");
    destruct(this_object());
    destruct(infile);
    if(outfile) destruct(outfile);
  }
  
  void create(object(Stdio.File) in, object(Stdio.File) out)
  {
    infile=in;
    outfile=out;
    in->set_nonblocking(read_callback, 0, close_callback);
    out->set_write_callback(write_callback);

    write=send_output;
    ::create();
    write(strlen(input) ? ">> " : "> ");
  }
}

