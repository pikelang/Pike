mapping data=([]);
string file;
Stdio.File log;

mixed `[](mixed key)
{
  return data[key];
}

void save()
{
  if(Stdio.write_file(file+".tmp",encode_value(data)))
  {
    if(mv(file+".tmp",file))
    {
      if(log) destruct(log);
      rm(file+".log");
    }
  }
}

mixed `[]=(mixed key, mixed val)
{
  if(data[key] == val) return val;
  data[key]=val;

  if(!log) log=Stdio.File(file+".log","wct");

  log->write(
    replace(encode_value( ({key,val}) ),"\0","\0\1")+"\0\0"
    );
    
  return val;
}

void set_many(array(mixed) key, array(mixed) val)
{
  if(sizeof(key) > sizeof(data)/10)
  {
    /* substatial update */
    data|=mkmapping(key,val);
    save();
  }else{
    /* insubstantial update */
    data|=mkmapping(key,val);

    if(!log) log=Stdio.File(file+".log","wct");
    log->write(Array.map(Array.map(Array.transpose( ({key, val}) ),
				   encode_value),
			 replace,"\0","\0\1")*"\0\0" + "\0\0");
  }

}

void create(string f)
{
  werror("Cache: %s",f);
  file=f;
  mixed err=catch {
    if(string tmp=Stdio.read_file(file))
      data=decode_value(tmp);
  };

  if(err)  werror(master()->describe_backtrace(err));
  werror(" %d entries",sizeof(data));

  err=catch {
    if(string s=Stdio.read_file(file+".log"))
    {
      array t=s/"\0\0";
      t=t[..sizeof(t)-2];
      foreach(t, string tmp)
	{
	  string key,val;
	  [key,val]=decode_value(replace(tmp,"\0\1","\0"));
	  data[key]=val;
	}
      werror(" + %d logged",sizeof(t));
      save();
    }
  };
  if(err)  werror(master()->describe_backtrace(err));

  werror("\n");

  atexit(save);
}


void destroy()
{
  save();
}
