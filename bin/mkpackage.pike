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

string sh_quote(string s)
{
  return "'"+replace(s, "'", "'\"'\"'")+"'";
}

class Package
{
  static string my_name;
  static string pike_filename; 
  static string install_filename; 
  static string extra_help; 
  static string extra_advanced_help; 
  static string extra_flags;
  static string extra_license;
  static string extra_version;

  static private mapping(array(string):string) options;
  
  static void create(string _my_name,
		     string _pike_filename,
		     string _install_filename,
		     string _extra_help,
		     string _extra_advanced_help,
		     string _extra_flags,
		     string _extra_license,
		     string _extra_version)
  {
    my_name             = _my_name;
    pike_filename       = _pike_filename;
    install_filename    = _install_filename;
    extra_help          = _extra_help;
    extra_advanced_help = _extra_advanced_help;
    extra_flags         = _extra_flags;
    extra_license       = _extra_license;
    extra_version       = _extra_version;

    options =
  ([ ({ "-h", "--help" }):
     "echo \"Usage: $MY_NAME [options]\"\n"
     "echo \n"
     "echo 'Options:'\n"
     "echo '  -h, --help              Display this help and exit.'\n"
     "echo '  -l, --list              Display included packages and exit.'\n"
     "echo '  -v, --version           Display version information and exit.'\n"
     "echo '  --features              Display feature information and exit.'\n"
     "echo '  --advanced-help         Display information about advanced features.'\n"
     "echo \"$EXTRA_HELP\"\n"
     "EXIT=yes",

     ({ "--advanced-help" }):
     "echo \"Usage: $MY_NAME [options]\"\n"
     "echo \n"
     "echo 'Options:'\n"
     "echo '  -h, --help              Display this help and exit.'\n"
     "echo '  -l, --list              Display included packages and exit.'\n"
     "echo '  -v, --version           Display version information and exit.'\n"
     "echo '  --features              Display feature information and exit.'\n"
     "echo '  --advanced-help         Display information about advanced features.'\n"
     "echo \"$EXTRA_HELP\"\n"
     "echo '  -a, --add=<package>     Add <package> and exit. <package> must be a .tar.gz'\n"
     "echo '                          file. Some tars have problem with this operation,'\n"
     "echo '                          but system tar may work (add /usr/bin to the'\n"
     "echo '                          beginning of your PATH).'\n"
     "echo \"$EXTRA_ADVANCED_HELP\"\n"
     "EXIT=yes",

     ({ "-v", "--version" }):
     "echo '"+extra_version+"'\n"
     "EXIT=yes",
     
     // We actually have two versions here, with and without =.
     ({ "-a=*", "--add=*" }):
     "PKG=`echo \"$1\" | sed -e 's/^.*=//'`\n"
     "(cd `dirname \"$PKG\"` && tar rf \"$TARFILE\" `basename \"$PKG\"`)\n"
     "EXIT=yes",
     ({ "-a", "--add" }):
     "if [ $# = 1 ]\n"
     "then\n"
     "   echo \"$MY_NAME: Missing argument to '$1'.\"\n"
     "   echo \"Try '$MY_NAME --help' for more information.\"\n"
     "else\n"
     "  shift\n"
     "  (cd `dirname \"$1\"` && tar rf \"$TARFILE\" `basename \"$1\"`)\n"
     "fi\n"
     "EXIT=yes",

     ({ "-l", "--list" }):
     "echo \"$CONTENTS\"\n"
     "EXIT=yes" ]);
  }
  
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
    
    string features = Process.popen(pike_filename+" --features")||"-unknown-";
    if(features[-1] == '\n')
      // Remove final newline since echo will provide it.
      features = features[..sizeof(features)-2];
    options[({ "--features" })] =
      (sizeof(features) ? "echo \""+features+"\"\n" : "") + "EXIT=yes";

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
		    "MY_NAME='"+my_name+"'\n"
		    "EXTRA_LICENSE="+sh_quote(extra_license)+"\n"
		    "export EXTRA_LICENSE\n"
		    // Check all arguments for possible options.
		    "while [ $# != 0 ]\n"
		    "do\n"
		    "  EXIT=no\n"
		    // FIXME: Apply proper quotes.
		    "  EXTRA_HELP='"+extra_help+"'\n"
		    "  EXTRA_ADVANCED_HELP='"+extra_advanced_help+"'\n"
		    "  case \"$1\" in\n"
		    +Array.map(sort(indices(options)),
			       lambda(array(string) flags)
			       {
				 return flags*"|"+") "+options[flags]+" ;;\n";
			       })*""+
		    "  "+extra_flags+"\n"
		    "  esac\n"
		    "  if [ \"x$HAS_ARG\" != x ]\n"
		    "  then\n"
		    "    if [ $# = 1 ]\n"
		    "    then\n"
		    "      echo \"$MY_NAME: Missing argument to '$1'.\"\n"
		    "      echo \"Try '$MY_NAME --help' for more information.\"\n"
		    "      EXITCODE=1\n"
		    "      EXIT=yes \n"
		    "    else\n"
		    "      ARGS=\"$ARGS '`echo \\\"$1\\\" | sed -e \\\"s/'/'\\\\\\\"'\\\\\\\"'/g\\\"`'\"\n"
		    "      shift\n"
		    "    fi\n"
		    "    HAS_ARG=''\n"
		    "  fi\n"
		    "  if [ $EXIT = yes ]\n"
		    "  then\n"
		    "    rm -f "+setup_filename+"\n"
		    "    exit $EXITCODE\n"
		    "  fi\n"
		    "  ARGS=\"$ARGS '`echo \\\"$1\\\" | sed -e \\\"s/'/'\\\\\\\"'\\\\\\\"'/g\\\"`'\"\n"
		    "  shift\n"
		    "done\n"
		    // Commence installation.
		    "mkdir "+unpack_directory+"\n"
		    "(cd "+unpack_directory+"\n"
		    " tar xf \"$TARFILE\" $CONTENTS\n"
		    " eval \"./"+basename(pike_filename)+" "
	                     "--script \\\"\\`pwd\\`/"+
		                            basename(install_filename)+"\\\""
		    " -- $ARGS\")\n"
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

    // Filter to root/root ownership.
    ((program)combine_path(__FILE__, "..", "tarfilter"))()->
      main(3, ({ "tarfilter", package_filename, package_filename }));
    
    chmod(package_filename, 0755);

    rmrf("#!", setup_filename);
  }

  Package add_packages(string ... _packages)
  {
    packages += _packages;
    
    return this_object();
  }
}

int main(int argc, array(string) argv)
{
  if(argc < 4)
  {
    write("Usage: make-package.pike <name> <pike binary> <install script> [packages ...]\n\n"
	  "Environment variables:\n"
	  "  EXTRA_PACKAGE_HELP\n"
	  "  EXTRA_PACKAGE_ADVANCED_HELP\n"
	  "  EXTRA_PACKAGE_FLAGS\n"
	  "  EXTRA_PACKAGE_LICENSE\n"
	  "  EXTRA_PACKAGE_VERSION\n");
    return 1;
  }
  
  Package(argv[1], argv[2], argv[3],
	  getenv("EXTRA_PACKAGE_HELP") || "",
	  getenv("EXTRA_PACKAGE_ADVANCED_HELP") || "",
	  getenv("EXTRA_PACKAGE_FLAGS") || "",
	  getenv("EXTRA_PACKAGE_LICENSE") || "",
	  getenv("EXTRA_PACKAGE_VERSION") || "Package version unknown.")->
    add_packages(@argv[4..])->make(argv[1]);
  
  return 0;
}
