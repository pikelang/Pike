#! /usr/bin/env pike
// -*- pike -*-
#pike __REAL_VERSION__
#require constant(Web.Sass)

inherit Tools.Standalone.process_files;
string version =
 sprintf("%d.%d.%d",(int)__REAL_VERSION__,__REAL_MINOR__,__REAL_BUILD__);
string description = "Compile SASS SCSS code.";

string default_flag_docs = #"
  -g, --genmap    Generate .map files for debugging
  -r, --recursive Processes directories recusively
  -v, --verbose   Writes more junk to stderr.
  -q, --quiet     Writes no output at all.
  -V, --version   Writes the version number of rsif.
  -h, --help      Shows this help message.
";

string usage = #"[options] [<infile.scss>] ...

sass (\"compile SASS SCSS file\") Compiles a SASS SCSS code down to plain CSS.
Available options:
" + default_flag_docs;

int want_args = 0;

int(0..) verbosity = 0;
string genmap = 0;

string|zero process(string input, string ... args) {
  Web.Sass.Api sc = Web.Sass.Api();
  sc->output_style = Web.Sass.STYLE_COMPRESSED;
  sc->sass_syntax = false;
  sc->source_map_embed = !!genmap;
  mapping output = sc->compile_string(input);
  if (!output) {
    werror("Problem converting from stdin\n");
    return 0;
  }
  if (verbosity)
    werror("Processed %d bytes from stdin to %d bytes on stdout\n",
     sizeof(input), sizeof(output->css));
  return output->css;
}

Regexp getextension = Regexp("[^/]\\.([^.]+)$");

string getbase(string path) {
  array(string) ext = getextension->split(path);
  return ext ? path[.. < sizeof(ext[0]) + 1] : path;
}

int(0..1) process_file(string path, string ... args) {
  string base = getbase(path) + ".css";
  if (base == path) {
    werror("Input %s must differ from output, aborted...\n", path);
    return 1;
  }
  Web.Sass.Compiler sc = Web.Sass.Compiler();
  mapping opt = ([
    "output_style":Web.Sass.STYLE_COMPRESSED,
  ]);
  if (genmap)
    opt->source_map_file
     = (genmap ? genmap + "/" : "") + basename(base) + ".map";
  else
    opt->omit_source_map_url = 1;
  sc->set_options(opt);
  mapping output = sc->compile_file(path);
  if (!output) {
    werror("Problem converting %O\n", output);
    return 1;
  }
  if (verbosity)
    werror("Processed %s to %d bytes in %s\n",
     path, sizeof(output->css), base);
  Stdio.File(base, "wtc")->write(output->css);
  string mapfile = base + ".map";
  if (genmap) {
    if (verbosity)
      werror("Generating %d bytes into %s\n", sizeof(output->map), mapfile);
    Stdio.File(mapfile, "wtc")->write(output->map);
  } else if(rm(mapfile) && verbosity)
    werror("Removing stale %s\n", mapfile);
  return 0;
}

int(0..) main( int argc, array(string) argv ) {
  if (has_suffix( lower_case(dirname( argv[0])), "standalone.pmod"))
    usage = "pike -x " + basename( argv[0] )-".pike" + " " + usage;
  foreach(Getopt.find_all_options( argv, ({
    ({ "genmap",     Getopt.MAY_HAVE_ARG, "-g,--genmap"/"," }),
    ({ "recurse",    Getopt.NO_ARG,       "-r,--recursive"/"," }),
    ({ "verbose",    Getopt.NO_ARG,       "-v,--verbose"/"," }),
    ({ "version",    Getopt.NO_ARG,       "-V,--version"/"," }),
    ({ "quiet",      Getopt.NO_ARG,       "-q,--quiet"/"," }),
    ({ "help",       Getopt.NO_ARG,       "-h,--help"/"," }) })), array opt )
    switch(opt[0]) {
      case "version":
	write( version + "\n" );
	return 0;
      case "quiet":
	verbosity = 0;
	break;
      case "help":
	write( usage );
	return 0;

      case "genmap":
	genmap = sizeof(opt) > 1 ? opt[1] : "";
	break;
      case "recurse":
	recursive = 1;
	break;
      case "verbose":
	verbosity++;
	break;
    }
  argv = Getopt.get_args(argv);

  int min_args = 2 + want_args;
  if( (argc = sizeof(argv)) < min_args ) {
    werror( usage );
    return 1;
  }

  array(string) args = ({});
  if( want_args )
     args = argv[1..want_args];
  if( argc == min_args && argv[-1] == "-" ) {
    string input = Stdio.stdin.read();
    if( input ) {
      string output = process( input, @args );
      write( "%s", output || input );
    }
    return 0;
  }

  int failures;
  foreach( argv[min_args-1..], string path )
    failures += process_path( path, @args );
  return !!failures;	// Do not return arbitrarily high numbers
}
