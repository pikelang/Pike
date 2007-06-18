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

void report_result (int succeeded, int failed, void|int skipped)
//! Use this to report the number of successful, failed, and skipped
//! tests in a script started using @[run_script].
{
  // Not really a watchdog command, but sent using the same format.
  write ("WD succeeded %d failed %d skipped %d\n",
	 succeeded, failed, skipped);
}

class WatchdogFilterStream
// Filter out watchdog commands on the form "WD <cmd>\n", passing
// everything else through.
{
  static array(string) wd_cmds = ({});
  static string cmd_buf;

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
  Stdio.File p2 = p->pipe (Stdio.PROP_IPC|
			   Stdio.PROP_NONBLOCK|
			   Stdio.PROP_BIDIRECTIONAL);
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
	write (filter (in));
	foreach (get_cmds(), string wd_cmd) {
	  if (sscanf (wd_cmd, "succeeded %d failed %d skipped %d",
		      succeeded, failed, skipped) == 3)
	    got_subresult = 1;
	  else
	    all_constants()->__send_watchdog_command (wd_cmd);
	}
      }
      void close_cb (mixed ignored)
      {
	if (p->errno()) {
	  werror ("Error reading output from subprocess: %s\n",
		  strerror (p->errno()));
	  failed++;
	}
	done = 1;
      }
      void run()
      {
	p->set_nonblocking (read_cb, 0, close_cb);
	while (!done) Pike.DefaultBackend();
      }
    } (p);

  subresult->run();
  int err = pid->wait();
  all_constants()->__watchdog_new_pid (getpid());

  if (!subresult->got_subresult) {
    // The subprocess didn't use report_result, probably.
    if (err == -1) {
      werror("No result from subprocess (died of signal %s)\n",
	     signame (pid->last_signal()) || (string) pid->last_signal());
    } else {
      werror("No result from subprocess (exited with error code %d).\n", err);
    }
    return 0;
  }

  else {
    if (err == -1) {
      werror ("Subprocess died of signal %s.\n",
	      signame (pid->last_signal()) || (string) pid->last_signal());
      subresult->failed++;
    }
    else if (err && !subresult->failed) {
      werror ("Subprocess exited with error code %d.\n", err);
      subresult->failed++;
    }
  }

  return ({subresult->succeeded, subresult->failed, subresult->skipped});
}
