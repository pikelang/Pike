#!/usr/local/bin/pike

/* $Id: export.pike,v 1.12 1997/12/22 18:42:33 hubbe Exp $ */

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
      system("cd "+dir+" ; autoconf");
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


int main(int argc, string *argv)
{
  mixed tmp;
  int e;
  string *files;

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

  vpath=replace(getversion()," ","-");

  if(file_stat("pike/CVS"))
  {
    werror("Bumping release number.\n");
    string s=Stdio.read_file("pike/src/version.c");
    sscanf(s,"%s release %d%s",string pre, int rel, string post);
    rel++;
    Stdio.File("pike/src/version.c","wct")->write(pre+" release "+rel+post);
    system("cd pike/src ; cvs commit -m 'release number bumped by export.pike' version.c");

    string tag=replace(vpath,({"Pike-","."}),({"","_"}));
#if 0
    mapping x=localtime(time());
    tag+=+"-"+sprintf("%02d%02d%02d-%02d%02d",
		      x->year,
		      x->mon+1,
		      x->mday,
		      x->hour,
		      x->min);
#else
    string tag+="-rel"+rel;
#endif

    werror("Creating tag "+tag+" in the background.\n");
    system("cd pike ; cvs tag -R -F "+tag+"&");
  }

  fix_configure("pike/src");

  foreach(get_dir("pike/src/modules") - ({"CVS","RCS"}), tmp)
    if(file_size("pike/src/modules/"+tmp) == -2)
      fix_configure("modules/"+tmp);

  system("ln -s pike "+vpath);

  files=sum(({ vpath+"/README", vpath+"/ANNOUNCE" }),
	    get_files(vpath+"/src"),
	    get_files(vpath+"/lib"),
	    get_files(vpath+"/bin"));

  werror("Creating "+vpath+".tar.gz:\n");
  system("tar cvf - "+files*" "+" | gzip -9 >pike/"+vpath+".tar.gz");
  rm(vpath);
  werror("Done.\n");
  return 0;
}
