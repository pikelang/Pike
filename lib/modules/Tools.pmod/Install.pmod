#pike __REAL_VERSION__
//
// Common routines which are useful for various install scripts based on Pike.
//

array(string) features()
{
  array a = ({});
  
  if(!_static_modules["Regexp"])
    a += ({ "dynamic_modules" });

#if efun(thread_create)
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

  foreach(({ "_Crypto", "Dbm", "GL", "GTK", "Gdbm", "Gmp", "Gz",
	     "_Image_JPEG", "_Image_GIF", "_Image_TIFF", "_Image_TTF", 
	     "Image.PNG", "Java", "Mird",
	     "Msql", "Mysql", "Odbc", "Oracle", "Perl", "Postgres", "Ssleay",
	     "sybase", "_WhiteFish", "X"  }),
	  string modname)
  {
    catch
    {
      if(([ "Java":2 ])[modname] <
	 sizeof(indices(master()->resolv(modname) || ({}))))
      {
	if(modname[0] == '_')
	  modname = replace(modname[1..], "_", ".");
	a += ({ modname });
      }
    };
  }

  return a;
}

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

class ProgressBar
{
  private int width = 45;

  private float phase_base, phase_size;
  private int max, cur;
  private string name;

  void set_current(int _cur)
  {
    cur = _cur;
  }

  void set_name(string _name)
  {
    name = _name;
  }
  
  void set_phase(float _phase_base, float _phase_size)
  {
    phase_base = _phase_base;
    phase_size = _phase_size;
  }
  
  void update(int increment)
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
    
    write("\r   %-13s |%s%c%s%s %4.1f %%  ",
	  name+":",
	  "="*bar,
	  is_full ? '|' : spinner,
	  is_full ? "" : " "*(width-bar-1),
	  is_full ? "" : "|",
	  100.0 * ratio);
  }

  void create(string _name, int _cur, int _max,
	      float|void _phase_base, float|void _phase_size)
    /* NOTE: max must be greater than zero. */
  {
    name = _name;
    max = _max;
    cur = _cur;
    
    phase_base = _phase_base || 0.0;
    phase_size = _phase_size || 1.0 - phase_base;
  }
}

class Readline
{
  inherit Stdio.Readline;

  private int match_directories_only;
  private string cwd;

  void trap_signal(int n)
  {
    werror("\r\nInterrupted, exit.\r\n");
    destruct(this_object());
    exit(1);
  }

  void destroy()
  {
    ::destroy();
    signal(signum("SIGINT"));
  }

  static private string low_edit(string data, string|void local_prompt,
				 array(string)|void attrs)
  {
    string r = ::edit(data, local_prompt, (attrs || ({})) | ({ "bold" }));
    if(!r)
    {
      // ^D?
      werror("\nTerminal closed, exit.\n");
      destruct(this_object());
      exit(0);
    }
    return r;
  }
  
  string edit(mixed ... args)
  {
    return low_edit(@args);
  }
  
  string edit_filename(mixed ... args)
  {
    match_directories_only = 0;

    get_input_controller()->bind("^I", file_completion);
    string s = low_edit(@args);
    get_input_controller()->unbind("^I");
    
    return s;
  }
  
  string edit_directory(mixed ... args)
  {
    match_directories_only = 1;
    
    get_input_controller()->bind("^I", file_completion);
    string s = low_edit(@args);
    get_input_controller()->unbind("^I");
    
    return s;
  }
  
  static private string file_completion(string tab)
  {
    string text = gettext();
    int pos = getcursorpos();
    
    array(string) path = make_absolute_path(text[..pos-1], cwd)/"/";
    array(string) files =
      glob(path[-1]+"*",
	   get_dir(sizeof(path)>1? path[..sizeof(path)-2]*"/"+"/":".")||({}));

    if(match_directories_only)
      files = Array.filter(files, lambda(string f, string p)
				  { return (file_stat(p+f)||({0,0}))[1]==-2; },
			   path[..sizeof(path)-2]*"/"+"/");
    
    switch(sizeof(files))
    {
    case 0:
      get_output_controller()->beep();
      break;
    case 1:
      insert(files[0][sizeof(path[-1])..], pos);
      if((file_stat((path[..sizeof(path)-2]+files)*"/")||({0,0}))[1]==-2)
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

  string absolute_path(string path)
  {
    return make_absolute_path(path, cwd && combine_path(getcwd(), cwd));
  }
   
  void set_cwd(string _cwd)
  {
    cwd = _cwd;
  }
  
  void create(mixed ... args)
  {
    signal(signum("SIGINT"), trap_signal);
    ::create(@args);
  }
}
