#!/usr/local/bin/pike

final constant TEST_SIZE = 16384;

string testdata = random_string(TEST_SIZE);

int verbose;
int testno;

mixed call_out_id;
object tf;
object stream;

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

void next(mixed ... args)
{
  testno++;

  function test;
  if (!(test = this_object()["test"+testno])) exit_test(0);
  mixed err;
  if (err = catch {
    log_status("System.FSEvents test: %d", testno);
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
 */

void test1()
{

  stream = System.FSEvents.EventStream(({}), 3.0, System.FSEvents.kFSEventStreamEventIdSinceNow,
    System.FSEvents.kFSEventStreamCreateFlagNone);

  stream->set_callback(next);
  stream->add_path(".");
  stream->start();

  /* first, create a test file */
  tf = Stdio.File("fsevent.test", "crw");
  tf->close();

  // our failure timeout
  call_out_id = call_out(exit_test, 10, 1);  
  call_out(do_delete, 0.1);
}

void do_delete()
{
  rm("fsevent.test");
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

#if constant(Pike.DefaultBackend.HAVE_CORE_FOUNDATION)
   Pike.DefaultBackend.enable_core_foundation(1);
#else
   werror("Pike does not have support for Core Foundation. FSEvents will not function!\n");
   exit(1);
#endif


  call_out(next, 0);
  return -1;
}
