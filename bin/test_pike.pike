#!/usr/local/bin/pike

#include <simulate.h>

#if !efun(_verify_internals)
#define _verify_internals()
#endif


int main(int argc, string *argv)
{
  int e, verbose, successes, errors, t, check;
  string *tests;
  program testprogram;
  int start, fail, mem;
  int loop=1;

  for(e=1;e<argc;e++)
  {
    string opt;
    int arg;
    arg=1;
    if(sscanf(argv[e],"--%s=%d",opt,arg)==2)
      opt="--"+opt;
    else if(sscanf(argv[e],"-%s%d",opt,arg)==2)
      opt="-"+opt;
    else
      opt=argv[e];
    
    switch(opt)
    {
      case "-h":
      case "--help":
        perror("Usage: "+argv[e]+" [-v | --verbose] [-h | --help] [-t <testno>] <testfile>\n");
        return 0;

      case "-v":
      case "--verbose":
        verbose+=arg;
        break;

      case "-s":
      case "--start-test":
	start=arg;
	start--;
	break;

      case "-f":
      case "--fail":
	fail+=arg;
	break;


      case "-l":
      case "--loop":
	loop+=arg;
	break;

      case "-t":
      case "--trace":
	t+=arg;
	break;

      case "-c":
      case "--check":
	check++;
	break;


      case "-m":
      case "--mem":
      case "--memory":
	mem++;
	break;

      default:
	if(tests)
	{
	  perror("Unknown argument: "+opt+".\n");
	  exit(1);
	}
	tests=(read_bytes(argv[e])||"")/"\n....\n";
    }
  }

  if(!tests)
  {
    tests=(clone((program)"/precompiled/file","stdin")->read(0x7fffffff)||"")/"\n....\n";
  }

  if(!tests)
  {
    perror("Failed to read test file!\n");
    exit(1);
  }

  tests=tests[0..sizeof(tests)-2];

  while(loop--)
  {
    successes=errors=0;

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
	if(!clone(compile_string("mixed c() { return "+condition+"; }","Cond "+(e+1)))->c())
	{
	  if(verbose)
	    perror("Not doing test "+(e+1)+"\n");
	  successes++;
	  continue;
	}
      }
      
      sscanf(test,"%s\n%s",type,test);
      sscanf(type,"%*s expected result: %s",type);
      
      if(verbose)
      {
	perror("Doing test "+(e+1)+"\n");
	if(verbose>1)
	  perror(test+"\n");
      }
      
      if(check > 1) _verify_internals();
      
      switch(type)
      {
      case "COMPILE_ERROR":
	master()->set_inhibit_compile_errors(1);
	if(catch(compile_string(test,"Test "+(e+1))))
	{
	  successes++;
	}else{
	  perror("Test "+(e+1)+" failed.\n");
	  perror(test+"\n");
	  errors++;
	}
	master()->set_inhibit_compile_errors(0);
	break;
	
      case "EVAL_ERROR":
	master()->set_inhibit_compile_errors(1);
	if(catch(clone(compile_string(test,"Test "+(e+1)))->a()))
	{
	  successes++;
	}else{
	  perror("Test "+(e+1)+" failed.\n");
	  perror(test+"\n");
	  errors++;
	}
	master()->set_inhibit_compile_errors(0);
	break;
	
      default:
	o=clone(compile_string(test,"Test "+(e+1)));
	
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
	    perror("Test "+(e+1)+" failed.\n");
	    perror(test+"\n");
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
	    perror("Test "+(e+1)+" failed.\n");
	    perror(test+"\n");
	    perror(sprintf("o->a(): %O\n",a));
	    perror(sprintf("o->b(): %O\n",b));
	    errors++;
	  }else{
	    successes++;
	  }
	  break;
	  
	case "EQUAL":
	  if(!equal(a,b))
	  {
	    perror("Test "+(e+1)+" failed.\n");
	    perror(test+"\n");
	    perror(sprintf("o->a(): %O\n",a));
	    perror(sprintf("o->b(): %O\n",b));
	    errors++;
	  }else{
	    successes++;
	  }
	  break;
	  
	default:
	  perror(sprintf("Unknown test type (%O).\n",type));
	  errors++;
	}
      }
      
      if(check > 2) _verify_internals();
      
      if(fail && errors)
	exit(1);
      
      a=b=0;
    }
    
    if(errors + successes != sizeof(tests))
    {
      perror("Errors + Successes != number of tests!\n");
      errors++;
    }
    
    if(errors || verbose)
    {
      perror("Failed tests: "+errors+".\n");
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

  return errors;
}
