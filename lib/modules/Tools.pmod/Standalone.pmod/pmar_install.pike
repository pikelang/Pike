
int DEBUG=1;

constant version = ("$Revision: 1.1 $"/" ")[1];
constant description = "Pike packaged module (PMAR) installer.";

int c;
int cc;

//!
//! a prototype installer for prepackaged modules
//!
//! note: portions of this code are highly inefficient (wrt tar filehandling).
//! we assume that this will be used infrequently enough that this is not
//! going to be a problem.
//!
//! a package file is a tar file that contains the following structure:
//!
//! ROOTDIR/
//!         METADATA.TXT
//!                a file containing metadata about the package
//!                format: KEY=value, where values include: 
//!                  PLATFORM, in the form of os/processor (either can be "any") 
//!                  MODULE, the name of the module, in Module.Submodule format.
//!                  VERSION, the version of this module.
//!         MODULE/
//!                any files that need to be installed in the module directory
//!         MODREF/ ???
//!                documentation suitable for inclusion in the modref
//!         INCLUDE/ ???
//!                any pike language include files to be installed
//!         SCRIPTS/
//!                preinstall.pike
//!                postinstall.pike

int main(int argc, array(string) argv)
{
  string s = Stdio.read_file(argv[1]);

  // we assume that the first entry in the package file is the directory
  // that contains the module to be installed.
  string fsroot = Filesystem.Tar(argv[1], 0, Stdio.FakeFile(s))->get_dir()[0];

  mapping sysinfo, metadata;
  object moduletool;
  string system_module_path;
  string system_doc_path;
  string system_include_path;

  sysinfo = get_sysinfo();
  metadata = get_package_metadata(s, fsroot);
  moduletool = Tools.Standalone.module();

  moduletool->load_specs(moduletool->include_path+"/specs");

  system_module_path = moduletool["system_module_path"];
//  system_include_path = moduletool["system_include_path"];
//  system_doc_path = moduletool["system_doc_path"];

  if(!verify_suitable_package(metadata, sysinfo))
  {
    werror("Package is not suitable for this system.\n");
    return 1;
  }

  if(has_dir(s, fsroot + "/MODULE"))
    untar(s, system_module_path, fsroot + "/MODULE");
//  if(has_dir(s, fsroot + "/INCLUDE"))
//    untar(s, system_include_path, fsroot + "/INCLUDE");
//  if(has_dir(s, fsroot + "/MODREF"))
//    untar(s, system_doc_path, fsroot + "/MODREF");

  return 0;
}

int has_dir(string s, string fsroot)
{
  object t;
  object stat;

  t=getfs(s, "/");
  string comp;

  mixed err = catch {

  foreach(fsroot / "/";; comp)
    t = t->cd(comp);
    if(!t) return 0;
  };

  if(err) return 0;
  
  t = t->cd("..");
  stat = t->stat(comp);

  if(!stat || !stat->isdir)
    return 0;

  return 1;
}

int verify_suitable_package(mapping metadata, mapping sysinfo)
{
  int osok, procok;
  string os, proc;
  string pos, pproc;

  [pos,pproc] = metadata->PLATFORM /"/";
  
  if(pos == "any") osok = 1;
  if(pproc == "any") procok = 1;

  if(lower_case(sysinfo->sysname) == pos) osok = 1;
  if(lower_case(sysinfo->architecture) == pproc) procok = 1;

  if(osok && procok) return 1;
  else return 0;
}

mapping get_sysinfo()
{
  return System.uname();
}

mapping get_package_metadata(string s, string fsroot)
{
  mapping metadata = ([]);

  object stat = getfs(s, fsroot)->stat("METADATA.TXT");

  if(!stat || !stat->isreg)
    throw(Error.Generic("missing METADATA.TXT in package!\n"));

  string f = getfs(s, fsroot)->open("METADATA.TXT", "r")->read();

  foreach((f/"\n") - ({""});; string line)
  {
    string k,v;
    array l;
    l=line/"=";
    k = String.trim_whites(l[0]);
    v = String.trim_whites(l[1]);
    metadata[k] = v;
  }

  return metadata;
}

static Filesystem.System getfs(string source, string cwd) {
  return Filesystem.Tar(sprintf("%s.tar", "d"), 0, Stdio.FakeFile(source))->cd(cwd);
}

int untar(string source, string path, void|string cwd) {
  if (!cwd)
    cwd = "/";
  object t = getfs(source, cwd);
  array files = t->get_dir();
  int c;
  foreach(sort(files), string fname) {
    // Get the actual filename
    fname = ((fname / "/") - ({""}))[-1];
    object stat = t->cd(cwd)->stat(fname);
    if (stat->isdir()) {
      string dir = Stdio.append_path(path, fname);
      c++;
      cc++;
      if (DEBUG)
        write(sprintf("%O [dir]\n", dir));
      //mkdir(dir);
      c += untar(source, dir, Stdio.append_path(cwd, fname));
    }
    else if (stat->isreg()) {
      string file = Stdio.append_path(path, fname);
      object f;
      if (mixed err = catch{
        if (DEBUG)
          write("%O [file %d bytes]\n", file, stat->size);
        //Stdio.write_file(file, t->cd(cwd)->open(fname, "r")->read());
      }) {
        werror("%O [error in tarfile!]\n\n", file);
        throw(err);
      }
      c++;
      cc++;
    }
    else {
      werror("Unknown file type for file %O\n", fname);
      continue;
    }
  }
  return c;
}

