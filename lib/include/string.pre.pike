#define BEGIN 32


string strmult(string str, int num)
{
#if 1
  num*=strlen(str);
  while(strlen(str) < num) str+=str;
  return str[0..num-1];
#endif
#if 0
  return sprintf("%~n",str,strlen(str)*num);
#endif
}

void create()
{
  add_constant("strmult",strmult);

  master()->add_precompiled_program("/precompiled/string_buffer", class {
    string *buffer=allocate(BEGIN);
    int ptr=0;
    
    static void fix()
    {
      string tmp=buffer*"";
      buffer=allocate(strlen(tmp)/128+BEGIN);
      buffer[0]=tmp;
      ptr=1;
    }
    
    string get_buffer()
    {
      if(ptr != 1) fix();
      return buffer[0];
    }
    
    void append(string s)
    {
      if(ptr==sizeof(buffer)) fix();
      buffer[ptr++]=s;
    }
    
    mixed cast(string to)
    {
      if(to=="string") return get_buffer();
      return 0;
    }

    void flush()
    {
      buffer=allocate(BEGIN);
      ptr=0;
    }
  });
}
