/*
 * $Id: extract.pike,v 1.6 2001/10/26 19:18:58 nilsson Exp $
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

int main(int n, array(string) args) {
  args = args[1..];

  int rootless;
  if(has_value(args, "--rootless")) {
    rootless = 1;
    args -= ({ "--rootless" });
  }

  if(!sizeof(args))
    error("No source file argument given.\n");
  string filename = args[0];

  if(sizeof(args)<2)
    error("Not enough arguments to 'extract' (No imgdest).\n");
  string imgdest = args[1];

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
    mirar_parser->make_doc_files( imgdest );
    return 0;
  }

  string suffix;
  if(!has_value(filename, "."))
    error("No suffix in file %O.\n", filename);
  sscanf((filename/"/")[-1], "%*s.%s", suffix);
  if( !(< "c", "pike", "pike.in", "pmod", "pmod.in" >)[suffix] )
    error("Unknown filetype %O.\n", suffix);

  string result;
  mixed err = catch {
    if( suffix == "c" )
      result = Tools.AutoDoc.ProcessXML.extractXML(filename);
    else {
      array(string) parents;
      if(catch(parents = rootless?({}):find_root(dirname(filename))) )
	parents = ({});

      string type = ([ "pike":"class",
		       "pike.in":"class",
		       "pmod":"module",
		       "pmod.in":"module" ])[suffix];

      string name = (filename/"/")[-1];
      if(name == "master.pike.in")
	name = "/master";
      else
	foreach( ({ "pike", "pike.in", "pmod", "pmod.in" }), string ext)
	  sscanf(name, "%s."+ext, name);
      if(name == "module") {
	if(!sizeof(parents))
	  error("Unknown module parent name.\n");
	name = parents[-1];
	parents = parents[..sizeof(parents)-2];
      }

      result = Tools.AutoDoc.ProcessXML.extractXML(filename, 1, type, name, parents);
    }
  };
  if(result) {
    write( Tools.AutoDoc.ProcessXML.moveImages(result, ".", imgdest) );
  }

  if (err) {
    if (arrayp(err) && _typeof(err[0]) <= Tools.AutoDoc.AutoDocError)
      werror("%O\n", err[0]);
    else if (objectp(err) && _typeof(err) <= Tools.AutoDoc.AutoDocError)
      werror("%O\n", err);
    else
      throw(err);
    exit(1);
  }

}
