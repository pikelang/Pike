/*
 * $Id: extract.pike,v 1.2 2001/09/17 23:21:08 nilsson Exp $
 *
 * AutoDoc mk II extraction script.
 *
 * Henrik Grubbström 2001-01-08
 */

class MirarDoc
{
  inherit "mirardoc.pike";
}

int main(int argc, array(string) argv)
{
  argv = argv[1..];

  int rootless;
  if(has_value(argv, "--rootless")) {
    rootless = 1;
    argv -= ({ "--rootless" });
  }

  if(!sizeof(argv))
    error("Not enough arguments to 'extract' (No source).\n");
  string path = argv[0];

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
      int i;

      if (has_value(raw, "**!") ||
	  (((i = search(raw, "//! ""module ")) != -1) &&
	   (sizeof(array_sscanf(raw[i+11..],"%s\n%*s")[0]/" ") == 1))) {
	// Mirar-style markup.
	// FIXME: Better heuristic for detecting //!-style MirarDoc...
	object mirar_parser = MirarDoc();
	int lineno = 1;
	foreach(raw/"\n", string line) {
	  mirar_parser->process_line(line, path, lineno++);
	}
	if(sizeof(argv)<2)
	  error("Not enough arguments to 'extract' (No imgdest).\n");
	mirar_parser->make_doc_files( argv[1] );
      } else if (has_value(raw, "/*!") || has_value(raw, "//!")) {
	Tools.AutoDoc.PikeObjects.Module info;
	array(string) segments = path/"/";

	int module_path_fixed;
	string name;

	if (has_suffix(path, ".c") || has_suffix(path, ".h")) {
	  info = Tools.AutoDoc.CExtractor.extract(raw, path);
	  module_path_fixed = 1;

	} else if (has_suffix(path, ".pmod")) {
	  if (segments[-1] == "module.pmod") {
	    // Handling of Foo.pmod/module.pmod.
	    segments = segments[..sizeof(segments)-2];
	  }
	  if (has_suffix(segments[-1], ".pmod")) {
	    name = segments[-1][..sizeof(segments[-1])-6];
	  } else {
	    // Not likely to occurr, but...
	    name = segments[-1];
	  }
	  info = Tools.AutoDoc.PikeExtractor.extractModule(raw, path, name);

	} else if (has_suffix(path, ".pike")) {
	  if (!rootless && segments[-1] == "module.pike") {
	    // Handling of Foo.pmod/module.pike.
	    segments = segments[..sizeof(segments)-2];
	  }
	  // Note: The below works for both .pike and .pmod.
	  name = segments[-1][..sizeof(segments[-1])-6];

	  info = Tools.AutoDoc.PikeExtractor.extractClass(raw, path, name);
	  if(rootless) {
	    Tools.AutoDoc.PikeObjects.Module m =
	      Tools.AutoDoc.PikeObjects.Module();
	    m->children = ({ info });
	    m->name = "";
	    info = m;
	  }
	} else if (has_suffix(path, ".pmod.in")) {
	  if (segments[-1] == "module.pmod.in") {
	    // Handling of Foo.pmod/module.pmod.
	    segments = segments[..sizeof(segments)-2];
	  }
	  if (has_suffix(segments[-1], ".pmod")) {
	    name = segments[-1][..sizeof(segments[-1])-6];
	  } else {
	    // This happens rather frequently...
	    name = segments[-1];
	  }
	  raw = replace(raw, "@module@", sprintf("%O", "___"+name));
	  info = Tools.AutoDoc.PikeExtractor.extractModule(raw, path, name);

	} else if (segments[-1] == "master.pike.in") {
	  module_path_fixed = 1;
	  info = Tools.AutoDoc.PikeExtractor.extractClass(raw, path, "/master");
	  Tools.AutoDoc.PikeObjects.Module m =
	    Tools.AutoDoc.PikeObjects.Module();
	  m->children = ({ info });
	  m->name = "";
	  info = m;

	} else {
	  werror("Unknown filetype %O\n", path);
	  exit(1);
	}

	if (!module_path_fixed && info) {
	  for(int i = sizeof(segments)-1; i-- > 0;) {
	    if (name = Stdio.read_bytes(segments[..i]*"/" + "/.autodoc")) {
	      segments = replace(name, ({ "\n", "\r" }), ({ "", "" }))/"." +
		map(segments[i+1..sizeof(segments)-2],
		    lambda(string p) {
		      if (has_suffix(p, ".pmod")) {
			return p[..sizeof(p)-6];
		      }
		      // Usually not reached.
		      return p;
		    });
	      foreach(reverse(segments) + ({ "" }), string seg) {
		if (info->name == "") {
		  // Probably doesn't happen, but...
		  info->name = seg;
		} else {
		  Tools.AutoDoc.PikeObjects.Module m =
		    Tools.AutoDoc.PikeObjects.Module();
		  m->children = ({ info });
		  m->name = seg;
		  info = m;
		}
	      }
	      break;
	    }
	  }
	}
	if(info) write( info->xml()+"\n" );
      }
    } else {
      werror("%O is not a plain file or directory.\n", path);
      exit(1);
    }
  };
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
