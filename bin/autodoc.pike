/*
 * $Id: autodoc.pike,v 1.6 2001/03/23 13:09:19 norlin Exp $
 *
 * AutoDoc mk II extraction script.
 *
 * Henrik Grubbström 2001-01-08
 */

int main(int argc, array(string) argv)
{
  foreach(argv[1..], string path) {
    object st = file_stat(path);

    werror("Extracting from %O...\n", path);

    if (!st) {
      werror("File not found: %O\n", path);
      exit(1);
    }

    mixed err = catch {
      if (st->isdir) {
  	// Recurse.
      } else if (st->isreg) {
  	string raw = Stdio.read_bytes(path);

  	Tools.AutoDoc.PikeObjects.Module info;
	if (has_value(raw, "/*!") || has_value(raw, "//!")) {
  	  if (has_suffix(path, ".c") || has_suffix(path, ".h")) {
  	    info = Tools.AutoDoc.CExtractor.extract(raw, path);
  	  } else if (has_suffix(path, ".cmod")) {
  	    info = Tools.AutoDoc.PikeExtractor.extractModule(raw, path);
  	  } else if (has_suffix(path, ".pike") || has_suffix(path, ".pmod") ||
  		     has_suffix(path, ".pmod.in")) {
  	    info = Tools.AutoDoc.PikeExtractor.extractModule(raw, path);
  	  } else {
            werror("Unknown filetype %O\n", path);
  	    exit(1);
  	  }
	}
	write(info?sprintf("%s\n",info->xml()):
	      "<module name=''><modifiers/></module>\n");
      } else {
        werror("%O is not a plain file or directory.\n", path);
  	exit(1);
      }
    };
    if (err) {
      if (_typeof(err) <= Tools.AutoDoc.AutoDocError)
        werror("%O\n", err);
      else
	throw(err);
      exit(1);
    }
  }
}
