#! /usr/bin/pike
// -*- pike -*- $Id: process_files.pike,v 1.2 2007/01/22 04:23:55 jhs Exp $
#pike __REAL_VERSION__

#ifdef SUGGESTED_MODE_OF_USAGE
inherit .process_files;
string version = ("$Revision: 1.2 $"/" ")[1];
string description = "One-liner tool description for plain \"pike -x\" here.";
string usage = #"Long usage description here; see rsif.pike for inspiration";
int want_args = 2; // rsif takes 2; how many do you want? This one wants two:

string process( string input, string arg1, string arg2 ) {
  if( has_value( input, arg1 ) ) // otherwise return 0 == didn't touch input
    return replace( input, arg1, arg2 );
}
#else

//! Boilerplate to quickly whip up rsif-like hacks for creating file processing
//! to churn away at stdin, or files and/or directories provided on the command
//! line, recursively or not, creating backup files or not, listing the paths
//! of all files action was taken for, and returning an exit status of how many
//! files that would have been taken action on which could not be written back.
//!
//! The all-round quickest way of making one of these tools is nicking the top
//! portion of @tt{lib/modules/Tools.pmod/Standalone.pmod/process_files.pike}
//! or copying @tt{rsif.pike} from the same directory. (The latter is 20 lines
//! long, including docs, or about four lines of code.) Inherit process_files,
//! and define your own @[version], @[description], @[usage], @[process], and,
//! if you want arguments, @[want_args].

string version;
//! Your hack's version number. If you version control your file with cvs, we
//! suggest you set the contents of this variable to something that that will
//! automatically expand to a number for every new revision, for instance
//! @example
//!   string version = ("$Revision: 1.2 $"/" ")[1];

string description = "Boilerplate for making rsif-like tools.";
//! One-liner that gets shown for this tool when running @tt{pike -x} without
//! additional options. (Assuming your tool resides in @tt{Standalone.pmod}.)
//! Does not include the name of the tool itself; just provide a nice, terse
//! description, ending with a period for conformity.

string usage;
//! Long description of the purpose and usage of your tool, for --help and the
//! case where not enough options are given on the command line. Its invocation
//! mode (for instance "pike -x yourtool", when invoked that way) is prepended,
//! and a list of available flags appended to this description.

int want_args = 0;
//! The number of (mandatory) command-line options your hack needs and which
//! your @[process] callback wants (beside the input file). By default 0.

//! Gets called with the contents of a file (or stdin). Return your output, or
//! 0 if you didn't do anything. @[args] are the first @[want_args]
//! command-line options provided, before the list of files to process.
string process( string input, string ... args ) {
  error("forgot to add your own implementation of process()!");
}
#endif

// things you need not provide your own versions of below:

int(0..) verbosity = 1; //! 0 in quiet mode, 1 by default, above = more output

int(0..1) overwrite = 1; //! 0 to make backups, 1 to overwrite (default)

int(0..1) recursive = 0; //! 1 to recurse into directories, 0 not to (default)

//! Flag docs to append to your @[usage] description. Explains default options.
string default_flag_docs = #"
  -b, --backups   Saves the unaltered file in <filename>~
  -r, --recursive Processes directories recusively
  -v, --verbose   Writes more junk to stderr.
  -q, --quiet     Writes no output at all.
  -V, --version   Writes the version number of rsif.
  -h, --help      Shows this help message.
";

//! Takes the path to a file and the first @[want_args] command-line options
//! provided, before the list of files to process. You might want to override
//! this method if your tool needs to see file paths.
//! @seealso
//!   @[process_path]
int(0..1) process_file( string path, string ... args ) {
  string input = Stdio.read_file( path );
  if( input ) {
    string output = process( input, @args );
    if( output && output != input ) {
      if( verbosity > 0 )
	write( "%s\n", path );

      int mode = file_stat( path )->mode;
      if( overwrite || mv( path, path+"~" ) ) {
	if( !Stdio.write_file( path, output, mode ) ) {
	  werror( "Failed to write file %s.\n", path );
	  return 1;
	}
      } else {
	werror( "Failed to create backup file %s~.\n", path );
	return 1;
      }
    }
  }
}

//! Takes the path to a file or directory and the first @[want_args] first
//! command-line options provided, before the list of files to process. You
//! might want to override this method if your tool needs to see all paths.
//! @seealso
//!   @[process_file]
int(0..) process_path( string path, string ... args ) {
  if( Stdio.is_dir( path ) ) {
    if( path[-1] != '/' )
      path += "/";
    int failures;
    if( recursive )
      foreach( get_dir(path), string fn )
	failures += process_path( path+fn, @args );
    return failures;
  }
  else
    return process_file( path, @args );
}

//! Base implementation of main program, handling basic flags and returning an
//! exit status matching the number of failures during operation.
//! @fixme
//!   No easy way of adding your own command-line flags implemented yet. This
//!   would be a rather natural next feature to add, once somebody needs some.
int(0..) main( int argc, array(string) argv ) {
  if( has_suffix( lower_case(dirname( argv[0] )), "standalone.pmod" ) )
    usage = "pike -x " + basename( argv[0] )-".pike" + " " + usage;
  foreach(Getopt.find_all_options( argv, ({
    ({ "backup",     Getopt.NO_ARG,       "-b,--backup"/"," }),
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

      case "backup":
	overwrite = 0;
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
    string file = Stdio.stdin.read();
    if( file ) {
      if( verbosity > 1 )
	werror( "Processing stdin.\n" );
      string output = process( file, @args );
      write( "%s", output || file );
    }
    return 0;
  }

  int failures;
  if( verbosity > 1 )
    werror( "Replaced strings in these files:\n" );
  foreach( argv[min_args-1..], string path )
    failures += process_path( path, @args );
  return failures;
}
