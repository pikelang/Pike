//
// Argument parser
// By Martin Nilsson
// $Id: Arg.pike,v 1.3 2007/09/16 13:32:54 nilsson Exp $
//

#pike __REAL_VERSION__

//! Base class for parsing an argument. Inherit this class to create
//! custom made argument types.
class Arg
{
  constant is_arg = 1;
  static Arg next;

  //! Should return 1 for set options or a string containing the value
  //! of the option. Returning 0 means the option was not set (or
  //! matched). To properly chain arguments parsers, return
  //! @expr{::get_value(argv, env)@} instead of @expr{0@}, unless you
  //! want to explicitly stop the chain and not set this option.
  int(0..1)|string get_value(array(string) argv, mapping(string:string) env)
  {
    if(next) return next->get_value(argv, env);
    return 0;
  }

  //! Should return a list of arguments that is parsed. To properly
  //! chain argument parsers, return @expr{your_args + ::get_args()@}.
  array(string) get_args()
  {
    return next->get_args();
  }

  static this_program `|(mixed thing)
  {
    if( !objectp(thing) || !thing->is_arg )
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

  //! This function will be called by @expr{_sprintf@}, which handles
  //! formatting of chaining between objects.
  static string __sprintf()
  {
    return sprintf("%O()", this_program);
  }

  static string _sprintf(int t)
  {
    if( t!='O' ) return UNDEFINED;
    if( !next )
      return __sprintf();
    else
      return sprintf("%s|%O", __sprintf(), next);
  }
}

//! Parses an argument without parameter, such as --help, -x or "x"
//! from -axb.
//!
//! @example
//!   Arg verbose = NoArg("-v")|NoArg("--verbose");
class NoArg
{
  inherit Arg;
  static string arg;
  static int double;

  static void create(string _arg)
  {
    if( sizeof(_arg)>2 && has_prefix(_arg, "--") )
      double = 1;
    else if( sizeof(_arg)!=2 || _arg[0]!='-' || _arg=="--" )
      error("%O not a valid argument.\n", _arg);
    arg = _arg;
  }

  int(0..1)|string get_value(array(string) argv, mapping(string:string) env)
  {
    if( double )
    {
      if( argv[0]==arg )
      {
        argv[0] = 0;
        return 1;
      }
      return ::get_value(argv, env);
    }

    if( sizeof(argv[0])>1 && argv[0][0]=='-' && argv[0][1]!='-' )
    {
      array parts = argv[0]/"=";
      if( has_value(parts[0], arg[1..1]) )
      {
        parts[0] -= arg[1..1];
        argv[0] = parts*"=";
        if(argv[0]=="-") argv[0] = 0;
        return 1;
      }
    }

    return ::get_value(argv, env);
  }

  array(string) get_args()
  {
    return ({ arg }) + ::get_args();
  }

  static string __sprintf()
  {
    return sprintf("%O(%O)", this_program, arg);
  }
}

//! Environment fallback for an argument. Can of course be used as
//! only Arg source.
//!
//! @example
//!   Arg debug = NoArg("--debug")|Env("MY_DEBUG");
class Env
{
  inherit Arg;
  static string name;

  static void create(string _name)
  {
    name = _name;
  }

  int(0..1)|string get_value(array(string) argv, mapping(string:string) env)
  {
    if( env[name] ) return env[name];
    return ::get_value(argv, env);
  }

  static string __sprintf()
  {
    return sprintf("%O(%O)", this_program, name);
  }
}

//! Default value for a setting.
//!
//! @example
//!   Arg output = HasArg("-o")|Default("a.out");
class Default
{
  inherit Arg;
  static string value;

  static void create(string _value)
  {
    value = _value;
  }

  string get_value(array(string) argv, mapping(string:string) env)
  {
    return value;
  }

  static string __sprintf()
  {
    return sprintf("%O(%O)", this_program, value);
  }
}

//! Parses an argument that may have a parameter. @tt{--foo@},
//! @tt{-x@} and x in a sequence like @tt{-axb@} will set the variable
//! to @expr{1@}. @tt{--foo=bar@}, @tt{-x bar@} and @tt{-x=bar@} will
//! set the variable to @expr{bar@}.
//!
//! @example
//!   Arg debug = MaybyArg("--debug");
class MaybyArg
{
  inherit NoArg;

  int(0..1)|string get_value(array(string) argv, mapping(string:string) env)
  {
    if( double )
    {
      // --foo
      if( argv[0]==arg )
      {
        argv[0] = 0;
        return 1;
      }

      // --foo=bar
      if( sscanf(argv[0], arg+"=%s", string ret)==1 )
      {
        argv[0] = 0;
        return ret;
      }

      return ::get_value(argv, env);
    }

    // -x
    if( sizeof(argv[0])>1 && argv[0][0]=='-' && argv[0][1]!='-' )
    {
      array parts = argv[0]/"=";

      if( has_value(parts[0], arg[1..1]) &&
          ( sizeof(parts)==1 ||
            parts[0][-1]!=arg[1] ) )
      {
        // -xy, -xy=z
        parts[0] -= arg[1..1];
        argv[0] = parts*"=";
        if(argv[0]=="-") argv[0] = 0;
        return 1;
      }
      else if( sizeof(parts)>1 && parts[0][-1]==arg[1] )
      {
        // -yx=z
          parts[0] -= arg[1..1];
          if( parts[0]=="-" )
            argv[0] = 0;
          else
            argv[0] = parts[0];
          return parts[1..]*"=";
      }

      return ::get_value(argv, env);
    }

    return ::get_value(argv, env);
  }
}

//! Parses an argument that has a parameter. @tt{--foo=bar@}, @tt{-x
//! bar@} and @tt{-x=bar@} will set the variable to @expr{bar@}.
//!
//! @example
//!   Arg user = HasArg("--user")|HasArg("-u");
class HasArg
{
  inherit NoArg;

  int(0..1)|string get_value(array(string) argv, mapping(string:string) env)
  {
    if( double )
    {
      // --foo bar
      if( argv[0]==arg )
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
      if( sscanf(argv[0], arg+"=%s", string ret)==1 )
      {
        argv[0] = 0;
        return ret;
      }
      return ::get_value(argv, env);
    }

    if( sizeof(argv[0])>1 && argv[0][0]=='-' && argv[0][1]!='-' )
    {
      array parts = argv[0]/"=";
      if( sizeof(parts[0]) && parts[0][-1]==arg[1..1] )
      {
        if( sizeof(parts)==1 )
        {
          // "-xxxy z"
          if(sizeof(argv)>1)
          {
            parts[0] -= arg[1..1];
            if( parts[0]=="-" )
              argv[0] = 0;
            else
              argv[0] = parts[0];
            string ret = argv[1];
            argv[1] = 0;
            return ret;
          }

          // Fail. "-y" without any more elements in argv.
          return ::get_value(argv, env);
        }
        else
        {
          // "-xxxy=z"
          parts[0] -= arg[1..1];
          if( parts[0]=="-" )
            argv[0] = 0;
          else
            argv[0] = parts[0];
          return parts[1..]*"=";
        }
      }
    }

    return ::get_value(argv, env);
  }
}

// FIXME: Support for rc files? ( Arg x = Arg("--x")|INIFile(path, name); )
// FIXME: Support for type casts? ( Arg level = Integer(Arg("--level"));

class LowOptions
{
  static mapping(string:Arg) args = ([]);
  static mapping(string:int(1..1)|string) values = ([]);
  static array(string) argv;

  static void create(array(string) _argv, void|mapping(string:string) env)
  {
    if(!env)
      env = getenv();

    // Make a list of all the arguments we can parse.
    foreach(::_indices(2), string index)
    {
      mixed val = ::`[](index, 2);
      if(objectp(val) && val->is_arg) args[index]=val;
    }

    argv = _argv[1..];
    mapping(string:Arg) unset = args+([]);
    while(1)
    {
      if(!sizeof(argv)) break;

      int(0..1)|string value;
      foreach(unset; string index; Arg arg)
      {
        value = arg->get_value(argv, env);
        if(value)
        {
          m_delete(unset, index);
          values[index] = value;
          break;
        }
      }

      if(!value)
        value = unhandled_argument(argv, env);

      if(!value)
        break;
      else
        while( sizeof(argv) && argv[0] == 0 )
          argv = argv[1..];
    }
    if( sizeof(unset) )
    {
      // FIXME: Make sure all fallbacks are properly fetched. Call
      // with (0, env)?
    }

  }

  static int(0..1) unhandled_argument(array(string) argv,
                                      mapping(string:string) env)
  {
    return 0;
  }
}

//! The option parser class that contains all the argument objects.
//!
class Options
{
  inherit LowOptions;

  static string|int `[](string id)
  {
    return values[id];
  }
  static string|int `->(string id)
  {
    return values[id];
  }

  static int(0..1)|string unhandled_argument(array(string) argv,
                                             mapping(string:string) env)
  {
    if( argv[0]!="--help" ) return 0;

    string s = index("help_pre");
    if( s )
      write( s+"\n" );

    foreach(args; string i; Arg arg)
    {
      // FIXME: Make a list of the attibutes parsed by arg.

      s = index(i+"_help");
      if( s )
        write( s ); // FIXME: Format
    }

    s = index("help_post");
    if( s )
      write( "\n"+s );
  }

  static string index(string i)
  {
    string s = ::`[](i, 2);
    if( !s ) return 0;
    if( !stringp(s) ) error("%O is not a string.\n", i);
    if( sizeof(s) )
    {
      if( s[-1]!='\n' )
        s += "\n";
      return s;
    }
    return 0;
  }
}


// --- Simple interface

class SimpleOptions
{
  inherit LowOptions;

  int(0..1) unhandled_argument(array(string) argv, mapping(string:string) env)
  {
    string arg = argv[0];
    if(!sizeof(arg) || arg[0]!='-') return 0;

    string name,value;

    if( has_prefix(arg, "--") )
    {
      sscanf( arg, "--%s=%s", name, value );
      if(!name) return 0; // arg == "--"
      values[name] = value||1;
      return 1;
    }

    sscanf( arg, "-%s=%s", name, value );
    if( !name ) return 0;
    foreach( name/1; int pos; string c )
      if( pos == sizeof(name)-1 )
        values[name] = value||1;
      else
        values[name] = 1;
    return 1;
  }
}

// Handles
// --foo         ->  "foo":1
// --foo=bar     ->  "foo":"bar"
// -bar          ->  "b":1,"a":1,"r":1
// -bar=foo      ->  "b":1,"a":1,"r":"foo" (?)
// --foo --bar   ->  "foo":1,"bar":1
// --foo - --bar -> "foo":1
// --foo x --bar -> "foo":1 (?)
//
// void main(int n, array argv)
// {
//   mapping args = Arg.parse(argv);
//   argv = args[Arg.REST];
// }


mapping(string:string|int(1..1)) parse(array(string) argv)
{
  return (mapping)SimpleOptions(argv);
}


// --- test stuff

class Getopt
{
  inherit Options;
  Arg verbose = NoArg("-v")|NoArg("--verbose")|Env("VERBOSE");
}


void main(int num, array args)
{
  werror("%O\n", Getopt( args )->verbose );
}
