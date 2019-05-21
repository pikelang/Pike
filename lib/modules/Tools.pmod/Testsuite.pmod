// Some tools for tests in Pikes selftest testsuite.

#pike __REAL_VERSION__

array(int) run_script (string|array(string) pike_script)
//! Runs an external pike script from a testsuite. Use
//! @[Tools.Testsuite.report_result] in the script to report how many
//! tests succeeded, failed, and were skipped. Those numbers are
//! returned in the array from this function.
{
  if (!all_constants()->RUNPIKE_ARRAY)
    error ("Can only be used within the testsuite.\n");
  return
    low_run_script (all_constants()->RUNPIKE_ARRAY +
		    (stringp (pike_script) ? ({pike_script}) : pike_script),
		    ([])) ||
    ({0, 1, 0});
}

protected int verbosity =
  lambda () {
    string v = getenv()->TEST_VERBOSITY;
    // Default to 1 in case test scripts are run standalone.
    return undefinedp (v) ? 1 : (int) v;
  }();
protected int on_tty =
  lambda () {
    string v = getenv()->TEST_ON_TTY;
    return undefinedp (v) ? 1 : (int) v;
  }();

protected string last_log;
// The last message passed to log_status if verbosity == 0 (always
// destined for stdout).

protected int last_line_length;
// The length of the last logged line that didn't end with a newline.
// Positive if it was logged to stdout, negative for stderr.

protected int last_line_inplace =
  lambda () {
    if (verbosity == 1 && !undefinedp (getenv()->TEST_VERBOSITY))
      // Initialize to 1 on verbosity level 1 when it looks like we're
      // being run from the main testsuite, since it typically logs an
      // "in place" message just before spawning the subtest.
      return 1;
    return 0;
  }();
// Set if the last line was logged by log_status (to stdout) on
// verbosity level 1. last_line_length is never negative when this is
// set.

void log_start (int verbosity, int on_tty)
{
  this::verbosity = verbosity;
  this::on_tty = on_tty;
  last_line_inplace = 0;
}

protected Thread.Mutex log_mutex = Thread.Mutex();
protected int twiddler_counter = -1;
protected int twiddle_time_base = time();
protected float last_twiddle;

protected void unlocked_log_msg_cont (string msg)
{
  if (last_line_inplace && last_line_length) {
    write ("\n");
    last_line_length = 0;
  }

  last_log = 0;
  if( String.width(msg)>8 )
    msg = string_to_utf8(msg);
  werror (msg);
  twiddler_counter = -abs (twiddler_counter);
  last_line_inplace = 0;

  if (has_suffix (msg, "\n") || has_suffix (msg, "\r"))
    last_line_length = 0;
  else {
    array(string) msg_lines = msg / "\n";
    if (sizeof (msg_lines) > 1) last_line_length = 0;
    // FIXME: This length calculation is too simplistic since it
    // assumes 1 char == 1 position.
    last_line_length -= sizeof (msg_lines[-1]);
  }
}

// Cut strings to below 4K
void log_msg_result (string msg, mixed ... args)
{
  array a = msg/"%";
  foreach(a; int pos; string part)
  {
    if(pos==0) continue;
    if(has_prefix(part, "O") && pos-1<=sizeof(args) &&
       stringp(args[pos-1]) && sizeof(args[pos-1])>4000)
    {
      part[0] = 's';
      a[pos] = part;
      args[pos-1] = sprintf("%O...", args[pos-1][..4000-1]);
    }
  }
  log_msg(a*"%", @args);
}

void log_msg (string msg, mixed... args)
//! Logs a testsuite message to stderr. The message is shown
//! regardless of the verbosity level. If the previous message was
//! logged without a trailing newline then a newline is inserted
//! first.
//!
//! The message should normally have a trailing newline - no extra
//! newline is added to it. Use @[log_status] to log a message
//! intended to be overwritten.
{
  if (sizeof (args)) msg = sprintf (msg, @args);

  Thread.MutexKey lock = log_mutex->lock();

  if (last_line_length) {
    if (last_line_length < 0) werror ("\n"); else write ("\n");
    last_line_length = 0;
  } else if (last_line_inplace) {
    write("\n");
    last_line_inplace = 0;
  }

  if (last_log) {
    if (!has_suffix (last_log, "\n")) last_log += "\n";
    werror (last_log);
    last_log = 0;
  }

  unlocked_log_msg_cont (msg);
}

void log_msg_cont (string msg, mixed... args)
//! Similar to @[log_msg], but doesn't insert a newline first if the
//! previous message didn't end with one. Does however insert a
//! newline if the previous message was logged "in place" by
//! @[log_status].
{
  if (sizeof (args)) msg = sprintf (msg, @args);
  Thread.MutexKey lock = log_mutex->lock();
  unlocked_log_msg_cont (msg);
}

void log_status (string msg, mixed... args)
//! Logs a testsuite status message to stdout. This is suitable for
//! nonimportant progress messages.
//!
//! @ul
//! @item
//!   If the verbosity is 0 then nothing is logged, but the message is
//!   saved and will be logged if the next call is to @[log_msg].
//! @item
//!   If the verbosity is 1, the message is "in place" on the current
//!   line without a trailing line feed, so that the next
//!   @[log_status] message will replace this one.
//! @item
//!   If the verbosity is 2 or greater, the message is logged with a
//!   trailing line feed.
//! @endul
//!
//! The message should be short and not contain newlines, to work when
//! the verbosity is 1, but if it ends with a newline then it's logged
//! normally. It can be an empty string to just clear the last "in
//! place" logged message - it won't be logged otherwise.
//!
//! @seealso
//! @[log_twiddler]
{
  if (sizeof (args)) msg = sprintf (msg, @args);

  Thread.MutexKey lock = log_mutex->lock();

  switch (verbosity) {
    case 0:
      last_log = (msg != "" && msg);
      return;

    case 1:
      if (!on_tty) {
	// Logging to a file or something, so our \r stunts won't look
	// good. Let's store it like for verbosity == 0; if the user
	// wants to log all these then (s)he should use verbosity > 1.
	last_log = (msg != "" && msg);
	return;
      }

      if (last_line_length < 0)
	werror ("\n");
      else if (last_line_inplace)
	write ("\r%*s\r", max (last_line_length, 40), "");
      else if (last_line_length > 0)
	write ("\n");
      twiddler_counter = -abs (twiddler_counter);
      write (msg);
      last_line_inplace = 1;
      break;

    default:
      if (msg == "")
	return;
      if (last_line_length < 0)
	werror ("\n");
      if (!has_suffix (msg, "\n")) msg += "\n";
      write (msg);
      last_line_inplace = 0;
      break;
  }

  if (has_suffix (msg, "\n") || has_suffix (msg, "\r"))
    last_line_length = 0;
  else {
    // FIXME: This length calculation is too simplistic since it
    // assumes 1 char == 1 position.
    last_line_length = sizeof ((msg / "\n")[-1]);
  }
}

void log_twiddler()
//! Logs a rotating bar for showing progress. Only output at the end
//! of an "in place" message written by @[log_status] on verbosity
//! level 1.
{
  if (verbosity != 1 || last_line_length <= 0 || !on_tty) return;

  float now = time (twiddle_time_base), t = now - last_twiddle;

  if (twiddler_counter < 0) {
    write (" ");
    last_line_length += 2;
    twiddler_counter = -twiddler_counter;
  }
  else if (t < 0.2) return;

  if (t >= 0.2) {
    twiddler_counter++;
    last_twiddle = now;
  }

  write ("%c\b", "|/-\\"[twiddler_counter & 3]);
}

void report_result (int succeeded, int failed, void|int skipped)
//! Use this to report the number of successful, failed, and skipped
//! tests in a script started using @[run_script]. Can be called
//! multiple times - the counts are accumulated.
{
  // DWIM: Under the assumption that report_result is done last thing
  // in the test, clear any status message before returning.
  log_status ("");

  // Not really a watchdog command, but sent using the same format.
  write ("WD succeeded %d failed %d skipped %d\n",
	 succeeded, failed, skipped);
}

class WatchdogFilterStream
// Filter out watchdog commands on the form "WD <cmd>\n", passing
// everything else through.
{
  protected array(string) wd_cmds = ({});
  protected string cmd_buf;

  string filter (string in)
  {
    string out_buf = "";
    do {
      if (cmd_buf) {
	cmd_buf += in;
	in = "";
	if (sscanf (cmd_buf, "WD %s\n%s", string wd_cmd, in) == 2) {
	  wd_cmds += ({wd_cmd});
	  cmd_buf = 0;
	}
	else if (has_value (cmd_buf, "\n")) {
	  in = cmd_buf;
	  cmd_buf = 0;
	}
	else break;
      }
      if (!cmd_buf) {
	int i = search (in, "WD ");
	if (i > -1) cmd_buf = in[i..], in = in[..i-1];
	else if (has_suffix (in, "WD")) cmd_buf = "WD", in = in[..<2];
	else if (has_suffix (in, "W")) cmd_buf = "W", in = in[..<1];
	out_buf += in;
	in = "";
      }
    } while (cmd_buf);
    return out_buf;
  }

  array(string) get_cmds()
  {
    array(string) res = wd_cmds;
    wd_cmds = ({});
    return res;
  }
}

array(int) low_run_script (array(string) command, mapping opts)
{
  if (!all_constants()->__watchdog_new_pid)
    error ("Can only be used within the testsuite.\n");

  Stdio.File p = Stdio.File();
  // Shouldn't need PROP_BIDIRECTIONAL, but I don't get a pipe/socket
  // that I can do nonblocking on otherwise (Linux 2.6, glibc 2.5). /mast
#ifdef __NT__
  Stdio.File p2 = p->pipe(Stdio.PROP_IPC);
#else /* !__NT__ */
  Stdio.File p2 = p->pipe (Stdio.PROP_IPC|
			   Stdio.PROP_NONBLOCK|
			   Stdio.PROP_BIDIRECTIONAL);
#endif /* __NT__ */
  if(!p2) {
    werror("Failed to create pipe: %s.\n", strerror (p->errno()));
    return ({0, 1, 0});
  }

  Process.create_process pid =
    Process.create_process (command, opts + (["stdout": p2]));
  p2->close();
  all_constants()->__watchdog_new_pid (pid->pid());

  object subresult =
    class (Stdio.File p) {
      inherit WatchdogFilterStream;
      string buf = "";
      int done;
      int succeeded, failed, skipped, got_subresult;
      void read_cb (mixed ignored, string in)
      {
	all_constants()->__signal_watchdog();
	write (filter (in));
	foreach (get_cmds(), string wd_cmd) {
	  if (sscanf (wd_cmd, "succeeded %d failed %d skipped %d",
		      int ok, int fail, int skip) == 3) {
	    succeeded += ok;
	    failed += fail;
	    skipped += skip;
	    got_subresult++;
	  }
	  else
	    all_constants()->__send_watchdog_command (wd_cmd);
	}
      }
      void close_cb (mixed ignored)
      {
	all_constants()->__signal_watchdog();
	if (p->errno()) {
          werror ("Error reading output from subprocess: %s.\n",
		  strerror (p->errno()));
	  failed++;
	}
	done = 1;
      }

      void reader_thread(Stdio.File watchdog_pipe)
      {
	while(1) {
	  string data = watchdog_pipe->read(1024, 1);
	  if (!data || (data=="")) break;
	  mixed err = catch { read_cb(0, data); };
	  if (err) master()->report_error(err);
	}
	mixed err = catch { close_cb(0); };
	if (err) master()->report_error(err);
	call_out(lambda(){}, 0);	// Wake up the backend.
      }

      void run()
      {
#ifdef __NT__
#if constant(thread_create)
	thread_create(reader_thread, p);
#endif
#else /* !__NT__ */
	p->set_nonblocking (read_cb, 0, close_cb);
#endif /* __NT__ */
	while (!done) Pike.DefaultBackend();
      }
    } (p);

  subresult->run();
  int err = pid->wait();
  all_constants()->__watchdog_new_pid (getpid());

  if (!subresult->got_subresult) {
    // The subprocess didn't use report_result, probably.
    all_constants()->__watchdog_show_last_test();
    if (err == -1) {
      werror("\nNo result from subprocess (died of signal %s)\n",
	     signame (pid->last_signal()) || (string) pid->last_signal());
    } else {
      werror("\nNo result from subprocess (exited with error code %d).\n", err);
    }
    return 0;
  }

  else if (err == -1) {
    all_constants()->__watchdog_show_last_test();
    werror ("\nSubprocess died of signal %s.\n",
	    signame (pid->last_signal()) || (string) pid->last_signal());
    subresult->failed++;
  }

  else if (err && !subresult->failed) {
    all_constants()->__watchdog_show_last_test();
    werror ("\nSubprocess exited with error code %d.\n", err);
    subresult->failed++;
  }

  else if (subresult->failed)
    all_constants()->__watchdog_show_last_test();

  return ({subresult->succeeded, subresult->failed, subresult->skipped});
}

//! Rerpesents a test in a testsuite.
class Test
{
  //! The file the testsuite (source) resides in.
  string file;

  //! The line number offset to this test in the file.
  int(1..) line;

  //! The test number in this file.
  int(1..) number;

  //! The type of the test. Any of
  //! @string
  //!   @value "COMPILE"
  //!     Compiles the source and make verifies there are no warnings
  //!     or errors.
  //!   @value "COMPILE_ERROR"
  //!     Compiles the source and expects to get a compilation error.
  //!   @value "COMPILE_WARNING"
  //!     Compiles the source and expects to get a compilation warning.
  //!   @value "EVAL_ERROR"
  //!     Evaluates the method a of the source source and expects an
  //!     evaluation error.
  //!   @value "FALSE"
  //!     Verifies that the response from method a of the source is false.
  //!   @value "TRUE"
  //!     Verifies that the response from method a of the source is true.
  //!   @value "PUSH_WARNING"
  //!     Evaluate the method a and take the resulting string and push
  //!     it to the set of ignored warnings. The same warning can be
  //!     pushed multiple times, but must be popped multiple times.
  //!   @value "POP_WARNING"
  //!     Evaluate the method a and take the resulting string and pop
  //!     the warning from the set of ignored warnings. When popped
  //!     the same number of times as it has been pushed, the warning
  //!     is no longer ignored.
  //!   @value "RUN"
  //!     Compiles and evaluates method a of the source, but ignores
  //!     the result.
  //!   @value "RUNCT"
  //!     Compiles and evaluates method a od source, and expects an
  //!     array(int) of two or three elements back.
  //!     @array
  //!       @elem int 0
  //!         The number of successful tests performed.
  //!       @elem int 1
  //!         The number of failed tests,
  //!       @elem int 2
  //!         Optionally the number of skipped tests.
  //!     @endarray
  //!   @value "EQ"
  //!     Compares the result of the method a and b with @[==].
  //!   @value "EQUAL"
  //!     Compares the result of the method a and b with @[equal].
  //! @endstring
  string type;

  //! The list of conditions that has to be met for this test to not
  //! be skipped. Each condition should be an expression.
  array(string) conditions = ({});

  //! The source code that is to be compiled and evaluated.
  string source;

  //!
  optional protected void create(string file, int line, int number,
                                 string type, string source,
                                 void|array(string) cond)
  {
    this::file = file;
    this::line = line;
    this::number = number;
    this::type = type;
    this::source = source;
    if( cond )
      conditions = cond;
  }

  protected class AdjustLine()
  {
    void compile_error(string file, int _line, string text) {
      log_msg("%s:%d: %s\n", file, _line+line-1, text);
    }
  }

  //! Compile the source code @[src] in the context of the file this
  //! test belongs. Note that no changes in error mode is done and
  //! compilation errors are not caught.
  program compile(string src)
  {
    return master()->get_inhibit_compile_errors() ?
      compile_string(src, file) :
      compile_string(src, file, AdjustLine());
  }

  array(Plugin) plugins = ({});

  //! Add a @[Plugin] object to the test, allowing the source code to
  //! be modified.
  void add_plugin(Plugin p)
  {
    if( p->active(this) )
      plugins += ({ p });
  }

  //! Applies all the plugins on the source code contained in this
  //! test and returns the result.
  string prepare_source()
  {
    string src = source;
    foreach(plugins;; Plugin plugin)
      src = plugin->preprocess(src);
    return src;
  }

  protected void dmalloc_name(int set)
  {
#if constant(Debug.dmalloc_set_name)
    if( set )
      Debug.dmalloc_set_name(name(),1);
    else
      Debug.dmalloc_set_name();
#endif
  }

  //! This value will be sent to @[MasterObject.set_inhibit_errors]
  //! before compilation by @[compile()].
  int(0..1)|object inhibit_errors = 1;

  Error.Compilation compilation_error;

  //! Set the error mode according to @[inhibi_errors], applies any
  //! source code plugins by calling @[prepare_source] and finally
  //! compiles the result. Any resulting compilation errors will be
  //! stored in @[compilation_error]. The error mode will be set to
  //! @expr{0@} after compiltion is done.
  variant program compile()
  {
    program ret;
    master()->set_inhibit_compile_errors(inhibit_errors);
    dmalloc_name(1);
    compilation_error = catch {
        ret = compile(prepare_source());
      };
    dmalloc_name(0);
    master()->set_inhibit_compile_errors(0);
    return ret;
  }

  //! The name of this test, in the form of filename:line:" Test
  //! "number. The result is then processed by any code plugins.
  string name()
  {
    string n = file + ":" + line + ": Test " + number;
    foreach(plugins;; Plugin plugin)
      n = plugin->process_name(n);
    return n;
  }
}

class Testsuite
{
  protected int(0..) _sizeof();
  protected this_program `+(mixed steps);
  protected int(0..1) `!();
  int(0..1) next();
  int(0..) index();
  Test value();
  optional string name();
}

//! Represents a "testsuite" file, after m4 processing.
class M4Testsuite
{
  inherit Testsuite;
  protected array tests;
  protected string file_name;
  protected Plugin compat;

  protected void create(string data, string fn)
  {
    file_name = fn;
    string pike_compat;

    if(sscanf (data, "START%s\n%s", pike_compat, data) == 2) {
      if(!has_suffix(data, "END"))
        log_msg("%s: Missing end marker.\n", fn);

      pike_compat = String.trim(pike_compat);
      if( sizeof(pike_compat) )
        compat = CompatPlugin(pike_compat);
    }
    else
      log_msg("%s: Missing start marker.\n", fn);

    tests = (data/"\n....\n")[..<1];
  }

  string name()
  {
    if(compat)
      return sprintf("%s in %s compat mode", file_name, compat->pike_compat);
    return file_name;
  }

  //! Represents a test case from a "testsuite" file, after m4
  //! processing.
  class M4Test
  {
    inherit Test;

    //! @param data
    //! The data from a testsuite file, after start/stop markers have
    //! been stripped and the file splitted on "...." tokens. From
    //! this the conditions, file, line, number, type and source will
    //! be parsed.
    protected void create(string data)
    {
      if( sscanf(data, "COND %s\n%s", string cond, data)==2 )
        conditions += ({ cond });
      if( sscanf(data, "%s:%d: test %d, expected result: %s\n%s",
                 file, line, number, type, data)!=5 )
      {
        sscanf(data, "%s\n", data);
        error("Illegal format. %O\n", data);
      }
      source = data;
    }

    program compile(string src)
    {
      return master()->get_inhibit_compile_errors() ?
        compile_string(src, file_name) :
        compile_string(src, file_name, AdjustLine());
    }

    variant program compile()
    {
      return ::compile();
    }
  }

  protected int(0..) _sizeof()
  {
    return sizeof(tests);
  }

  // Iterator API

  protected int position;

  protected int(0..1) `!()
  {
    return position >= sizeof(tests);
  }

  int(0..) next()
  {
    position++;
    return position < sizeof(tests);
  }

  protected this_program `+(mixed steps)
  {
    if(!intp(steps))
      error("Can only step forward integer number of steps.\n");
    position += steps;
    return this;
  }

  int index()
  {
    return position;
  }

  Test value()
  {
    if( `!() ) return UNDEFINED;
    Test ret = M4Test(tests[position]);
    if(compat)
      ret->add_plugin(compat);
    return ret;
  }
}

class InTestsuite
{
  inherit M4Testsuite;

  protected void create(string data, string fn)
  {
    file_name = fn;
    string pike_compat;

    if(sscanf (data, "START_MARKER%s\n%s", pike_compat, data) == 2) {
      if(!has_suffix(data, "END_MARKER"))
        log_msg("%s: Missing end marker.\n", fn);

      if( sscanf(pike_compat, "(%s)", pike_compat) )
        compat = CompatPlugin(pike_compat);
    }
    else
      log_msg("%s: Missing start marker.\n", fn);

    tests = ({});
    parse(data);
  }

  protected mapping macros = ([
    "ignore_warning":"DOTEST(PUSH_WARNING,"
#"mixed a() { return [[$1]]; })
$2
DOTEST(POP_WARNING,"
#"mixed a() { return [[$1]]; })
",

    "test_any":"DOTEST(EQ,"
#"mixed a() { [[$1]]; }
mixed b() { return [[$2]]; })",

    "test_any_equal":"DOTEST(EQUAL,"
#"mixed a() { $1; }
mixed b() { return $2; })",

    "test_eq":"DOTEST(EQ,"
#"mixed a() { return [[$1]]; }
mixed b() { return [[$2]]; })",

    "test_equal":"DOTEST(EQUAL,"
#"mixed a() { return $1; }
mixed b() { return $2; })",

    "test_do":"DOTEST(RUN,"
    "mixed a() { $1; })",

    "test_true":"DOTEST(TRUE,"
    "mixed a() { return $1; })",

    "test_false":"DOTEST(FALSE,"
    "mixed a() { return $1; })",

    "test_compile":"DOTEST(COMPILE,"
    "void x() { $1 ; } )",

    "test_compile_any":"DOTEST(COMPILE,"
    "$1 )",

    "test_compile_error":"DOTEST(COMPILE_ERROR,"
    "mixed a() { $1;})",

    "test_compile_error_any":"DOTEST(COMPILE_ERROR,"
    "$1)",

    "test_compile_warning":"DOTEST(COMPILE_WARNING,"
    "mixed a() { $1;})",

    "test_eval_error":"DOTEST(EVAL_ERROR,"
    "mixed a() { $1; })",

    "test_define_program":"DOTEST(RUN,"
#"void a() { add_constant(\"$1\",class c {
  $2
});})",

    "test_program":"DOTEST(TRUE,"
    "$1)",

    "test_program_eq":"DOTEST(EQ,"
#"$1
mixed b() {return $2;})",

    "test_program_equal":"DOTEST(EQUAL,"
#"$1
mixed b() {return $2;})",

    "test_tests":"DOTEST(RUNCT,"
    "$1)",

    "cond_begin":#"
define([[CONDITION]],[[COND $1
]])",

    "cond_end":#"
define([[CONDITION]],[[]])
",

    "cond":#"
cond_begin([[$1]])
$2
cond_end
",

    "cond_resolv":"cond([[master()->resolv(String.trim(\"$1\"))]],[[$2]])",
    "ifefun":"cond([[all_constants()->$1]],[[$2]])",

    "test_cmp":#"
test_true($1<$2)
test_false($1>$2)
test_true($2>$1)
test_false($2<$1)
test_true($1<=$2)
test_false($1>=$2)
test_true($2>=$1)
test_false($2<=$1)
test_true($2<=$2)
test_true($1>=$1)",

    "test_cmp3":#"
test_cmp($1,$2)
test_cmp($2,$3)
test_cmp($1,$3)

 test_true(\`<($1,$2,$3))
test_false(!\`<($1,$2,$3))
test_false(\`<($1,$3,$2))
 test_true(!\`<($1,$3,$2))
test_false(\`<($2,$3,$1))
test_false(\`<($2,$1,$3))
test_false(\`<($3,$1,$2))
test_false(\`<($3,$2,$1))
test_false(\`<($3,$2,$2,$1))

 test_true(\`<=($1,$2,$3))
test_false(!\`<=($1,$2,$3))
test_false(\`<=($1,$3,$2))
test_false(\`<=($2,$3,$1))
test_false(\`<=($2,$1,$3))
 test_true(!\`<=($2,$1,$3))
test_false(\`<=($3,$1,$2))
test_false(\`<=($3,$2,$1))
 test_true(\`<=($1,$1,$2,$2,$3,$3))

test_false(\`>($1,$2,$3))
test_false(\`>($1,$3,$2))
test_false(\`>($2,$3,$1))
test_false(\`>($2,$1,$3))
test_false(\`>($3,$1,$2))
 test_true(\`>($3,$2,$1))
test_false(\`>($3,$2,$2,$1))

test_false(\`>=($1,$2,$3))
test_false(\`>=($1,$3,$2))
test_false(\`>=($2,$3,$1))
test_false(\`>=($2,$1,$3))
test_false(\`>=($3,$1,$2))
 test_true(!\`>=($3,$1,$2))
 test_true(\`>=($3,$2,$1))
 test_true(\`>=($3,$3,$2,$2,$1,$1))
test_false(!\`>=($3,$3,$2,$2,$1,$1))

test_equal(min($2,$1,$3), $1)
test_equal(max($2,$1,$3), $3)
",
  ]);

  string expand(string s, array args)
  {
    array split = s/"$";
    foreach( split; int i; string part )
    {
      if(!i) continue;
      if( sscanf(part, "%d%s", int arg, part)==2 )
      {
        arg--;
        if( arg>=0 && arg<sizeof(args) )
          split[i] = args[arg] + part;
        else
          split[i] = part;
      }
      else
        split[i] = "$"+part;
    }
    return split*"";
  }

  string dequote(string data)
  {
    string pre, post;
    while( sscanf(data, "%s[[%s", pre, post)==2 )
      data = pre+post;
    while( sscanf(data, "%s]]%s", pre, post)==2 )
      data = pre+post;
    return data;
  }

  string parse(string data)
  {
    int pos;

    array(string) parse_args()
    {
      array args = ({});
      if( data[pos] == '(' )
      {
        ADT.Stack pstack = ADT.Stack();
        pstack->push('(');
        int start = pos;

        void add_arg()
        {
          string arg = String.trim(data[start+1..pos-1]);
          args += ({ arg });
        };

        while( sizeof(pstack) && pos<sizeof(data) )
        {
          switch(data[++pos])
          {
          case '(':
          case '[': pstack->push(data[pos]); break;

          case ')':
          case ']':
            int p = pstack->pop();
            if( p!=([ ']':'[', ')':'(' ])[data[pos]] )
              error("Mismatched parenthesis. %O\n", data[start..pos]);
            if( sizeof(pstack)==0 )
              add_arg();
            break;

          case ',':
            if( sizeof(pstack)==1 )
            {
              add_arg();
              start=pos;
            }
          }
        }
      }
      else
        return 0;

      pos++;
      return args;
    };

    String.Buffer ret = String.Buffer();
    while(1)
    {
      while( pos<sizeof(data) &&
             !( (data[pos]>='A' && data[pos]<='Z') ||
                (data[pos]>='a' && data[pos]<='z') ))
      {
        ret->putchar(data[pos]);
        pos++;
      }
      if( pos>=sizeof(data) ) return (string)ret;
      int start = pos;
      while( pos<sizeof(data) &&
             ((data[pos]>='0' && data[pos]<='9') ||
              (data[pos]>='A' && data[pos]<='Z') ||
              (data[pos]>='a' && data[pos]<='z') ||
              data[pos]=='_')) pos++;
      string token = data[start..pos-1];
      switch(token)
      {
      case "dnl":
        while(data[pos]!='\n' && pos<sizeof(data)) pos++;
        if( pos>=sizeof(data) ) return (string)ret;
        break;

      case "define":
        array args = parse_args();
        if( !args )
        {
          ret->add(token);
          break;
        }
        if(sizeof(args)!=2) error("Need two arguments for define.\n");
        macros[dequote(args[0])] = args[1];
        break;

      case "ifelse":
        args = parse_args();
        if(sizeof(args)!=4) error("ifelse only supported with 4 arguments.\n");
        if(dequote(args[0])==dequote(args[1]))
          ret->add(parse(args[2]));
        else
          ret->add(parse(args[3]));
        break;

      case "DOTEST":
        args = parse_args();
        if(sizeof(args)!=2)
          error("Need two arguments for DOTEST. %O\n", sizeof(args));
        string cond = macros->CONDITION;
        if(cond)
        {
          cond = dequote(cond);
          if( !sscanf(cond, "COND %s", cond) )
            cond = "";
          cond = String.trim(cond);
          if( cond=="" )
            cond = 0;
        }
        tests += ({ Test(file_name, 0/*FIXME*/, sizeof(tests)+1,
                         dequote(parse(args[0])), dequote(parse(args[1])),
                         cond && ({ cond })) });
        break;

      default:
        if( macros[token] )
          ret->add(parse( expand(macros[token], parse_args()) ));
        else
          ret->add(token);
        break;
      }
    }
    return (string)ret;
  }

  Test value()
  {
    if( `!() ) return UNDEFINED;
    Test ret = tests[position];
    if(compat)
      ret->add_plugin(compat);
    return ret;
  }

}


//! Interface for source code plugins, added to a @[Test] by calling
//! @[add_plugin].
class Plugin
{
  //! Returns 1 if the plugin is active (i.e. should be called by the
  //! test), otherwise 0. Defaults to 1.
  int(0..1) active(Test t) { return 1; }

  //! Allows for modifications of the name of the test. By default
  //! just returns the name unmodified.
  string process_name(string name) { return name; }

  //! Called by the test to modify the source code. By default just
  //! returns the unmodified source.
  string preprocess(string source) { return source; }
}

//! A source code plugin for running code in different Pike compat
//! modes. Instantiated with the pike compat version to run in,
//! e.g. @expr{"7.8"@}.
class CompatPlugin(string pike_compat)
{
  inherit Plugin;

  //! Modifies the name by adding the version and "compat" after the
  //! test name, e.g. "testsuite:1: Test 1 (7.8 compat)".
  string process_name(string name)
  {
    return sprintf("%s (%s compat)", name, pike_compat);
  }

  //! Modifies the source code to add "#pike" and the version at the
  //! top of the code, followed by "#pragma no_deprecation_warnings".
  string preprocess(string source)
  {
    return "#pike " + pike_compat + "\n" +
      "#pragma no_deprecation_warnings\n" +
      source;
  }
}
