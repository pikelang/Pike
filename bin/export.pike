#! /usr/bin/env pike

/* $Id: export.pike,v 1.64 2004/04/02 20:23:14 grubba Exp $ */

multiset except_modules = (<>);
string vpath;

string dirname(string dir)
{
  array tmp=dir/"/";
  if(tmp[-1]=="") tmp=tmp[..sizeof(tmp)-2];
  tmp=tmp[..sizeof(tmp)-2];
  if(sizeof(tmp)<2) return "/";
  return tmp*"/";
}

array(string) get_files(string path)
{
  array(string) files = get_dir(path);

  if(!getenv("PIKE_EXPORT_CVS_DIRS"))
    files -= ({ "CVS", "RCS", ".cvsignore" });

  array(string) ret = ({});
  foreach(files, string fn)
  {
    if( fn=="core" ) continue;
    if( fn[-1]=='~' ) continue;
    if( fn[0]=='#' && fn[-1]=='#' ) continue;
    if( fn[0]=='.' && fn[1]=='#' ) continue;

    if( path==vpath+"/src/modules" && except_modules[fn] )
      continue;

    if( path==vpath+"/bin" && except_modules[fn] )
      continue;

    if( has_prefix(path, vpath+"/lib/modules") &&
	(except_modules[fn] || except_modules[fn - ".pmod"]))
      continue;

    fn = path+"/"+fn;

    if( Stdio.file_size(fn)==-2 )
      ret += get_files(fn);
    else
      ret += ({ fn });
  }
  return ret;
}

void fix_configure(string dir)
{
  Stdio.Stat config=file_stat(dir+"/configure");
  Stdio.Stat config_in=file_stat(dir+"/configure.in");

  if(config_in)
  {
    if(!config || config_in->mtime > config->mtime)
    {
      werror("Fixing configure in "+dir+".\n");
      Process.create_process( ({"autoconf"}),
			      (["cwd":dir]) )->wait();
    }
  }
}

array(int) getversion()
{
  string s = Stdio.read_file(pike_base_name+"/src/version.h");

  if(!s)
  {
    werror("Failed to read version.h\n");
    werror("cwd=%s  version.h=%s\n", getcwd(), pike_base_name+"/src/version.h");
    exit(1);
  }

  int maj, min, build;

  if ((!sscanf(s, "%*sPIKE_MAJOR_VERSION %d", maj)) ||
      (!sscanf(s, "%*sPIKE_MINOR_VERSION %d", min)) ||
      (!sscanf(s, "%*sPIKE_BUILD_VERSION %d", build))) {

    werror("Failed to get Pike version.\n");
    exit(1);
  }

  return ({ maj, min, build });
}

void bump_version(int|void is_release)
{
  werror("Bumping release number.\n");
  Process.create_process( ({ "cvs", "update", "version.h" }),
			  ([ "cwd":pike_base_name+"/src" ]) )->wait();

  string s = Stdio.read_file(pike_base_name+"/src/version.h");
  sscanf(s, "%s PIKE_BUILD_VERSION %d%s", string pre, int rel, string post);
  rel++;
  Stdio.write_file( pike_base_name+"/src/version.h",
		    pre+" PIKE_BUILD_VERSION "+rel+post );
  Process.create_process( ({ "cvs", "commit", "-m",
			     "release number bumped to "+rel+" by export.pike",
			     "version.h" }),
			  ([ "cwd":pike_base_name+"/src" ]) )->wait();

  s = Stdio.read_file(pike_base_name+"/packaging/debian/changelog");
  if (s) {
    werror("Bumping Debian changelog.\n");
    array(int) version = getversion();
    s = sprintf("pike%d.%d (%d.%d.%d-1) experimental; urgency=low\n"
		"\n" +
		"  * %s\n"
		"\n"
		" -- Pike build system <pike-devel@lists.lysator.liu.se>  %s\n"
		"\n"
		"%s",
		version[0], version[1],
		version[0], version[1], version[2],
		is_release?
		"Release number bumped by export.pike.\n":
		"The latest cvs snapshot\n",
		Calendar.Second()->format_smtp(),
		s);
    Stdio.write_file(pike_base_name+"/packaging/debian/changelog", s);
    Process.create_process( ({ "cvs", "commit", "-m",
			       "release number bumped to "+rel+" by export.pike",
			       "changelog" }),
			     ([ "cwd":pike_base_name+"/packaging/debian" ])
			     )->wait();

  }
}

array(string) build_file_list(string vpath, string list_file)
{
  array(string) ret=({ }), missing=({ });
  if(!file_stat(list_file)) {
    werror("Could not find %s\n", list_file);
    exit(1);
  }
  foreach(Stdio.read_file(list_file) / "\n", string line)
    {
      if( !sizeof(line) || line[0]=='#' )
	continue;
      string name=vpath+line;
      Stdio.Stat fs;
      if((fs = file_stat(name)) && fs->isdir)
	ret += get_files(name);
      else if(fs && fs->isreg)
	ret += ({ name });
      else
	missing +=({ name });
    }

  if(!ignore_missing && sizeof(missing)){
    werror("The following files and/or directories were not found:\n\t"
	   + missing * "\t\n"
	   + "\n(you might want to add --force)\n");
    return 0;
  }
  return ret;
}

constant stamp=#"Pike export stamp
time:%t
major:%maj
minor:%min
build:%bld
year:%Y
month:%M
day:%D
hour:%h
minute:%m
second:%s
";

string pike_base_name;
string srcdir;
int(0..1) rebuild, ignore_missing;

int main(int argc, array(string) argv)
{
  array(string) files;
  string export_list, filename;
  object cvs;
  int tag, snapshot, t;

  foreach(Getopt.find_all_options(argv, ({
    ({ "srcdir",    Getopt.HAS_ARG, "--srcdir"     }),
    ({ "rebuild",   Getopt.NO_ARG,  "--rebuild"    }),
    ({ "tag",       Getopt.NO_ARG,  "--tag"        }),
    ({ "help",      Getopt.NO_ARG,  "--help"       }),
    ({ "exportlist",Getopt.HAS_ARG, "--exportlist" }),
    ({ "filename",  Getopt.HAS_ARG, "--name"       }),
    ({ "force",     Getopt.NO_ARG,  "--force"      }),
    ({ "timestamp", Getopt.HAS_ARG, "--timestamp"  }),
    ({ "snapshot",  Getopt.NO_ARG,  "--snapshot"   }),
  }) ),array opt)
    {
      switch(opt[0])
      {
	case "srcdir":
	  srcdir=opt[1];
	  if(basename(srcdir)=="src")
	    srcdir=dirname(srcdir);
	  pike_base_name=".";
	
	  cd(srcdir);
	  break;
	
        case "exportlist":
	  export_list=opt[1];
	  break;
	
        case "filename":
	  filename=opt[1];
	  break;

	case "rebuild":
	  rebuild=1;
	  break;
	
        case "force":
	  ignore_missing=1;
	  break;

        case "tag":
	  tag=1;
	  break;
	
        case "help":
	  write(documentation);
	  return 0;

        case "timestamp":
	  t=(int)opt[1];
	  break;

	case "snapshot":
	  snapshot=1;
	  break;
      }
    }


  argv -= ({ 0 });
  except_modules = (multiset)argv[1..];
  if(!srcdir || !export_list || !filename) {
    werror(documentation);
    return 1;
  }

  export_list=srcdir+"/"+export_list;

  if(rebuild)
  {
    werror("Not yet finished!\n");
    exit(1);
    object autoconfig=Process.create_process(({"./run_autoconfig"}),
					     (["cwd":pike_base_name]));
    /* make depend... */
    /* And other things... */
  }

  if(tag && file_stat(pike_base_name+"/CVS"))
  {
    bump_version(1);

    array(int) version = getversion();
    vpath = sprintf("Pike-v%d.%d.%d", @version);
    string tag = sprintf("v%d_%d_%d", @version);

    werror("Creating tag "+tag+" in the background.\n");
    cvs = Process.create_process( ({"cvs", "tag", "-R", "-F", tag}) );
  }

  t = t||time();
  mapping m = gmtime(t);
  array(int) version = getversion();
  mapping symbols=([
    "%maj":(string) version[0],
    "%min":(string) version[1],
    "%bld":(string) version[2],
    "%Y":sprintf("%04d",1900+m->year),
    "%M":sprintf("%02d",1+m->mon),
    "%D":sprintf("%02d",m->mday),
    "%h":sprintf("%02d",m->hour),
    "%m":sprintf("%02d",m->min),
    "%s":sprintf("%02d",m->sec),
    "%t":(string)t,
  ]);

  filename = replace(filename,symbols);

  if (snapshot) {
    vpath = sprintf("Pike-v%d.%d-snapshot", @version);
  } else {
    vpath = filename;
  }

  fix_configure(pike_base_name+"/src");

  foreach(get_dir(pike_base_name+"/src/modules") - ({"CVS","RCS"}), string fn)
    if(Stdio.file_size(pike_base_name+"/src/modules/"+fn) == -2)
      fix_configure("modules/"+fn);

  rm(vpath);
  symlink(".", vpath);

  files = build_file_list(vpath,export_list);
  if(!files) // Unable to build file list.
    return 1;

  Stdio.write_file("buildid.txt", replace(stamp, symbols));
  files += ({ vpath+"/buildid.txt" });

  werror("Creating "+filename+".tar.gz:\n");

  int first = 1;
  foreach(files/25.0, files)
    {
      if(Process.create_process
	 ( ({"tar",
	     first?"cvf":"rvf",
	     pike_base_name+"/"+filename+".tar" }) +
	   files)->wait())
      {
	werror("Tar file creation failed!\n");
	if(cvs) cvs->wait();
	rm(vpath);
	exit(1);
      }
      first = 0;
    }

  rm(vpath);
  string build = sprintf("%s-%s-%s", uname()->sysname, uname()->release,
			 uname()->machine);
  build = "build/"+replace(lower_case(build), ({ " ", "/", "(", ")" }),
			   ({ "-", "_", "_", "_" }));
  if(file_stat(build+"/autodoc.xml") && file_stat(build+"/doc_build/images")) {
    mkdir(vpath);
    mkdir(vpath+"/refdoc");
    Stdio.cp(build+"/autodoc.xml", vpath+"/refdoc/autodoc.xml");
    Process.create_process( ({ "cp", "-R", build+"/doc_build/images",
			       vpath+"/refdoc/images" }) )->wait();
    if(Process.create_process
       ( ({"tar", "rvf", pike_base_name+"/"+filename+".tar",
	   vpath+"/refdoc/autodoc.xml", vpath+"/refdoc/images" }) )->wait())
      {
	werror("Tar file creation failed!\n");
	if(cvs) cvs->wait();
	Stdio.recursive_rm(vpath);
	exit(1);
      }
    Stdio.recursive_rm(vpath);
  }

  if(Process.create_process
     ( ({"gzip",
	 "-9",
	 pike_base_name+"/"+filename+".tar"
     }) )->wait())
    {
      werror("Gzip failed!\n");
      if(cvs) cvs->wait();
      exit(1);
    }

  rm("buildid.txt");
  werror("Done.\n");

  if(cvs)
  {
    cvs->wait();
    bump_version();
  }

  return 0;
}

constant documentation = #"
Usage: export.pike <arguments> <except modules>

Creates a pike distribution. Needs one tar and one gzip binary in the path.
Mandatory arguments:

--name=<name>
	Name of export archive (%maj, %min, %bld, %Y, %M, %D, %h, %m, %s
	are replaced with apropiate values).
--exportlist=<listfile>
	A file which lists all the files and directories to be exported.
--srcdir=<dir>
	The path to Pike source directory.

Optional arguments:

--timestamp=<int>
        The timestamp of the build, if other than the real one.
--rebuild
	Not implemented.
--tag	Bump the Pike build version and tag the CVS tree.
--force
	Force export, ignore missing files.
--snapshot
	Use the generic name \"Pike-%maj.%min-snapshot\" for the
	base directory, instead of the same as the one specified
	with --name.
--help  Show this text.
";
