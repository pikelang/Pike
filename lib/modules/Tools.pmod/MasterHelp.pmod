#pike __REAL_VERSION__

//! This module contains usage strings for the @[master()->_main()].

//! Summary of the options for the Pike interpreter binary, and the
//! default master.
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
 -x [<tool>]          : Execute a built in tool.

 Use --help=options|environment for further help.
";

//! Complete set of options for the Pike interpreter binary, and the
//! default master.
constant options_help =
  #"Usage: pike [-driver options] script [script arguments]
Driver options:
 -I --include-path=<p>: Add <p> to the include path.
 -M --module-path=<p> : Add <p> to the module path.
 -P --program-path=<p>: Add <p> to the program path.
 -e --execute=<cmd>   : Run the given command instead of a script.
 -h --help            : Show sort help message
    --help=<section>  : Shows longer help section about
                        options or environment.
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
 -tg                  : Log the gc runs to stderr.
 -V <v> --compat=<v>  : Run with compatibility for version <v>.
 -x [<tool>]          : Execute a built in tool.
 --debug-without=<m>  : Prohibit the resolver to resolve mobule <m>.
 -E --preprocess=<f>  : Run only the preprocessor.
 -w --warnings        : Enable warnings
 -W --no-warnings     : Disable warnings
 --picky-cpp          : Enable usually supressed cpp warnings and errors.
 --autoreload         : Automatically reload changed modules.
 --no-precompile      : Always compile all modules on start.
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

//! The set of environment variables that the default master looks at.
constant environment_help =
#"The Pike master looks at the following environment variables:

PIKE_MASTER        : This file will be used as the Pike master.

PIKE_INCLUDE_PATH  : These paths will be added to the include paths.
PIKE_PROGRAM_PATH  : These paths will be added to the program paths.
PIKE_MODULE_PATH   : These paths will be added to the module paths.

OSTYPE             : If set to \"cygwin32\", cygwin path mangling will be used.

LONG_PIKE_ERRORS   : If set, full paths will be used in errors.
SHORT_PIKE_ERRORS  : If set, only filenames will be used in errors.

PIKE_BACKTRACE_LEN : Backtraces will be limited to this length (default 200).
";

//! Useful recipe for when all else fails.
constant kladdkaka_help =
#"Ingredients:
  4 eggs
  5 dl sugar
  2 pinches salt
  6 tbs/90 ml cocoa
  200 g melted butter
  3 dl wheat-flour
  2 tsp/10 ml vanilla sugar

1. Mix everything together
2. Pour into a greased circular form.
3. Bake in 175 degrees C (350 degrees F) for 1 hour.
4. Serve while it is still hot with whipped cream.
";

//! Select a suitable help message.
string do_help(string|int what) {
  if( intp(what) ) return opt_summary;
  what -= "-";
  if( stringp(this[what+"_help"]) )
    return this[what+"_help"];
  return "No help available on that topic.\n";

}
