#!/usr/local/bin/pike

#define PC Parser.C

#ifdef OLD
import ".";
#define PC .C
#endif

#if 0
string strip(string s)
{
  array(string) tmp=s/".";
  switch(tmp[-1])
  {
    case "protos":
    case "pp":
    case "pph":
    case "h":
    case "c":
    case "cmod":
      tmp[-1]="pp";
      s=tmp*".";
  }
  s=replace(s,"_t.","."); /* magic */
  s=replace(s,"\\\\","/");
  s=basename(lower_case(s));
  return s;
}
#endif

string fixdir(string d)
{
  return combine_path(getcwd(),d);
}

string my_dirname(string s)
{
  s=replace(s,"\\\\","/");
  return dirname(lower_case(s));
}

string low_strip_other_files(string data, string s)
{
#if 0
  return data;
#endif
#if 1
  /* Filter out data from system include files, this should
   * speed up operation significantly
   */
  if(!data)
  {
    werror("File %s missing?\n",s);
    return 0;
  }

  data-="\r";
  array tmp=data/"\n#";
  string ret=tmp[0];
  string current_dir=getcwd();
  string source_dir=fixdir(getenv("SRCDIR"));
  string target_dir=my_dirname(s);
  int on=1;

  foreach(tmp[1..], string x)
    {
      string file;
      if(sscanf(x,"line %*d \"%s\"", file) ||
	 sscanf(data," %*d \"%s\"", file))

	if(file)
	{
	  string dir=my_dirname(file);
	  on= dir==current_dir || dir==source_dir || dir==target_dir;
	  if (!on) {
	    // No match yet. check if it's a suffix.
	    on = (dir == current_dir[sizeof(current_dir) - sizeof(dir)..]) ||
	      (dir == source_dir[sizeof(source_dir) - sizeof(dir)..]) ||
	      (dir == target_dir[sizeof(target_dir) - sizeof(dir)..]);
	    if (!on) {
	      // Special case for bison.
	      on = (search(lower_case(file), "bison") != -1) ||
		(search(lower_case(file), "yacc") != -1);
	    }
	  }
	}

      if(on) ret+="#"+x;
    }
  return ret;
#endif
#if 0
  array x=data/"PMOD_EXPORT";
  x[0]="";
  return x*"PMOD_EXPORT";
#endif
#if 0
  if(!data)
  {
    werror("File %s missing?\n",s);
    return 0;
  }

  data-="\r";
  array tmp=data/"\n#";
  string ret=tmp[0];
  string cf=s;
  int cl=1;

  s=strip(s);
  foreach(tmp[1..], string x)
    {
      string file;
      if(sscanf(x,"line %*d \"%s\"", file) ||
	 sscanf(data," %*d \"%s\"", file))
	if(file) cf=strip(file);


      if(cf == s) ret+="#"+x;
    }
  return ret;
#endif
}

string my_read_file(string s)
{
  if(getenv("SRCDIR"))
  {
    string tmp=combine_path(getenv("SRCDIR"),s);
    if(Stdio.file_size(tmp) != -1)  s=tmp;
  }
  return Stdio.read_file(s);
}


array flatten(array a)
{
  array ret=({});
  foreach(a, a) ret+=arrayp(a)?flatten(a):({a});
  return ret;
}

string arg(array x, int num)
{
  if( sizeof(x) <= ( num >= 0 ? num : ~num )) return "";
  return (string) ( arrayp(x[num]) ? x[num][0] : x[num] );
}

array(string) fixarg(array s)
{
//  werror("%O\n",s);
  if(sizeof(s)>1)
  {
    switch(arg(s,-1))
    {
      case "void":
      case "int":
      case "char":
      case "unsigned":
      case "*":
      case "short":
      case "long":
      case "float":
      case "double":
      case "...":
      case "(":
	break;

      default:
	switch(arg(s,-2))
	{
	  case "union":
	  case "enum":
	  case "struct":
	  case "const":
	  case "register":
	    break;

	  default:
	    s=s[..sizeof(s)-2];
	}
    }
  }

  return flatten(s);
}

string implode(array(string) s)
{
  string ret=" ";
  foreach(flatten(s), mixed s)
    {
      switch(ret[-1])
      {
	case 'a'..'z':
	case 'A'..'Z':
	case '_':
	case '0'..'9':
	  switch(((string)s)[0])
	  {
	    case 'a'..'z':
	    case 'A'..'Z':
	    case '_':
	    case '0'..'9':
	      ret+=" ";
	  }
      }
      ret+=(string)s;
    }
  return ret[1..];
}


array classify(array s)
{
//  werror("%O\n",s);
//  werror("Classifying..");
  array data=({});
  s=s/({"PMOD_PROTO"}) * ({ "PMOD_EXPORT","PPROTO" });
//  werror("CLASSIFY %d\n",sizeof(s/ ({"PMOD_EXPORT"})));
  foreach((s/ ({"PMOD_EXPORT"}))[1..], array expr)
    {
      int a;
      int proto=0;
      int var=1;

      if(expr[0]=="PPROTO")
      {
	expr=expr[1..];
	proto=1;
      }

      for(a=0;a<sizeof(expr);a++)
      {
	if(arrayp(expr[a]))
	{
	  if(sizeof(expr[a]))
	  {
	    switch((string)(expr[a][0]))
	    {
	      default: continue;
	      case "(": var=0;continue;
	      case "{": break;
	    }
	    break;
	  }
	}else{
	  switch((string) expr[a])
	  {
	    default:               continue;
	    case ";":              if(!proto && !var) a=0;
	    case "=":
	    case "__attribute__":  break;
	  }
	  break;
	}
      }
      expr=expr[..a-1];
      if(!sizeof(expr)) continue;
//      werror("Considering %O\n",expr[0..4]);
      while(1)
      {
//	werror("FN: %O\n",expr);
	switch((string)expr[0])
	{
	  case "extern":
	    expr=expr[1..];
	    continue;

	    break;
	    
	  case "typedef":
	  case "typdef":
	    break; /* ignored */

	  case "struct":
	  case "union":
	  case "enum":
//	    werror("%O\n",expr);
	    if(sizeof(expr) <= 2) break; /* Forward decl */
	  default:
//	    werror("  %O\n",expr);
	    int ptr=sizeof(expr)-1;
	    while(arrayp(expr[ptr]) && expr[ptr][0]=='[') ptr--;

	    if(arrayp(expr[ptr]))
	    {
	      if("(" != (string) (expr[ptr][0]) ) break; /* Ignore structs and unions */

//	      werror("GURKA!\n");
	      array args=map(expr[ptr][1..sizeof(expr[ptr])-2]/({","}),fixarg);
	      string rettype=implode(expr[..sizeof(expr)-3]);
//	      werror("%O\n",expr);
	      mixed name=expr[-2];
	      string location;
	      if(arrayp(name))
	      {
		location=name[0]->file+":"+name[0]->line;
		name=PC.simple_reconstitute(name);
	      }else{
		location=name->file+":"+name->line;
		name=(string)name;
	      }
	      data+=({ ([
		"class": "function",
		"rettype": rettype,
		"location":location,
		"name": name,
		"args": args,
		"post_type":implode(expr[ptr+1..]),
		"ptrtype":sprintf("%s(*)(%s)",rettype,implode(args*({","}))),
		"proto":sprintf("%s %s(%s)",rettype,name,implode(args*({","}))),
		]) });
	    }else{
//	      werror("FNORD\n");
	      string type=implode(expr[..sizeof(expr)-2]);
	      data+=({ ([
		"arg_type":"",
		"post_type":implode(expr[ptr+1..]),
		"class": "variable",
		"ptrtype": type+"*",
		"type": type,
		"location":sprintf("%s:%d",expr[ptr]->file||"",expr[ptr]->line),
		"name": expr[ptr],
		"proto":implode(expr),
		]) });
	    }
	}

	break;
      }

    }
  return data;
}

array prototypes=({});

void low_process_file(mixed data, string file)
{
  if(mixed err=catch {
    data=low_strip_other_files(data,file);
    if(!data || !sizeof(data)) return;
    data=PC.split(data);
    data=PC.tokenize(data,file);
    if(!data || !sizeof(data)) return;
    data=PC.hide_whitespaces(data);
    data=PC.strip_line_statements(data);
    data=PC.group(data);
    if(sizeof(data) && sizeof( (string) (data[0]) ))
    {
      switch(((string)data[0])[0] )
      {
	case ' ': case '\n': case '\t': case '\14': case '\r':
	  data=data[1..];
      }
    }
    data=classify(data);
    prototypes+=data;
  }) throw(err);
//  werror("%s Done\n",file);
}

void process_file(string file)
{
//  werror("Processing %s\n",file);
  low_process_file(my_read_file(file), file);
}

int main(int argc, string *argv)
{
  int protos_only;
  int run_cpp;
  int num_threads=1;
  /* Hmm, threaded operation doesn't seem to work, maybe
   * my windoze machine needs more memory?
   */

  foreach(Getopt.find_all_options(argv,aggregate(
    ({"protos",Getopt.NO_ARG,({"--protos"})}),
    ({"run_cpp",Getopt.NO_ARG,({"--cpp"})}),
    ({"threads",Getopt.HAS_ARG,({"--threads"})}),
    ),1),array opt)
  {
    switch(opt[0])
    {
      case "protos": protos_only++; break;
      case "run_cpp": run_cpp++; break;
      case "threads": num_threads=(int)opt[1]; break;
    }
  }

  argv=Getopt.get_args(argv,1);

  if (!getenv("SRCDIR")) {
    werror("$SRCDIR has not been set!\n");
    exit(1);
  }

  if(run_cpp)
  {
    object f=Stdio.File();
    object p=f->pipe(Stdio.PROP_IPC);
    object proc=Process.create_process(argv[1..], (["stdout":p]));
    
    destruct(p);
    string data=f->read(0x7fffffff);
    if(int x=proc->wait())
    {
      werror("CPP failed with error code %d\n",x);
      exit(1);
    }
    low_process_file(data, argv[-1]);
  }else{

#if constant(thread_create)
    if(num_threads >1)
    {
      werror("Using %d threads\n",num_threads);
      Thread.Fifo fifo=Thread.Fifo();
      void worker()
	{
	  while(string f=fifo->read()) process_file(f);
	};
      array threads=allocate(num_threads, thread_create)(worker);
    foreach(argv[1..], string file) fifo->write(file);
    foreach(threads, mixed t) fifo->write(0);
    threads->wait();
    }else
#endif
      foreach(argv[1..], string file)
	process_file(file);
  }
  

  prototypes=values(mkmapping((array(string))(prototypes->name),prototypes));
  sort((array(string))(prototypes->name),prototypes);

  if(protos_only)
  {
    foreach(prototypes, mixed expr)
      write("PMOD_PROTO %s;\n",expr->proto);
    werror("[ %d symbol%s exported ]\n", sizeof(prototypes),sizeof(prototypes)==1?"":"s");
    exit(0);
  }else{
    string ret="/* Fake prototypes */\n";
    foreach(prototypes, mixed expr)
      ret+=sprintf("extern int %s;\n",expr->name);
    ret+="\nvoid *PikeSymbol[]= {\n";
    foreach(prototypes, mixed expr)
      ret+=sprintf("  (void *)& %s,\n",expr->name,expr->name);
    ret+="0 };\n\n";
    
    /* Fixme: touch module_magic.h if we need to
     * recompile all the modules.
     */
    
    Stdio.write_file("export_functions.c",ret);
    
    ret="PMOD_EXPORT extern void **PikeSymbol;\n";
    int num=0;
    foreach(prototypes, mixed expr)
      {
	ret+=sprintf("/* %s */\n#define %s (*(%s)(PikeSymbol[%d]))\n",
		     expr->location || "",
		     expr->name,
		     expr->ptrtype,
		     num++);
	
	if(expr->post_type!="")
	  werror("NOSUPP: %s %s %s\n",expr->type,expr->name,expr->post_type);
      }
    
    Stdio.stdout->write(ret);
    
    werror("[ %d symbol%s exported ]\n", sizeof(prototypes),sizeof(prototypes)==1?"":"s");
    if(!sizeof(prototypes)) exit(1);
  }
}
