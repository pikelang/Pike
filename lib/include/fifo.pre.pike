void create()
{
  master()->add_precompiled_program("/precompiled/fifo", class {
    inherit "/precompiled/condition": r_cond;
    inherit "/precompiled/condition": w_cond;
    inherit "/precompiled/mutex": lock;

    mixed *buffer;
    int r_ptr, w_ptr;

    int query_messages()    {  return w_ptr - r_ptr;  }
    
    mixed read()
    {
      mixed tmp;
      object key=lock::lock();
      while(!query_messages()) r_cond::wait(key);
      tmp=buffer[r_ptr++ % sizeof(buffer)];
      w_cond::signal();
      return tmp;
    }

    void write(mixed v)
    {
      object key=lock::lock();
      while(query_messages() == sizeof(buffer)) w_cond::wait(key);
      buffer[w_ptr++ % sizeof(buffer)]=v;
      r_cond::signal();
    }

    varargs void create(int size)
    {
      buffer=allocate(size || 128);
    }
  });
}
