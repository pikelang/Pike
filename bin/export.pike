#!/usr/local/bin/pike

/* $Id: export.pike,v 1.24 1999/07/02 14:16:54 grubba Exp $ */

#include <simulate.h>

multiset except_modules  =(<>);
string vpath;

string *get_files(string path)
{
  string *files,tmp,*ret;
  files=get_dir(path);
  files-=({"CVS","RCS",".cvsignore"});
  ret=({});
  foreach(files,tmp)
  {
    if(tmp[-1]=='~') continue;
    if(tmp[0]=='#' && tmp[-1]=='#') continue;
    if(tmp[0]=='.' && tmp[1]=='#') continue;

    if(path==vpath+"/src/modules" && except_modules[tmp])
      continue;

    if(path==vpath+"/bin" && except_modules[tmp])
      continue;

    if(search(path,vpath+"/lib/modules")==0 &&
       (except_modules[tmp] || except_modules[tmp - ".pmod"]))
      continue;

    tmp=path+"/"+tmp;

    if(file_size(tmp)==-2)
    {
      ret+=get_files(tmp);
    }else{
      ret+=({tmp});
    }
  }
  return ret;
}

void fix_configure(string dir)
{
  int *config,*config_in;
  config=file_stat(dir+"/configure");
  config_in=file_stat(dir+"/configure.in");

  if(config_in)
  {
    if(!config || config_in[3] > config[3])
    {
      werror("Fixing configure in "+dir+".\n");
      Process.create_process(({"autoconf"}),(["cwd":dir]))->wait();
    }
  }
}

string getversion()
{
  string s=Stdio.read_file("pike/src/version.c");
  if(!sscanf(s,"%*spush_text(\"%s\")",s))
  {
    werror("Failed to get Pike version.\n");
    exit(1);
  }
  return s;
}

void bump_version()
{
  werror("Bumping release number.\n");
  Process.create_process(({ "cvs", "update", "version.h" }),
			 ([ "cwd":"pike/src" ]))->wait();

  string s=Stdio.read_file("pike/src/version.h");
  sscanf(s,"%s PIKE_BUILD_VERSION %d%s",string pre, int rel, string post);
  rel++;
  Stdio.File("pike/src/version.h", "wct")->
    write(pre+" PIKE_BUILD_VERSION "+rel+post);
  Process.create_process(({ "cvs", "commit", "-m",
			    "release number bumped to "+rel+" by export.pike",
			    "version.h" }),
			 ([ "cwd":"pike/src" ]))->wait();
}


int main(int argc, string *argv)
{
  mixed tmp;
  int e;
  string *files;
  object cvs;

  tmp=reverse(argv[0]/"/");
  except_modules=mklist(argv[1..]);
  e=search(tmp,"pike");
  if(e==-1)
  {
    werror("Couldn't find Pike source dir.\n");
    werror("Use /full/path/export.pike <except modules>.\n");
    exit(1);
  }
  tmp=reverse(tmp[e+1..]);
  cd(tmp*"/");
  werror("Sourcedir = "+tmp*"/"+"/pike\n");

  if(file_stat("pike/CVS"))
  {
    bump_version();

    vpath=replace(replace(getversion()," ","-"),"-release-",".");
    string tag=replace(vpath,({"Pike-","."}),({"","_"}));

    werror("Creating tag "+tag+" in the background.\n");
    cvs=Process.create_process(({"cvs","tag","-R","-F",tag}),
			   (["cwd":"pike"]));
  }else{
    vpath=replace(replace(getversion()," ","-"),"-release-",".");
  }

  fix_configure("pike/src");

  foreach(get_dir("pike/src/modules") - ({"CVS","RCS"}), tmp)
    if(file_size("pike/src/modules/"+tmp) == -2)
      fix_configure("modules/"+tmp);

    werror("vpath = %s\n",vpath);
  system("ln -s pike "+vpath);

  files=sum(({ vpath+"/README", vpath+"/ANNOUNCE" }),
	    get_files(vpath+"/src"),
	    get_files(vpath+"/lib"),
	    get_files(vpath+"/bin"));

  werror("Creating "+vpath+"-indigo.tar.gz:\n");
  object o=Stdio.File();
  spawn("tar cvf - "+files*" ",0,o->pipe(Stdio.PROP_IPC));
  spawn("gzip -9",o,Stdio.File("pike/"+vpath+"-indigo.tar.gz","wct"))->wait();
  rm(vpath);
  werror("Done.\n");

  if(cvs)
  {
    cvs->wait();
    bump_version();
  }
  return 0;
}
