#!/usr/local/bin/pike

/* $Id: export.pike,v 1.5 1997/05/31 22:03:36 grubba Exp $ */

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
      perror("Fixing configure in "+dir+".\n");
      system("cd "+dir+" ; autoconf");
    }
  }
}

int main(int argc, string *argv)
{
  mixed tmp;
  int e;
  string *files;
  vpath=replace(version()," ","-");

  tmp=reverse(argv[0]/"/");
  except_modules=mklist(argv[1..]);
  e=search(tmp,"pike");
  if(e==-1)
  {
    perror("Couldn't find Pike source dir.\n");
    perror("Use /full/path/export.pike <except modules>.\n");
    exit(1);
  }
  tmp=reverse(tmp[e+1..]);
  cd(tmp*"/");
  perror("Sourcedir = "+tmp*"/"+"/pike\n");

  fix_configure("pike/src");

  foreach(get_dir("pike/src/modules") - ({"CVS","RCS"}), tmp)
    if(file_size("pike/src/modules/"+tmp) == -2)
      fix_configure("modules/"+tmp);

  system("ln -s pike "+vpath);

  files=sum(({ vpath+"/README" }),
	    get_files(vpath+"/src"),
	    get_files(vpath+"/doc"),
	    get_files(vpath+"/lib"),
	    get_files(vpath+"/bin"));

  perror("Creating "+vpath+".tar.gz:\n");
  system("tar cvzf pike/"+vpath+".tar.gz "+files*" ");
  rm(vpath);
  perror("Done.\n");
  return 0;
}
