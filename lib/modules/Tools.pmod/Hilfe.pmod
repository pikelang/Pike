#pike __REAL_VERSION__

//
// Incremental Pike Evaluator
//

constant cvs_version = ("$Id: Hilfe.pmod,v 1.117 2004/05/23 17:51:53 nilsson Exp $");
constant hilfe_todo = #"List of known Hilfe bugs/room for improvements:

- Hilfe can not handle enums.
- Hilfe can not handle typedefs.
- Hilfe can not handle implicit lambdas.
- Hilfe can not handle unnamed classes.
- Hilfe can not handle named lambdas.
- Hilfe should possibly handle imports better, e.g. overwrite the
  local variables/constants/functions/programs.
- Filter exit/quit from history. Could be done by adding a 'pop'
  method to Readline.History and calling it from StdinHilfe's
  destroy.
- Add some better multiline edit support.
- Tab completion of variable and module names.
";

// The Big To Do:
// The parser part of Hilfe should really be redesigned once again.
// The user input is first run through Parser.Pike.split and outputted
// as a token stream. This stream is fed into a streming parser which
// then relocates the variables and outputs expression objects with
// evaluation destinations already assigned. Note that the streaming
// parser can not start on the next expression before the previous
// expression has been evaulated, because the variable table might not
// be up to date.

//! Abstract class for Hilfe commands.
class Command {

  //! Returns a one line description of the command. This help should
  //! be shorter than 54 characters.
  string help(string what);

  //! A more elaborate documentation of the command. This should be
  //! less than 68 characters per line.
  string doc(string what, string with) { return help(what); }

  //! The actual command callback. Messages to the user should be
  //! written out by using the safe_write method in the @[Evaluator]
  //! object.
  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens);
}


//
// Built in commands
//

//! Variable reset command. Put ___Hilfe->commands->reset =
//! Tools.Hilfe.CommandReset(); in your .hilferc to have this command
//! defined when you open Hilfe.
class CommandReset {
  inherit Command;
  string help(string what) { return "Undefines the given symbol."; }
  string doc(string what, string with) {
    return "Undefines any variable, constant, function or program, "
      "specified by\nname. Example: \"reset tmp\"\n";
  }

  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens) {
    string n = sizeof(words)>1 && words[1];
    if(!n) {
      e->safe_write("No symbol given as argument to reset.\n");
      return;
    }
    if(zero_type(e->variables[n]) && zero_type(e->constants[n]) &&
       zero_type(e->functions[n]) && zero_type(e->programs[n])) {
      e->safe_write("Symbol %O not defined.\n", n);
      return;
    }

    m_delete(e->variables, n);
    m_delete(e->types, n);
    m_delete(e->constants, n);
    m_delete(e->functions, n);
    m_delete(e->programs, n);
  }
}

//! Helper function that formats a time span in nanoseconds to
//! something more human readable (ns, ms or s).
string format_hr_time(int i) {
  if(i<1000) return i+"ns";
  if(i<1000000) return sprintf("%.2fms", i/1000.0);
  return sprintf("%.2fs", i/1000000.0);
}

private class CommandSet {
  inherit Command;

  string help(string what) { return "Change Hilfe settings."; }
  string doc(string what, string with) {
    if(with=="format")
      return documentation_set_format;
    return documentation_set;
  }

  private void bench_reswrite(function(string, mixed ... : int) w,
			      string sres, int num, mixed res,
			      int last_compile_time, int last_eval_time) {
    if(!sres)
      w( "Compilation: %s, Execution: %s\n",
	 format_hr_time(last_compile_time),
	 format_hr_time(last_eval_time) );

    else
      w( "Result %d: %s\nCompilation: %s, Execution: %s\n",
	 num, replace(sres, "\n", "\n        "+(" "*sizeof(""+num))),
	 format_hr_time(last_compile_time),
	 format_hr_time(last_eval_time) );
  }

  private class Reswriter (string format) {
    void `()(function(string, mixed ... : int) w, string sres, int num,
	     mixed res, int last_compile_time, int last_eval_time) {
      mixed err = catch {
	if(!sres)
	  w("Ok.\n");
	else
	  w(format, sres, num, res,
	    format_hr_time(last_compile_time),
	    format_hr_time(last_eval_time),
	    last_compile_time, last_eval_time);
      };
      if(err)
	w("Hilfe Error: Could not format result.\n%s\n",
	  describe_backtrace(err));
    }
  }

  private class Intwriter (string type, function fallback) {
    void `()(function(string, mixed ... : int) w, string sres, int num,
	     mixed res, int last_compile_time, int last_eval_time) {
      if(res && intp(res)) {
	if(type=="b") {
	  string s = sprintf("%b", res);
	  s = "0"*(8-sizeof(s)%8) + s;
	  w( s/8*" " + "\n" );
	}
	else
	  w("%"+type+"\n", res);
      }
      else
	fallback(w, sres, num, res, last_compile_time, last_eval_time);
    }
  }

  private array my_indices(string|mapping|multiset|object|program in) {
    if(objectp(in) || programp(in))
      return sort(indices(in));
    return indices(in);
  }

  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens) {

    line = sizeof(words)>1 && words[1];
    function(string, mixed ... : void) write = e->safe_write;

    if(!line) {
      write("No setting to change given.\n");
      return;
    }

    int(0..1) arg_check(string arg) {
      if(line!=arg) return 0;
      if(sizeof(words)<3) {
	write("Insufficient number of arguments to set %s.\n", line);
	line = "";
	return 0;
      }
      return 1;
    };

    if(words[1]=="hedda") {
      mapping(string:string) vars = ([
	"foo":"mixed",
	"bar":"mixed",
	"i":"int",
	"s":"string",
	"f":"float",
	"m":"mapping",
	"a":"array"
      ]);
      mapping vals = ([	"s":"", "f":0.0, "m":([]), "a":({}) ]);
      foreach(vars; string name; string type)
	if(!e->variables[name]) {
	  e->variables[name] = vals[name];
	  e->types[name] = type;
	}
      e->functions->indices = my_indices;
      return;
    }

    if(arg_check("warnings")) {
      switch( words[2] ) {
      case "strict":
	e->warnings = 1;
	e->strict_types = 1;
	return;
      case "on":
	e->warnings = 1;
	e->strict_types = 0;
	return;
      default:
	e->warnings = 0;
	e->strict_types = 1;
	return;
      }
    }

    if(arg_check("trace")) {
      e->trace_level = (int)words[2];
      return;
    }

    if(arg_check("assembler_debug")) {
#if constant(_assembler_debug)
      e->assembler_debug_level = (int)words[2];
#else
      write("Assembler debug not available.\n");
#endif
      return;
    }

    if(arg_check("compiler_trace")) {
#if constant(_compiler_trace)
      e->compiler_trace_level = (int)words[2];
#else
      write("Compiler trace not available.\n");
#endif
      return;
    }

    if(arg_check("debug")) {
#if constant(_debug)
      e->debug_level = (int)words[2];
#else
      write("Debug not available.\n");
#endif
      return;
    }

    if(arg_check("history")) {
      e->history->set_maxsize((int)words[2]);
      return;
    }

    if(arg_check("format")) {
      switch(words[2]) {
      case "default":
	e->reswrite = e->std_reswrite;
	return;
      case "bench":
	e->reswrite = bench_reswrite;
	return;
      case "bin":
	e->reswrite = Intwriter("b", e->std_reswrite);
	return;
      case "oct":
	e->reswrite = Intwriter("o", e->std_reswrite);
	return;
      case "hex":
	e->reswrite = Intwriter("x", e->std_reswrite);
	return;
      case "sprintf":
	string f;
	foreach(tokens, string token)
	  if(token[0]=='"') f=token;
	if(!f)
	  write("No formatting string given.\n");
	else {
	  // FIXME: We should do a real string compilation.
	  f = replace(f, ([ "\\n":"\n", "\\\"":"\"" ]) );
	  e->reswrite = Reswriter(f[1..sizeof(f)-2]);
	}
	return;
      }
      write("No result presentation format %O defined.\n", words[2]);
      return;
    }

    if(line=="") return;
    write("No setting named %O exists.\n", line);
    // FIXME: Allow user to change prompt (>/>>)?
  }
}

private class CommandExit {
  inherit Command;
  string help(string what) { return "Exit Hilfe."; }

  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens) {
    e->safe_write("Exiting.\n");
    destruct(e);
    exit(0);
  }
}

private class CommandHelp {
  inherit Command;
  string help(string what) { return "Show help text."; }

  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens) {
    line = words[1..]*" ";
    function(string, mixed ... : void) write = e->safe_write;

    switch(line) {

    case "me more":
      write( documentation_help_me_more );
      return;

    case "hilfe todo":
      write(hilfe_todo);
      return;

    case "about hilfe":
      e->print_version();
      write(cvs_version+#"
Initial version written by Fredrik Hübinette 1996-2000
Rewritten by Martin Nilsson 2002
");
      return;
    }

    if(sizeof(words)>1 && e->commands[words[1]]) {
      string ret = e->commands[words[1]]->doc(words[1], words[2..]*"");
      if(ret) write(ret);
      return;
    }

    write("\n");
    e->print_version();
    write( #"Hilfe is a tool to evaluate Pike code interactively and
incrementally. Any Pike function, expression or variable declaration
can be entered at the command line. There are also a few extra
commands:

");

    array err = ({});
    foreach(sort(indices(e->commands)), string cmd) {
      string ret;
      err += ({ catch( ret = e->commands[cmd]->help(cmd) ) });
      if(ret)
	write(" %-10s - %s\n", cmd, ret);
    }

    write( #" .          - Abort current input batch.

Enter \"help me more\" for further Hilfe help.
");

    foreach(err-({0}), mixed err)
      write(describe_backtrace(err)+"\n\n");
  }
}

private class CommandDot {
  inherit Command;
  string help(string what) { return 0; }

  private constant usr_vector_a = ({
    89, 111, 117, 32, 97, 114, 101, 32, 105, 110, 115, 105, 100, 101, 32, 97,
    32, 72, 105, 108, 102, 101, 46, 32, 73, 116, 32, 115, 109, 101, 108, 108,
    115, 32, 103, 111, 111, 100, 32, 104, 101, 114, 101, 46, 32, 89, 111,
    117, 32, 115, 101, 101, 32 });
  private constant usr_vector_b = ({
    32, 89, 111, 117, 32, 99, 97, 110, 32, 103, 111, 32, 105, 110, 32, 97,
    110, 121, 32, 100, 105, 114, 101, 99, 116, 105, 111, 110, 32, 102, 114,
    111, 109, 32, 104, 101, 114, 101, 46 });
  private constant usr_vector_c = ({
    32, 89, 111, 117, 32, 97, 114, 101, 32, 99, 97, 114, 114, 121, 105, 110,
    103, 32 });
  private constant usr_vector_d = usr_vector_c[..8] + ({
    101, 109, 112, 116, 121, 32, 104, 97, 110, 100, 101, 100, 46 });

  private array(string) thing(array|mapping|object thing, string what,
			      void|string a, void|string b) {
    if(!sizeof(thing)) return ({});
    return ({ sizeof(thing)+" "+what+(sizeof(thing)==1?(a||""):(b||"s")) });
  }

  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens) {
    string ret = (string)usr_vector_a;

    array(string) tmp = ({});
    tmp += thing(e->imports, "import");
    tmp += thing(e->inherits, "inherit");
    tmp += thing(e->history, "history entr", "y", "ies");
    if(sizeof(tmp))
      ret += String.implode_nicely(tmp) + ".";
    else
      ret += "nothing.";

    tmp = ({});
    tmp += thing(e->variables, "variable");
    tmp += thing(e->constants, "constant");
    tmp += thing(e->functions, "function");
    tmp += thing(e->programs, "program");
    if(sizeof(tmp))
      ret += (string)usr_vector_c + String.implode_nicely(tmp) + ".";
    else
      ret += (string)usr_vector_d;

    ret += (string)usr_vector_b;
    e->safe_write("%-=67s\n", ret);
  }
}

class CommandDump {
  inherit Command;

  private function(string, mixed ... : int) write;

  string help(string what) { return "Dump variables and other info."; }
  string doc(string what, string with) { return documentation_dump; }

  private void wrapper(Evaluator e) {
    if(!e->last_compiled_expr) {
      write("No wrapper compiled so far.\n");
      return;
    }
    write("Last compiled wrapper:\n");
    int i;
    string w = map(e->last_compiled_expr/"\n",
		   lambda(string in) {
		     return sprintf("%03d: %s", ++i, in);
		   })*"\n";
    write(w+"\n");
  }

  private string print_mapping(array(string) ind, array val) {
    int m = max( @filter(map(ind, sizeof), `<, 20), 8 );
    foreach(ind; int i; string name)
      write("%-*s : %s\n", m, name, replace(sprintf("%O", val[i]), "\n",
					    "\n"+(" "*m)+"   "));
  }

  private void dump(Evaluator e) {
    if(sizeof(e->constants)) {
      write("\nConstants:\n");
      array(string) a=indices(e->constants);
      array b=values(e->constants);
      sort(a,b);
      print_mapping(a,b);
    }
    if(sizeof(e->variables)) {
      write("\nVariables:\n");
      array(string) a=indices(e->variables);
      array b=values(e->variables);
      sort(a,b);
      a = map(a, lambda(string in) { return e->types[in] + " " + in; } );
      print_mapping(a,b);
    }
    if(sizeof(e->functions)) {
      write("\nFunctions:\n");
      foreach(sort(indices(e->functions)), string name)
	write("%s\n", name);
    }
    if(sizeof(e->programs)) {
      write("\nPrograms:\n");
      foreach(sort(indices(e->programs)), string name)
	write("%s\n", name);
    }
    if(sizeof(e->inherits)) {
      write("\nInherits:\n" + e->inherits*"\n" + "\n");
    }
    if(sizeof(e->imports)) {
      write("\nImports:\n" + e->imports*"\n" + "\n");
    }
  }

  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens) {
    write = e->safe_write;

    line = words[1..]*"";
    switch( line ) {
    case "wrapper":
      wrapper(e);
      return;
    case "state":
      write(e->state->status());
      return;
    case "history":
      write(e->history->status());
      return;
    case "memory":
      write(master()->resolv("Debug.pp_memory_usage")());
      return;
    case "":
      dump(e);
      return;
    }
    write("Unknown dump specifier.\n");
    write(doc(0,0)+"\n");
  }
}

private class CommandHej {
  inherit Command;
  string help(string what) { return 0; }
  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens) {
    if(line[0]=='.') e->safe_write( (string)({ 84,106,97,98,97,33,10 }) );
  }
}

private class CommandNew {
  inherit Command;
  string help(string what) { return "Clears the Hilfe state."; }
  string doc(string what, string with) { return documentation_new; }

  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens) {

    line = sizeof(words)>1 && words[1];
    switch(line) {
    case "variables":
      e->types = ([]);
    case "constants":
    case "functions":
    case "programs":
      e[line] = ([]);
      return;

    case "imports":
    case "inherits":
      e[line] = ({});
      return;

    case "history":
      e->history->flush();
      return;
    }

    if(line) {
      e->safe_write("Unknown specifier %O.\n", line);
      return;
    }
    e->reset_evaluator();
  }
}

private class CommandStartStop {
  inherit Command;
  SubSystems subsystems;

  void create() {
    subsystems=SubSystems();
  }

  string help(string what) {
    switch(what){
    case "start":
      return "Start a subsystem.";
    case "stop":
      return "Stop a subsystem.";
    }
  }

  string doc(string what, string with) {
    if(what=="start")
      return subsystems->doc(1);
    if(what=="stop")
      return subsystems->doc(0);
  }

  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens) {
    if(sizeof(words)>=2) {
      subsystems[words[0]](e, words[1], words[1..]);
      return;
    }
    e->safe_write("No subsystem selected. Available subsystems:\n");
    e->safe_write("%{   %s\n%}", subsystems->list());
  }
}


//
// Backend subsystem
//

#if constant(thread_create)
private class SubSysBackend {
  int(0..1) is_running;

  constant startdoc = "backend [once]\n"
  "\tStarts the backend thread. If \"once\" is specified execution\n"
  "\twill end at first exception. Can be restarted with \"start backend\".\n";

  constant stopdoc = "backend\n\tstop the backend thread.\n";

  void create(){
    is_running=0;
  }

  void start(Evaluator e, array(string) words){
    int(0..1) once = (sizeof(words)>=2 && words[1]=="once");
    add_constant("backend_thread",
		 thread_create(backend_loop, e->safe_write, once));
  }

  void stop(Evaluator e, array(string) words){
    call_out(throw, 0, 0);
  }

  int(0..1) runningp() {
    return is_running;
  }

  private void backend_loop(function(string:int) write_err, int(0..1) once){
    is_running=1;
    object backend = master()->resolv("Pike.DefaultBackend");
    mixed err;
    do {
      err = catch {
	while(1)
	  backend(3600.0);
      };
      if(err)
	write_err(describe_backtrace(err));
    } while(!once && err);
    if(once && err)
      write_err("Backend done.\r\n");
    is_running=0;
  }

}
#endif // constant(thread_create)

private class SubSysLogger {

  constant startdoc = "logging [<filename>]\n"
  "\tLogs all input and output to a log file. If no file name is \n"
  "\tspecified logging will be appended to hilfe.log in the current\n"
  "\twork directory.\n";

  constant stopdoc = "logging\n\tTurns off logging to file.\n";
  int(0..1) running;

  static class Logger {

    Stdio.File logfile;
    Evaluator e;
    constant is_logger = 1;

    void create(Evaluator _e, Stdio.File _logfile) {
      e = _e;
      logfile = _logfile;
      e->add_input_hook(this);
      running = 1;
    }

    void destroy() {
      e && e->remove_input_hook(this);
      running = 0;
    }

    int(0..0) `() (string in) {
      if(!running) return 0;
      if(catch( logfile->write(in) )) {
	e->remove_writer(this);
	e->safe_write("Error writing to log file. Terminating logger.\n");
      }
      return 0;
    }

  }

  void start(Evaluator e, array(string) words) {
    string fn = sizeof(words)>1 ? words[1] : "hilfe.log";
    Stdio.File logfile = Stdio.File( fn, "wac" );
    if(!logfile) {
      e->safe_write("Hilfe Error: Could not open log file %O.\n", fn);
      return;
    }
    e->safe_write("Logging to log file %O.\n", fn);
    e->add_writer( Logger(e, logfile) );
    e->safe_write("Hilfe logger activated "+ctime(time()));
  }

  void stop(Evaluator e, void|array(string) words) {
    running = 0;
    if(arrayp(e->write)) {
      foreach([array]e->write, mixed w)
	if(objectp(w) && ([object]w)->is_logger)
	  e->remove_writer([object(Logger)]w);
    }
    else if(objectp(e->writer) && ([object]e->writer)->is_logger)
      destruct([object]e->writer);
  }

  int(0..1) runningp() { return running; }
}

private class SubSysPhish {

  constant startdoc = "phish\n"
    "\tStart the Pike Hilfe Shell.\n";
  constant stopdoc = "phish\n\tTurns off phish.\n";

  static int(0..1) running;
  static int(0..1) in_expr;
  static Evaluator e;

  int(0..1) runningp() { return running; }

  static int(0..1) do_cmd(string cmd) {
    if(in_expr) {
      return 0;
    }

    array c = cmd[..sizeof(cmd)-2]/" ";
    if(e->commands[c[0]] || cmd==".\n")
      return 0;

    string src = c[0]+"(";
    array arg = ({});
    for(int i=1; i<sizeof(c); i++) {
      if(c[i][0]=="\"") {
	string str = "";
	do {
	  str += c[i++];
	} while(i+1<sizeof(c) && c[i+1][-1]!="\"");
	arg += ({ str });
      }
      else
	arg += ({ "\"" + c[i] + "\"" });
    }
    e->add_buffer(src + arg*"," + ");\n");
    return 1;
  }

  void start(Evaluator _e, array(string) words) {
    if(running) return;
    e = _e;
    e->add_input_hook(do_cmd);
    running = 1;
  }

  void stop(Evaluator e, void|array(string) words) {
    e->remove_input_hook(do_cmd);
    running = 0;
  }
}

//
// Support stuff..
//


// General subsystem handler class.
private class SubSystems {
  mapping(string:object) subsystems;

  void create (){
    // Register the subsystems here.
    subsystems = ([
#if constant(thread_create)
      "backend":SubSysBackend(),
#endif
      "logging":SubSysLogger(),
      "phish":SubSysPhish(),
    ]);
  }

  array(string) list() {
    return indices(subsystems);
  }

  string doc(int(0..1) startp) {
    array(string) res = ({});
    string what = (startp?"startdoc":"stopdoc");
    foreach(sort(indices(subsystems)), string name)
      res += ({ (startp?" start ":" stop ") +
	replace(subsystems[name][what], "\t", "    ") });
    return res*"\n";
  }

  void start(Evaluator e, string what, array(string) words){
    if(subsystems[what]) {

      if(!subsystems[what]->runningp())
	subsystems[what]->start(e, words);
      else
	e->safe_write("%s is already running.\n", what);

    }
    else
      e->safe_write("No such subsystem (%O).\n", what);
  }

  void stop(Evaluator e, string what, array(string) words){
    if(subsystems[what]) {

      if(subsystems[what]->runningp())
	subsystems[what]->stop(e,words);
      else
	e->safe_write("%s is not running.\n", what);
    }
    else
      e->safe_write("No such subsystem (%O).\n", what);
  }
}

private constant whitespace = (< ' ', '\n' ,'\r', '\t' >);
private constant termblock = (< "catch", "do", "gauge", "lambda",
				"class stop" >);
private constant modifier = (< "extern", "final", "inline", "local", "nomask",
			       "optional", "private", "protected", "public",
			       "static", "variant" >);
private constant notype = (< "(", ")", "->", "[", "]", ":", ";",
			     "+", "++", "-", "--",
			     "%", "*", "/", "&", "&&", "||", ",",
			     "<", ">", "==", "=", "!=", "?",
			     "+=", "-=", "%=", "/=", "&=", "|=",
			     "~=", "<<", ">>", "<<=", ">>=", "<=",
			     ">=", "^", "^=" >);

private class Expression {
  private array(string) tokens;
  private mapping(int:int) positions;
  private array(int) depths;
  private multiset(int) sscanf_depths;

  void create(array(string) t) {
    tokens = t;

    positions = ([]);
    int pos;

    int depth;
    depths = allocate(sizeof(t));

    sscanf_depths = (<>);

    foreach(t; int i; string t) {
      if(!whitespace[t[0]])
	positions[pos++] = i;

      if(t=="(")
	depth++;
      else if(t==")")
	depth--;
      depths[i]=depth;

      if(t=="sscanf") sscanf_depths[depth+1]=1;
    }
  }

  int _sizeof() {
    return sizeof(positions);
  }

  // Returns a token or a token range without whitespaces.
  string `[](int f, void|int t) {
    if(f>sizeof(positions) || -f>sizeof(positions))
      return 0;
    if(!t)
      return tokens[positions[f]];
    if(t>=sizeof(positions))
      t = sizeof(positions)-1;

    // Negative t not boundary checked.
    return tokens[positions[f]..positions[t]]*"";
  }

  string `[]= (int f, string v) {
    return tokens[positions[f]] = v;
  }

  int depth(int f) {
    return depths[positions[f]];
  }

  int(0..1) in_sscanf(int f) {
    return sscanf_depths[depth(f)];
  }

  // See if there are any forbidden modifiers used in the expression,
  // e.g. "private int x;" is not valid inside Hilfe.
  string check_modifiers() {
    foreach(sort(values(positions)), int pos)
      if(modifier[tokens[pos]])
	return "Hilfe Error: Modifier \"" + tokens[pos] +
	  "\" not allowed on top level in Hilfe.\n";
      else
	return 0;
    return 0;
  }

  // Returns the expression verbatim.
  string code() {
    return tokens*"";
  }

  // Returns at which position the type declaration that
  // begins at position @[position] ends. A return value of
  // -1 means that the token or tokens from @[position]
  // can not be a type declaration.
  int(-1..) endoftype(int(-1..) position) {
    string t = `[](position);
    if( (< "int", "float", "string",
           "array", "mapping", "multiset",
           "function", "object", "program", "void" >)[ t ] ) {
      // We are in a type declaration.
      position++;

      t = `[](position);
      if( !(< "(", "|" >)[t] )
        return position-1;

      if( t=="(" ) {
	int plevel;
	for(; position<sizeof(positions); position++) {
	  t = `[](position);
	  if( t=="(" ) plevel++;
	  if( t==")" ) plevel--;
	  if( !plevel ) {
	    if( `[](position+1)=="|" )
	      return endoftype(position+2);
	    return position;
	  }
	  // We will not index outside the array,
	  // since "|" can't be the last entry.

	  if( t=="|" ) position++;
	}
      }

      if( t=="|" )
	return endoftype(position+1);

      return position;
    }

    // Any sequence beginning with any of these can't be
    // a type declaration.
    if( (< "break", "continue", "class", "!", "-",
           "(", "~", "[", "`" >)[ t ] )
      return -1;
    if( notype[ t ] )
      return -1;

    if( t=="." ) position++;

    for(; position<sizeof(positions); position++) {
      if( notype[ `[](position+1) ] )
        return -1;
      if( `[](position+1)=="." ) {
        position++;
        continue;
      }

      if( `[](position+1)=="|" ) {
	return endoftype(position+2);
      }

      return position;
    }

    // Well, we did find a type declaration, but it
    // wasn't used for anything except perhaps loading
    // a module into pike, e.g. "spider;"
    return -1;
  }

  // Returns the position of the matching parenthesis of
  // the given kind, starting from the given position. The
  // position should be the position after the opening
  // parenthesis, or later. Assuming balanced code. Returns
  // -1 on failure.
  int(-1..) find_matching(string token, int(0..)|void pos) {
    string find = ([ "(":")", "{":"}", "[":"]",
		     "({":"})", "([":"])", "(<":">)" ])[token];
    int plevel=1;
    while( pos<sizeof(positions) ) {
      if( `[](pos)==token ) plevel++;
      if( `[](pos)==find ) plevel--;
      if(!plevel) return pos;
      pos++;
    }
    return -1;
  }

  // Is there a block starting at @[pos]?
  int(0..1) is_block(int pos) {

    if( (< "for", "foreach", "switch", "while", "lambda", "class",
	   "do", "gauge", "catch" >)[ `[](pos) ] )
      return 1;

    if( `[](pos+1)=="{" && `[](pos)!="(")
      return 1;
    if( `[](pos+1)!="(" )
      return 0;
    pos = find_matching("(", pos+2);
    if( pos==-1 )
      return 0;
    if( `[](pos+1)=="{" )
      return 1;
    return 0;
  }

  string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O)", this_program, tokens);
  }
}

//! In every Hilfe object (@[Evaluator]) there is a ParserState object
//! that manages the current state of the parser. Essentially tokens are
//! entered in one end and complete expressions are outputted in the other.
//! The parser object is accessible as ___Hilfe->state from Hilfe expressions.
private class ParserState {
  private ADT.Stack pstack = ADT.Stack();
  private constant starts = ([ ")":"(", "}":"{", "]":"[",
			       ">)":"(<", "})":"({", "])":"([" ]);
  private array(string) pipeline = ({ });
  private array(Expression) ready = ({ });
  private string last;
  private string block;

  private mapping low_state = ([]);

  //! Feed more tokens into the state.
  void feed(array(string) tokens) {
    foreach(tokens, string token) {
      // comments
      if(sizeof(token)>1 && (token[0..1]=="//" || token[0..1]=="/*")) continue;

      pipeline += ({ token });

      // If we start a block at the uppermost level, what kind of block is it?
      if(token=="{" && !pstack->ptr) {
	if(!block)
	  block = last;
	else if(block=="class" && last=="class")
	  block = "class stop";
	pstack->push(token);
	continue;
      }
      if(token=="lambda" && !pstack->ptr) {
	block = token;
	last = token;
	continue;
      }
      if(token=="class" && !pstack->ptr) {
	if(sizeof(pipeline)>1) {
	  // Kludge to get "object foo=class{}();" past the kludge below.
	  block = "class stop";
	}
	else {
	  // Kludge to get "class A {}" to work without semicolon.
	  block = token;
	}
	last = token;
	continue;
      }

      // Do we begin any kind of parenthesis level?
      if(token=="(" || token=="{" || token=="[" ||
	 token=="(<" || token=="({" || token=="([") {
	pstack->push(token);
	continue;
      }

      // Do we end any kind of parenthesis level?
      if(token==")" || token=="}" || token=="]" ||
	 token==">)" || token=="})" || token=="])" ) {
	if(!pstack->ptr)
	   throw(sprintf("%O end parenthesis without start parenthesis.",
			 token));
	if(pstack->top()!=starts[token])
	   throw(sprintf("%O end parenthesis does not match closest start "
			 "parenthesis %O.",
			 token, pstack->top()));
	pstack->pop();
      }

      // expressions
      if(token==";" && !pstack->ptr) {
	ready += ({ Expression(pipeline) });
	pipeline = ({});
	block = 0;
	continue;
      }

      // If we end a block at the uppermost level, and it doesn't need a ";",
      // then we can move out that block of the pipeline.
      if(token=="}" && !pstack->ptr && !termblock[block]) {
	ready += ({ Expression(pipeline) });
	pipeline = ({});
	block = 0;
	continue;
      }

      // Preprocessor
      if( token[0]=='#' ) {
	string tmp = token-" ";
	if( has_prefix(tmp, "#error") && !pstack->ptr) {
	  ready += ({ Expression( pipeline ) });
	  pipeline = ({});
	  block = 0;
	  continue;
	}
      }

      if(!whitespace[token[0]])
	last = token;
    }
  }

  //! Read out completed expressions. Returns an array where every element
  //! is an expression represented as an array of tokens.
  array(Expression) read() {
    array(Expression) ret = ({});

    foreach(ready, Expression expr)
      if(expr[0]!=";")
	ret += ({ expr });

    ready = ({});
    return ret;
  }

  private string caught_error;

  //! Prints out any error that might have occured while
  //! @[push_string] was executed. The error will be
  //! printed with the print function @[w].
  void show_error(function(string, mixed ... : int) w) {
    if(!error) return;
    w("Hilfe Error: %s", caught_error);
    caught_error = 0;
  }

  //! Sends the input @[line] to @[Parser.Pike] for tokenization,
  //! but keeps a state between each call to handle multiline
  //! /**/ comments and multiline #"" strings.
  array(string) push_string(string line) {
    array(string) tokens;
    array err;
    if(sizeof(line) && line[0]=='#') {
      string tmp = line-" ";
      if( has_prefix(tmp, "#define") || has_prefix(tmp, "#undef") ) {
	caught_error = "Preprocessor defines not possible inside Hilfe.\n";
	return 0;
      }
      // FIXME: It should be possible to add charset support, but
      // would it be worth it?
      if( has_prefix(tmp, "#charset") ) {
	caught_error = "Preprocessor charset declaration not possible "
	  "inside Hilfe.\n";
	return 0;
      }
      // FIXME: We would like #pike to work.
      if( has_prefix(tmp, "#pike") ) {
	caught_error = "Version selection does not work inside Hilfe.\n";
	return 0;
      }
      if( has_prefix(tmp, "#pragma") ){
	caught_error = "Pragma does not work inside Hilfe.\n";
	return 0;
      }
      if( has_prefix(tmp, "#if") || has_prefix(tmp, "#elif") ||
	  has_prefix(tmp, "#else") || has_prefix(tmp, "#endif") ) {
	caught_error = "Preprocess instructions if, elif, ifdef, ifndef "
	  "else and endif not possible in Hilfe.\n";
	return 0;
      }
    }
    if(err = catch( tokens = Parser.Pike.split(line, low_state) )) {
      caught_error = err[0];
      return 0;
    }
    return tokens;
  }

  //! Returns true if there is any waiting expression that can be fetched
  //! with @[read].
  int datap() {
    if(sizeof(pipeline)==1 && whitespace[pipeline[0][0]])
      pipeline = ({});
    return sizeof(ready);
  }

  //! Are we in the middle of an expression. Used e.g. for changing the
  //! Hilfe prompt when entering multiline expressions.
  int(0..1) finishedp() {
    if(pstack->ptr) return 0;
    if(low_state->in_token) return 0;
    if(!sizeof(pipeline)) return 1;
    if(sizeof(pipeline)==1 && whitespace[pipeline[0][0]]) {
      pipeline = ({});
      return 1;
    }
    return 0;
  }

  //! Clear the current state.
  void flush() {
    pstack->reset();
    pipeline = ({});
    ready = ({});
    last = 0;
    block = 0;
    low_state = ([]);
  }

  //! Returns the current parser state. Used by "dump state".
  string status() {
    string ret = "Current parser state\n";
    ret += sprintf("Parenthesis stack: %s\n", pstack->arr[..pstack->ptr]*" ");
    ret += sprintf("Current pipeline: %O\n", pipeline);
    ret += sprintf("Last token: %O\n", last);
    ret += sprintf("Current block: %O\n", block);
    return ret;
  }
}

//! In every Hilfe object (@[Evaluator]) there is a HilfeHistory
//! object that manages the result history. That history object is
//! accessible both from __ and ___Hilfe->history in Hilfe expressions.
private class HilfeHistory {

  inherit ADT.History;

  // Add content overview
  string status() {
    string ret = "";
    int abs_num = get_first_entry_num();
    int rel_num = -_sizeof();
    for(abs_num; abs_num<get_latest_entry_num()+1; abs_num++)
      ret += sprintf(" %2d (%2d) : %s\n", abs_num, rel_num++,
		     replace(sprintf("%O", `[](abs_num)), "\n",
			     "\n           "));
    ret += sprintf("%d out of %d possible entries used.\n",
		   _sizeof(), get_maxsize());
    return ret;
  }

  // Give better names in backtraces.
  mixed `[](int i) {
    mixed ret;
    array err = catch( ret = ::`[](i) );
    if(err)
      error(err[0]);
    return ret;
  }

  // Give the object a better name.
  string _sprintf(int t) {
    return t=='O' && sprintf("%O(%d/%d)", this_program,
			     _sizeof(), get_maxsize() );
  }
}


//
// The actual Hilfe
//

//! This class implements the actual Hilfe interpreter. It is accessible
//! as ___Hilfe from Hilfe expressions.
class Evaluator {

  //! This mapping contains the available Hilfe commands, including the
  //! built in ones (dump, exit, help, new, quit), so it is possible to
  //! replace or remove them. The name of a command should be 10
  //! characters or less.
  mapping(string:Command) commands = ([]);

  //! Keeps the state, e.g. multiline input in process etc.
  ParserState state = ParserState();

  //! The locally defined variables (name:value).
  mapping(string:mixed) variables;

  //! The types of the locally defined variables (name:type).
  mapping(string:string) types;

  //! The locally defined constants (name:value).
  mapping(string:mixed) constants;

  //! The locally defined functions (name:value).
  mapping(string:function) functions;

  //! The locally defined programs (name:value).
  mapping(string:program) programs;

  //! The current imports.
  array(string) imports;

  //! The current inherits.
  array(string) inherits;

  //! The current result history.
  HilfeHistory history = HilfeHistory(10);

  //! The function to use when writing to the user.
  array|object|function(string : int(0..)) write;

  //! Adds another output function.
  void add_writer(object|function(string : int(0..)) new) {
    if(arrayp(write))
      write += ({ new });
    else if (write)
      write = ({ write, new });
    else
      write = new;
  }

  //! Removes an output function.
  void remove_writer(object|function old) {
    if(arrayp(write))
      write -= ({ old });
    else
      write = 0;
  }

  //! An output method that shouldn't crash.
  int safe_write(string in, mixed ... args) {
    if(!write) return 0;
    mixed err = catch {
      if(sizeof(args))
	in = sprintf(in, @args);
      int ret = write(in);
      if(!ret && sizeof(in)) return sizeof(in);
      return ret;
    };
    catch {
      write("HilfeError: Error while outputting data.\n");
      write(describe_backtrace(err));
      return 0;
    };
    werror("HilfeError: Error while outputting data.\n");
    return 0;
  }

  static array input_hooks = ({});

  //! Adds a function to the input hook, making
  //! all user data be fed into the function.
  //! @seealso
  //!  @[remove_input_hook]
  void add_input_hook(function|object new) {
    input_hooks += ({ new });
  }

  //! Removes a function from the input hook.
  //! @seealso
  //!  @[add_input_hook]
  void remove_input_hook(function|object old) {
    input_hooks -= ({ old });
  }


  //!
  void create()
  {
    print_version();
    commands->set = CommandSet();
    commands->exit = CommandExit();
    commands->quit = commands->exit;
    commands->help = CommandHelp();
    commands->dump = CommandDump();
    commands->new = CommandNew();
    commands->hej = CommandHej();
    commands->look = CommandDot();
    commands->start = CommandStartStop();
    commands->stop = commands->start;
    reset_evaluator();
  }

  //! Displays the current version of Hilfe.
  void print_version()
  {
    safe_write(version()+
	      " running Hilfe v3.5 (Incremental Pike Frontend)\n");
    int major = master()->compat_major;
    int minor = master()->compat_minor;
    if( major!=-1 || minor!=-1 )
      safe_write("(running in Pike %d.%d compat mode)\n", major, minor);
  }

  //! Clears the current state, history and removes all locally
  //! defined variables, constants, functions and programs. Removes
  //! all imports and inherits. It does not reset the command mapping
  //! nor reevaluate the .hilferc file.
  void reset_evaluator() {
    state->flush();
    history->flush();
    variables = ([]);
    types = ([]);
    constants = ([]);
    functions = ([]);
    programs = ([]);
    imports = ({});
    inherits = ({});
  }

  //! Input a line of text into Hilfe. It checks if @[s] is
  //! ".", in which case it calls state->flush(). Otherwise
  //! just calls add_buffer.
  void add_input_line(string s)
  {
    if( sizeof(input_hooks) && `+(@input_hooks(s+"\n")) )
      return;

    if(s==".")
    {
      state->flush();
      return;
    }

    add_buffer(s);
  }

  //! Add buffer tokenizes the input string and determines if the
  //! new line is a Hilfe command. If not, it updates the current
  //! state with the new tokens and sends any and all complete
  //! expressions to evaluation in @[parse_expression].
  void add_buffer(string s)
  {
    // Tokenize the input
    array(string) tokens = state->push_string(s+"\n");
    array(string) words = s/" ";
    string command = words[0];

    // See if first token is a command and not a defined entity.
    if(commands[command] && zero_type(constants[command]) &&
       zero_type(variables[command]) && zero_type(functions[command]) &&
       (sizeof(words)==1 || words[1]!=";")) {
      commands[command]->exec(this, s, words, tokens);
      return;
    }

    // See if the command is executed in overridden mode.
    if(sizeof(command) && command[0]=='.') {
      command = command[1..];
      if(commands[command]) {
	commands[command]->exec(this, s, words, tokens);
	return;
      }
    }

    if(!tokens) {
      state->show_error(safe_write);
      return;
    }

    // Push new tokens into our state.
    mixed err = catch( state->feed(tokens) );
    if(err) {
      if(stringp(err))
	safe_write("Hilfe Error: %s\n", err);
      else
	safe_write(describe_backtrace(err));
      state->flush();
    }

    // See if any complete expressions came out on the other side.
    if(state->datap())
      foreach(state->read(), Expression expression) {
	string ret = parse_expression(expression);
	if(ret) safe_write(ret);
      }
  }


  //
  //
  // Parser code
  //
  //

  private int(0..1) hilfe_error(mixed err) {
    if(!err) return 1;
    mixed err2 = catch {
      if( (arrayp(err) && sizeof(err)==2 && stringp(err[0])) ||
	  (objectp(err) && err->backtrace) ) {
	array files = map(reverse(err[1]), lambda(mixed in) {
					  if(in) return in[0];
					  return 0;
					});
	int pos = search(files, "HilfeInput");
	safe_write(describe_backtrace( ({ err[0],
					  err[1][sizeof(err[1])-pos..] }) ));
     } else
	safe_write("Hilfe Error: Unknown format of thrown error "
		   "(not backtrace).\n(%O)\n", err);
    };
    if(err2)
      safe_write("Hilfe Error: Error while printing backtrace.\n");
    return 0;
  }

  private void add_hilfe_constant(string code, string var) {
    if(object o = hilfe_compile("constant " + code +
				";\nmixed ___HilfeWrapper() { return " +
				var + "; }", var)) {
      hilfe_error( catch( constants[var] = o->___HilfeWrapper() ) );
    }
  }

  private void add_hilfe_variable(string type, string code, string var) {
    int(0..1) existed;
    mixed old_value;
    if(!zero_type(variables[var])) {
      old_value = m_delete(variables, var);
      existed = 1;
    }

    object o = hilfe_compile(type + " " + code +
			     ";\nmixed ___HilfeWrapper() { return " +
			     var + "; }", var);

    if(	o && hilfe_error( catch( variables[var] =
				 o->___HilfeWrapper() ) ) ) {
      types[var] = type;
    }
    else if(existed)
      variables[var] = old_value;
  }

  private void add_hilfe_entity(string type, string code,
				string var, mapping vtype) {
    int(0..1) existed;
    mixed old_value;
    if(vtype[var]) {
      old_value = m_delete(vtype, var);
      existed = 1;
    }

    object o = hilfe_compile(type + " " + code +
			     ";\nmixed ___HilfeWrapper() { return " +
			     var + "; }", var);

    if(	o && hilfe_error( catch( vtype[var] = o->___HilfeWrapper() ) ) )
      return;

    if(existed)
      vtype[var] = old_value;
  }

  // Rewrites "dangerous" tokens (int/string/float-variables) to
  // operate directly in the variable mapping. It rewrites all other
  // variables as well, but we didn't have to.
  private int rel_parser( Expression expr, multiset(string) symbols,
			  void|int p ) {
    int top = !p;
    while( p<sizeof(expr)) {
      if( expr->is_block(p) ) {
	string type = expr[p++];
	multiset(string) new_scope = symbols+(<>);

	// No () for catch, gauge, do and possibly class.
	// Get variable names for next scope clobber.
	if(expr[p]=="(") {
	  p++;

	  switch(type) {

	  case "foreach":
	    p = relocate(expr, symbols, new_scope, p, ",");
	    if(expr[p]==";") {
	      p++;
	      p = relocate(expr, symbols, new_scope, p);
	    }
	    p++;
	    p = relocate(expr, symbols, new_scope, p);
	    p++;
	    break;

	  case "for":
	    p = relocate(expr, symbols, new_scope, p);
	    p++;
	    p = relocate(expr, new_scope, new_scope, p);
	    p++;
	    p = relocate(expr, new_scope, new_scope, p);
	    p++;
	    break;

	  // FIXME: Detect named lambdas.

	  case "lambda":
	  case "class": // Unnamed class
	  default: // Function definition
	    while(expr[p-1]!=")") {
	      p = relocate(expr, symbols, new_scope, p, ",");
	      p++;
	    }
	    break;
	  }

	}
	else if( type=="class" && expr[p+1]=="(" ) {
	  p += 2; // Skip class name
	  while(expr[p-1]!=")") {
	    p = relocate(expr, symbols, new_scope, p, ",");
	    p++;
	  }
	}

	if(expr[p]=="{") {
	  p++;
	  p = rel_parser(expr, new_scope, p);
	  p++;
	  continue;
	}

  	p = relocate(expr, new_scope, 0, p);
  	p++;
  	continue;
      }

      if(expr[p]=="}")
	return p;

      // expr is an expression
      p = relocate(expr, symbols, symbols, p, 0, top);
      p++;
    }
    return p;
  }

#if 0
  // Some debug code to intercept calls to relocate. Aspect
  // Oriented Programming would be handy here...
  private int relocate( Expression expr, multiset(string) symbols,
			multiset(string) next_symbols, int p,
			void|string safe_word, void|int(0..1) top) {
    int op = p;
    //    werror("%O %O %d\n", (symbols?indices(symbols||(<>))*", ":0),
    //    	   (next_symbols?indices(next_symbols||(<>))*", ":0), top );
    p = _relocate(expr, symbols, next_symbols, p, safe_word, top);
    werror(" relocate %O %O\n", op, expr[op..p]);
    return p;
  }
#endif

  private int relocate( Expression expr, multiset(string) symbols,
			multiset(string) next_symbols, int p,
			void|string safe_word, void|int(0..1) top) {
    // Type declaration?
    int pos = expr->endoftype(p);
    if(pos>=0) {
      pos++;

      if( (< ";", ",", "=", ")" >)[expr[pos+1]]) {
	// We are declaring the variable expr[pos]
	while(pos<sizeof(expr)) {
	  int from = pos;
	  int plevel;
	  while( !(< ",", ";",")" >)[expr[pos]] || plevel ) {
	    if(expr[pos]=="(" || expr[pos]=="{") plevel++;
	    else if(expr[pos]==")" || expr[pos]=="}") plevel--;

	    pos++;
	    if(pos==sizeof(expr)) {
	      // Something went wrong. End relocation completely.
	      werror("Variable declaration detection in relocation broke.\n");
	      return pos;
	    }
	  }

	  // Relocate symbols in the variable assignment.
	  for(int i=from+1; i<pos; i++){
	    string t = expr[i];

	    if(expr->in_sscanf(i)) {
	      int nv = expr->endoftype(i);
	      if(nv>=0) {
		if(top) {
		  string var = expr[nv+1];
		  types[var] = expr[i..nv];
		  variables[var] = 0;
		  symbols[var] = 1;
		  for(int j=i; j<=nv; j++)
		    expr[j]="";
		  i=nv;
		  continue;
		}
		else {
		  i=nv+1;
		  continue;
		}
	      }
	    }

	    if(symbols[t]) {
	      if(types[t])
		expr[i] = "(([mapping(string:"+types[expr[i]]+")]___hilfe)->"+
		  t+")";
	      else
		expr[i] = "(___hilfe->"+t+")";
	    }
	  }

	  if(next_symbols)
	    next_symbols[expr[from]] = 0;
	  else
	    symbols[expr[from]] = 0;

	  if(top)
	    symbols[expr[from]] = 1;

	  if( expr[pos]==safe_word || (< ";", ")" >)[expr[pos]] ||
	      plevel<0 )
	    return pos;

	  pos++;
	}
      }
      else {
	// We are declaring a function. Take one step back so that the
	// function name will be the first token to is_block.
	pos--;
      }
      return pos;
    }

    int plevel;
    for( ; p<sizeof(expr); p++) {
      string t = expr[p];

      if( (< "(", "{" >)[t] ) {
  	plevel++;
  	continue;
      }

      if( (< "}", ")" >)[t] ) {
	if(!plevel) return p;
	plevel--;
	continue;
      }

      if( t=="{" )
	error("HilfeError: Error in relocation parser (relocate:'}')");

      if(t=="lambda") return p-1;

      if( !plevel && (t==safe_word || t==";") )
	return p;

      if(expr->in_sscanf(p)) {
	int nv = expr->endoftype(p);
	if(nv>=0) {
	  if(top) {
	    string var = expr[nv+1];
	    types[var] = expr[p..nv];
	    variables[var] = 0;
	    symbols[var] = 1;
	    for(int j=p; j<=nv; j++)
	      expr[j]="";
	    p=nv;
	    continue;
	  }
	  else {
	    p=nv+1;
	    continue;
	  }
	}
      }

      // Rewrite variable
      if(symbols[t]) {
	if(types[expr[p]])
	  expr[p] = "(([mapping(string:"+types[expr[p]]+")]___hilfe)->"+
	    expr[p]+")";
	else
	  expr[p] = "(___hilfe->"+expr[p]+")";
	continue;
      }

      // Skip tokens preceded by . or ->
      if( (< ".", "->" >)[t] ) {
	p++;
	continue;
      }
    }

    return p;
  }

  //! Parses a Pike expression. Returns 0 if everything went well,
  //! or a string with an error message otherwise.
  string parse_expression(Expression expr)
  {
    // Check for modifiers
    expr->check_modifiers();

    // Rewrite variables for the Hilfe wrapper
    rel_parser(expr, (multiset)(indices(variables)) );

    switch(expr[0])
    {
      case "if":
      case "for":
      case "do":
      case "while":
      case "foreach":
	// Parse loops.
	evaluate(expr->code(), 0);
	return 0;

      case "inherit":
      {
	inherits += ({ expr[1..sizeof(expr)-2] });
	if(!hilfe_compile(""))
	  inherits = inherits[..sizeof(inherits)-2];
	return 0;
      }

      case "import":
      {
	imports += ({ expr[1..sizeof(expr)-2] });
	if(!hilfe_compile(""))
	  imports = imports[..sizeof(imports)-2];
	return 0;
      }

      case "constant":
      {
	int pos = 1;

	while(pos<sizeof(expr)) {
	  int from = pos;
	  int plevel;
	  while((expr[pos]!="," && expr[pos]!=";") || plevel) {
	    if(expr[pos]=="(") plevel++;
	    else if(expr[pos]==")") plevel--;
	    pos++;
	    if(pos==sizeof(expr))
	      return "Hilfe Error: Bug in constant handling. "
		"Please report this!\n";
	  }
	  add_hilfe_constant(expr[from..pos-1], expr[from]);
	  pos++;
	}

	return 0;
      }

      case "class":

	if(expr[1]=="{") {
	  // Unnamed class ("class {}();").
	  evaluate("return " + expr->code(), 1);
	}
	else
	  add_hilfe_entity("class", expr[1..], expr[1], programs);
	return 0;
    }

    int pos = expr->endoftype(0);
    if(pos>=0) {
      string type = expr[..pos++];

      if( (< ";", ",", "=" >)[expr[pos+1]] ) {
	// We are declaring the variable expr[pos]
	while(pos<sizeof(expr)) {
	  int from = pos;
	  int plevel;
	  while((expr[pos]!="," && expr[pos]!=";") || plevel) {
	    if(expr[pos]=="(" || expr[pos]=="{") plevel++;
	    else if(expr[pos]==")" || expr[pos]=="}") plevel--;
	    pos++;
	    if(pos==sizeof(expr))
	      return "Hilfe Error: Bug in variable handling or error "
		"in variable assignment.\n";
	  }
	  if(constants[expr[from]])
	    return "Hilfe Error: \"" + expr[from] +
	      "\" already defined as constant.\n";
	  add_hilfe_variable(type, expr[from..pos-1], expr[from]);
	  pos++;
	}
	return 0;
      }

      if( expr[pos+1]=="(" ) {
	// We are defining the function expr[pos]
	if(constants[expr[pos]])
	  return "Hilfe Error: \"" + expr[pos] +
	    "\" already defined as constant.\n";
	add_hilfe_entity(type, expr[pos..], expr[pos], functions);
	return 0;
      }
    }

    // parse expressions
    evaluate("return " + expr->code(), 1);
    return 0;
  }


  //
  //
  // Compilation/Evaluation code
  //
  //

  //! The last created wrapper in which an expression was evaluated.
  string last_compiled_expr;

  //! The last compile time;
  int(0..) last_compile_time;

  //! The last evaluation time;
  int(0..) last_eval_time;

  //! Strict types?
  int(0..1) strict_types;

  //! Show warnings?
  int(0..1) warnings;

  //! The current trace level.
  int trace_level;
#if constant(_assembler_debug)
  //! The current assembler debug level.
  //! Only available if Pike is compiled with RTL debug.
  int assembler_debug_level;
#endif
#if constant(_compiler_trace)
  //! The current compiler trace level.
  //! Only available if Pike is compiled with RTL debug.
  int compiler_trace_level;
#endif
#if constant(_debug)
  //! The current debug level.
  //! Only available if Pike is compiled with RTL debug.
  int debug_level;
#endif

  //! The standard @[reswrite] function.
  void std_reswrite(function w, string sres, int num, mixed res) {
    if(!sres)
      w("Ok.\n");
    else
      w( "(%d) Result: %s\n", num,
	 replace(sres, "\n", "\n           "+(" "*sizeof(""+num))) );
  }

  //! The function used to write results.
  //! Gets as arguments in order: The safe_write function
  //! (function(string, mixed ...:int), the result as a string (string),
  //! the history entry number (int), the result (mixed), the compilation
  //! time (int) and the evaulation time (int). If the evaluated expression
  //! didn't return anything (e.g. a for loop) then 0 will be given as the
  //! result string.
  function reswrite = std_reswrite;


  private string hch_errors = "";
  private string hch_warnings = "";
  private class HilfeCompileHandler {

    int stack_level;
    void create(int _stack_level) {
      stack_level = _stack_level;
      hch_errors = "";
      hch_warnings = "";
    }

    mapping(string:mixed) hilfe_symbols;

    mapping(string:mixed) get_default_module() {
      return all_constants() + hilfe_symbols;
    }

    string format(string file, int line, string err) {
      if(file=="HilfeInput")
	file = "";
      else
	file = master()->trim_file_name(file)+":";
      if(err[-1]!='\n') err += "\n";
      string linestr = line?(string)line:"-";
      return sprintf(": %s%s:%s", file, linestr, err);
    }

    void compile_error(string file, int line, string err) {
      hch_errors += "Compiler Error" + format(file, line, err);
    }

    void compile_warning(string file, int line, string warn) {
      hch_warnings += "Compiler Warning" + format(file, line, warn);
    }

    int compile_exception (object|array trace)
    {
      if (!objectp (trace) ||
	  !trace->is_cpp_error && !trace->is_compilation_error) {
	// Errors thrown directly by cpp() and compile() are normally not
	// interesting; they've already been reported to compile_error.
	catch {
	  trace = ({trace[0], trace[1][stack_level + 1..]});
	  if (trace[1][0][0] == "Optimizer")
	    // When the compiler evaluates constants there's a
	    // somewhat odd frame "Optimizer:0 0()" at the top.
	    trace[1] = trace[1][1..];
	};
	hch_errors += "Compiler Exception: " + describe_backtrace (trace);
      }
      return 1;
    }

    void show_errors() {
      safe_write(hch_errors);
    }

    void show_warnings() {
      safe_write(hch_warnings);
    }
  };

  //! Creates a wrapper and compiles the pike code @[f] in it.
  //! If a new variable is compiled to be tested, its name
  //! should be given in @[new_var] so that magically defined
  //! entities can be undefined and a warning printed.
  object hilfe_compile(string f, void|string new_var)
  {
    if(new_var && commands[new_var])
      safe_write("Hilfe Warning: Command %O no longer reachable. "
		 "Use %O instead.\n",
		 new_var, "."+new_var);

    if(new_var=="___hilfe" || new_var=="___Hilfe" ||
       new_var=="___HilfeWrapper" ) {
      safe_write("Hilfe Error: Symbol %O must not be defined.\n"
		"             It is used internally by Hilfe.\n", new_var);
      return 0;
    }

    if(new_var=="=") {
      safe_write("Hilfe Error: No variable name specified.\n");
      return 0;
    }

    mapping symbols = constants + functions + programs;

    if(new_var=="__")
      safe_write("Hilfe Warning: History variable __ is no "
		 "longer reachable.\n");
    else if(zero_type(symbols["__"]) && zero_type(variables["__"])) {
      symbols["__"] = history;
    }

    if(new_var=="_")
      safe_write("Hilfe Warning: History variable _ is no "
		 "longer reachable.\n");
    else if(zero_type(symbols["_"]) && zero_type(variables["__"])
	    && sizeof(history)) {
      symbols["_"] = history[-1];
    }

    string prog;
    if(strict_types)
      prog = "#pragma strict_types\n";
    else
      prog = "";

    prog +=
      map(inherits, lambda(string f) { return "inherit "+f+";\n"; }) * "" +

      map(imports, lambda(string f) { return "import "+f+";\n"; }) * "" +

      "mapping(string:mixed) ___hilfe = ___Hilfe->variables;\n# 1\n" + f +
      "\n";

    HilfeCompileHandler handler = HilfeCompileHandler (sizeof (backtrace()));

    handler->hilfe_symbols = symbols;
    handler->hilfe_symbols->___Hilfe = this;
    handler->hilfe_symbols->write = safe_write;

    last_compiled_expr = prog;
    program p;
    mixed err;

#if constant(_assembler_debug)
    if(assembler_debug_level)
      _assembler_debug(assembler_debug_level);
#endif
#if constant(_compiler_trace)
    if(compiler_trace_level)
      _compiler_trace(compiler_trace_level);
#endif

    last_compile_time = gethrtime();
    err = catch(p=compile_string(prog, "HilfeInput", handler));
    last_compile_time = gethrtime()-last_compile_time;

#if constant(_assembler_debug)
    _assembler_debug(0);
#endif
#if constant(_compiler_trace)
    _compiler_trace(0);
#endif

    if(warnings||err)
      handler->show_warnings();
    if(err) {
      handler->show_errors();
      return 0;
    }

    object o;
    if(hilfe_error( catch(o=p()) ))
      return o;

    return 0;
  }

  //! Compiles the Pike code @[a] and evaluates it by
  //! calling ___HilfeWrapper in the generated object.
  //! If @[show_result] is set the result will be displayed
  //! and the result buffer updated with its value.
  void evaluate(string a, int(0..1) show_result)
  {
    if(trace_level)
      a = "\ntrace("+trace_level+");\n" + a;
#if constant(_debug)
    if(debug_level)
      a = "\n_debug("+debug_level+");\n" + a;
#endif
    a = "mixed ___HilfeWrapper() { " + a + " ; }";

    object o;
    if( o=hilfe_compile(a) )
    {
      mixed res;
      last_eval_time = gethrtime();
      mixed err = catch{
	res = o->___HilfeWrapper();
	trace(0);
#if constant(_debug)
	_debug(0);
#endif
      };
      last_eval_time = gethrtime()-last_eval_time;

      if( err || (err=catch(a=sprintf("%O", res))) )
      {
	if(objectp(err) && err->is_generic_error)
	  catch { err = ({ err[0], err[1] }); };

	if(arrayp(err) && sizeof(err)==2 && arrayp(err[1]))
	{
	  err[1]=err[1][sizeof(backtrace())..];
	  safe_write(describe_backtrace(err));
	}
	else
	  safe_write("Hilfe Error: Error in evaluation: %O\n",err);
      }
      else {
	if(show_result) {
	  history->push(res);
	  reswrite( safe_write, a, history->get_latest_entry_num(), res,
		    last_compile_time, last_eval_time );
	}
	else
	  reswrite( safe_write, 0, history->get_latest_entry_num(), 0,
		    last_compile_time, last_eval_time );
      }
    }
  }
}


//
// Different wrappers that give the Hilfe a user interface
//

//! This is a wrapper containing a user interface to the Hilfe @[Evaluator]
//! so that it can actually be used. This wrapper uses the @[Stdio.Readline]
//! module to interface with the user. All input history is handled by
//! that module, and as a consequence loading and saving .hilfe_history is
//! handled in this class. Also .hilferc is handled by this class.
class StdinHilfe
{
  inherit Evaluator;

  //! The readline object,
  Stdio.Readline readline;

  //! Saves the user input history, if possible, when called.
  void save_history() {
    catch {
      if(string home=getenv("HOME")||getenv("USERPROFILE")) {
	rm(home+"/.hilfe_history~");
	if(object f=Stdio.File(home+"/.hilfe_history~","wct")) {
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

  void load_history() {
    catch {
      if(string home=getenv("HOME")||getenv("USERPROFILE"))
	if(string s=Stdio.read_file(home+"/.hilfe_history"))
	  readline->enable_history(s/"\n");
    };
  }

  void load_hilferc() {
    if(string home=getenv("HOME")||getenv("USERPROFILE"))
      if(string s=Stdio.read_file(home+"/.hilferc"))
	map(s/"\n", add_buffer);
  }

  void signal_trap(int s) {
    exit(1);
  }

  //! Any hilfe statements given in the init array will be executed
  //! once .hilferc has been executed.
  void create(void|array(string) init)
  {
    write=predef::write;
    ::create();

    load_hilferc();
    if(init) map(init, add_buffer);

    readline = Stdio.Readline();
    load_history();
    if(!readline->get_history())
      readline->enable_history(512);
    signal(signum("SIGINT"),signal_trap);

    for(;;) {
      readline->set_prompt(state->finishedp() ? "> " : ">> ");
      string s=readline->read();

      if(!s)
	break;

      save_history();
      add_input_line(s);
    }
    destruct(readline);
    write("Terminal closed.\n");
  }
}

//!
class GenericHilfe
{
  inherit Evaluator;

  //!
  void create(Stdio.FILE in, Stdio.File out)
  {
    write=out->write;
    ::create();

    while(1)
    {
      write(state->finishedp() ? "> " : ">> ");
      if(string s=in->gets())
	add_input_line(s);
      else {
	write("Terminal closed.\n");
	return;
      }
    }
  }
}

//!
class GenericAsyncHilfe
{
  inherit Evaluator;
  Stdio.File infile, outfile;

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
	write(state->finishedp() ? "> " : ">> ");
      }
  }

  void close_callback()
  {
    write("Terminal closed.\n");
    destruct(this);
    destruct(infile);
    if(outfile) destruct(outfile);
  }

  //!
  void create(Stdio.File in, Stdio.File out)
  {
    infile=in;
    outfile=out;
    in->set_nonblocking(read_callback, 0, close_callback);
    out->set_write_callback(write_callback);

    write=send_output;
    ::create();
    write(state->finishedp() ? "> " : ">> ");
  }
}

int main() {
  StdinHilfe();
  return 0;
}


//
// Help texts for the built in commands.
//

constant documentation_set =
#"Change Hilfe settings. Used as \"set <setting> <parameter>\".
Available parameters:

assembler_debug
    Changes the level of assembler debug used when evaluating
    expressions in Pike. Requires that Pike is compiled with
    RTL debug.

compiler_trace
    Changes the level of compiler trace used when evaluating
    expressions in Pike. Requires that Pike is compiled with
    RTL debug.

debug
    Changes the level of debug used when evaluating expressions in
    Pike. Requires that Pike is compiled with RTL debug.

format
    Changes the formatting of the result values from evaluated
    Pike expressions. Enter \"help set format\" for more
    information.

hedda
    Initializes some variables for quick access, unless they are
    already defined. These variables may be added: mixed foo,
    mixed bar, int i, float f=0.0, mapping m=([]), array a=({})
    and string s=\"\".

history
    Change the maximum number of entries that are kept in the
    result history. Default is 10.

trace
     Changes the level of trace used when evaluating expressions
     in Pike. Possible values are:
       0 Off
       1 Calls to Pike functions are printed.
       2 Calls to buitin functions are printed.
       3 Every opcode interpreted is printed.
       4 Arguments to these opcodes are printed as well.

warnings
    Change the current level of warnings checking. Possible
    values are:
       off     No warnings are shown.
       on      Normal warnings are shown.
       strict  Try a little harder to show warnings.
";

constant documentation_set_format =
#"\"set format\" changes the formatting of the result values from
evaluated Pike expressions. Currently the following set format
parameters are available:

default  The normal result formatting.
bench    A result formatting extended with compilation and
         evaluation times.
sprintf  The result formatting will be decided by the succeeding
         Pike string. The sprintf will be given the following
         arguments:

           0  The result as a string.
           1  The result number in the history.
           2  The result in its native type.
           3  The compilation time as a string.
           4  The evaluation time as a string.
           5  The compilation time in nanoseconds as an int.
           6  The evaluation time in nanoseconds as an int.

         Usage examples:
           set format sprintf \"%s (%[2]t)\\n\"
           set format sprintf \"%s (%d/%[3]s/%[4]s)\\n\"
";

constant documentation_help_me_more =
#"Some commands have extended help available. This can be displayed by
typing help followed by the name of the command, e.g. \"help dump\".
Commands clobbered by e.g. variable declarations can be reached by
prefixing a dot to the command, e.g. \".exit\".

A history of the last returned results is kept and can be accessed
from your hilfe expressions with the variable __. You can either
\"address\" your results with absolute addresses, e.g. __[2] to get
the second result ever, or with relative addresses, e.g. __[-1] to
get the last result. The last result is also available in the
variable _, thus _==__[-1] is true. The magic _ and __ variable can
be clobbered with local definitions to disable them, e.g. by typing
\"int _;\". The result history is currently ten entries long.

A history of the 512 last entered lines is kept in Hilfe. You can
browse this list with your arrow keys up/down. When you exit Hilfe
your history will be saved in .hilfe_history in the directory set
in environment variable $HOME or $USERPROFILE. Next time hilfe is
started the history is imported.

You can put a .hilferc file in the directory set in your
environment variable $HOME or $USERPROFILE. The contents of this
file will be evaluated in hilfe during each startup.

Note that there are a few symbols that you can not define, since
they are used by Hilfe. They are:

___hilfe         A mapping containing all defined symbols.
___Hilfe         The Hilfe object.
___HilfeWrapper  A wrapper around the entered expression.


Type \"help hilfe todo\" to get a list of known Hilfe bugs/lackings.
Type \"help about hilfe\" to get an extended version information.
";

constant documentation_dump =
#"dump
      Shows the currently defined constants, variables, functions
      and programs. It also lists all active inherits and imports.

dump history
      Shows all items in the history queue.

dump memory
      Shows the current memory usage.

dump state
      Shows the current parser state. Only useful for debugging
      Hilfe.

dump wrapper
      Show the latest Hilfe wrapper that the last expression was
      evaluated in. Useful when debugging Hilfe (i.e. investigating
      why valid Pike expressions don't compile).
";

constant documentation_new =
#"new
      Clears the current Hilfe state. This includes the parser
      state, variables, constants, functions, programs, inherits,
      imports and the history. It does not include the currently
      installed commands. Note that code in your .hilferc will not
      be reevaluated.

new history
      Remove all history entries from the result history.

new constants
new functions
new programs
new variables
      Clears all locally defined symbols of the given type.

new imports
new inherits
      Removes all imports/inherits made.
";
