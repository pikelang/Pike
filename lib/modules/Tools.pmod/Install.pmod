#pike __REAL_VERSION__

//!
//! Common routines which are useful for various install scripts based on Pike.
//!

//! Return an array of enabled features.
//!
//! @note
//!   Used by the @[master] when given the option @tt{--features@}.
//!
//! @seealso
//!   @[Tools.Standalone.features]
array(string) features()
{
  array a = ({}), m = ({});

  mapping runtime_info = Pike.get_runtime_info();

  if (runtime_info->auto_bignum)
    a += ({"auto_bignum"});

  if (!(<"default">)[runtime_info->bytecode_method])
    a += ({"machine_code"});

#if constant(load_module)
  a += ({ "dynamic_modules" });
#endif

#if constant(thread_create)
  a += ({ "threads" });
#endif

#if constant(Stdio.__HAVE_OOB__)
  a += ({ "out-of-band_data" });
#endif

#if constant(__builtin.__DOUBLE_PRECISION_FLOAT__)
  a += ({ "double_precision_float" });
#endif
#if constant(__builtin.__LONG_DOUBLE_PRECISION_FLOAT__)
  a += ({ "long_double_precision_float" });
#endif
#if constant(__builtin.__SINGLE_PRECISION_FLOAT__)
  a += ({ "single_precision_float" });
#endif
#if constant(get_profiling_info)
  a += ({ "profiling" });
#endif

#if constant(_debug)
  a += ({ "rtl_debug" });
#endif

#if 0
  // No use reporting stuff that always exists. This list is for
  // things that might not be compiled in due to configure options,
  // missing libs, etc. /mast
  m += ({ "PostgresNative" });
#endif

  foreach(({ "Nettle", "Crypto.AES.GCM", "Crypto.ECC.Curve", "Dbm", "DVB",
             "_Ffmpeg", "GL", "GLUT", "Gdbm", "Crypto.ECC.Curve25519",
	     "Gmp", "Gz", "_Image_FreeType", "_Image_GIF", "_Image_JPEG",
             "_Image_TIFF", "_Image_TTF", "_Image_XFace", "Image.PNG",
             "_Image_WebP",
	     "Java.machine", "Mird", "Msql", "Mysql", "Odbc", "Oracle",
	     "PDF.PDFlib", "Perl",
             "Postgres", "SANE", "SDL", "Ssleay", "Yp", "sybase", "_WhiteFish",
	     "X", "Bz2", "COM", "Fuse", "GTK2", "Gettext", "HTTPAccept",
	     "Kerberos", "SQLite", "_Image_SVG", "_Regexp_PCRE", "GSSAPI",
	     "Protocols.DNS_SD", "Gnome2", "MIME", "Standards.JSON",
	     "Web.Sass", "VCDiff", "ZXID", "System.FSEvents.EventStream",
	     "System.Inotify" }),
	  string modname)
  {
    catch
    {
      object tmp;
      if(sizeof(indices((tmp = master()->resolv(modname)) || ({})) -
		({ "dont_dump_module" })) ||
	 (tmp && !objectp(tmp)))
      {
	if(modname[0] == '_')
	  modname = replace(modname[1..], "_", ".");
	m += ({ (["Java.machine":"Java"])[modname] || modname });

	if (modname == "Mysql") {
	  // Check taste of Mysql client library.
	  //
	  // Classic:	"MySQL (Copyright Abandoned)/3.23.49"
	  // Mysql GPL:	"MySQL Community Server (GPL)/5.5.30"
	  // MariaDB:	"MySQL (Copyright Abandoned)/5.5.0"
	  string client_ver = tmp["client_info"] && tmp["client_info"]();
	  string license = "Unknown";
	  string version = "Unknown";
	  sscanf(client_ver, "%*s(%s)/%s", license, version);
	  if ((license == "Copyright Abandoned") && (version > "5")) {
	    m += ({ "MariaDB" });
	  }
	}
      }
    };
  }

  foreach (({"Regexp.PCRE.Widestring", "Java.NATIVE_METHODS"}), string symbol)
    catch {
      if (has_index(all_constants(), symbol) ||
	  !undefinedp(master()->resolv(symbol)))
	m += ({symbol});
    };

  return a + sort (m);
}

//!
string make_absolute_path(string path, string|void cwd)
{
#if constant(getpwnam)
  if(sizeof(path) && path[0] == '~')
  {
    string user, newpath;
    sscanf(path, "~%s/%s", user, newpath);

    if(user && sizeof(user))
    {
      array a = getpwnam(user);
      if(a && sizeof(a) >= 7)
	return combine_path(a[5], newpath);
    }

    return combine_path(getenv("HOME"), path[2..]);
  }
#endif

  if(!sizeof(path) || path[0] != '/')
    return combine_path(cwd || getcwd(), "./", path);

  return path;
}

//! A class keeping some methods and state to conveniently render
//! ASCII progress bars to stdout.
class ProgressBar
{
  private int width = 45;

  private float phase_base, phase_size;
  private int max, cur;
  private string name;

  //! Change the amount of progress without updating on stdout.
  void set_current(int _cur)
  {
    cur = _cur;
  }

  //! Change the name of the progress bar without updating on stdout.
  void set_name(string _name)
  {
    name = _name;
  }

  //!
  void set_phase(float _phase_base, float _phase_size)
  {
    phase_base = _phase_base;
    phase_size = _phase_size;
  }

  //! Write the current look of the progressbar to stdout.
  //! @param increment
  //!   the number of increments closer to completion since last call
  //! @returns
  //!   the length (in characters) of the line with the progressbar
  int update(int increment)
  {
    cur += increment;
    cur = min(cur, max);

    float ratio = phase_base + ((float)cur/(float)max) * phase_size;
    if(1.0 < ratio)
      ratio = 1.0;

    int bar = (int)(ratio * (float)width);
    int is_full = (bar == width);

    // int spinner = (max < 2*width ? '=' : ({ '\\', '|', '/', '-' })[cur&3]);
    int spinner = '=';

    return write("\r   %-13s |%s%c%s%s %4.1f %%  ",
		 name+":",
		 "="*bar,
		 is_full ? '|' : spinner,
		 is_full ? "" : " "*(width-bar-1),
		 is_full ? "" : "|",
		 100.0 * ratio) - 1;
  }

  //! @decl void create(string name, int cur, int max, float|void phase_base,@
  //!                   float|void phase_size)
  //! @param name
  //! The name (printed in the first 13 columns of the row)
  //! @param cur
  //! How much progress has been made so far
  //! @param max
  //! The amount of progress signifying 100% done. Must be greater than zero.
  protected void create(string _name, int _cur, int _max,
			float|void _phase_base, float|void _phase_size)
  {
    name = _name;
    max = _max;
    cur = _cur;

    phase_base = _phase_base || 0.0;
    phase_size = _phase_size || 1.0 - phase_base;
  }
}

//!
class Readline
{
  inherit Stdio.Readline;

  private int match_directories_only;
  private string cwd;

  //!
  void trap_signal(int n)
  {
    werror("\r\nInterrupted, exit.\r\n");
    destruct(this);
    exit(1);
  }

  protected void _destruct()
  {
    ::_destruct();
    signal(signum("SIGINT"));
  }

  protected private string low_edit(string data, string|void local_prompt,
				 array(string)|void attrs)
  {
    string r = ::edit(data, local_prompt, (attrs || ({})) | ({ "bold" }));
    if(!r)
    {
      // ^D?
      werror("\nTerminal closed, exit.\n");
      destruct(this);
      exit(0);
    }
    return r;
  }

  //!
  string edit(mixed ... args)
  {
    return low_edit(@args);
  }

  //!
  string edit_filename(mixed ... args)
  {
    match_directories_only = 0;

    get_input_controller()->bind("^I", file_completion);
    string s = low_edit(@args);
    get_input_controller()->unbind("^I");

    return s;
  }

  //!
  string edit_directory(mixed ... args)
  {
    match_directories_only = 1;

    get_input_controller()->bind("^I", file_completion);
    string s = low_edit(@args);
    get_input_controller()->unbind("^I");

    return s;
  }

  protected private string file_completion(string tab)
  {
    string text = gettext();
    int pos = getcursorpos();

    array(string) path = make_absolute_path(text[..pos-1], cwd)/"/";
    array(string) files =
      glob(path[-1]+"*",
	   get_dir(sizeof(path)>1? path[..<1]*"/"+"/":".")||({}));

    if(match_directories_only)
      files = Array.filter(files, lambda(string f, string p)
				  { return Stdio.is_dir(p + f); },
			   path[..<1]*"/"+"/");

    switch(sizeof(files))
    {
    case 0:
      get_output_controller()->beep();
      break;
    case 1:
      insert(files[0][sizeof(path[-1])..], pos);
      if( Stdio.is_dir( (path[..<1]+files) * "/" ) )
	insert("/", getcursorpos());
      break;
    default:
      string pre = String.common_prefix(files)[sizeof(path[-1])..];
      if(sizeof(pre))
      {
	insert(pre, pos);
      } else {
	if(!sizeof(path[-1]))
	  files = Array.filter(files, lambda(string f)
				      { return !(sizeof(f) && f[0] == '.'); });
	list_completions(sort(files));
      }
      break;
    }
  }

  //!
  string absolute_path(string path)
  {
    return make_absolute_path(path, cwd && combine_path(getcwd(), cwd));
  }

  //!
  void set_cwd(string _cwd)
  {
    cwd = _cwd;
  }

  protected void create(mixed ... args)
  {
    signal(signum("SIGINT"), trap_signal);
    ::create(@args);
  }
}
