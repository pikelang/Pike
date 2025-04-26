
#pike __REAL_VERSION__
constant description = "Runs some built in Pike benchmarks.";
constant help = #"
Benchmarks Pike with %d built in benchmark tests.
Arguments:

-h, --help
  Shows this help text.

-l, --list
  Shows a list of available tests.

-s<number>, --max-seconds=<number>
  Runs a test at most <number> of seconds, rounded up to the closest
  complete test. Defaults to 3.

-t<glob>[,<glob>...], --tests=<glob>[,<glob>...]
  Only run the specified tests.

--json, -j
  Output result as JSON instead of human readable text

--compare=<file>, -c <file>
  Read a result previously created by saving the output of --json and
  print relative results
";


string dot( string a, int width, bool al, bool odd )
{
  string pre="",post="";
  int wanted = (width-strlen(a));
  string pad = " ."*(wanted/2+1);
  if( al )
    return pre+a+" "+pad[wanted&1..wanted-1-!(wanted&1)]+post;
  return pre+pad[1..wanted-1]+" "+a+post;
}

string color( float pct )
{
  if( pct > 0 )
  {
    if( pct > 20 )
      return "\e[31;1m";
    if( pct > 10 )
      return "\e[33;1m";
    return "";
  }
  if( pct < -10 )
    return  "\e[32;1m" ;
  return "";
}

mapping(string:Tools.Shoot.Test) tests;
bool json;
mapping comparison;
int seconds_per_test = 3;
array(string) test_globs = ({"*"});

int main(int num, array(string) args)
{
  tests = Tools.Shoot.tests();
  foreach(Getopt.find_all_options(args, ({
     ({ "help",    Getopt.NO_ARG,  "-h,--help"/"," }),
     ({ "maxsec",  Getopt.HAS_ARG, "-s,--max-seconds"/"," }),
     ({ "tests",   Getopt.HAS_ARG, "-t,--tests"/"," }),
     ({ "json",    Getopt.NO_ARG,  "-j,--json"/"," }),
     ({ "compare", Getopt.HAS_ARG, "-c,--compare"/"," }),
     ({ "list",    Getopt.NO_ARG,  "-l,--list"/"," }),
   })), array opt)
   {
    switch(opt[0])
    {
      case "json":
        json = true;
        break;
      case "compare":
        /* Convenience: When using make benchmark there will be some
           junk output before the actual json.
        */
        string data = Stdio.read_file( opt[1] );
        if( !data )
          exit(1,"Failed to read comparison file %q\n",
               combine_path( getcwd(), opt[1] ));
        if( sscanf( data, "%*[^{]{%s", data ) )
          data = "{"+data;
        comparison = Standards.JSON.decode( data );
        break;
      case "help":
        write(help, sizeof(tests));
        write("\nAvailable tests:\n%{    %s\n%}", sort(indices(tests)));
	return 0;
      case "maxsec":
	seconds_per_test = (int)opt[1];
	break;
      case "tests":
	test_globs = opt[1] / ",";
        break;
      case "list":
        write("Available tests:\n%{  %s\n%}",
              sort(indices(tests)));
        return 0;
        break;
    }
  }

  if( json )
    write("{\n");
  else if( !comparison )
    write("-"*59+"\n%-40s%19s\n"+"-"*59+"\n",
          "Test","Result");
  else
    write("-"*65+"\n%-40s%18s%7s\n"+"-"*65+"\n",
          "Test","Result","Change");

#if constant(Thread.Thread)
  call_out(Thread.Thread, 0, run_tests);
#else
  call_out(run_tests, 0);
#endif
  return -1;
}

void run_tests()
{
  int failed;
  mixed err = catch {
   array(string) to_run = glob(test_globs,sort(indices (tests)));
   mapping res;
   float total_pct;
   int n_tests;
   bool odd;
   bool isatty = Stdio.Terminfo.is_tty();

   foreach (to_run; int i; string id)
   {
     n_tests++;
     res = Tools.Shoot.run( tests[id], seconds_per_test );

     if (res->readable == "FAIL") failed = 1;

     if( json )
     {
       if( comparison )
       {
         if( int on = comparison[id]->n_over_time )
         {
           res->delta  = res->n_over_time - on;
           res->delta_pct = res->delta*100.0 / on;
         }
       }
       write( "%s%-40s", (i?",\n  ":"  "),"\""+id+"\":" );
       write( Standards.JSON.encode( res ) );
     }
     else if( comparison )
     {
       int on = comparison[id]->n_over_time;
       if( !on )
         write( dot(res->readable,19,false,odd)+"\n");
       else
       {
         int diff = res->n_over_time - on;
         float pct = diff*100.0 / on;
         total_pct += pct;
         if( isatty )
           write( color( pct ) );
         write("%42s%s %5.1f%%\n",
               dot(id,42,true,odd=!odd),
               dot(res->readable,16,false,odd),
               pct);
         if( isatty ) write( "\e[0m" );
       }
     }
     else
     {
       write(dot(id,42,true,odd=!odd) +
             dot(res->readable,17,false,odd)+"\n");
     }
   }
   if( json )
     write( "\n}\n");
   else if( comparison )
   {
     write("-"*65+"\n"+
           " "*40+"%24.1f%%\n"+
           "-"*65+"\n",
           total_pct / n_tests);
   }
   else
     write("-"*59+"\n");
    };
  if( err )
  {
    write("\n"+describe_backtrace(err));
    exit(1);
  }
  exit(failed);
}
