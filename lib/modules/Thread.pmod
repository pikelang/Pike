#pike __REAL_VERSION__

#if constant(thread_create)
constant Thread=__builtin.thread_id;

// The reason for this inherit is rather simple.
// It's now possible to write Thread Thread( ... );
//
// This makes the interface look somewhat more thought-through.
//
inherit Thread;

optional Thread `()( mixed f, mixed ... args )
{
  return thread_create( f, @args );
}

optional constant MutexKey=__builtin.mutex_key;
optional constant Mutex=__builtin.mutex;
optional constant Condition=__builtin.condition;
optional constant _Disabled=__builtin.threads_disabled;
optional constant Local=__builtin.thread_local;

//! @decl Thread.Thread Thread.Thread(function(mixed...:void) f,
//!                                   mixed ... args)
//!
//! This function creates a new thread which will run simultaneously
//! to the rest of the program. The new thread will call the function
//! @[f] with the arguments @[args]. When @[f] returns the thread will cease
//! to exist.
//!
//! All Pike functions are 'thread safe' meaning that running
//! a function at the same time from different threads will not corrupt
//! any internal data in the Pike process.
//!
//! @returns
//! The returned value will be the same as the return value of
//! @[Thread.this_thread()] for the new thread.
//!
//! @note
//! This function is only available on systems with POSIX or UNIX or WIN32
//! threads support.
//!
//! @seealso
//! @[Mutex], @[Condition], @[this_thread()]
//!
optional constant thread_create = predef::thread_create;

//! @decl Thread.Thread this_thread()
//!
//! This function returns the object that identifies this thread.
//!
//! @seealso
//! @[Thread.Thread()]
//!
optional constant this_thread = predef::this_thread;

//! @decl array(Thread.Thread) all_threads()
//!
//! This function returns an array with the thread ids of all threads.
//!
//! @seealso
//! @[Thread.Thread()]
//!
optional constant all_threads = predef::all_threads;



//! @[Thread.Fifo] implements a fixed length first-in, first-out queue.
//! A fifo is a queue of values and is often used as a stream of data
//! between two threads.
//!
//! @note
//! Fifos are only available on systems with threads support.
//!
//! @seealso
//! @[Thread.Queue]
//!
optional class Fifo {
  inherit Condition : r_cond;
  inherit Condition : w_cond;
  inherit Mutex : lock;
  
  array buffer;
  int ptr, num;
  int read_tres, write_tres;

  //! This function returns the number of elements currently in the fifo.
  //!
  //! @seealso
  //! @[read()], @[write()]
  //!
  int size() {  return num; }

  //! This function retrieves a value from the fifo. Values will be
  //! returned in the order they were written. If there are no values
  //! present in the fifo the current thread will sleep until some other
  //! thread writes a value to the fifo.
  //!
  //! @seealso
  //! @[write()], @[read_array()]
  //!
  mixed read()
  {
    mixed tmp;
    object key=lock::lock(2);
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
      w_cond::broadcast();
    }
    key = 0;
    return tmp;
  }

  //! This function returns all values currently in the fifo. Values in
  //! the array will be in the order they were written. If there are no
  //! values present in the fifo the current thread will sleep until
  //! some other thread writes a value to the fifo.
  //!
  //! @seealso
  //! @[write()], @[read()]
  //!
  array read_array()
  {
    array ret;
    object key=lock::lock(2);
    while(!num) r_cond::wait(key);
    if(num==1)
    {
      ret=buffer[ptr..ptr];
      buffer[ptr++] = 0;	// Throw away any references.
      ptr%=sizeof(buffer);
      num--;
    }else{
      if (ptr+num < sizeof(buffer)) {
	ret = buffer[ptr..ptr+num-1];
      } else {
	ret = buffer[ptr..]+buffer[..num-(sizeof(buffer)-ptr)-1];
      }
      ptr=num=0;
      buffer=allocate(sizeof(buffer)); // Throw away any references.
    }
    key = 0;
    w_cond::broadcast();
    return ret;
  }

  //! Append a @[value] to the end of the fifo. If there is no more
  //! room in the fifo the current thread will sleep until space is
  //! available.
  //!
  //! @seealso
  //! @[read()]
  //!
  void write(mixed value)
  {
    object key=lock::lock(2);
    while(num == sizeof(buffer)) w_cond::wait(key);
    buffer[(ptr + num) % sizeof(buffer)] = value;
    if(write_tres)
    {
      if(num++ == write_tres)
	r_cond::broadcast();
    }else{
      num++;
      r_cond::broadcast();
    }
    key = 0;
  }

  //! @decl void create()
  //! @decl void create(int size)
  //!
  //! Create a fifo. If the optional @[size] argument is present it
  //! sets how many values can be written to the fifo without blocking.
  //! The default @[size] is 128.
  //!
  static void create(int|void size)
  {
    write_tres=0;
    buffer=allocate(read_tres=size || 128);
  }

  static string _sprintf( int f )
  {
    switch( f )
    {
      case 't':
	return "Thread.Fifo";
      case 'O':
	return sprintf( "%t(%d / %d)", this_object(), size(), read_tres );
    }
  }
};

//! @[Thread.Queue] implements a queue, or a pipeline. The main difference
//! between @[Thread.Queue] and @[Thread.Fifo] is that @[Thread.Queue]
//! will never block in write(), only allocate more memory.
//!
//! @note
//! Queues are only available on systems with POSIX or UNIX or WIN32
//! thread support.
//!
//! @seealso
//! @[Thread.Fifo]
//!
optional class Queue {
  inherit Condition : r_cond;
  inherit Mutex : lock;
  
  array buffer=allocate(16);
  int r_ptr, w_ptr;
  
  //! This function returns the number of elements currently in the queue.
  //!
  //! @seealso
  //! @[read()], @[write()]
  //!
  int size() {  return w_ptr - r_ptr;  }

  //! This function retrieves a value from the queue. Values will be
  //! returned in the order they were written. If there are no values
  //! present in the queue the current thread will sleep until some other
  //! thread writes a value to the queue.
  //!
  //! @seealso
  //! @[write()]
  //!
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

  //! This function puts a @[value] last in the queue. If the queue is
  //! too small to hold the @[value] the queue will be expanded to make
  //! room for it.
  //!
  //! @seealso
  //! @[read()]
  //!
  void write(mixed value)
  {
    object key=lock::lock();
    if(w_ptr >= sizeof(buffer))
    {
      buffer=buffer[r_ptr..];
      buffer+=allocate(sizeof(buffer)+1);
      w_ptr-=r_ptr;
      r_ptr=0;
    }
    buffer[w_ptr] = value;
    w_ptr++;
    key=0; // Must free this one _before_ the signal...
    r_cond::signal();
  }

  static string _sprintf( int f )
  {
    switch( f )
    {
      case 't':
	return "Thread.Queue";
      case 'O':
	return sprintf( "%t(%d)", this_object(), size() );
    }
  }
}



optional class Farm
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


    static string _sprintf( int f )
    {
      switch( f )
      {
	case 't':
	  return "Thread.Farm().Result";
	case 'O':
	  return sprintf( "%t(%d %O)", this_object(), ready, value );
      }
    }
  }

  static class Handler
  {
    Condition cond = Condition();
    array(object|array(function|array)) job;
    object thread;

    float total_time;
    int handled, max_time;

    static int ready;

    void handler()
    {
      array(object|array(function|array)) q;
      while( 1 )
      {
        ready = 1;
        cond->wait();
        if( q = job )
        {
          mixed res, err;
          int st = gethrtime();
          if( err = catch(res = q[1][0]( @q[1][1] )) && q[0])
            ([object]q[0])->provide_error( err );
          else if( q[0] )
            ([object]q[0])->provide( res );
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

    void run( array(function|array) what, object|void resobj )
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

    static void create()
    {
      thread = thread_create( handler );
    }


    static string _sprintf( int f )
    {
      switch( f )
      {
	case 't':
	  return "Thread.Farm().Handler";
	case 'O':
	  return sprintf( "%t(%f / %d,  %d)", total_time, max_time,handled );
      }
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
    object(Handler) t = free_threads[0];
    free_threads = free_threads[1..];
    return t;
  }
        

  static void dispatcher()
  {
    while( array q = [array]job_queue->read() )
      aquire_thread()->run( q[1], q[0] );
  }

  static class ValueAdjuster( object r, object r2, int i, mapping v )
  {
    void go(mixed vn, int err)
    {
      ([array]r->value)[ i ] = vn;
      if( err )
        r->provide_error( err );
      if( !--v->num_left )
        r->provide( r->value );
      destruct();
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


  static string _sprintf( int f )
  {
    switch( f )
    {
      case 't':
	return "Thread.Farm";
      case 'O':
	return sprintf( "%t(/* %s */)", this_object, debug_status() );
    }
  }



  static void create()
  {
    thread_create( dispatcher );
  }
}

#else /* !constant(thread_create) */

// Simulations of some of the classes for nonthreaded use.

class Local
{
  static mixed data;
  mixed get() {return data;}
  mixed set (mixed val) {return data = val;}
}

class MutexKey (static function(:void) dec_locks)
{
  int `!()
  {
    // Should be destructed when the mutex is, but we can't pull that
    // off. Try to simulate it as well as possible.
    if (dec_locks) return 0;
    destruct (this_object());
    return 1;
  }

  static void destroy()
  {
    if (dec_locks) dec_locks();
  }
}

//! @[Thread.Mutex] is a class that implements mutual exclusion locks.
//! Mutex locks are used to prevent multiple threads from simultaneously
//! execute sections of code which access or change shared data. The basic
//! operations for a mutex is locking and unlocking. If a thread attempts
//! to lock an already locked mutex the thread will sleep until the mutex
//! is unlocked.
//!
//! @note
//! This class is simulated when Pike is compiled without thread support,
//! so it's always available.
//!
//! In POSIX threads, mutex locks can only be unlocked by the same thread
//! that locked them. In Pike any thread can unlock a locked mutex.
//!
class Mutex
{
  static int locks = 0;
  static void dec_locks() {locks--;}

  //! @decl MutexKey lock()
  //! @decl MutexKey lock(int type)
  //!
  //! This function attempts to lock the mutex. If the mutex is already
  //! locked by a different thread the current thread will sleep until the
  //! mutex is unlocked. The value returned is the 'key' to the lock. When
  //! the key is destructed or has no more references the lock will
  //! automatically be unlocked. The key will also be destructed if the lock
  //! is destructed.
  //!
  //! The @[type] argument specifies what @[lock()] should do if the
  //! mutex is already locked by this thread:
  //! @integer
  //!   @value 0 (default)
  //!     Throw an error.
  //!   @value 1
  //!     Sleep until the mutex is unlocked. Useful if some
  //!     other thread will unlock it.
  //!   @value 2
  //!     Return zero. This allows recursion within a locked region of
  //!     code, but in conjunction with other locks it easily leads
  //!     to unspecified locking order and therefore a risk for deadlocks.
  //!
  //! @seealso
  //! @[trylock()]
  //!
  MutexKey lock (int|void type)
  {
    switch (type) {
      default:
	error ("Unknown mutex locking style: %d\n", type);
      case 0:
	if (locks) error ("Recursive mutex locks.\n");
	break;
      case 1:
	if (locks)
	  // To be really accurate we should hang now, but somehow
	  // that doesn't seem too useful.
	  error ("Deadlock detected.\n");
	break;
      case 2:
	if (locks) return 0;
    }
    locks++;
    return MutexKey (dec_locks);
  }

  //! @decl MutexKey trylock()
  //! @decl MutexKey trylock(int type)
  //!
  //! This function performs the same operation as @[lock()], but if the mutex
  //! is already locked, it will return zero instead of sleeping until it's
  //! unlocked.
  //!
  //! @seealso
  //! @[lock()]
  //!
  MutexKey trylock (int|void type)
  {
    switch (type) {
      default:
	error ("Unknown mutex locking style: %d\n", type);
      case 0:
	if (locks) error ("Recursive mutex locks.\n");
	break;
      case 1:
      case 2:
    }
    if (locks) return 0;
    locks++;
    return MutexKey (dec_locks);
  }
}

#endif /* !constant(thread_create) */
