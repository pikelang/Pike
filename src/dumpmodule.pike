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

#define error(X) throw( ({ (X), backtrace() }) )

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
string mkmodulename(string dirname, mixed x)
{
  if(!sscanf(dirname,"%*slib/modules/%s",dirname))
    return 0;
  dirname-=".pmod";
  dirname=replace(dirname,"/",".");
  if(master()->resolv(dirname) == x)
    return dirname;
  return 0;
}

class __Codec
{
  string last_id;

  string nameof(mixed x)
  {
    string tmp;
//    if(logfile) logfile->write("%O\n",x);
//    werror("%O\n",x);
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
	    if(reverse(tmp)[..4]=="ekip.")
	    {
	      if(string mod=mkmodulename(x, tmp))
		return mod;
	    }
	    return fixup_path(tmp);
	  }
#if 0
	  if(tmp = search(values(_static_modules), x)!=-1)
	  {
	    return "_static_modules."+(indices(_static_modules)[tmp]);
	  }
#endif
	}
	break;

      case "object":
	if(program p=search(master()->objects,x))
	{
	  if(tmp = search(master()->programs,p))
	  {
	    if(reverse(tmp)[..4]=="domp.")
	    {
	      if(string mod=mkmodulename(x, tmp))
		return mod;
	    }
	    return fixup_path(tmp);
	  }else{
#if 0
	    werror("Completely failed to find this program:\n");
	    _describe(p);
#endif
	  }
	}
	if(object_program(x) == master()->dirnode)
	{
	  if(string mod=mkmodulename(x, x->dirname))
	    return mod;
	}
#if 0
	if (tmp = mkmapping(values(__builtin), indices(__builtin))[x]) {
	  return "resolv:__builtin."+tmp;
	}
#endif
	break;
    }
    return ([])[0];
  } 

  function functionof(string x)
  {
    if(sscanf(x,"efun:%s",x))
      return all_constants()[x];

    if(sscanf(x,"resolv:%s",x))
      return master()->resolv(x);

    werror("Failed to decode %s\n",x);
    return 0;
  }


  object objectof(string x)
  {
    if(sscanf(x,"efun:%s",x))
      return all_constants()[x];

    if(sscanf(x,"resolv:%s",x))
      return master()->resolv(x);

    if(sscanf(x,"mpath:%s",x))
      foreach(master()->pike_module_path, string path)
	if(object ret=master()->low_cast_to_object(combine_path(path,x),0))
	  return ret;

    if(object tmp=(object)x) return tmp;
    werror("Failed to decode %s\n",x);
    return 0;
    
  }

  program programof(string x)
  {
    if(sscanf(x,"efun:%s",x))
      return all_constants()[x];

    if(sscanf(x,"resolv:%s",x))
      return master()->resolv(x);

    if(sscanf(x,"_static_modules.%s",x))
    {
      return (program)_static_modules[x];
    }

    if(sscanf(x,"mpath:%s",x))
      foreach(master()->pike_module_path, string path)
	if(program ret=master()->cast_to_program(combine_path(path,x),0))
	  return ret;

    if(program tmp=(program)x) return tmp;
    werror("Failed to decode %s\n",x);
    return 0;
  }

  mixed encode_object(object x)
  {
    if(x->_encode) return x->_encode();
//    if(logfile)
//      logfile->write("Cannot encode objects yet: %s\n",master()->stupid_describe(x,100000));
#if 0
    werror("\n>>>>>>encode object was called for:<<<<<<\n");
    _describe(x);
    werror("\n");
#endif
    error("Cannot encode objects yet.\n");
//    error(sprintf("Cannot encode objects yet. %O\n",indices(x)));
  }

  mixed decode_object(program p, mixed data)
  {
    object ret=p(@data[0]);
    if(sizeof(data)>1) ret->_decode(data[1]);
    error("Cannot encode objects yet.\n");
  }
}

class Codec
{
  inherit __Codec;

  mapping debug_data=([]);

  string nameof(mixed x)
    {
      string ret=::nameof(x);
      if(ret)
	debug_data[ret]=x;
      return ret;
    }

  function functionof(string x)
    {
      function ret=::functionof(x);
      if(debug_data[x] != ret)
	werror("functionof(%O) returned the wrong value (%O != %O)\n",x,debug_data[x],ret);
      return ret;
    }

  object objectof(string x)
    {
      object ret=::objectof(x);
      if(debug_data[x] != ret)
	werror("objectof(%O) returned the wrong value (%O != %O)\n",x,debug_data[x],ret);
      return ret;
    }
  

  program programof(string x)
    {
      program ret=::programof(x);
      if(debug_data[x] != ret)
	werror("programof(%O) returned the wrong value (%O != %O)\n",x,debug_data[x],ret);
      return ret;
    }
  
}

Stdio.File logfile;

class Handler
{
  void compile_error(string file,int line,string err)
    {
      if(!logfile) return;
      logfile->write(sprintf("%s:%d:%s\n",file,line,err));
    }

  void compile_warning(string file,int line,string err)
    {
      if(!logfile) return;
      logfile->write(sprintf("%s:%d:%s\n",file,line,err));
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
    logfile->write("##%s##\n",file);

  if(!quiet)
    werror(file +": ");
  
  mixed err=catch {
    rm(file+".o"); // Make sure no old files are left
    if(mixed s=file_stat(fakeroot(file)))
    {
      if(s[1]<=0)
      {
	werror("is a directory or special file.\n");
	break;
      }
    }else{
      if(!quiet)
      werror("does not exist.\n");
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
	  switch(quiet)
	  {
	    case 1: werror("."); break;
	    case 0: werror("dumped.\n");
	  }
	}else{
	  switch(quiet)
	  {
	    case 1: werror("i"); break;
	    case 0: werror("Decode of %O failed.\n", file);
	  }
	}
      }else{
	switch(quiet)
	{
	  case 1: werror(","); break;
	  case 0: werror("Not dumping %O.\n",file);
	}
      }
    }else{
      switch(quiet)
      {
	case 1: werror("!"); break;
	case 0: werror("Compilation of %O failed.\n", file);
      }
    }
  };
  if(err)
  {
#ifdef ERRORS
    err[0]="While dumping "+file+": "+err[0];
    werror(master()->describe_backtrace(err));
#else
    if(quiet)
    {
      if(quiet<2)
	werror("X");
      if(logfile)
      {
//	err[0]="While dumping "+file+": "+err[0];
//	logfile->write("================================================\n");
	logfile->write(master()->describe_backtrace(err));
      }
    }else{
      werror(err[0]);
    }
#endif
  }
}

int main(int argc, array(string) argv)
{
  /* Redirect all debug and error messages to a logfile. */
  Stdio.File("dumpmodule.log", "caw")->dup2(Stdio.stderr);

  foreach( (array)all_constants(), [string name, mixed func])
    function_names[func]="efun:"+name;

  function_names[Stdio.stdin]="resolv:Stdio.stdin";
  function_names[Stdio.stdout]="resolv:Stdio.stdout";
  function_names[Stdio.stderr]="resolv:Stdio.stderr";
  function_names[_static_modules.Builtin]="resolv:__builtin";
//  function_names[__builtin.__backend]="resolv:__builtin.__backend";

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
//    werror("Dumping modules ");
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

  // werror("Files to dump: %O\n", argv);
  
  foreach(argv, string file)
  {
    if(progress_bar)
      progress_bar->update(1);
      
    dumpit(file);
  }

  if(quiet==1)
    werror("\n");
}
