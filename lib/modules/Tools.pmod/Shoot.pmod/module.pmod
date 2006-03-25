/* 
 *    Shootouts
 *        or 
 * Pike speed tests
 *
 */

#pike __REAL_VERSION__

import ".";

static int procfs=-1;
static Gmp.mpz dummy; // load the Gmp module

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
   if (sizeof(res) && !has_value(res[0],"/")
#ifdef __NT__
       && !has_value(res[0], "\\")
#endif /* __NT__ */
       )
      res[0] = locate_binary(getenv("PATH")/
#if defined(__NT__) || defined(__amigaos__)
			   ";"
#else
			   ":"
#endif
			   ,res[0]);
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
#if 0
	 werror("%O %s\n",
	       runpike+
	       ({ "-e","Tools.Shoot._shoot(\""+id+"\")" }),
		(runpike+
		 ({ "-e","Tools.Shoot._shoot(\""+id+"\")" }))*" ");
#endif
		
	 object p=
	    Process.create_process( 
	       runpike+
	       ({ "-e","Tools.Shoot._shoot(\""+id+"\")" }),
	       (["stdout":pipe->pipe(Stdio.PROP_IPC)]) );
	 int err = errno();
	 status=pipe->read();
	 int ret = p->wait();

	 if (status=="")
	 {
	    write("Failed to spawn pike or run test (code:%d, errno:%d)\n",
		  ret, err);
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

      write("%6.3fs %6.3fs %s %5s%s\n",
	    tseconds,
	    useconds,
	    memusage>0 ? sprintf("%5dkb", memusage/1024) : "   ?   ",
	    "("+nruns+")",
	    test->present_n
	    ?" ("+(tg
		   ?test->present_n(testntot,nruns,truns,tg,memusage)
		   :"no time?")+")"
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

   if (Stdio.is_dir("/proc/"+getpid()+"/.")) {
     string s;
     if ((s=Stdio.read_bytes("/proc/"+getpid()+"/statm")))
       write("%d\n",((int)s)*4096);
     else {
       int offset = 44;
       int bytes = 4;
       if (Pike.get_runtime_info()->abi == 64) {
	 // Adjust for pr_addr being 8 bytes.
	 offset += 4;
	 // pr_size is also 8 bytes.
	 bytes *= 2;
       }
       // We're only interested in pr_size.
       if ( (s=Stdio.read_bytes("/proc/"+getpid()+"/psinfo", offset, bytes)) &&
	    (sizeof(s) == bytes)) {
	 sscanf(s, (Pike.get_runtime_info()->abi == 64)?
		(Pike.get_runtime_info()->native_byteorder == 4321)?
		"%8c":"%-8c":
		(Pike.get_runtime_info()->native_byteorder == 4321)?
		"%4c":"%-4c",
		int pr_size);
	 write("%d\n", pr_size * 1024);
       } else 
	 write("-1\n");
     }
   } else
     write("-1\n");

   write("%.6f\n",tg);

   if (test->present_n)
     write("%d\n",test->n);
   else
     write("no\n");
}
