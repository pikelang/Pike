//  startpid = (int)find_arg(argv, "s", ({ "start-script-pid" }),
// 			     ({ "ROXEN_START_SCRIPT_PID"}));

//  configuration_dir = find_arg(argv, "d", ({ "config-dir",
//					     "configurations",
//					     "configuration-directory" }),
//			          ({ "ROXEN_CONFIGDIR", "CONFIGURATIONS" }),
//			           "../configurations");


string|int find_option(array argv,
		       array|string shortform,
		       array|string|void longform,
		       array|string|void envvars,
		       mixed|void def,
		       int|void throw_errors)
{
  mixed value;
  int i,hasarg;

  hasarg=query_num_arg() > 4;
  if(!arrayp(longform)) longform=({longform});
  if(!arrayp(shortform)) shortform=({shortform});
  if(!arrayp(envvars)) envvars=({envvars});

  for(i=1; i<sizeof(argv); i++)
  {
    if(argv[i] && strlen(argv[i]) > 1)
    {
      if(argv[i][0] == '-')
      {
	if(argv[i][1] == '-')
	{
	  string tmp;
	  int nf;

	  if(argv[i]=="--") break;

	  sscanf(tmp=argv[i], "%s=%s", tmp, value);
	  
	  if(search(longform, tmp[2..]) != -1)
	  {
	    argv[i]=0;
	    if(hasarg)
	    {
	      if(!value)
	      {
		if(i == sizeof(argv)-1)
		{
		  if (throw_errors) {
		    throw(({ "No argument to option "+tmp+".\n",
			       backtrace() }));
		  } else {
		    werror("No argument to option "+tmp+".\n");
		    exit(1);
		  }
		}
		value=argv[i+1];
		argv[i+1]=0;
	      }
	      return value;
	    } else {
	      return value || 1;
	    }
	  }
	} else {
	  int j;
	  for(j=1;j<strlen(argv[i]);j++)
	  {
            string opt;
            int pos;
	    if(search(shortform, opt=argv[i][j..j]) != -1)
	    {
              string arg;
              arg=argv[i][j+1..];

	      if(hasarg)
	      {
		if(arg=="")
		{
		  if(i == sizeof(argv)-1)
                  {
		    if (throw_errors) {
		      throw(({ "No argument to option -"+argv[i][j..j]+".\n",
				 backtrace() }));
		    } else {
		      werror("No argument to option -"+argv[i][j..j]+".\n");
		      exit(1);
		    }
		  }

                  value=argv[i+1];
		  argv[i+1] = 0;
		} else {
		  value=arg;
                  arg="";
		}
	      } else {
		value=1;
	      }

	      argv[i]=argv[i][..j-1]+arg;
	      if(argv[i]=="-") argv[i]=0;
	      return value;
	    }
	  }
	}
      }
    }
  }

  if(arrayp(envvars))
    foreach(envvars, value)
      if(value && (value=getenv(value)))
	return value;
  
  return def;
}

/*
 * ({ "name", type, ({aliases}), env_var, default })
 */

constant HAS_ARG=1;
constant NO_ARG=2;
constant MAY_HAVE_ARG=3;

#define NAME 0
#define TYPE 1
#define ALIASES 2
#define ENV 3
#define DEF 4

array find_all_options(array(string) argv, array options,
		       void|int posix_me_harder, void|int throw_errors)
{
  mapping quick=([]);
  foreach(options, mixed opt)
    {
      mixed aliases=opt[ALIASES];
      if(!arrayp(aliases)) aliases=({aliases});
      foreach(aliases, mixed optname)
	{
	  if(optname[0..1]=="--")
	  {
	    quick[optname]=opt;
	  }else{
	    foreach(optname[1..]/"",string optletter)
	      {
		quick["-"+optletter]=opt;
	      }
	  }
	}
    }

  array ret=({});
  for(int e=1;e<sizeof(argv);e++)
  {
    if(!argv[e]) continue;

    if(strlen(argv[e]) && argv[e][0]=='-')
    {
      if(strlen(argv[e])>1 && argv[e][1]=='-')
      {
	string opt=argv[e];
	if(opt=="--") break;

	string arg;
	sscanf(opt,"%s=%s",opt, arg);
	if(array option=quick[opt])
	{
	  argv[e]=0;
	  if(!arg && option[TYPE]==HAS_ARG)
	  {
	    if(e==sizeof(argv)-1)
	    {
	      if (throw_errors) {
		throw(({ "No argument to option "+opt+".\n",
			   backtrace() }));
	      } else {
		werror("No argument to option "+opt+".\n");
		exit(1);
	      }
	    }
	    arg=argv[e+1];
	    argv[e+1]=0;
	  }
	  ret+=({ ({ option[0], arg || 1 }) });
	}
      }else{
	array(string) foo=argv[e]/"";
	for(int j=1;j<strlen(foo);j++)
	{
	  string opt="-"+foo[j];
	  if(array option=quick[opt])
	  {
	    foo[j]=0;
	    string arg;
	    if(option[TYPE]!=NO_ARG)
	    {
	      arg=argv[e][j+1..];
	      
	      if(option[TYPE]==HAS_ARG && arg=="")
	      {
		if(e==sizeof(argv)-1)
		{
		  if (throw_errors) {
		    throw(({ "No argument to option "+opt+".\n",
			       backtrace() }));
		  } else {
		    werror("No argument to option "+opt+".\n");
		    exit(1);
		  }
		}
		arg=argv[e+1];
		argv[e+1]=0;
	      }else{
		foo=foo[..j];
	      }
	    }

	    ret+=({ ({ option[0], arg || 1 }) });
	  }
	}
	argv[e]=foo*"";
	if(argv[e]=="-") argv[e]=0;
      }
    }else{
      if(posix_me_harder || getenv("POSIX_ME_HARDER"))
	break;
    }
  }

  multiset done=mkmultiset(column(ret, 0));
  foreach(options, array(string) option)
    {
      string name=option[NAME];
      if(done[name]) continue;
      if(sizeof(option) > ENV)
      {
	mixed foo=option[ENV];
	if(!foo) continue;
	if(stringp(foo)) foo=({foo});
	foreach(foo, foo)
	  {
	    if(foo=getenv(foo))
	    {
	      ret+=({ ({name, foo}) });
	      done[name]=1;
	      break;
	    }
	  }

	if(!done && sizeof(option)>3 && option[3])
	{
	  ret+=({ ({name, option[3]}) });
	  done[name]=1;
	}
      }
    }
  return ret;
}


array(string) get_args(array(string) argv, void|int posix_me_harder,
		       void|int throw_errors)
{
  int i;
  for(i=1;i<sizeof(argv);i++)
  {
    if(argv[i] && strlen(argv[i])>1 && argv[i][0]=='-')
    {
      if(argv[i][1]=='-')
      {
	if(argv[i]=="--")
	{
	  argv[i]=0;
	  break;
	}else{
	  if (throw_errors) {
	    throw(({ "Unknown option "+argv[i]+".\n",
		       backtrace() }));
	  } else {
	    werror("Unknown option "+argv[i]+".\n");
	    exit(1);
	  }
	}
      }else{
	if(strlen(argv[i]) == 2) {
	  if (throw_errors) {
	    throw(({ "Unknown option "+argv[i]+".\n",
		       backtrace() }));
	  } else {
	    werror("Unknown option "+argv[i]+".\n");
	  }
	} else {
	  if (throw_errors) {
	    throw(({ "Unknown options "+argv[i]+".\n",
		       backtrace() }));
	  } else {
	    werror("Unknown options "+argv[i]+".\n");
	  }
	}
	exit(1);
      }
    }else{
      if(posix_me_harder || getenv("POSIX_ME_HARDER"))
	break;
    }
  }

  argv-=({0,1});

  return argv;
}

