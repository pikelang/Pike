/* $Id: master.pike,v 1.47 1997/07/19 21:33:47 hubbe Exp $
 *
 * Master-file for Pike.
 */

#define UNDEFINED (([])[0])
#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

string describe_backtrace(mixed *trace);
object low_cast_to_object(string oname, string current_file);

string pike_library_path;
string *pike_include_path=({});
string *pike_module_path=({});
string *pike_program_path=({});

string combine_path_with_cwd(string path)
{
  return combine_path(path[0]=='/'?"/":getcwd(),path);
}

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


void add_include_path(string tmp)
{
  tmp=combine_path_with_cwd(tmp);
  pike_include_path-=({tmp});
  pike_include_path=({tmp})+pike_include_path;
}

void remove_include_path(string tmp)
{
  tmp=combine_path_with_cwd(tmp);
  pike_include_path-=({tmp});
}

void add_module_path(string tmp)
{
  tmp=combine_path_with_cwd(tmp);
  pike_module_path-=({tmp});
  pike_module_path=({tmp})+pike_module_path;
}


void remove_module_path(string tmp)
{
  tmp=combine_path_with_cwd(tmp);
  pike_module_path-=({tmp});
}


void add_program_path(string tmp)
{
  tmp=combine_path_with_cwd(tmp);
  pike_program_path-=({tmp});
  pike_program_path=({tmp})+pike_module_path;
}


void remove_program_path(string tmp)
{
  tmp=combine_path_with_cwd(tmp);
  pike_program_path-=({tmp});
}


mapping (string:program) programs=(["/master":object_program(this_object())]);

#define capitalize(X) (upper_case((X)[..0])+(X)[1..])

static program low_findprog(string pname, string ext)
{
  program ret;
  string fname=pname+ext;
  if(ret=programs[fname]) return ret;
  if(file_stat(fname))
  {
    switch(ext)
    {
    case "":
    case ".pike":
      if ( mixed e=catch { ret=compile_file(fname); } )
      {
	if(arrayp(e) &&
	   sizeof(e)==2 &&
	   arrayp(e[1]) &&
	   sizeof(e[1]) == sizeof(backtrace()))
	  e[1]=({});
	throw(e);
      }
      break;
#if constant(load_module)
    case ".so":
      ret=load_module(fname);
#endif /* load_module */
    }
    return programs[fname]=ret;
  }else{
    return UNDEFINED;
  }
}

static program findprog(string pname, string ext)
{
  switch(ext)
  {
  case ".pike":
  case ".so":
    return low_findprog(pname,ext);

  default:
    pname+=ext;
    return
      low_findprog(pname,"") ||
      low_findprog(pname,".pike") ||
      low_findprog(pname,".so");
  }
}

/* This function is called when the driver wants to cast a string
 * to a program, this might be because of an explicit cast, an inherit
 * or a implict cast. In the future it might receive more arguments,
 * to aid the master finding the right program.
 */
program cast_to_program(string pname, string current_file)
{
  string ext;
  string nname;
  if(sscanf(reverse(pname),"%s.%s",ext, nname) && search(ext, "/") == -1)
  {
    ext="."+reverse(ext);
    pname=reverse(nname);
  }else{
    ext="";
  }
  if(pname[0]=='/')
  {
    pname=combine_path("/",pname);
    return findprog(pname,ext);
  }else{
    string cwd;
    if(current_file)
    {
      string *tmp=current_file/"/";
      cwd=tmp[..sizeof(tmp)-2]*"/";
    }else{
      cwd=getcwd();
    }

    if(program ret=findprog(combine_path(cwd,pname),ext))
      return ret;

    foreach(pike_program_path, string path)
      if(program ret=findprog(combine_path(path,pname),ext))
	return ret;

    return 0;
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
  if(stringp(prog))
    prog=cast_to_program(prog,backtrace()[-2][0]);
  return prog(@args);
}

/* Note that create is called before add_precompiled_program
 */
void create()
{
  /* make ourselves known */
  add_constant("add_include_path",add_include_path);
  add_constant("remove_include_path",remove_include_path);
  add_constant("add_module_path",add_module_path);
  add_constant("remove_module_path",remove_module_path);
  add_constant("add_program_path",add_program_path);
  add_constant("remove_program_path",remove_program_path);
  add_constant("master",lambda() { return this_object(); });
  add_constant("describe_backtrace",describe_backtrace);
  add_constant("mkmultiset",lambda(mixed *a) { return aggregate_multiset(@a); });
  add_constant("strlen",sizeof);
  add_constant("new",new);
  add_constant("clone",new);
  add_constant("UNDEFINED",UNDEFINED);

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
  return cast_to_program(pname, current_file);
}

mapping (program:object) objects=([object_program(this_object()):this_object()]);

object low_cast_to_object(string oname, string current_file)
{
  program p;
  object o;

  p=cast_to_program(oname, current_file);
  if(!p) return 0;
  if(!(o=objects[p])) o=objects[p]=p();
  return o;
}

/* This function is called when the drivers wants to cast a string
 * to an object because of an implict or explicit cast. This function
 * may also receive more arguments in the future.
 */
object cast_to_object(string oname, string current_file)
{
  if(object o=low_cast_to_object(oname, current_file))
    return o;
  error("Cast to object failed\n");
}

class dirnode
{
  string dirname;
  mapping cache=([]);
  void create(string name) { dirname=name; }
  object|program ind(string index)
  {
    object m=((object)"/master");
    if(mixed o=m->findmodule(dirname+"/module"))
    {
      if(mixed tmp=o->_module_value) o=tmp;
      if(o=o[index]) return o;
    }
    index = dirname+"/"+index;
    if(object o=((object)"/master")->findmodule(index))
    {
      if(mixed tmp=o->_module_value) o=tmp;
      return o;
    }
    return (program) index;
  }

  object|program `[](string index)
  {
    mixed ret;
    if(!zero_type(ret=cache[index]))
    {
      if(ret) return ret;
      return UNDEFINED;
    }
    return cache[index]=ind(index);
  }
};

static mapping(string:mixed) fc=([]);

object findmodule(string fullname)
{
  mixed *stat;
  object o;
  if(!zero_type(o=fc[fullname]))
  {
    return o;
  }

  if(mixed *stat=file_stat(fullname+".pmod"))
  {
    if(stat[1]==-2)
      return fc[fullname]=dirnode(fullname+".pmod");
  }

  if(o=low_cast_to_object(fullname+".pmod","/."))
    return fc[fullname]=o;
    
#if constant(load_module)
  if(file_stat(fullname+".so"))
    return fc[fullname]=low_cast_to_object(fullname,"/.");
#endif

  return fc[fullname]=UNDEFINED;
}

varargs mixed resolv(string identifier, string current_file)
{
  mixed ret;
  string *tmp,path;

  if(current_file)
  {
    tmp=current_file/"/";
    tmp[-1]=identifier;
    path=combine_path_with_cwd( tmp*"/");
    ret=findmodule(path);
  }

  if(!ret)
  {
    foreach(pike_module_path, path)
      {
	string file=combine_path(path,identifier);
	if(ret=findmodule(file)) break;
      }
    
    if(!ret)
    {
      string path=combine_path(pike_library_path+"/modules",identifier);
      ret=findmodule(path);
    }
  }


  if(ret)
  {
    if(mixed tmp=ret->_module_value) ret=tmp;
    return ret;
  }
  return UNDEFINED;
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
  object tmp;
  string a,b;
  mixed *q;

  foreach(env,a) if(sscanf(a,"%s=%s",a,b)) environment[a]=b;
  add_constant("getenv",getenv);
  add_constant("putenv",putenv);

  add_constant("write",_static_modules.files()->file("stdout")->write);

  a=backtrace()[-1][0];
  q=a/"/";
  pike_library_path = q[0..sizeof(q)-2] * "/";

  add_include_path(pike_library_path+"/include");
  add_module_path(pike_library_path+"/modules");
  add_program_path(getcwd());
  add_module_path(getcwd());

  q=(getenv("PIKE_INCLUDE_PATH")||"")/":"-({""});
  for(i=sizeof(q)-1;i>=0;i--) add_include_path(q[i]);

  q=(getenv("PIKE_PROGRAM_PATH")||"")/":"-({""});
  for(i=sizeof(q)-1;i>=0;i--) add_program_path(q[i]);

  q=(getenv("PIKE_MODULE_PATH")||"")/":"-({""});
  for(i=sizeof(q)-1;i>=0;i--) add_module_path(q[i]);

  
  if(sizeof(argv)>1 && sizeof(argv[1]) && argv[1][0]=='-')
  {
    tmp=resolv("Getopt");
    
    q=tmp->find_all_options(argv,({
      ({"version",tmp->NO_ARG,({"-v","--version"})}),
	({"help",tmp->NO_ARG,({"-h","--help"})}),
	  ({"execute",tmp->HAS_ARG,({"-e","--execute"})}),
	    ({"modpath",tmp->HAS_ARG,({"-M","--module-path"})}),
	      ({"ipath",tmp->HAS_ARG,({"-I","--include-path"})}),
		({"ppath",tmp->HAS_ARG,({"-P","--program-path"})}),
		  ({"ignore",tmp->HAS_ARG,"-ms"}),
		    ({"ignore",tmp->MAY_HAVE_ARG,"-Ddatpl",0,1})}),1);
    
    /* Parse -M and -I backwards */
    for(i=sizeof(q)-1;i>=0;i--)
    {
      switch(q[i][0])
      {
      case "modpath":
	add_module_path(q[i][1]);
	break;
	
      case "ipath":
	add_include_path(q[i][1]);
	break;
	
      case "ppath":
	add_program_path(q[i][1]);
	break;
      }
    }
    
    foreach(q, mixed *opts)
    {
      switch(opts[0])
      {
      case "version":
	werror(version() + " Copyright (C) 1994-1997 Fredrik Hübinette\n"
	       "Pike comes with ABSOLUTELY NO WARRANTY; This is free software and you are\n"
	       "welcome to redistribute it under certain conditions; Read the files\n"
	       "COPYING and DISCLAIMER in the Pike distribution for more details.\n");
	exit(0);
	
      case "help":
	werror("Usage: pike [-driver options] script [script arguments]\n"
	       "Driver options include:\n"
	       " -I --include-path=<p>: Add <p> to the include path\n"
	       " -M --module-path=<p> : Add <p> to the module path\n"
	       " -P --program-path=<p>: Add <p> to the program path\n"
	       " -e --execute=<cmd>   : Run the given command instead of a script.\n"
	       " -h --help            : see this message\n"
	       " -v --version         : See what version of pike you have.\n"
	       " -s#                  : Set stack size\n"
	       " -m <file>            : Use <file> as master object.\n"
	       " -d -d#               : Increase debug (# is how much)\n"
	       " -t -t#               : Increase trace level\n"
	  );
	exit(0);

      case "execute":
	compile_string("#include <simulate.h>\nmixed create(){"+opts[1]+";}")();
	exit(0);
      }
    }

    argv=tmp->get_args(argv,1);
  }

  if(sizeof(argv)==1)
  {
    argv=argv[0]/"/";
    argv[-1]="hilfe";
    argv=({ argv*"/" });
    if(!file_stat(argv[0]))
    {
      if(file_stat("/usr/local/bin/hilfe"))
	argv[0]="/usr/local/bin/hilfe";
      else if(file_stat("../bin/hilfe"))
	argv[0]="/usr/local/bin/hilfe";
      else
      {
	werror("Couldn't find hilfe.\n");
	exit(1);
      }
    }
  }else{
    argv=argv[1..];
  }

  program tmp=(program)argv[0];

  if(!tmp)
  {
    werror("Pike: Couldn't find script to execute.\n");
    exit(1);
  }

  object script=tmp();

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
    path=combine_path_with_cwd(tmp*"/");
    if(!file_stat(path)) return 0;
  }
  else
  {
    foreach(pike_include_path, path)
      {
	path=combine_path(path,f);
	if(file_stat(path))
	  break;
	else
	  path=0;
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
      cast_to_object(path[0..sizeof(path)-2]+"pre.pike","/");
    }
  }

  return path;
}

// FIXME
string stupid_describe(mixed m)
{
  switch(string typ=sprintf("%t",m))
  {
  case "int":
  case "float":
    return (string)m;

  case "string":
    if(sizeof(m) < 60 && sscanf(m,"%*[-a-zAZ0-9.~`!@#$%^&*()_]%n",int i) && i==sizeof(m))
    {
      return "\""+m+"\"";
    }

  case "array":
  case "mapping":
  case "multiset":
    return typ+"["+sizeof(m)+"]";

  default:
    return sprintf("%t",m);
  }
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
	  row=function_name(tmp[2])+"(";
	  for(int e=3;e<sizeof(tmp);e++)
	  {
	    row+=stupid_describe(tmp[e])+",";
	  }

	  if(sizeof(tmp)>3)
	    row=row[..sizeof(row)-2];
	  row+=") in ";
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
