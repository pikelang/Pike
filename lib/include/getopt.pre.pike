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

  hasarg=query_num_arg() == 5;
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
		argv[i+1]=1;
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
      } else {
	if(getenv("POSIX_ME_HARDER"))
	  break;
      }
    }
  }

  if(arrayp(envvars))
    foreach(envvars, value)
      if(value && (value=getenv(value)))
	return value;
  
  return def;
}


string *get_args(string *argv)
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
    }
  }

  argv-=({0});

  return argv;
}

void create()
{
  add_constant("find_option",find_option);
  add_constant("get_args",get_args);
}
