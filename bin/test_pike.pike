#!/usr/local/bin/pike

/* $Id: test_pike.pike,v 1.15 1999/01/21 09:11:42 hubbe Exp $ */

#include <simulate.h>

#if !efun(_verify_internals)
#define _verify_internals()
#endif

int foo(string opt)
{
  if(opt=="" || !opt) return 1;
  return (int)opt;
}

mapping(string:int) cond_cache=([]);

#if constant(thread_create)
#define HAVE_DEBUG
#endif

int main(int argc, string *argv)
{
  int e, verbose, successes, errors, t, check;
  string *tests,tmp;
  program testprogram;
  int start, fail, mem;
  int loop=1;
  int end=0x7fffffff;
  string extra_info="";


#if constant(signal) && constant(signum)
  if(signum("SIGQUIT")>=0)
  {
    signal(signum("SIGQUIT"),lambda()
	   {
	     master()->handle_error( ({"\nSIGQUIT recived, printing backtrace and continuing.\n",backtrace() }) );
	   });
  }
#endif

  string *args=backtrace()[0][3];
  args=args[..sizeof(args)-1-argc];
  add_constant("RUNPIKE",Array.map(args,Process.sh_quote)*" ");

  foreach(Getopt.find_all_options(argv,aggregate(
    ({"help",Getopt.NO_ARG,({"-h","--help"})}),
    ({"verbose",Getopt.NO_ARG,({"-v","--verbose"})}),
    ({"start",Getopt.HAS_ARG,({"-s","--start-test"})}),
    ({"end",Getopt.HAS_ARG,({"--end-after"})}),
    ({"fail",Getopt.MAY_HAVE_ARG,({"-f","--fail"})}),
    ({"loop",Getopt.MAY_HAVE_ARG,({"-l","--loop"})}),
    ({"trace",Getopt.MAY_HAVE_ARG,({"-t","--trace"})}),
    ({"check",Getopt.MAY_HAVE_ARG,({"-c","--check"})}),
    ({"mem",Getopt.MAY_HAVE_ARG,({"-m","--mem","--memory"})}),
#ifdef HAVE_DEBUG
    ({"debug",Getopt.MAY_HAVE_ARG,({"-d","--debug"})}),
#endif
    )),array opt)
    {
      switch(opt[0])
      {
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

  argv=Getopt.get_args(argv,1);
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
	string test,condition;
	int type;
	object o;
	mixed a,b;
	
	if(check) _verify_internals();
	
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
	    continue;
	  }
	}
	
	sscanf(test,"%s\n%s",type,test);
	sscanf(type,"%*s expected result: %s",type);
	
	if(verbose)
	{
	  werror("Doing test %d (%d total)%s\n",e+1,successes+errors+1,extra_info);
	  if(verbose>1)
	    werror(test+"\n");
	}
	
	if(check > 1) _verify_internals();
	
	string fname = argv[f] + ": Test " + (e + 1);

	switch(type)
	{
	  case "COMPILE":
	    if(catch(compile_string(test, fname)))
	    {
	      werror(fname + " failed.\n");
	      werror(test+"\n");
	      errors++;
	    }else{
	      successes++;
	    }
	    break;
	    
	  case "COMPILE_ERROR":
	    master()->set_inhibit_compile_errors(1);
	    if(catch(compile_string(test, fname)))
	    {
	      successes++;
	    }else{
	      werror(fname + " failed.\n");
	      werror(test+"\n");
	      errors++;
	    }
	    master()->set_inhibit_compile_errors(0);
	    break;
	    
	  case "EVAL_ERROR":
	    master()->set_inhibit_compile_errors(1);
	    if(catch(clone(compile_string(test, fname))->a()))
	    {
	      successes++;
	    }else{
	      werror(fname + " failed.\n");
	      werror(test+"\n");
	      errors++;
	    }
	    master()->set_inhibit_compile_errors(0);
	    break;
	    
	  default:
	    o=clone(compile_string(test,fname));
	    
	    if(check > 1) _verify_internals();
	    
	    a=b=0;
	    if(t) trace(t);
	    if(functionp(o->a)) a=o->a();
	    if(functionp(o->b)) b=o->b();
	    if(t) trace(0);
	    
	    if(check > 1) _verify_internals();
	    
	    switch(type)
	    {
	      case "FALSE":
		a=!a;
		
	      case "TRUE":
		if(!a)
		{
		  werror(fname + " failed.\n");
		  werror(test+"\n");
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
		  werror(test+"\n");
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
		  werror(test+"\n");
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
      
  werror("Total tests: %d\n",successes+errors);

  return errors;
}
