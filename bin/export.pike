#!/usr/local/bin/pike

#include <simulate.h>

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

    tmp=path+"/"+tmp;
    switch(tmp)
    {
      case "pike/src/modules/image":
      case "pike/src/modules/spider":
      case "pike/src/modules/pipe":
	continue;
    }
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
  string s=replace(version()," ","_");

  tmp=explode(argv[0],"/");
  tmp=reverse(tmp);
  e=search(tmp,"pike");
  if(e==-1)
  {
    perror("Couldn't find Pike source dir.\n");
    perror("Use export.pike <sourcedir>.\n");
    exit(1);
  }
  tmp=tmp[e+1..sizeof(tmp)-1];
  tmp=reverse(tmp);
  cd(tmp*"/");
  perror("Sourcedir = "+tmp*"/"+"/pike\n");

  fix_configure("pike/src");

  foreach(get_dir("pike/src/modules") - ({"CVS","RCS"}), tmp)
    if(file_size("pike/src/modules/"+tmp) == -2)
      fix_configure("modules/"+tmp);

  files=sum(({ "pike/README" }),
	    get_files("pike/src"),
	    get_files("pike/doc"),
	    get_files("pike/lib"),
	    get_files("pike/bin"));

  perror("Creating "+s+".tar.gz:\n");
  system("tar cvzf pike/"+s+".tar.gz "+files*" ");
  perror("Done.\n");
  return 0;
}
