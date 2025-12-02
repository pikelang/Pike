/* -*- mode: C; c-basic-offset: 4; -*-
 *
 *    Shootouts
 *        or
 * Pike speed tests
 *
 */

#pike __REAL_VERSION__

import ".";


//! Format an integer with SI-suffixes.
string format_big_number(int i)
{
    if( i > 10000000000 )
        return i/1000000000+"G";
    if( i > 10000000 )
        return i/1000000+"M";
    if( i > 10000 )
        return i/1000+"k";

    return (string)i;
}

//! Run a single benchmark test in the current process and
//! return the result as JSON on stdout.
//!
//! @param test
//!   Benchmark to run.
//!
//! @param maximum_seconds
//!   Number of seconds to run the test before terminating it.
//!
//! @param overhead
//!   Ignored, obsolete.
//!
//! @returns
//!   Writes a JSON-encoded mapping with the following fields on success:
//!   @mapping
//!     @member float "time"
//!       Actual number of seconds that the test ran.
//!     @member int "loops"
//!       Number of times that the test ran.
//!     @member int "n"
//!       Number of sub tests that were run.
//!     @member string "readable"
//!       Description of the test result.
//!     @member int "n_over_time"
//!       Number of sub tests per second.
//!   @endmapping
//!   On benchmark failure a JSON-encoded mapping with the single entry
//!   @expr{"readable"@} set to @expr{"FAIL"@} is written to stdout.
//!
//! @note
//!   This is the funcction that is called in a sub-process by @[run()].
//!
//! @seealso
//!   @[run()], @[tests()]
void run_sub( Test test, int maximum_seconds,
              __deprecated__(float)|void overhead)
{
    float tg=0.0;
    int testntot=0;
    int nloops = 0;
    int norm;
    for (;;nloops++)
    {
        mixed context = 0;
        if (test->prepare)
            context = test->prepare();
        int start_cpu = gethrvtime();
        testntot += test->perform(context);
        tg += (gethrvtime()-start_cpu) / 1000000.0;
        if (tg >= maximum_seconds) break;
    }

    norm = (int)(testntot/tg);

    string res = (test->present_n ?
                  test->present_n(testntot,nloops,tg,tg,1) :
                  format_big_number(norm)+"/s");


    write( Standards.JSON.encode( ([ "time":tg,"loops":nloops,"n":testntot,"readable":res,"n_over_time":norm ]) )+"\n" );
}

private mapping(string:Test) _tests;
private mapping(Test:string) rtests;

//! Return a mapping with all known and enabled benchmark tests.
//!
//! @returns
//!   Returns a mapping indexed on the display names
//!   of the benchmark tests.
mapping(string:Test) tests()
{
  if( !_tests )
  {
    _tests = ([]);
    rtests = ([]);
    foreach (indices(Tools.Shoot), string test)
    {
      program p;
      Test t;
      if ((programp(p=Tools.Shoot[test])) && (t=p())->perform && !t->disabled)
      {
        if( !t->name )
          exit(1,"The test %O does not have a name\n", t );
        if( _tests[t->name] )
          exit(1,"The tests %O and %O have the same name\n", t, _tests[t->name] );
        _tests[t->name]=t;
        rtests[t] = test;
      }
    }
  }

  return _tests;
}

//! Run a single benchmark test.
//!
//! @param test
//!   Benchmark to run.
//!
//! @param maximum_seconds
//!   Number of seconds to run the test before terminating it.
//!
//! @param overhead
//!   Ignored, obsolete.
//!
//! @returns
//!   Returns a mapping with the following fields on success:
//!   @mapping
//!     @member float "time"
//!       Actual number of seconds that the test ran.
//!     @member int "loops"
//!       Number of times that the test ran.
//!     @member int "n"
//!       Number of sub tests that were run.
//!     @member string "readable"
//!       Description of the test result.
//!     @member int "n_over_time"
//!       Number of sub tests per second.
//!   @endmapping
//!   On benchmark failure a mapping with the single entry
//!   @expr{"readable"@} set to @expr{"FAIL"@} is returned.
//!
//! @seealso
//!   @[run_sub()], @[tests()]
mapping(string:int|float|string) run(Test test, int maximum_seconds,
                                     __deprecated__(float)|void overhead)
{
    Stdio.File fd = Stdio.File();
    string test_name;
    if( !rtests )
      tests();

    if( !(test_name = rtests[ test ]) )
      error("Test %O is not a test\n", test);


    Process.spawn_pike( ({"-e", sprintf("Tools.Shoot.run_sub( Tools.Shoot[%q](), %d )",
                                        test_name, maximum_seconds ) }),
                        (["stdout":fd->pipe()]));
    mixed err = catch {
      return Standards.JSON.decode( fd->read() );
    };
    master()->handle_error(err);
    // JSON decoding failed. Likely due to not having received any data from
    // the subprocess. The most common cause for this is that the subprocess
    // crashed with a SIGSEGV or SIGBUS or similar.
    return ([ "readable": "FAIL" ]);
}
