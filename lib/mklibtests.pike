//
// Creates testsuit files form the testsuit.in files
// found in the lib directory.
// $Id: mklibtests.pike,v 1.3 2002/08/03 13:46:27 nilsson Exp $
//

string src_dir;
string dest_dir;
string bin_dir;

mapping(string:string) make_vars;

void recurse(string path) {
  foreach(get_dir(src_dir+path), string fn) {
    Stdio.Stat ls = file_stat(src_dir+path+fn);
    Stdio.Stat ds = file_stat(dest_dir+path+fn);

    if(ls->isdir) {
      if(!ds) {
	if(!Stdio.mkdirhier(dest_dir+path+fn)) {
	  werror("Could not create %s\n", dest_dir+path+fn);
	  continue;
	}
      }

      recurse(path+fn+"/");
      continue;
    }

    if(fn!="testsuite.in") continue;

    ds = file_stat(dest_dir+path+"testsuite");
    if(!ds || ds->mtime<ls->mtime) {
      object f;
      if(catch(f=Stdio.File(dest_dir+path+"testsuite", "cwt"))) {
	werror("Could not create %s\n", dest_dir+path+"testsuite");
	continue;
      }
      Process.create_process( ({ combine_path(bin_dir,
					      "mktestsuite"),
				 src_dir+path+fn }),
			      ([ "stdout":f ]) )->wait();
      write("%stestsuite updated.\n", path);
    }
    else
      write("%stestsuite already up to date.\n", path);

  }
}

void init_make_vars() {
  string s = getenv()->MAKEFLAGS;
  if(!s) {
    make_vars = ([]);
    return;
  }

  array a = s/" ";
  a = Array.filter(a, has_value, "=");
  a = a[*]/"=";
  make_vars = mkmapping( @Array.transpose(a) );
}

int main(int n, array(string) args) {

  //  init_make_vars();

  foreach(args, string arg) {
    sscanf(arg, "--srcdir=%s", src_dir);
    sscanf(arg, "--destdir=%s", dest_dir);
    sscanf(arg, "--bindir=%s", bin_dir);
  }

  if(!src_dir) {
    werror("No source directory selected.\n");
    return 1;
  }

  if(!dest_dir) {
    werror("No destination directory selected.\n");
    return 1;
  }

  if(!bin_dir) {
    werror("No binary directory selected.\n");
    return 1;
  }

  if(src_dir[-1]!='/') src_dir+="/";
  if(dest_dir[-1]!='/') dest_dir+="/";
  if(bin_dir[-1]!='/') bin_dir+="/";

  recurse("");

  return 0;
}
