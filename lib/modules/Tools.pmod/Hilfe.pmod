#pike __REAL_VERSION__

//
// Incremental Pike Evaluator
//

constant hilfe_todo = #"List of known Hilfe bugs/room for improvements:

- Hilfe can not handle enums.
- Hilfe can not handle typedefs.
- Hilfe can not handle implicit lambdas.
- Hilfe can not handle unnamed classes.
- Hilfe can not handle named lambdas.
- Hilfe should possibly handle imports better, e.g. overwrite the
  local variables/constants/functions/programs.
- Add some better multiline edit support.
- Improve doc command to get documentation from c-code.
";

// The Big To Do:
// The parser part of Hilfe should really be redesigned once again.
// The user input is first run through Parser.Pike.split and output
// as a token stream. This stream is fed into a streaming parser which
// then relocates the variables and outputs expression objects with
// evaluation destinations already assigned. Note that the streaming
// parser can not start on the next expression before the previous
// expression has been evaluated, because the variable table might not
// be up to date.

//! Abstract class for Hilfe commands.
class Command {

  //! Returns a one line description of the command. This help should
  //! be shorter than 54 characters.
  string help(string|zero what);

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
    if(!has_index(e->variables, n) && !has_index(e->constants, n) &&
       !has_index(e->functions, n) && !has_index(e->programs, n)) {
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

protected class CommandSet {
  inherit Command;

  string help(string what) { return "Change Hilfe settings."; }
  string doc(string what, string with) {
    if(with=="format")
      return documentation_set_format;
    return documentation_set;
  }

  protected void bench_reswrite(function(string, mixed ... : int) w,
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

  protected class Reswriter (string format) {
    protected void `()(function(string, mixed ... : int) w,
		       string sres, int num, mixed res,
		       int last_compile_time, int last_eval_time) {
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

  protected class Intwriter (string type, function fallback) {
    protected void `()(function(string, mixed ... : int) w, string sres,
                       int num, mixed res, int last_compile_time,
                       int last_eval_time) {
      if(res && intp(res)) {
	if(type=="b") {
	  string s = sprintf("%b", res);
	  s = "0"*(8-sizeof(s)%8) + s;
	  w( "0b" + s/8*" " + "\n" );
	}
	else
	  w(type+"\n", res);
      }
      else
	fallback(w, sres, num, res, last_compile_time, last_eval_time);
    }
  }

  protected array my_indices(string|mapping|multiset|object|program in) {
    if(objectp(in) || programp(in))
      return sort(indices(in));
    return indices(in);
  }

  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens) {

    line = sizeof(words)>1 && words[1];
    function(array(string)|string, mixed ... : void) write = e->safe_write;

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
      case "off":
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
#if constant(Debug.assembler_debug)
      e->assembler_debug_level = (int)words[2];
#else
      write("Assembler debug not available.\n");
#endif
      return;
    }

    if(arg_check("compiler_trace")) {
#if constant(Debug.compiler_trace)
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
	e->reswrite = Intwriter("0%o", e->std_reswrite);
	return;
      case "hex":
	e->reswrite = Intwriter("0x%x", e->std_reswrite);
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
	  e->reswrite = Reswriter(f[1..<1]);
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

protected class CommandExit {
  inherit Command;
  string help(string what) { return "Exit Hilfe."; }

  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens) {
    if (e->readline) {
      e->readline->get_history()->pop(line);
      e->save_history();
    }
    e->safe_write("Exiting.\n");
    destruct(e);
    exit(0);
  }
}

protected class CommandDoc {
  inherit Command;
  string help(string what)
  {
    return #"Show documentation for pike modules and classes.
Documentation for the symbol before the cursor is also accessible
by pressing F1.";
  }

  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens)
  {
    if (sizeof(tokens) > 3)
      output_doc(e, Parser.Pike.group(tokens[2..]));
  }

  void output_doc(Evaluator e, array(array(string)|string) tokens)
  {
    object|function module;
    string type;
    array rest;
    object docs;

    if (!sizeof(tokens))
      return;
    if (tokens == ({ "write", "\n\n" }))
      module = Stdio.stdout->write;
    else
      [module, rest, type] = resolv(e, tokens);

    if (!module)
      return;
    if (type != "autodoc")
      docs = master()->show_doc(module);
    else
      docs = module;

    object child;
    if (docs->?documentation)
    {
      if (docs->objects && sizeof(docs->objects))
        e->safe_write("\n%{%s\n%}\n", docs->objects->print());
      e->safe_write(docs->documentation->text+"\n");
    }
    else if (docs && (child=docs->findObject("create")) && child->documentation)
    {
      e->safe_write("\n%{%s\n%}\n", replace(child->objects->print()[*], ([ "create":(tokens[2..]-({"\n\n"}))*"", " | ":"|" ]) ));
      e->safe_write(child->documentation->text+"\n");
    }
    else if (docs && sizeof(docs->docGroups))
      e->safe_write("\n%{%s\n%}\n", Array.flatten(docs->docGroups->objects->print()));
    else if (objectp(module))
      e->safe_write("\n%{%s\n%}\n", indices(module));
    else if (functionp(module))
      e->safe_write("\n%O\n", _typeof(module));
    else
      e->safe_write("Documentation not found!\n");
  }
}

protected class CommandHelp {
  inherit Command;
  string help(string what) { return "Show help text."; }

  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens) {
    line = words[1..]*" ";
    function(array(string)|string, mixed ... : void) write = e->safe_write;

    switch(line) {

    case "me more":
      write( documentation_help_me_more );
      return;

    case "hilfe todo":
      write(hilfe_todo);
      return;

    case "about hilfe":
      e->print_version();
      return;
    }

    if(sizeof(words)>1 && e->commands[words[1]]) {
      string ret = e->commands[words[1]]->doc(words[1], words[2..]*"");
      if(ret) write(ret+"\n");
      return;
    }

    write("\n");
    e->print_version();
    write( #"Hilfe is a tool to evaluate Pike code interactively and
incrementally. Any Pike function, expression or variable declaration
can be entered at the command line. Tab completion is also available.
There are also a few extra commands:

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

protected class CommandDot {
  inherit Command;
  string|zero help(string what) { return 0; }

  local protected constant usr_vector_a = ({
    89, 111, 117, 32, 97, 114, 101, 32, 105, 110, 115, 105, 100, 101, 32, 97,
    32, 72, 105, 108, 102, 101, 46, 32, 73, 116, 32, 115, 109, 101, 108, 108,
    115, 32, 103, 111, 111, 100, 32, 104, 101, 114, 101, 46, 32, 89, 111,
    117, 32, 115, 101, 101, 32 });
  local protected constant usr_vector_b = ({
    32, 89, 111, 117, 32, 99, 97, 110, 32, 103, 111, 32, 105, 110, 32, 97,
    110, 121, 32, 100, 105, 114, 101, 99, 116, 105, 111, 110, 32, 102, 114,
    111, 109, 32, 104, 101, 114, 101, 46 });
  local protected constant usr_vector_c = ({
    32, 89, 111, 117, 32, 97, 114, 101, 32, 99, 97, 114, 114, 121, 105, 110,
    103, 32 });
  local protected constant usr_vector_d = usr_vector_c[..8] + ({
    101, 109, 112, 116, 121, 32, 104, 97, 110, 100, 101, 100, 46 });

  protected array(string) thing(array|mapping|object thing, string what,
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

  protected function(array(string)|string, mixed ... : int) write;

  string help(string what) { return "Dump variables and other info."; }
  string doc(string|zero what, string|zero with) { return documentation_dump; }

  protected void wrapper(Evaluator e) {
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

  protected string print_mapping(array(string) ind, array val) {
    int m = max( @filter(map(ind, sizeof), `<, 20), 8 );
    foreach(ind; int i; string name)
      write("%-*s : %s\n", m, name, replace(sprintf("%O", val[i]), "\n",
					    "\n"+(" "*m)+"   "));
  }

  protected void dump(Evaluator e) {
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
      write(Pike.Lazy.Debug.pp_memory_usage());
      return;
    case "":
      dump(e);
      return;
    }
    write("Unknown dump specifier.\n");
    write(doc(0,0)+"\n");
  }
}

protected class CommandHej {
  inherit Command;
  string|zero help(string what) { return 0; }
  void exec(Evaluator e, string line, array(string) words,
	    array(string) tokens) {
    if(line[0]=='.') e->safe_write( (string)({ 84,106,97,98,97,33,10 }) );
  }
}

protected class CommandNew {
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

protected class CommandStartStop {
  inherit Command;
  SubSystems subsystems = SubSystems();

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
protected class SubSysBackend {
  int(0..1) is_running;

  constant startdoc = "backend [once]\n"
  "\tStarts the backend thread. If \"once\" is specified execution\n"
  "\twill end at first exception. Can be restarted with \"start backend\".\n";

  constant stopdoc = "backend\n\tstop the backend thread.\n";

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

  protected void backend_loop(function(string:int) write_err, int(0..1) once){
    is_running=1;
    object backend = Pike.DefaultBackend;
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

protected class SubSysLogger {

  constant startdoc = "logging [<filename>]\n"
  "\tLogs all input and output to a log file. If no file name is \n"
  "\tspecified logging will be appended to hilfe.log in the current\n"
  "\twork directory.\n";

  constant stopdoc = "logging\n\tTurns off logging to file.\n";
  int(0..1) running;

  protected class Logger (Evaluator e, Stdio.File logfile)
  {
    constant is_logger = 1;

    protected void create() {
      e->add_input_hook(this);
      running = 1;
    }

    protected void _destruct() {
      e && e->remove_input_hook(this);
      running = 0;
    }

    protected int(0..0) `() (string in) {
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

//
// Support stuff..
//


// General subsystem handler class.
protected class SubSystems {
  mapping(string:object) subsystems;

  protected void create () {
    // Register the subsystems here.
    subsystems = ([
#if constant(thread_create)
      "backend":SubSysBackend(),
#endif
      "logging":SubSysLogger(),
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

local {

protected constant whitespace = (< ' ', '\n' ,'\r', '\t' >);
protected constant termblock = (< "catch", "do", "gauge", "lambda",
                                  "class stop" >);
protected constant modifier = (< "extern", "final", "inline", "local",
                                 "optional", "private", "protected",
                                 "public", "static", "variant", "deprecated" >);

protected constant types = (< "string", "int", "float", "array", "mapping",
                              "multiset", "mixed", "object", "program",
                              "function", "void" >);

// infix token may appear between two literals
protected constant infix = (< "!=", "%", "%=", "&", "&=", "*", "*=",
                              "+", "+=", ",", "-", "-=",
                              "->", "->=", "/", "/=",
                              "<", "<<", "<<=", "<=", "==",
                              ">", ">=", ">>", ">>=",
                              "^", "^=", "|", "|=", "~", "~=",
                              "&&", "||", "=", "..", "**" >);

// before literal but not after
protected constant prefix = (< "!", "@", "(", "({", "([", "(<", "[", "{",
                               "<", ">" >);

// after literal but not before
protected constant postfix = (< ")", "})", "])", ">)", "]", "}" >);

// before or after literal but not between
protected constant prepostfix = (< "--", "++" >);

// between two expressions
protected constant seperator = (< "?", ":", ",", ";" >);

protected constant reference = ([ ".":"module", "->":"object", "[":"array" ]);

protected constant group = ([ "(":")", "({":"})", "([":"])", "(<":">)",
                              "[":"]", "{":"}" ]);

// Symbols not valid in type expressions.
// All of the above except ".", "|", "&" and "~".
protected constant notype = (infix+prefix+postfix+prepostfix+seperator) -
  (< ".", "|", "&", "~" >);

}

string typeof_token(string|array token)
{
  string type;
  if (arrayp(token))
  {
    if (token[0] == "(" && token[-1] == ")")
      type = "argumentgroup";
    else if (token[0] == "[" && token[-1] == "]")
      type = "referencegroup";
  }
  else if ( reference[token] )
    type = "reference";
  else if ( (token[0]==token[-1] && (< '"', '\'' >)[token[0]])
            || token == array_sscanf(token, "%[0-9.]")[0] )
    type = "literal";
  else if (token == array_sscanf(token, "%[a-zA-Z0-9_]")[0] || token[0]=='`')
    type = "symbol";
    // FIXME: handle unicode chars
  else if (token == array_sscanf(token, "%[ \t\r\n]")[0])
    type = "whitespace";
  else if (seperator[token])
    type = "seperator";
  else if (prefix[token])
    type = "prefix";
  else if (prepostfix[token])
    type = "prepostfix";
  else if (postfix[token])
    type = "postfix";
  else if (infix[token])
    type = "infix";
  else
    type = "unknown";
  return type;
}



//! Represents a Pike expression
class Expression {
  protected array(string) tokens;
  protected mapping(int:int) positions;
  protected array(int) depths;
  protected multiset(int) sscanf_depths;

  //! @param t
  //!   An array of Pike tokens.
  protected void create(array(string) t) {
    positions = ([]);
    depths = allocate(sizeof(t));
    sscanf_depths = (<>);

    generate_offsets(t);
    tokens = t;
  }

  protected void generate_offsets(array t)
  {
    int pos = tokens && sizeof(tokens);
    int depth;

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

  //! The number of non-whitespace tokens in the expression.
  protected int _sizeof() {
    return sizeof(positions);
  }

  //! Returns a token or a token range without whitespaces.
  protected string|zero `[](int f, void|int t) {
    if(f>sizeof(positions) || -f>sizeof(positions))
      return 0;
    if(!t)
      return tokens[positions[f]];
    if(t>=sizeof(positions))
      t = sizeof(positions)-1;

    // Negative t not boundary checked.
    return tokens[positions[f]..positions[t]]*"";
  }

  //! Replaces a token with a new token.
  protected string `[]= (int f, string v) {
    return tokens[positions[f]] = v;
  }

  //! Return the parenthesis depth at the given position.
  int depth(int f) {
    return depths[positions[f]];
  }

  //! Returns 1 if the current position is within a sscanf expression.
  int(0..1) in_sscanf(int f) {
    return sscanf_depths[depth(f)];
  }

  //! See if there are any forbidden modifiers used in the expression,
  //! e.g. "private int x;" is not valid inside Hilfe.
  //! @returns
  //!   Returns an error message as a string if the expression
  //!   contains a forbidden modifier, otherwise @expr{0@}.
  string|zero check_modifiers() {
    foreach(sort(values(positions)), int pos)
      if(modifier[tokens[pos]])
	return "Hilfe Error: Modifier \"" + tokens[pos] +
	  "\" not allowed on top level in Hilfe.\n";
      else
	return 0;
    return 0;
  }

  //! Returns the expression verbatim.
  string code() {
    return tokens*"";
  }

  //! Returns at which position the type declaration that begins at
  //! position @[position] ends. A return value of -1 means that the
  //! token or tokens from @[position] can not be a type declaration.
  int(-1..) endoftype(int(-1..) position) {
    string t = `[](position);
    if(types[ t ] ) {
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
           "(", "~", "[", "`", "else", ".." >)[ t ] )
      return -1;
    if( t[0]>='0' && t[0]<='9' )
      return -1;
    if( notype[ t ] )
      return -1;

    if( t=="." ) position++;

    for(; position<sizeof(positions); position++) {
      // NB: It seems the tokenizer does not recognize multiset start
      //     and multiset end as separate tokens.
      if ((`[](position+1) == "(") && (`[](position+2) == "<")) {
        // Generic binding.
        position = find_matching("(", position+2);
        if (position < 0) return -1;
        if (`[](position-1) != ">") return -1;
      }
      if( notype[ `[](position+1) ] )
        return -1;
      if( `[](position+1)=="." ) {
        position++;
        continue;
      }

      if( (< "|", "&" >)[`[](position+1)] ) {
	return endoftype(position+2);
      }

      return position;
    }

    // Well, we did find a type declaration, but it
    // wasn't used for anything except perhaps loading
    // a module into pike, e.g. "_Roxen;"
    return -1;
  }

  //! Returns the position of the matching parenthesis of the given
  //! kind, starting from the given position. The position should be
  //! the position after the opening parenthesis, or later. Assuming
  //! balanced code. Returns -1 on failure.
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

  //! Is there a block starting at @[pos]?
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

  //! An Expression object can be cast to an array or a string. In
  //! both forms all tokens, including white spaces will be returned.
  protected mixed cast(string to)
  {
    switch(to)
    {
    case "array": return tokens;
    case "string": return code();
    }
    return UNDEFINED;
  }

  protected string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O)", this_program, tokens);
  }
}

//! In every Hilfe object (@[Evaluator]) there is a ParserState object
//! that manages the current state of the parser. Essentially tokens are
//! entered in one end and complete expressions are output in the other.
//! The parser object is accessible as ___Hilfe->state from Hilfe expressions.
protected class ParserState(Evaluator evaluator) {

  protected ADT.Stack pstack = ADT.Stack();
  protected constant starts = ([ ")":"(", "}":"{", "]":"[",
			       ">)":"(<", "})":"({", "])":"([" ]);
  protected array(string) pipeline = ({ });
  protected array(Expression) ready = ({ });
  protected string last;
  protected string block;

  protected mapping low_state = ([]);

  //! Feed more tokens into the state.
  void feed(array(string) tokens) {
    foreach(tokens, string token) {
      // comments
      if(sizeof(token)>1 && (token[0..1]=="//" || token[0..1]=="/*")) continue;

      pipeline += ({ token });

      // If we start a block at the uppermost level, what kind of block is it?
      if(!sizeof(pstack))
        switch(token)
        {
        case "{":
          if(!block)
            block = last;
          else if(block=="class" && last=="class")
            block = "class stop";
          pstack->push(token);
          continue;

        case "(":
          if(block=="class" && last=="class")
            block = "class stop";
          break;

        case "lambda":
          block = token;
          last = token;
          continue;

        case "class":
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
	if(!sizeof(pstack))
	   throw(sprintf("%O end parenthesis without start parenthesis.",
			 token));
	if(pstack->top()!=starts[token])
	   throw(sprintf("%O end parenthesis does not match closest start "
			 "parenthesis %O.",
			 token, pstack->top()));
	pstack->pop();
      }

      // expressions
      if(token==";" && !sizeof(pstack)) {
	ready += ({ Expression(pipeline) });
	pipeline = ({});
	block = 0;
	continue;
      }

      // If we end a block at the uppermost level, and it doesn't need a ";",
      // then we can move out that block of the pipeline.
      if(token=="}" && !sizeof(pstack) && !termblock[block]) {
	ready += ({ Expression(pipeline) });
	pipeline = ({});
	block = 0;
	continue;
      }

      // Preprocessor
      if( token[0]=='#' ) {
	string tmp = token-" ";
	if( has_prefix(tmp, "#error") && !sizeof(pstack)) {
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

    for(int pos; pos<sizeof(ready); pos++)
    {
      Expression expr = ready[pos];
      if(expr[0]==";") continue;
      if( expr[0]=="if" )
      {
        if( pos<sizeof(ready)-1 && ready[pos+1][0]=="else" )
          while( pos<sizeof(ready)-1 && ready[pos+1][0]=="else" )
            expr = Expression( (array)expr + (array)ready[++pos] );
        else
          expr = Expression( (array)expr+({ " else ___Hilfe->last_else=1;" }));
      }
      else if( expr[0]=="else" )
        expr = Expression( ({ "if", "(___Hilfe->last_else);" })+(array)expr );

      ret += ({ expr });
    }

    ready = ({});
    return ret;
  }

  protected string caught_error;

  //! Prints out any error that might have occurred while
  //! @[push_string] was executed. The error will be
  //! printed with the print function @[w].
  void show_error(function(array(string)|string, mixed ... : int) w) {
    if(!caught_error) return;
    w("Hilfe Error: %s", caught_error);
    caught_error = 0;
  }

  //! Sends the input @[line] to @[Parser.Pike] for tokenization,
  //! but keeps a state between each call to handle multiline
  //! /**/ comments and multiline #"" strings.
  array(string)|zero push_string(string line) {
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
        if( tmp=="#pragmastrict_types\n" ) {
          evaluator->strict_types = 1;
        } else {
          caught_error = "Pragma does not work inside Hilfe.\n";
        }
        return 0;
      }
      if( has_prefix(tmp, "#if") || has_prefix(tmp, "#elif") ||
	  has_prefix(tmp, "#else") || has_prefix(tmp, "#endif") ) {
	caught_error = "Preprocess instructions if, elif, ifdef, ifndef "
	  "else and endif not possible in Hilfe.\n";
	return 0;
      }
    }
    if(err = catch( tokens = Parser.Pike.low_split(line, low_state) )) {
      caught_error = err[0];
      return 0;
    }
    if (low_state->remains && has_prefix(low_state->remains, "\"")) {
      caught_error =
        Parser.Pike.UnterminatedStringError(tokens*"", low_state->remains)[0];
      m_delete(low_state, "remains");
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
    if(sizeof(pstack)) return 0;
    if(low_state->remains) return 0;
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
    ret += sprintf("Parenthesis stack: %s\n", pstack->arr[..sizeof(pstack)]*" ");
    ret += sprintf("Current pipeline: %O\n", pipeline);
    ret += sprintf("Last token: %O\n", last);
    ret += sprintf("Current block: %O\n", block);
    return ret;
  }
}

//! In every Hilfe object (@[Evaluator]) there is a HilfeHistory
//! object that manages the result history. That history object is
//! accessible both from __ and ___Hilfe->history in Hilfe expressions.
protected class HilfeHistory {

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
  protected mixed `[](int i) {
    mixed ret;
    array err = catch( ret = ::`[](i) );
    if(err)
      error(err[0]);
    return ret;
  }

  // Give the object a better name.
  protected string _sprintf(int t) {
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
  ParserState state = ParserState(this);

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
      write = ([array]write) + ({ new });
    else if (write)
      write = ({ write, new });
    else
      write = new;
  }

  //! Removes an output function.
  void remove_writer(object|function old) {
    if(arrayp(write))
      write = ([array]write) - ({ old });
    else
      write = 0;
  }

  //! An output method that shouldn't crash.
  int safe_write(array(string)|string in, mixed ... args) {
    if(!write) return 0;
    mixed err = catch {
      if(arrayp(in)) in *= "";
      if(sizeof(args))
	in = sprintf(in, @args);
      if (String.width(in) > 8) {
	// Variable and function names may contain wide characters...
	// Perform Unicode escaping.
	in = map(in/"",
		 lambda(string s) {
		   if (String.width(s) <= 8) return s;
		   int c = s[0] & 0xffffffff;
		   if (c <= 0xffff) return sprintf("\\u%04x", c);
		   return sprintf("\\U%08x", c);
		 }) * "";
      }
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

  protected array input_hooks = ({});

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

  protected void create()
  {
    print_version();
    commands->set = CommandSet();
    commands->exit = CommandExit();
    commands->quit = commands->exit;
    commands->help = CommandHelp();
    commands->doc = CommandDoc();
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
    int(0..1) finished = state->finishedp();
    array(string) tokens = state->push_string(s+"\n");
    array(string) words = s/" ";
    string command = words[0];

    if(finished)
    {
      // See if first token is a command and not a defined entity.
      if(commands[command] && !has_index(constants, command) &&
         !has_index(variables, command) && !has_index(functions, command) &&
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

  protected int(0..1) hilfe_error(mixed err) {
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

  protected void add_hilfe_constant(string code, string var) {
    if(object o = hilfe_compile("constant " + code +
				";\nmixed ___HilfeWrapper() { return " +
				var + "; }", var)) {
      hilfe_error( catch( constants[var] = o->___HilfeWrapper() ) );
    }
  }

  protected void add_hilfe_variable(string type, string code, string var) {
    int(0..1) existed;
    mixed old_value;
    if(has_index(variables, var)) {
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

  protected void add_hilfe_entity(string type, string code,
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
  protected int rel_parser( Expression expr, multiset(string) symbols,
			  void|int p ) {
    int top = !p;
    while( p<sizeof(expr)) {
      if( expr->is_block(p) ) {
	string type = expr[p++];
        if(type=="class") top=0;
	multiset(string) new_scope = symbols+(<>);

        // No () for gauge, do and possibly class.
	// Get variable names for next scope clobber.
	if(expr[p]=="(") {
	  p++;

	  switch(type) {

          case "catch":
            p = relocate(expr, symbols, new_scope, p);
            p++;
            break;

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
  protected int relocate( Expression expr, multiset(string) symbols,
			  multiset(string)|zero next_symbols, int p,
			  void|string safe_word, void|int(0..1) top) {
    int op = p;
    werror("%O %O %d\n", (symbols?indices(symbols||(<>))*", ":0),
           (next_symbols?indices(next_symbols||(<>))*", ":0), top );
    p = _relocate(expr, symbols, next_symbols, p, safe_word, top);
    werror(" relocate %O..%O %O\n", op, p, expr[op..p]);
    return p;
  }
#endif

  protected int relocate( Expression expr, multiset(string) symbols,
			  multiset(string)|zero next_symbols, int p,
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

            if(t==".") {
              i++;
              continue;
            }

            if(t=="lambda") {
              int d = expr->depth(i);
              do {
                i++;
                if( i==pos ) break;
              } while( expr->depth(++i)>d );
              continue;
            }

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
		expr[i] = "(([mapping(string:"+types[expr[i]]+")]"
		  "(mixed)___hilfe)->" + t + ")";
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
	  expr[p] = "(([mapping(string:"+types[expr[p]]+")]"
	    "(mixed)___hilfe)->" + expr[p] + ")";
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
  string|zero parse_expression(Expression expr)
  {
    // Check for modifiers
    expr->check_modifiers();

    // Rewrite variables for the Hilfe wrapper
    rel_parser(expr, (multiset)(indices(variables)) );

    switch(expr[0])
    {
      case "if":
      case "else":
      case "for":
      case "do":
      case "while":
      case "foreach":
      case "switch":
	// Value-less statements
	evaluate(expr->code(), 0);
	return 0;

      case "inherit":
      {
	inherits += ({ expr[1..<1] });
	if(!hilfe_compile(""))
	  inherits = inherits[..<1];
	return 0;
      }

      case "import":
      {
	imports += ({ expr[1..<1] });
	if(!hilfe_compile(""))
	  imports = imports[..<1];
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

	if(expr[1]=="{" || expr[1]=="(") {
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
  int(0..1) warnings = 1;

  //! The current trace level.
  int trace_level;
#if constant(Debug.assembler_debug)
  //! The current assembler debug level.
  //! Only available if Pike is compiled with RTL debug.
  int assembler_debug_level;
#endif
#if constant(Debug.compiler_trace)
  //! The current compiler trace level.
  //! Only available if Pike is compiled with RTL debug.
  int compiler_trace_level;
#endif
#if constant(_debug)
  //! The current debug level.
  //! Only available if Pike is compiled with RTL debug.
  int debug_level;
#endif

  //! Should an else expression be carried out?
  int(0..1) last_else;

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
  //! time (int) and the evaluation time (int). If the evaluated expression
  //! didn't return anything (e.g. a for loop) then 0 will be given as the
  //! result string.
  function reswrite = std_reswrite;


  protected string hch_errors = "";
  protected string hch_warnings = "";

  protected class HilfeCompileHandler (int stack_level) {

    protected void create() {
      hch_errors = "";
      hch_warnings = "";
    }

    mapping(string:mixed) hilfe_symbols;

    mapping(string:mixed) get_default_module() {
      object compat = get_active_compilation_handler();
      if (compat->?get_default_module) {
	// Support things like @expr{7.4::rusage}.
	return (compat->get_default_module()||all_constants()) + hilfe_symbols;
      }
      return all_constants() + hilfe_symbols;
    }

    string format(string file, int line, string err) {
      if(file=="HilfeInput")
	file = "";
      else
	file = master()->trim_file_name(file)+":";
      if(err[-1]!='\n') err += "\n";
      string linestr = line?(string)line:"-";
      return sprintf(": %s%s: %s", file, linestr, err);
    }

    void compile_error(string file, int line, string err) {
      hch_errors += "Compiler Error" + format(file, line, err);
    }

    void compile_warning(string file, int line, string warn) {
      hch_warnings += "Compiler Warning" + format(file, line, warn);
    }

    int compile_exception (object|array trace)
    {
      if (objectp (trace) && trace->is_cpp_or_compilation_error)
	// Backtraces for errors thrown by cpp(), compile() or a
	// compile callback are normally not interesting. Return zero
	// to let the caller format an error message instead.
	return 0;

      catch {
	trace = ({trace[0], trace[1][stack_level + 1..]});
	if (trace[1][0][0] == "Optimizer")
	  // When the compiler evaluates constants there's a
	  // somewhat odd frame "Optimizer:0 0()" at the top.
	  trace[1] = trace[1][1..];
      };
      hch_errors += "Compiler Exception: " + describe_backtrace (trace);
      return 1;
    }

    void show_errors() {
      if (sizeof(hch_errors))
        safe_write(hch_errors);
    }

    void show_warnings() {
      if (sizeof(hch_warnings))
        safe_write(hch_warnings);
    }
  }

  //! Creates a wrapper and compiles the pike code @[f] in it.
  //! If a new variable is compiled to be tested, its name
  //! should be given in @[new_var] so that magically defined
  //! entities can be undefined and a warning printed.
  object|zero hilfe_compile(string f, void|string new_var)
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
    else if(!has_index(symbols, "__") && !has_index(variables, "__")) {
      symbols["__"] = history;
    }

    if(new_var=="_")
      safe_write("Hilfe Warning: History variable _ is no "
		 "longer reachable.\n");
    else if(!has_index(symbols, "_") && !has_index(variables, "_")
	    && sizeof(history)) {
      symbols["_"] = history[-1];
    }

    string prog;
    int major = master()->compat_major;
    int minor = master()->compat_minor;
    if( major!=-1 || minor!=-1 )
      prog = sprintf("#pike %d.%d\n", major, minor);
    else
      prog = "";

    if(strict_types)
      prog += "#pragma strict_types\n";

    prog +=
      map(inherits, lambda(string f) { return "inherit "+f+";\n"; }) * "" +

      map(imports, lambda(string f) { return "import "+f+";\n"; }) * "" +

      "mapping(string:mixed)|zero ___hilfe = ___Hilfe->variables;\n# 1\n" + f +
      "\n";

    HilfeCompileHandler handler = HilfeCompileHandler (sizeof (backtrace()));

    handler->hilfe_symbols = symbols;
    handler->hilfe_symbols->___Hilfe = this;
    handler->hilfe_symbols->write = safe_write;

    last_compiled_expr = prog;
    program p;
    mixed err;

#if constant(Debug.assembler_debug)
    if(assembler_debug_level)
      Debug.assembler_debug(assembler_debug_level);
#endif
#if constant(Debug.compiler_trace)
    if(compiler_trace_level)
      Debug.compiler_trace(compiler_trace_level);
#endif

    float compile_time = gauge {
      err = catch(p=compile_string(prog, "HilfeInput", handler));
    };
    last_compile_time = (int)(compile_time*1000000);

#if constant(Debug.assembler_debug)
    Debug.assembler_debug(0);
#endif
#if constant(Debug.compiler_trace)
    Debug.compiler_trace(0);
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
        a = "\ntrace("+trace_level+");\n"+a;
#if constant(_debug)
    if(debug_level)
      a = "\n_debug("+debug_level+");\n" + a;
#endif
    a = "mixed ___HilfeWrapper() { " + a + " ; }";

    last_else = 0;
    object o;
    if( o=hilfe_compile(a) )
    {
      mixed res, err;
      float eval_time = gauge {
	err = catch {
	  res = o->___HilfeWrapper();
	};
	trace(0);
#if constant(_debug)
	_debug(0);
#endif
      };
      last_eval_time = (int)(eval_time*1000000);

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

mapping base_objects(Evaluator e)
{
  return all_constants() + e->constants + e->variables;
}

array(object|array(string)) resolv(Evaluator e, array completable,
                                   void|object base, void|string type)
{
  if (e->variables->DEBUG_COMPLETIONS)
    e->safe_write("resolv(%O, %O, %O)\n", completable, base, type);
  if (!sizeof(completable))
    return ({ base, completable, type });

  if (stringp(completable[0]) &&
      completable[0] == array_sscanf(completable[0], "%[ \t\r\n]")[0])
    return ({ base, completable[1..], type });

  if (typeof_token(completable[0]) == "argumentgroup" && type != "autodoc")
    return resolv(e, completable, master()->show_doc(base), "autodoc");

  if (!base && completable[0] == ".")
  {
    if (sizeof(completable) == 1)
      return ({ 0, completable, "module" });
    else
    {
      catch
      {
        // quick and dirty attempt to load a local module
        base=compile_string(sprintf("object o=.%s;", completable[1]), 0)()->o;
      };
      if (!base)
        return ({ 0, completable, type });
      if (objectp(base))
        return resolv(e, completable[2..], base, "module");
      return resolv(e, completable[2..], base);
    }
  }

  if (!base && sizeof(completable) > 1)
  {
    if (completable[0] == "master" && sizeof(completable) >=2
        && typeof_token(completable[1]) == "argumentgroup")
    {
      return resolv(e, completable[2..], master(), "object");
    }
    if (base=base_objects(e)[completable[0]])
      return resolv(e, completable[1..], base, "object");

    if (sizeof(completable) > 1
        && (base=master()->root_module[completable[0]]))
      return resolv(e, completable[1..], base, "module");
    return ({ 0, completable, type });
  }

  if (sizeof(completable) > 1)
  {
    object newbase;
    if (reference[completable[0]] && sizeof(completable) > 1)
      return resolv(e, completable[1..], base);
    if (type == "autodoc")
    {
      if (typeof_token(completable[0]) == "symbol"
          && (newbase = base->findObject(completable[0])))
        return resolv(e, completable[1..], newbase, type);
      else if (sizeof(completable) > 2
            && typeof_token(completable[0]) == "argumentgroup"
            && typeof_token(completable[1]) == "reference")
        return resolv(e, completable[2..], base, type);
      else
        return ({ base, completable, type });
    }
    if (!functionp(base) && (newbase=base[completable[0]]) )
      return resolv(e, completable[1..], newbase, type);
  }

  return ({ base, completable, type });
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

  protected int do_write(string a, mixed ... args )
  {
    if( sizeof( args ) )
      a = sprintf(a,@args);
    write(a);
    return strlen(a);
  }

  //! Any hilfe statements given in the init array will be executed
  //! once .hilferc has been executed.
  protected void create(void|array(string) init)
  {
    readline = Stdio.Readline();
    write = readline->write;
    ::create();

    add_constant("werror",do_write);
    add_constant("write",do_write);

    load_hilferc();
    if(init) map(init, add_buffer);

    load_history();
    if(!readline->get_history())
      readline->enable_history(512);
    readline->get_input_controller()->bind("\t", handle_completions);
    readline->get_input_controller()->bind("\\!k1", handle_doc);

    signal(signum("SIGINT"),signal_trap);

    for(;;) {
      readline->set_prompt(state->finishedp() ? "> " : ">> ");
      string s=readline->read();

      if(!s)
	break;

      save_history();
      add_input_line(s);
    }
    write("Terminal closed.\n");
    destruct(readline);
  }

  array get_resolvable(array tokens, void|int debug)
  {
    array completable = ({});
    string tokentype;

    foreach(reverse(tokens);; string token)
    {
      string _tokentype = typeof_token(token);

      if (debug)
        write(sprintf("%O = %s\n", token, _tokentype));

      if ( ( _tokentype == "reference" &&
             (!tokentype || tokentype == "symbol"))
            || (_tokentype == "symbol" && (!tokentype
                 || (< "reference", "referencegroup",
                       "argumentgroup" >)[tokentype]))
            || ( (<"argumentgroup", "referencegroup" >)[_tokentype]
                 && (!tokentype || tokentype == "reference"))
         )
      {
        completable += ({ token });
        tokentype = _tokentype;
      }
      else if (_tokentype == "whitespace")
        ;
      else
        break;
    }

    // keep the last whitespace
    if (arrayp(tokens) && sizeof(tokens) &&
        typeof_token(tokens[-1]) == "whitespace")
      completable = ({ " " }) + completable;
    return reverse(completable);
  }

  void handle_doc(string key)
  {
    array tokens;
    string input = readline->gettext()[..readline->getcursorpos()-1];
    mixed error = catch
    {
      tokens = tokenize(input);
    };
    if (error || !tokens || !sizeof(tokens))
      return;

    array completable = get_resolvable(tokens);
    if (sizeof(completable))
      CommandDoc()->output_doc(this, completable);
  }

  void handle_completions(string key)
  {
    mixed old_handler = master()->get_inhibit_compile_errors();
    HilfeCompileHandler handler = HilfeCompileHandler(sizeof(backtrace()));
    master()->set_inhibit_compile_errors(handler);

    array tokens;
    string input = readline->gettext()[..readline->getcursorpos()-1];
    array|string completions;

    mixed error = catch
    {
      tokens = tokenize(input);
    };

    if(error)
    {
      if (objectp(error) && error->is_unterminated_string_error)
      {
        error = catch
        {
          completions = get_file_completions((input/"\"")[-1]);
        };
      }

      if (error)
      {
        if(!objectp(error))
          error = Error.mkerror(error);
        readline->message(sprintf("%s\nAn error occurred, attempting to complete your input!\nPlease include the backtrace above and the line below in your report:\ninput: %s\n", error->describe(), input));
        completions = ({});
      }
    }

    if (tokens && !completions)
    {
      array completable = get_resolvable(tokens, variables->DEBUG_COMPLETIONS);

      if (completable && sizeof(completable))
      {
        error = catch
        {
          completions = get_module_completions(completable);
        };
        error = Error.mkerror(error);
      }
      else if (!tokens || !sizeof(tokens))
        completions = sort(indices(master()->root_module)) +
          sort(indices(base_objects(this)));
        // FIXME: base_objects should not be sorted like this

      if (!completions || !sizeof(completions))
      {
        string token = tokens[-1];
        if( sizeof(tokens) >= 2 && typeof_token(token) == "whitespace" )
          token = tokens[-2];

        if (variables->DEBUG_COMPLETIONS)
          readline->message(sprintf("type: %s\n", typeof_token(token)));

        completions = sort(indices(master()->root_module)) +
          sort(indices(base_objects(this)));
        // FIXME: base_objects should not be sorted like this

        switch(typeof_token(token))
        {
          case "symbol":
          case "literal":
          case "postfix":
            completions = (array)(infix+seperator);
            break;
          case "prefix":
          case "infix":
          case "seperator":
          default:
            completions += (array)prefix;
        }
        foreach(reverse(tokens);; string token)
        {
            if (group[token])
            {
              completions += ({ group[token] }) ;
              break;
            }
        }
      }

      if (error)
      {
        readline->message(sprintf("%s\nAn error occurred, attempting to complete your input!\nPlease include the backtrace above and the lines below in your report:\ninput: %s\ntokens: %O\ncompletable: %O\n", error->describe(), input, tokens, completable, ));
      }
      else if (variables->DEBUG_COMPLETIONS)
        readline->message(sprintf("input: %s\ntokens: %O\ncompletable: %O\ncompletions: %O\n", input, tokens, completable, completions));
    }

    handler->show_errors();
    handler->show_warnings();
    master()->set_inhibit_compile_errors(old_handler);

    if(completions && sizeof(completions))
    {
      if(stringp(completions))
        readline->insert(completions, readline->getcursorpos());
      else
        readline->list_completions(completions);
    }
  }

  array tokenize(string input)
  {
      array tokens = Parser.Pike.split(input);
      if (variables->DEBUG_COMPLETIONS)
          readline->message(sprintf("\n\ntokenize(%O): %O\n\n", input, tokens));
      // drop the linebreak that split appends
      if (tokens[-1] == "\n")
        tokens = tokens[..<1];
      else if (tokens[-1][-1] == '\n')
        tokens[-1] = tokens[-1][..<1];

      tokens = Parser.Pike.group(tokens);
      return tokens;
  }

  array|string get_file_completions(string path)
  {
    array files = ({});
    if ( (< "", ".", ".." >)[path-"../"] )
      files += ({ ".." });

    if (!sizeof(path) || path[0] != '/')
      path = "./"+path;

    string dir = dirname(path);
    string file = basename(path);
    catch
    {
      files += get_dir(dir);
    };

    if (!sizeof(files))
      return ({});

    array completions = Array.filter(files, has_prefix, file);
    string prefix = String.common_prefix(completions)[sizeof(file)..];

    if (sizeof(prefix))
    {
      return prefix;
    }

    mapping filetypes = ([ "dir":"/", "lnk":"@", "reg":"" ]);

    if (sizeof(completions) == 1 && file_stat(dir+"/"+completions[0])->isdir )
    {
      return "/";
    }
    else
    {
      foreach(completions; int count; string item)
      {
        Stdio.Stat stat = file_stat(dir+"/"+item);
        if (stat)
          completions[count] += filetypes[stat->type]||"";

        stat = file_stat(dir+"/"+item, 1);
        if (stat->?type == "lnk")
          completions[count] += filetypes["lnk"];
      }
      return completions;
    }
  }

  array|string get_module_completions(array completable)
  {
    array rest = completable;
    object base;
    string type;
    int(0..1) space;

    if (!completable)
      completable = ({});

    if (completable[-1]==' ')
    {
      space = true;
      completable = completable[..<1];
    }

    if (sizeof(completable) > 1)
      [base, rest, type] = resolv(this, completable);

    if (variables->DEBUG_COMPLETIONS)
      safe_write(sprintf("get_module_completions(%O): %O, %O, %O\n", completable, base, rest, type));
    array completions = low_get_module_completions(rest, base, type, space);
    if (sizeof(completions) == 1)
      return completions[0];
    else
      return completions;
  }

  mapping reftypes = ([ "module":".",
                     "object":"->",
                     "mapping":"->",
                     "function":"(",
                     "program":"(",
                     "method":"(",
                     "class":"(",
                   ]);

  array low_get_module_completions(array completable, object base, void|string type, void|int(0..1) space)
  {
      if (variables->DEBUG_COMPLETIONS)
        safe_write(sprintf("low_get_module_completions(%O\n, %O, %O, %O)\n", completable, base, type, space));

      if (!completable)
        completable = ({});

      mapping other = ([]);
      array modules = ({});
      mixed error;

      if (base && !sizeof(completable))
      {
        if (space)
          return (array)infix;
        if (type == "autodoc")
            return ({ reftypes[base->objtype||base->objects[0]->objtype]||"" });
        if (objectp(base))
          return ({ reftypes[type||"object"] });
        if (mappingp(base))
          return ({ reftypes->object });
        else if(functionp(base))
          return ({ reftypes->function });
        else if (programp(base))
          return ({ reftypes->program });
        else
          return (array)infix;
      }

      if (!base && sizeof(completable) && completable[0] == ".")
      {
          array modules = sort(get_dir("."));
          if (sizeof(completable) > 1)
          {
            modules = Array.filter(modules, has_prefix, completable[1]);
            if (sizeof(modules) == 1)
              return ({ (modules[0]/".")[0][sizeof(completable[1])..] });
            string prefix = String.common_prefix(modules)[sizeof(completable[1])..];
            if (prefix)
              return ({ prefix });

            if (sizeof(completable) == 2)
              return modules;
            else
              return ({});
          }
          else
            return modules;
      }
      else if (!base)
      {
          if (type == "autodoc")
          {
            if (variables->DEBUG_COMPLETIONS)
              safe_write("autodoc without base\n");
            return ({});
          }
          other = base_objects(this);
          base = master()->root_module;
      }

      if (type == "autodoc")
      {
        if (base->docGroups)
          modules = Array.uniq(Array.flatten(base->docGroups->objects->name));
        else
          return ({});
      }
      else
      {
        error = catch
        {
          modules = sort(indices(base));
        };
        error = Error.mkerror(error);
      }

      if (sizeof(other))
        modules += indices(other);

      if (sizeof(completable) == 1)
      {
          if (type == "autodoc"
              && typeof_token(completable[0]) == "argumentgroup")
            if (space)
              return (array)infix;
            else
              return ({ reftypes->object });
          if (reference[completable[0]])
            return modules;
          if (!stringp(completable[0]))
            return ({});

          // FIXME: handle non-string indices better
          modules = sort((array(string))modules);
          modules = Array.filter(modules, has_prefix, completable[0]);
          string prefix = String.common_prefix(modules);
          string module;

          if (prefix == completable[0] && sizeof(modules)>1 && (base[prefix]||other[prefix]))
            return modules + low_get_module_completions(({}), base[prefix]||other[prefix], type, space);

          prefix = prefix[sizeof(completable[0])..];
          if (sizeof(prefix))
            return ({ prefix });

          if (sizeof(modules)>1)
            return modules;
          else if (!sizeof(modules))
            return ({});
          else
          {
            module = modules[0];
            modules = ({});
            object thismodule;

            if(other && other[module])
            {
              thismodule = other[module];
              type = "object";
            }
            else if (intp(base[module]) || floatp(base[module]) || stringp(base[module]) )
              return (array)infix;
            else
            {
              // FIXME: need to check if thismodule is something indexable
              thismodule = base[module];
              if (!type)
                type = "module";
            }

            return low_get_module_completions(({}), thismodule, type, space);
          }
      }

      if (completable && sizeof(completable))
      {
          if ( (< "reference", "argumentgroup" >)[typeof_token(completable[0])])
            return low_get_module_completions(completable[1..], base, type||reference[completable[0]], space);
          else
            safe_write(sprintf("UNHANDLED CASE: completable: %O\nbase: %O\n", completable, base));
      }

      return modules;
  }
}

//!
class GenericHilfe
{
  inherit Evaluator;

  //!
  protected void create(Stdio.FILE in, Stdio.File out)
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
class GenericAsyncHilfe (Stdio.File infile, Stdio.File outfile)
{
  inherit Evaluator;

  string outbuffer="";

  void write_callback()
  {
    if (!sizeof(outbuffer)) return;
    int i=outfile->write(outbuffer);
    outbuffer=outbuffer[i..];
  }

  void send_output(string s, mixed ... args)
  {
    if (sizeof(args))
      outbuffer+=sprintf(s,@args);
    else
      outbuffer+=s;
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
    destruct(outfile);
  }

  protected void create()
  {
    infile->set_nonblocking(read_callback, 0, close_callback);
    outfile->set_write_callback(write_callback);

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
    already defined. These variables may be added: mixed foo, mixed
    bar, int i, float f=0.0, mapping m=([]), array a=({}) and string
    s=\"\". The indices function will also sort indices made on
    objects and programs.

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
       on      Normal warnings are shown (default).
       strict  Try a little harder to show warnings.
";

constant documentation_set_format =
#"\"set format\" changes the formatting of the result values from
evaluated Pike expressions. Currently the following set format
parameters are available:

default  The normal result formatting.
bench    A result formatting extended with compilation and
         evaluation times.
bin      Returns integers in binary form.
hex      Returns integers in hexadecimal form.
oct      Returns integers in octal form.
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

Tab completion works on modules, global and local symbols and operators.
Within quotes completion is offered for files and directories.

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
