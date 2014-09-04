/* 
 *    Shootouts
 *        or
 * Pike speed tests
 *
 */

#pike __REAL_VERSION__

import ".";


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

// This function runs the actual test, it is started in a sub-process from run.
void run_sub( Test test, int maximum_seconds, float overhead)
{
    float tg=0.0;
    int testntot=0;
    int nloops = 0;
    int norm;

    Pike.gc_parameters(([ "enabled" : 0 ]));
    for (;;nloops++)
    {
        int start_cpu = gethrvtime();
        testntot += test->perform();
        tg += (gethrvtime()-start_cpu) / 1000000.0;
        if (tg >= maximum_seconds) break;
    }
    Pike.gc_parameters(([ "enabled" : 1 ]));
    gc();

    norm = (int)(testntot/tg);

    string res = (test->present_n ?
                  test->present_n(testntot,nloops,tg,tg,1) :
                  format_big_number(norm)+"/s");


    write( Standards.JSON.encode( ([ "time":tg,"loops":nloops,"n":testntot,"readable":res,"n_over_time":norm ]) )+"\n" );
}

private mapping(string:Test) _tests;
private mapping(Test:string) rtests;

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
      if ((programp(p=Tools.Shoot[test])) &&  (t=p())->perform)
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

mapping(string:int|float) run(Test test, int maximum_seconds, float overhead)
{
    Stdio.File fd = Stdio.File();
    string test_name;
    if( !rtests )
      tests();

    if( !(test_name = rtests[ test ]) )
      error("Test %O is not a test\n", test);


    Process.spawn_pike( ({"-e", sprintf("Tools.Shoot.run_sub( Tools.Shoot[%q](), %d, %f )",
                                test_name, maximum_seconds, overhead ) }),
                        (["stdout":fd->pipe()]));
    return Standards.JSON.decode( fd->read() );
}

