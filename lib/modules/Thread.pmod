constant Mutex=__builtin.mutex;
constant Condition=__builtin.condition;

class Fifo {
  inherit Condition : r_cond;
  inherit Condition: w_cond;
  inherit Mutex: lock;
  
  array buffer;
  int ptr, num;
  int read_tres, write_tres;
  
  int size() {  return num; }
  
  mixed read()
    {
      mixed tmp;
      object key=lock::lock();
      while(!num) r_cond::wait(key);
      tmp=buffer[ptr];
      buffer[ptr++] = 0;	// Throw away any references.
      ptr%=sizeof(buffer);
      if(read_tres < sizeof(buffer))
      {
	if(num-- == read_tres)
	  w_cond::broadcast();
      }else{
	num--;
	w_cond::signal();
      }
      return tmp;
    }

  array read_array()
  {
    array ret;
    object key=lock::lock();
    while(!num) r_cond::wait(key);
    if(num==1)
    {
      ret=buffer[ptr..ptr];
      buffer[ptr++] = 0;	// Throw away any references.
      ptr%=sizeof(buffer);
      num--;
    }else{
      ret=buffer[ptr..]+buffer[..num-sizeof(ret)-1];
      ptr=num=0;
      buffer=allocate(sizeof(buffer)); // Throw away any references.
    }
    w_cond::broadcast();
    return ret;
  }
  
  void write(mixed v)
    {
      object key=lock::lock();
      while(num == sizeof(buffer)) w_cond::wait(key);
      buffer[(ptr + num) % sizeof(buffer)]=v;
      if(write_tres)
      {
	if(num++ == write_tres)
	  r_cond::broadcast();
      }else{
	num++;
	r_cond::signal();
      }
    }

  void create(int|void size)
    {
      write_tres=0;
      buffer=allocate(read_tres=size || 128);
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
      tmp=buffer[r_ptr];
      buffer[r_ptr++] = 0;	// Throw away any references.
      key=0;
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
      key=0; // Must free this one _before_ the signal...
      r_cond::signal();
    }
};

