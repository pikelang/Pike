// -*- Pike -*-

// $Id$

#pike __REAL_VERSION__

constant version =
 sprintf("%d.%d.%d",(int)__REAL_VERSION__,__REAL_MINOR__,__REAL_BUILD__);
constant description = "Pike module installer.";

// Source directory
string srcdir;
string make=getenv("MAKE")||"make";
string make_flags="";
string include_path=master()->include_prefix;
string config_args="";
#ifdef NOT_INSTALLED
string src_path=combine_path(__FILE__,"../../../../../src");
string bin_path=combine_path(src_path,"../bin");
#else
string src_path=include_path;
string bin_path=include_path;
#endif

// this is not the ideal location for all systems, but it's a start.
string local_module_path=combine_path(getenv("HOME")||"","lib/pike/modules");
bool old_style_module = false;
// we prefer the last element, because if there are more than one
// master() puts the lib/modules path last.
string system_module_path=master()->system_module_path[-1];

// where do we install the documentation?
string system_doc_path = master()->doc_prefix;

string run_pike;

#define NOT 0
#define AUTO 1
#define ALWAYS 2

mapping(string:int) run=
([
  "automake":AUTO,
  "autoheader":AUTO,
  "autoconf":AUTO,
  "configure":AUTO,
  "depend":AUTO,
  "make":AUTO,
]);

mapping(string:string) specs = ([]);

void load_specs(string fn)
{
  string name, val;
  foreach(Stdio.read_file(fn)/"\n", string line)
    if(2==sscanf(line, "%s=%s", name, val) && !specs[name])
      specs[name] = val;

  // We should try to find the core autodoc file
  if (!system_doc_path) {
    if(file_stat(combine_path(system_module_path,
			      "../../doc/src/core_autodoc.xml")))
      system_doc_path=combine_path(system_module_path, "../../doc");
    else if(file_stat(combine_path(system_module_path,
				   "../../../doc/pike/src/core_autodoc.xml")))
      system_doc_path=combine_path(system_module_path, "../../../doc/pike");
    else if(file_stat(combine_path(system_module_path, "../../doc")))
      system_doc_path=combine_path(system_module_path, "../../doc");
    else if(file_stat(combine_path(system_module_path, "../../../doc/pike")))
      system_doc_path=combine_path(system_module_path, "../../../doc/pike");
    else {
      // No autodoc file or directory, but we set this path as doc path anyway.
      system_doc_path = combine_path(system_module_path, "../../doc");
    }
  }
}

array(string) do_split_quoted_string(string s)
{
  array(string) res = Process.split_quoted_string(s);
  if(sizeof(res) == 1 && res[0] == "" &&
     search(s, "'")<0 && search(s, "\"")<0)
    return ({});
  else
    return res;
}

string fix(string path)
{
  if(!has_value(path, "$src")) return path;
  if(!srcdir)
  {
    string s=Stdio.read_file("Makefile");
    if(s)
    {
      sscanf(s,"%*s\nSRCDIR=%s\n",srcdir);
    }
    if(!srcdir)
    {
      srcdir=".";
    }
    
    if(!srcdir)
    {
      werror("You must provide --source=<source dir>\n");
      exit(1);
    }
  }
  return replace(path,"$src",srcdir);
}

void do_zero()
{
  foreach(indices(run), string x) if(run[x]==AUTO) m_delete(run,x);
}

int max_time_of_files(string ... a)
{
  int t=0;
  foreach(a, string file)
  {
    Stdio.Stat s = file_stat(fix(file));
    if(!s) return 0;
    t = max(t, s->mtime);
  }
  return t;
}


int just_run(mapping options, string ... cmd)
{
  werror(" %s\n",cmd*" ");
  return Process.create_process(cmd,(["env":getenv()])|options)->wait();
}

void run_or_fail(mapping options,string ... cmd)
{
  werror(" %s\n",cmd*" ");
  int ret=Process.create_process(cmd,(["env":getenv()])|options)->wait();
  if(ret)
    exit(ret);
}


void do_make(array(string) cmd)
{
  int tmp1;
  string lmp;
  string full_srcdir;

  if(search(cmd, "testsuite")!=-1)
    lmp="./plib/modules";
  else
    lmp = local_module_path;
  
  if(srcdir !=".") full_srcdir=srcdir + "/";
  else full_srcdir=getcwd() + "/";

  array extra_args = ({});
  
  if( old_style_module )
  {
    extra_args =
      ({"PIKE_INCLUDES=-I"+include_path,
	"PIKE_SRC_DIR="+src_path,
	"BUILD_BASE="+include_path,
	"MODULE_BASE="+combine_path(include_path, "modules"),
	"TMP_BINDIR="+bin_path,
	"SRCDIR="+fix("$src"),
	"FULL_SRCDIR=" + full_srcdir,
	"TMP_MODULE_BASE=.",
	"PIKE_EXTERNAL_MODULE=pike_external_module",
	"CORE_AUTODOC_PATH=" + combine_path(system_doc_path, "src/core_autodoc.xml"),
	"SYSTEM_DOC_PATH=" + system_doc_path + "/",
	"SYSTEM_MODULE_PATH=" + system_module_path,
	"LOCAL_MODULE_PATH=" + lmp,
	"RUNPIKE="+run_pike,
      });
  }
  else
  {
    extra_args = ({
      "PIKE="+run_pike,
      "SRCDIR="+fix("$src"),
      "MODULE_INSTALL_DIR="+combine_path(__FILE__,"../../.."),
      "LOCAL_MODULE_PATH=" + lmp,
    });
  }
  
  array(string) makecmd=({make})+do_split_quoted_string(make_flags)+extra_args+cmd;

  if(tmp1=max_time_of_files("Makefile"))
  {
    rm("remake");
    tmp1=just_run(([]),@makecmd);
    if(tmp1)
    {
      if(max_time_of_files("remake"))
      {
	run_or_fail(([]),@makecmd);
      }else{
	exit(tmp1);
      }
    }
  }else{
    werror("No Makefile.\n");
  }
}

int main(int argc, array(string) argv)
{
  string specspath=combine_path(include_path, "specs");

  if(!Stdio.is_file(specspath))
  {
    werror("Missing specs file at %s\n",specspath);
    return 1;
  }

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

  load_specs(specspath);
  
  foreach(Getopt.find_all_options(argv,aggregate(
    ({"autoconf",Getopt.NO_ARG,({"--autoconf"}) }),
    ({"configure",Getopt.NO_ARG,({"--configure"}) }),
    ({"autoheader",Getopt.NO_ARG,({"--autoheader"}) }),
    ({"automake",Getopt.NO_ARG,({"--automake"}) }),
    ({"depend",Getopt.NO_ARG,({"--depend"}) }),
    ({"all",Getopt.NO_ARG,({"--all"}) }),
    ({"make",Getopt.NO_ARG,({"--make"}) }),
    ({"auto",Getopt.NO_ARG,({"--auto"}) }),
    ({"source",Getopt.HAS_ARG,({"--source"}) }),
    ({"query",Getopt.HAS_ARG,({"--query"}) }),
    ({"config_args",Getopt.HAS_ARG,({"--configure-args"}) }),
    ({"help",Getopt.NO_ARG,({"--help"}) }),
    )),array opt)
    {
      switch(opt[0])
      {
	case "query":
	  if(opt[1]=="specs")
	    write("%O\n", specs);
	  else if(stringp(this[opt[1]]))
	    write("%s\n", this[opt[1]]);
	  else
	    write("Unknown variable %s.\n", opt[1]);
	  exit(0);
        case "help":
	  write(help, version);
	  exit(0);

        case "config_args": config_args=opt[1]; break;
	case "source": srcdir=opt[1]; break;
	case "automake": run->automake=ALWAYS; do_zero(); break;
	case "autoheader": run->autoheader=ALWAYS; do_zero(); break;
	case "autoconf": run->autoconf=ALWAYS; do_zero(); break;
	case "configure": run->configure=ALWAYS; do_zero(); break;
	case "make": run->make=ALWAYS; do_zero(); break;
	case "depend": run->depend=ALWAYS; do_zero(); break;
	  
	case "all": 
	  run->depend=run->autoheader=run->autoconf=run->configure=run->make=ALWAYS;
	  break;
	  
	case "auto":
	  run->depend=run->autoheader=run->autoconf=run->configure=run->make=AUTO;
	  break;
      }
    }

  argv=Getopt.get_args(argv);
  string configure_args="";
  
  foreach( argv, string arg )
    if( sscanf( arg, "CONFIGUREARGS=%s", configure_args ) )
      argv-=({arg});

  string configure_in = "$src/configure.in";
  if( max_time_of_files( configure_in ) <  max_time_of_files( "$src/configure.ac" ) )
    configure_in = "$src/configure.ac" ;
  int tmp1;

  if(run->automake)
  {
    if(tmp1=max_time_of_files("$src/Makefile.am",configure_in))
    {
      write("** Running automake\n"); 
      if(run->automake == ALWAYS ||
	 max_time_of_files("$src/Makefile.in") < tmp1)
      {
	run_or_fail((["cwd":srcdir]),"aclocal");
	run_or_fail((["cwd":srcdir]),"automake");
	rm(fix("$src/stamp-h.in"));
      }
    }
  }
  
  
  if( max_time_of_files( configure_in ) <  max_time_of_files( "$src/configure.ac" ) )
    configure_in = "$src/configure.ac" ;

  string configure_content = Stdio.read_file(fix(configure_in))||"";
  old_style_module = has_value(configure_content, "AC_MODULE_INIT" );
  if( old_style_module )
  {
    write("** Old style module\n");
  }

  string stamp_file = "$src/stamp-h.in";

  if( sscanf( configure_content, "%*sAC_CONFIG_HEADER(%[^)])", stamp_file ) == 2 )
    stamp_file="$src/"+stamp_file+".in";
  
  if(run->autoheader)
  {
    if(tmp1=max_time_of_files("$src/acconfig.h",configure_in))
    {
      if(run->autoheader==ALWAYS || max_time_of_files(stamp_file) <= tmp1)
      {
	run_or_fail((["cwd":srcdir]),"autoheader");
	if( stamp_file == "$src/stamp-h.in" )
	  Stdio.write_file(fix(stamp_file),"foo\n");
      }
    }
  }

  if(run->autoconf)
  {
    if(tmp1=max_time_of_files(configure_in))
    {
      if(run->autoconf==ALWAYS || max_time_of_files("$src/configure") <= tmp1)
      {
	if( old_style_module )
	{
	  write("** Running autoconf (with extra compat macros)\n");
	  string data = Process.popen("autoconf --version");
	  data = (data/"\n")[0];
	  float v;
	  sscanf(data, "%*s%f", v);

	  // If we fail to determine the autoconf version we assume
	  // yet another incompatble autoconf change.
	  if(!v || v>2.52)
	    run_or_fail((["cwd":srcdir]),"autoconf","--include="+src_path);
	  else
	    run_or_fail((["cwd":srcdir]),"autoconf","--localdir="+src_path);
	}
	else
	{
	  write("** Running autoconf\n");
	  run_or_fail( (["cwd":srcdir]), "autoconf" );
	}
      }
    }
  }

  if(run->configure)
  {
    if( tmp1=max_time_of_files("$src/configure") )
    {
      if(run->configure == ALWAYS || (max_time_of_files("Makefile") <= tmp1))
      {
	if( old_style_module )
	{
	  if(!max_time_of_files("propagated_variables")) {
	    write("** Copying propagated_variables\n");
	    Stdio.write_file("propagated_variables", Stdio.read_file(combine_path(include_path, "propagated_variables")));
	  }

	  write("** Running configure (with extra compat args)\n");
	  array(string) cfa = do_split_quoted_string(specs->configure_args||"");
	  mapping(string:string) cfa_env = ([]);
	  foreach(cfa; int i; string arg)
	  {
	    string key, val;
	    if(2==sscanf(arg, "%[a-zA-Z0-9_]=%s", key, val))
	    {
	      cfa_env[key] = val;
	      cfa[i] = 0;
	    }
	  }
	  run_or_fail((["env":getenv()|cfa_env|
			(specs & ({"CC","CFLAGS","CPPFLAGS","CPP",
				   "LDFLAGS","LDSHARED"}))|
			(["PIKE_SRC_DIR":src_path,
			  "BUILD_BASE":include_path])]),
		      fix("$src/configure"),
		      "--cache-file=./config.cache",
		      @(cfa-({0})), @do_split_quoted_string(config_args));
	}
	else
	{
	  write("** Running configure\n");
	  run_or_fail( ([ "env":getenv()|
			  ([
			    "PIKE":run_pike,
			    "MODULE_INSTALL_DIR":combine_path(__FILE__,"../../.."),
			    "LOCAL_MODULE_PATH":local_module_path,
			  ])
		       ]),
		       fix("$src/configure"),
		       @do_split_quoted_string(configure_args));
	}
      }
    }
  }

  if( old_style_module && run->depend )
  {
    if(!max_time_of_files("$src/dependencies") || run->depend == ALWAYS)
    {
      write("** Running make depend\n");
      // Create an empty file first..
      Stdio.write_file(srcdir+"/dependencies","");
      do_make( ({"depend"}) );
      do_make( ({"Makefile"}) );
    }
  }

  if(run->make)
  {
    write("** Running "+argv[1..]*" ");
    do_make(argv[1..]);
  }
}

constant help=#"Pike module installer %s.
module <options> <arguments passed to make>

Information options:
  --help        Prints this text
  --query=X     Shows the value of setting X. Possible settings are
                include_path, configure_command, src_path, bin_path,
                system_module_path, make and specs.

Stage selection:
                If none of these stages are selected, all of them will
                be run, given that the timestamps of the dependncies
                indicates that it is needed. If one or more stage is
                selected, these stages will always run, but none of
                the unselected ones.

  --automake    Runs aclocal and automake.
  --autoheader  Runs autoheader.
  --autoconf    Runs autoconf.
  --configure   Runs configure.
  --depend      Runs make depend and make Makefile.
  --make        Runs make.
  --all         Runs all of the above.
  --auto        Sets all stages to auto (default).

Environment:
  --source=X    Path to the source directory.
  --configure-args
                Additional arguments to configure.
";
