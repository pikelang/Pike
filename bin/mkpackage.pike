/* Module for creating executable installation scripts based on Pike. */

#define ERR(msg) throw(({ "(Install) "+sprintf msg+"\n", backtrace() }))

int mkdirhier(string path, int|void mod)
{
  string a = "";
  int r = 1;

  if(Stdio.is_dir(path))
    return query_num_arg() == 1 ? 1 : (chmod(path, mod),1);
  
  foreach(path/"/", string d)
  {
    a += d + "/";
    if(sizeof(d))
      if(query_num_arg() == 1)
	r = mkdir(a);
      else
	r = mkdir(a, mod);
  }
  
  return r || (query_num_arg() == 2 && (chmod(path, mod),1));
}



void rmrf(string ... path)
{
  Process.create_process(({ "rm","-rf", @path }))->wait();
}

class Package
{
  static private string install_filename, pike_filename;
  static private mapping(array(string):string) options =
  ([ ({ "-h", "--help" }):
     "echo \"Usage: $TARFILE [options]\"\n"
     "echo \n"
     "echo 'Options:'\n"
     "echo '  -h, --help              Display this help and exit.'\n"
     "echo '  -l, --list              Display the contents of the package and exit.'\n"
     "echo '  -v, --version           Display version information and exit.'\n"
     "echo '  -a, --add <component>   Add a component to the package and exit.'\n"
     "echo\n"
     "echo 'When no arguments are given, the installation script will be started.'\n"
     "exit",
     ({ "-v", "--version" }):
     "echo 'Package version unknown.'\n"
     "exit",
     ({ "-a", "--add-component" }):
     "shift\n"
     "(cd `dirname \"$1\"` &&\n"
     " tar rf \"$TARFILE\" `basename \"$1\"`)\n"
     "exit",
     ({ "-l", "--list" }):
     "echo \"$CONTENTS\"\n"
     "exit" ]);
  static private array(string) packages = ({});
  
  static private string unique_name(int c)
  {
    return sprintf("%ctmP%07x", c, random(0xfffffff));
  }

  string basename(string path)
  {
    return (path/"/")[-1];
  }
  
  string make(string package_filename)
  {
    string setup_filename = unique_name('S')+".sh";
    string unpack_directory = unique_name('D');

    string setup = ("#!/bin/sh\n"
		    "TARFILE=\"$1\"; shift; ARGS=''\n"
		    "CONTENTS=`tar tf \"$TARFILE\" | sed -ne '/^"+
		    replace(basename(install_filename), ".", "\\.")+"/,$p'`\n"
		    // Convert $TARFILE to an absolute path
		    "case \"$TARFILE\" in\n"
		    "    /*)\n"
		    "    ;;\n"
		    "    *)\n"
		    "      TARFILE=\"`pwd`/$TARFILE\"\n"
		    "    ;;\n"
		    "esac\n"
		    // Check all arguments for possible options.
		    "while [ $# != 0 ]\n"
		    "do\n"
		    "  case \"$1\" in\n"
		    +Array.map(sort(indices(options)),
			       lambda(array(string) flags)
			       {
				 return flags*"|"+") "+options[flags]+" ;;\n";
			       })*""+
		    "  esac\n"
		    "  ARGS=\"$ARGS '`echo \\\"$1\\\" | sed -e \\\"s/'/'\\\\\\\"'\\\\\\\"'/g\\\"`'\"\n"
		    "  shift\n"
		    "done\n"
		    // Commence installation.
		    "mkdir "+unpack_directory+"\n"
		    "(cd "+unpack_directory+"\n"
		    " tar xf \"$TARFILE\" $CONTENTS\n"
		    " ./"+basename(pike_filename)+" "
	                     "--script \"`pwd`\"/"+
		                            basename(install_filename)+")\n"
		    "rm -rf "+setup_filename+" "+unpack_directory+"\n");
    
    string bootstrap = sprintf("#!/bin/sh\n"
			       "tar xf \"$0\" %s\n"
			       "exec ./%s \"$0\" \"$@\"\n",
			       setup_filename,
			       setup_filename,
			       setup_filename);
		    
    rmrf("#!", setup_filename);
    
    if(!Stdio.write_file(setup_filename, setup))
      ERR(("Failed to write setup script %O., ", setup_filename));
    chmod(setup_filename, 0755);
		   
    if(!mkdirhier(bootstrap, 0755))
      ERR(("Failed to create bootstrap %O., ", bootstrap));

    Process.create_process(({ "tar", "cf",
			      package_filename,
			      bootstrap,
			      setup_filename }))->wait();

    packages = ({ install_filename, pike_filename }) + packages;
    
    string original_wd = getcwd();
    foreach(packages, string package)
    {
      array(string) path_parts = package/"/";
      
      cd(path_parts[0..sizeof(path_parts)-2]*"/");
      Process.create_process(({ "tar", "rf",
				combine_path(original_wd, package_filename),
				path_parts[-1] }))->wait();
      cd(original_wd);
    }

    chmod(package_filename, 0755);

    rmrf("#!", setup_filename);
  }

  Package add_packages(string ... _packages)
  {
    packages += _packages;
    
    return this_object();
  }
  
  void create(string _pike_filename, string _install_filename)
  {
    pike_filename = _pike_filename;
    install_filename = _install_filename;
  }
}

int main(int argc, array(string) argv)
{
  if(argc < 4)
  {
    werror("Usage: make-package.pike <name> <pike binary> <install script> [packages ...]\n");
    return 1;
  }
  
  Package(argv[2], argv[3])->add_packages(@argv[4..])->make(argv[1]);
  
  return 0;
}
