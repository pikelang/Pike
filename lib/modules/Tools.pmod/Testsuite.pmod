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
    werror("Failed to create pipe: %s\n", strerror (p->errno()));
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
	  werror ("Error reading output from subprocess: %s\n",
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
