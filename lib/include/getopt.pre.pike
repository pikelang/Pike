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
		       mixed|void def)
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
		  werror("No argument to option "+tmp+".\n");
		  exit(1);
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
                    werror("No argument to option -"+argv[i][j..j]+".\n");
                    exit(1);
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
 * ({ "name", ({aliases}), env_var, default })
 */
mixed *find_all_options(string *argv, mixed *options, void|int posix_me_harder)
{
  mapping quick=([]);
  foreach(options, mixed opt)
    {
      foreach(stringp(opt[1])?({opt[1]}):opt[1], mixed optname)
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

  mixed *ret=({});
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
	if(mixed *option=quick[opt])
	{
	  argv[e]=0;
	  if(!arg && sizeof(option)>3)
	  {
	    if(e==sizeof(argv)-1)
	    {
	      werror("No argument to option "+opt+".\n");
	      exit(1);
	    }
	    arg=argv[e+1];
	    argv[e+1]=0;
	  }
	  ret+=({ ({ option[0], arg || 1 }) });
	}
      }else{
	string *foo=argv[e]/"";
	for(int j=1;j<strlen(foo);j++)
	{
	  string opt="-"+foo[j];
	  if(mixed *option=quick[opt])
	  {
	    foo[j]=0;
	    string arg=argv[e][j+1..];
	    if(sizeof(option)>3)
	    {
	      if(arg=="")
	      {
		if(e==sizeof(argv)-1)
		{
		  werror("No argument to option "+opt+".\n");
		  exit(1);
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
  foreach(options, string *option)
    {
      if(done[ret]) continue;
      if(sizeof(option) > 2)
      {
	mixed foo=option[2];
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


string *get_args(string *argv, void|int posix_me_harder)
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
	  werror("Unknown option "+argv[i]+".\n");
	  exit(1);
	}
      }else{
	if(strlen(argv[i]) == 2)
	  werror("Unknown option "+argv[i]+".\n");
	else
	  werror("Unknown options "+argv[i]+".\n");
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

void create()
{
  add_constant("find_option",find_option);
  add_constant("get_args",get_args);
}
