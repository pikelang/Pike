constant Mutex=__builtin.mutex;
constant Condition=__builtin.condition;


class Fifo {
  inherit Condition : r_cond;
  inherit Condition : w_cond;
  inherit Mutex : lock;
  
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
  inherit Condition : r_cond;
  inherit Mutex : lock;
  
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
}



class Farm
{
  class Result
  {
    int ready;
    mixed value;
    function done_cb;

    int status()
    {
      return ready;
    }

    mixed result()
    {
      return value;
    }

    mixed `()()
    {
      while(!ready)     ft_cond->wait();
      if( ready < 0 )   throw( value );
      return value;
    }

    void set_done_cb( function to )
    {
      if( ready )
        to( value, ready<0 );
      else
        done_cb = to;
    }

    void provide_error( mixed what )
    {
      value = what;
      ready = -1;
      if( done_cb )
        done_cb( what, 1 );
    }
      
    void provide( mixed what )
    {
      ready = 1;
      value = what;
      if( done_cb )
        done_cb( what, 0 );
    }
  }

  static class Handler
  {
    Condition cond = Condition();
    array job;
    object thread;

    float total_time;
    int handled, max_time;

    static int ready;

    void handler()
    {
      array q;
      while( 1 )
      {
        ready = 1;
        cond->wait();
        if( q = job )
        {
          mixed res, err;
          int st = gethrtime();
          if( err = catch(res = q[1][0]( @q[1][1] )) && q[0])
            q[0]->provide_error( err );
          else if( q[0] )
            q[0]->provide( res );
          object lock = mutex->lock();
          free_threads += ({ this_object() });
          lock = 0;
          st = gethrtime()-st;
          total_time += st/1000.0;
          handled++;
          job = 0;
          if( st > max_time )
            max_time = st;
          ft_cond->broadcast();
        } else  {
          object lock = mutex->lock();
          threads -= ({ this_object() });
          free_threads -= ({ this_object() });
          lock = 0;
          destruct();
          return;
        }
      }
    }

    void run( array what, object|void resobj )
    {
      while(!ready) sleep(0.1);
      job = ({ resobj, what });
      cond->signal();
    }

    string debug_status()
    {
      return ("Thread:\n"
              " Handled works: "+handled+"\n"+
              (handled?
               " Average time:  "+((int)(total_time / handled))+"ms\n"
               " Max time:      "+sprintf("%2.2fms\n", max_time/1000.0):"")+
              " Status:        "+(job?"Working":"Idle" )+"\n"+
              (job?
               ("    "+
                replace( describe_backtrace(thread->backtrace()),
                         "\n",
                         "\n    ")):"")
              +"\n\n");
    }

    void create()
    {
      thread = thread_create( handler );
    }
  }

  static Mutex mutex = Mutex();
  static Condition ft_cond = Condition();
  static Queue job_queue = Queue();

  static array(Handler) threads = ({});
  static array(Handler) free_threads = ({});
  static int max_num_threads = 20;

  static Handler aquire_thread()
  {
    object lock = mutex->lock();
    while( !sizeof(free_threads) )
    {
      if( sizeof(threads) < max_num_threads )
      {
        threads += ({ Handler() });
        free_threads += ({ threads[-1] });
      } else {
        lock = 0;
        ft_cond->wait( );
        mutex->lock();
      }
    }
    object t = free_threads[0];
    free_threads = free_threads[1..];
    return t;
  }
        

  static void dispatcher()
  {
    while( array q = job_queue->read() )
      aquire_thread()->run( q[1], q[0] );
  }

  static class ValueAdjuster
  {
    object r, r2;
    mapping v;
    int i;

    void go(mixed vn, int err)
    {
      r->value[ i ] = vn;
      if( err )
        r->provide_error( err );
      if( !--v->num_left )
        r->provide( r->value );
      destruct();
    }
    void create( object _1,object _2,int _3,mapping _4 )
    {
      r = _1; r2 = _2; i=_3; v=_4;
    }
  }

  object run_multiple( array fun_args )
  {
    Result r = Result(); // private result..
    r->value = allocate( sizeof( fun_args ) );
    mapping nl = ([ "num_left":sizeof( fun_args ) ]);
    for( int i=0; i<sizeof( fun_args ); i++ )
    {
      Result  r2 = Result();
      r2->set_done_cb( ValueAdjuster( r, r2, i, nl )->go );
      job_queue->write( ({ r2, fun_args[i] }) );
    }
    return r;
  }


  void run_multiple_async( array fun_args )
  {
    for( int i=0; i<sizeof( fun_args ); i++ )
      job_queue->write( ({ 0, fun_args[i] }) );
  }


  object run( function f, mixed ... args )
  {
    object ro = Result();
    job_queue->write( ({ 0, ({f, args }) }) );
    return ro;
  }

  void run_async( function f, mixed ... args )
  {
    job_queue->write( ({ 0, ({f, args }) }) );
  }

  int set_max_num_threads( int to )
  {
    int omnt = max_num_threads;
    if( to <= 0 )
      error("Illegal argument 1 to set_max_num_threads,"
            "num_threads must be > 0\n");

    max_num_threads = to;
    while( sizeof( threads ) > max_num_threads )
    {
      object key = mutex->lock();
      while( sizeof( free_threads ) )
        free_threads[0]->cond->signal();
      key = 0;
      if( sizeof( threads ) > max_num_threads)
        ft_cond->wait();
    }
    ft_cond->broadcast( );
    return omnt;
  }

  string debug_status()
  {
    string res = sprintf("Thread farm\n"
                         "  Max threads     = %d\n"
                         "  Current threads = %d\n"
                         "  Working threads = %d\n"
                         "  Jobs in queue   = %d\n\n",
                         max_num_threads, sizeof(threads), 
                         (sizeof(threads)-sizeof(free_threads)),
                         job_queue->size() );
    
    foreach( threads, Handler t )
      res += t->debug_status();
    return res;
  }

  void create()
  {
    thread_create( dispatcher );
  }
}


