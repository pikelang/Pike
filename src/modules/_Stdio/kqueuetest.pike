#!/usr/local/bin/pike

final constant TEST_SIZE = 16384;

string testdata = random_string(TEST_SIZE);

int verbose;
int testno;

mixed call_out_id;
object tf;

/*
 * Some helper functions.
 */

constant log_msg = Tools.Testsuite.log_msg;
constant log_status = Tools.Testsuite.log_status;

void exit_test (int failure)
{
  Tools.Testsuite.report_result (max (testno - !!failure, 0), !!failure);
  exit (failure);
}

/*
 * The driver function.
 */

void next()
{
  testno++;

  function test;
  if (!(test = this_object()["test"+testno])) exit_test(0);
  mixed err;
  if (err = catch {
    log_status("Kqueue test: %d", testno);
    test();
  }) {
    catch {
      log_msg("Test %d failed!\n"
	      "%s\n",
	      testno,
	      describe_backtrace(err));
    };
    exit_test(1);
  }
}

void done()
{
  remove_call_out(call_out_id);
  tf->close();
 if(0)
 {
    log_msg("Test %d failed.\n", testno);
    exit_test(1);
  }
  call_out(next, 0);
}

/*
 * The actual tests.
 * NOTE: We don't test NOTE_REVOKE, as there's no reliable way to generate this 
 *       event type from within Pike.
 */

void test6()
{
  tf = Stdio.File("kqueue.test");
  
  // our failure timeout
  call_out_id = call_out(exit_test, 5, 1);
  
  tf->set_fs_event_callback(done, Stdio.NOTE_DELETE);

  call_out(do_delete, 0);
}

void test5()
{
  tf = Stdio.File("kqueue.test");
  
  // our failure timeout
  call_out_id = call_out(exit_test, 5, 1);
  
  tf->set_fs_event_callback(done, Stdio.NOTE_ATTRIB);

  call_out(do_chmod, 0);
}

void test4()
{
  tf = Stdio.File("kqueue.test");
  
  // our failure timeout
  call_out_id = call_out(exit_test, 5, 1);
  
  tf->set_fs_event_callback(done, Stdio.NOTE_LINK);

  call_out(do_link, 0);
}

void test3()
{
  tf = Stdio.File("kqueue.tst");
  
  // our failure timeout
  call_out_id = call_out(exit_test, 5, 1);
  
  tf->set_fs_event_callback(done, Stdio.NOTE_RENAME);

  call_out(do_rename, 0);
}

void test2()
{
  tf = Stdio.File("kqueue.tst");
  tf->set_nonblocking();  
  tf->set_fs_event_callback(done, Stdio.NOTE_WRITE);

  // our failure timeout
  call_out_id = call_out(exit_test, 5, 1);
  
  call_out(do_write, 0.1);
}

void test1()
{
  /* first, create a test file */
  tf = Stdio.File("kqueue.tst", "crw");
  tf->close();

  tf = Stdio.File("kqueue.tst");
  tf->set_nonblocking();  
  tf->set_fs_event_callback(done, Stdio.NOTE_EXTEND);

  // our failure timeout
  call_out_id = call_out(exit_test, 5, 1);
  
  call_out(do_extend, 0.1);
}


void do_rename()
{
  mv("kqueue.tst", "kqueue.test");
}

void do_chmod()
{
  System.chmod("kqueue.tst", 777);
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
#if constant(alarm)
  alarm(5*60);	// 5 minutes should be sufficient for this test.
#endif
  call_out(next, 0);
  return -1;
}
