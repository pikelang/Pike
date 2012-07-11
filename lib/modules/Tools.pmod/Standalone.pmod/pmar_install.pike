
#pike __REAL_VERSION__

int DEBUG=1;

constant version =
 sprintf("%d.%d.%d",(int)__REAL_VERSION__,__REAL_MINOR__,__REAL_BUILD__);
constant description = "Pike packaged module (PMAR) installer.";

int forcing;
int ilocal;

string md5; // the optional md5 checksum to compare the package against.
string s; // the pmar file contents
string fsroot; // the first directory in the pmar archive.
object uri; // if a uri was passed as the pmar source, this is it.
string tempfile; // if loaded from a remote source, we cache the file locally.
string pmar_name; // the base pmar filename.
int c;
int cc;
object file_object;
int uninstall;
int restart;

mapping sysinfo, metadata;
object moduletool;
string system_module_path;

//!
//! a prototype installer for prepackaged modules
//!
//! note: portions of this code are highly inefficient (wrt tar filehandling).
//! we assume that this will be used infrequently enough that this is not
//! going to be a problem.
//!
//! a package file is a tar file that contains the following structure:
//!
//! ROOTDIR/
//!         METADATA.TXT
//!                a file containing metadata about the package
//!                format: KEY=value, where values include: 
//!                  PLATFORM, in the form of os/processor (either can be "any") 
//!                  MODULE, the name of the module, in Module.Submodule format.
//!                  VERSION, the version of this module.
//!         MODULE/
//!                any files that need to be installed in the module directory
//!         MODREF/ ???
//!                documentation suitable for inclusion in the modref
//!         INCLUDE/ ???
//!                any pike language include files to be installed
//!         SCRIPTS/ 
//!                standalone (no bundled dependencies) scripts used to perform custom actions
//!                they receive the installer object (this) and the System.Filesystem object of the 
//!                package archive as arguments to the constructor. The method "run()" should
//!                perform the actual action. The run() method should return true or false
//!                to indicate success or failure.
//!                  preinstall.pike
//!                  postinstall.pike

void print_help(array argv)
{
    werror("Usage: %s [--local] [--force] [--md5=MD5HASH] [--help] pmarfile\n", argv[0]);
}

int main(int argc, array(string) argv)
{
  array components;
  
  array opts = Getopt.find_all_options(argv,aggregate(
    ({"local",Getopt.NO_ARG,({"--local"}) }),
    ({"uninstall",Getopt.NO_ARG,({"--uninstall"}) }),
    ({"tempfile",Getopt.HAS_ARG,({"--tempfile"}) }),
    ({"components",Getopt.HAS_ARG,({"--components"}) }),
    ({"force",Getopt.NO_ARG,({"--force"}) }),
    ({"restart",Getopt.NO_ARG,({"--restart"}) }),
    ({"md5",Getopt.HAS_ARG,({"--md5"}) }),
    ({"help",Getopt.NO_ARG,({"--help"}) }),
    ));
    
  argv=Getopt.get_args(argv);
  
  if(sizeof(argv) < 2) 
  {
    print_help(argv);
    return 1;
  }
  
  foreach(opts,array opt)
  {
    switch(opt[0])
    {
      case "help":
        print_help(argv);
        return 0;
        break;
      case "md5":
        md5 = lower_case(opt[1]);
        break;
      case "local":
        ilocal = 1;
        break;
      case "force":
        forcing = 1;
        break;
      case "components":
        components = decode_value(MIME.decode_base64(opt[1]));
        break;
      case "uninstall":
        uninstall = 1;
        break;
      case "tempfile":
        tempfile = opt[1];
      case "restart":
         restart = 1;
    }
  }

  catch(uri = Standards.URI(argv[1]));
  
  if(uninstall)
  {
    write("Uninstalling in 5 seconds...\n");
    sleep(5);
    do_uninstall(components, ilocal);
    write("Uninstall complete, restarting installation.\n");
    
    array monger_args = ({"-x", "pmar_install", "--restart"});
    
     if(ilocal)
       monger_args += ({"--local"});
     
     if(tempfile)
       monger_args += ({ "--tempfile=" + tempfile });  
     monger_args += ({ (string)uri });
   
     Process.spawn_pike(monger_args);
    return 0;
  }  
  
  if(!uri) // probably a file specification.
  {
    uri = Standards.URI("file://" + combine_path(getcwd(), argv[1])); 
    pmar_name = basename(uri->path);
    write("Loading PMAR from local file " + (string)uri + "...\n"); 

    s = Stdio.read_file(argv[1]);    
    if(!s) 
    {
      werror("PMAR does not exist or contains no data.\n");
      exit(1);
    }
 }
  else if((<"file">)[uri->scheme])
  {
    s = Stdio.read_file(argv[1]);    
    pmar_name = basename(uri->path);
    if(!s) 
    {
      werror("PMAR does not exist or contains no data.\n");
      exit(1);
    }
  }
  else if((<"http", "https">)[uri->scheme])
  {
    pmar_name = basename(uri->path);
    if(tempfile)
    {
      write("Reading from cached download.\n");
      s = Stdio.read_file(tempfile);
    }
    else
    {
      write("Fetching PMAR from " + (string)uri + "...\n"); 
      s = Protocols.HTTP.get_url_data(uri);
    }
    if(!s) 
    {
      werror("ERROR: PMAR does not exist or contains no data.\n");
      exit(1);
    }
    if(!tempfile)
      tempfile = savetemp_pmar(s, pmar_name);
  }
  else
  {
    werror("ERROR: Unsupported URI type " + uri->scheme + ".\n");
    exit(1);
  }
  
  file_object = Stdio.FakeFile(s);

  // has this pmar been gzipped?  
  if(s[0..2] == sprintf("%c%c%c", 0x1F, 0x8B, 0x08 ))
  {
#if constant(Gz.File)
    write("Reading from gzipped archive.\n");
    file_object = Gz.File(file_object);
#else
    werror("ERROR: Gz support is not available in this Pike. Please gunzip pmar before installing.\n");
#endif /* Gz.File */
  }

  string checksum = md5hash(file_object->read());
  write("MD5 Checksum of package: %s\n", checksum);

  if(md5 && String.hex2string(md5) != String.hex2string(checksum))
  {
    werror("ERROR: MD5 mismatch; package is not genuine.\n");
    werror("expected : %s\n", md5);
    werror("got      : %s\n", checksum);
    
    if(tempfile) rm(tempfile);
    exit(1);
  }
  else if(md5)
  {
    write("MD5 Checksum matches.\n");
  }

  file_object->seek(0);
  
  fsroot = Filesystem.Tar(pmar_name, 0, file_object)->get_dir()[0];

  // we assume that the first entry in the package file is the directory
  // that contains the module to be installed.

  sysinfo = get_sysinfo();
  metadata = get_package_metadata(file_object, fsroot);
  moduletool = Tools.Standalone.module();

  moduletool->load_specs(moduletool->include_path+"/specs");
 
  // we assume the local module install path is $HOME/lib/pike/modules.
  if(!ilocal)
    system_module_path = moduletool["system_module_path"];
  else 
  {
    system_module_path = getenv("HOME") + "/lib/pike/modules";
  }

  if(!file_stat(system_module_path))
  {
    werror("ERROR: module installation directory %s does not exist.\n", system_module_path);
    if(tempfile) rm(tempfile);
    return 2;
  }

//  system_include_path = moduletool["system_include_path"];
//  system_doc_path = moduletool["system_doc_path"];

  if(!forcing && !verify_suitable_package(metadata, sysinfo))
  {
    werror("ERROR: Package is not suitable for this system.\n");
    if(tempfile) rm(tempfile);
    return 1;
  }

  //
  // first, we should uninstall any modules included with this new package.
  //
  if(restart)
  {
    write("Restarting installation process.\n");
  }
  else
  {
     array(string) comp = ({});
     foreach(metadata->MODULE/",";;string mod)
     {
       mod = String.trim_all_whites(mod);
      write("Uninstalling any previous version of %s...\n", mod);

      // we spawn a new pike to do this because Windows prohibits modifying a file if it's already
      // in use, and the act of querying a module's existance will cause any .so files to be tied
      // up for the duration of the process lifetime.
    
      object d = Tools.Monger.MongerUser();
      string local_ver;
      array local_components;
     
      [local_ver, local_components] = d->find_components(mod, ilocal);
    
      comp += local_components;
#if 0    
      object d = Tools.Monger.MongerUser();
      d->uninstall(mod, ilocal);
#endif /* 0 */
    }
  
    if(sizeof(comp))
    {
      array monger_args = ({"-x", "pmar_install", "--uninstall"});
    
      if(ilocal)
        monger_args += ({"--local"});
     
      monger_args += ({"--components=" + MIME.encode_base64(encode_value(Array.uniq(comp)))});
      if(tempfile)
        monger_args += ({ "--tempfile=" + tempfile });  
      if(md5)
        monger_args += ({ "--md5=" + md5 });  
      monger_args += ({ (string)uri });
   
      Process.spawn_pike(monger_args);
      return 0;
    } 
  }
  
  if(!preinstall(this, getfs(file_object, "/")))
  {
    werror("Preinstall failed.\n");
    if(tempfile) rm(tempfile);
    return 1;
  }

  if(has_dir(file_object, fsroot + "/MODULE"))
    untar(file_object, system_module_path, fsroot + "/MODULE");
//  if(has_dir(s, fsroot + "/INCLUDE"))
//    untar(s, system_include_path, fsroot + "/INCLUDE");
//  if(has_dir(s, fsroot + "/MODREF"))
//    untar(s, system_doc_path, fsroot + "/MODREF");

  if(!postinstall(this, getfs(file_object, "/")))
  {
    werror("Postinstall failed.\n");
    if(tempfile) rm(tempfile);
    return 1;
  }

  write("Installation complete.\n");

  if(tempfile) rm(tempfile);
  return 0;
}

void do_uninstall(array components, int ilocal)
{
  object d = Tools.Monger.MongerUser();
  d->low_uninstall(components, ilocal);
}

int runscript(string sn, object installer, object pmar)
{
  object stat;

  // it's okay if we don't have any scripts.
  if(!has_dir(file_object, fsroot+"/SCRIPTS"))
    return 1;

  pmar = pmar->cd(fsroot+ "/SCRIPTS");

  if(!(stat = pmar->stat(sn + ".pike")))
    return 1;

  else if(!stat->isreg)
  {
    werror(sn + " script is not a regular file!\n");
    return 0;
  }

  string script = pmar->open(sn + ".pike", "r")->read();

  program p = compile_string(script, sn + ".pike");

  object r = p(installer, pmar->cd(fsroot));

  return r->run();
}

int preinstall(object installer, object pmar)
{
  return runscript("preinstall", installer, pmar);
}

int postinstall(object installer, object pmar)
{
  return runscript("postinstall", installer, pmar);
}

int has_dir(object s, string fsroot)
{
  object t;
  object stat;

  t=getfs(s, "/");
  string comp;

  mixed err = catch {

  foreach(fsroot / "/";; comp)
    t = t->cd(comp);
    if(!t) return 0;
  };

  if(err) return 0;
  
  t = t->cd("..");
  stat = t->stat(comp);

  if(!stat || !stat->isdir)
    return 0;

  return 1;
}

// TODO: we should also verify minimum/maximum valid pike versions, too.
int verify_suitable_package(mapping metadata, mapping sysinfo)
{
  int osok, procok;
  string pos, pproc;

  [pos,pproc] = metadata->PLATFORM /"/";
  
  if(pos == "any") osok = 1;
  if(pproc == "any") procok = 1;

  if(lower_case(sysinfo->sysname) == pos) osok = 1;
  if(lower_case(sysinfo->architecture) == pproc) procok = 1;

  if(osok && procok) return 1;
  else return 0;
}

mapping get_sysinfo()
{
  return System.uname();
}

mapping get_package_metadata(object s, string fsroot)
{
  mapping metadata = ([]);

  object stat = getfs(s, fsroot)->stat("METADATA.TXT");

  if(!stat || !stat->isreg)
    throw(Error.Generic("Missing METADATA.TXT in package!\n"));

  string f = getfs(s, fsroot)->open("METADATA.TXT", "r")->read();

  foreach((f/"\n") - ({""});; string line)
  {
    string k,v;
    array l;
    l=line/"=";
    k = String.trim_whites(l[0]);
    v = String.trim_whites(l[1]);
    metadata[k] = v;
  }

  return metadata;
}

protected Filesystem.System getfs(object source, string cwd) {
  source->seek(0);
  return Filesystem.Tar(sprintf("%s.tar", "d"), 0, source)->cd(cwd);
}

int untar(object source, string path, void|string cwd) {
  if (!cwd)
    cwd = "/";
  object t = getfs(source, cwd);
  array files = t->get_dir();
  int c;
  foreach(sort(files), string fname) {
    // Get the actual filename
    fname = ((fname / "/") - ({""}))[-1];
    object stat = t->cd(cwd)->stat(fname);
    if (stat->isdir()) {
      string dir = Stdio.append_path(path, fname);
      c++;
      cc++;
      if (DEBUG)
        write(sprintf("creating: %s [dir]\n", dir));
      mkdir(dir);
      c += untar(source, dir, Stdio.append_path(cwd, fname));
    }
    else if (stat->isreg()) {
      string file = Stdio.append_path(path, fname);
      if (mixed err = catch{
        if (DEBUG)
          write("writing:  %s [file %d bytes]\n", file, stat->size);
          Stdio.write_file(file, t->cd(cwd)->open(fname, "r")->read());
      }) {
        werror("failed:   %s [error writing file]\n\n", file);
//        throw(err);
      }
      c++;
      cc++;
    }
    else {
      werror("Unknown file type for file %O\n", fname);
      continue;
    }
  }
  return c;
}

string savetemp_pmar(string data, string filename)
{
  string fn;
  string dir;
  string path;
  
  mapping e = getenv();
  
  array x = ({});
  x+=({e["TMP"]});
  x+=({e["TMPDIR"]});
  x+=({e["TEMP"]});
  x+=({"C:\\TEMP"});
  x+=({"/tmp"});

  foreach(x;; dir)
  {
    object s;
    if((s = file_stat(dir)) && s->isdir)
    {
      break;
    }
    else dir = 0;
  }
  
  // fallback to the current directory.
  if(!dir) dir = getcwd();
  
  fn = md5hash((string)random(1000) + filename + " " + (string)time());
  path = combine_path(dir , fn);
  
  Stdio.write_file(path, data);
//  write("Temp file: " + path + "\n");
  return path;
}

string md5hash(string input)
{
  string h = Crypto.MD5()->hash(input);
  return String.string2hex(h);
}
