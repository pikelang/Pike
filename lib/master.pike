string describe_backtrace(mixed *trace);

string pike_library_path;

mapping (string:string) environment=([]);

varargs mixed getenv(string s)
{
  if(!s) return environment;
  return environment[s];
}

void putenv(string var, string val)
{
  environment[var]=val;
}

mapping (string:program) programs=([]);

#define capitalize(X) (upper_case((X)[..0])+(X)[1..])

/* This function is called whenever a module has built a clonable program
 * with functions written in C and wants to notify the Pike part about
 * this. It also supplies a suggested name for the program.
 */
void add_precompiled_program(string name, program p)
{
  programs[name]=p;

  if(sscanf(name, "/precompiled/%s", name)) {
    string const="";
    name /= "/";
    foreach(reverse(name), string s) {
      const = capitalize(s) + const;
      add_constant(const, p);
    }
  }
}

/* This function is called when the driver wants to cast a string
 * to a program, this might be because of an explicit cast, an inherit
 * or a implict cast. In the future it might receive more arguments,
 * to aid the master finding the right program.
 */
program cast_to_program(string pname)
{
  if(pname[sizeof(pname)-3..sizeof(pname)]==".pike")
    pname=pname[0..sizeof(pname)-4];

  function findprog=lambda(string pname)
  {
    program ret;

    if(ret=programs[pname]) return ret;
  
    if(file_stat(pname))
    {
      ret=compile_file(pname);
    }
    else if(file_stat(pname+".pike"))
    {
      ret=compile_file(pname+".pike");
    }
#if efun(ldopen)
    else if(file_stat(pname+".so"))
    {
      ldopen(pname);
      ret=programs[pname];
    }
#endif
    if(ret) programs[pname]=ret;
    return ret;
  };

  if(pname[0]=='/')
  {
    return findprog(pname);
  }else{
    if(search(pname,"/")==-1)
    {
      string path;
      if(string path=getenv("PIKE_INCLUDE_PATH"))
      {
	foreach(path/":", path)
	  if(program ret=findprog(combine_path(getcwd(),
					       combine_path(path,pname))))
	     return ret;
      }
    }
    return findprog(combine_path(getcwd(),pname));
  }
}

/* This function is called when an error occurs that is not caught
 * with catch(). It's argument consists of:
 * ({ error_string, backtrace }) where backtrace is the output from the
 * backtrace() efun.
 */
void handle_error(mixed *trace)
{
  predef::trace(0);
  werror(describe_backtrace(trace));
}



object new(mixed prog, mixed ... args)
{
  return ((program)prog)(@args);
}

/* Note that create is called before add_precompiled_program
 */
void create()
{
  /* make ourselves known */
  add_constant("master",lambda() { return this_object(); });
  add_constant("describe_backtrace",describe_backtrace);
  add_constant("version",lambda() { return "Pike v0.4"; });
  add_constant("mkmultiset",lambda(mixed *a) { return aggregate_multiset(@a); });
  add_constant("strlen",sizeof);
  add_constant("new",new);
  add_constant("clone",new);

  random_seed(time() + (getpid() * 0x11111111));
}

/*
 * This function is called whenever a inherit is called for.
 * It is supposed to return the program to inherit.
 * The first argument is the argument given to inherit, and the second
 * is the file name of the program currently compiling. Note that the
 * file name can be changed with #line, or set by compile_string, so
 * it can not be 100% trusted to be a filename.
 * previous_object(), can be virtually anything in this function, as it
 * is called from the compiler.
 */
program handle_inherit(string pname, string current_file)
{
  program p;
  string *tmp;
  p=cast_to_program(pname);
  if(p) return p;
  tmp=current_file/"/";
  tmp[-1]=pname;
  return cast_to_program(tmp*"/");
}

mapping (string:object) objects=(["/master.pike":this_object()]);

/* This function is called when the drivers wants to cast a string
 * to an object because of an implict or explicit cast. This function
 * may also receive more arguments in the future.
 */
object cast_to_object(string oname)
{
  object ret;

  if(oname[0]!='/')
    oname=combine_path(getcwd(),oname);

  if(oname[sizeof(oname)-3..sizeof(oname)]==".pike")
    oname=oname[0..sizeof(oname)-4];

  if(ret=objects[oname]) return ret;

  return objects[oname]=cast_to_program(oname)();
}


/* This function is called when all the driver is done with all setup
 * of modules, efuns, tables etc. etc. and is ready to start executing
 * _real_ programs. It receives the arguments not meant for the driver
 * and an array containing the environment variables on the same form as
 * a C program receives them.
 */
void _main(string *argv, string *env)
{
  int i;
  object script;
  string a,b;
  string *q;

  foreach(env,a) if(sscanf(a,"%s=%s",a,b)) environment[a]=b;
  add_constant("getenv",getenv);
  add_constant("putenv",putenv);

  add_constant("write",cast_to_program("/precompiled/file")("stdout")->write);

  a=backtrace()[-1][0];
  q=a/"/";
  pike_library_path = q[0..sizeof(q)-2] * "/";

//  clone(compile_file(pike_library_path+"/simulate.pike"));

  if(!sizeof(argv))
  {
    werror("Usage: pike [-driver options] script [script arguments]\n");
    exit(1);
  }
  script=(object)argv[0];

  if(!script->main)
  {
    werror("Error: "+argv[0]+" has no main().\n");
    exit(1);
  }

  i=script->main(sizeof(argv),argv,env);
  if(i >=0) exit(i);
}

mixed inhibit_compile_errors;

void set_inhibit_compile_errors(mixed f)
{
  inhibit_compile_errors=f;
}

string trim_file_name(string s)
{
  if(getenv("SHORT_PIKE_ERRORS"))
    return (s/"/")[-1];
  return s;
}

/*
 * This function is called whenever a compiling error occurs,
 * Nothing strange about it.
 * Note that previous_object cannot be trusted in ths function, because
 * the compiler calls this function.
 */
void compile_error(string file,int line,string err)
{
  if(!inhibit_compile_errors)
  {
    werror(sprintf("%s:%d:%s\n",trim_file_name(file),line,err));
  }
  else if(functionp(inhibit_compile_errors))
  {
    inhibit_compile_errors(file,line,err);
  }
}

/* This function is called whenever an #include directive is encountered
 * it receives the argument for #include and should return the file name
 * of the file to include
 * Note that previous_object cannot be trusted in ths function, because
 * the compiler calls this function.
 */
string handle_include(string f,
		      string current_file,
		      int local_include)
{
  string *tmp, path;

  if(local_include)
  {
    tmp=current_file/"/";
    tmp[-1]=f;
    path=combine_path(getcwd(),tmp*"/");
    if(!file_stat(path)) return 0;
  }
  else
  {
    if(path=getenv("PIKE_INCLUDE_PATH"))
    {
      foreach(path/":", path)
      {
	path=combine_path(path,f);
	if(file_stat(path))
	  break;
	else
	  path=0;
      }
    }
    
    if(!path)
    {
      path=combine_path(pike_library_path+"/include",f);
      if(!file_stat(path)) path=0;
    }
  }

  if(path)
  {
    /* Handle preload */

    if(path[-1]=='h' && path[-2]=='.' &&
       file_stat(path[0..sizeof(path)-2]+"pre.pike"))
    {
      cast_to_object(path[0..sizeof(path)-2]+"pre.pike");
    }
  }

  return path;
}

/* It is possible that this should be a real efun,
 * it is currently used by handle_error to convert a backtrace to a
 * readable message.
 */
string describe_backtrace(mixed *trace)
{
  int e;
  string ret;

  if(arrayp(trace) && sizeof(trace)==2 && stringp(trace[0]))
  {
    ret=trace[0];
    trace=trace[1];
  }else{
    ret="";
  }

  if(!arrayp(trace))
  {
    ret+="No backtrace.\n";
  }else{
    for(e=sizeof(trace)-1;e>=0;e--)
    {
      mixed tmp;
      string row;

      tmp=trace[e];
      if(stringp(tmp))
      {
	row=tmp;
      }
      else if(arrayp(tmp))
      {
	row="";
	if(sizeof(tmp)>=3 && functionp(tmp[2]))
	{
	  row=function_name(tmp[2])+" in ";
	}

	if(sizeof(tmp)>=2 && stringp(tmp[0]) && intp(tmp[1]))
	{
	  row+="line "+tmp[1]+" in "+trim_file_name(tmp[0]);
	}else{
	  row+="Unknown program";
	}
      }
      else
      {
	row="Destructed object";
      }
      ret+=row+"\n";
    }
  }

  return ret;
}
