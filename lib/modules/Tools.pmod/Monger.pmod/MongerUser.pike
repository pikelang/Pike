// -*- Pike -*-

#pike __REAL_VERSION__

constant version =
 sprintf("%d.%d.%d",(int)__REAL_VERSION__,__REAL_MINOR__,__REAL_BUILD__);
constant description = "Monger: the Pike module manger.";

constant SOURCE_PACKAGE = 0;
constant SOURCE_CONTROL = 1;
constant PURE_PIKE_PMAR = 2;
constant PLATFORM_SPECIFIC_PMAR = 3;

string repository = "http://modules.gotpike.org:8000/xmlrpc/index.pike";
string builddir = combine_path(System.get_home()||".", ".monger");

int use_force = 0;
int use_local = 0;
int use_short = 0;
int use_source = 0;
int use_pmar = 0;
int show_urls = 0;
string my_command;
string argument;
string my_version;
string run_pike;
array(string) pike_args = ({});
string original_dir;

object repo_obj;

mapping created = (["files" : ({}), "dirs": ({})]);

int main(int argc, array(string) argv)
{
  run_pike = master()->_pike_file_name;
#ifdef NOT_INSTALLED
  pike_args += ({ "-DNOT_INSTALLED" });
#endif
#ifdef PRECOMPILED_SEARCH_MORE
  pike_args += ({ "-DPRECOMPILED_SEARCH_MORE" });
#endif
  if(master()->_master_file_name)
    pike_args += ({ "-m"+master()->_master_file_name });
  putenv("RUNPIKE", run_pike+sizeof(pike_args)?" "+pike_args*" ":"");

  if(argc==1) return do_help();

  array opts = Getopt.find_all_options(argv,aggregate(
    ({"list",Getopt.NO_ARG,({"--list"}) }),
    ({"download",Getopt.NO_ARG,({"--download"}) }),
    ({"query",Getopt.NO_ARG,({"--query"}) }),
    ({"show_urls",Getopt.NO_ARG,({"--show-urls"}) }),
    ({"repository",Getopt.HAS_ARG,({"--repository"}) }),
    ({"builddir",Getopt.HAS_ARG,({"--builddir"}) }),
    ({"install",Getopt.NO_ARG,({"--install"}) }),
    ({"local",Getopt.NO_ARG,({"--local"}) }),
    ({"source",Getopt.NO_ARG,({"--source"}) }),
    ({"pmar",Getopt.NO_ARG,({"--pmar"}) }),
    ({"version",Getopt.HAS_ARG,({"--version"}) }),
    ({"force",Getopt.NO_ARG,({"--force"}) }),
    ({"short",Getopt.NO_ARG,({"--short"}) }),
    ({"help",Getopt.NO_ARG,({"--help"}) }),
    ));

  argv=Getopt.get_args(argv);

  if(sizeof(argv)>2) 
    exit(1, "Too many extra arguments!\n");
  else if(sizeof(argv)>1)
    argument = argv[1];

  foreach(opts,array opt)
  {
    switch(opt[0])
    {
      case "repository":
        repository = opt[1];
        break;
      case "show_urls":
        show_urls = 1;
        break;
      case "builddir":
        Stdio.Stat fs = file_stat(opt[1]);
        if(fs && fs->isdir)
          builddir = opt[1];
        else
          exit(1, "Build directory " + opt[1] + " does not exist.\n");
        break;
      case "version":
        my_version = opt[1];
        break;
      case "force":
        use_force = 1;
        break;
      case "source":
        use_source = 1;
        break;
      case "pmar":
        use_pmar = 1;
        break;      
      case "short":
        use_short = 1;
        break;
      case "local":
        use_local = 1;
        // local installs should have the local module path added.
        add_module_path(get_local_modulepath());
        break;
    }
  }

  // create the build directory if it doesn't exist.
  Stdio.Stat fs = file_stat(builddir);
  if(!fs)
    mkdir(builddir);
  else if (!fs->isdir)
    exit(1, "Build directory " + builddir + " is not a directory.\n");
  
  
  foreach(opts,array opt)
  {
    switch(opt[0])
    {
      case "help":
        return do_help();
        break;
      case "list":
        my_command=opt[0];
        if(argument && strlen(argument))
          do_list(argument);
        else
          do_list();
        break;
      case "download":
        my_command=opt[0];
        if(argument)
          do_download(argument, my_version||UNDEFINED);
        else
          exit(1, "download error: module name must be specified\n");
        break;
      case "install":
        my_command=opt[0];
        function f;
        if(use_pmar)
          f = do_pmar_install;
        else 
          f = do_install;
        if(argument)
          f(argument, my_version||UNDEFINED);
        else
          exit(1, "install error: module name must be specified\n");
        break;
      case "query":
        my_command=opt[0];
        if(argument)
          do_query(argument, my_version||UNDEFINED);
        else
          exit(1, "query error: module name must be specified\n");
        break;
    }
  }

 return 0;
}

int(0..0) do_help()
{
  write(
#"This is Monger, the manager for Fresh Pike.

Usage: pike -x monger [options] modulename

--list               list known modules in the repository, optionally
                       limited to those whose name contains the last 
                       argument in the argument list (modulename)
--query              retrieves information about module modulename
--show-urls          display download and other urls in query view
--download           download the module modulename
--install            install the module modulename
--repository=url     sets the repository source to url
--builddir=path      sets the build directory to path, default is 
                       the $HOME/.monger
--force              force the performance of an action that might 
                       otherwise generate a warning
--source             use source control url to obtain module source
--pmar               attempt to use pmar (pre-built archive)
--short              when querying a module, don't show info about the 
                       selected version.
--local              perform installation in a user's local rather than 
                       system directory
--version=ver        work with the specified version of the module
");
  return 0;
}


void do_query(string name, string|void version)
{
  string rversion;
  int module_id;
  mapping vi = get_module_info(name);
  if( !vi ) return;
  module_id = (int)vi->module_id;
  
  write("Module: %s\n", vi->name);
  write("Description: %s\n", vi->description);
  write("Author/Owner: %s\n", vi->owner);
  if(vi->source_control_type)
    write("%s URL: %s\n", vi->source_control_type, vi->source_control_url);


  array versions = get_module_versions(module_id);
  
  if(sizeof(versions))
  {
    write("Current version: %s\n", versions[0]);
    rversion = get_recommended_module_version(module_id);
    if(rversion)
      write("Recommended version: %s (for Pike %s)\n", rversion, get_pike_version());
    else
      write("No recommended version (for Pike %s)\n", get_pike_version());    
  }
  else
  {
    write("No released versions.\n");
  }

  if(!use_short && version)
  {
    mapping svi;
    catch(svi = get_module_version_info(module_id, version));

    if(!svi) 
    {
      write("Version %s not available.\n", (string)version);
      return 0;
    }
    
    if(rversion && rversion == version) svi->version_type = "recommended";
    else 
    {
      array rvs = get_compatible_module_versions(module_id);
      if(rvs && search(rvs, version) != -1)
        svi->version_type = "compatible";
      else
        svi->version_type = "not recommended";
    }
    write("Version: %s (%s)\t", svi->version, svi->version_type);
    write("License: %s\n", svi->license);
    write("Changes: %s\n\n", svi->changes);
    if(svi->download && show_urls)
    {
      if(stringp(svi->download))
        write("Download URL: %s\n\n", svi->download);
      else if(arrayp(svi->download))
        foreach(svi->download;;string u)
          write("Download URL: %s\n\n", u);   
      if(stringp(svi->pmar_download))
        write("PMAR Download URL: %s\n\n", svi->pmar_download);
    }

    if(svi->download)
      write("This module version is available for automated installation.\n");
    if(svi->has_pmar)
      write("A prebuilt module archive (PMAR) is available for this version.\n");
  }

  catch 
  {
    string local_ver = master()->resolv(name)["__version"];

    if(local_ver) write("Version %s of this module is currently installed.\n", 
                        local_ver);
  };
}

object get_repository()
{
  if(!repo_obj)
    repo_obj = xmlrpc_handler(repository);
  return repo_obj;
}

string get_pike_version()
{
  return sprintf("%d.%d.%d", (int)__REAL_MAJOR__, 
      (int)__REAL_MINOR__, (int)__REAL_BUILD__);
}

mapping get_module_info(string name)
{
  int module_id;
  mixed err;
  
  err = catch {
    module_id = get_repository()->get_module_id(name);
  };
  if(err)
  {
    werror("An error occurred: %s", err[0]);
    return 0;
  }

  mapping vi = get_repository()->get_module_info((int)module_id);

  return vi;
}

array get_compatible_module_versions(int|string module)
{
  if(stringp(module))
    module = get_repository()->get_module_id(module);
  
  array v = get_repository()->get_compatible_module_versions((int)module, get_pike_version());

  return v;
}

string get_recommended_module_version(int|string module)
{
  if(stringp(module))
    module = get_repository()->get_module_id(module);
  
  string v = get_repository()->get_recommended_module_version((int)module, get_pike_version());

  return v;
}

array(string) get_module_versions(int|string module)
{
  if(stringp(module))
    module = get_repository()->get_module_id(module);
  
  array v = get_repository()->get_module_versions((int)module);

  return v;
}

mapping get_module_version_info(int|string module, string version)
{
  if(stringp(module))
    module = get_repository()->get_module_id(module);
  
  mapping v = get_repository()->get_module_version_info((int)module, version);

  return v;
}

// 
mapping get_module_action_data(string name, string|void version)
{
  string dv;
  mixed err;
  mapping info = get_module_info(name);
  
  string v;

  err = catch {
    v = get_recommended_module_version(name);
  };

  if(err)
    write("an error occurred while getting the recommended version.\n");

  info->version_type="recommended";

  if(!v)
    info->version_type = "not found";  
  else if(version && version != v)
    info->version_type="not recommended";
  
  if(version && use_force)
  {
    dv=my_version;
  }
  else if(version && version!=v)
  {
    write("Requested version %s is not the recommended version.\n"
          "use --force to force %s of this version.\n", 
          version, my_command);
    return 0;    
  }
  else if(version)
  {
    write("Selecting requested and recommended version %s for %s.\n", 
          v, my_command);
    dv=version;
  }
  else if(v)
  {
    write("Selecting recommended version %s for %s.\n", v, my_command);
    dv=v;
  }
  else
  {
    write("repository error: no recommended version to %s.\n"
	 "use --force --version=ver to force %s of a particular version.\n",
         my_command, my_command);
     return 0;
  }
  mapping vi = get_module_version_info(name, dv);

  return vi + info;
}

string get_file(mapping version_info, string|void path, int|void from_source)
{
  array rq;
  int have_path;

  // NOTE: if we are re-using an existing source control clone or checkout, we 
  // need to do some more work here, as it's probably not good to just do a 
  // build from a directory that might have outdated build by-products left in
  // it. We should do a git clean or equivalent.
  if(from_source == SOURCE_CONTROL && version_info->source_control_url && sizeof( version_info->source_control_type))
  {
    write("fetching source from source control (%s)...\n", version_info->source_control_type);
    string bin = lower_case(version_info->source_control_type);
    string lpath = version_info->name + "-source";
    if(path) lpath = Stdio.append_path(path, lpath);
    if(file_stat(lpath))
    {
	  have_path = 1;
	}
	if(have_path && !use_force)
	{
      throw(Error.Generic(sprintf("get_file: repository path %s already exists, use --force to override.\n", lpath)));
	}
	
    if(!Process.search_path(bin))
      throw(Error.Generic(sprintf("get_file: no %s found in PATH.\n", bin)));
    mapping res;
    string oldcwd = getcwd();

    switch(bin)
    {
      case "svn":
        if(have_path)
        {  
          cd(lpath);
          res = Process.run(({"svn", "update"}));
          cd(oldcwd);
        }
        else
        {
          res = Process.run(({"svn", "checkout", version_info->source_control_url, lpath}));
        }
        if(res->exitcode)
        {
          werror(res->stderr);
          throw(Error.Generic(bin + " returned non-zero exit code (" + res->exitcode + ").\n"));
        }
        break;
      case "hg":
        if(have_path)
        {
          cd(lpath);
          res = Process.run(({"hg", "pull", "-u", version_info->source_control_url}));
          cd(oldcwd);
        }
        else
        {
          res = Process.run(({"hg", "clone", version_info->source_control_url, lpath}));
        }
        if(res->exitcode)
        {
          werror(res->stderr);
          throw(Error.Generic(bin + " returned non-zero exit code (" + res->exitcode + ").\n"));
        }
        break;
      case "git":
        if(have_path)
        {
          cd(lpath);
          res = Process.run(({"git", "pull", version_info->source_control_type, lpath}));
          cd(oldcwd);
        }
        else
        {
          res = Process.run(({"git", "clone", version_info->source_control_url, lpath}));
        }
        if(res->exitcode)
        {
          werror(res->stderr);
          throw(Error.Generic(bin + " returned non-zero exit code (" + res->exitcode + ").\n"));        
        }
        break;
      default:
        throw(Error.Generic("Invalid source control type " + bin + ".\n"));
    }

    write("cloned %s repository to %s.", bin, lpath);
    return lpath;
  }
  else if(from_source == SOURCE_CONTROL && !version_info->source_control_url)
  {
    throw(Error.Generic("cannot download from source control, no source control location specified.\n"));
  }
  else if(from_source == PURE_PIKE_PMAR && version_info->has_pmar)
  {
    write("beginning download of pure-pike PMAR for version %s...\n", version_info->version);
    if(arrayp(version_info->pmar_download))
      foreach(version_info->pmar_download;; string u)
      {
        rq = Protocols.HTTP.get_url_nice(u);
        if(rq) break;  
      }
    else
      rq = Protocols.HTTP.get_url_nice(version_info->pmar_download);
    if(!rq) 
      throw(Error.Generic("download error: unable to access download url\n"));
    else
    {
      string lpath = version_info->filename;
      if(path) lpath = combine_path(path, lpath);
      Stdio.write_file(lpath, rq[1]);
      
      write("wrote pmar to file %s (%d bytes)\n", lpath, sizeof(rq[1]));
      return lpath;
    }
  }
  else if(version_info->download)
  {
    write("beginning download of version %s...\n", version_info->version);
    if(arrayp(version_info->download))
      foreach(version_info->download;; string u)
      {
        rq = Protocols.HTTP.get_url_nice(u);
        if(rq) break;  
      }
    else
      rq = Protocols.HTTP.get_url_nice(version_info->download);
    if(!rq) 
      throw(Error.Generic("download error: unable to access download url\n"));
    else
    {
      string lpath = version_info->filename;
      if(path) lpath = combine_path(path, lpath);
      Stdio.write_file(lpath, rq[1]);
      
      write("wrote module to file %s (%d bytes)\n", lpath, sizeof(rq[1]));
      return lpath;
    }
  }
  else 
    return 0;
}

void do_download(string name, string|void version)
{
    mapping vi = get_module_action_data(name, version);
    int t = SOURCE_PACKAGE;
    if(use_source) t = SOURCE_CONTROL;
    else if(use_pmar) t = PURE_PIKE_PMAR;
    if(vi)
      get_file(vi, 0, t);
    else
      exit(1, "download error: no suitable download available.\n");
}

void do_pmar_install(string name, string|void version)
{
  mapping vi = get_module_action_data(name, version);
  if(!vi)
    exit(1, "install error: no suitable download available.\n");
  if(!vi->has_pmar || !vi->pmar_download)
    exit(1, "install error: no PMAR available for this version.\n");

  array args = ({"-x", "pmar_install", vi->pmar_download});
  if(use_local)
    args += ({"--local"});

  // TODO: if the repository returns the MD5 hash for the PMAR, we should pass that
  // along to the installer for verification.
  
  object installer = Process.spawn_pike(args, ([]));
  int res = installer->wait();

#if __NT__
// because NT is stupid, we have to do all kinds of "exit the process" tactics
// in order to free in-use files. therefore, the call to pmar_install will return
// before it's actually finished doing its thing.
// 
// it would probably be better if pmar_install signalled us in some way, but
// that's a task for another day.
  write("PMAR install will run in this console for a few more seconds.\n"
        "Please check the output in your console to make sure the installation was a success.\n");
#else
  if(res)
  {
    werror("install error: PMAR install failed.\n");
  }
  else
  {
    write("install successful.\n");
  }

#endif
}

void do_install(string name, string|void version)
{
  int res;
  string fn;
  mapping vi = get_module_action_data(name, version);

  original_dir = getcwd();

  if(vi)
    fn = get_file(vi, builddir, use_source);
  else 
    exit(1, "install error: no suitable download available.\n");

  cd(builddir);

  // now we should uncompress the file.

  if(!use_source && file_stat(fn)->isreg)
  {
    if((fn)[sizeof(fn)-3..] == ".gz")
    {
      write("uncompressing...%s\n", fn);
      if(!Process.search_path("gzip"))
        exit(1, "install error: no gzip found in PATH.\n");
      else
        res = Process.system("gzip -f -d " + fn);

      if(res)
        exit(1, "install error: uncompress failed.\n");
      fn = (fn)[0.. sizeof(fn)-4];
 	}
    else fn = vi->filename;

    created->file += ({ fn });

    werror("working with tar file " + fn + "\n");

	string workingdir = fn[0..sizeof(fn)-5];
	
    if(file_stat(workingdir))
    {
       if(use_force)
       {
         write("Deleting existing build directory %s in 5 seconds.\n", workingdir);
         sleep(5);
         Stdio.recursive_rm(workingdir);
       }
       else
       {
         exit(1, sprintf("Build directory %s exists and is likely a previous failed build. Use --force to delete.\n", workingdir));
       }
    }

    if(!Process.search_path("tar"))
      exit(1, "install error: no tar found in PATH.\n");
    else
      res = Process.system("tar xvf " + fn);
    
    if(res)
      exit(1, "install error: untar failed.\n");
    else
      created->dirs += ({workingdir});  


    // change directory to the module
    cd(combine_path(builddir, workingdir));
  }
  else
  {
    write("working with source control copy...\n");
    created->dirs += ({fn});
    cd(fn);
  }
  // now, build the module.

  array jobs = ({"", "verify", (use_local?"local_":"") + "install"});

  object builder;

  foreach(jobs, string j)
  {
    if(j == "install")
    {
      uninstall(name, 0);
    }
    else if(j=="local_install")
    {
      uninstall(name, 1);
    }

    write("\nRunning %O in %O\n\n", run_pike+" "+pike_args*" "+" -x module "+j,
	  getcwd());
    builder = Process.create_process(
      ({run_pike}) + pike_args + ({"-x", "module"}) + (({ j }) - ({""})));
    res = builder->wait();
  
    if(res)
    {
      werror("install error: make %s failed.\n\n", j);

      if(created->file)
        werror("the following files have been preserved in %s:\n\n%s\n\n", 
             builddir, created->file * "\n");

      if(created->dirs)
        werror("the following directories have been preserved in %s:\n\n%s\n\n", 
             builddir, created->dirs * "\n");

      exit(1);
    }
    else
    {
       write("make %s successful.\n", j);
    }
  }

  // now we should clean up our mess.
  foreach(created->file || ({}), string file)
  {
    write("cleaning up %s\n", file);
    rm(file);
  }

  foreach(created->dirs || ({}), string dir)
  {
    write("removing directory %s\n", dir);
    Stdio.recursive_rm(dir);
  }

  cd(original_dir);
}

void do_list(string|void name)
{
  array results;

  object x = xmlrpc_handler(repository);

  if(name && strlen(name))
  {
    results=x->get_modules_match(name);
  }
  else
    results=x->get_modules();

#if 0
  if(name && strlen(name))
    write("Found %d modules in the repository:\n\n", 
      sizeof(results[0]));
  else
    write("Found %d modules with a name containing %s:\n\n", 
      sizeof(results[0]), name);

#endif

  foreach(results, string r)
  {
    write("%s\n", r);
  }
}

array find_components(string name, int|void _local)
{
  string local_ver;
  array components;

  mixed e = catch
  {
    local_ver = master()->resolv(name)["__version"];

    if(local_ver) write("Version %s of this module is currently installed.\n",
                        local_ver);

    components = master()->resolv(name)["__components"];
  };

  if(e)
   werror(e[0]);
  return ({local_ver, components||({})});
}

int uninstall(string name, int|void _local)
{
  string local_ver;
  array components;

  [local_ver, components] = find_components(name, _local);

  if(!local_ver) return 0;
  if(!components)
  {
    werror("no components element found for this module. Unable to reliably uninstall.\n");
    return 0;
  }
  
  low_uninstall(components, _local);
  
  return 1;
}

void low_uninstall(array components, int _local)
{    
  string dir;

  if(_local)
  {
    dir = get_local_modulepath();
  }
  else
  {
    dir = master()->system_module_path[-1];
  }

  array elems = reverse(Array.sort_array(components));
  
  foreach(elems;; string comp)
  {
    string path = Stdio.append_path(dir, comp);
    object s = file_stat(path);
    if(!s)
    {
      werror("warning: %s does not exist.\n", path);
      continue;
    }
    
    werror("deleting: " + path + " [%s]\n", (s->isdir?"dir":"file"));
    rm(path);
  }
}

string get_local_modulepath()
{
  string dir = System.get_home();

  if(!dir)
  {
    throw(Error.Generic("Unable to determine home directory. "
          "Please set HOME environment variable and retry.\n"));
  }
  dir = combine_path(dir, "lib/pike/modules");
 
  return dir;
}

// make a remote xmlrpc service act more like a pike object.
class xmlrpc_handler
{
  Protocols.XMLRPC.Client x;

  void create(string loc)
  {
    x = Protocols.XMLRPC.Client(loc);
  }

  protected class _caller (string n){

    mixed `()(mixed ... args)
    {
      array|Protocols.XMLRPC.Fault r;
      if(args)
	      r = x[n](@args);
      else
	      r = x[n]();
      if(objectp(r)) // we have an error, throw it.
        error(r->fault_string);
      else return r[0];
    }
  }

  function `->(string n, mixed ... args)
  {
    return _caller(n);
  }
}
