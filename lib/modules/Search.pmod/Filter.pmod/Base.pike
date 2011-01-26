// $Id$
#pike __REAL_VERSION__

//! The MIME content types this class can filter.
constant contenttypes = ({ });

//!
.Output filter(Standards.URI uri, string|Stdio.File data,
	      string content_type, mixed ... more);


string my_popen(array(string) args, string|void cwd, int|void wait_for_exit,
		array(string)|void watchdog_args)
  // A smarter version of Process.popen: No need to quote arguments.
{    
  Stdio.File pipe0 = Stdio.File();
  Stdio.File pipe1 = pipe0->pipe(Stdio.PROP_IPC);
  if(!pipe1)
    if(!pipe1) error("my_popen failed (couldn't create pipe).\n");
  mapping setup = ([ "env":getenv(), "stdout":pipe1, "nice": 20 ]);
  if (cwd)
    setup["cwd"] = cwd;
  Process.create_process proc = Process.create_process(args, setup);
  pipe1->close();

  //  Launch watchdog if provided (requires wait_for_exit)
  Process.Process watchdog;
  if (wait_for_exit && watchdog_args) {
    //  Insert pid of running process where caller used "%p"
    array(string) wd_args =
      replace(watchdog_args + ({ }), "%p", (string) proc->pid());
    watchdog = Process.spawn_pike(wd_args);
  }
  
  string result = pipe0->read();
  if(!result)
    error("my_popen failed with error "+pipe0->errno()+".\n");
  pipe0->close();
  if (wait_for_exit)
    proc->wait();
  
  if (watchdog)
    watchdog->kill(9);
  
  return result;
}
