#! /usr/bin/env pike

/* $Id: test_pike.pike,v 1.98 2004/04/23 16:05:26 grubba Exp $ */

#if !constant(_verify_internals)
#define _verify_internals()
#endif

#if !constant(_dmalloc_set_name)
void _dmalloc_set_name(mixed ... args) {}
#endif

int foo(string opt)
{
  if(opt=="" || !opt) return 1;
  return (int)opt;
}

int maybe_tty = 1;

mapping(string:int) cond_cache=([]);

#if constant(thread_create)
#define HAVE_DEBUG
#endif

void print_code(string test)
{
  array lines = Locale.Charset.encoder("iso-8859-1", 0,
				       lambda(string s) {
					 return sprintf("\\%o", s[0]);
				       })->feed(test)->drain()/"\n";
  foreach(lines; int r; string line) {
    werror("%3d: %s\n", r+1, line);
  }
  werror("\n");
  return;
}

array find_testsuites(string dir)
{
  array(string) ret=({});
  if(array(string) s=get_dir(dir||"."))
  {
    if(has_value(s,"no_testsuites")) return ret;
    foreach(s, string file)
      {
	switch(file)
	{
	  case "testsuite":
	  case "module_testsuite":
	    ret+=({ combine_path(dir||"",file) });
	}
      }

    foreach(s, string file)
      {
	string name=combine_path(dir||"",file);
	if(Stdio.is_dir(name))
	  ret+=find_testsuites(name);
      }
  }
  return ret;
}

array(string) read_tests( string fn ) {
  string|array(string) tests = Stdio.read_file( fn );
  if(!tests) {
    werror("Failed to read test file %O, errno=%d.\n",
	   fn, errno());
    exit(1);
  }

  tests = tests/"\n....\n";
  return tests[0..sizeof(tests)-2];
}

class WarningFlag {
  int(0..1) warning;
  array(string) warnings = ({});

  void compile_warning(string file, int line, string text) {
    warnings += ({ line+": "+text });
    warning = 1;
  }

  void compile_error(string file, int line, string text) {
    werror("%s:%d: %s\n", file,line,text);
  }
}

//
// Watchdog stuff
//

// 20 minutes should be enough..
#if !constant(_reset_dmalloc)
#define WATCHDOG_TIMEOUT 60*20
#else
// ... unless we're running dmalloc
#define WATCHDOG_TIMEOUT 60*80
#endif

#if constant(thread_create)
#define WATCHDOG
#define WATCHDOG_PIPE
object watchdog_pipe;
#else
#if constant(kill)
#define WATCHDOG
#define WATCHDOG_SIGNAL
#endif
#endif

#ifdef WATCHDOG
object watchdog;
int use_watchdog=1;
int watchdog_time;

void signal_watchdog()
{
#ifdef WATCHDOG
  if(use_watchdog && time() - watchdog_time > 30)
  {
    watchdog_time=time();
#ifdef WATCHDOG_PIPE
    watchdog_pipe->write("x",1);
#endif

#ifdef WATCHDOG_SIGNAL
    watchdog->kill(signum("SIGQUIT"));
#endif
  }
#endif
}
#endif

void run_watchdog(int pid) {
#ifdef WATCHDOG
  int cnt=0;
  int last_time=time();
  int exit_quietly;
#ifdef WATCHDOG_PIPE
  thread_create(lambda() {
		  object o=Stdio.File("stdin");
		  while(sizeof(o->read(1) || ""))
		  {
		    last_time=time();
		  }
		  exit_quietly=1;
		});
#endif // WATCHDOG_PIPE

#ifdef WATCHDOG_SIGNAL
  werror("Setting signal (1)\n");
  if(signum("SIGQUIT")>=0)
  {
    werror("Setting signal (2)\n");
    signal( signum("SIGQUIT"),
	    lambda() {
	      last_time=time();
	    });
  }
  else {
    exit(1);
  }
#endif // WATCHDOG_SIGNAL

  while(1)
  {
    sleep(10);
    if(exit_quietly) _exit(0);
#ifndef __NT__
    if(!kill(pid, 0)) _exit(0);
#endif

    /* I hope 30 minutes per test is enough for everybody */
    if(time() - last_time > WATCHDOG_TIMEOUT)
    {
      werror("\n[WATCHDOG] Pike testsuite timeout, sending SIGABRT.\n");
      kill(pid, signum("SIGABRT"));
      for(int q=0;q<60;q++) if(!kill(pid,0)) _exit(0); else sleep(1);
      werror("\n"
	     "[WATCHDOG] This is your friendly watchdog again...\n"
	     "[WATCHDOG] testsuite failed to die from SIGABRT, sending SIGKILL\n");
      kill(pid, signum("SIGKILL"));
      for(int q=0;q<60;q++) if(!kill(pid,0)) _exit(0); else sleep(1);
      werror("\n"
	     "[WATCHDOG] This is your friendly watchdog AGAIN...\n"
	     "[WATCHDOG] SIGKILL, SIGKILL, SIGKILL, DIE!\n");
      kill(pid, signum("SIGKILL"));
      kill(pid, signum("SIGKILL"));
      kill(pid, signum("SIGKILL"));
      kill(pid, signum("SIGKILL"));
      for(int q=0;q<60;q++) if(!kill(pid,0)) _exit(0); else sleep(1);
      werror("\n"
	     "[WATCHDOG] Giving up, must be a device wait.. :(\n");
      _exit(0);
    }
  }
#else
  _exit(1);
#endif // else WATCHDOG
}

//
// Main program
//

int main(int argc, array(string) argv)
{
  int e, verbose, prompt, successes, errors, t, check, asmdebug;
  int skipped;
  array(string) tests;
  array(string) forked;
  program testprogram;
  int start, fail, mem;
  int loop=1;
  int end=0x7fffffff;
  string extra_info="";
  int shift;

#if constant(System.getrlimit)
  // Attempt to enable coredumps.
  // Many Linux distributions default to having coredumps disabled.
  catch {
    [ int current, int max ] = System.getrlimit("core");
    if ((current != -1) && ((current < max) || (max == -1))) {
      // Not unlimited, and less than max.
      // Attempt to raise.
      System.setrlimit("core", max, max);
    }
  };
#endif /* constant(System.getrlimit) */

  if(signum("SIGQUIT")>=0)
  {
    signal(signum("SIGQUIT"),lambda()
	   {
	     master()->handle_error( ({"\nSIGQUIT recived, printing backtrace and continuing.\n",backtrace() }) );

	     mapping x=_memory_usage();
	     foreach(sort(indices(x)),string p)
	     {
	       if(sscanf(p,"%s_bytes",p))
	       {
		 werror("%20ss:  %8d   %8d Kb\n",p,
			x["num_"+p+"s"],
			x[p+"_bytes"]/1024);
	       }
	     }
	   });
  }

  array(string) args=backtrace()[0][3];
  array(string) testsuites=({});
  args=args[..sizeof(args)-1-argc];
  add_constant("RUNPIKE_ARRAY", args);
  add_constant("RUNPIKE", map(args, Process.sh_quote)*" ");

  foreach(Getopt.find_all_options(argv,aggregate(
    ({"no-watchdog",Getopt.NO_ARG,({"--no-watchdog"})}),
    ({"watchdog",Getopt.HAS_ARG,({"--watchdog"})}),
    ({"help",Getopt.NO_ARG,({"-h","--help"})}),
    ({"verbose",Getopt.MAY_HAVE_ARG,({"-v","--verbose"})}),
    ({"prompt",Getopt.NO_ARG,({"-p","--prompt"})}),
    ({"start",Getopt.HAS_ARG,({"-s","--start-test"})}),
    ({"end",Getopt.HAS_ARG,({"-e","--end-after"})}),
    ({"fail",Getopt.NO_ARG,({"-f","--fail"})}),
    ({"forked",Getopt.NO_ARG,({"-F","--forked"})}),
    ({"loop",Getopt.HAS_ARG,({"-l","--loop"})}),
    ({"trace",Getopt.HAS_ARG,({"-t","--trace"})}),
    ({"check",Getopt.MAY_HAVE_ARG,({"-c","--check"})}),
#if constant(_assembler_debug)
    ({"asm",Getopt.MAY_HAVE_ARG,({"--assembler-debug"})}),
#endif
    ({"mem",Getopt.NO_ARG,({"-m","--mem","--memory"})}),
    ({"auto",Getopt.MAY_HAVE_ARG,({"-a","--auto"})}),
    ({"notty",Getopt.NO_ARG,({"-T","--notty"})}),
#ifdef HAVE_DEBUG
    ({"debug",Getopt.MAY_HAVE_ARG,({"-d","--debug"})}),
#endif
    ({"regression",Getopt.NO_ARG,({"-r","--regression"})}),
    )),array opt)
    {
      switch(opt[0])
      {
	case "no-watchdog":
	  use_watchdog=0;
	  break;

	case "watchdog":
	  run_watchdog( (int)opt[1] );
	  break;
	
	case "notty":
	  maybe_tty = 0;
	  break;

	case "help":
	  write(doc);
	  return 0;

	case "verbose": verbose+=foo(opt[1]); break;
	case "prompt": prompt+=foo(opt[1]); break;
	case "start": start=foo(opt[1]); start--; break;
	case "end": end=foo(opt[1]); break;
	case "fail": fail=1; break;
        case "forked": {
 	  array(string) orig_argv = backtrace()[0][3];
	  int i = search(orig_argv, argv[0]);
	  if (i < 0) {
	    werror("Forked operation failed: Failed to find %O in %O\n",
		   argv[0], orig_argv);
	    break;
	  }
	  forked = orig_argv[..i];
	  break;
	}
	case "loop": loop=foo(opt[1]); break;
	case "trace": t+=foo(opt[1]); break;
	case "check": check+=foo(opt[1]); break;
        case "asm": asmdebug+=foo(opt[1]); break;
	case "mem": mem=1; break;

	case "auto":
	  if(stringp(opt[1]))
	    testsuites=find_testsuites(opt[1]);
	  else
	    testsuites=find_testsuites(".");
	  break;

        case "regression":
	  add_constant("regression", 1);
	  break;

#ifdef HAVE_DEBUG
	case "debug":
	{
	  object p=Stdio.Port();
	  p->bind(0);
	  werror("Debug port is: %s\n",p->query_address());
	  sscanf(p->query_address(),"%*s %d",int portno);
	  extra_info+=sprintf(" dport:%d",portno);
	  thread_create(lambda(object p){
	    while(p)
	    {
	      if(object o=p->accept())
	      {
		object q=Stdio.FILE();
		q->assign(o);
		destruct(o);
		Tools.Hilfe.GenericHilfe(q,q);
	      }
	    }
	  },p);
	}
#endif
      }
    }

  if (forked) {
    if (!use_watchdog) forked += ({ "--no-watchdog" });
    if (!maybe_tty) forked += ({ "--notty" });
    if (verbose) forked += ({ "--verbose=" + verbose });
    if (prompt) forked += ({ "--prompt=" + prompt });
    if (start) forked += ({ "--start-test=" + (start+1) });
    // FIXME: end handling is not quite correct.
    if (end != 0x7fffffff) forked += ({ "--end-after=" + end });
    if (fail) forked += ({ "--fail" });
    // forked is handled here.
    // loop is handled here.
    if (t) forked += ({ "--trace=" + t });
    if (check) forked += ({ "--check=" + check });
    if (asmdebug) forked += ({ "--asm=" + asmdebug });
    if (mem) forked += ({ "--memory" });
    // auto already handled.
    if (all_constants()->regression) forked += ({ "--regression" });
    // debug port not propagated.
    //werror("forked:%O\n", forked);
  }

  add_constant("_verbose", verbose);
  if(verbose)
    write("Begin tests at "+ctime(time()));

#ifdef WATCHDOG
  int watchdog_time=time();

  if(use_watchdog && !forked)
  {
#ifdef WATCHDOG_PIPE
    object watchdog_tmp=Stdio.File();
    watchdog_pipe=watchdog_tmp->pipe(Stdio.PROP_IPC);
    watchdog=Process.create_process(
      backtrace()[0][3] + ({  "--watchdog="+getpid() }),
      (["stdin":watchdog_tmp ]));
    destruct(watchdog_tmp);
#endif

#ifdef WATCHDOG_SIGNAL
    watchdog=Process.create_process(
      backtrace()[0][3] + ({  "--watchdog="+getpid() }) );
#endif
  }
  add_constant("__signal_watchdog",signal_watchdog);
#else
  add_constant("__signal_watchdog",lambda(){});
#endif // else WATCHDOG_PIPE

  testsuites += Getopt.get_args(argv, 1)[1..];
  if(!sizeof(testsuites))
  {
    werror("No tests found. Use --help for more information.\n");
    exit(1);
  }

#if 1
  // Store the name of all constants so that we can see
  // if any constant has been leaked from the testsuite.
  array const_names = indices(all_constants());
#endif

#if constant(_assembler_debug)
  if(asmdebug)
    _assembler_debug(asmdebug);
#endif

  while(loop--)
  {
    successes=errors=0;
    if (forked) {
      foreach(testsuites, string testsuite) {
	Stdio.File p = Stdio.File();
	object pid =
	  Process.create_process(forked + ({ testsuite }),
				 ([ "stdout":p->pipe(Stdio.PROP_IPC) ]));
	string raw_results;
	string results = lower_case(raw_results = p->read());
	int err = pid->wait();
	int total = 0;
	int failed = 0;
	int skip = 0;
	if (((sscanf(results, "%*sfailed tests:%d", failed) != 2) +
	     (sscanf(results, "%*stotal tests:%d", total) != 2) +
	     (sscanf(results, "%*s(%d tests skipped)", skip) != 2)) == 3) {
	  // Failed to parse the result totally.
	  werror("Failed to parse subresult for testsuite %O (exitcode:%d):\n"
		 "%s", testsuite, err, raw_results);
	  errors++;
	} else {
	  werror("Subresult: %d tests, %d failed, %d skipped\n",
		 total, failed, skip);
	  errors += failed;
	  successes += total - failed;
	  skipped += skip;
	}
	if (fail && errors) {
	  exit(1);
	}
      }
    } else {
  testloop:
    foreach(testsuites, string testsuite)
    {
      tests = read_tests( testsuite );

      werror("Doing tests in %s (%d tests)\n", testsuite, sizeof(tests));
      int qmade, qskipped, qmadep, qskipp;

      int testno, testline;
      for(e=start;e<sizeof(tests);e++)
      {
	signal_watchdog();

	int skip=0, prev_errors = errors;
	object o;
	mixed a,b;

	// Extra consistency checks?
	if(check)
        {
	  if(check>0 || (e % -check)==0 )
	      _verify_internals();
	}
	if(check>3) {
	  gc();
	  _verify_internals();
	}

	string test = tests[e];

	// Is there a condition for this test?
	string condition;
	if( sscanf(test, "COND %s\n%s", condition, test)==2 )
        {
	  int tmp;
	  if(!(tmp=cond_cache[condition]))
	  {
	    mixed err = catch {
	      tmp=!!(compile_string("mixed c() { return "+condition+"; }",
				    "Cond "+(e+1))()->c());
	    };

	    if(err) {
	      werror( "\nConditional %d %s failed:\n"
		      "%s\n", e+1, testline?"(line "+testline+")":"",
		      describe_backtrace(err) );
	      errors++;
	      tmp = -1;
	    }

	    if(!tmp) tmp=-1;
	    cond_cache[condition]=tmp;
	  }

	  if(tmp==-1)
	  {
	    if(verbose>1)
	      werror("Not doing test "+(e+1)+"\n");
	    successes++;
	    skipped++;
	    skip=1;
	  }
	}

	string|int type;
	sscanf(test, "%s\n%s", type, test);

	string testfile;
	sscanf(type, "%s: test %d, expected result: %s", testfile, testno, type);

	if (testfile) {
	  array split = testfile / ":";
	  testline = (int) split[-1];
	  testfile = split[..sizeof (split) - 2] * ":";
	}

	string pad_on_error = "\n";
	if(maybe_tty && Stdio.Terminfo.is_tty())
        {
	  if(verbose<2) {
	    werror("test %d, line %d\r", e+1, testline);
	    pad_on_error = "                                        \r";
	  }
	}
	else if(verbose){
	  if(skip) {
	    if(qmade) werror(" Made %d test%s.\n", qmade, qmade==1?"":"s");
	    qmade=0;
	    qskipp=1;
	    qskipped++;
	  }
	  else {
	    if(qskipped) werror(" Skipped %d test%s.\n", qskipped, qskipped==1?"":"s");
	    qskipped=0;
	    qmadep=1;
	    qmade++;
	  }
	}
	else {
	  /* Use + instead of . so that sendmail and
	   * cron will not cut us off... :(
	   */
	  switch( (e-start) % 50)
	  {
	  case 0:
	    werror("%5d: ", e);
	    // fallthrough

	  default:
	    werror(skip?"-":"+");
	    break;
		
	  case 9:
	  case 19:
	  case 29:
	  case 39:
	    werror(skip?"- ":"+ ");
	    break;

	  case 49:
	    werror(skip?"-\n":"+\n");
	  }
	}
	if(skip) continue;

	if (!testfile || !testno || !type) {
	  werror ("%sInvalid format in test file: %O\n", pad_on_error, type);
	  errors++;
	  continue;
	}

	if (prompt) {
	  if (Stdio.Readline()->
	      read(sprintf("About to run test: %d. [<RETURN>/'quit']: ",
			   testno)) == "quit") {
	    break testloop;
	  }
	}

	if(verbose>1)
	{
	  werror("Doing test %d (%d total) at %s:%d%s\n",
		 testno, successes+errors+1, testfile, testline, extra_info);
	  if(verbose>2) print_code(test);
	}

	if(check > 1) _verify_internals();
	
	shift++;
	string fname = testfile + ":" + testline + ": Test " + testno +
	  " (shift " + (shift%3) + ")";

	string widener = ([ 0:"",
			    1:"\nint \x30c6\x30b9\x30c8=0;\n",
			    2:"\nint \x10001=0;\n" ])[shift%3];

	// widener += "#pragma strict_types\n";

	if(test[-1]!='\n') test+="\n";

	int computed_line=sizeof(test/"\n");
	array gnapp= test/"#";
	for(int e=1;e<sizeof(gnapp);e++)
	{
	  if(sscanf(gnapp[e],"%*d"))
	  {
	    computed_line=0;
	    break;
	  }
	}
	string linetester="int __cpp_line=__LINE__; int __rtl_line=[int]backtrace()[-1][1];\n";

	string to_compile = test + linetester + widener;

	if((shift/6)&1)
	{
	  if(search("don't save parent",to_compile) != -1)
	  {
	    fname+=" (save parent)";
	    to_compile=
	      "#pragma save_parent\n"
	      "# 1\n"
	      +to_compile;
	  }
	}

	if((shift/3)&1)
	{
	  fname+=" (CRNL)";
	  to_compile=replace(to_compile,"\n","\r\n");
	}

	// _optimizer_debug(5);
	
	if(verbose>9) print_code(to_compile);
	WarningFlag wf;
	switch(type)
        {
	  mixed at,bt;
	  mixed err;
	case "COMPILE":
	  wf = WarningFlag();
	  master()->set_inhibit_compile_errors(wf);
	  _dmalloc_set_name(fname,0);
	  if(catch(compile_string(to_compile, testsuite)))
	  {
	    _dmalloc_set_name();
	    master()->set_inhibit_compile_errors(0);
	    werror(pad_on_error + fname + " failed.\n");
	    print_code(test);
	    errors++;
	  }
	  else {
	    _dmalloc_set_name();
	    master()->set_inhibit_compile_errors(0);

	    if(wf->warning) {
	      werror(pad_on_error + fname + " produced warning.\n");
	      werror("%{%s\n%}", wf->warnings);
	      print_code(test);
	      errors++;
	      break;
	    }

	    successes++;
	  }
	  break;
	
	case "COMPILE_ERROR":
	  master()->set_inhibit_compile_errors(1);
	  _dmalloc_set_name(fname,0);
	  if(catch(compile_string(to_compile, testsuite)))
	  {
	    _dmalloc_set_name();
	    successes++;
	  }
	  else {
	    _dmalloc_set_name();
	    werror(pad_on_error + fname + " failed (expected compile error).\n");
	    print_code(test);
	    errors++;
	  }
	  master()->set_inhibit_compile_errors(0);
	  break;

	case "COMPILE_WARNING":
	  wf = WarningFlag();
	  master()->set_inhibit_compile_errors(wf);
	  _dmalloc_set_name(fname,0);
	  if(catch(compile_string(to_compile, testsuite)))
	  {
	    _dmalloc_set_name();
	    werror(pad_on_error + fname + " failed.\n");
	    print_code(test);
	    errors++;
	  }
	  else {
	    _dmalloc_set_name();
	    if( wf->warning )
	      successes++;
	    else {
	      werror(pad_on_error + fname + " failed (expected compile warning).\n");
	      print_code(test);
	      errors++;
	    }
	  }
	  master()->set_inhibit_compile_errors(0);
	  break;

	case "EVAL_ERROR":
	  master()->set_inhibit_compile_errors(1);
	  _dmalloc_set_name(fname,0);

	  at = gauge {
	    err=catch {
	      a = compile_string(to_compile, testsuite)()->a();
	    };
	  };
	  if(err)
	  {
	    _dmalloc_set_name();
	    successes++;
	    if(verbose>3)
	      werror("Time in a(): %f\n",at);
	  }
	  else {
	    _dmalloc_set_name();
	    werror(pad_on_error + fname + " failed (expected eval error).\n");
	    werror("Got %O\n", a);
	    print_code(test);
	    errors++;
	  }
	  master()->set_inhibit_compile_errors(0);
	  break;
	
	default:
	  if (err = catch{
	    wf = WarningFlag();
	    master()->set_inhibit_compile_errors(wf);
	    _dmalloc_set_name(fname,0);
	    o=compile_string(to_compile,testsuite)();
	    _dmalloc_set_name();

	    if(check > 1) _verify_internals();
	
	    a=b=0;
	    if(t) trace(t);
	    _dmalloc_set_name(fname,1);
	    if(functionp(o->a))
	    {
	      // trace(10);
	      at = gauge { a=o->a(); };
	      // trace(0);
	    }

	    if(functionp(o->b))
	    {
	      bt = gauge { b=o->b(); };
	    }
		
	    _dmalloc_set_name();

	    if(t) trace(0);
	    if(check > 1) _verify_internals();

	    if(wf->warning) {
	      werror(pad_on_error + fname + " produced warning.\n");
	      werror("%{%s\n%}", wf->warnings);
	      print_code(test);
	      errors++;
	      break;
	    }
	    master()->set_inhibit_compile_errors(0);

	  }) {
	    if(t) trace(0);
	    master()->set_inhibit_compile_errors(0);
	    werror(pad_on_error + fname + " failed.\n");
	    print_code(test);
	    if (arrayp(err) && sizeof(err) && stringp(err[0])) {
	      werror("Error: " + master()->describe_backtrace(err));
	    }
	    if (objectp(err)) {
	      werror("Error: " + master()->describe_backtrace(err));
	    }
	    errors++;
	    break;
	  }

	  if( o->__cpp_line != o->__rtl_line ||
	      ( computed_line && computed_line!=o->__cpp_line))
	    {
	      werror(pad_on_error + fname + " Line numbering failed.\n");
	      print_code(to_compile);
	      werror("   Preprocessed:\n");
	      print_code(cpp(to_compile, fname));
	      werror("   CPP lines: %d\n",o->__cpp_line);
	      werror("   RTL lines: %d\n",o->__rtl_line);
	      if(computed_line)
		werror("Actual lines: %d\n",computed_line);
	      errors++;
	    }

	  if(verbose>2)
	    werror("Time in a(): %f, Time in b(): %O\n",at,bt);
	
	  switch(type)
	  {
	  case "FALSE":
	    if(a)
	    {
	      werror(pad_on_error + fname + " failed.\n");
	      print_code(test);
	      werror(sprintf("o->a(): %O\n",a));
	      errors++;
	    }
	    else {
	      successes++;
	    }
	    break;
		
	  case "TRUE":
	    if(!a)
	    {
	      werror(pad_on_error + fname + " failed.\n");
	      print_code(test);
	      werror(sprintf("o->a(): %O\n",a));
	      errors++;
	    }
	    else {
	      successes++;
	    }
	    break;
		
	  case "RUN":
	    successes++;
	    break;

	  case "RUNCT":
	    if(!a || !arrayp(a) || sizeof(a)!=2 || !intp(a[0]) || !intp(a[1])) {
	      werror(pad_on_error + fname + " failed to return proper results.\n");
	      print_code(test);
	      werror(sprintf("o->a(): %O\n",a));
	      errors++;
	    }
	    else {
	      successes += a[0];
	      errors += a[1];
	      if (verbose>1)
		if(a[1])
		  werror("%d/%d tests failed.\n", a[1], a[0]+a[1]);
		else
		  werror("Did %d tests in %s.\n", a[0], fname);
	    }
	    break;

	  case "EQ":
	    if(a!=b)
	    {
	      werror(pad_on_error + fname + " failed.\n");
	      print_code(test);
	      werror(sprintf("o->a(): %O\n",a));
	      werror(sprintf("o->b(): %O\n",b));
	      errors++;
	      if (stringp(a) && stringp(b) && (sizeof(a) == sizeof(b)) &&
		  (sizeof(a) > 20)) {
		werror("Differences at:\n");
		int i;
		for(i = 0; i < sizeof(a); i++) {
		  if (a[i] != b[i]) {
		    werror("  %4d: 0x%04x != 0x%04x\n", i, a[i], b[i]);
		  }
		}
	      }
	    }
	    else {
	      successes++;
	    }
	    break;
		
	  case "EQUAL":
	    if(!equal(a,b))
	    {
	      werror(pad_on_error + fname + " failed.\n");
	      print_code(test);
	      werror(sprintf("o->a(): %O\n",a));
	      werror(sprintf("o->b(): %O\n",b));
	      errors++;
	      if (stringp(a) && stringp(b) && (sizeof(a) == sizeof(b)) &&
		  (sizeof(a) > 20)) {
		werror("Differences at:\n");
		int i;
		for(i = 0; i < sizeof(a); i++) {
		  if (a[i] != b[i]) {
		    werror("  %4d: 0x%04x != 0x%04x\n", i, a[i], b[i]);
		  }
		}
	      }
	    }
	    else {
	      successes++;
	    }
	    break;
		
	  default:
	    werror(sprintf("\n%s: Unknown test type (%O).\n", fname, type));
	    errors++;
	  }
	}

	if (errors > prev_errors) werror ("\n");

	if(check > 2) _verify_internals();
	
	if(fail && errors)
	  exit(1);

	if(successes+errors > end)
	{
	  break testloop;
	}
	
	a=b=0;
      }

      if(maybe_tty && Stdio.Terminfo.is_tty())
      {
	werror("                                        \r");
      }
      else if(verbose) {
	if(!qskipp && !qmadep);
	else if(!qskipp) werror("Made all tests\n");
	else if(!qmadep) werror("Skipped all tests\n");
	else if(qmade) werror(" Made %d test%s.\n", qmade, qmade==1?"":"s");
	else if(qskipped) werror(" Skipped %d test%s.\n", qskipped, qskipped==1?"":"s");
      }
      else {
	werror("\n");
      }
    }
    }
    if(mem)
    {
      int total;
      tests=0;
      gc();
      mapping tmp=_memory_usage();
      werror(sprintf("%-10s: %6s %10s\n","Category","num","bytes"));
      foreach(sort(indices(tmp)),string foo)
      {
	if(sscanf(foo,"%s_bytes",foo))
	{
	  werror(sprintf("%-10s: %6d %10d\n",
			foo+"s",
			tmp["num_"+foo+"s"],
			tmp[foo+"_bytes"]));
	  total+=tmp[foo+"_bytes"];
	}
      }
      werror( "%-10s: %6s %10d\n",
	     "Total", "", total );
    }
  }
  if(errors || verbose>1)
  {
    write("Failed tests: "+errors+".\n");
  }

  write("Total tests: %d  (%d tests skipped)\n",successes+errors,skipped);
  if(verbose)
    write("Finished tests at "+ctime(time()));

#if 1
  if(verbose && sizeof(all_constants())!=sizeof(const_names)) {
    multiset const_names = (multiset)const_names;
    foreach(indices(all_constants()), string const)
      if( !const_names[const] )
	werror("Leaked constant %O\n", const);
  }
#endif

  add_constant("regression");
  add_constant("_verbose");
  add_constant("__signal_watchdog");
  add_constant("RUNPIKE");

#if defined(WATCHDOG_SIGNAL) || defined(WATCHDOG_PIPE)
  if(watchdog)
  {
#ifdef WATCHDOG_PIPE
    destruct(watchdog_pipe);
#endif
    catch { watchdog->kill(signum("SIGKILL")); };
    watchdog->wait();
  }
#endif

  return errors;
}

constant doc = #"
Usage: test_pike [args] [testfiles]

--no-watchdog       Watchdog will not be used.
--watchdog=pid      Run only the watchdog and monitor the process with the given
                    pid.
-h, --help          Prints this message.
-v[level],
--verbose[=level]   Select the level of verbosity. Every verbose level includes
                    the printouts from the levels below.
                    0  No extra printouts.
                    1  Some additional information printed out after every
                       finished block of tests.
                    2  Some extra information about test that will or won't be
                       run.
                    3  Every test is printed out.
                    4  Time spent in individual tests are printed out.
                    10 The actual Pike code compiled, including wrappers, is
                       printed.
-p, --prompt        The user will be asked before every test is run.
-sX, --start-test=X Where in the testsuite testing should start, e.g. ignores X
                    tests in every testsuite.
-eX, --end-after=X  How many tests should be run.
-f, --fail          If set, the test program exits on first failure.
-F, --forking       If set, each testsuite will run in a separate process.
-lX, --loop=X       The number of times the testsuite should be run. Default is
                    1.
-tX, --trace=X      Run tests with trace level X.
-c[X], --check[=X]  The level of extra Pike consistency checks performed.
                    1   _verify_internals is run before every test.
                    2   _verify_internals is run after every compilation.
                    3   _verify_internals is run after every test.
                    4   An extra gc and _verify_internals is run before
                        every test.
                    X<0 For values below zero, _verify_internals will be run
                        before every n:th test, where n=abs(X).
-m, --mem, --memory Print out memory allocations after the tests.
-a, --auto[=dir]    Let the test program find the testsuits self.
-T, --notty         Format output for non-tty.
-d, --debug         Opens a debug port.
";
