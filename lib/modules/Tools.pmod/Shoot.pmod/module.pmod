/* 
 *    Shootouts
 *        or 
 * Pike speed tests
 *
 */

import ".";

int procfs=-1;

// internal to find a pike binary

private static string locate_binary(array path, string name)
{
   string dir;
   object info;
   foreach(path, dir)
   {
      string fname = dir + "/" + name;
      if ((info = file_stat(fname))
	  && (info[0] & 0111))
	 return fname;
   }
   return 0;
}

// internal to find/make pike exec arguments

static array runpike=
lambda()
{
   array res=({master()->_pike_file_name});
   if (master()->_master_file_name)
      res+=({"-m"+master()->_master_file_name});
   foreach (master()->pike_module_path;;string path)
      res+=({"-M"+path});
   if (sizeof(res) && !has_value(res[0],"/"))
      res[0]=locate_binary(getenv("PATH")/":",res[0]);
   return res;
}();

//! The test call/result class.
//! Instantiated with a test id and the test object itself.
class ExecTest(string id,Test test)
{
   float useconds;
   float tseconds;
   int nruns;
   int memusage;

   //! This function runs the actual test, by spawning off
   //! a new pike and call it until at least one of these conditions:
   //! maximum_seconds has passed, or
   //! the number of runs is at least maximum_runs.
   int(0..1) run(int maximum_seconds,
	    int maximum_runs,
	    void|int silent)
   {
      int t0=time();
      float tz=time(t0),truns;
      nruns=0;
      string status;
      float tg=0.0;
      int testntot=0;

      if (!silent) 
	 write(test->name+"..........................."[sizeof(test->name)..]);

      for (;;)
      {
	 nruns++;
	 Stdio.File pipe=Stdio.File();
	 object p=
	    Process.create_process( 
	       runpike+
	       ({ "-e","Tools.Shoot._shoot(\""+id+"\")" }),
	       (["stdout":pipe->pipe(Stdio.PROP_IPC)]) );
	 status=pipe->read();
	 p->wait();

	 if (status=="")
	 {
	    write("failed to spawn pike or run test\n");
	    return 1;
	 }

	 array v=status/"\n";
	 memusage=(int)v[0];
	 tg+=(float)v[1];
	 if (v[2]!="no")
	    testntot+=(int)v[2];

	 truns=time(t0)-tz;
	 if (truns >= maximum_seconds || 
	     nruns>=maximum_runs) break;
      }
      useconds=tg/nruns;
      tseconds=truns/nruns;

      if (silent) return 0;

      write("%6.3fs %6.3fs %5dkb %5s%s\n",
	    tseconds,
	    useconds,
	    memusage/1024,
	    "("+nruns+")",
	    test->present_n
	    ?" ("+test->present_n(testntot,nruns,tseconds,
				  useconds,memusage)+")"
	    :"");
      return 0;
   }
}

//! This function is called in the spawned pike,
//! to perform the test but also to write some important 
//! data to stdout. @[id] is the current test.
void _shoot(string id)
{
   object test;
   float tg=gauge { (test=Tools.Shoot[id]())->perform(); };

   string s;
   if ( (s=Stdio.read_bytes("/proc/"+getpid()+"/statm")) )
      write("%d\n",((int)s)*4096);
   else 
      write("-1\n");

   write("%.6f\n",tg);

   if (test->present_n)
      write("%d\n",test->n);
   else
      write("no\n");
}
