#! /usr/bin/env pike
#pike __REAL_VERSION__

constant description = "Executes tests according to testsuite files.";

import Tools.Testsuite;

protected enum exit_codes {
  EXIT_OK,
  EXIT_TEST_FAILED,
  EXIT_TEST_NOT_FOUND,
  EXIT_WATCHDOG_FAILED,
};

#if !constant(_verify_internals)
#define _verify_internals()
#endif

int foo(string opt)
{
  if(opt=="" || !opt) return 1;
  return (int)opt;
}

int maybe_tty = 1;

mapping(string:int) cond_cache=([]);

float float_precision = 1.0;

#if constant(thread_create)
#define HAVE_DEBUG
#endif

void print_code(string test)
{
  test = Charset.encoder("iso-8859-1", 0,
                         lambda(string s) {
                           return sprintf("\\%o", s[0]);
                         })->feed(test)->drain();
  foreach(test/"\n"; int r; string line) {
    log_msg("%3d: %s\n", r+1, line);
  }
  log_msg("\n");
}

void report_size()
{
#if 0
  Process.system(sprintf("/usr/proc/bin/pmap %d|tail -1", getpid()));
#endif
}

array(string) find_testsuites(string dir)
{
  array(string) ret = ({});
  if(array(string) s = get_dir(dir))
  {
    if(has_value(s, "no_testsuites")) return ret;
    foreach(s, string file)
    {
      string name = combine_path(dir, file);
      if(Stdio.is_dir(name))
	ret += find_testsuites(name);
      else if(file=="testsuite") {
	ret += ({ combine_path(dir, file) });
      } else if(has_suffix(file, ".test")) {
	ret += ({ combine_path(dir, file) });
      }
    }
  }
  return ret;
}

class ScriptTestsuite(string|zero file_name)
{
  inherit Testsuite;

  int pos = -1;

  protected int(1..1) _sizeof() { return 1; }
  protected int(0..0) _iterator_next()
  {
    pos++;
    if (pos) {
      pos = -1;
      return UNDEFINED;
    }
    return 0;
  }
  protected int(0..) _iterator_index() { return pos && UNDEFINED; }
  protected Test _iterator_value()
  {
    return Test(file_name, 1, 1, "RUNCT",
                sprintf("array a() { return Tools.Testsuite.run_script (({ %q })); }", file_name));
  }
  this_program skip(int steps)
  {
    pos += steps;
  }
}

object(Testsuite)|zero read_tests( string fn ) {
  string tests = Stdio.read_file( fn );
  if(!tests) {
    log_msg("Failed to read test file %O, errno=%d.\n",
	    fn, errno());
    exit(EXIT_TEST_NOT_FOUND);
  }

  string test_type = "legacy";
  sscanf(tests, "%*sTEST:%*[ \t]%s%*[ \t\n]", test_type);
  if (test_type == "RUN-AS-PIKE-SCRIPT")
    return ScriptTestsuite(fn);

  tests = String.trim(tests);
  if(!sizeof(tests)) return 0;

  if( fn=="testsuite" || has_suffix(fn, "/testsuite") )
    return M4Testsuite(tests, fn);

  log_msg("Unable to make sense of test file %O.\n", fn);
  return 0;
}

mapping(string:int) pushed_warnings = ([]);

class WarningFlag {
  int(0..1) warning;
  array(string) warnings = ({});

  void compile_warning(string file, int line, string text) {
    if (pushed_warnings[text]) return;
    if (sizeof(filter(indices(pushed_warnings), glob, text))) return;
    warnings += ({ sprintf("%s:%d: Warning: %s", file, line, text) });
    warning = 1;
  }

  void write_warnings(string fname, string source)
  {
    log_msg("%s produced warning.\n"
            "%{%s\n%}", fname, warnings);
    print_code(source);
  }

  void compile_error(string file, int line, string text) {
    log_msg("%s:%d: Error: %s\n", file,line,text);
  }
}

//
// Watchdog stuff
//

#ifndef WATCHDOG_TIMEOUT
// 10 minutes should be enough...
#if !constant(_reset_dmalloc)
#define WATCHDOG_TIMEOUT 60*10
#else
// ... unless we're running dmalloc
#define WATCHDOG_TIMEOUT 60*40
#endif
#endif

// FIXME: Should make WATCHDOG_TIMEOUT even larger when running in valgrind.

#define WATCHDOG_MSG(fmt, x...) log_msg ("[WATCHDOG]: " fmt, x)

#if 0
#define WATCHDOG_DEBUG_MSG(fmt, x...) WATCHDOG_MSG (fmt, x)
#else
#define WATCHDOG_DEBUG_MSG(fmt, x...) 0
#endif

int use_watchdog=1;
int watchdog_time;

void send_watchdog_command (string cmd, mixed... args)
{
  watchdog_time=time();
  write ("WD " + cmd + "\n", @args);
}

void signal_watchdog()
// Signals activity to the watchdog.
{
  if(use_watchdog && time() - watchdog_time > 30)
    send_watchdog_command ("ping");
}

void watchdog_new_pid (int pid)
{
  if (use_watchdog)
    send_watchdog_command ("pid %d", pid);
}

void watchdog_start_new_test (string current_test, mixed... args)
// Tells the watchdog that a new test is about to be run. This clears
// the test output buffer if the watchdog is buffering stdout (i.e. is
// not running in verbose mode).
{
  if (use_watchdog)
    send_watchdog_command ("cur " + current_test, @args);
}

void watchdog_show_last_test()
// If the watchdog buffers stdout instead of sending it on right away
// then this will make it send the output from the last test,
// including timestamps at the beginning of each line. Each test is
// only shown once.
{
  if (use_watchdog)
    send_watchdog_command ("show last");
}

class Watchdog
{
  Stdio.File stdin;
  int parent_pid, watched_pid;
  string stdout_buf = "", current_test;
  int verbose, timeout_phase;
  int start_time = time();

  protected inherit WatchdogFilterStream;

  string format_timestamp()
  {
    int t = time() - start_time;
    return sprintf ("%02d:%02d:%02d", t / 3600, t / 60 % 60, t %60);
  }

  void stdin_read (mixed ignored, string in)
  {
    WATCHDOG_DEBUG_MSG ("Reset timeout at %s", ctime (time()));
    remove_call_out (timeout);
    call_out (timeout, WATCHDOG_TIMEOUT);

    in = filter (in);
    foreach (get_cmds(), string wd_cmd) {
      if (wd_cmd == "ping") {}
      else if (sscanf (wd_cmd, "pid %d", int new_pid)) {
	watched_pid = new_pid;
	timeout_phase = 0;
	WATCHDOG_DEBUG_MSG ("Changed watched pid to %d\n", watched_pid);
      }
      else if (sscanf (wd_cmd, "cur %s", wd_cmd)) {
	if (wd_cmd != "") current_test = wd_cmd;
	stdout_buf = "";
	WATCHDOG_DEBUG_MSG ("New test: %s\n", current_test);
      }
      else if (wd_cmd == "show last") {
	WATCHDOG_DEBUG_MSG ("Showing output from last test\n");
	log_msg (stdout_buf);
	stdout_buf = "";
      }
      else
	WATCHDOG_MSG ("Got unknown command: %O\n", wd_cmd);
    }

    if (in != "") {
      if (verbose)
	write (in);
      else {
	// Buffer up to 100 kb of stdout noise applying to the last
	// test, so it can be printed out if it hangs or fails.

	// Do away with "\r         \r" lines and convert remaining \r's to \n.
	array(string) rl = in / "\r";
	for (int i = 0; i < sizeof (rl) - 2; i++)
	  if (sscanf (rl[i], "%*[ ]%*c") == 1) rl[i] = 0;
	in = (rl - ({0})) * "\n";

	string ts = "[" + format_timestamp() + "] ";
	if (stdout_buf == "" || has_suffix (stdout_buf, "\n"))
	  stdout_buf += ts;
	int nl = has_suffix (in, "\n");
	in = replace (in[..<nl], "\n", "\n" + ts) + in[<nl - 1..];
	stdout_buf += in;
	while (sizeof (stdout_buf) > 100000 &&
	       sscanf (stdout_buf, "%*s\n%s", stdout_buf) == 2) {}
      }
    }
  }

  void stdin_close (mixed ignored)
  {
    if (stdin->errno()) {
      WATCHDOG_MSG ("Error reading stdin pipe: %s.\n",
		    strerror (stdin->errno()));
    }
    if (!timeout_phase) {
      _exit(EXIT_OK);
    } else {
      _exit(EXIT_WATCHDOG_FAILED);
    }
  }

  void check_parent_pid()
  {
    if (!kill (parent_pid, 0)) {
      WATCHDOG_DEBUG_MSG ("Parent process %d gone - exiting\n", parent_pid);
      _exit(EXIT_OK);
    }
    call_out (check_parent_pid, 10);
  }

  void timeout()
  {
    WATCHDOG_DEBUG_MSG ("Timeout, phase: %d\n", timeout_phase);

    if (watched_pid != parent_pid && !kill(watched_pid, 0)) {
      WATCHDOG_DEBUG_MSG ("Watched process %d gone\n", watched_pid);
      // Automatically go back to watch our parent. If it doesn't
      // respond, it goes too.
      watched_pid = parent_pid;
      timeout_phase = 0;
      call_out (timeout, WATCHDOG_TIMEOUT);
    }

    else {
      string ts = format_timestamp();
      switch (timeout_phase) {
	case 0:
	  WATCHDOG_MSG ("%s: Pike testsuite timeout.", ts);
	  if (current_test) {
	    WATCHDOG_MSG ("Current test: %s", current_test);
	    current_test = 0;
	  }
	  if (stdout_buf != "") {
	    WATCHDOG_MSG ("Output from last test:\n");
	    log_msg (stdout_buf);
	    stdout_buf = "";
	  }
	  WATCHDOG_MSG ("%s: Sending SIGABRT to %d.\n", ts, watched_pid);
	  kill(watched_pid, signum("SIGABRT"));
	  timeout_phase = 1;
	  call_out (timeout, 60);
	  break;

	case 1:
	  WATCHDOG_MSG ("%s: This is your friendly watchdog again...", ts);
	  WATCHDOG_MSG ("Testsuite failed to die from SIGABRT, "
			"sending SIGKILL to %d.\n", watched_pid);
	  kill(watched_pid, signum("SIGKILL"));
	  timeout_phase = 2;
	  call_out (timeout, 60);
	  break;

	case 2:
	  WATCHDOG_MSG ("%s: This is your friendly watchdog AGAIN...", ts);
	  WATCHDOG_MSG ("SIGKILL, SIGKILL, SIGKILL, DIE, %d!\n", watched_pid);
	  kill(watched_pid, signum("SIGKILL"));
	  sleep (0.1);
	  kill(watched_pid, signum("SIGKILL"));
	  sleep (0.1);
	  kill(watched_pid, signum("SIGKILL"));
	  sleep (0.1);
	  kill(watched_pid, signum("SIGKILL"));
	  timeout_phase = 3;
	  call_out (timeout, 60);
	  break;

	case 3:
	  WATCHDOG_MSG ("%s: Giving up on %d, must be a device wait.. :(\n",
			ts, watched_pid);
	  break;
      }
    }
  }

  void reader_thread(Stdio.File watchdog_pipe)
  {
    while(1) {
      string data = watchdog_pipe->read(1024, 1);
      if (!data || (data=="")) break;
      mixed err = catch { stdin_read(0, data); };
      if (err) master()->report_error(err);
    }
    mixed err = catch { stdin_close(0); };
    if (err) master()->report_error(err);
  }

  protected void create (int pid, int verbose)
  {
    parent_pid = watched_pid = pid;
    this::verbose = verbose;
    WATCHDOG_DEBUG_MSG ("Watchdog started.\n");
    stdin = Stdio.File ("stdin");
#ifdef __NT__
#if constant(thread_create)
    // Non-blocking I/O on non-sockets is not supported on NT.
    // Simulate with a thread that performs a blocking read.
    thread_create(reader_thread, stdin);
#endif
#else /* !__NT__ */
    stdin->set_nonblocking (stdin_read, 0, stdin_close);
#endif /* __NT__ */
    call_out (check_parent_pid, 10);
    call_out (timeout, WATCHDOG_TIMEOUT);
  }
}

array(string) find_test(string ts)
{
  if(Stdio.is_file(ts))
    return ({ ts });
  if(Stdio.is_dir(ts))
    return find_testsuites(ts);

  // Let's DWIM
  string try;
  if(Stdio.is_file(try="tlib/modules/"+replace(ts, ".", ".pmod/")+".pmod/testsuite"))
    return ({ try });
  if(Stdio.is_file(try="../../lib/modules/"+replace(ts, ".", ".pmod/")+".test"))
    return ({ try });
  if(Stdio.is_file(try="modules/"+ts+"/testsuite")) return ({ try });
  if(Stdio.is_file(try="post_modules/"+ts+"/testsuite")) return ({ try });
  return ({});
}

//
// Plugins
//

class WidenerPlugin
{
  inherit Plugin;

  @Pike.Annotations.Implements(Plugin);

  int shift;

  int(0..1) active(Test t)
  {
    shift = t->number % 3;
    return !!shift;
  }

  string process_name(string name)
  {
    return sprintf("%s (shift %d)", name, shift);
  }

  string preprocess(string source)
  {
    string widener = ([ 0:"",
                        1:"\nint \x30c6\x30b9\x30c8=0;\n",
                        2:"\nint \x10001=0;\n" ])[shift];
    return source + "\n" + widener;
  }
}

class SaveParentPlugin
{
  inherit Plugin;

  @Pike.Annotations.Implements(Plugin);

  int(0..1) active(Test t)
  {
    if( has_value(t->source, "don't save parent") ) return 0;
    return 0; // Disabled for now.
    return (t->number/6)&1;
  }

  string process_name(string name)
  {
    return name + " (save parent)";
  }

  string preprocess(string source)
  {
    return "#pragma save_parent\n# 1\n" + source;
  }
}

class CRNLPlugin
{
  inherit Plugin;

  @Pike.Annotations.Implements(Plugin);

  int(0..1) active(Test t)
  {
    return (t->number/3)&1;
  }

  string process_name(string name)
  {
    return name + " (CRNL)";
  }

  string preprocess(string source)
  {
    return replace(source, "\n", "\r\n");
  }
}

class LinePlugin
{
  inherit Plugin;

  @Pike.Annotations.Implements(Plugin);

  string preprocess(string source)
  {
    if(source[-1]!='\n') source+="\n";

    int computed_line=sizeof(source/"\n");
    foreach((source/"#")[1..], string cpp)
    {
      // FIXME: We could calculate the offset from this value.
      if(has_prefix(cpp,"line") || sscanf(cpp,"%*d"))
      {
        computed_line=0;
        break;
      }
    }

    return source +
      "int __cpp_line=__LINE__; "
      "int __rtl_line=([array(array(int))]backtrace())[-1][1]; "
      "int __computed_line="+computed_line+";\n";
  }

  int(0..1) inspect(Test t, object o)
  {
    if( o->__cpp_line != o->__rtl_line ||
        ( o->__computed_line && o->__computed_line!=o->__cpp_line))
    {
      log_msg(t->name() + " Line numbering failed.\n");
      print_code(t->prepare_source());
      log_msg("   Preprocessed:\n");
      print_code(cpp(t->prepare_source(), t->file));
      log_msg("   CPP lines: %d\n",o->__cpp_line);
      log_msg("   RTL lines: %d\n",o->__rtl_line);
      if(o->__computed_line)
        log_msg("Actual lines: %d\n",o->__computed_line);
      return 0;
    }
    return 1;
  }
}

//
// Custom master
//
// This is mostly used to catch code that triggers
// calls of master()->handle_error().
//

class TestMaster
{
  inherit "/master";

  protected int error_counter;
  int get_and_clear_error_counter()
  {
    int ret = error_counter;
    error_counter = 0;
    return ret;
  }

  void handle_error(array|object trace)
  {
    if (arrayp(trace) && stringp(trace[0]) &&
	has_prefix(trace[0], "Ignore")) {
      return;
    }
    error_counter++;
    log_msg(sprintf("Runtime error: %s\n",
		    describe_backtrace(trace)));
  }

  protected void create(object|void orig_master)
  {
    ::create();

    if (orig_master) {
      environment = orig_master->getenv(-1);
      /* Copy variables from the original master. */
      foreach(indices(orig_master), string varname) {
	catch {
	  this[varname] = orig_master[varname];
	};
	/* Ignore errors when copying functions, etc. */
      }
    }

    programs["/master"] = this_program;
    objects[this_program] = this;
  }
}

//
// Main program
//

int main(int argc, array(string) argv)
{
  int watchdog_pid, subprocess, failed_cond;
  int verbose, prompt, successes, errors, t, check, asmdebug, optdebug;
  int skipped;
  array(string) forked;
  int start, fail, mem;
  int loop=1;
  int end=0x7fffffff;
  string extra_info="";

  object real_master = master();

  replace_master(TestMaster(real_master));
  add_constant("__real_master", real_master);

#if constant(System.getrlimit)
  // Attempt to enable coredumps.
  // Many Linux distributions default to having coredumps disabled.
  catch {
    [ int current, int max ] = System.getrlimit("core");
    if ((current != -1) && ((current < max) || (max == -1))) {
      // Not unlimited, and less than max.
      // Attempt to raise.
      System.setrlimit("core", max, max);
    }
  };
#endif /* constant(System.getrlimit) */

  if(signum("SIGQUIT")>=0)
  {
    signal(signum("SIGQUIT"),lambda()
	   {
	     master()->handle_error( ({"\nSIGQUIT recived, printing backtrace and continuing.\n",backtrace() }) );

	     mapping x=_memory_usage();
	     foreach(sort(indices(x)),string p)
	     {
	       if(sscanf(p,"%s_bytes",p))
	       {
		 log_msg("%20ss:  %8d   %8d Kb\n",p,
			 x["num_"+p+"s"],
			 x[p+"_bytes"]/1024);
	       }
	     }
	   });
  }

  array(string) args=backtrace()[0][3];
  array(string) testsuites=({});

  foreach(Getopt.find_all_options(argv,aggregate(
    ({"no-watchdog",Getopt.NO_ARG,({"--no-watchdog"})}),
    ({"watchdog",Getopt.HAS_ARG,({"--watchdog"})}),
    ({"subproc-start", Getopt.HAS_ARG, ({"--subproc-start"})}),
    ({"help",Getopt.NO_ARG,({"-h","--help"})}),
    ({"verbose",Getopt.MAY_HAVE_ARG,({"-v","--verbose"})}),
    ({"prompt",Getopt.NO_ARG,({"-p","--prompt"})}),
    ({"start",Getopt.HAS_ARG,({"-s","--start-test"})}),
    ({"end",Getopt.HAS_ARG,({"-e","--end-after"})}),
    ({"fail",Getopt.NO_ARG,({"-f","--fail"})}),
    ({"forked",Getopt.NO_ARG,({"-F","--forked"})}),
    ({"loop",Getopt.HAS_ARG,({"-l","--loop"})}),
    ({"trace",Getopt.HAS_ARG,({"-t","--trace"})}),
    ({"check",Getopt.MAY_HAVE_ARG,({"-c","--check"})}),
#if constant(Debug.assembler_debug)
    ({"asm",Getopt.MAY_HAVE_ARG,({"--assembler-debug"})}),
#endif
#if constant(Debug.optimizer_debug)
    ({"opt",Getopt.MAY_HAVE_ARG,({"--optimizer-debug"})}),
#endif
    ({"mem",Getopt.NO_ARG,({"-m","--mem","--memory"})}),
    ({"auto",Getopt.MAY_HAVE_ARG,({"-a","--auto"})}),
    ({"notty",Getopt.NO_ARG,({"-T","--notty"})}),
#ifdef HAVE_DEBUG
    ({"debug",Getopt.MAY_HAVE_ARG,({"-d","--debug"})}),
#endif
    ({"subprocess", Getopt.NO_ARG, ({"--subprocess"})}),
    ({"cond",Getopt.NO_ARG,({"--failed-cond","--failed-conditionals"})}),
    )),array opt)
    {
      switch(opt[0])
      {
	case "no-watchdog":
	  use_watchdog=0;
	  break;

	case "watchdog":
	  watchdog_pid = (int)opt[1];
	  break;

	case "subproc-start":
	  args = (opt[1] / " ") - ({""}) + args;
	  break;

	case "notty":
	  maybe_tty = 0;
	  break;

	case "help":
	  write(doc);
	  return EXIT_OK;

	case "verbose": verbose+=foo(opt[1]); break;
	case "prompt": prompt+=foo(opt[1]); break;
	case "start": start=foo(opt[1]); start--; break;
	case "end": end=foo(opt[1]); break;
	case "fail": fail=1; break;
        case "forked": {
 	  array(string) orig_argv = backtrace()[0][3];
	  int i = search(orig_argv, "-x");
	  if (i < 0) {
	    i = search(orig_argv, argv[0]);
	    if (i < 0) {
	      log_msg("Forked operation failed: Failed to find %O in %O\n",
		      argv[0], orig_argv);
	      break;
	    }
	  } else {
	    // pike -x test_pike
	    i++;
	  }
	  forked = orig_argv[..i];
	  break;
	}
	case "loop": loop=foo(opt[1]); break;
	case "trace": t+=foo(opt[1]); break;
	case "check": check+=foo(opt[1]); break;
        case "asm": asmdebug+=foo(opt[1]); break;
        case "opt": optdebug+=foo(opt[1]); break;
	case "mem": mem=1; break;

        case "auto":
          if(stringp(opt[1]))
            testsuites+=sort(find_testsuites(opt[1]));
          else
            testsuites+=sort(find_testsuites("."));
          break;

        case "subprocess":
	  subprocess = 1;
          break;

        case "cond":
          failed_cond = 1;
          break;

#ifdef HAVE_DEBUG
	case "debug":
	{
	  object p=Stdio.Port();
	  p->bind(0);
	  log_msg("Debug port is: %s\n",p->query_address());
	  sscanf(p->query_address(),"%*s %d",int portno);
	  extra_info+=sprintf(" dport:%d",portno);
	  thread_create(lambda(object p){
	    while(p)
	    {
	      if(object o=p->accept())
	      {
		object q=Stdio.FILE();
		q->assign(o);
		destruct(o);
		Tools.Hilfe.GenericHilfe(q,q);
	      }
	    }
	  },p);
	}
#endif
      }
    }

  int mantissa_bits = Pike.get_runtime_info()->float_size - 9;
  if (mantissa_bits > 23) {
    // 32-bit IEEE has 23 bits mantissa.
    // 64-bit IEEE has 52-bits mantissa.
    // 128-bit IEEE has 112-bits mantissa.
    // IBM 128-bit has 106-bits mantissa.
    mantissa_bits -= 3;
    if (mantissa_bits > 52)
      mantissa_bits -= 10;
  }

  // NB: Allow fluctuation in the least significant decimal digit (ie 4bits).
  float_precision = 1.0/(1<<(mantissa_bits - 5));

  // For easy access in spawned test scripts.
  putenv ("TEST_VERBOSITY", (string) verbose);
  putenv ("TEST_ON_TTY", (string) (maybe_tty && Stdio.Terminfo.is_tty()));

  log_start (verbose, maybe_tty && Stdio.Terminfo.is_tty());

  if (watchdog_pid) {
#if defined(__NT__) && !constant(thread_create)
    log_msg("Watchdog not supported on NT without threads.\n");
    return EXIT_WATCHDOG_FAILED;
#else
    Watchdog (watchdog_pid, verbose);
    return -1;
#endif
  }

  // FIXME: Make this code more robust!
  args=args[..<argc];
  if (sizeof(args) && args[-1] == "-x") {
    // pike -x test_pike
    args = args[..<1];
  }
  add_constant("RUNPIKE_ARRAY", args);
  add_constant("RUNPIKE", map(args, Process.sh_quote)*" ");

  if (forked) {
    if (!use_watchdog) forked += ({ "--no-watchdog" });
    if (!maybe_tty) forked += ({ "--notty" });
    if (verbose) forked += ({ "--verbose=" + verbose });
    if (prompt) forked += ({ "--prompt=" + prompt });
    if (start) forked += ({ "--start-test=" + (start+1) });
    // FIXME: end handling is not quite correct.
    if (end != 0x7fffffff) forked += ({ "--end-after=" + end });
    if (fail) forked += ({ "--fail" });
    // forked is handled here.
    // loop is handled here.
    if (t) forked += ({ "--trace=" + t });
    if (check) forked += ({ "--check=" + check });
    if (asmdebug) forked += ({ "--asm=" + asmdebug });
    if (optdebug) forked += ({ "--asm=" + optdebug });
    if (mem) forked += ({ "--memory" });
    // auto already handled.
    if (failed_cond) forked += ({ "--failed-cond" });
    forked += ({"--subprocess"});
    // debug port not propagated.
    //log_msg("forked:%O\n", forked);
  }

  Process.create_process watchdog;
  if(use_watchdog)
  {
    // A subprocess can assume the watchdog is already installed on stdout.
    if (!subprocess) {
      Stdio.File orig_stdout = Stdio.File();
      Stdio.stdout->dup2 (orig_stdout);
      Stdio.File pipe_1 = Stdio.File();
      // Don't really need PROP_BIDIRECTIONAL, but it's necessary for
      // some reason to make a pipe/socket that the watchdog process can
      // do nonblocking on (Linux 2.6/glibc 2.5). Maybe a bug in the new
      // epoll stuff? /mast
      // No, you had swapped the read and write ends of the pipe. /grubba
#ifdef __NT__
      Stdio.File pipe_2 = pipe_1->pipe(Stdio.PROP_IPC);
#else /* !__NT__ */
      Stdio.File pipe_2 = pipe_1->pipe(Stdio.PROP_IPC | Stdio.PROP_NONBLOCK);
#endif /* __NT__ */
      if (!pipe_2) {
        log_msg ("Failed to create pipe for watchdog: %s.\n",
		 strerror (pipe_1->errno()));
	exit(EXIT_WATCHDOG_FAILED);
      }
      pipe_2->dup2 (Stdio.stdout);
      pipe_2->close();
      watchdog=Process.create_process(
	backtrace()[0][3] + ({ "--watchdog="+getpid() }),
	(["stdin": pipe_1, "stdout": orig_stdout]));
      pipe_1->close();
      orig_stdout->close();
      // Note that the following message gets sent to the watchdog.
      log_msg ("Forked watchdog pid %d.\n", watchdog->pid());
    }
  }

  else {
    // Move stdout to a higher fd, so that close on exec works.
    // This makes sure the original stdout gets closed even if
    // some subprocess hangs.
    Stdio.File stdout = Stdio.File();
    Stdio.stdout->dup2(stdout);
    //stdout->assign(Stdio.stdout->_fd->dup());
    if (verbose)
      Stdio.stderr->dup2(Stdio.stdout);
    else {
#ifdef __NT__
      Stdio.File ("NUL:", "w")->dup2 (Stdio.stdout);
#else
      Stdio.File ("/dev/null", "w")->dup2 (Stdio.stdout);
#endif
    }
    stdout->set_close_on_exec(1);
  }

  add_constant("__signal_watchdog",signal_watchdog);
  add_constant("__watchdog_new_pid", watchdog_new_pid);
  add_constant("__watchdog_start_new_test", watchdog_start_new_test);
  add_constant("__watchdog_show_last_test", watchdog_show_last_test);
  add_constant("__send_watchdog_command", send_watchdog_command);
  add_constant("_verbose", verbose);
  add_constant ("log_msg", log_msg);
  add_constant ("log_msg_cont", log_msg_cont);
  add_constant ("log_status", log_status);

  if(!subprocess)
    log_msg("Begin tests at %s (pid %d)\n", ctime(time())[..<1], getpid());

  foreach(Getopt.get_args(argv, 1)[1..], string ts) {
    array(string) tests = find_test(ts);
    if(!sizeof(tests))
      exit(EXIT_TEST_NOT_FOUND, "Could not find test %O.\n", ts);
    testsuites += tests;
  }

  if(!sizeof(testsuites))
    exit(EXIT_TEST_NOT_FOUND,
	 "No tests found. Use --help for more information.\n");

#if 1
  // Store the name of all constants so that we can see
  // if any constant has been leaked from the testsuite.
  array const_names = indices(all_constants());
#endif

#if constant(Debug.assembler_debug)
  if(asmdebug)
    Debug.assembler_debug(asmdebug);
#endif
#if constant(Debug.optimizer_debug)
  if(optdebug)
    Debug.optimizer_debug(optdebug);
#endif

  while(loop--)
  {
    successes=errors=0;
    if (forked) {
      foreach(testsuites, string testsuite) {
	int failure;
	array(int) subres =
          low_run_script (forked + ({ testsuite }), ([]));
	if (!subres) {
	  log_status ("");
	  log_msg("Failed to spawn testsuite %O.\n", testsuite);
	  errors++;
	  failure = 1;
	} else {
	  [int sub_succeeded, int sub_failed, int sub_skipped] = subres;
	  if (!(sub_succeeded || sub_failed || sub_skipped))
	    continue;
	  if (verbose || sub_failed) {
	    log_status ("");
	    log_msg("Subresult: %d tests, %d failed, %d skipped\n",
		    sub_succeeded + sub_failed, sub_failed, sub_skipped);
	  }
	  successes += sub_succeeded;
	  errors += sub_failed;
	  skipped += sub_skipped;
	  if (sub_failed) failure = 1;
	}
	if ((verbose > 1) || (fail && errors))
	  log_msg("Accumulated: %d tests, %d failed, %d skipped\n",
		  successes + errors, errors, skipped);
	if (fail && errors) {
	  exit(EXIT_TEST_FAILED);
	}
      }
    } else {
  testloop:
    foreach(testsuites, string testsuite)
    {
      Testsuite tests = read_tests( testsuite );
      if (!objectp(tests) || !sizeof(tests))
	continue;

      log_msg("Doing tests in %s (%s)\n",
              tests->name ? tests->name() : testsuite,
              ({sizeof(tests) + " tests",
                0 && subprocess && ("pid " + getpid())}) * ", ");
      int qmade, qskipped, qmadep, qskipp;

      tests->skip(start);
      foreach(tests; int e; Test test)
      {
	if (!((e-start) % 10))
	  report_size();

	int skip=0, prev_errors = errors;
	object o;
	mixed a,b;

	// Extra consistency checks?
	if(check)
        {
	  if(check>0 || (e % -check)==0 )
	      _verify_internals();
	}
	if(check>3) {
	  gc();
	  _verify_internals();
	}

        // Is there a condition for this test?
        if( sizeof(test->conditions) )
        {
          // FIXME: Support more than one condition (current testsuite
          // format only handles one though)
          if( sizeof(test->conditions)>1 )
            error("Only one concurrent condition supported.\n");

          int tmp;
          string condition = test->conditions[0];
	  if(!(tmp=cond_cache[condition]))
	  {
	    mixed err = catch {
              tmp=!!(test->compile("mixed c() { return "+condition+"; }")()->c());
	    };

	    if(err) {
	      if (err && err->is_cpp_or_compilation_error)
                log_msg( "Conditional %d failed.\n", e+1);
	      else
                log_msg( "Conditional %d failed:\n%s\n", e+1,
			 describe_backtrace(err) );
              print_code( condition );
	      errors++;
	      tmp = -1;
	    }

            if (tmp != 1) {
              if ((verbose > 1 || failed_cond) && !err) {
                log_msg("Conditional %d failed:\n", e+1);
		print_code( condition );
	      }
	    } else if (verbose > 5) {
              log_msg("Conditional %d succeeded.\n", e+1);
	      if (verbose > 9) {
		print_code( condition );
	      }
	    }

	    if(!tmp) tmp=-1;
	    cond_cache[condition]=tmp;
	  }

	  if(tmp==-1)
	  {
	    if(verbose>1)
              log_msg("Not doing test "+(e+1)+"\n");
            successes++;
	    skipped++;
	    skip=1;
	  }
	}

        string source = test->source;

        watchdog_start_new_test ("Test %d at %s:%d", e + 1,
                                 test->file, test->line);

	if(maybe_tty && Stdio.Terminfo.is_tty())
        {
	  if(verbose == 1) {
	    // \r isn't necessary here if everyone uses
	    // Tools.Testsuite.{log_msg|log_status}, but it avoids
	    // messy lines for all the tests that just write directly.
	    log_status ("test %d, line %d\r", e+1, test->line);
	  }
	}
	else if(verbose > 1){
	  if(skip) {
	    if(qmade)
	      log_msg("Made %d test%s.\n", qmade, qmade==1?"":"s");
	    qmade=0;
	    qskipp=1;
	    qskipped++;
	  }
	  else {
	    if(qskipped)
	      log_msg("Skipped %d test%s.\n", qskipped, qskipped==1?"":"s");
	    qskipped=0;
	    qmadep=1;
	    qmade++;
	  }
	}
	else if (verbose) {
	  /* Use + instead of . so that sendmail and
	   * cron will not cut us off... :(
	   */
	  switch( (e-start) % 50)
	  {
	  case 0:
	    log_msg("%5d: ", e);
	    // fallthrough

	  default:
	    log_msg_cont(skip?"-":"+");
	    break;

	  case 9:
	  case 19:
	  case 29:
	  case 39:
	    log_msg_cont(skip?"- ":"+ ");
	    break;

	  case 49:
	    log_msg_cont(skip?"-\n":"+\n");
	  }
	}
	if(skip) continue;

        test->add_plugin( WidenerPlugin() );
        test->add_plugin( SaveParentPlugin() );
        test->add_plugin( CRNLPlugin() );
        test->add_plugin( LinePlugin() );

	if (prompt) {
	  if (Stdio.Readline()->
	      read(sprintf("About to run test: %d. [<RETURN>/'quit']: ",
			   test->number)) == "quit") {
	    break testloop;
	  }
	}

	if(verbose>1)
	{
          log_msg("Doing test %d (%d total) at %s:%d%s\n", test->number,
                  successes+errors+1, test->file, test->line, extra_info);
	  if(verbose>2) print_code(source);
	}

	if(check > 1) _verify_internals();

        string fname = test->name();

        if(verbose>9) print_code(test->prepare_source());
        switch(test->type)
        {
          WarningFlag wf;
          mixed at,bt;
	  mixed err;
	case "COMPILE":
	  wf = WarningFlag();
          test->inhibit_errors = wf;
          test->compile();
          if(test->compilation_error)
	  {
            if (objectp(test->compilation_error) &&
		test->compilation_error->is_cpp_or_compilation_error)
	      log_msg ("%s failed.\n", fname);
	    else
              log_msg ("%s failed:\n%s", fname,
                       describe_backtrace (test->compilation_error));
	    print_code(source);
	    errors++;
	  }
          else
          {
            if(wf->warning) {
              wf->write_warnings(fname, source);
              errors++;
	      break;
	    }

	    successes++;
	  }
	  break;

	case "COMPILE_ERROR":
          test->compile();
          if(test->compilation_error)
	  {
            if (objectp(test->compilation_error) &&
		test->compilation_error->is_cpp_or_compilation_error) {
              successes++;
	    }
	    else {
              log_msg ("%s failed.\n"
		       "Expected compile error, got another kind of error:\n%s",
                       fname, describe_backtrace (test->compilation_error));
	      print_code(source);
	      errors++;
	    }
	  }
	  else {
            log_msg (fname + " failed (expected compile error).\n");
	    print_code(source);
	    errors++;
	  }
          break;

	case "COMPILE_WARNING":
	  wf = WarningFlag();
          test->inhibit_errors = wf;
          test->compile();
          if(test->compilation_error)
	  {
            if (objectp(test->compilation_error) &&
		test->compilation_error->is_cpp_or_compilation_error)
	      log_msg ("%s failed.\n", fname);
	    else
              log_msg ("%s failed:\n%s", fname,
                       describe_backtrace (test->compilation_error));
	    print_code(source);
	    errors++;
	  }
	  else {
            if( wf->warning )
	      successes++;
	    else {
	      log_msg(fname + " failed (expected compile warning).\n");
	      print_code(source);
	      errors++;
	    }
	  }
          break;

	case "EVAL_ERROR":
          at = gauge {
	    err=catch {
	      // Is it intentional that compilation errors are
	      // considered success too? /mast
	      // Yes, apparently it is. There are tests that don't
	      // care whether the error is caught during compilation
              // or evaluation. /mast
                a = test->compile()()->a();
            };
          };
          if(err)
	  {
            successes++;
	    if(verbose>3)
	      log_msg("Time in a(): %f\n",at);
	  }
	  else {
	    watchdog_show_last_test();
            log_msg("%s failed (expected eval error).\n"
		    "Got %O\n", fname, a);
	    print_code(source);
	    errors++;
	  }
          break;

	default:
	  if (err = catch{
	    wf = WarningFlag();
            test->inhibit_errors = wf;
            o=test->compile()();
            if(test->compilation_error)
              throw(test->compilation_error);

	    if(check > 1) _verify_internals();

	    a=b=0;
	    if(t) trace(t);

	    if(functionp(o->a))
	    {
	      // trace(10);
	      at = gauge { a=o->a(); };
	      // trace(0);
	    }

	    if(functionp(o->b))
	    {
	      bt = gauge { b=o->b(); };
	    }

	    if(t) trace(0);
	    if(check > 1) _verify_internals();

            if(wf->warning) {
              wf->write_warnings(fname, source);
              errors++;
	      break;
	    }
          }) {
	    if(t) trace(0);
            watchdog_show_last_test();
            if (objectp(test->compilation_error) &&
		test->compilation_error->is_cpp_or_compilation_error)
	      log_msg ("%s failed.\n", fname);
	    else
              log_msg ("%s failed:\n%s\n", fname,
                       describe_backtrace (test->compilation_error||err));
	    print_code(source);
	    errors++;
	    break;
          }

          foreach(test->plugins;; Plugin plugin)
            if( plugin->inspect && !plugin->inspect(test, o) )
              errors++;

	  if(verbose>2)
	    log_msg("Time in a(): %f, Time in b(): %O\n",at,bt);

	  switch(test->type)
	  {
	  case "FALSE":
	    if(a)
	    {
	      watchdog_show_last_test();
	      log_msg(fname + " failed.\n");
	      print_code(source);
	      log_msg_result("o->a(): %O\n",a);
	      errors++;
	    }
	    else {
	      successes++;
	    }
	    break;

	  case "TRUE":
	    if(!a)
	    {
	      watchdog_show_last_test();
	      log_msg(fname + " failed.\n");
	      print_code(source);
	      log_msg_result("o->a(): %O\n",a);
	      errors++;
	    }
	    else {
	      successes++;
	    }
	    break;

	  case "PUSH_WARNING":
	    if (!stringp(a)) {
	      watchdog_show_last_test();
	      log_msg(fname + " failed.\n");
	      print_code(source);
	      log_msg_result("o->a(): %O\n", a);
	    } else {
	      pushed_warnings[a]++;
	    }
	    break;

	  case "POP_WARNING":
	    if (!stringp(a)) {
	      watchdog_show_last_test();
	      log_msg(fname + " failed.\n");
	      print_code(source);
	      log_msg_result("o->a(): %O\n", a);
	    } else if (pushed_warnings[a]) {
	      if (!--pushed_warnings[a]) {
		m_delete(pushed_warnings, a);
	      }
	    } else {
	      watchdog_show_last_test();
	      log_msg(fname + " failed.\n");
	      print_code(source);
	      log_msg_result("o->a(): %O not pushed!\n", a);
	    }
	    break;

	  case "RUN":
	    successes++;
	    break;

	  case "RUNCT":
	    if(!a || !arrayp(a) ||
	       sizeof(a) < 2 || !intp(a[0]) || !intp(a[1]) ||
	       (sizeof (a) == 3 && !intp (a[2])) ||
	       sizeof (a) > 3) {
	      watchdog_show_last_test();
	      log_msg(fname + " failed to return proper results.\n");
	      print_code(source);
	      log_msg_result("o->a(): %O\n",a);
	      errors++;
	    }
	    else {
	      successes += a[0];
	      errors += a[1];
	      if (sizeof (a) >= 3) skipped += a[2];
	      if ((verbose>1) || a[1])
		if(a[1])
		  log_msg("%d/%d tests failed%s.\n",
		      a[1], a[0]+a[1],
		      sizeof (a) >= 3 ? " (skipped " + a[2] + ")" : "");
		else
		  log_msg("Did %d tests in %s%s.\n", a[0], fname,
		      sizeof (a) >= 3 ? " (skipped " + a[2] + ")" : "");
	    }
	    break;

	  case "EQ":
            if(a==b)
            {
	      successes++;
	    }
            else {
	      watchdog_show_last_test();
	      log_msg(fname + " failed.\n");
	      print_code(source);
	      log_msg_result("o->a()%s: %O\n"
                             "o->b()%s: %O\n",
			     (stringp(a)||arrayp(a))?
			     sprintf(" [%d elements]", sizeof(a)):"", a,
			     (stringp(b)||arrayp(b))?
			     sprintf(" [%d elements]", sizeof(b)):"", b);
	      errors++;
	      if (stringp(a) && stringp(b) && (sizeof(a) == sizeof(b)) &&
		  (sizeof(a) > 20)) {
		log_msg("Differences at:\n");
		int i;
		for(i = 0; i < sizeof(a); i++) {
		  if (a[i] != b[i]) {
		    log_msg("  %4d: 0x%04x != 0x%04x\n", i, a[i], b[i]);
		  }
		}
	      }
#if 0 && constant(_dump_program_tables)
	      _dump_program_tables(object_program(o));
#endif
	    }
            break;

	  case "EQUAL":
            if(equal(a,b))
            {
	      successes++;
	    }
            else {
	      watchdog_show_last_test();
	      log_msg(fname + " failed.\n");
	      print_code(source);
	      log_msg_result("o->a()%s: %O\n"
                             "o->b()%s: %O\n",
			     (stringp(a)||arrayp(a))?
			     sprintf(" [%d elements]", sizeof(a)):"", a,
			     (stringp(b)||arrayp(b))?
			     sprintf(" [%d elements]", sizeof(b)):"", b);
	      errors++;
	      if (stringp(a) && stringp(b) && (sizeof(a) == sizeof(b)) &&
		  (sizeof(a) > 20)) {
		log_msg("Differences at:\n");
		int i;
		for(i = 0; i < sizeof(a); i++) {
		  if (a[i] != b[i]) {
		    log_msg("  %4d: 0x%04x != 0x%04x\n", i, a[i], b[i]);
		  }
		}
              } else if (arrayp(a) && arrayp(b) && (sizeof(a) == sizeof(b))) {
                log_msg("Differences at:\n");
                int i;
                for(i = 0; i < sizeof(a); i++) {
                  if (!equal(a[i], b[i])) {
                    log_msg("  %4d: %O != %O\n", i, a[i], b[i]);
                  }
                }
	      }
	    }
            break;

          case "APPROX":
            int match = 0;
            float delta = 1.0;
            if (floatp(a) && floatp(b)) {
              if (a != 0.0) {
                if (a > 0.0) {
                  delta = a;
                } else {
                  delta = -a;
                }
              }
              if (b != 0.0) {
                if (b > 0.0) {
                  if (b < delta) {
                    delta = b;
                  }
                } else {
                  if (-b < delta) {
                    delta = -b;
                  }
                }
              } else {
                // When comparing against 0.0, we allow for
                // a float_precision discrepancy.
                if (delta > 1.0) {
                  delta = 1.0;
                }
              }
              delta *= float_precision;
              match = ((a + delta >= b) && (a - delta <= b));
            } else {
              // For convenience, allow returning eg strings
              // to signal errors.
              match = equal(a, b);
            }
            if (match) {
              successes++;
            }
            else {
              watchdog_show_last_test();
              log_msg(fname + " failed.\n");
              print_code(source);
              log_msg_result("o->a(): %O\n"
                             "o->b(): %O\n",
                             a, b);
              if (floatp(a) && floatp(b)) {
                log_msg_result("diff (a - b): %O\n"
                               "expected precision: %O\n",
                               a - b, delta);
              }
              errors++;
            }
            break;

	  default:
	    log_msg("%s: Unknown test type (%O).\n", fname, test->type);
	    errors++;
	  }

	  // Count asynchronously generated runtime errors too.
	  int runtime_errors = master()->get_and_clear_error_counter();
	  if (runtime_errors) {
	    // The errors have already been reported.
	    log_msg(fname + " caused runtime errors.\n");
	    print_code(source);
	    errors += runtime_errors;
	  }
	}

	if(check > 2) _verify_internals();

	if(fail && errors)
	  exit(EXIT_TEST_FAILED);

	if(successes+errors > end)
	{
	  break testloop;
	}

	a=b=0;
      }

      if(maybe_tty && Stdio.Terminfo.is_tty()) {}
      else if(verbose > 1) {
	if(!qskipp && !qmadep);
	else if(!qskipp)
	  log_msg("Made all tests\n");
	else if(!qmadep)
	  log_msg("Skipped all tests\n");
	else if(qmade)
	  log_msg(" Made %d test%s.\n", qmade, qmade==1?"":"s");
	else if(qskipped)
	  log_msg(" Skipped %d test%s.\n", qskipped, qskipped==1?"":"s");
      }
      else if (verbose) {
	log_msg("\n");
      }
    }
    }
    report_size();
    if(mem)
    {
      int total;
      gc();
      mapping tmp=_memory_usage();
      log_msg("%-10s: %6s %10s\n","Category","num","bytes");
      foreach(sort(indices(tmp)),string foo)
      {
	if(sscanf(foo,"%s_bytes",foo))
	{
	  log_msg("%-10s: %6d %10d\n",
		  foo+"s",
		  tmp["num_"+foo+"s"],
		  tmp[foo+"_bytes"]);
	  total+=tmp[foo+"_bytes"];
	}
      }
      log_msg( "%-10s: %6s %10d\n",
	       "Total", "", total );
    }
  }

  if (!subprocess) {
    log_status ("");

    if(errors || verbose>1)
    {
      log_msg("Failed tests: "+errors+".        \n");
    }

    log_msg("Total tests: %d (%d tests skipped)\n", successes+errors, skipped);
    if(verbose)
      log_msg("Finished tests at "+ctime(time()));
  }
  else {
    // Clear the output buffer in the watchdog so that
    // Tools.Testsuite.low_run_script doesn't print it when errors is
    // nonzero. That has instead been handled above for each failing
    // test.
    watchdog_start_new_test ("");

    report_result (successes, errors, skipped);
  }

#if 1
  if(verbose && sizeof(all_constants())!=sizeof(const_names)) {
    multiset const_names = (multiset)const_names;
    foreach(indices(all_constants()), string const_name)
      if( !const_names[const_name] )
	log_msg("Leaked constant %O\n", const_name);
  }
#endif

  add_constant("_verbose");
  add_constant("__signal_watchdog");
  add_constant("RUNPIKE");

  if(watchdog)
  {
    Stdio.stdout->close();
    watchdog->wait();
  }

  return errors && EXIT_TEST_FAILED;
}

constant doc = #"
Usage: test_pike [args] [testfiles]

--no-watchdog       Watchdog will not be used.
--watchdog=pid      Run only the watchdog and monitor the process with
                    the given pid.
--subproc-start=X   Whenever subprocesses are spawned to run tests, they get
                    started with X in front of the pike binary. X is split on
                    spaces to form an argument list. A useful example for X is
                    valgrind.
-h, --help          Prints this message.
-v[level],
--verbose[=level]   Select the level of verbosity. Every verbosity level
                    includes the printouts from the levels below.
                    0  No extra printouts.
                    1  Some additional information printed out after every
                       finished block of tests.
                    2  Some extra information about tests that will or won't be
                       run.
                    3  Every test is printed out.
                    4  Time spent in individual tests is printed out.
                    10 The actual Pike code compiled, including wrappers, is
                       printed.
-p, --prompt        The user will be asked before every test is run.
-sX, --start-test=X Where in the testsuite testing should start, i.e. ignores X
                    tests in every testsuite.
-eX, --end-after=X  How many tests should be run.
-f, --fail          If set, the test program exits on first failure.
-F, --forking       If set, each testsuite will run in a separate process.
-lX, --loop=X       The number of times the testsuite should be run. Default is
                    1.
-tX, --trace=X      Run tests with trace level X.
-c[X], --check[=X]  The level of extra Pike consistency checks performed.
                    1   _verify_internals is run before every test.
                    2   _verify_internals is run after every compilation.
                    3   _verify_internals is run after every test.
                    4   An extra gc and _verify_internals is run before
                        every test.
                    X<0 For values below zero, _verify_internals will be run
                        before every n:th test, where n=abs(X).
-m, --mem, --memory Print out memory allocations after the tests.
-a, --auto[=dir]    Let the test program find the testsuites automatically.
-T, --notty         Format output for non-tty.
-d, --debug         Opens a debug port.
--failed-cond       Outputs failing test conditionals.
";
