/*
 * $Id: extract_autodoc.pike,v 1.44 2004/04/23 13:41:10 per Exp $
 *
 * AutoDoc mk II extraction script.
 *
 * Henrik Grubbström 2001-01-08
 */

#pike __REAL_VERSION__

constant description = "Extracts autodoc from Pike or C code.";

string imgsrc;
string imgdir;

int verbosity = 2;

int main(int n, array(string) args) {

  string srcdir, builddir = "./";
  array(string) root = ({"predef::"});

  foreach(Getopt.find_all_options(args, ({
    ({ "srcdir",     Getopt.HAS_ARG,      "--srcdir" }),
    ({ "imgsrc",     Getopt.HAS_ARG,      "--imgsrc" }),
    ({ "builddir",   Getopt.HAS_ARG,      "--builddir" }),
    ({ "imgdir",     Getopt.NO_ARG,       "--imgdir" }),
    ({ "root",       Getopt.HAS_ARG,      "--root" }),
    ({ "quiet",      Getopt.NO_ARG,       "-q,--quiet"/"," }),
    ({ "help",       Getopt.NO_ARG,       "-h,--help"/"," }) })), array opt)
    switch(opt[0])
    {
    case "srcdir":
      srcdir = combine_path(getcwd(), opt[1]);
      if(srcdir[-1]!='/') srcdir += "/";
      break;
    case "imgsrc":
      imgsrc = combine_path(getcwd(), opt[1]);
      break;
    case "builddir":
      builddir = combine_path(getcwd(), opt[1]);
      if(builddir[-1]!='/') builddir += "/";
      break;
    case "imgdir":
      imgdir = combine_path(getcwd(), opt[1]);
      if(imgdir[-1]!='/') imgdir += "/";
      break;
    case "root":
      root = opt[1]/".";
      break;
    case "quiet":
      verbosity = 0;
      break;
    case "help":
      werror("Usage:\n"
	     "\tpike -x extract_autodoc [-q] --srcdir=<srcdir> \n"
	     "\t     [--imgsrcdir=<imgsrcdir>] [--builddir=<builddir>]\n"
	     "\t     [--imgdir=<imgdir>] [--root=<module>] [file1 [... filen]]\n");
      return 0;
    }

  args = args[1..] - ({0});

  if(srcdir)
    recurse(srcdir, builddir, 0, root);
  else if(sizeof(args)) {
    foreach(args, string fn)
    {
      Stdio.Stat stat = file_stat(fn);
      Stdio.Stat dstat = file_stat(builddir+fn+".xml");

      // Build the xml file if it doesn't exist, if it is older than the
      // source file, or if the root has changed since the previous build.
      if(!dstat || dstat->mtime < stat->mtime) {
        string res = extract(fn, imgdir, builddir, root);

        if(!res) exit(1);
        Stdio.write_file(builddir+fn+".xml", res);
        }
    }
  }
  else {
    werror("No source directory or input files given.\n");
    return 1;
  }
}

void recurse(string srcdir, string builddir, int root_ts, array(string) root)
{
  if (verbosity > 0)
    werror("Extracting from %s\n", srcdir);

  Stdio.Stat st;
  if(st = file_stat(srcdir+"/.autodoc")) {
    // Note .autodoc files are space-separated to allow for namespaces like
    //      "7.0::".
    root = (Stdio.read_file(srcdir+"/.autodoc")/"\n")[0]/" " - ({""});
    if (!sizeof(root) || !has_suffix(root[0], "::")) {
      // The default namespace is predef::
      root = ({ "predef::" }) + root;
    }
    root_ts = st->mtime;
  }

  if(!file_stat(srcdir)) {
    werror("Could not find directory %O.\n", srcdir);
    return;
  }

  // do not recurse into the build dir directory to avoid infinite loop
  // by building the autodoc of the autodoc and so on
  if(search(builddir, srcdir) == -1)
    foreach(get_dir(srcdir), string fn) {
      if(fn=="CVS") continue;
      if(fn[0]=='.') continue;
      if(fn[-1]=='~') continue;
      if(fn[0]=='#' && fn[-1]=='#') continue;

      Stdio.Stat stat = file_stat(srcdir+fn, 1);

      if (!stat) continue;

      if(stat->isdir) {
	if(!file_stat(builddir+fn)) mkdir(builddir+fn);
	string mod_name = fn;
	sscanf(mod_name, "%s.pmod", mod_name);
	recurse(srcdir+fn+"/", builddir+fn+"/", root_ts, root + ({mod_name}));
	continue;
      }

      if(stat->size<1) continue;

      if(!has_suffix(fn, ".pike") && !has_suffix(fn, ".pike.in") &&
	 !has_suffix(fn, ".pmod") && !has_suffix(fn, ".pmod.in") &&
	 //       !has_suffix(fn, ".cmod") && !has_suffix(fn, ".cmod.in") &&
	 !has_suffix(fn, ".c")) continue;

      Stdio.Stat dstat = file_stat(builddir+fn+".xml");

      // Build the xml file if it doesn't exist, if it is older than the
      // source file, or if the root has changed since the previous build.
      if(!dstat || dstat->mtime < stat->mtime || dstat->mtime < root_ts) {
	string res = extract(srcdir+fn, imgdir, builddir, root);
	if(!res) exit(1);
	Stdio.write_file(builddir+fn+".xml", res);
      }
    }
}

string extract(string filename, string imgdest,
	       string builddir, array(string) root)
{
  if (verbosity > 1)
    werror("Extracting file %O...\n", filename);
  string file = Stdio.read_file(filename);

  if (!file) {
    werror("WARNING: Failed to read file %O!\n", filename);
    return "\n";
  }

  int i;
  if (has_value(file, "**""!") ||
      (((i = search(file, "//! ""module ")) != -1) &&
       (sizeof(array_sscanf(file[i+11..],"%s\n%*s")[0]/" ") == 1))) {
    // Mirar-style markup.
    if(imgsrc && imgdest) {
      Tools.AutoDoc.MirarDocParser mirar_parser =
	Tools.AutoDoc.MirarDocParser(imgsrc, !verbosity);
      int lineno = 1;
      foreach(file/"\n", string line) {
	mirar_parser->process_line(line, filename, lineno++);
      }
      return mirar_parser->make_doc_files(builddir, imgdest, root[0]);
    }
    else
      error("Found Mirar style markup. Need imgsrc and imgdir.\n");
  }

  string name_sans_suffix, suffix;
  if (has_suffix(filename, ".in")) {
    name_sans_suffix = filename[..sizeof(filename)-4];
  }
  else
    name_sans_suffix = filename;
  if(!has_value(name_sans_suffix, "."))
    error("No suffix in file %O.\n", name_sans_suffix);
  suffix = ((name_sans_suffix/"/")[-1]/".")[-1];
  if( !(< "c", "cpp",/* "cmod", */ "pike", "pmod", >)[suffix] )
    error("Unknown filetype %O.\n", suffix);
  name_sans_suffix =
    name_sans_suffix[..sizeof(name_sans_suffix)-(sizeof(suffix)+2)];

  string result;
  mixed err = catch {
    if( suffix == "c" || suffix == "cpp" /* || suffix == "cmod" */)
      result = Tools.AutoDoc.ProcessXML.extractXML(filename,0,0,0,root);
    else {
      string type = ([ "pike":"class", "pmod":"module", ])[suffix];
      string name = (name_sans_suffix/"/")[-1];
#if 0
      werror("root: %{%O, %}\n"
	     "type: %O\n"
	     "name: %O\n", root, type, name);
#endif /* 0 */
      if(name == "master.pike")
	name = "/master";
      if(name == "module" && (filename/"/")[-1] != "module.pike") {
	if(sizeof(root)<2)
	  error("Unknown module parent name.\n");
	name = root[-1];
	root = root[..sizeof(root)-2];
      } else if ((name == "__default") && (sizeof(root) == 1)) {
	// Pike backward compatibility module.
	name = root[0][..sizeof(root[0])-3];
	root = ({});
	type = "namespace";
      }

      result =
	Tools.AutoDoc.ProcessXML.extractXML(filename, 1, type, name, root);
    }
  };

  if (err) {
    if (arrayp(err) && _typeof(err[0]) <= Tools.AutoDoc.AutoDocError)
      werror("%O\n", err[0]);
    else if (objectp(err) && _typeof(err) <= Tools.AutoDoc.AutoDocError)
      werror("%O\n", err);
    else
      werror("%s\n", describe_backtrace(err));

    return 0;
  }

  if(!result) result="";

  if(sizeof(result) && imgdest)
    result = Tools.AutoDoc.ProcessXML.moveImages(result, builddir,
						 imgdest, !verbosity);
  return result+"\n";
}
