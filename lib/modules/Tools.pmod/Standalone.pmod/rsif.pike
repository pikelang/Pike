#! /usr/bin/env pike
// -*- pike -*- $Id: rsif.pike,v 1.2 2003/09/15 18:35:13 nilsson Exp $

#pike __REAL_VERSION__

string version = ("$Revision: 1.2 $"/" ")[1];
int verbosity = 1; // more output
int overwrite = 1; // no backups

constant usage = #"rsif [options] <from> <to> <files>

rsif (\"replace string in file\") replaces all occurrences of the string
<from> with the string <to> in listed files. The name of the files that
were changed are written to stdout. Available options:

  -b, --backups   Saves the unaltered file in <filename>~
  -v, --verbose   Writes more junk to stderr.
  -q, --quiet     Writes no output at all.
  -V, --version   Writes the version number of rsif.
  -h, --help      Shows this help message.\n";

int main(int argc, array(string) argv)
{
  foreach(Getopt.find_all_options(argv, ({
    ({ "backup",     Getopt.NO_ARG,       "-b,--backup"/"," }),
    ({ "verbose",    Getopt.NO_ARG,       "-v,--verbose"/"," }),
    ({ "version",    Getopt.NO_ARG,       "-V,--version"/"," }),
    ({ "quiet",      Getopt.NO_ARG,       "-q,--quiet"/"," }),
    ({ "help",       Getopt.NO_ARG,       "-h,--help"/"," }) })), array opt)
    switch(opt[0])
    {
      case "backup":
	overwrite = 0;
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

  string from = argv[1], to = argv[2], file;
  if(argc == 4 && argv[-1] == "-")
  {
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

  int failures, mode;
  if(verbosity > 1)
    werror("Replaced strings in these files:\n");
  foreach(argv[3..], string filename)
  {
    if(file = Stdio.read_file(filename))
    {
      if(has_value(file, argv[1]))
      {
	if(verbosity > 0)
	  write("%s\n", filename);
    	file = replace(file, argv[1], argv[2]);
	mode = file_stat(filename)->mode;
	if(overwrite || mv(filename, filename + "~"))
	  failures += !Stdio.write_file(filename, file, mode);
	else
	{
	  werror("Failed to create backup file.\n");
	  failures++;
	}
      }
    }
  }
  return failures;
}
