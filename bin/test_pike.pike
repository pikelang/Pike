#!/usr/local/bin/pike

/* $Id: test_pike.pike,v 1.44 2000/03/31 22:29:43 hubbe Exp $ */

import Stdio;

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

int istty_cache;
int istty()
{
#ifdef __NT__
  return 1;
#else
  if(!istty_cache)
  {
    istty_cache=!!Stdio.stdin->tcgetattr();
    if(!istty_cache)
    {
      istty_cache=-1;
    }else{
      switch(getenv("TERM"))
      {
	case "dumb":
	case "emacs":
	  istty_cache=-1;
      }
    }
  }
  return istty_cache>0;
#endif
}

mapping(string:int) cond_cache=([]);

#if constant(thread_create)
#define HAVE_DEBUG
#endif

void bzot(string test)
{
  int line=1;
  int tmp=strlen(test)-1;
  while(test[tmp]=='\n') tmp--;
  foreach(test[..tmp]/"\n",string s)
    werror("%3d: %s\n",line++,s);
}

array find_testsuites(string dir)
{
  array(string) ret=({});
  if(array(string) s=get_dir(dir||"."))
  {
    foreach(s, string file)
      {
	string name=combine_path(dir||"",file);
	if(file_size(name)==-2)
	  ret+=find_testsuites(name);
      }
    
    foreach(s, string file)
      {
	switch(file)
	{
	  case "testsuite":
	  case "module_testsuite":
	    ret+=({ combine_path(dir||"",file) });
	}
      }
  }
  return ret;
}

// 20 minutes should be enough..
#define WATCHDOG_TIMEOUT 60*20

#if constant(thread_create)
#define WATCHDOG
#define WATCHDOG_PIPE
object watchdog_pipe;
#else
#if constant(signal) && constant(signum)
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
//	    werror("{WATCHDOG} Ping!\n");
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

int main(int argc, array(string) argv)
{
  int e, verbose, successes, errors, t, check;
  int skipped;
  array(string) tests;
  string tmp;
  program testprogram;
  int start, fail, mem;
  int loop=1;
  int end=0x7fffffff;
  string extra_info="";
  int shift;

#if constant(signal) && constant(signum)
  if(signum("SIGQUIT")>=0)
  {
    signal(signum("SIGQUIT"),lambda()
	   {
	     master()->handle_error( ({"\nSIGQUIT recived, printing backtrace and continuing.\n",backtrace() }) );
	   });
  }
#endif

  array(string) args=backtrace()[0][3];
  array(string) testsuites=({});
  args=args[..sizeof(args)-1-argc];
  add_constant("RUNPIKE",Array.map(args,Process.sh_quote)*" ");

  foreach(Getopt.find_all_options(argv,aggregate(
    ({"no-watchdog",Getopt.NO_ARG,({"--no-watchdog"})}),
    ({"watchdog",Getopt.HAS_ARG,({"--watchdog"})}),
    ({"help",Getopt.NO_ARG,({"-h","--help"})}),
    ({"verbose",Getopt.MAY_HAVE_ARG,({"-v","--verbose"})}),
    ({"start",Getopt.HAS_ARG,({"-s","--start-test"})}),
    ({"end",Getopt.HAS_ARG,({"--end-after"})}),
    ({"fail",Getopt.MAY_HAVE_ARG,({"-f","--fail"})}),
    ({"loop",Getopt.MAY_HAVE_ARG,({"-l","--loop"})}),
    ({"trace",Getopt.MAY_HAVE_ARG,({"-t","--trace"})}),
    ({"check",Getopt.MAY_HAVE_ARG,({"-c","--check"})}),
    ({"mem",Getopt.MAY_HAVE_ARG,({"-m","--mem","--memory"})}),
    ({"auto",Getopt.MAY_HAVE_ARG,({"-a","--auto"})}),
    ({"notty",Getopt.NO_ARG,({"-t","--notty"})}),
#ifdef HAVE_DEBUG
    ({"debug",Getopt.MAY_HAVE_ARG,({"-d","--debug"})}),
#endif
    )),array opt)
    {
      switch(opt[0])
      {
	case "no-watchdog":
	  use_watchdog=0;
	  break;

	case "watchdog":
#ifdef WATCHDOG
	  int cnt=0;
	  int pid=(int)opt[1];
	  int last_time=time();
	  int exit_quietly;
#ifdef WATCHDOG_PIPE
	  thread_create(lambda() {
	    object o=Stdio.File("stdin");
	    while(strlen(o->read(1) || ""))
	      {
//		werror("[WATCHDOG] Pong!\n");
                last_time=time();
              }
//	    werror("[WATCHDOG] exiting.\n");
	    exit_quietly=1;
	  });
#endif

#ifdef WATCHDOG_SIGNAL
	  werror("Setting signal (1)\n");
	  if(signum("SIGQUIT")>=0)
	  {
	    werror("Setting signal (2)\n");
	    signal(signum("SIGQUIT"),lambda() {
//	      werror("[WATCHDOG] Pong!\n");
	      last_time=time();
	    });
	  }else{
	    exit(1);
	  }
#endif	  
//	  werror("[WATCHDOG] started, watching %d.\n",pid);

	  while(1)
	  {
	    sleep(10);
	    if(exit_quietly) _exit(0);
#ifndef __NT__
	    if(!kill(pid, 0)) _exit(0);
#endif
//	    werror("[WATCHDOG] t=%d\n",time()-last_time);

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
#endif
	  break;
	  
	case "notty":
	  istty_cache=-1;
	  break;

	case "help":
	  werror("Usage: "+argv[e]+" [-v | --verbose] [-h | --help] [-t <testno>] <testfile>\n");
	  return 0;

	case "verbose": verbose+=foo(opt[1]); break;
	case "start": start=foo(opt[1]); start--; break;
	case "end": end=foo(opt[1]); break;
	case "fail": fail+=foo(opt[1]); break;
	case "loop": loop+=foo(opt[1]); break;
	case "trace": t+=foo(opt[1]); break;
	case "check": check+=foo(opt[1]); break;
	case "mem": mem+=foo(opt[1]); break;

	case "auto":
	  testsuites=find_testsuites(".");
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

#ifdef WATCHDOG
  int watchdog_time=time();

  if(use_watchdog)
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
#endif

  argv=Getopt.get_args(argv,1)+testsuites;
  if(sizeof(argv)<1)
  {
    if(!tmp)
    {
      werror("No tests?\n");
      exit(1);
    }
  }

  while(loop--)
  {
    successes=errors=0;
    for(int f=1;f<sizeof(argv);f++)
    {
      tmp=read_bytes(argv[f]);
      if(!tmp)
      {
	werror("Failed to read test file, errno="+errno()+".\n");
	exit(1);
      }
      
      tests=tmp/"\n....\n";
      tmp=0;
      tests=tests[0..sizeof(tests)-2];
      
      werror("Doing tests in %s (%d tests)\n",argv[f],sizeof(tests));
      
	for(e=start;e<sizeof(tests);e++)
	{
	  signal_watchdog();

	  int skip=0;
	  string test,condition;
	  string|int type;
	  object o;
	  mixed a,b;
	
	  if(check) _verify_internals();
	  if(check>3) {
	    gc();
	    _verify_internals();
	  }
	
	  test=tests[e];	
	  if(sscanf(test,"COND %s\n%s",condition,test)==2)
	  {
	    int tmp;
	    if(!(tmp=cond_cache[condition]))
	    {
	      tmp=!!(clone(compile_string("mixed c() { return "+condition+"; }","Cond "+(e+1)))->c());
	      if(!tmp) tmp=-1;
	      cond_cache[condition]=tmp;
	    }
	  
	    if(tmp==-1)
	    {
	      if(verbose)
		werror("Not doing test "+(e+1)+"\n");
	      successes++;
	      skipped++;
	      skip=1;
	    }
	  }


	  if(istty())
	  {
	    werror("%6d\r",e+1);
	  }else{
	    /* Use + instead of . so that sendmail and
	     * cron will not cut us off... :(
	     */
	    switch( (e-start) % 50)
	    {
	      case 0:
		werror("%5d: ",e);
		break;
		
	      case 9:
	      case 19:
	      case 29:
	      case 39:
		werror(skip?"- ":"+ ");
		break;
		
	      default:
		werror(skip?"-":"+");
		break;
		
	      case 49:
		werror(skip?"-\n":"+\n");
	    }
	  }
	  if(skip) continue;

	
	  sscanf(test,"%s\n%s",type,test);
	  sscanf(type,"%*s expected result: %s",type);
	
	  if(verbose)
	  {
	    werror("Doing test %d (%d total)%s\n",e+1,successes+errors+1,extra_info);
	    if(verbose>1) bzot(test);
	  }

	  if(check > 1) _verify_internals();
	
	  shift++;
	  string fname = argv[f] + ": Test " + (e + 1) +
	    " (shift " + (shift%3) + ")";

	  string widener = ([ 0:"",
			    1:"\nint \x30c6\x30b9\x30c8=0;\n",
			    2:"\nint \x10001=0;\n" ])[shift%3];

	  // widener += "#pragma strict_types\n";

	  if(test[-1]!='\n') test+="\n";

	  int computed_line=sizeof(test/"\n");
	  array gnapp= test/"#";
	  for(int e=0;e<sizeof(gnapp);e++)
	  {
	    if(sscanf(gnapp[e],"%*d"))
	    {
	      computed_line=0;
	      break;
	    }
	  }
	  string linetester="int __cpp_line=__LINE__; int __rtl_line=[int]backtrace()[-1][1];\n";

	  string to_compile = test + linetester + widener;

	  if((shift/3)&1)
	  {
	    fname+=" (CRNL)";
	    to_compile=replace(to_compile,"\n","\r\n");
	  }
	   
	  if(verbose>9) bzot(to_compile);
	  switch(type)
	  {
	    case "COMPILE":
	      _dmalloc_set_name(fname,0);
	      if(catch(compile_string(to_compile, fname)))
	      {
		_dmalloc_set_name();
		werror(fname + " failed.\n");
		bzot(test);
		errors++;
	      }else{
		_dmalloc_set_name();
		successes++;
	      }
	      break;
	    
	    case "COMPILE_ERROR":
	      master()->set_inhibit_compile_errors(1);
	      _dmalloc_set_name(fname,0);
	      if(catch(compile_string(to_compile, fname)))
	      {
		_dmalloc_set_name();
		successes++;
	      }else{
		_dmalloc_set_name();
		werror(fname + " failed.\n");
		bzot(test);
		errors++;
	      }
	      master()->set_inhibit_compile_errors(0);
	      break;
	    
	    case "EVAL_ERROR":
	      master()->set_inhibit_compile_errors(1);
	      _dmalloc_set_name(fname,0);
	      if(catch(clone(compile_string(to_compile, fname))->a()))
	      {
		_dmalloc_set_name();
		successes++;
	      }else{
		_dmalloc_set_name();
		werror(fname + " failed.\n");
		bzot(test);
		errors++;
	      }
	      master()->set_inhibit_compile_errors(0);
	      break;
	    
	    default:
	      mixed err;
	      if (err = catch{
		_dmalloc_set_name(fname,0);
		o=clone(compile_string(to_compile,fname));
		_dmalloc_set_name();
	    
		if(check > 1) _verify_internals();
	    
		a=b=0;
		if(t) trace(t);
		_dmalloc_set_name(fname,1);
		if(functionp(o->a)) a=o->a();
		if(functionp(o->b)) b=o->b();
		_dmalloc_set_name();

		if(t) trace(0);
		if(check > 1) _verify_internals();

	      }) {
		werror(fname + " failed.\n");
		bzot(test);
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
		werror(fname + " Line numbering failed.\n");
		bzot(test + linetester);
		werror("   CPP lines: %d\n",o->__cpp_line);
		werror("   RTL lines: %d\n",o->__rtl_line);
		if(computed_line)
		  werror("Actual lines: %d\n",computed_line);
		errors++;
	      }
	    
	      switch(type)
	      {
		case "FALSE":
		  if(a)
		  {
		    werror(fname + " failed.\n");
		    bzot(test);
		    werror(sprintf("o->a(): %O\n",a));
		    errors++;
		  }else{
		    successes++;
		  }
		  break;
		
		case "TRUE":
		  if(!a)
		  {
		    werror(fname + " failed.\n");
		    bzot(test);
		    werror(sprintf("o->a(): %O\n",a));
		    errors++;
		  }else{
		    successes++;
		  }
		  break;
		
		case "RUN":
		  successes++;
		  break;
		
		case "EQ":
		  if(a!=b)
		  {
		    werror(fname + " failed.\n");
		    bzot(test);
		    werror(sprintf("o->a(): %O\n",a));
		    werror(sprintf("o->b(): %O\n",b));
		    errors++;
		  }else{
		    successes++;
		  }
		  break;
		
		case "EQUAL":
		  if(!equal(a,b))
		  {
		    werror(fname + " failed.\n");
		    bzot(test);
		    werror(sprintf("o->a(): %O\n",a));
		    werror(sprintf("o->b(): %O\n",b));
		    errors++;
		  }else{
		    successes++;
		  }
		  break;
		
		default:
		  werror(sprintf("%s: Unknown test type (%O).\n", fname, type));
		  errors++;
	      }
	  }
	
	  if(check > 2) _verify_internals();
	
	  if(fail && errors)
	    exit(1);

	  if(!--end) exit(0);
	
	  a=b=0;
      }

	if(istty())
	{
	  werror("             \r");
	}else{
	  werror("\n");
	}
    }
    if(mem)
    {
      int total;
      tests=0;
      gc();
      mapping tmp=_memory_usage();
      write(sprintf("%-10s: %6s %10s\n","Category","num","bytes"));
      foreach(sort(indices(tmp)),string foo)
	{
	  if(sscanf(foo,"%s_bytes",foo))
	  {
	    write(sprintf("%-10s: %6d %10d\n",
			  foo+"s",
			  tmp["num_"+foo+"s"],
			  tmp[foo+"_bytes"]));
	    total+=tmp[foo+"_bytes"];
	  }
	}
      write(sprintf("%-10s: %6s %10d\n",
		    "Total",
		    "",
		    total));
    }
  }
  if(errors || verbose)
  {
    werror("Failed tests: "+errors+".\n");
  }
      
  werror("Total tests: %d  (%d tests skipped)\n",successes+errors,skipped);

#ifdef WATCHDOG_SIGNAL
  if(use_watchdog)
  {
    watchdog->kill(signum("SIGKILL"));
    watchdog->wait();
  }
#endif

#ifdef WATCHDOG_PIPE
  if(use_watchdog)
  {
    destruct(watchdog_pipe);
#if constant(signum)
    catch { watchdog->kill(signum("SIGKILL")); };
#endif
    watchdog->wait();
  }
#endif

  return errors;
}
