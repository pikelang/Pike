void create()
{
  master()->add_precompiled_program("/precompiled/fifo", class {
    inherit "/precompiled/condition": r_cond;
    inherit "/precompiled/condition": w_cond;
    inherit "/precompiled/mutex": lock;

    mixed *buffer;
    int r_ptr, w_ptr;

    int size() {  return (w_ptr+sizeof(buffer) - r_ptr) % sizeof(buffer);  }
    
    mixed read()
    {
      mixed tmp;
      object key=lock::lock();
      while(!size()) r_cond::wait(key);
      tmp=buffer[r_ptr];
      if(++r_ptr >= sizeof(buffer)) r_ptr=0;
      w_cond::signal();
      return tmp;
    }

    void write(mixed v)
    {
      object key=lock::lock();
      while(size() == sizeof(buffer)) w_cond::wait(key);
      buffer[w_ptr]=v;
      if(++w_ptr >= sizeof(buffer)) r_ptr=0;
      r_cond::signal();
    }

    varargs void create(int size)
    {
      buffer=allocate(size || 128);
    }
  });

  master()->add_precompiled_program("/precompiled/queue", class {
    inherit "/precompiled/condition": r_cond;
    inherit "/precompiled/mutex": lock;

    mixed *buffer=allocate(16);
    int r_ptr, w_ptr;

    int size() {  return w_ptr - r_ptr;  }
    
    mixed read()
    {
      mixed tmp;
      object key=lock::lock();
      while(!size()) r_cond::wait(key);
      tmp=buffer[r_ptr++];
      return tmp;
    }

    void write(mixed v)
    {
      object key=lock::lock();
      if(w_ptr >= sizeof(buffer))
      {
	buffer=buffer[r_ptr..];
	buffer+=allocate(sizeof(buffer)+1);
	w_ptr-=r_ptr;
	r_ptr=0;
      }
      buffer[w_ptr]=v;
      w_ptr++;
      r_cond::signal();
    }
  });
}
