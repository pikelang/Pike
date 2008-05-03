//
// Argument parser
// By Martin Nilsson
// $Id: Arg.pmod,v 1.5 2008/05/03 17:14:50 nilsson Exp $
//

#pike __REAL_VERSION__

class OptLibrary
{

  //! Base class for parsing an argument. Inherit this class to create
  //! custom made option types.
  class Opt
  {
    constant is_opt = 1;
    static Opt next;

    //! Should return 1 for set options or a string containing the
    //! value of the option. Returning 0 means the option was not set
    //! (or matched). To properly chain arguments parsers, return
    //! @expr{::get_value(argv, env)@} instead of @expr{0@}, unless
    //! you want to explicitly stop the chain and not set this option.
    int(0..1)|string get_value(array(string) argv, mapping(string:string) env)
    {
      if(next) return next->get_value(argv, env);
      return 0;
    }

    //! Should return a list of options that are parsed. To properly
    //! chain argument parsers, return @expr{your_opts +
    //! ::get_opts()@}.
    array(string) get_opts()
    {
      if(!next) return ({});
      return next->get_opts();
    }

    static this_program `|(mixed thing)
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

  //! Parses an option without parameter, such as --help, -x or "x"
  //! from -axb.
  //!
  //! @example
  //!   Opt verbose = NoOpt("-v")|NoOpt("--verbose");
  class NoOpt
  {
    inherit Opt;
    static string opt;
    static int double;

    static void create(string _opt)
    {
      if( sizeof(_opt)>2 && has_prefix(_opt, "--") )
        double = 1;
      else if( sizeof(_opt)!=2 || _opt[0]!='-' || _opt=="--" )
        error("%O not a valid option.\n", _opt);
      opt = _opt;
    }

    int(0..1)|string get_value(array(string) argv, mapping(string:string) env)
    {
      if( !sizeof(argv) ) return ::get_value(argv, env);

      if( double )
      {
        if( argv[0]==opt )
        {
          argv[0] = 0;
          return 1;
        }
        return ::get_value(argv, env);
      }

      if( sizeof(argv[0])>1 && argv[0][0]=='-' && argv[0][1]!='-' )
      {
        array parts = argv[0]/"=";
        if( has_value(parts[0], opt[1..1]) )
        {
          parts[0] -= opt[1..1];
          argv[0] = parts*"=";
          if(argv[0]=="-") argv[0] = 0;
          return 1;
        }
      }

      return ::get_value(argv, env);
    }

    array(string) get_opts()
    {
      return ({ opt }) + ::get_opts();
    }

    static string __sprintf()
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

    int(0..1)|string get_value(array(string) argv, mapping(string:string) env)
    {
      if( !sizeof(argv) ) return ::get_value(argv, env);

      if( double )
      {
        // --foo
        if( argv[0]==opt )
        {
          argv[0] = 0;
          return 1;
        }

        // --foo=bar
        if( sscanf(argv[0], opt+"=%s", string ret)==1 )
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

        if( has_value(parts[0], opt[1..1]) &&
            ( sizeof(parts)==1 ||
              parts[0][-1]!=opt[1] ) )
        {
          // -xy, -xy=z
          parts[0] -= opt[1..1];
          argv[0] = parts*"=";
          if(argv[0]=="-") argv[0] = 0;
          return 1;
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

        return ::get_value(argv, env);
      }

      return ::get_value(argv, env);
    }

    static string __sprintf()
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

    int(0..1)|string get_value(array(string) argv, mapping(string:string) env)
    {
      if( !sizeof(argv) ) return ::get_value(argv, env);

      if( double )
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
        return ::get_value(argv, env);
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
            return ::get_value(argv, env);
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

      return ::get_value(argv, env);
    }

    static string __sprintf()
    {
      return sprintf("Arg.HasOpt(%O)", opt);
    }
  }

} // -- OptLibrary

object REST = class {
    static string _sprintf(int t)
    {
      return "Arg.REST";
    }
  }();

// FIXME: Support for rc files? ( Opt x = Opt("--x")|INIFile(path, name); )
// FIXME: Support for type casts? ( Opt level = Integer(Opt("--level"));

class LowOptions
{
  static inherit OptLibrary;

  static mapping(string:Opt) opts = ([]);
  static mapping(string:int(1..1)|string) values = ([]);
  static array(string) argv;
  static string application;

  static void create(array(string) _argv, void|mapping(string:string) env)
  {
    if(!env)
      env = getenv();

    // Make a list of all the arguments we can parse.
    foreach(::_indices(2), string index)
    {
      mixed val = ::`[](index, 2);
      if(objectp(val) && val->is_opt) opts[index]=val;
    }

    application = _argv[0];
    argv = _argv[1..];
    mapping(string:Opt) unset = opts+([]);

    while(1)
    {
      if(!sizeof(argv)) break;

      int(0..1)|string value;
      foreach(unset; string index; Opt arg)
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
      int(0..1)|string value;
      foreach(unset; string index; Opt arg)
      {
        value = arg->get_value(({}), env);
        if(value)
        {
          m_delete(unset, index);
          values[index] = value;
        }
      }
    }

  }

  static int(0..1) unhandled_argument(array(string) argv,
                                      mapping(string:string) env)
  {
    return 0;
  }

  static mixed cast(string to)
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
    if( !sizeof(argv) || argv[0]!="--help" ) return 0;

    string s = index("help_pre");
    if( s )
      write( s+"\n" );

    foreach(opts; string i; Opt opt)
    {
      write( opt->get_opts()*", " + "\n");
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
      sscanf( arg, "--%s=%s", name, value ) || sscanf( arg, "--%s", name );
      if(!name) return 0; // arg == "--"
      values[name] = value||1;
      argv[0]=0;
      return 1;
    }

    sscanf( arg, "-%s=%s", name, value ) || sscanf( arg, "-%s", name );
    if( !name || !sizeof(name) ) return 0;
    foreach( name/1; int pos; string c )
      if( pos == sizeof(name)-1 )
        values[c] = value||1;
      else
        values[c] = 1;
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
// --foo - --bar -> "foo":1
// --foo x --bar -> "foo":1 (?)
//
// void main(int n, array argv)
// {
//   mapping opts = Arg.parse(argv);
//   argv = opts[Arg.REST];
// }


mapping(string:string|int(1..1)) parse(array(string) argv)
{
  return (mapping)SimpleOptions(argv);
}
