#!/usr/local/bin/pike

int last_len;
int redump_all;
string pike;
array(string) to_dump=({});
array(string) to_export=({});

int export;

#define MASTER_COOKIE "(#*&)@(*&$Master Cookie:"

void fail(string fmt, mixed ... args)
{
  if(last_len) write("\n");
  Stdio.perror(sprintf(fmt,@args));
  werror("**Installation failed.\n");
  exit(1);
}

string status(string doing, string file, string|void msg)
{
  file=replace(file,"\n","\\n");
  if(strlen(file)>50)
    file="..."+file[strlen(file)-48..];

  if(msg) file+=" "+msg;
  if(doing) file=doing+" "+file;
  string s="\r"+file;
  int t=strlen(s);
  if(t<last_len) s+=" "*(last_len-t);
  last_len=t;
  write(s);
}

int mkdirhier(string dir)
{
  int tomove;
  if(export) return 1;

  if(dir=="") return 1;
  status("creating",dir);
  mixed s=file_stat(dir);
  if(s)
  {
    if(s[1]<0)
      return 1;

    if(glob("*.pmod",dir))
    {
      if(!mv(dir,dir+".tmp"))
	fail("mv(%s,%s)",dir,dir+".tmp");
      tomove=1;
    }else{
      werror("Warning: Directory '%s' already exists as a file.\n",dir);
      if(!mv(dir,dir+".old"))
	fail("mv(%s,%s)",dir,dir+".old");
    }
  }

  mkdirhier(dirname(dir));
  if(!mkdir(dir))
    fail("mkdir(%s)",dir);

  chmod(dir,0755);

  if(tomove)
    if(!mv(dir+".tmp",dir+"/module.pmod"))
      fail("mv(%s,%s)",dir+".tmp",dir+"/module.pmod");

  return 1;
}

int compare_files(string a,string b)
{
  mixed sa=file_stat(a);
  mixed sb=file_stat(b);
  if(sa && sb && sa[1]==sb[1])
    return Stdio.read_file(a) == Stdio.read_file(b);
  return 0;
}

int low_install_file(string from,
		     string to,
		     void|int mode)
{
  if(export)
  {
//    werror("FROM: %O\n",from);
    to_export+=({ from });
    return 1;
  }

  status("installing",to);

  if(compare_files(from,to))
  {
    status("installing",to,"Already installed");
    return 0;
  }
  mkdirhier(dirname(to));
  switch(query_num_arg())
  {
    case 2:
      mode=0755;
  }
  string tmpfile=to+"-"+getpid()+"-"+time();
  if(!Stdio.cp(from,tmpfile))
    fail("copy(%s,%s)",from,tmpfile);

  /* Chown and chgrp not implemented yet */
  chmod(tmpfile,mode);

  /* Need to rename the old file to .old */
  if(file_stat(to))
  {
    rm(to+".old"); // Ignore errors
#if constant(hardlink)
    if( catch { hardlink(to,to+".old"); })
#endif 
      mv(to,to+".old");
  }
  if(!mv(tmpfile,to))
    fail("mv(%s,%s)",tmpfile,to);

  return 1;
}

int install_file(string from,
		 string to,
		 void|int mode,
		 void|int dump)
{
  int ret;
  if(query_num_arg() == 2)
    ret=low_install_file(from,to);
  else
    ret=low_install_file(from,to,mode);
  if((ret || redump_all) && dump)
  {
    switch(reverse(to)[..4])
    {
      case "ekip.":
	if(glob("*/master.pike",to)) break;
      case "domp.":
	to_dump+=({to});
    }
  }
  return ret;
}

string stripslash(string s)
{
  while(strlen(s)>1 && s[-1]=='/') s=s[..strlen(s)-2];
  return s;
}


void install_dir(string from, string to,int dump)
{
  from=stripslash(from);
  to=stripslash(to);

  mkdirhier(to);
  foreach(get_dir(from),string file)
    {
      if(file=="CVS") continue;
      if(file[..1]==".#") continue;
      if(file[0]=='#' && file[-1]=='#') continue;
      if(file[-1]=='~') continue;
      mixed stat=file_stat(combine_path(from,file));
      if (stat) {
	if(stat[1]==-2)
	{
	  install_dir(combine_path(from,file),combine_path(to,file),dump);
	}else{
	  install_file(combine_path(from,file),combine_path(to,file),0755,dump);
	}
      } else {
	werror(sprintf("\nstat:0, from:%O, file:%O, combined:%O\n",
		       from, file, combine_path(from, file)));
      }
    }
}

void install_header_files(string from, string to)
{
  from=stripslash(from);
  to=stripslash(to);
  mkdirhier(to);
  foreach(get_dir(from),string file)
    {
      if(file[..1]==".#") continue;
      if(file[-1]!='h' || file[-2]!='.') continue;
      install_file(combine_path(from,file),combine_path(to,file));
    }
}

void dumpmodules(string dir)
{
  dir=stripslash(dir);

  foreach(get_dir(dir),string file)
    {
      string f=combine_path(dir,file);
      mixed stat=file_stat(f);
      if(stat[1]==-2)
      {
	dumpmodules(f);
      }else{
	switch(reverse(file)[..4])
	{
	  case "domp.":
	  case "ekip.":
	    mixed stat2=file_stat(f+".o");

	    if(stat2 && stat2[3]>=stat[3])
	      continue;
	    
	    Process.create_process( ({pike,combine_path(vars->SRCDIR,"dumpmodule.pike"),f}) )->wait();
	}
      }
    }
}

mapping vars=([]);

string export_base_name;

int mklink(string from, string to)
{
#if constant(symlink)
  catch  {
    symlink(from, to);
    return 1;
  };
#endif
  return 0;
}

void do_export()
{
  export=0;

  cd("..");

  string tmpname=sprintf("PtmP%07x",random(0xfffffff));

  status("Creating","script glue");

  Stdio.write_file(tmpname+".x",
		   "#!/bin/sh\n"
		   "echo Unpacking...\n"
		   "tar xf \"$1\" "+tmpname+".tar.gz\n"
		   "gzip -dc "+tmpname+".tar.gz | tar xf -\n"
		   "rm -rf "+tmpname+".tar.gz\n"
		   "shift\n"
		   "( cd "+export_base_name+".dir\n"
		   "  build/pike -DNOT_INSTALLED -mbuild/master.pike -Mbuild/lib/modules -Mlib/modules bin/install.pike --interactive \\\n"
		   "  TMP_LIBDIR=\"build/lib\"\\\n"
		   "  LIBDIR_SRC=\"lib\"\\\n"
		   "  SRCDIR=\"src\"\\\n"
		   "  TMP_BINDIR=\"bin\"\\\n"
		   "  TMP_BUILDDIR=\"build\"\\\n"
		   "  MANDIR_SRC=\"man\"\\\n"
		   "  \"$@\"\n"
		   ")\n"
		   "rm -rf "+export_base_name+".dir "+tmpname+".x\n"
    );
  chmod(tmpname+".x",0755);
  string script=sprintf("#!/bin/sh\n"
			"tar xf \"$0\" %s.x\n"
			"exec ./%s.x \"$0\" \"$@\"\n",tmpname,tmpname,tmpname);
  if(strlen(script) >= 100)
  {
    werror("Script too long!!\n");
    exit(1);
  }

  string *parts=(script/"/");
  mkdirhier( parts[..sizeof(parts)-2]*"/");
  Stdio.write_file(script,"");

  to_export=Array.map(to_export,
	      lambda(string s)
	      {
		return combine_path(export_base_name+".dir",s);
	      });


  status("Creating",tmpname+".tar");
  
  Process.create_process(({"tar","cf",tmpname+".tar"})+ to_export)
    ->wait();

  status("Creating",tmpname+".tar.gz");

  Process.create_process(({"gzip","-9",tmpname+".tar"}))->wait();

  to_export=({script,tmpname+".x",tmpname+".tar.gz"});

  status("Creating",export_base_name);

  Process.create_process( ({ "tar","cf", export_base_name})+ to_export)
    ->wait();

  chmod(export_base_name,0755);

  status("Cleaning up","");

  Process.create_process( ({ "rm","-rf",
			       export_base_name+".dir",
			       export_base_name+".x"
			       }) ) ->wait();


  if(last_len)
  {
    status(0,"");
    status(0,"");
  }

  exit(0);
}

int main(int argc, string *argv)
{
  int traditional;
  string prefix;
  string exec_prefix;
  string lib_prefix;
  string include_prefix;
  string man_prefix;
  string lnk;
  string old_exec_prefix;
  object interactive;

  foreach(argv[1..], string foo)
    if(sscanf(foo,"%s=%s",string var, string value)==2)
      vars[var]=value;

  prefix=vars->prefix || "/usr/local";

  string install_type="";
  if(argc > 1) install_type=argv[1];

  if(!vars->TMP_BINDIR)
    vars->TMP_BINDIR=combine_path(vars->SRCDIR,"../bin");


  if(!vars->TMP_BUILDDIR) vars->TMP_BUILDDIR=".";

  while(1)
  {
  switch(install_type)
  {
    case "--traditional":
      exec_prefix=vars->exec_prefix;
      lib_prefix=vars->lib_prefix;
      include_prefix=combine_path(prefix,"include","pike");
      man_prefix=vars->man_prefix;
      break;

    case "--interactive":
      write("\n");
      write("   Welcome to the interactive Pike installation script.\n");
      write("\n");   
      interactive=Stdio.Readline();
      if(!vars->prefix)
	prefix=interactive->edit(prefix,"Install prefix: ");
      
      if(!vars->pike_name)
	vars->pike_name=interactive->edit(
	  combine_path(vars->exec_prefix || combine_path(prefix, "bin"),"pike"), "Pike binary name: ");
      destruct(interactive);
      install_type="--new-style";
//      trace(2);
      continue;

    case "--export":
      string ver=replace(replace(version()," ","-"),"-release-",".");
#if constant(uname)
      mixed u=uname();
      if(u->sysname=="AIX")
      {
	export_base_name=sprintf("%s-%s-%s.%s",
				 ver,
				 uname()->sysname,
				 uname()->version,
				 uname()->release);
      }else{
	export_base_name=sprintf("%s-%s-%s-%s",
				 ver,
				 uname()->sysname,
				 uname()->release,
				 uname()->machine);
      }
      export_base_name=replace(export_base_name,"/","-");
#else
      export_base_name=ver;
#endif


      mkdirhier(export_base_name+".dir");

      mklink(vars->LIBDIR_SRC,export_base_name+".dir/lib");
      mklink(vars->SRCDIR,export_base_name+".dir/src");
      mklink(getcwd(),export_base_name+".dir/build");
      mklink(vars->TMP_BINDIR,export_base_name+".dir/bin");
      mklink(vars->MANDIR_SRC,export_base_name+".dir/man");
      
      cd(export_base_name+".dir");

      vars->TMP_LIBDIR="build/lib";
      vars->LIBDIR_SRC="lib";
      vars->SRCDIR="src";
      vars->TMP_BINDIR="bin";
      vars->MANDIR_SRC="man";
      vars->TMP_BUILDDIR="build";

      export=1;
      to_export+=({ combine_path(vars->TMP_BINDIR,"install.pike") });
      
    case "":
    default:
    case "--new-style":
      if(!(lnk=vars->pike_name) || !strlen(lnk)) {
	lnk=combine_path(vars->exec_prefix || combine_path(vars->prefix, "bin"),"pike");
	old_exec_prefix=vars->exec_prefix; // to make the directory for pike link
      }
      prefix=combine_path("/",getcwd(),prefix,"pike",
			  replace(version()-"Pike v"," release ","."));
      exec_prefix=combine_path(prefix,"bin");
      lib_prefix=combine_path(prefix,"lib");
      include_prefix=combine_path(prefix,"include","pike");
      man_prefix=combine_path(prefix,"man");
      break;
  }
  break;
  }

  pike=combine_path(exec_prefix,"pike");

  mixed err=catch {
    if(export)
    {
      to_export+=({combine_path(vars->TMP_BUILDDIR,"pike")});
    }else{
      write("\nInstalling Pike in %s...\n\n",prefix);
      
      string pike_bin_file=combine_path(vars->TMP_BUILDDIR,"pike");
      status("Finalizing",pike_bin_file);
      string pike_bin=Stdio.read_file(pike_bin_file);
      int pos=search(pike_bin,MASTER_COOKIE);
      
      if(pos<=0 && strlen(pike_bin) < 10000 && 
	 file_stat(combine_path(vars->TMP_BUILDDIR,"pike.exe")))
      {
	pike_bin_file=combine_path(vars->TMP_BUILDDIR,"pike.exe");
	status("Finalizing",pike_bin_file);
	pike_bin=Stdio.read_file(pike_bin_file);
	pos=search(pike_bin,MASTER_COOKIE);
	pike+=".exe";
      }
      
      if(pos>=0)
      {
	status("Finalizing",pike_bin_file,"...");
	Stdio.write_file(pike_bin_file=combine_path(vars->TMP_BUILDDIR,"pike.tmp"),pike_bin);
	Stdio.File f=Stdio.File(pike_bin_file,"rw");
	f->seek(pos+strlen(MASTER_COOKIE));
	f->write(combine_path(lib_prefix,"master.pike"));
	f->close();
	status("Finalizing",pike_bin_file,"done");
      }else{
	write("Warning! Failed to finalize master location!\n");
      }
      if(install_file(pike_bin_file,pike)) redump_all=1;
    }

    install_file(combine_path(vars->TMP_BUILDDIR,"hilfe"),combine_path(exec_prefix,"hilfe"));
    string master=combine_path(vars->LIBDIR_SRC,"master.pike.in");
    
    if(export)
    {
      to_export+=({master,
		   combine_path(vars->TMP_BUILDDIR,"master.pike"),
		   combine_path(vars->SRCDIR,"dumpmaster.pike"),
		   combine_path(vars->SRCDIR,"dumpmodule.pike")
      });
    }else{
//    werror("Making master with libdir=%s\n",lib_prefix);
      status("Finalizing",master);
      string master_data=Stdio.read_file(master);
      master_data=replace(master_data,"¤lib_prefix¤",replace(lib_prefix,"\\","\\\\"));
      Stdio.write_file(combine_path(vars->TMP_LIBDIR,"master.pike"),master_data);
      status("Finalizing",master,"done");
    }
    
    install_dir(vars->TMP_LIBDIR,lib_prefix,1);
    install_dir(vars->LIBDIR_SRC,lib_prefix,1);
    
    install_header_files(vars->SRCDIR,include_prefix);
    install_header_files(vars->TMP_BUILDDIR,include_prefix);
    
    install_file(combine_path(vars->TMP_BUILDDIR,"modules/dynamic_module_makefile"),
		 combine_path(include_prefix,"dynamic_module_makefile"));
    install_file(combine_path(vars->TMP_BUILDDIR,"aclocal"),
		 combine_path(include_prefix,"aclocal.m4"));
    
    if(file_stat(vars->MANDIR_SRC))
    {
      install_dir(vars->MANDIR_SRC,combine_path(man_prefix,"man1"),0);
    }
  };

  if(last_len)
  {
    status(0,"");
    status(0,"");
  }

  if(err) throw(err);


  if(export)
  {
    do_export();
  }else{
    string master=combine_path(lib_prefix,"master.pike");
    mixed s1=file_stat(master);
    mixed s2=file_stat(master+".o");
    if(!s1 || !s2 || s1[3]>=s2[3] || redump_all)
    {
      Process.create_process( ({pike,"-m",combine_path(vars->SRCDIR,"dumpmaster.pike"),master}))->wait();
    }
    
//    dumpmodules(combine_path(vars->lib_prefix,"modules"));
    if(sizeof(to_dump))
    {
      foreach(to_dump, string mod) rm(mod+".o");
      Process.create_process( ({pike,combine_path(vars->SRCDIR,"dumpmodule.pike")}) + to_dump)->wait();
    }
    
#if constant(symlink)
    if(lnk)
    {
      mixed s=file_stat(lnk,1);
      if(s)
      {
	if(!mv(lnk,lnk+".old"))
	{
	  werror("Failed to move %s\n",lnk);
	  exit(1);
	}
      }
      if (old_exec_prefix) {
	mkdirhier(old_exec_prefix);
      }
      mkdirhier(dirname(lnk));
      symlink(pike,lnk);
    }
#endif
  }
  write("\nDone\n");
    
  return 0;
}
