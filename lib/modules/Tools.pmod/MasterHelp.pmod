#pike __REAL_VERSION__

constant opt_summary =
  #"Usage: pike [-driver options] script [script arguments]
Driver options include:
 -M --module-path=<p> : Add <p> to the module path
 -e --execute=<cmd>   : Run the given command instead of a script.
 -v --version         : See what version of pike you have.
 --dumpversion        : Prints out only the version number.
 --features           : List Pike features.
 --info               : List information about the Pike build and setup.
 --show-paths         : See the paths and master that pike uses.
 -m <file>            : Use <file> as master object.
 -d -d#               : Increase debug (# is how much)
 -t -t#               : Increase trace level
 -x <tool>            : Execute a built in tool.

 Use --help=options to show all available options.
";

constant options_help =
  #"Usage: pike [-driver options] script [script arguments]
Driver options:
 -I --include-path=<p>: Add <p> to the include path.
 -M --module-path=<p> : Add <p> to the module path.
 -P --program-path=<p>: Add <p> to the program path.
 -e --execute=<cmd>   : Run the given command instead of a script.
 -h --help            : See this message.
 -v --version         : See what version of pike you have.
 --dumpversion        : Prints out only the version number.
 --features           : List Pike features.
 --info               : List information about the Pike build and setup.
 --show-paths         : See the paths and master that pike uses.
 -s#                  : Set Pike stack size.
 -ss#                 : Set thread stack size.
 -m <file>            : Use <file> as master object.
 -d -d# -d<what>      : Increase debug.
 -t -t#               : Increase trace level.
 -V <v> --compat=<v>  : Run with compatibility for version <v>.
 -x <tool>            : Execute a built in tool.
 --debug-without=<m>  : Prohibit the resolver to resolve mobule <m>.
 -E --preprocess=<f>  : Run only the preprocessor.
 -w --warnings        : Enable warnings
 -W --no-warnings     : Disable warnings
 --autoreload         : Automatically reload changed modules.
 --compiler-trace     : Turn on tracing of the Pike compiler.
 --assembler-debug=#  : Set peephole optimizer debug level.
 --optimizer-debug=#  : Set global optimizer debug level.
 --debug=#            : Increase or set debug level.
 --trace=#            : Increase the trace level.
 -D<symbol>[=value]   : Define the preprocessor symbol (to value).
 -q#                  : End excution after # Pike instructions
 -a -a#               : Increase peep hole optimizer debug level.
 -p -p#               : Increase level of profiling.
 -l -l#               : Increase debug level in the global optimizer.
 -rt                  : Turn on runtime type checking.
 -rT                  : Turn on #pragma strict_types for all files.
";

string do_help(string|int what) {
  if( intp(what) ) return opt_summary;
  what -= "-";
  if( stringp(this[what+"_help"]) )
    return this[what+"_help"];
  return "No help available on that topic.\n";

}
