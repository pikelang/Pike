// $Id: benchmark.pike,v 1.7 2004/07/08 15:36:42 grubba Exp $

#pike __REAL_VERSION__

constant description = "Runs some built in Pike benchmarks.";
constant help = #"
Benchmarks Pike with %d built in benchmark tests.
Arguments:

-h, --help
  Shows this help text.

-m<number>, --max-runs=<number>
  Runs a test at most <number> of times. Defaults to 25.

-s<number>, --max-seconds=<number>
  Runs a test at most <number> of seconds, rounded up to the closest
  complete test. Defaults to 5.

-t<glob>[,<glob>...], --tests=<glob>[,<glob>...]
  Only run the specified tests.
";

int(0..) main(int num, array(string) args)
{
   mapping(string:Tools.Shoot.Test) tests=([]);
   array results=({});
   
   foreach (indices(Tools.Shoot),string test)
   {
      program p;
      Tools.Shoot.Test t;
      if ((programp(p=Tools.Shoot[test])) && 
	  (t=p())->perform)
	 tests[test]=t;
   }

   int seconds_per_test = 5;
   int maximum_runs = 25;
   array(string) test_globs = ({"*"});

   foreach(Getopt.find_all_options(args, ({
     ({ "help",    Getopt.NO_ARG,  "-h,--help"/"," }),
     ({ "maxrun",  Getopt.HAS_ARG, "-m,--max-runs"/"," }),
     ({ "maxsec",  Getopt.HAS_ARG, "-s,--max-seconds"/"," }),
     ({ "tests",   Getopt.HAS_ARG, "-t,--tests"/"," }),
   })), array opt)
    switch(opt[0])
    {
      case "help":
	write(help, sizeof(tests));
	return 0;
      case "maxrun":
	maximum_runs = (int)opt[1];
	break;
      case "maxsec":
	seconds_per_test = (int)opt[1];
	break;
      case "tests":
	test_globs = opt[1] / ",";
    }

   int ecode;
   ecode += Tools.Shoot.ExecTest("Overhead",Tools.Shoot.Overhead())
     ->run(0,1,1); // fill caches

   write("test                        total    user    mem   (runs)\n");

   /* Run overhead check first by default. */
   array(string) to_run = ({"Overhead"}) + (sort (indices (tests)) - ({"Overhead"}));
   array(string) to_run_names = rows (tests, to_run)->name;

   foreach (test_globs, string test_glob)
     foreach (to_run; int idx; string id)
       if (id && glob (test_glob, to_run_names[idx])) {
	 ecode += Tools.Shoot.ExecTest(id,tests[id])
	   ->run(seconds_per_test,maximum_runs);
	 // Don't run test more than once if it matches several globs.
	 to_run[idx] = 0;
       }
   return ecode;
}
