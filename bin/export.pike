#!/usr/local/bin/pike

/* $Id: export.pike,v 1.27 1999/09/06 11:01:45 hubbe Exp $ */

#include <simulate.h>
import Stdio;

multiset except_modules  =(<>);
string vpath;


string dirname(string dir)
{
  array tmp=dir/"/";
  if(tmp[-1]=="") tmp=tmp[..sizeof(tmp)-2];
  tmp=tmp[..sizeof(tmp)-2];
  if(!sizeof(tmp)) return "/";
  return tmp*"/";
}

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
//  werror("FNORD:%O\n",getcwd());
  string s=Stdio.read_file(pike_base_name+"/src/version.h");

  int maj, min, build;

  if ((!sscanf(s, "%*sPIKE_MAJOR_VERSION %d", maj)) ||
      (!sscanf(s, "%*sPIKE_MINOR_VERSION %d", min)) ||
      (!sscanf(s, "%*sPIKE_BUILD_VERSION %d", build))) {

    werror("Failed to get Pike version.\n");
    exit(1);
  }
  return sprintf("Pike v%d.%d release %d", maj, min, build);
}

void bump_version()
{
  werror("Bumping release number.\n");
  Process.create_process(({ "cvs", "update", "version.h" }),
			 ([ "cwd":pike_base_name+"/src" ]))->wait();

  string s=Stdio.read_file(pike_base_name+"/src/version.h");
  sscanf(s,"%s PIKE_BUILD_VERSION %d%s",string pre, int rel, string post);
  rel++;
  Stdio.File(pike_base_name+"/src/version.h", "wct")->
    write(pre+" PIKE_BUILD_VERSION "+rel+post);
  Process.create_process(({ "cvs", "commit", "-m",
			    "release number bumped to "+rel+" by export.pike",
			    "version.h" }),
			 ([ "cwd":pike_base_name+"/src" ]))->wait();
}

string pike_base_name;
string srcdir;
int rebuild;

int main(int argc, string *argv)
{
  mixed tmp;
  int e;
  string *files;
  object cvs;


  foreach(Getopt.find_all_options(argv,aggregate(
    ({ "srcdir", "--srcdir", Getopt.HAS_ARG }),
    ({ "rebuild","--rebuild", Getopt.NO_ARG }),
    )),array opt)
    {
      switch(opt[0])
      {
	case "srcdir":
	  srcdir=opt[1];
	  if(basename(srcdir)=="src")
	    srcdir=dirname(srcdir);
	  pike_base_name=basename(srcdir);
	  cd(dirname(srcdir));
	  break;

	case "rebuild":
	  rebuild=1;
      }
    }
      
  argv=Getopt.get_args(argv);

  if(!srcdir)
  {
    tmp=reverse(argv[0]/"/");
    except_modules=mklist(argv[1..]);
    e=search(tmp,"pike");
    if(e==-1)
    {
      werror("Couldn't find Pike source dir.\n");
      werror("Use export.pike --srcdir=<dir> <except modules>.\n");
      exit(1);
    }
    tmp=reverse(tmp[e+1..]);
    cd(sizeof(tmp)<1 ? tmp*"/" : "/");
    werror("Sourcedir = "+tmp*"/"+"/pike\n");
    pike_base_name="pike";
  }

  if(rebuild)
  {
    werror("Not yet finished!\n");
    exit(1);
    object autoconfig=Process.create_process(({"./run_autoconfig"}),
					     (["cwd":pike_base_name]));
    /* make depend... */
    /* And other things... */
  }

  if(file_stat(pike_base_name+"/CVS"))
  {
    bump_version();

    vpath=replace(replace(getversion()," ","-"),"-release-",".");
    string tag=replace(vpath,({"Pike-","."}),({"","_"}));

    werror("Creating tag "+tag+" in the background.\n");
    cvs=Process.create_process(({"cvs","tag","-R","-F",tag}),
			   (["cwd":pike_base_name]));
  }else{
    vpath=replace(replace(getversion()," ","-"),"-release-",".");
  }

  fix_configure(pike_base_name+"/src");

  foreach(get_dir(pike_base_name+"/src/modules") - ({"CVS","RCS"}), tmp)
    if(file_size(pike_base_name+"/src/modules/"+tmp) == -2)
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
  spawn("gzip -9",o,Stdio.File(pike_base_name+"/"+vpath+"-indigo.tar.gz","wct"))->wait();
  rm(vpath);
  werror("Done.\n");

  if(cvs)
  {
    cvs->wait();
    bump_version();
  }
  return 0;
}
