#pike __REAL_VERSION__

#pragma strict_types

//! @[Getopt] is a group of functions which can be used to find command
//! line options.
//!
//! Command line options come in two flavors: long and short. The short ones
//! consists of a dash followed by a character (@tt{-t@}), the long ones
//! consist of two dashes followed by a string of text (@tt{--test@}).
//! The short options can also be combined, which means that you can write
//! @tt{-tda@} instead of @tt{-t -d -a@}.
//!
//! Options can also require arguments, in which case they cannot be
//! combined. To write an option with an argument you write
//! @tt{-t @i{argument@}@} or @tt{-t@i{argument@}@} or
//! @tt{--test=@i{argument@}@}.

static void my_error(string err, int throw_errors) {
  if(throw_errors) error(err);
  werror(err);
  exit(1);
}

//!   This is a generic function to parse command line options of the
//!   type @tt{-f@}, @tt{--foo@} or @tt{--foo=bar@}.
//!
//! @param argv
//!   The first argument should be the array of strings that was sent as
//!   the second argument to your @tt{main()@} function.
//!
//! @param shortform
//!   The second is a string with the short form of your option. The
//!   short form must be only one character long. It can also be an
//!   array of strings, in which case any of the options in the array
//!   will be accepted.
//!
//! @param longform
//!   This is an alternative and maybe more readable way to give the
//!   same option. If you give @tt{"foo"@} as @[longform] your program
//!   will accept @tt{--foo@} as argument. This argument can also be
//!   an array of strings, in which case any of the options in the
//!   array will be accepted.
//!
//! @param envvars
//!   This argument specifies an environment variable that can be used
//!   to specify the same option, to make it easier to customize
//!   program usage. It can also be an array of strings, in which case
//!   any of the mentioned variables in the array may be used.
//!
//! @param def
//!   This argument has two functions: It specifies if the option takes an
//!   argument or not, and it informs @[find_option()] what to return if the
//!   option is not present. If @[def] is given and the option does not have an
//!   argument @[find_option()] will fail. @[def] can be specified as
//!   @[UNDEFINED] or left out if the option does not take an argument.
//!
//! @param throw_errors
//!   If @[throw_errors] has been specified @[find_option()] will throw
//!   errors on failure. If it has been left out, or is @tt{0@} (zero), it will
//!   instead print an error message on @[Stdio.stderr] and exit the
//!   program with result code 1 on failure.
//!
//! @returns
//!   Returns the value the option has been set to if any.
//!
//!   If the option is present, but has not been set to anything @tt{1@}
//!   will be returned.
//!
//!   Otherwise if any of the environment variables specified in @[envvars] has
//!   been set, that value will be returned.
//!
//!   If all else fails, @[def] will be returned.
//!
//! @throws
//!   If an option that requires an argument lacks an argument and
//!   @[throw_errors] is set an error will be thrown.
//!
//! @note
//!   @[find_option()] modifies @[argv]. Parsed options will be removed
//!   from @[argv]. Elements of @[argv] that have been removed entirely will
//!   be replaced with zeroes.
//!
//!   This function reads options even if they are written after the first
//!   non-option on the line.
//!
//!   Index @tt{0@} (zero) of @[argv] is not scanned for options, since it
//!   is reserved for the program name.
//!
//!   Only the first ocurrance of an option will be parsed. To parse
//!   multiple ocurrances, call @[find_option()] multiple times.
//!
//! @seealso
//!   @[Getopt.get_args()]
//!
string|int(0..1) find_option(array(string) argv,
			     array(string)|string shortform,
			     array(string)|string|void longform,
			     array(string)|string|void envvars,
			     string|int(0..1)|void def,
			     int|void throw_errors)
{
  string|int(0..1) value;

  int(0..1) hasarg = !zero_type(def);
  if(!arrayp(longform)) longform = ({ [string]longform });
  if(!arrayp(shortform)) shortform = ({ [string]shortform });
  if(stringp(envvars)) envvars = ({ [string]envvars });

  foreach(argv; int i; string opt) {
    if(!i || !opt || sizeof(opt)<2 || opt[0]!='-') continue;

    if(opt[1] == '-') {
      if(opt=="--") break;

      string tmp=opt;
      sscanf(tmp, "%s=%s", tmp, value);
	  
      if(has_value([array(string)]longform, tmp[2..])) {
	argv[i]=0;

	if(hasarg && !value) {
	  if(i == sizeof(argv)-1)
	    my_error( "No argument to option "+tmp+".\n",throw_errors );

	  value=argv[i+1];
	  argv[i+1]=0;
	}

	return value || 1;
      }
    }
    else {
      foreach(opt/1; int j; string sopt) {

	if(has_value([array(string)]shortform, sopt)) {
	  string arg = opt[j+1..];

	  if(hasarg) {
	    if(arg == "") {
	      if(i == sizeof(argv)-1)
		my_error( "No argument to option -"+sopt+".\n",throw_errors );

	      value=argv[i+1];
	      argv[i+1] = 0;
	    }
	    else {
	      value=arg;
	      arg="";
	    }
	  }
	  else
	    value=1;

	  argv[i] = opt[..j-1]+arg;
	  if(argv[i]=="-") argv[i]=0;
	  return value;
	}
      }
    }
  }

  if(arrayp(envvars))
    foreach([array(string)]envvars, value)
      if(value && (value=[string]getenv([string]value)))
	return value;
  
  return def;
}

//!
constant HAS_ARG=1;

//!
constant NO_ARG=2;

//!
constant MAY_HAVE_ARG=3;


// ({ "name", type, "alias"|({"aliases"}), "env_var", default })

#define NAME 0
#define TYPE 1
#define ALIASES 2
#define ENV 3
#define DEF 4

#define SIZE 5


//!   This function does the job of several calls to @[find_option()].
//!   The main advantage of this is that it allows it to handle the
//!   @tt{@b{POSIX_ME_HARDER@}@} environment variable better. When the either
//!   the argument @[posix_me_harder] or the environment variable
//!   @tt{@b{POSIX_ME_HARDER@}@} is true, no arguments will be parsed after
//!   the first non-option on the command line.
//!
//! @param argv
//!   The should be the array of strings that was sent as
//!   the second argument to your @tt{main()@} function.
//!
//! @param options
//!   Each element in the array @[options] should be an array on the
//!   following form:
//!   @array
//!   	@elem string name
//!   	  Name is a tag used to identify the option in the output.
//!   	@elem int type
//!   	  Type is one of @[Getopt.HAS_ARG], @[Getopt.NO_ARG] and
//!   	  @[Getopt.MAY_HAVE_ARG] and it affects how the error handling
//!   	  and parsing works.
//!   	  You should use @[HAS_ARG] for options that require a path, a number
//!       or similar. @[NO_ARG] should be used for options that do not need an
//!   	  argument, such as @tt{--version@}. @[MAY_HAVE_ARG] should be used
//!   	  for options that may or may not need an argument.
//!   	@elem string|array(string) aliases
//!   	  This is a string or an array of string of options that will be
//!   	  looked for. Short and long options can be mixed, and short options
//!   	  can be combined into one string. Note that you must include the
//!       dashes so that @[find_all_options()] can distinguish between
//!   	  long and short options. Example: @tt{({"-tT","--test"})@}
//!   	  This would make @[find_all_options] look for @tt{-t@},
//!   	  @tt{-T@} and @tt{--test@}.
//!   	@elem void|string|array(string) env_var
//!   	  This is a string or an array of strings containing names of
//!   	  environment variables that can be used instead of the
//!   	  command line option.
//!   	@elem void|mixed default
//!   	  This is the default value a @[MAY_HAVE_ARG] option will have in the
//!       output if it was set but not assigned any value.
//!   @endarray
//!
//!   Only the first three elements need to be included.
//!
//! @param posix_me_harder
//!   Don't scan for arguments after the first non-option.
//!
//! @param throw_errors
//!   If @[throw_errors] has been specified @[find_all_options()] will throw
//!   errors on failure. If it has been left out, or is @tt{0@} (zero), it will
//!   instead print an error message on @[Stdio.stderr] and exit the
//!   program with result code 1 on failure.
//!
//! @returns
//!   The good news is that the output from this function is a lot simpler.
//!   @[find_all_options()] returns an array where each element is an array on
//!   this form:
//!   @array
//!   	@elem string name
//!   	  Option identifier name from the input.
//!   	@elem mixed value
//!   	  Value given. If no value was specified, and no default has been
//!   	  specified, the value will be 1.
//!   @endarray
//!
//! @note
//!   @[find_all_options()] modifies @[argv].
//!
//!   Index @tt{0@} (zero) of @[argv] is not scanned for options, since it
//!   is reserved for the program name.
//!
//! @seealso
//!   @[Getopt.get_args()], @[Getopt.find_option()]
//!
array(array) find_all_options(array(string) argv,
		       array(array(array(string)|string|int)) options,
		       void|int(-1..1) posix_me_harder, void|int throw_errors)
{
  // --- Initialize variables

  mapping(string:array(string|int|array(string))) quick=([]);

  foreach(options; int i; array(array(string)|string|int) opt) {
    if(sizeof(opt)!=SIZE) {
      options[i] = opt + allocate(SIZE-sizeof(opt));
      opt = options[i];
    }
    array(string)|string aliases = [array(string)|string]opt[ALIASES];
    if(!arrayp(aliases)) aliases = ({[string]aliases});

    foreach([array(string)]aliases, string optname)
      if(optname[0..1]=="--")
	quick[optname]=opt;
      else
	foreach(optname[1..]/1,string optletter)
	  quick["-"+optletter]=opt;
  }

  posix_me_harder = posix_me_harder!=-1 &&
    (posix_me_harder || !!getenv("POSIX_ME_HARDER"));

  // --- Do the actual parsing of arguments.

  array(array) ret=({});
  foreach(argv; int e; string opt) {

    if(!e || !opt) continue;
    if(sizeof(opt)<2 || opt[0]!='-') {
      if(posix_me_harder) break;
      continue;
    }

    if(opt[1]=='-') {

      if(opt=="--") break;

      string arg;
      sscanf(opt, "%s=%s", opt, arg);
      if(array option=quick[opt]) {
	argv[e]=0;
	if(!arg && option[TYPE]==HAS_ARG) {
	  if(e==sizeof(argv)-1)
	    my_error( "No argument to option "+opt+".\n", throw_errors );

	  arg = argv[e+1];
	  argv[e+1] = 0;
	}

	ret+=({ ({ option[NAME], arg || option[DEF] || 1 }) });
      }
    }
    else {
      array(string) opts = opt/1;
      foreach(opts; int j; string s_opt) {
	s_opt = "-"+s_opt;

	if(array option=quick[s_opt]) {
	  opts[j]=0;
	  string arg;

	  if(option[TYPE]!=NO_ARG) { // HAS_ARG or MAY_HAVE_ARG
	    arg = opt[j+1..];
	      
	    if(option[TYPE]==HAS_ARG && arg=="") {
	      if(e==sizeof(argv)-1)
		my_error( "No argument to option "+opt+".\n", throw_errors );

	      arg = argv[e+1];
	      argv[e+1] = 0;
	    }
	    else {
	      arg = opts[j+1..]*"";
	      opts = opts[..j];
	    }
	  }

	  if (arg == "") arg = 0;
	  ret+=({ ({ option[NAME], arg || option[DEF] || 1 }) });
	  if(sizeof(opts)==j+1) break; // if opts=opts[..j] we're done.
	}
      }

      argv[e] = opts*"";
      if(argv[e]=="-") argv[e]=0;
    }
  }

  // --- Fill out empty slots with environment values

  multiset(string) done = [multiset(string)]mkmultiset(column(ret, 0));
  foreach(options, array(string|int|array(string)) option) {
    string name=[string]option[NAME];
    if(done[name]) continue;

    if(option[ENV]) {
      array(string)|string foo=[array(string)|string]option[ENV];
      if(!foo) continue;
      if(stringp(foo)) foo = ({ [string]foo });

      foreach([array(string)]foo, foo)
	if(foo=[string]getenv([string]foo)) {
	  ret += ({ ({name, foo}) });
	  done[name] = 1;
	  break;
	}
    }
  }

  return ret;
}

//! This function returns the remaining command line arguments after
//! you have run @[find_option()] or @[find_all_options()] to find
//! all the options in the argument list. If there are any options
//! left not handled by @[find_option()] or @[find_all_options()]
//! this function will fail.
//!
//! If @[throw_errors] has been specified @[get_args()] will throw errors
//! on failure. If it has been left out, or is @tt{0@} (zero), it will
//! instead print an error message on @[Stdio.stderr] and exit the
//! program with result code 1 on failure.
//!
//! @returns
//! On success a new @[argv] array without the parsed options is
//! returned.
//!
//! @seealso
//! @[Getopt.find_option()], @[Getopt.find_all_options()]
//!
array(string) get_args(array(string) argv, void|int(-1..1) posix_me_harder,
		       void|int throw_errors)
{
  posix_me_harder = posix_me_harder!=-1 &&
    (posix_me_harder || !!getenv("POSIX_ME_HARDER"));

  foreach(argv; int i; string opt) {

    if(!i || !stringp(opt)) continue;
    if(sizeof(opt)<2 || opt[0]!='-') {
      if(posix_me_harder) break;
      continue;
    }

    if(opt[1]=='-') {

      if(opt=="--") {
	argv[i]=0;
	break;
      }

      my_error( "Unknown option "+opt+".\n", throw_errors );
    }
    else {
      if(strlen(argv[i]) == 2)
	my_error( "Unknown option "+argv[i]+".\n", throw_errors );
      my_error( "Unknown options "+argv[i]+".\n", throw_errors );
    }
  }

  argv -= ({0, 1});

  return argv;
}
