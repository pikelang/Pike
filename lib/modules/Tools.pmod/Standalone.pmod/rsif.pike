#! /usr/bin/env pike
// -*- pike -*- $Id: rsif.pike,v 1.3 2002/12/14 04:34:15 nilsson Exp $

constant version = ("$Revision: 1.3 $"/" ")[1];
constant description = "Replaces strings in files.";
int(0..1) verbosity = 1; // more output
int(0..1) overwrite = 1; // no backups
int(0..1) recursive = 0; // not recursive

constant usage = #"rsif [options] <from> <to> <files>

rsif (\"replace string in file\") replaces all occurrences of the
string <from> with the string <to> in listed files. The name of the
files that were changed are written to stdout. Directories may be
given instead of files, in which case all the files in that directory
will be processed. Available options:

  -b, --backups   Saves the unaltered file in <filename>~
  -r, --recursive Processes directories recusively
  -v, --verbose   Writes more junk to stderr.
  -q, --quiet     Writes no output at all.
  -V, --version   Writes the version number of rsif.
  -h, --help      Shows this help message.
";

int(0..) process_path(string path, string from, string to) {

  if(Stdio.is_dir(path)) {
    if( path[-1] != '/' )
      path += "/";
    int failures;
    foreach(get_dir(path), string fn)
      failures += process_path( path+fn, from, to );
    return failures;
  }
  else
    return process(path, from, to);
}

int(0..1) process(string path, string from, string to) {

  string file = Stdio.read_file(path);
  if(file) {
    if(has_value(file, from)) {
      if(verbosity > 0)
	write("%s\n", path);
      string file = replace(file, from, to);
      int mode = file_stat(path)->mode;

      if(overwrite || mv(path, path + "~"))
	return !Stdio.write_file(path, file, mode);
      else {
	werror("Failed to create backup file.\n");
	return 1;
      }
    }
  }
}

int(0..) main(int argc, array(string) argv)
{
  foreach(Getopt.find_all_options(argv, ({
    ({ "backup",     Getopt.NO_ARG,       "-b,--backup"/"," }),
    ({ "recurse",    Getopt.NO_ARG,       "-r,--recursive"/"," }),
    ({ "verbose",    Getopt.NO_ARG,       "-v,--verbose"/"," }),
    ({ "version",    Getopt.NO_ARG,       "-V,--version"/"," }),
    ({ "quiet",      Getopt.NO_ARG,       "-q,--quiet"/"," }),
    ({ "help",       Getopt.NO_ARG,       "-h,--help"/"," }) })), array opt)
    switch(opt[0])
    {
      case "backup":
	overwrite = 0;
	break;
      case "recurse":
	recursive = 1;
	break;
      case "verbose":
	verbosity++;
	break;
      case "version":
	write(version + "\n");
	return 0;
      case "quiet":
	verbosity = 0;
	break;
      case "help":
	write(usage);
	return 0;
    }
  argv = Getopt.get_args(argv);

  if(4 > (argc = sizeof(argv))) {
    werror(usage);
    return 1;
  }

  string from = argv[1], to = argv[2];
  if(argc == 4 && argv[-1] == "-")
  {
    string file;
    if(file = Stdio.stdin.read())
    {
      if(has_value(file, argv[1]))
      {
	if(verbosity > 1)
	  werror("Processing stdin.\n");
	write(replace(file, argv[1], argv[2]));
      } else
	write(file);
    }
    return 0;
  }

  int failures;
  if(verbosity > 1)
    werror("Replaced strings in these files:\n");
  foreach(argv[3..], string path)
    failures += process_path(path, argv[1], argv[2]);

  return failures;
}
