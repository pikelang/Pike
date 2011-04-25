// -*- Pike -*-

// $Id$

#pike __REAL_VERSION__

// Source directory
string srcdir;
string make="make";
string make_flags="";
string include_path=master()->include_prefix;
string configure_command="configure";
#ifdef NOT_INSTALLED
string src_path=combine_path(__FILE__,"../../../../../src");
string bin_path=combine_path(src_path,"../bin");
#else
string src_path=include_path;
string bin_path=include_path;
#endif
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
  if(search(path,"$src")==-1) return path;
  if(!srcdir)
  {
    string s=Stdio.read_file("Makefile");
    if(s)
    {
      sscanf(s,"%*s\nSRCDIR=%s\n",srcdir);
    }
    if(!srcdir && file_stat("configure.in"))
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
  foreach(a,string file)
    {
      mixed s=file_stat(fix(file));
      if(!s) return 0;
      t=max(t,s[3]);
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
  array(string) makecmd=(
    ({make})+
    do_split_quoted_string(make_flags)+
    ({"PIKE_INCLUDES=-I"+include_path,
      "PIKE_SRC_DIR="+src_path,
      "BUILD_BASE="+include_path,
#ifdef NOT_INSTALLED
      "MODULE_BASE="+include_path+"/modules",
#else
      "MODULE_BASE="+include_path,
#endif
      "TMP_BINDIR="+bin_path,
      "SRCDIR="+fix("$src"),
      "TMP_MODULE_BASE=.",
      "RUNPIKE="+run_pike,
    })+
    cmd);

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
    )),array opt)
    {
      switch(opt[0])
      {
	case "query":
	  write("%s\n",this_object()[opt[1]]);
	  exit(0);

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

  load_specs(include_path+"/specs");
  
  int tmp1;
  if(run->automake)
  {
    if(tmp1=max_time_of_files("$src/Makefile.am","$src/configure.in"))
    {
      if(run->automake == ALWAYS ||
	 max_time_of_files("$src/Makefile.in") < tmp1)
      {
	run_or_fail((["dir":srcdir]),"aclocal");
	run_or_fail((["dir":srcdir]),"automake");
	rm(fix("$src/stamp-h.in"));
      }
    }
  }

  if(run->autoheader)
  {
    if(tmp1=max_time_of_files("$src/acconfig.h","$src/configure.in"))
    {
      if(run->autoheader==ALWAYS ||
	 max_time_of_files("$src/stamp-h.in") <= tmp1)
      {
	run_or_fail((["dir":srcdir]),"autoheader");
	rm(fix("$src/stamp-h.in"));
	Stdio.write_file(fix("$src/stamp-h.in"),"foo\n");
      }
    }
  }

  if(run->autoconf)
  {
    if(tmp1=max_time_of_files("$src/configure.in"))
    {
      if(run->autoconf==ALWAYS ||
	 max_time_of_files("$src/configure") <= tmp1)
      {
	run_or_fail((["dir":srcdir]),"autoconf","--localdir="+src_path);
      }
    }
  }

  if(run->configure)
  {
    // If there is a makefile, we can use the auto-run-configure
    // feature...
    if(run->configure == ALWAYS || !max_time_of_files("Makefile"))
    {
      if(max_time_of_files("$src/configure"))
      {
	if(!max_time_of_files("config.cache"))
	  if(max_time_of_files(include_path+"/config.cache"))
	  {
	    Stdio.cp(include_path+"/config.cache","config.cache");
	  }
	  else
	  {
	    Stdio.append_file("config.cache", "");
	  }
	array(string) cfa = do_split_quoted_string(specs->configure_args||"");
	mapping(string:string) cfa_env = ([]);
	foreach(cfa; int i; string arg) {
	  string key, val;
	  if(2==sscanf(arg, "%[a-zA-Z0-9_]=%s", key, val)) {
	    cfa_env[key] = val;
	    cfa[i] = 0;
	  }
	}
	run_or_fail((["env":getenv()|cfa_env|
		      (specs & ({"CC","CFLAGS","CPPFLAGS","CPP",
				 "LDFLAGS","LDSHARED"}))|
		      (["PIKE_SRC_DIR":src_path,
			"BUILD_BASE":include_path])]),
		    srcdir+"/"+configure_command,
		    "--cache-file=./config.cache",
		    @(cfa-({0})));
      }
    }
  }

  if(run->depend)
  {
    if(!max_time_of_files("$src/dependencies") || run->depend == ALWAYS)
    {
      // Create an empty file first..
      Stdio.write_file(srcdir+"/dependencies","");
      do_make( ({"depend"}) );
      do_make( ({"Makefile"}) );
    }
  }

  if(run->make)
  {
    do_make(argv[1..]);
  }
}
