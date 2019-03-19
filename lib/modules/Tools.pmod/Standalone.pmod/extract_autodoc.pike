/*
 * AutoDoc mk II extraction script.
 *
 * Henrik Grubbström 2001-01-08
 */

#pike __REAL_VERSION__

constant description = "Extracts autodoc from Pike or C code.";

string imgsrc;
string imgdir;

Tools.AutoDoc.Flags flags = Tools.AutoDoc.FLAG_NORMAL;

int verbosity = Tools.AutoDoc.FLAG_NORMAL;

int source_timestamp;

int num_updated_files;

// See the BMML rewriter further below about these values.
// Not that there is both a start and an end sentinel.
constant bmml_invalidation_times = ({
  0, 855275859, 855644314, 855648534, 855725630, 857038037, 0x7fffffff,
});

int bmml_invalidate_before;
int bmml_invalidate_after;

int main(int n, array(string) args)
{
  string srcdir, builddir = "./";
  array(string) root = ({"predef::"});

  int return_count;

  foreach(Getopt.find_all_options(args, ({
    ({ "srcdir",     Getopt.HAS_ARG,      "--srcdir" }),
    ({ "imgsrc",     Getopt.HAS_ARG,      "--imgsrc" }),
    ({ "builddir",   Getopt.HAS_ARG,      "--builddir" }),
    ({ "imgdir",     Getopt.NO_ARG,       "--imgdir" }),
    ({ "root",       Getopt.HAS_ARG,      "--root" }),
    ({ "compat",     Getopt.NO_ARG,       "--compat" }),
    ({ "count",      Getopt.NO_ARG,       "--count" }),
    ({ "timestamp",  Getopt.HAS_ARG,      "--source-timestamp" }),
    ({ "no-dynamic", Getopt.NO_ARG,       "--no-dynamic" }),
    ({ "keep-going", Getopt.NO_ARG,       "--keep-going" }),
    ({ "verbose",    Getopt.NO_ARG,       "-v,--verbose"/"," }),
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
    case "compat":
      flags |= Tools.AutoDoc.FLAG_COMPAT;
      break;
    case "count":
      return_count = 1;
      break;
    case "no-dynamic":
      flags |= Tools.AutoDoc.FLAG_NO_DYNAMIC;
      break;
    case "keep-going":
      flags |= Tools.AutoDoc.FLAG_KEEP_GOING;
      break;
    case "root":
      root = opt[1]/".";
      break;
    case "quiet":
      flags = (flags & ~Tools.AutoDoc.FLAG_VERB_MASK)|Tools.AutoDoc.FLAG_QUIET;
      break;
    case "verbose":
      verbosity = flags & Tools.AutoDoc.FLAG_VERB_MASK;
      if (verbosity < Tools.AutoDoc.FLAG_DEBUG) verbosity++;
      flags = (flags & ~Tools.AutoDoc.FLAG_VERB_MASK)|verbosity;
      break;
    case "timestamp":
      // This is currently only used by the BMML handler.
      // It's used to convert /precompiled/foo module-references
      // to their corresponding module name as of this timestamp.
      source_timestamp = (int)opt[1];

      for (int i = sizeof(bmml_invalidation_times); i--;) {
	if (bmml_invalidation_times[i] < source_timestamp) {
	  bmml_invalidate_before = bmml_invalidation_times[i];
	  bmml_invalidate_after = bmml_invalidation_times[i+1];
	  break;
	}
      }
      break;
    case "help":
      werror("Usage:\n"
	     "\tpike -x extract_autodoc [-q|--quiet] [-v|--verbose]\n"
	     "\t     [--compat] [--no-dynamic] [--keep-going]\n"
	     "\t     --srcdir=<srcdir>\n"
	     "\t     [--imgsrcdir=<imgsrcdir>] [--builddir=<builddir>]\n"
	     "\t     [--imgdir=<imgdir>] [--root=<module>]\n"
	     "\t     [file1 [... filen]]\n");
      return 0;
    }

  verbosity = flags & Tools.AutoDoc.FLAG_VERB_MASK;

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
      if(!dstat || dstat->mtime <= stat->mtime) {
        string res = extract(fn, imgdir, builddir, root);

        if(!res) {
	  if (flags & Tools.AutoDoc.FLAG_KEEP_GOING) continue;
	  exit(1);
	}

	num_updated_files++;

	if (sizeof(res) && (res != "\n")) {
	  // Validate the extracted XML.
	  mixed err = catch {
	      Parser.XML.Tree.simple_parse_input(res);
	    };
	  if (err) {
	    werror("Extractor generated broken XML for file %s:\n"
		   "%s",
		   builddir + fn + ".xml", describe_error(err));
	    rm(builddir+fn+".xml");
	    Stdio.write_file(builddir+fn+".brokenxml", res);
	    Stdio.write_file(builddir+fn+".xml.stamp",
			     (string)source_timestamp);
	    werror("Result saved as %s.\n", builddir + fn + ".brokenxml");
	    if (flags & Tools.AutoDoc.FLAG_KEEP_GOING) continue;
	    exit(1);
	  }
	}

        Stdio.write_file(builddir+fn+".xml", res);
	Stdio.write_file(builddir+fn+".xml.stamp", (string)source_timestamp);
      }
    }
  }
  else {
    werror("No source directory or input files given.\n");
    return 1;
  }

  if (return_count) return num_updated_files;
  return 0;
}

void recurse(string srcdir, string builddir, int root_ts, array(string) root)
{
  if (verbosity > 1)
    werror("Extracting from %s\n", srcdir);

  Stdio.Stat st;
  if(file_stat(srcdir+"/.noautodoc"))
    return;

  if(st = file_stat(srcdir+"/.autodoc")) {
    // Note .autodoc files are space-separated to allow for namespaces like
    //      "7.0::".
    root = (Stdio.read_file(srcdir+"/.autodoc")/"\n")[0]/" " - ({""});
    if (!sizeof(root) || !has_suffix(root[0], "::")) {
      if (sizeof(root) && has_value(root[0], "::")) {
	// Broken .autodoc file
	werror("Invalid syntax in %s.\n"
	       ":: Must be last in the token.\n",
	       srcdir + "/.autodoc");
	if (!flags & Tools.AutoDoc.FLAG_COMPAT) {
	  error("Invalid syntax in .autodoc file.\n");
	}
	array(string) a = root[0]/"::";
	root = ({ a[0] + "::" }) + a[1..] + root[1..];
      } else {
	// The default namespace is predef::
	root = ({ "predef::" }) + root;
      }
    }
    root_ts = st->mtime;
  } else if (st = file_stat(srcdir+"/.bmmlrc")) {
    if (Stdio.read_file(srcdir+"/.bmmlrc") == "prefix internal\n") {
      root = ({ "c::" });
    }
    root_ts = st->mtime;
  }

  foreach(get_dir(builddir), string fn) {
    if ((fn != ".cache.xml") && has_suffix(fn, ".xml")) {
      if (!Stdio.is_file(srcdir + fn[..<4])) {
	if (verbosity > 0)
	  werror("The file %O is no more.\n", srcdir + fn[..<4]);

	num_updated_files++;
	rm(builddir + fn);
	rm(builddir + fn[..<4] + ".brokenxml");
	rm(builddir + fn + ".stamp");
	rm(builddir + ".cache.xml.stamp");
      } else if (source_timestamp && (source_timestamp < 950000000) &&
		 (sizeof(fn/".") == 2)) {
	// BMML.
	int old_ts = (int)Stdio.read_bytes(builddir + fn + ".stamp");
	if ((old_ts < bmml_invalidate_before) ||
	    (old_ts >= bmml_invalidate_after)) {
	  // BMML file that may have changed at this time.
	  if (verbosity > 1)
	    werror("Forcing reextraction of %s.\n", srcdir + fn[..<4]);
	  rm(builddir + fn + ".stamp");
	}
      }
    } else if (Stdio.is_dir(builddir + fn) && !Stdio.is_dir(srcdir + fn)) {
      // Recurse and clean away old obsolete files.
      recurse(srcdir + fn + "/", builddir + fn + "/", root_ts, root);
      rm(builddir + fn + "/.cache.xml.stamp");
      rm(builddir + fn + "/.cache.xml");
      // Try deleting the directory.
      rm(builddir + fn);
      rm(builddir + ".cache.xml.stamp");
    }
  }

  if(!file_stat(srcdir)) {
    if (!Stdio.is_dir(builddir))
      werror("Could not find directory %O.\n", srcdir);
    return;
  }

  // do not recurse into the build dir directory to avoid infinite loop
  // by building the autodoc of the autodoc and so on
  if(search(builddir, srcdir) == -1) {
    foreach(filter(get_dir(srcdir), has_suffix, ".cmod"), string fn) {
      Stdio.Stat stat = file_stat(srcdir + fn);
      if (!stat || !stat->isreg) continue;
      int mtime = stat->mtime;

      // Check for #cmod_include.
      multiset(string) checked = (<>);
      string data = Stdio.read_bytes(srcdir + fn);
      foreach(filter(data/"\n", has_prefix, "#"), string line) {
	if (sscanf(line, "#%*[ \t]cmod_include%*[ \t]\"%s\"", string inc) > 2) {
	  if (!checked[inc]) {
	    checked[inc] = 1;
	    stat = file_stat(combine_path(srcdir, inc));
	    if (stat && stat->isreg && stat->mtime > mtime) {
	      mtime = stat->mtime;
	    }
	  }
	}
      }

      string target = fn[..<5] + ".c";
      stat = file_stat(srcdir + target);
      if (!stat || stat->mtime <= mtime) {
	// Regenerate the target.
	mixed err;
	if (err = catch {
	    Tools.Standalone.precompile()->
	      main(6, ({ "precompile.pike", "--api=max", "-w",
			 "-o", srcdir+target, srcdir+fn }));
	  }) {
	  // Something failed.
	  werror("Precompilation of %s to %s failed:\n"
		 "%s",
		 srcdir+fn, srcdir+target, describe_error(err));
	  rm(srcdir+target);
	  rm(builddir+target+".xml");
	  rm(builddir+target+".xml.stamp");
	}
      }
    }
    foreach(get_dir(srcdir), string fn) {
      if(fn=="CVS") continue;
      if(fn[0]=='.') continue;
      if(fn[-1]=='~') continue;
      if(fn[0]=='#' && fn[-1]=='#') continue;

      Stdio.Stat stat = file_stat(srcdir+fn);

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
	 !has_suffix(fn, ".c") && !has_suffix(fn, ".cc") &&
         !has_suffix(fn, ".m") && !has_suffix(fn, ".bmml") &&
	 has_value(fn, ".")) continue;

      Stdio.Stat dstat = file_stat(builddir+fn+".xml") &&
	file_stat(builddir+fn+".xml.stamp");

      // Build the xml file if it doesn't exist, if it is older than the
      // source file, or if the root has changed since the previous build.
      if(!dstat || dstat->mtime <= stat->mtime || dstat->mtime <= root_ts) {
	string res = extract(srcdir+fn, imgdir, builddir, root);
	if(!res) {
	  if (!(flags & Tools.AutoDoc.FLAG_KEEP_GOING))
	    exit(1);
	  res = "";
	}

	if (sizeof(res) && (res != "\n")) {
	  // Validate the extracted XML.
	  mixed err = catch {
	      Parser.XML.Tree.simple_parse_input(res);
	    };
	  if (err) {
	    werror("Extractor generated broken XML for file %s:\n"
		   "%s",
		   builddir + fn + ".xml", describe_error(err));
	    Stdio.write_file(builddir+fn+".brokenxml", res);
	    if (Stdio.exist(builddir+fn+".xml")) {
	      num_updated_files++;
	      rm(builddir+fn+".xml");
	    }
	    Stdio.write_file(builddir+fn+".xml.stamp",
			     (string)source_timestamp);
	    werror("Result saved as %s.\n", builddir + fn + ".brokenxml");
	    if (flags & Tools.AutoDoc.FLAG_KEEP_GOING) continue;
	    exit(1);
	  }
	}

	string orig = Stdio.read_bytes(builddir + fn + ".xml");
	if (res != orig) {
	  num_updated_files++;
      Stdio.mkdirhier( builddir );
	  Stdio.write_file(builddir+fn+".xml", res);
	  rm(builddir + fn + ".brokenxml");
	}
	Stdio.write_file(builddir+fn+".xml.stamp", (string)source_timestamp);
      }
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

  if (has_suffix(filename, ".bmml") || !has_value(basename(filename), ".")) {
    if (has_suffix(filename, "/control_structures") ||
	has_suffix(filename, "/reserved")) {
      // These two have a unique "markup" that is not yet supported
      // by the BMML parser.
      return "";
    }
    if(flags & Tools.AutoDoc.FLAG_COMPAT &&
       (source_timestamp >= 855275859) &&
       has_value(file, "/precompiled/")) {
      // After new module system was implemented.
      // Convert the old module names according to the timestamp
      if (source_timestamp < 855644314) {
	// Before the modules were capitalized.
	file = replace(file,
		       ({ "/precompiled/FILE",
			  "/precompiled/file",
			  "/precompiled/port",
			  "/precompiled/stack",
			  "/precompiled/string_buffer",
		       }),
		       ({ "stdio.FILE",
			  "stdio.File",
			  "stdio.Port",
			  "stack",
			  "string_functions.String_buffer",
		       }));
      } else {
	file = replace(file,
		       ({ "/precompiled/FILE",
			  "/precompiled/file",
			  "/precompiled/port",
			  "/precompiled/stack",
			  "/precompiled/string_buffer",
		       }),
		       ({ "Stdio.FILE",
			  "Stdio.File",
			  "Stdio.Port",
			  "Stack",
			  "String.String_buffer",
		       }));
      }
      if (source_timestamp < 855648534) {
	// Before the modules were renamed and capitalized.
	file = replace(file,
		       ({ "/precompiled/gdbm",
			  "/precompiled/mpz",
			  "/precompiled/regexp",
			  "/precompiled/sql",
			  "/precompiled/sql_result",
			  "/precompiled/sql/*",
			  "/precompiled/sql/mysql",
			  "/precompiled/sql/mysql_result",
		       }),
		       ({ "gdbmmod.gdbm",
			  "gmpmod.mpz",
			  "regexp.regexp",
			  "sql.sql",
			  "sql.sql_result",
			  "sql.*",
			  "sql.mysql",
			  "sql.mysql_result",
		       }));
      } else {
	file = replace(file,
		       ({ "/precompiled/gdbm",
			  "/precompiled/mpz",
			  "/precompiled/regexp",
			  "/precompiled/sql",
			  "/precompiled/sql_result",
			  "/precompiled/sql/*",
			  "/precompiled/sql/mysql",
			  "/precompiled/sql/mysql_result",
		       }),
		       ({ "Gdbm.gdbm",
			  "Gmp.mpz",
			  "Regexp",
			  "sql.sql",
			  "sql.sql_result",
			  "sql.*",
			  "sql.mysql",
			  "sql.mysql_result",
		       }));
      }
      if (source_timestamp < 855725630) {
	// Before the builtin was capitalized.
	file = replace(file,
		       ({ "/precompiled/condition",
			  "/precompiled/mutex",
			  "builtin/",
		       }),
		       ({ "builtin.condition",
			  "builtin.mutex",
			  "builtin.",
		       }));
      } else if (source_timestamp < 857038037) {
	// Before the Thread module.
	file = replace(file,
		       ({ "/precompiled/condition",
			  "/precompiled/mutex",
			  "builtin/",
		       }),
		       ({ "Builtin.condition",
			  "Builtin.mutex",
			  "Builtin.",
		       }));
      } else {
	file = replace(file,
		       ({ "/precompiled/condition",
			  "/precompiled/mutex",
			  "builtin/",
		       }),
		       ({ "Thread.Condition",
			  "Thread.Mutex",
			  "Builtin.",
		       }));
      }
      if (source_timestamp < 855644314) {
	// Before the modules were capitalized.
	file = replace(file,
		       ({ "/precompiled/fifo",
			  "/precompiled/queue",
		       }),
		       ({ "fifo.Fifo",
			  "fifo.Queue",
		       }));
      } else if (source_timestamp < 857038037) {
	// Before the Thread module.
	file = replace(file,
		       ({ "/precompiled/fifo",
			  "/precompiled/queue",
		       }),
		       ({ "Fifo.Fifo",
			  "Fifo.Queue",
		       }));
      } else {
	file = replace(file,
		       ({ "/precompiled/fifo",
			  "/precompiled/queue",
		       }),
		       ({ "Thread.Fifo",
			  "Thread.Queue",
		       }));
      }
    }
    file = replace(file, "Myslq", "Mysql");
    Tools.AutoDoc.BMMLParser bmml_parser = Tools.AutoDoc.BMMLParser();
    return bmml_parser->convert_page(filename, basename(filename), file,
				     flags, root);
  }

  int i;
  if (has_value(file, "**""!") ||
      ((((i = search(file, "//""! ""module ")) != -1) ||
	((i = search(file, "//""! ""submodule ")) != -1)) &&
       (sizeof(array_sscanf(file[i..],"%s\n%*s")[0]/" ") == 3))) {
    // Mirar-style markup.
    if(imgsrc && imgdest) {
      mixed err = catch {
	Tools.AutoDoc.MirarDocParser mirar_parser =
	  Tools.AutoDoc.MirarDocParser(imgsrc, flags);

	if (flags & Tools.AutoDoc.FLAG_COMPAT) {
	  // Special cases for some files that have
	  // known breakage due to missing headers.
	  foreach((["/files/termios.c":
		    "**""! module Stdio\n"
		    "**\n"
		    "**""! class File\n"
		    "**\n",
		    "/Image/blit.c":
		    "**""! module Image\n"
		    "**\n"
		    "**""! class image\n"
		    "**\n",
		    "Calendar.pmod/Gregorian.pmod":
		    "//! module Calendar\n"
		    "//\n",
		    "Calendar.pmod/Stardate.pmod":
		    "//! module Calendar\n"
		    "//\n",
		    "Calendar_I.pmod/Gregorian.pmod":
		    "//! module Calendar_I\n"
		    "//\n",
		    "Calendar_I.pmod/Stardate.pmod":
		    "//! module Calendar_I\n"
		    "//\n",
		  ]); string suffix; string lines) {
	    if (has_suffix(filename, suffix) &&
		!has_value(file, (lines/"\n")[0])) {
	      foreach(lines/"\n"; int lineno; string line) {
		mirar_parser->process_line(line, "MAGIC:" + filename, lineno+1);
	      }
	      break;
	    }
	  }
	  // Repair some known typos and other breakage.
	  foreach((["/Image/blit.c":
		    ({ ({
		      "object add_layers(array(int|object)) layer0,",
		      "object add_layers(int x1,int y1,int x2,int y2,"
		      "array(int|object)) layer0,",
		    }), ({
		      "object add_layers(array(int|object) layer0,",
		      "object add_layers(int x1,int y1,int x2,int y2,"
		      "array(int|object) layer0,",
		    }) }),
		    "/Image/colors.c":
		    ({ ({
		      "**""! constant modifiers=({",
		    }), ({
		      "**""! array(string) modifiers=({",
		    }) }),
		    "/Parser/html.c":
		    ({ ({
		      "simplifying work for custom classes that\n"
		      "**""!\tinherits <ref>Parser.HTML</ref>.",
		    }), ({
		      "simplifying work for custom classes that\n"
		      "**""!\tinherit <ref>Parser.HTML</ref>.",
		    }) }),
		    "/Calendar.pmod/Islamic.pmod":
		    ({ ({
		      "//""! submodule Gregorian\n",
		    }), ({
		      "//""! submodule Islamic\n",
		    }) }),
		    "/Calendar_I.pmod/Gregorian.pmod":
		    ({ ({
		      "//""! module Calendar\n",
		    }), ({
		      "//""! module Calendar_I\n",
		    }) }),
		    "/Calendar_I.pmod/Stardate.pmod":
		    ({ ({
		      "//""! module Calendar\n",
		    }), ({
		      "//""! module Calendar_I\n",
		    }) }),
		    "/Calendar_I.pmod/module.pmod":
		    ({ ({
		      "//""! module Calendar\n",
		    }), ({
		      "//""! module Calendar_I\n",
		    }) }),
		  ]); string suffix; array(array(string)) repl) {
	    if (has_suffix(filename, suffix)) {
	      file = replace(file, @repl);
	      break;
	    }
	  }
	  if (has_suffix(filename, "/Image/colors.c") &&
	      has_value(file, "sort(grey")) {
	    // Special fix to force stable output
	    // for the list of colors. (Used to be mapping-sort...)
	    if (!has_value(file, "sort(grey->name(), grey);")) {
	      file = replace(file, "sort(grey",
			     "sort(grey->name(), grey);"
			     "sort(colored->name(), colored);"
			     "sort(grey");
	    }
	  }
	}

	int lineno = 1;
	foreach(file/"\n", string line) {
	  mirar_parser->process_line(line, filename, lineno++);
	}
	return mirar_parser->make_doc_files(builddir, imgdest, root[0]);
      };
      if (!(flags & Tools.AutoDoc.FLAG_KEEP_GOING)) throw(err);
      werror("MirarDoc extractor failed: %s\n", describe_backtrace(err));
      return "\n";
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
  if( !(< "c", "cc", "cpp", "m", /* "cmod", */ "pike", "pmod", >)[suffix] )
    error("Unknown filetype %O.\n", suffix);
  name_sans_suffix =
    name_sans_suffix[..sizeof(name_sans_suffix)-(sizeof(suffix)+2)];

  string result;
  mixed err = catch {
    if( suffix == "c" || suffix == "cc" || suffix == "cpp"
	/* || suffix == "cmod" */)
      result = Tools.AutoDoc.ProcessXML.extractXML(filename,0,0,0,root,flags);
    else {
      string type = ([ "pike":"class", "pmod":"module", ])[suffix];
      string name = (name_sans_suffix/"/")[-1];
#if 0
      werror("root: %{%O, %}\n"
	     "type: %O\n"
	     "name: %O\n", root, type, name);
#endif /* 0 */
      // Support filenames with embedded dots.
      // cf GTK2:refdoc/GTK2.pmod/G.Object.pike
      root += name/".";
      name = root[-1];
      root = root[..sizeof(root)-2];
      if(name == "module" && type != "class") {
	if(sizeof(root)<2)
	  error("Unknown module parent name.\n");
	name = root[-1];
	root = root[..sizeof(root)-2];
      } else if ((name == "__default") && (sizeof(root) == 1)) {
	// Pike backward compatibility module.
	name = root[0];
	if (has_suffix(name, "::")) name = name[..sizeof(name)-3];
	root = ({});
	type = "namespace";
      }

      result = Tools.AutoDoc.ProcessXML.extractXML(filename, 1, type,
						   name, root, flags);
    }
  };

  if (err) {
    if (!verbosity)
      ;
    werror("\nERROR: %s\n", describe_error(err));
    // return 0;
  }

  if(!result) result="";

  if(sizeof(result) && imgdest)
    result = Tools.AutoDoc.ProcessXML.moveImages(result,
						 combine_path(filename, ".."),
						 imgdest, !verbosity);
  return result+"\n";
}
