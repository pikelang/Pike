/*
 * $Id: autodoc.pike,v 1.2 2001/01/11 20:31:44 grubba Exp $
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
  	if (has_suffix(path, ".c") || has_suffix(path, ".h")) {
  	  info = Tools.AutoDoc.CExtractor.extract(raw, path);
  	} else if (has_suffix(path, ".cmod")) {
  	  info = Tools.AutoDoc.PikeExtractor.extractModule(raw, path);
  	} else if (has_suffix(path, ".pike") || has_suffix(path, ".pmod")) {
  	  info = Tools.AutoDoc.PikeExtractor.extractModule(raw, path);
  	} else {
  	  werror(sprintf("Unknown filetype %O\n", path));
  	  exit(1);
  	}
	write(sprintf("%s\n", info));
      } else {
  	werror(sprintf("%O is not a plain file or directory.\n", path));
  	exit(1);
      }
    };
    if (err && intp(err[1])) {
      werror(sprintf("%s:%d: %s\n", path, err[1], (string)err[0]));
    }
  }
}
