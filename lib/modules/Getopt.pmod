#pike __REAL_VERSION__

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


//  startpid = (int)find_arg(argv, "s", ({ "start-script-pid" }),
// 			     ({ "ROXEN_START_SCRIPT_PID"}));

//  configuration_dir = find_arg(argv, "d", ({ "config-dir",
//					     "configurations",
//					     "configuration-directory" }),
//			          ({ "ROXEN_CONFIGDIR", "CONFIGURATIONS" }),
//			           "../configurations");


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
//!   argument @[find_option()] will fail.
//!
//! @param throw_errors
//!   If @[throw_errors] has been specified @[find_option] will throw errors
//!   on failure. If it has been left out, or is @tt{0@} (zero), it will
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
//!   been set, that value will be return.
//!
//!   If all else fails, @[def] will be returned.
//!
//! @note
//!   @[find_option()] modifies argv.
//!
//!   This function reads options even if they are written after the first
//!   non-option on the line.
//!
//!   Index @tt{0@} (zero) of @[argv] is not scanned for options, since it
//!   is reserved for the program name.
//!
//! @seealso
//!   @[Getopt.get_args()]
//!
string|int find_option(array(string) argv,
		       array(string)|string shortform,
		       array(string)|string|void longform,
		       array(string)|string|void envvars,
		       string|int|void def,
		       int|void throw_errors)
{
  string|int value;
  int i,hasarg;

  hasarg=query_num_arg() > 4;
  if(!arrayp(longform)) longform=({[string]longform});
  if(!arrayp(shortform)) shortform=({[string]shortform});
  if(!arrayp(envvars)) envvars=({[string]envvars});

  for(i=1; i<sizeof(argv); i++)
  {
    if(argv[i] && strlen(argv[i]) > 1)
    {
      if(argv[i][0] == '-')
      {
	if(argv[i][1] == '-')
	{
	  string tmp;
	  int nf;

	  if(argv[i]=="--") break;

	  sscanf(tmp=argv[i], "%s=%s", tmp, value);
	  
	  if(search(longform, tmp[2..]) != -1)
	  {
	    argv[i]=0;
	    if(hasarg)
	    {
	      if(!value)
	      {
		if(i == sizeof(argv)-1)
		{
		  if (throw_errors) {
		    throw(({ "No argument to option "+tmp+".\n",
			       backtrace() }));
		  } else {
		    werror("No argument to option "+tmp+".\n");
		    exit(1);
		  }
		}
		value=argv[i+1];
		argv[i+1]=0;
	      }
	      return value;
	    } else {
	      return value || 1;
	    }
	  }
	} else {
	  int j;
	  for(j=1;j<strlen(argv[i]);j++)
	  {
            string opt;
            int pos;
	    if(search(shortform, opt=argv[i][j..j]) != -1)
	    {
              string arg;
              arg=argv[i][j+1..];

	      if(hasarg)
	      {
		if(arg=="")
		{
		  if(i == sizeof(argv)-1)
                  {
		    if (throw_errors) {
		      throw(({ "No argument to option -"+argv[i][j..j]+".\n",
				 backtrace() }));
		    } else {
		      werror("No argument to option -"+argv[i][j..j]+".\n");
		      exit(1);
		    }
		  }

                  value=argv[i+1];
		  argv[i+1] = 0;
		} else {
		  value=arg;
                  arg="";
		}
	      } else {
		value=1;
	      }

	      argv[i]=argv[i][..j-1]+arg;
	      if(argv[i]=="-") argv[i]=0;
	      return value;
	    }
	  }
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

/*
 * ({ "name", type, ({aliases}), env_var, default })
 */

//!
constant HAS_ARG=1;

//!
constant NO_ARG=2;

//!
constant MAY_HAVE_ARG=3;

#define NAME 0
#define TYPE 1
#define ALIASES 2
#define ENV 3
#define DEF 4

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
//!   	  This is the default value the option will have in the output
//!   	  from this function. Options without defaults will be omitted
//!   	  from the output if they are not found in argv.
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
array find_all_options(array(string) argv,
		       array(array(array(string)|string)) options,
		       void|int posix_me_harder, void|int throw_errors)
{
  mapping(string:array(string|array(string))) quick=([]);
  foreach(options, array(array(string)|string) opt)
    {
      array(string)|string aliases=[array(string)|string]opt[ALIASES];
      if(!arrayp(aliases)) aliases=({[string]aliases});
      foreach([array(string)]aliases, string optname)
	{
	  if(optname[0..1]=="--")
	  {
	    quick[optname]=opt;
	  }else{
	    foreach(optname[1..]/"",string optletter)
	      {
		quick["-"+optletter]=opt;
	      }
	  }
	}
    }

  array ret=({});
  for(int e=1;e<sizeof(argv);e++)
  {
    if(!argv[e]) continue;

    if(strlen(argv[e]) && argv[e][0]=='-')
    {
      if(strlen(argv[e])>1 && argv[e][1]=='-')
      {
	string opt=argv[e];
	if(opt=="--") break;

	string arg;
	sscanf(opt,"%s=%s",opt, arg);
	if(array option=quick[opt])
	{
	  argv[e]=0;
	  if(!arg && option[TYPE]==HAS_ARG)
	  {
	    if(e==sizeof(argv)-1)
	    {
	      if (throw_errors) {
		throw(({ "No argument to option "+opt+".\n",
			   backtrace() }));
	      } else {
		werror("No argument to option "+opt+".\n");
		exit(1);
	      }
	    }
	    arg=argv[e+1];
	    argv[e+1]=0;
	  }
	  ret+=({ ({ option[0], arg || 1 }) });
	}
      }else{
	array(string) foo=argv[e]/"";
	for(int j=1;j<strlen(foo);j++)
	{
	  string opt="-"+foo[j];
	  if(array option=quick[opt])
	  {
	    foo[j]=0;
	    string arg;
	    if(option[TYPE]!=NO_ARG)
	    {
	      arg=argv[e][j+1..];
	      
	      if(option[TYPE]==HAS_ARG && arg=="")
	      {
		if(e==sizeof(argv)-1)
		{
		  if (throw_errors) {
		    throw(({ "No argument to option "+opt+".\n",
			       backtrace() }));
		  } else {
		    werror("No argument to option "+opt+".\n");
		    exit(1);
		  }
		}
		arg=argv[e+1];
		argv[e+1]=0;
	      }else{
		foo=foo[..j];
	      }
	    }

	    ret+=({ ({ option[0], arg || 1 }) });
	  }
	}
	argv[e]=foo*"";
	if(argv[e]=="-") argv[e]=0;
      }
    }else{
      if(posix_me_harder != -1)
	if(posix_me_harder || getenv("POSIX_ME_HARDER"))
	  break;
    }
  }

  multiset done=mkmultiset(column(ret, 0));
  foreach(options, array(string|array(string)) option)
    {
      string name=[string]option[NAME];
      if(done[name]) continue;
      if(sizeof(option) > ENV)
      {
	array(string)|string foo=option[ENV];
	if(!foo) continue;
	if(stringp(foo)) foo=({[string]foo});
	foreach([array(string)]foo, foo)
	  {
	    if(foo=[string]getenv([string]foo))
	    {
	      ret+=({ ({name, foo}) });
	      done[name]=1;
	      break;
	    }
	  }

	if(!done && sizeof(option)>3 && option[3])
	{
	  ret+=({ ({name, option[3]}) });
	  done[name]=1;
	}
      }
    }
  return ret;
}

//! This function returns the remaining command line arguments after
//! you have run @[find_options()] or @[find_all_options()] to find
//! all the options in the argument list. If there are any options
//! left not handled by @[find_options()] or @[find_all_options()]
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
array(string) get_args(array(string) argv, void|int posix_me_harder,
		       void|int throw_errors)
{
  int i;
  for(i=1;i<sizeof(argv);i++)
  {
    if(argv[i] && strlen(argv[i])>1 && argv[i][0]=='-')
    {
      if(argv[i][1]=='-')
      {
	if(argv[i]=="--")
	{
	  argv[i]=0;
	  break;
	}else{
	  if (throw_errors) {
	    throw(({ "Unknown option "+argv[i]+".\n",
		       backtrace() }));
	  } else {
	    werror("Unknown option "+argv[i]+".\n");
	    exit(1);
	  }
	}
      }else{
	if(strlen(argv[i]) == 2) {
	  if (throw_errors) {
	    throw(({ "Unknown option "+argv[i]+".\n",
		       backtrace() }));
	  } else {
	    werror("Unknown option "+argv[i]+".\n");
	  }
	} else {
	  if (throw_errors) {
	    throw(({ "Unknown options "+argv[i]+".\n",
		       backtrace() }));
	  } else {
	    werror("Unknown options "+argv[i]+".\n");
	  }
	}
	exit(1);
      }
    }else{
      if(posix_me_harder != -1)
	if(posix_me_harder || getenv("POSIX_ME_HARDER"))
	  break;
    }
  }

  argv-=({0,1});

  return argv;
}

