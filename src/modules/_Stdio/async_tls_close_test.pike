// This test belongs in SSL.pmod, but it's here because there's
// currently no way to filter out test pike files from pmod dirs.

int main (int argc, array(string) argv)
{
  string fail;

  if (argc < 3) {
    werror("Expected at least two arguments.\n");
    exit(1);
  }

  int clean_close = argv[1] == "1";
  int block = argv[2] == "1";

  Pike.Backend blocked_backend = Pike.Backend();
  Stdio.Port blocked_server = Stdio.Port();
  blocked_server->set_backend (blocked_backend);
  blocked_server->bind (36565, lambda () {
				 fail = "Should never accept a connection.\n";
			       });

  Stdio.File con = Stdio.File();
  SSL.File tlscon;
  con->async_connect (
    "127.0.0.1", 36565,
    lambda (int success) {
      if (success) {
	tlscon = SSL.File (con, SSL.Context());
	tlscon->connect("localhost");
	tlscon->set_write_callback (lambda () {
				      fail = "Handshake should not succeed.\n";
				    });
      }
      else
	fail = "Failed to connect.";
    });

  call_out (
    lambda () {
      // Got the connection timeout we want.
      if (!fail) {
	if (mixed err = catch {
	    int init_is_open = tlscon->is_open();
	    if (block) tlscon->set_blocking();
	    int close_err;
	    if (mixed err = catch (tlscon->close (0, clean_close))) {
	      if (tlscon->errno() != System.ECONNRESET)
		throw (err);
	      close_err = 1;
	    }
	    array(int) res = ({init_is_open,
			       close_err,
			       tlscon->is_open(),
			       objectp (tlscon->shutdown()),
			       tlscon->errno()});
	    int successful;
	    if (clean_close) {
	      if (block)
		// Clean close in blocking mode should block in
		// tlscon->close until the other thread destructs the
		// server and we get ECONNRESET.
		successful = equal (res, ({1, 1, 0, 0, System.ECONNRESET}));
	      else
		// Clean close in nonblocking mode shouldn't really
		// close the fd, but the later shutdown should, and
		// then set EPIPE.
		successful = equal (res, ({1, 0, 2, 0, System.EPIPE}));
	    }
	    else {
	      // A nonclean close always succeeds without any ado.
	      successful = equal (res, ({1, 0, 0, 0, 0}));
	    }
	    if (!successful)
	      Tools.Testsuite.log_msg (
		"Async tls close on stalled connection failed: "
		"clean_close=%d block=%d res=%{%O %}\n",
		clean_close, block, res);
	    Tools.Testsuite.report_result (successful, !successful);
	    exit (!successful);
	  })
	  fail = "Error in call out: " + describe_backtrace (err);
      }
      if (fail) werror (fail);
      exit (2);
    }, 0.5);

  thread_create (lambda () {
		   sleep (1);
#if 0
		   werror ("\n");
		   foreach (all_threads(), Thread.Thread t)
		     werror ("\n" + describe_backtrace (t->backtrace()));
#endif
		   destruct (blocked_server);
		 });

  return -1;
}
