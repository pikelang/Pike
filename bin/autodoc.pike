/*
 * $Id: autodoc.pike,v 1.5 2001/03/05 19:27:38 grubba Exp $
 *
 * AutoDoc mk II extraction script.
 *
 * Henrik Grubbström 2001-01-08
 */

int main(int argc, array(string) argv)
{
  foreach(argv[1..], string path) {
    object st = file_stat(path);

    werror(sprintf("Extracting from %O...\n", path));

    if (!st) {
      werror(sprintf("File not found: %O\n", path));
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
  	    werror(sprintf("Unknown filetype %O\n", path));
  	    exit(1);
  	  }
	}
	write(info?sprintf("%s\n",info->xml()):
	      "<module name=''><modifiers/></module>\n");
      } else {
  	werror(sprintf("%O is not a plain file or directory.\n", path));
  	exit(1);
      }
    };
    if (err) {
      if (intp(err[1])) {
	werror(sprintf("%s:%d: %s\n", path, err[1], (string)err[0]));
      }
      else if (objectp(err[1])) {
	werror(sprintf("%s:%d..%d:\n"
		       "\t%s\n",
		       path, err[1]->firstline, err[1]->lastline,
		       (string)err[0]));
      }
      else
	throw(err);
      exit(1);
    }
  }
}
