/*
 * $Id: autodoc.pike,v 1.10 2001/04/25 21:36:13 grubba Exp $
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
	  array(string) segments = path/"/";

	  int module_path_fixed;
	  string name;

  	  if (has_suffix(path, ".c") || has_suffix(path, ".h")) {
  	    info = Tools.AutoDoc.CExtractor.extract(raw, path);
	    module_path_fixed = 1;
  	  } else if (has_suffix(path, ".cmod")) {
	    name = segments[-1][..sizeof(segments[-1])-6];
  	    info = Tools.AutoDoc.PikeExtractor.extractModule(raw, path, name);
  	  } else if (has_suffix(path, ".pmod") ||
		     has_suffix(path, ".pmod.in")) {
	    if (segments[-1] == "module.pmod") {
	      // Handling of Foo.pmod/module.pmod.
	      segments = segments[..sizeof(segments)-2];
	    }
	    if (has_suffix(segments[-1], ".pmod")) {
	      name = segments[-1][..sizeof(segments[-1])-6];
	    } else {
	      name = segments[-1][..sizeof(segments[-1])-9];
	    }
  	    info = Tools.AutoDoc.PikeExtractor.extractModule(raw, path, name);
  	  } else if (has_suffix(path, ".pike")) {
	    if (segments[-1] == "module.pike") {
	      // Handling of Foo.pmod/module.pike.
	      segments = segments[..sizeof(segments)-2];
	    }
	    // Note: The below works for both .pike and .pmod.
	    name = segments[-1][..sizeof(segments[-1])-6];
  	    info = Tools.AutoDoc.PikeExtractor.extractClass(raw, path, name);
  	  } else {
            werror("Unknown filetype %O\n", path);
  	    exit(1);
  	  }

	  if (!module_path_fixed && info) {
	    for(int i = sizeof(segments)-1; i-- > 0;) {
	      if (name = Stdio.read_bytes(segments[..i]*"/" + "/.autodoc")) {
		segments = name/"." +
		  map(segments[i+1..sizeof(segments)-2],
		      lambda(string p) {
			if (has_suffix(p, ".pmod")) {
			  return p[..sizeof(p)-6];
			}
			// Usually not reached.
			return p;
		      });
		foreach(reverse(segments), string seg) {
		  Tools.AutoDoc.PikeObjects.Module m =
		    Tools.AutoDoc.PikeObjects.Module();
		  m->children = ({ info });
		  m->name = seg;
		  info = m;
		}
		break;
	      }
	    }
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
