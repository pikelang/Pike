constant Mutex=__builtin.mutex;
constant Condition=__builtin.condition;

class Fifo {
  inherit Condition : r_cond;
  inherit Condition: w_cond;
  inherit Mutex: lock;
  
  mixed *buffer;
  int ptr, num;
  
  int size() {  return num; }
  
  mixed read()
    {
      mixed tmp;
      object key=lock::lock();
      while(!num) r_cond::wait(key);
      tmp=buffer[ptr++];
      ptr%=sizeof(buffer);
      num--;
      w_cond::signal();
      return tmp;
    }
  
  void write(mixed v)
    {
      object key=lock::lock();
      while(num == sizeof(buffer)) w_cond::wait(key);
      buffer[(ptr + num++) % sizeof(buffer)]=v;
      r_cond::signal();
    }
  
  varargs void create(int size)
    {
      buffer=allocate(size || 128);
    }
};

class Queue {
  inherit Condition: r_cond;
  inherit Mutex: lock;
  
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
};

