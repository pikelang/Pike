#! /usr/bin/env pike

#pike __REAL_VERSION__

/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: dump.pike,v 1.4 2004/09/25 20:49:11 nilsson Exp $
*/

constant description = "Dumps Pike files into object files.";

int quiet=1, report_failed=0, recursive=0, update=0, nt_install=0;
string target_dir = 0;
string update_stamp = 0;

program p; /* program being dumped */

#ifdef PIKE_FAKEROOT
string fakeroot(string s)
{
  return PIKE_FAKEROOT+combine_path(getcwd(),s);
}
#else
#define fakeroot(X) X
#endif

object progress_bar;

class GTKProgress {
#if constant(GTK.Window)
  GTK.ProgressBar bar;
#endif
  float progress;
  float scale;

  void update(int num) {
    progress += scale*num;
    progress = min(progress, 1.0);
#if constant(GTK.Window)
    bar->update(progress);
#endif
  }

#if constant(GTK.Window)
  GTK.Window win;
  GTK.Frame frame;

  void create() {
    GTK.setup_gtk();
    win = GTK.Window(GTK.WINDOW_TOPLEVEL)
      ->set_title(version() + " Module Dumping")
      ->add(frame = GTK.Frame("Dumping Pike modules...")->
	    set_border_width(16)->
	    add(bar = GTK.ProgressBar()->set_usize(150,20)));
    win->show_all();
  }
#endif
}

Stdio.File logfile = Stdio.stderr;

int padline = 0;
string next_file = 0;

void logstart (int one_line)
{
  if (padline) logfile->write ("\n");
  if (one_line) {
    logfile->write("#### %s: ", next_file);
    padline = 0;
  }
  else {
    logfile->write("#### %s:\n", next_file);
    padline = 1;
  }
  next_file = 0;
}

void logmsg (mixed... args)
{
  if(logfile) {
    if (next_file) logstart (1);
    if (args) logfile->write (@args);
  }
}

void logmsg_long (mixed... args)
{
  if(logfile) {
    if (next_file) logstart (0);
    if (args) logfile->write (@args);
  }
}

class MyMaster
{
  inherit "/master";

  void handle_error (mixed trace)
  {
    logmsg_long (describe_backtrace (trace));
  }

  static void create()
  {
    object old_master = master();
    ::create();
    foreach (indices (old_master), string var)
      catch {this[var] = old_master[var];};
    programs["/master"] = this_program;
    objects[this_program] = this;
  }
}

class Handler
{
  void compile_error(string file,int line,string err)
  {
    logmsg_long("%s:%d:%s\n",file,line,err);
  }

  void compile_warning(string file,int line,string err)
  {
    logmsg_long("%s:%d:%s\n",file,line,err);
  }

  int compile_exception (array|object trace)
  {
    if (!objectp (trace) || !trace->is_cpp_error && !trace->is_compilation_error)
      logmsg_long (describe_backtrace (trace));
    return 1;
  }
}

program compile_file(string file, object|void handler)
{
  if(master()->relocate_module) {
    string s = master()->master_read_file(file);
    return master()->compile_string(s,
				    master()->unrelocate_module ?
				    master()->unrelocate_module (file) :
				    file,
				    handler);
  } else
    return master()->compile_file(file, handler);
}

int dumpit(string file, string outfile)
{
  int ok = 0;
  next_file = file;

do_dump: {
    if(Stdio.Stat s=file_stat(fakeroot(file)))
    {
      if (update) {
	if (Stdio.Stat o = file_stat (fakeroot(outfile) + ".o"))
	  if (o->mtime >= s->mtime) {
	    if (!quiet) logmsg ("Up-to-date.\n");
	    ok = 1;
	    break do_dump;
	  }
      }
      rm(fakeroot(outfile)+".o"); // Make sure no old files are left

      if (s->isdir && recursive) {
	if (array(string) dirlist = get_dir (fakeroot (file))) {
	  if (!quiet) logmsg ("Is a directory (dumping recursively).\n");
	  foreach (dirlist, string subfile)
	    if (subfile != "CVS" &&
		(has_suffix (subfile, ".pike") ||
		 has_suffix (subfile, ".pmod") ||
		 Stdio.is_dir (file + "/" + subfile)))
	      if (!dumpit (combine_path (file, subfile),
			   combine_path (outfile, subfile)))
		return 0;
	  ok = 1;
	  break do_dump;
	}
	else {
	  logmsg ("Is an unreadable directory (not dumped recursively): %s.\n",
		  strerror (errno()));
	  break do_dump;
	}
      }
      else if (!s->isreg)
      {
	logmsg("Is a directory or special file (not dumped).\n");
	break do_dump;
      }
    }else{
      if(!quiet) logmsg("Does not exist (not dumped).\n");
      break do_dump;
    }

    mixed err;
    if(!(err = catch {
	// Kludge: Resolve the module through master()->resolv since
	// it handles cyclic references better than we do in
	// compile_file above.
	master()->resolv (master()->program_path_to_name (file));

	p=compile_file(file, Handler());

      }) && programp (p))
    {
      if(!p->dont_dump_module && !p->dont_dump_program)
      {
	string s;
	if ((err = catch {
	    s=encode_value(p, master()->Encoder(p));
	    p=decode_value(s, master()->Decoder());
	  }))
	  logmsg_long(describe_backtrace(err));

	else if(programp(p))
	{
	  string dir = combine_path (outfile, "..");
	  if (!Stdio.is_dir (fakeroot(dir)))
	    if (!Stdio.mkdirhier (fakeroot(dir))) {
	      logmsg ("Failed to create target directory %O: %s.\n",
		      dir, strerror (errno()));
	      break do_dump;
	    }

	  Stdio.write_file(fakeroot(outfile)+".o",s);
	  ok = 1;
	  if(!quiet) logmsg("Dumped.\n");
	}

	else if(!quiet)
	  logmsg("Decode of %O failed (not dumped).\n", file);
      }

      else if(!quiet)
	logmsg("Not dumping %O (not dumped).\n", file);
    }

    else {
      // This should never happen. If it does then it's not safe to
      // continue dumping since later modules might do #if constant(...)
      // on something for modifiers in this one and would then be dumped
      // incorrectly without errors.
      if (err && (!objectp (err) || !err->is_compilation_error || !err->is_cpp_error))
	// compile() should never throw any other error, but we play safe.
	logmsg_long("Compilation of %O failed (not dumped):\n%s",
		    file, describe_backtrace(err));
      else
	logmsg("Compilation of %O failed (not dumped).\n", file);
      if (report_failed)
	write ("Aborting dumping since %s didn't compile\n", file);
      return 0;
    }
  }

  if (report_failed && !ok)
    write ("Dumping failed for %s\n", file);
  return 1;
}

constant help = #"Dumps pike files into object files.
Arguments:

-h, --help
  Show this message

-l [X], --log-file[=X]
  Outputs a dump log to X or, if not provided, dumpmodule.log.

-v, --verbose
  Be more verbose.

-q, --quiet
  Be quiet.

--distquiet
  Be quiet and log to /dev/null.

--progress-bar=X,Y
  Show a progress bar that runs between X and Y.

--report-failed
  Report dumps that fails.

-r, --recursive
  Recurse .pmod directories.

-t X, --target-dir=[X]
  Where to put the dumped files.

-u, --update-only
  Only redump files that are newer than the dumped file.
";

void setup_logging(void|string file) {
  logfile = Stdio.File(stringp(file) && file || "dumpmodule.log",
		       "caw");
  /* Redirect all debug and error messages to the logfile. */
  logfile->dup2(Stdio.stderr);
}

int pos;
array files;

void dump_files() {

  if(pos>=sizeof(files)) {
    if (update_stamp)
      Stdio.write_file (update_stamp, version());
    exit(0);
  }

  string file = files[pos++];

  if(progress_bar)
    progress_bar->update(1);

  string outfile = file;
  if (target_dir) {
#ifdef __NT__
    outfile = replace (outfile, "\\", "/");
#endif
    outfile = combine_path (target_dir, ((outfile / "/") - ({""}))[-1]);
  }

  if (!dumpit(file, outfile) && !nt_install)
    pos = sizeof(files); // exit

  call_out(dump_files, 0);
}

int main(int argc, array(string) argv)
{
  replace_master (MyMaster());

  foreach (Getopt.find_all_options (argv, ({
    ({"help", Getopt.NO_ARG, ({"-h", "--help"})}),
    ({"log-file", Getopt.MAY_HAVE_ARG, ({"-l", "--log-file"})}),
    ({"verbose", Getopt.NO_ARG, ({"-v", "--verbose"})}),
    ({"quiet", Getopt.NO_ARG, ({"-q", "--quiet"})}), // The default.
    ({"distquiet", Getopt.NO_ARG, ({"--distquiet"})}),
    ({"progress-bar", Getopt.HAS_ARG, ({"--progress-bar"})}),
    ({"report-failed", Getopt.NO_ARG, ({"--report-failed"})}),
    ({"recursive", Getopt.NO_ARG, ({"-r", "--recursive"})}),
    ({"target-dir", Getopt.HAS_ARG, ({"-t", "--target-dir"})}),
    ({"update-only", Getopt.MAY_HAVE_ARG, ({"-u", "--update-only"})}),
    ({"nt-install", Getopt.NO_ARG, ({"--nt-install"})}),
  })), array opt)
    switch (opt[0]) {

      case "help":
	write(help);
	return 0;

      case "nt-install":
	quiet = 1;
	nt_install = 1;
	setup_logging();
	break;

      case "log-file":
	setup_logging(opt[1]);
	break;

      case "verbose":
	quiet = 0;
	break;

      case "quiet":
	quiet=1;
	break;

      case "distquiet":
	quiet=1;
	logfile=0;
	break;

      case "progress-bar":
	quiet = 1;

	progress_bar = Tools.Install.ProgressBar("Precompiling",
						 @array_sscanf(opt[1], "%d,%d"),
						 0.2, 0.8);
	break;

      case "report-failed":
	report_failed = 1;
	break;

      case "recursive":
	recursive = 1;
	break;

      case "target-dir":
	target_dir = opt[1];
	break;

      case "update-only":
	if (stringp (opt[1])) {
	  update_stamp = opt[1];
	  update = Stdio.read_file (update_stamp) == version();
	}
	else
	  update = 1;
	break;
    }

  // Remove the name of the program.
  argv = argv[1..];

  if(nt_install) {
    progress_bar = GTKProgress();
    files = ({});
    foreach(master()->pike_module_path, string path) {
      object fs = Filesystem.Traversion(path);
      foreach(fs;  string d; string f) {
	if(fs->stat()->isdir) continue;
	if(f=="master.pike") continue;
	if( has_suffix(f, ".pike") || has_suffix(f, ".pmod") )
	  files += ({ d+f });
      }
    }
    progress_bar->scale = 1.0/sizeof(files);
  }
  else
    files = Getopt.get_args(argv);

  call_out(dump_files, 0);
  return -1;
}
