
#pike __REAL_VERSION__
constant description = "Runs some built in Pike benchmarks.";
constant help = #"
Benchmarks Pike with %d built in benchmark tests.
Arguments:

-h, --help
  Shows this help text.

-s<number>, --max-seconds=<number>
  Runs a test at most <number> of seconds, rounded up to the closest
  complete test. Defaults to 3.

-t<glob>[,<glob>...], --tests=<glob>[,<glob>...]
  Only run the specified tests.

--json, -j
  Output result as JSON instead of human readable text

--compare, -c
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

int(0..) main(int num, array(string) args)
{
  mapping(string:Tools.Shoot.Test) tests= Tools.Shoot.tests();
  int seconds_per_test = 3;
  array(string) test_globs = ({"*"});
  bool json;
  mapping comparison;

  foreach(Getopt.find_all_options(args, ({
     ({ "help",    Getopt.NO_ARG,  "-h,--help"/"," }),
     ({ "maxsec",  Getopt.HAS_ARG, "-s,--max-seconds"/"," }),
     ({ "tests",   Getopt.HAS_ARG, "-t,--tests"/"," }),
     ({ "json",    Getopt.NO_ARG, "-j,--json"/"," }),
     ({ "compare",    Getopt.HAS_ARG, "-c,--compare"/"," }),
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
	return 0;
      case "maxsec":
	seconds_per_test = (int)opt[1];
	break;
      case "tests":
	test_globs = opt[1] / ",";
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

   /* Run overhead check first. */
   float overhead_time;
   array(string) to_run = glob(test_globs,sort(indices (tests)-({"Overhead"})));
   mapping res = Tools.Shoot.run( tests["Overhead"], 1, 0.0 );
   float total_pct;
   int n_tests;
   overhead_time = res->time / res->n;
   bool odd;
   bool isatty = Stdio.Terminfo.is_tty();

   foreach (to_run; int i; string id)
   {
     n_tests++;
     res = Tools.Shoot.run( tests[id], seconds_per_test, overhead_time );

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
         write("%40s%s %5.1f%%\n",
               dot(id,40,true,odd=!odd),
               dot(res->readable,18,false,odd),
               pct);
         if( isatty ) write( "\e[0m" );
       }
     }
     else
     {
       write(dot(id,40,true,odd=!odd) +
             dot(res->readable,19,false,odd)+"\n");
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
     write("-"*58+"\n");
}

