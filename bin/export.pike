#! /usr/bin/env pike

multiset except_modules = (<>);
string vpath;

string main_branch;

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
  files -= ({ ".git", ".gitignore", ".gitattributes", });

  array(string) ret = ({});
  foreach(files, string fn)
  {
    if( fn=="core" ) continue;
    if( fn[-1]=='~' ) continue;
    if( sizeof(fn)>2 && fn[0]=='#' && fn[-1]=='#' ) continue;
    if( sizeof(fn)>2 && fn[0]=='.' && fn[1]=='#' ) continue;

    if( path==vpath+"/src/modules" && except_modules[fn] )
      continue;

    if( path==vpath+"/bin" && except_modules[fn] )
      continue;

    if( has_prefix(path, vpath+"/lib/modules") &&
	(except_modules[fn] || except_modules[fn - ".pmod"]))
      continue;

    fn = path+"/"+fn;

    if( Stdio.is_dir(fn) )
      ret += get_files(fn);
    else
      ret += ({ fn });
  }
  return ret;
}

void run( string cmd, mixed ... args )
{
  function fail;
  if( functionp(args[-1]) )
  {
    fail = args[-1];
    args = args[..<1];
  }
  mapping mods;
  if( mappingp(args[-1]) )
  {
    mods = args[-1];
    args = args[..<1];
  }

  mapping r = Process.run( ({ cmd }) + args, mods );
  if( r->exitcode && fail )
  {
    fail(r);
  }
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
      run( "autoconf", ([ "cwd":dir ]) );
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

string low_bump_version(int|void bump_minor)
{
  string s = Stdio.read_file(pike_base_name+"/src/version.h");
  if (bump_minor) {
    sscanf(s, "%s PIKE_MINOR_VERSION %d%s", string pre, int minor, string post);
    minor++;
    s = pre + " PIKE_MINOR_VERSION " + minor + post;
  }
  sscanf(s, "%s PIKE_BUILD_VERSION %d%s", string pre, int rel, string post);
  if (bump_minor) {
    rel = 0;
  } else {
    rel++;
  }
  s = pre + " PIKE_BUILD_VERSION " + rel + post;
  Stdio.write_file( pike_base_name+"/src/version.h", s);
  return ((array(string))getversion())*".";
}

string git_cmd(string ... args)
{
  mapping r = Process.run( ({ "git" }) + args );
  if( r->exitcode )
    exit(r->exitcode, "Git command \"git %s\" failed.\n%s", args*" ", r->stderr||"");
  return String.trim_all_whites(r->stdout||"");
}

void git_bump_version()
{
  werror("Bumping release number.\n");

  int bump_minor = 0;

  string branch = git_cmd("symbolic-ref", "HEAD");

  if (has_prefix(branch, "refs/heads/")) {
    branch = branch[sizeof("refs/heads/")..];
    string upstream_branch =
      git_cmd("config", "--get", "branch."+branch+".merge");
    if (has_prefix(upstream_branch, "refs/heads/")) {
      upstream_branch = upstream_branch[sizeof("refs/heads/")..];

      // Bump the minor if the upstream branch is a single number.
      bump_minor = (sizeof(upstream_branch/".") < 2) && (int)upstream_branch;
    }
  }

  string rel = low_bump_version(bump_minor);

  string attrs = Stdio.read_file(pike_base_name+"/.gitattributes");

  if (attrs) {
    string new_attrs = (attrs/"\n/src/version.h foreign_ident\n")*"\n";
    if (new_attrs != attrs) {
      werror("Adjusting attributes.\n");
      Stdio.write_file(pike_base_name+"/.gitattributes", new_attrs);
    }
    git_cmd("add", ".gitattributes");
  }

  git_cmd("add", "src/version.h");

  git_cmd("commit", "-m", "release number bumped to "+rel+" by export.pike");
}

array(string) build_file_list(string list_file)
{
  if(!file_stat(list_file)) {
    werror("Could not find %s\n", list_file);
    exit(1);
  }

  array(string) ret=({ }), missing=({ });
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
int(0..1) ignore_missing;

void clean_exit(int err, mixed ... msg)
{
  rm("buildid.txt");

  if( vpath )
    Stdio.recursive_rm(vpath);

  /* Roll forward to a useable state. */
  if(main_branch)
    git_cmd("checkout", main_branch);

  exit(err, @msg);
}

int main(int argc, array(string) argv)
{
  array(string) files;
  string export_list, filename;
  int tag, snapshot, t;

  foreach(Getopt.find_all_options(argv, ({
    ({ "srcdir",    Getopt.HAS_ARG, "--srcdir"     }),
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

        case "force":
	  ignore_missing=1;
	  break;

        case "tag":
	  tag=1;
	  break;
	
        case "help":
	  exit(0, documentation);

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
  if(!srcdir || !export_list || !filename)
    exit(1, documentation);

  if (tag) {
    main_branch = git_cmd("symbolic-ref", "-q", "HEAD");
    if (!has_prefix(main_branch, "refs/heads/"))
      exit(1, "Unexpected HEAD: %O\n", main_branch);

    main_branch = main_branch[sizeof("refs/heads/")..];
    string remote = git_cmd("remote");
    if (!sizeof(remote)) remote = 0;
    if (remote) git_cmd("pull", "--rebase", remote);

    /* Tagging in git is fast, so there's no need to
     * do it asynchronously. We also want to perform
     * the version bumps back-to-back, to avoid
     * ambiguities regarding the stable version.
     *
     * Note that the tagging is performed in a
     * second push in case the paranoia rebase
     * below actually has any effect.
     */

    git_bump_version();
    array(int) version = getversion();
    vpath = sprintf("Pike-v%d.%d.%d", @version);
    string tag = sprintf("v%d.%d.%d", @version);

    git_bump_version();

    if (remote) {
      /* Push the result. */
      git_cmd("pull", "--rebase", remote);	/* Paranoia... */
      git_cmd("push", remote);
    }

    /* Now it's time for the tags. */
    git_cmd("tag", tag, "HEAD^");
    if (remote) {
      git_cmd("push", remote, "refs/tags/" + tag + ":refs/tags/" + tag);
    }

    /* Bumping is done, now go back to the stable version,
     * so that we can create the dist files. */
    git_cmd("checkout", tag);
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

  vpath = filename = replace(filename,symbols);

  if (snapshot)
    vpath = sprintf("Pike-v%d.%d-snapshot", @version);

  fix_configure(pike_base_name+"/src");

  foreach(get_dir(pike_base_name+"/src/modules"), string fn)
    if(Stdio.is_dir(pike_base_name+"/src/modules/"+fn))
      fix_configure("modules/"+fn);

  rm(vpath);
  symlink(".", vpath);

  files = build_file_list(export_list);
  if(!files)
    clean_exit(1, "Unable to build file list.\n");

  Stdio.write_file("buildid.txt", replace(stamp, symbols));
  files += ({ vpath+"/buildid.txt" });

  werror("Creating "+filename+".tar.gz.\n");

  int first = 1;
  foreach(files/25.0, files)
  {
    run( "tar", first?"cf":"rf", pike_base_name+"/"+filename+".tar", @files )
    {
      clean_exit(1, "Tar file creation failed!\n");
    };
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
    run( "cp", "-R", build+"/doc_build/images", vpath+"/refdoc/images" );
    run( "tar", "rf", pike_base_name+"/"+filename+".tar",
         vpath+"/refdoc/autodoc.xml", vpath+"/refdoc/images" )
    {
      clean_exit(1, "Tar file creation failed!\n");
    };
    Stdio.recursive_rm(vpath);
  }

  run( "gzip", "-9", pike_base_name+"/"+filename+".tar" )
  {
    clean_exit(1, "Gzip failed!\n");
  };

  clean_exit(0, "Done.\n");
}

constant documentation = #"
Usage: export.pike <arguments> <except modules>

Creates a pike distribution. Needs one tar and one gzip binary in the
path. The modules listed in <except modules> will not be included in
the exported Pike.

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
--tag	Bump the Pike build version and tag git.
--force
	Force export, ignore missing files.
--snapshot
	Use the generic name \"Pike-%maj.%min-snapshot\" for the
	base directory, instead of the same as the one specified
	with --name.
--help  Show this text.
";
