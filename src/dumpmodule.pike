#!/usr/local/bin/pike

int quiet=0;
program p; /* program being dumped */

#ifdef PIKE_FAKEROOT
string fakeroot(string s)
{
  return PIKE_FAKEROOT+combine_path(getcwd(),s);
}
#else
#define fakeroot(X) X
#endif

Tools.Install.ProgressBar progress_bar;

mapping function_names=([]);

static string fixup_path(string x)
{
  if(master()->relocate_module) {
    foreach(master()->pike_module_path, string path) {
      path = combine_path(path, "");
      if(x[..sizeof(path)-1] == path)
	return "/${PIKE_MODULE_PATH}/"+x[sizeof(path)..];
    }
    /* This is necessary to find compat modules... */
    foreach(master()->pike_module_path, string path) {
      path = combine_path(path, "..", "");
      if(x[..sizeof(path)-1] == path)
	return "/${PIKE_MODULE_PATH}/../"+x[sizeof(path)..];
    }
  }
  return x;
}

/* FIXME: this is a bit ad-hoc */
string mkmodulename(mixed x, string dirname)
{
  if(!sscanf(dirname,"%*slib/modules/%s",dirname))
    return 0;
  dirname-=".pmod";
  dirname=replace(dirname,"/",".");
  if(has_suffix(dirname, ".module"))
     sscanf(dirname, "%s.module", dirname);
  if(master()->resolv(dirname) == x)
    return dirname;
  return 0;
}

class Codec
{
  string nameof(mixed x)
  {
    string tmp;
    if(p!=x)
      if(tmp = function_names[x])
	return tmp;

    switch(sprintf("%t",x))
    {
#if 0
      case "function":
	if (!function_object(x) &&
	    (tmp = search(all_constants(), x))) {
	  function_names[tmp = "efun:"+tmp] = x;
	  return tmp;
	}
	break;
#endif
      case "program":
	if(p!=x)
	{
	  if(tmp = search(master()->programs,x))
	  {
	    if(has_suffix(tmp, ".pike"))
	    {
	      if(string mod=mkmodulename(x, tmp))
		return "resolv:"+mod;
	    }
	    return fixup_path(tmp);
	  }
	}
	break;

      case "object":
	if(program p=search(master()->objects,x))
	{
	  if(tmp = search(master()->programs,p))
	  {
	    if(has_suffix(tmp, ".pmod"))
	    {
	      if(string mod=mkmodulename(x, tmp))
		return "resolv:"+mod;
	    }
	    return fixup_path(tmp);
	  }
	}

	/*
	if(object_program(x) == master()->dirnode)
	{
	  if(string mod=mkmodulename(x, x->dirname))
	    return mod;
	}
	*/
	break;
    }
    return ([])[0];
  } 

  mixed encode_object(object x)
  {
    if(x->_encode) return x->_encode();
    error("Cannot encode objects yet (%O,%O).\n", x, indices(x)[..10]*",");
  }
}

Stdio.File logfile;

class Handler
{
  void compile_error(string file,int line,string err)
    {
      if(logfile)
	logfile->write("%s:%d:%s\n",file,line,err);
    }

  void compile_warning(string file,int line,string err)
    {
      if(logfile)
	logfile->write("%s:%d:%s\n",file,line,err);
    }
}

program compile_file(string file, object|void handler)
{
  if(master()->relocate_module) {
    string s = master()->master_read_file(file);
    return master()->compile_string(s,fixup_path(file), handler);
  } else
    return master()->compile_file(file, handler);
}

void dumpit(string file)
{
  if(logfile)
    logfile->write("\n##%s##\n",file);

  mixed err=catch {
    rm(file+".o"); // Make sure no old files are left
    if(mixed s=file_stat(fakeroot(file)))
    {
      if(s[1]<=0)
      {
	if(logfile) logfile->write("is a directory or special file (not dumped).\n");
	break;
      }
    }else{
      if(!quiet && logfile)
	logfile->write("does not exist (not dumped).\n");
      break;
    }
    if(programp(p=compile_file(file, Handler())))
    {
      if(!p->dont_dump_program)
      {
	string s=encode_value(p, Codec());
	p=decode_value(s,master()->Codec());
	if(programp(p))
	{
	  Stdio.File(fakeroot(file) + ".o","wct")->write(s);
	  if(!quiet && logfile) logfile->write("dumped.\n");
	}
	else if(!quiet && logfile)
	  logfile->write("Decode of %O failed (not dumped).\n", file);
      }
      else if(!quiet && logfile)
	logfile->write("Not dumping %O (not dumped).\n", file);
    }
    else if(!quiet && logfile)
      logfile->write("Compilation of %O failed (not dumped).\n", file); // This should never happen.
  };

  if(err)
  {
    if(quiet)
    {
      if(quiet<2)
	werror("X");
      if(logfile)
	  logfile->write(master()->describe_backtrace(err));
    }
    else
	werror(master()->describe_backtrace(err));
  }
}

int main(int argc, array(string) argv)
{
  /* Redirect all debug and error messages to a logfile. */
  Stdio.File("dumpmodule.log", "caw")->dup2(Stdio.stderr);

  foreach( (array)all_constants(), [string name, mixed func])
    function_names[func]="efun:"+name;

  // It is possible to override functions/objects/programs by
  // inserting another value into the function_names mapping here.
  function_names[Image.Color]="resolv:Image.Color";
  function_names[Image.Color.white]="resolv:Image.Color.white";
  function_names[Image.PNM]="resolv:Image.PNM";
  function_names[Image.X]="resolv:Image.X";
  function_names[Stdio.stdin]="resolv:Stdio.stdin";
  // Thread.Thread ?
  // master() ?

  // Hack to get Calendar files to compile in correct order.
  object tmp = Calendar;

  // Remove the name of the program.
  argv = argv[1..];
  
  if(argv[0]=="--quiet")
  {
    quiet=1;
    argv=argv[1..];

    // FIXME: Make this a command line option..
    // It should not be done when running a binary dist
    // installation...
    logfile=Stdio.File("dumpmodule.log","caw");
  }

  if(argv[0]=="--distquiet")
  {
    quiet=2;
    argv=argv[1..];
    logfile=0;
  }

  if(argv[0] == "--progress-bar")
  {
      quiet = 2;
      logfile = Stdio.File("dumpmodule.log","caw");
      
      progress_bar = Tools.Install.ProgressBar("Precompiling",
					       @array_sscanf(argv[1], "%d,%d"),
					       0.2, 0.8);
      
      argv = argv[2..];
  }

  foreach(argv, string file)
  {
    if(progress_bar)
      progress_bar->update(1);
      
    dumpit(file);
  }

  if(quiet==1)
    werror("\n");
}
