// $Id: benchmark.pike,v 1.4 2003/01/19 18:47:44 nilsson Exp $

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

-t<glob>, --tests=<glob>
  Only run the specified tests.
";

int(0..) main(int num, array(string) args)
{
   mapping(string:Tools.Shoot.Test) tests=([]);
   array results=({});
   
   foreach (indices(Tools.Shoot);;string test)
   {
      program p;
      Tools.Shoot.Test t;
      if ((programp(p=Tools.Shoot[test])) && 
	  (t=p())->perform)
	 tests[test]=t;
   }

   int seconds_per_test = 5;
   int maximum_runs = 25;
   string test_glob = "*";

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
	test_glob = opt[1];
    }

   int ecode;
   ecode += Tools.Shoot.ExecTest("Overhead",Tools.Shoot.Overhead())
     ->run(0,1,1); // fill caches

   write("test                        total    user    mem   (runs)\n");

/* always run overhead check first */
   foreach (glob(test_glob,({"Overhead"})+(sort(indices(tests))-({"Overhead"})));;string id)
   {
      ecode += Tools.Shoot.ExecTest(id,tests[id])
	->run(seconds_per_test,maximum_runs);
   }
   return ecode;
}
