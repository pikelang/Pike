#!/usr/local/bin/pike

int last_len;
int redump_all;
string pike;
array(string) to_dump=({});

void fail(string fmt, mixed ... args)
{
  if(last_len) write("\n");
  Stdio.perror(sprintf(fmt,@args));
  werror("**Installation failed.\n");
  exit(1);
}

string status(string doing, string file, string|void msg)
{
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
  status("creating",dir);
  mixed s=file_stat(dir);
  if(s)
  {
    if(s[1]<0)
      return 1;

    werror("mkdir: Directory '%s' already exists as a file.\n",dir);
    exit(1);
  }

  mkdirhier(dirname(dir));
  if(!mkdir(dir))
    fail("mkdir(%s)",dir);

  chmod(dir,0755);

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
      if(stat[1]==-2)
      {
	install_dir(combine_path(from,file),combine_path(to,file),dump);
      }else{
	install_file(combine_path(from,file),combine_path(to,file),0755,dump);
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
      if(file[-1]!='h' && file[-2]!='.') continue;
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
	    
	    Process.create_process( ({pike,combine_path(getenv("SRCDIR"),"dumpmodule.pike"),f}) )->wait();
	}
      }
    }
}


int main(int argc, string *argv)
{
  mixed err=catch {
    pike=combine_path(getenv("exec_prefix"),"pike");
    write("\nInstalling Pike... in %s\n\n",getenv("prefix"));
    if(install_file("pike",pike))
    {
      redump_all=1;
    }
    install_file("hilfe",combine_path(getenv("exec_prefix"),"hilfe"));
    install_dir(getenv("TMP_LIBDIR"),getenv("lib_prefix"),1);
    install_dir(getenv("LIBDIR_SRC"),getenv("lib_prefix"),1);
    
    install_header_files(getenv("SRCDIR"),
			 combine_path(getenv("prefix"),"include/pike"));
    install_header_files(".",combine_path(getenv("prefix"),"include/pike"));
    
    install_file("modules/dynamic_module_makefile",
		 combine_path(getenv("prefix"),"include/pike/dynamic_module_makefile"));
    install_file("./aclocal",
		 combine_path(getenv("prefix"),"include/pike/aclocal.m4"));
    
    if(file_stat(getenv("MANDIR_SRC")))
    {
      install_dir(getenv("MANDIR_SRC"),combine_path(getenv("man_prefix"),"man1"),0);
    }




  };

  if(last_len)
  {
    status(0,"");
    status(0,"");
  }

  if(err) throw(err);

  string master=combine_path(getenv("lib_prefix"),"master.pike");
  mixed s1=file_stat(master);
  mixed s2=file_stat(master+".o");
  if(!s1 || !s2 || s1[3]>=s2[3] || redump_all)
  {
    Process.create_process( ({pike,"-m",combine_path(getenv("SRCDIR"),"dumpmaster.pike"),master}))->wait();
  }
  
//    dumpmodules(combine_path(getenv("lib_prefix"),"modules"));
  if(sizeof(to_dump))
  {
    Process.create_process( ({pike,combine_path(getenv("SRCDIR"),"dumpmodule.pike")}) + to_dump)->wait();
  }


  return 0;
}
