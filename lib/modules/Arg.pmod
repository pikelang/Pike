#pike __REAL_VERSION__

//! Argument parsing module
//!
//! This module supports two rather different methods of argument
//! parsing.  The first is suitable quick argument parsing without
//! much in the way of checking:
//!
//! @code
//! int main( int c, array(string) argv )
//! {
//!   mapping arguments = Arg.parse(argv);
//!   array files = arguments[Arg.REST];
//!   if( arguments->help ) print_help();
//!   ...
//! }
//! @endcode
//!
//! The @[Arg.parse] method will return a mapping from argument name
//! to the argument value, if any.
//!
//! Non-option arguments will be placed in the index Arg.REST
//!
//! The second way to use this module is to inherit the Options class
//! and add supported arguments.
//!
//! @code
//! class MyArguments {
//!    inherit Arg.Options;
//!    Opt verbose = NoOpt("-v")|NoOpt("--verbose");
//!    Opt help = MaybeOpt("--help");
//!    Opt output = HasOpt("--output")|HasOpt("-o");
//! };
//! @endcode
//!
//! Then, in main:
//!
//! @code
//! MyArguments args = MyArguments(argv);
//! @endcode
//!
//! See the documentation for @[OptLibrary] for details about the various
//! Opt classes.

//! This class contains a library of parser for different type of
//! options.
class OptLibrary
{

  //! Base class for parsing an argument. Inherit this class to create
  //! custom made option types.
  class Opt
  {
    constant is_opt = 1;
    protected Opt next;

    //! The overloading method should calculate the value of the
    //! option and return it. Methods processing @[argv] should only
    //! look at argv[0] and if it matches, set it to 0. Returning
    //! @expr{UNDEFINED@} means the option was not set (or
    //! matched). To properly chain arguments parsers, return
    //! @expr{::get_value(argv, env, previous)@} instead of
    //! @expr{UNDEFINED@}, unless you want to explicitly stop the
    //! chain and not set this option.
    mixed get_value(array(string) argv, mapping(string:string) env,
                    mixed previous)
    {
      if(next) return next->get_value(argv, env, previous);
      return UNDEFINED;
    }

    //! Should return a list of options that are parsed. To properly
    //! chain argument parsers, return @expr{your_opts +
    //! ::get_opts()@}.
    array(string) get_opts()
    {
      if(!next) return ({});
      return next->get_opts();
    }

    protected this_program `|(mixed thing)
    {
      if( !objectp(thing) || !thing->is_opt )
        error("Can only or %O with another %O.\n",
              this, this_program);

      if( next )
      {
        next = next | thing;
        return this;
      }

      next = thing;
      return this;
    }

    //! This function will be called by @expr{_sprintf@}, which
    //! handles formatting of chaining between objects.
    protected string __sprintf()
    {
      return sprintf("%O()", this_program);
    }

    protected string _sprintf(int t)
    {
      if( t!='O' ) return UNDEFINED;
      if( !next )
        return __sprintf();
      else
        return sprintf("%s|%O", __sprintf(), next);
    }
  }

  //! Wrapper class that converts the option argument to an integer.
  //! @example
  //!   Opt foo = Int(HasOpt("--foo")|Default(4711));
  class Int(Opt opt)
  {
    inherit Opt;

    mixed get_value(array(string) argv, mapping(string:string) env,
                    mixed previous)
    {
      return (int)opt->get_value(argv, env, previous);
    }

    array(string) get_opts()
    {
      return opt->get_opts();
    }

    protected this_program `|(mixed thing)
    {
      error("OR:ing Int objects not supported.\n");
    }

    protected string _sprintf(int t)
    {
      return t=='O' && sprintf("%O(%O)", this_program, opt);
    }
  }

  //! Wrapper class that allows multiple instances of an option.
  //! @example
  //!   Opt filter = Multiple(HasOpt("--filter"));
  class Multiple(Opt opt)
  {
    inherit Opt;

    protected mixed values = ({});

    mixed get_value(array(string) argv, mapping(string:string) env,
                    mixed previous)
    {
      mixed v = opt->get_value(argv, env, previous);
      if(undefinedp(v)) return UNDEFINED;
      values += ({ v });
      return values;
    }

    array(string) get_opts()
    {
      return opt->get_opts();
    }

    protected this_program `|(mixed thing)
    {
      error("OR:ing Multiple objects not supported.\n");
    }

    protected string _sprintf(int t)
    {
      return t=='O' && sprintf("%O(%O)", this_program, values);
    }
  }

  //! Parses an option without parameter, such as --help, -x or "x"
  //! from -axb.
  //!
  //! @example
  //!   Opt verbose = NoOpt("-v")|NoOpt("--verbose");
  class NoOpt
  {
    inherit Opt;
    protected string opt;

    protected void create(string _opt)
    {
      switch (sizeof(_opt))
      {
        default:
          if (has_prefix(_opt, "--"))
            break;
        case 2:
          if (_opt[0]=='-' && _opt[1]!='-')
            break;
        case 1:
        case 0:
          error("%O not a valid option.\n", _opt);
      }
      opt = _opt;
    }

    mixed get_value(array(string) argv, mapping(string:string) env,
                    mixed previous)
    {
      if( !sizeof(argv) ) return previous || ::get_value(argv, env, previous);

      if( argv[0]==opt )
      {
        argv[0] = 0;
        return (int)previous+1;
      }

      if (sizeof(opt) == 2 && sizeof(argv[0])>2
       && argv[0][0]=='-' && argv[0][1]!='-'
       && has_value(argv[0], opt[1..1]))
      {
        argv[0] -= opt[1..1];
        return (int)previous+1;
      }

      return previous || ::get_value(argv, env, previous);
    }

    array(string) get_opts()
    {
      return ({ opt }) + ::get_opts();
    }

    protected string __sprintf()
    {
      return sprintf("Arg.NoOpt(%O)", opt);
    }
  }

  //! Environment fallback for an option. Can of course be used as
  //! only Opt source.
  //!
  //! @example
  //!   Opt debug = NoOpt("--debug")|Env("MY_DEBUG");
  class Env
  {
    inherit Opt;
    protected string name;

    protected void create(string _name)
    {
      name = _name;
    }

    mixed get_value(array(string) argv, mapping(string:string) env,
                    mixed previous)
    {
      if( env[name] ) return env[name];
      return ::get_value(argv, env, previous);
    }

    protected string __sprintf()
    {
      return sprintf("Arg.Env(%O)", name);
    }
  }

  //! Default value for a setting.
  //!
  //! @example
  //!   Opt output = HasOpt("-o")|Default("a.out");
  class Default
  {
    inherit Opt;
    protected mixed value;

    protected void create(mixed _value)
    {
      value = _value;
    }

    mixed  get_value(array(string) argv, mapping(string:string) env,
                     mixed previous)
    {
      return value;
    }

    protected string __sprintf()
    {
      return sprintf("Arg.Default(%O)", value);
    }
  }

  //! Parses an option that may have a parameter. @tt{--foo@},
  //! @tt{-x@} and x in a sequence like @tt{-axb@} will set the
  //! variable to @expr{1@}. @tt{--foo=bar@}, @tt{-x bar@} and
  //! @tt{-x=bar@} will set the variable to @expr{bar@}.
  //!
  //! @example
  //!   Opt debug = MaybeOpt("--debug");
  class MaybeOpt
  {
    inherit NoOpt;

    mixed get_value(array(string) argv, mapping(string:string) env,
                    mixed previous)
    {
      if( !sizeof(argv) ) return previous || ::get_value(argv, env, previous);

      if( sizeof(opt) > 2 )
      {
        // --foo
        if( argv[0]==opt )
        {
          argv[0] = 0;
          return (int)previous+1;
        }

        // --foo=bar
        if( sscanf(argv[0], opt+"=%s", string ret)==1 )
        {
          argv[0] = 0;
          // FIXME: Make an array if previous is set?
          return ret;
        }

        return previous || ::get_value(argv, env, previous);
      }

      // -x
      if( sizeof(argv[0])>1 && argv[0][0]=='-' && argv[0][1]!='-' )
      {
        array parts = argv[0]/"=";

        if( has_value(parts[0], opt[1..1]) &&
            ( sizeof(parts)==1 ||
              parts[0][-1]!=opt[1] ) )
        {
          // -xy, -xy=z
          parts[0] -= opt[1..1];
          argv[0] = parts*"=";
          if(argv[0]=="-") argv[0] = 0;
          return (int)previous+1;
        }
        else if( sizeof(parts)>1 && parts[0][-1]==opt[1] )
        {
          // -yx=z
          parts[0] -= opt[1..1];
          if( parts[0]=="-" )
            argv[0] = 0;
          else
            argv[0] = parts[0];
          return parts[1..]*"=";
        }

        return previous || ::get_value(argv, env, previous);
      }

      return previous || ::get_value(argv, env, previous);
    }

    protected string __sprintf()
    {
      return sprintf("Arg.MaybeOpt(%O)", opt);
    }
  }

  //! Parses an option that has a parameter. @tt{--foo=bar@}, @tt{-x
  //! bar@} and @tt{-x=bar@} will set the variable to @expr{bar@}.
  //!
  //! @example
  //!   Opt user = HasOpt("--user")|HasOpt("-u");
  class HasOpt
  {
    inherit NoOpt;

    mixed get_value(array(string) argv, mapping(string:string) env,
                    mixed previous)
    {
      if( !sizeof(argv) ) return previous || ::get_value(argv, env, previous);

      if( sizeof(opt) > 2 )
      {
        // --foo bar
        if( argv[0]==opt )
        {
          if( sizeof(argv)>1 )
          {
            argv[0] = 0;
            string ret = argv[1];
            argv[1] = 0;
            return ret;
          }
          return 0; // FIXME: Signal failure
        }

        // --foo=bar
        if( sscanf(argv[0], opt+"=%s", string ret)==1 )
        {
          argv[0] = 0;
          return ret;
        }
        return previous || ::get_value(argv, env, previous);
      }

      if( sizeof(argv[0])>1 && argv[0][0]=='-' && argv[0][1]!='-' )
      {
        array parts = argv[0]/"=";
        if( sizeof(parts[0]) && parts[0][-1]==opt[1] )
        {
          if( sizeof(parts)==1 )
          {
            // "-xxxy z"
            if(sizeof(argv)>1)
            {
              parts[0] -= opt[1..1];
              if( parts[0]=="-" )
                argv[0] = 0;
              else
                argv[0] = parts[0];
              string ret = argv[1];
              argv[1] = 0;
              return ret;
            }

            // Fail. "-y" without any more elements in argv.
            return previous || ::get_value(argv, env, previous);
          }
          else
          {
            // "-xxxy=z"
            parts[0] -= opt[1..1];
            if( parts[0]=="-" )
              argv[0] = 0;
            else
              argv[0] = parts[0];
            return parts[1..]*"=";
          }
        }
      }

      return previous || ::get_value(argv, env, previous);
    }

    protected string __sprintf()
    {
      return sprintf("Arg.HasOpt(%O)", opt);
    }
  }

} // -- OptLibrary

protected class LookupKey
{
  protected string name;
  protected void create(string name) { this::name = name; }
  protected string _sprintf(int t) { return t=='O' && "Arg."+name; }
}

LookupKey REST = LookupKey("REST");
LookupKey APP  = LookupKey("APP");
LookupKey PATH = LookupKey("PATH");

//! @decl constant REST;
//!
//! Constant used to represent command line arguments not identified
//! as options. Can be used both in the response mapping from @[parse]
//! and when indexing an @[Options] object.

//! @decl constant APP;
//!
//! Constant used to represent the name of the application from the
//! command line. Can be used when indexing an @[Options] object.

//! @decl constant PATH;
//!
//! Constant used to represent the full path of the application from
//! the command line. Can be used when indexing an @[Options] object.

// FIXME: Support for rc files? ( Opt x = Opt("--x")|INIFile(path, name); )
// FIXME: Support for type casts? ( Opt level = Integer(Opt("--level"));

class LowOptions
  //!
{
  protected inherit OptLibrary;

  //!
  protected mapping(string:Opt) opts = ([]);

  //!
  protected mapping(string:int(1..1)|string) values = ([]);

  //!
  protected array(string) argv;

  //!
  protected string application;

  //!
  protected void create(array(string) _argv, void|mapping(string:string) env)
  {
    if(!env)
      env = getenv();

    // Make a list of all the arguments we can parse.
    foreach(::_indices(this, 0), string index)
    {
      mixed val = ::`[](index, this, 0);
      if(objectp(val) && val->is_opt) opts[index]=val;
    }

    application = _argv[0];
    argv = _argv[1..];
    mapping(string:Opt) unset = opts+([]);

    while(sizeof(argv))
    {
      array(string) pre = argv+({});
      foreach(opts; string index; Opt arg)
      {
        mixed value = arg->get_value(argv, env, values[index]);
        if(!undefinedp(value))
        {
          m_delete(unset, index);
          values[index] = value;
          argv -= ({ 0 });
        }
      }
      argv -= ({ 0 });

      if(equal(pre, argv))
      {
        unhandled_argument(argv, env);
        if(equal(pre, argv))
          break;
      }
    }

    if( sizeof(unset) )
    {
      mixed value;
      foreach(unset; string index; Opt arg)
      {
        value = arg->get_value(({}), env, values[index]);
        if(!undefinedp(value))
          values[index] = value;
      }
    }

  }

  protected string index(string i)
  {
    string s = ::`[](i, this, 1);
    if( !s ) return 0;
    if( !stringp(s) ) error("%O is not a string.\n", i);
    if( sizeof(s) )
    {
      return s;
    }
    return 0;
  }

  protected void usage()
  {
    string s = index("help_pre");
    if( s )
      write( "%s\n", s );

    foreach(sort((array)opts), [string i, Opt opt])
    {
      string opts = opt->get_opts()*", ";
      s = index(i+"_help");
      if ((sizeof(opts) > 23) || !s) {
	write( "\t" + opts + "\n");
	if (s) write("\t\t\t\t");
      } else if (sizeof(opts) > 15) {
	write( "\t" + opts + "\t");
      } else if (sizeof(opts) > 7) {
	write( "\t" + opts + "\t\t");
      } else {
	write( "\t" + opts + "\t\t\t");
      }
      if (s) {
	array(string) lines = s/"\n";
	foreach(lines, string line) {
	  if (sizeof(line) <= 46) {
	    write("%s\n", line);
	  } else {
	    line = sprintf("%-46=s", line);
	    array(string) a = line/"\n";
	    write("%s\n", a[0]);
	    foreach(a[1..], string l) {
	      write("\t\t\t\t%s\n", l);
	    }
	  }
	}
      }
    }

    s = index("help_post");
    if( s )
      write( "%s\n", s );
  }

  //!
  protected int(0..1) unhandled_argument(array(string) argv,
                                      mapping(string:string) env)
  {
    return 0;
  }

  //!
  protected mixed cast(string to)
  {
    switch( to )
    {
    case "mapping":
      return values + ([ REST : argv ]);
    case "array":
      return argv;
    }
    return UNDEFINED;
  }
}

//! The option parser class that contains all the argument objects.
//!
class Options
{
  inherit LowOptions;

  //! Options that trigger help output.
  Opt help = NoOpt("--help");
  protected string help_help = "Help about usage.";

  protected void create(array(string) argv, void|mapping(string:string) env)
  {
    ::create(argv, env);

    if (values->help) {
      usage();
    }
  }

  protected mixed `[](mixed id)
  {
    if( id==REST ) return argv;
    if( id==PATH ) return application;
    if( id==APP )  return basename(application);
    return values[id];
  }

  protected array _indices(object|void ctx, int|void access)
  {
    if (!access) {
      return ::_indices(this, access) + ({ REST, PATH, APP });
    }
    return ::_indices(ctx, access);
  }

  protected array _values(object|void ctx, int|void access)
  {
    if (!access) {
      return ::_values(this, access) +
	({ argv, application, basename(application) });
    }
    return ::_values(ctx, access);
  }

  protected array _types(object|void ctx, int|void access)
  {
    if (!access) {
      return map(predef::values(values) +
		 ({ argv, application, basename(application) }), _typeof);
    }
    return ::_types(ctx, access);
  }

  protected mixed `->(string id)
  {
    return values[id];
  }

  //!
  protected int(0..1) unhandled_argument(array(string) argv,
					 mapping(string:string) env)
  {
    if( !sizeof(argv) || argv[0]!="--help" ) return 0;
    if(!values->help) values->help=1;
  }
}


// --- Simple interface

class SimpleOptions
//! Options parser with a unhandled_argument implementation that
//! parses most common argument formats.
{
  inherit LowOptions;

 //! Handles arguments as described in @[Arg.parse]
  int(0..1) unhandled_argument(array(string) argv, mapping(string:string) env)
  {
    string arg = argv[0];
    if(!sizeof(arg) || arg[0]!='-') return 0;

    string name,value;

    if( has_prefix(arg, "--") )
    {
      sscanf( arg, "--%s=%s", name, value ) || sscanf( arg, "--%s", name );
      if(!name) return 0; // arg == "--"
      if( value )
        values[name] = value;
      else
        values[name]++;
      argv[0]=0;
      return 1;
    }

    sscanf( arg, "-%s=%s", name, value ) || sscanf( arg, "-%s", name );
    if( !name || !sizeof(name) ) return 0;
    foreach( name/1; int pos; string c )
    {
      if( value && pos == sizeof(name)-1 )
        values[c] = value;
      else
        values[c]++;
    }
    argv[0]=0;
    return 1;
  }
}

// Handles
// --foo         ->  "foo":1
// --foo=bar     ->  "foo":"bar"
// -bar          ->  "b":1,"a":1,"r":1
// -bar=foo      ->  "b":1,"a":1,"r":"foo" (?)
// --foo --bar   ->  "foo":1,"bar":1
// --foo - --bar ->  "foo":1
// --foo x --bar ->  "foo":1 (?)
// -foo          ->  "f":1,"o":2
// -x -x -x      ->  "x":3
//
// void main(int n, array argv)
// {
//   mapping opts = Arg.parse(argv);
//   argv = opts[Arg.REST];
// }

//! Convenience function for simple argument parsing.
//!
//! Handles the most common cases.
//!
//! The return value is a mapping from option name to the option value.
//!
//! The special index Arg.REST will be set to the remaining arguments
//! after all options have been parsed.
//!
//! The following argument syntaxes are supported:
//!
//! @code
//! --foo         ->  "foo":1
//! --foo=bar     ->  "foo":"bar"
//! -bar          ->  "b":1,"a":1,"r":1
//! -bar=foo      ->  "b":1,"a":1,"r":"foo" (?)
//! --foo --bar   ->  "foo":1,"bar":1
//! --foo - --bar ->  "foo":1
//! --foo x --bar ->  "foo":1 (?)
//! -foo          ->  "f":1,"o":2
//! -x -x -x      ->  "x":3
//! @endcode
//!
//! @example
//! @code
//! void main(int n, array argv)
//! {
//!   mapping opts = Arg.parse(argv);
//!   argv = opts[Arg.REST];
//!   if( opts->help ) /*... */
//! }
//! @endcode
mapping(string:string|int(1..)) parse(array(string) argv)
{
  return (mapping)SimpleOptions(argv);
}
