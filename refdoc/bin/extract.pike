/*
 * $Id: extract.pike,v 1.14 2002/05/30 15:21:39 grubba Exp $
 *
 * AutoDoc mk II extraction script.
 *
 * Henrik Grubbström 2001-01-08
 */

class MirarDoc
{
  inherit "mirardoc.pike";
}

array(string) find_root(string path) {
  if(file_stat(path+"/.autodoc"))
    return reverse((Stdio.read_file(path+"/.autodoc")/"\n")[0]/" ") - ({""});
  array(string) parts = path/"/";
  string name = parts[-1];
  sscanf(name, "%s.pmod", name);
  return find_root(parts[..sizeof(parts)-2]*"/") +
    ({ (parts[-1]/".")[0] });
}

string imgdir;

int main(int n, array(string) args) {

  if(n!=4) {
    write("%s <srcdir> <builddir> <imgdir>\n", args[0]);
    exit(1);
  }

  string srcdir = args[1];
  string builddir = args[2];
  imgdir = args[3];
  if(srcdir[-1]!='/') srcdir += "/";
  if(builddir[-1]!='/') builddir += "/";
  if(imgdir[-1]!='/') imgdir += "/";
  recurse(srcdir, builddir);
}

void recurse(string srcdir, string builddir) {
  werror("Extracting from %s\n", srcdir);
  foreach(get_dir(srcdir), string fn) {
    if(fn=="CVS") continue;
    if(fn[0]=='.') continue;
    if(fn[-1]=='~') continue;
    if(fn[0]=='#' && fn[-1]=='#') continue;

    Stdio.Stat stat = file_stat(srcdir+fn, 1);

    if(stat->isdir) {
      if(!file_stat(builddir+fn)) mkdir(builddir+fn);
      recurse(srcdir+fn+"/", builddir+fn+"/");
      continue;
    }

    if(stat->size<1) continue;

    if(!has_suffix(fn, ".pike") && !has_suffix(fn, ".pike.in") &&
       !has_suffix(fn, ".pmod") && !has_suffix(fn, ".pmod.in") &&
       !has_suffix(fn, ".c")) continue;

    Stdio.Stat dstat = file_stat(builddir+fn+".xml");

    if(!dstat || dstat->mtime < stat->mtime) {
      string res = extract( srcdir+fn, imgdir, 0, builddir);
      if(!res) exit(1);
      Stdio.write_file(builddir+fn+".xml", res);
    }
  }
}

string extract(string filename, string imgdest, int(0..1) rootless, string builddir) {

  werror("Extracting file %O...\n", filename);
  string file = Stdio.read_file(filename);

  int i;
  if (has_value(file, "**!") ||
      (((i = search(file, "//! ""module ")) != -1) &&
       (sizeof(array_sscanf(file[i+11..],"%s\n%*s")[0]/" ") == 1))) {
    // Mirar-style markup.
    MirarDoc mirar_parser = MirarDoc();
    int lineno = 1;
    foreach(file/"\n", string line) {
      mirar_parser->process_line(line, filename, lineno++);
    }
    return mirar_parser->make_doc_files( builddir, imgdest );
  }

  string suffix;
  if (has_suffix(filename, ".in")) {
    filename = filename[..sizeof(filename)-4];
  }
  if(!has_value(filename, "."))
    error("No suffix in file %O.\n", filename);
  suffix = ((filename/"/")[-1]/".")[-1];
  if( !(< "c", "pike", "pmod", >)[suffix] )
    error("Unknown filetype %O.\n", suffix);

  string result;
  mixed err = catch {
    if( suffix == "c" )
      result = Tools.AutoDoc.ProcessXML.extractXML(filename);
    else {
      array(string) parents;
      if(catch(parents = rootless?({}):find_root(dirname(filename))) )
	parents = ({});

      string type = ([ "pike":"class", "pmod":"module", ])[suffix];

      string name = (filename/"/")[-1];
      if(name == "master.pike")
	name = "/master";
      else {
	name = name[..sizeof(name)-(sizeof(suffix)+2)];
      }
      if(name == "module" && !rootless) {
	if(!sizeof(parents))
	  error("Unknown module parent name.\n");
	name = parents[-1];
	parents = parents[..sizeof(parents)-2];
      }

      result = Tools.AutoDoc.ProcessXML.extractXML(filename, 1, type, name, parents);
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

  if(result && sizeof(result))
    return Tools.AutoDoc.ProcessXML.moveImages(result, builddir, imgdest);

  return "\n";
}
