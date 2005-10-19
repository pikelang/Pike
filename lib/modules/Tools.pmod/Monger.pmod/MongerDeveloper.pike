// -*- Pike -*-

// $Id: MongerDeveloper.pike,v 1.1 2005/10/19 03:26:03 bill Exp $

#pike __REAL_VERSION__

constant version = ("$Revision: 1.1 $"/" ")[1];
constant description = "MongerDeveloper: the Pike module manger.";

private string default_repository = "http://modules.gotpike.org:8000/xmlrpc/index.pike";
private string default_builddir = getenv("HOME") + "/.monger";
private string repository;
private string builddir;

private int use_force=0;
private int use_local=0;
private string my_command;
private string argument;
private string my_version;
private string run_pike;
private array(string) pike_args = ({});
private string original_dir;
private string username, password;
private mapping created = ([]);

void create()
{
  repository = default_repository;
  builddir = default_builddir;
}

//! set the username and password for accessing the remote repository.
void set_auth(string _username, string _password)
{
  username = _username;
  password = _password;
}

//! specify an alternate repository location.
//!
//! this should be a URL specifying the XMLRPC endpoint for the repository.
void set_repository(string _repository)
{
  repository = _repository;
} 

//! sets the default repository location (modules.gotpike.org)
void set_default_repository()
{
  repository = default_repository;
} 

//! sets the monger directory to use for working and storing 
//! configurations.
void set_directory(string _directory)
{
  builddir = _directory;
} 

//! sets the default directory for working and storing configurations 
//! ($HOME/.monger)
void set_default_directory()
{
  builddir = default_builddir;
} 

//!
int add_new_version(string module_name, string version, 
                    string changes, string license)
{
  mixed e; // for catching errors
  int module_id;
  mapping data = ([]);

  object x = xmlrpc_handler(repository);

  module_id=(int)(x->get_module_id(module_name));

  data->changes = changes;
  data->license = license;

  return x->new_module_version(module_id, version, data, ({username, password}));
  
}

//!
int update_version(string module_name, string version, 
                    string|void changes, string|void license)
{
  mixed e; // for catching errors
  int module_id;
  mapping data = ([]);

  object x = xmlrpc_handler(repository);

  module_id=(int)(x->get_module_id(module_name));

  if(changes)
    data->changes = changes;
  if(license)
    data->license = license;

  return x->update_module_version(module_id, version, data, ({username, password}));
  
}

//!
int user_change_password(string|void _username, string _newpassword)
{
  mixed e; // for catching errors
  int module_id;

  object x = xmlrpc_handler(repository);
 
  if(!_username) _username = username;

  return x->user_change_password(_username, _newpassword, ({username, password}));  
}

//!
int user_change_email(string|void _username, string _newemail)
{
  mixed e; // for catching errors
  int module_id;

  object x = xmlrpc_handler(repository);
 
  if(!_username) _username = username;

  return x->user_change_email(_username, _newemail, ({username, password}));  
}

//!
int set_version_active(string module_name, string version, 
                    int active)
{
  mixed e; // for catching errors
  int module_id;
  mapping data = ([]);

  object x = xmlrpc_handler(repository);

  module_id=(int)(x->get_module_id(module_name));

  return x->set_module_version_active(module_id, version, active, 
    ({username, password}));
  
}

//!
int set_module_source(string module_name, string version, 
                    string filename)
{
  mixed e; // for catching errors
  int module_id;
  mapping data = ([]);

  object x = xmlrpc_handler(repository);

  module_id=(int)(x->get_module_id(module_name));

  string contents = Stdio.read_file(filename);

  if(!contents || !strlen(contents)) error("unable to read file %s.\n", filename);

  filename = explode_path(filename)[-1];

  return x->set_module_source(module_id, version, filename, 
    MIME.encode_base64(contents), 
    ({username, password}));
  
}

//!
int set_dependency(string module_name, string version, 
                    string dependency, string min_version, string max_version,
                    int(0..1) required)
{
  mixed e; // for catching errors
  int module_id;
  mapping data = ([]);

  object x = xmlrpc_handler(repository);

  module_id=(int)(x->get_module_id(module_name));

  return x->set_dependency(module_id, version, dependency, min_version,
    max_version, required, 
    ({username, password}));
  
}

//!
int delete_dependency(string module_name, string version, 
                    string dependency, string min_version, string max_version)
{
  mixed e; // for catching errors
  int module_id;
  mapping data = ([]);

  object x = xmlrpc_handler(repository);

  module_id=(int)(x->get_module_id(module_name));

  return x->delete_dependency(module_id, version, dependency, min_version,
    max_version, 
    ({username, password}));
  
}

//!
array get_dependencies(string module_name, string version)
{
  mixed e; // for catching errors
  int module_id;
  mapping data = ([]);

  object x = xmlrpc_handler(repository);

  module_id=(int)(x->get_module_id(module_name));

  array a = x->get_module_dependencies(module_id, version);
  foreach(a, mapping r)
    if(r->dependency == "0") r->dependency = "Pike";
    else 
      r->dependency = x->get_module_info((int)(r->dependency))->name;
  return a;
}

private mapping get_module_action_data(string name, string|void version)
{
  int module_id;
  string dv;

  string pike_version = 
    sprintf("%d.%d.%d", (int)__REAL_MAJOR__, 
      (int)__REAL_MINOR__, (int)__REAL_BUILD__);

  object x = xmlrpc_handler(repository);

  module_id=x->get_module_id(name);

  string v;

  mapping info = x->get_module_info((int)module_id);

  mixed err=catch(v = x->get_recommended_module_version((int)module_id, pike_version));

  if(err)
    write("an error occurred while getting the recommended version.\n");

  info->version_type="recommended";

  if(version && version != v)
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
    exit(1);
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
    exit(1, "repository error: no recommended version to %s.\n"
	 "use --force --version=ver to force %s of a particular version.\n",
         my_command, my_command);

  mapping vi = x->get_module_version_info((int)module_id, dv);

  return vi + info;
}

private void do_download(string name, string|void version)
{
  mixed e; // for catching errors
  int module_id;

  mapping vi = get_module_action_data(name, version);

  if(vi->download)
  {
    write("beginning download of version %s...\n", vi->version);
    array rq = Protocols.HTTP.get_url_nice(vi->download);
    if(!rq) 
      exit(1, "download error: unable to access download url\n");
    else
    {
      Stdio.write_file(vi->filename, rq[1]);
      write("wrote module to file %s (%d bytes)\n", vi->filename, sizeof(rq[1]));
    }
  }
  else 
    exit(1, "download error: no download available for this module version.\n");
}

private void do_install(string name, string|void version)
{
  mixed e; // for catching errors
  int module_id;
  int res;

  mapping vi = get_module_action_data(name, version);

  if(vi->download)
  {
    original_dir = getcwd();
    cd(builddir);

    write("beginning download of version %s...\n", vi->version);
    array rq = Protocols.HTTP.get_url_nice(vi->download);
    if(!rq) 
      exit(1, "download error: unable to access download url\n");
    else
    {
      Stdio.write_file(vi->filename, rq[1]);
      write("wrote module to file %s (%d bytes)\n", vi->filename, sizeof(rq[1]));
    }
  }
  else 
    exit(1, "install error: no download available for this module version.\n");

  // now we should uncompress the file.
  string fn;

  if((vi->filename)[sizeof(vi->filename)-3..] == ".gz")
  {
    fn = (vi->filename)[0.. sizeof(vi->filename)-4];
    write("uncompressing...%s\n", vi->filename);
    if(!Process.search_path("gzip"))
      exit(1, "install error: no gzip found in PATH.\n");
    else
      res = Process.system("gzip -f -d " + vi->filename);

    if(res)
      exit(1, "install error: uncompress failed.\n");

  }
  else fn = vi->filename;

  created->file += ({ fn });

  werror("working with tar file " + fn + "\n");

  if(!Process.search_path("tar"))
    exit(1, "install error: no tar found in PATH.\n");
  else
    res = Process.system("tar xvf " + fn);

  if(res)
    exit(1, "install error: untar failed.\n");
  else
    created->dirs += ({fn[0..sizeof(fn)-5]});  


  // change directory to the module
  cd(combine_path(builddir, fn[0..sizeof(fn)-5]));

  // now, build the module.

  array jobs = ({"all", "verify", (use_local?"local_":"") + "install"});

  object builder;

  foreach(jobs, string j)
  {
    write("\nRunning %O in %O\n\n", run_pike+" "+pike_args*" "+" -x module "+j,
	  getcwd());
    builder = Process.create_process(
      ({run_pike}) + pike_args + ({"-x", "module"}) + ({ j }));
    res = builder->wait();
  
    if(res)
    {
      werror("install error: make %s failed.\n\n", j);

      werror("the following files have been preserved in %s:\n\n%s\n\n", 
             builddir, created->file * "\n");

      werror("the following directories have been preserved in %s:\n\n%s\n\n", 
             builddir, created->dirs * "\n");

      exit(1);
    }
    else
    {
       werror("make %s successful.\n", j);
    }
  }

  // now we should clean up our mess.
  foreach(created->file, string file)
  {
    write("cleaning up %s\n", file);
    rm(file);
  }

  foreach(created->dirs, string dir)
  {
    write("removing directory %s\n", dir);
    Stdio.recursive_rm(dir);
  }

  cd(original_dir);
}

private void do_list(string|void name)
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


  foreach(results, string r)
  {
    write("%s\n", r);
  }
}


class xmlrpc_handler
{
  Protocols.XMLRPC.Client x;

  void create(string loc)
  {
    x = Protocols.XMLRPC.Client(loc);
  }
 

  static class _caller (string n){

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
