#!/usr/local/bin/pike

final constant TEST_SIZE = 16384;

string testdata = random_string(TEST_SIZE);

string|zero testdir;

int verbose;
int testno;
int failures;

mixed call_out_id;
object tf;

/*
 * Some helper functions.
 */

constant log_msg = Tools.Testsuite.log_msg;
constant log_status = Tools.Testsuite.log_status;

void exit_test()
{
  Tools.Testsuite.report_result (max (testno - failures, 0), failures);
  exit(!!failures);
}

void fail_test(string msg)
{
  log_msg("Test %d failed: %s.\n", testno, msg);
  failures++;
  call_out(next, 0);
}

/*
 * The driver function.
 */

void next()
{
  testno++;

  function test;
  if (!(test = this["test"+testno])) exit_test();
  mixed err;
  if (err = catch {
    log_status("Kqueue test: %d", testno);
    test();
  }) {
    fail_test(describe_backtrace(err));
  }
}

void done()
{
  remove_call_out(call_out_id);
  tf->close();
  call_out(next, 0);
}

/*
 * The actual tests.
 * NOTE: We don't test NOTE_REVOKE, as there's no reliable way to generate this 
 *       event type from within Pike.
 */

void test9()
{
  if (testdir) {
    cd("/");
    if (!rm(testdir)) {
      fail_test(sprintf("Failed to remove test directory: %O: %s\n",
                        testdir, strerror(errno())));
      return;
    }
  }
  next();
}

void test8()
{
  log_status("kqueue: NOTE_DELETE\n");
  tf = Stdio.File("kqueue.test");

  // our failure timeout
  call_out_id = call_out(fail_test, 5, "Timeout.");

  tf->set_fs_event_callback(done, Stdio.NOTE_DELETE);

  call_out(do_delete, 0);
}

void test7()
{
  log_status("kqueue: NOTE_ATTRIB\n");
  tf = Stdio.File("kqueue.test");

  // our failure timeout
  call_out_id = call_out(fail_test, 5, "Timeout.");

  tf->set_fs_event_callback(done, Stdio.NOTE_ATTRIB);

  call_out(do_chmod, 0);
}

void test6()
{
  log_status("kqueue: NOTE_LINK\n");
  tf = Stdio.File("kqueue.test");

  // our failure timeout
  call_out_id = call_out(fail_test, 5, "Timeout.");

  tf->set_fs_event_callback(done, Stdio.NOTE_LINK);

  call_out(do_link, 0);
}

void test5()
{
  log_status("kqueue: NOTE_RENAME\n");
  tf = Stdio.File("kqueue.tst");

  // our failure timeout
  call_out_id = call_out(fail_test, 5, "Timeout.");

  tf->set_fs_event_callback(done, Stdio.NOTE_RENAME);

  call_out(do_rename, 0);
}

void test4()
{
  log_status("kqueue: NOTE_WRITE\n");
  tf = Stdio.File("kqueue.tst");
  tf->set_nonblocking();
  tf->set_fs_event_callback(done, Stdio.NOTE_WRITE);

  // our failure timeout
  call_out_id = call_out(fail_test, 5, "Timeout.");

  call_out(do_write, 0.1);
}

void test3()
{
  log_status("kqueue: NOTE_EXTEND\n");
  /* first, create a test file */
  tf = Stdio.File("kqueue.tst", "crw");
  tf->close();

  tf = Stdio.File("kqueue.tst");
  tf->set_nonblocking();
  tf->set_fs_event_callback(done, Stdio.NOTE_EXTEND);

  // our failure timeout
  call_out_id = call_out(fail_test, 5, "Timeout");

  call_out(do_extend, 0.1);
}

void test2()
{
  if (testdir) {
    if (!cd(testdir)) {
      fail_test(sprintf("Failed to change cwd to test directory %O: %s\n",
                        testdir, strerror(errno())));
      return;
    }
  }
  next();
}

void test1()
{
  string tmpdir = getenv()->TMPDIR || "/var/tmp";
  testdir = sprintf("%s/pike-%d-%d-testdir", tmpdir, getuid(), getpid());
  if (!mkdir(testdir) && (errno() != System.EEXIST)) {
    fail_test(sprintf("Failed to create test directory %O: %s\n",
                      testdir, strerror(errno())));
    testdir = UNDEFINED;
    return;
  }
  next();
}

void do_rename()
{
  mv("kqueue.tst", "kqueue.test");
}

void do_chmod()
{
  System.chmod("kqueue.tst", 0640);
}

void do_write()
{
  object tf1 = Stdio.File("kqueue.tst", "rw");
  tf1->seek(0);
  tf1->write("foo");
  tf1->close();
}


void do_extend()
{
  object tf1 = Stdio.File("kqueue.tst", "rw");
  tf1->write("foo2");
  tf1->close();
}

void do_link()
{
  System.hardlink("kqueue.test", "kqueue.tst");
}

void do_delete()
{
  rm("kqueue.test");
  rm("kqueue.tst");
}

/*
 * Start the backend.
 */

int main(int argc, array(string) argv)
{
  verbose = (int) (getenv()->TEST_VERBOSITY || 2);
  umask(0022);
#if constant(alarm)
  alarm(5*60);	// 5 minutes should be sufficient for this test.
#endif
  call_out(next, 0);
  return -1;
}
