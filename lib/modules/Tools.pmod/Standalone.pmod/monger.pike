// -*- Pike -*-

// $Id: monger.pike,v 1.1 2004/04/20 21:38:46 bill Exp $

#pike __REAL_VERSION__

constant version = ("$Revision: 1.1 $"/" ")[1];
constant description = "Monger: the Pike module manger.";

string repository = "http://modules.gotpike.org:8000/xmlrpc/index.pike";

int use_force=0;
string my_command;
string argument;
string my_version;
string run_pike;

int main(int argc, array(string) argv)
{
  run_pike = master()->_pike_file_name;
#ifdef NOT_INSTALLED
  run_pike += " -DNOT_INSTALLED";
#endif
#ifdef PRECOMPILED_SEARCH_MORE
  run_pike += " -DPRECOMPILED_SEARCH_MORE";
#endif
  if(master()->_master_file_name)
    run_pike += " -m"+master()->_master_file_name;
  putenv("RUNPIKE", run_pike);

  array opts = Getopt.find_all_options(argv,aggregate(
    ({"list",Getopt.NO_ARG,({"--list"}) }),
    ({"download",Getopt.NO_ARG,({"--download"}) }),
    ({"query",Getopt.NO_ARG,({"--query"}) }),
    ({"repository",Getopt.HAS_ARG,({"--repository"}) }),
    ({"version",Getopt.HAS_ARG,({"--version"}) }),
    ({"force",Getopt.NO_ARG,({"--force"}) }),
    ({"help",Getopt.NO_ARG,({"--help"}) }),
    ));

  argv=Getopt.get_args(argv);

  if(sizeof(argv)>2) 
  {
    werror("Too many extra arguments!\n"); 
    exit(1);
  }
  else if(sizeof(argv)>1) argument = argv[1];

  foreach(opts,array opt)
  {
    switch(opt[0])
    {
      case "repository":
        repository = opt[1];
        break;
      case "version":
        my_version = opt[1];
        break;
      case "force":
        use_force = 1;
        break;
    }
  }

  foreach(opts,array opt)
  {
    switch(opt[0])
    {
      case "list":
        if(argument && strlen(argument))
          do_list(argument);
        else
          do_list();
        break;
      case "download":
        if(argument)
          do_download(argument, my_version||UNDEFINED);
        else
        {
          werror("download error: module name must be specified\n");
          exit(1);
        }
        break;
      case "query":
        if(argument)
          do_query(argument, my_version||UNDEFINED);
        else
        {
          werror("query error: module name must be specified\n");
          exit(1);
        }
        break;
      case "help":
        do_help();
        return 0;
        break;
    }
  }

 return 0;
}

void do_help()
{
  write(
#       "This is Monger, the manager for Fresh Pike.

Usage: pike -x monger [options] modulename

--list               list known modules in the repository, optionally
                       limited to those whose name contains the last 
                       argument in the argument list (modulename)
--query              retrieves information about module modulename
--repository=url     sets the repository source to url
--force              force the performance of an action that might 
                       otherwise generate a warning
--download           download the module modulename
--version=ver        work with the specified version of the module
");
}


void do_query(string name, string|void version)
{
  mixed e; // for catching errors
  int module_id;

  string pike_version = 
    sprintf("%d.%d.%d", (int)__REAL_MAJOR__, 
      (int)__REAL_MINOR__, (int)__REAL_BUILD__);

  object x = xmlrpc_handler(repository);

  module_id=x->get_module_id(name);

  mapping m = x->get_module_info((int)module_id);
  string v;

  write("%s: %s\n", m->name, m->description);

  string qv;

  catch(v = x->get_recommended_module_version((int)module_id, pike_version));

  if(my_version && use_force)
  {
    write("Forcing query of version %s.\n", my_version);
    qv=my_version;
  }
  else if(my_version && my_version!=v)
  {
    write("Requested version %s is not the recommended version.\n"
          "use --force to force a query of this version.\n", my_version);
    exit(1);
  }
  else if(my_version)
  {
    write("Selecting requested and recommended version %s for query.\n", v);
    qv=my_version;
  }
  else if(v)
  {
    qv=v;
  }
  else
  {
    werror("No version of this module is recommended for this Pike.\n");
    exit(1);
  }

  mapping vi = x->get_module_version_info((int)module_id, qv);

  write("Version: %s\t", vi->version);
  write("License: %s\n", vi->license);
  write("Changes: %s\n\n", vi->changes);
  
  if(vi->download)
    write("This module is available for automated installation.\n");
}

void do_download(string name, string|void version)
{
  mixed e; // for catching errors
  int module_id;

  string pike_version = 
    sprintf("%d.%d.%d", (int)__REAL_MAJOR__, 
      (int)__REAL_MINOR__, (int)__REAL_BUILD__);

  object x = xmlrpc_handler(repository);

  module_id=x->get_module_id(name);

  string v,dv;

  catch(v = x->get_recommended_module_version((int)module_id, pike_version));

  if(my_version && use_force)
  {
    write("Forcing download of version %s.\n", my_version);
    dv=my_version;
  }
  else if(my_version && my_version!=v)
  {
    write("Requested version %s is not the recommended version.\n"
          "use --force to force download of this version.\n", my_version);
    exit(1);
  }
  else if(my_version)
  {
    write("Selecting requested and recommended version %s for download.\n", v);
    dv=my_version;
  }
  else if(v)
  {
    write("Selecting recommended version %s for download.\n", v);
    dv=v;
  }
  else
  {
    werror("download error: no recommended version to download.\n"
      "use --force --version=ver to force download of a particular version.\n"); 
    exit(1);
  }

  mapping vi = x->get_module_version_info((int)module_id, dv);

  if(vi->download)
  {
    write("beginning download of version %s...\n", dv);
    array rq = Protocols.HTTP.get_url_nice(vi->download);
    if(!rq) 
    {
      werror("download error: unable to access download url\n");
      exit(1);
    }
    else
    {
      Stdio.write_file(vi->filename, rq[1]);
      write("wrote module to file %s (%d bytes)\n", vi->filename, sizeof(rq[1]));
    }
  }
  else 
  {
    werror("download error: no download available for this module version.\n");
    exit(1);
  }
}

void do_list(string|void name)
{
  mixed e; // for catching errors
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


class xmlrpc_handler
{
  object x;

  void create(string loc)
  {
    x = Protocols.XMLRPC.Client(loc);
  }
 

  class _caller (string n, object c){

    mixed `()(mixed ... args)
    {
      array|Protocols.XMLRPC.Fault r;
      if(args)
        r = c[n](@args);
      else
        r = c[n]();
      if(objectp(r)) // we have an error, so throw it.
        throw(({r->fault_string, backtrace()[2..]}));
      else return r[0];
    }

  }

  function `->(string n, mixed ... args)
  {
    return _caller(n, x);
  }

}
