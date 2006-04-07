#pike __REAL_VERSION__

#if constant(thread_create)
constant Thread=__builtin.thread_id;

// The reason for this inherit is rather simple.
// It's now possible to write Thread Thread( ... );
//
// This makes the interface look somewhat more thought-through.
//
inherit Thread;

// We don't want to create a thread of the module...
static void create(mixed ... args)
{
}

optional Thread `()( mixed f, mixed ... args )
{
  return thread_create( f, @args );
}

optional constant MutexKey=__builtin.mutex_key;
optional constant Mutex=__builtin.mutex;
optional constant Condition=__builtin.condition;
optional constant _Disabled=__builtin.threads_disabled;
optional constant Local=__builtin.thread_local;

optional constant thread_create = predef::thread_create;

optional constant this_thread = predef::this_thread;

optional constant all_threads = predef::all_threads;

constant THREAD_NOT_STARTED = __builtin.THREAD_NOT_STARTED;
constant THREAD_RUNNING = __builtin.THREAD_RUNNING;
constant THREAD_EXITED = __builtin.THREAD_EXITED;


//! @[Fifo] implements a fixed length first-in, first-out queue.
//! A fifo is a queue of values and is often used as a stream of data
//! between two threads.
//!
//! @seealso
//!   @[Queue]
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
  //!   @[read()], @[write()]
  //!
  int size() {  return num; }

  static nomask mixed read_unlocked()
  {
    mixed tmp=buffer[ptr];
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
    return tmp;
  }

  //! This function retrieves a value from the fifo. Values will be
  //! returned in the order they were written. If there are no values
  //! present in the fifo the current thread will sleep until some other
  //! thread writes one.
  //!
  //! @seealso
  //!   @[try_read()], @[read_array()], @[write()]
  //!
  mixed read()
  {
    object key=lock::lock();
    while(!num) r_cond::wait(key);
    mixed res = read_unlocked();
    key = 0;
    return res;
  }

  //! This function retrieves a value from the fifo if there is any
  //! there. Values will be returned in the order they were written.
  //! If there are no values present in the fifo then @[UNDEFINED]
  //! will be returned.
  //!
  //! @seealso
  //!   @[read()]
  //!
  mixed try_read()
  {
    if (!num) return UNDEFINED;
    object key=lock::lock();
    if (!num) return UNDEFINED;
    mixed res = read_unlocked();
    key = 0;
    return res;
  }

  static nomask array read_all_unlocked()
  {
    array ret;

    switch (num) {
      case 0:
	ret = ({});
	break;

      case 1:
	ret=buffer[ptr..ptr];
	buffer[ptr++] = 0;	// Throw away any references.
	ptr%=sizeof(buffer);
	num = 0;
	w_cond::broadcast();
	break;

      default:
	if (ptr+num < sizeof(buffer)) {
	  ret = buffer[ptr..ptr+num-1];
	} else {
	  ret = buffer[ptr..]+buffer[..num-(sizeof(buffer)-ptr)-1];
	}
	ptr=num=0;
	buffer=allocate(sizeof(buffer)); // Throw away any references.
	w_cond::broadcast();
	break;
    }

    return ret;
  }

  //! This function returns all values in the fifo as an array. The
  //! values in the array will be in the order they were written. If
  //! there are no values present in the fifo the current thread will
  //! sleep until some other thread writes one.
  //!
  //! @seealso
  //!   @[read()], @[try_read_array()]
  //!
  array read_array()
  {
    object key=lock::lock();
    while(!num) r_cond::wait(key);
    array ret = read_all_unlocked();
    key = 0;
    return ret;
  }

  //! This function returns all values in the fifo as an array but
  //! doesn't wait if there are no values there. The values in the
  //! array will be in the order they were written.
  //!
  //! @seealso
  //!   @[read_array()]
  //!
  array try_read_array()
  {
    if (!num) return ({});
    object key=lock::lock();
    array ret = read_all_unlocked();
    key = 0;
    return ret;
  }

  static nomask void write_unlocked (mixed value)
  {
    buffer[(ptr + num) % sizeof(buffer)] = value;
    if(write_tres)
    {
      if(num++ == write_tres)
	r_cond::broadcast();
    }else{
      num++;
      r_cond::broadcast();
    }
  }

  //! Append a @[value] to the end of the fifo. If there is no more
  //! room in the fifo the current thread will sleep until space is
  //! available. The number of items in the queue after the write is
  //! returned.
  //!
  //! @seealso
  //!   @[read()]
  //!
  int write(mixed value)
  {
    object key=lock::lock();
    while(num == sizeof(buffer)) w_cond::wait(key);
    write_unlocked (value);
    int items = num;
    key = 0;
    return items;
  }

  //! Append a @[value] to the end of the fifo. If there is no more
  //! room in the fifo then zero will be returned, otherwise the
  //! number of items in the fifo after the write is returned.
  //!
  //! @seealso
  //!   @[read()]
  //!
  int try_write(mixed value)
  {
    if (num == sizeof (buffer)) return 0;
    object key=lock::lock();
    if (num == sizeof (buffer)) return 0;
    write_unlocked (value);
    int items = num;
    key = 0;
    return items;
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
    return f=='O' && sprintf( "%O(%d / %d)", this_program,
			      size(), read_tres );
  }
}

//! @[Queue] implements a queue, or a pipeline. The main difference
//! between @[Queue] and @[Fifo] is that @[Queue]
//! will never block in write(), only allocate more memory.
//!
//! @seealso
//!   @[Fifo]
//!
optional class Queue {
  inherit Condition : r_cond;
  inherit Mutex : lock;
  
  array buffer=allocate(16);
  int r_ptr, w_ptr;
  
  //! This function returns the number of elements currently in the queue.
  //!
  //! @seealso
  //!   @[read()], @[write()]
  //!
  int size() {  return w_ptr - r_ptr;  }

  //! This function retrieves a value from the queue. Values will be
  //! returned in the order they were written. If there are no values
  //! present in the queue the current thread will sleep until some other
  //! thread writes one.
  //!
  //! @seealso
  //!   @[try_read()], @[write()]
  //!
  mixed read()
  {
    mixed tmp;
    object key=lock::lock();
    while(w_ptr == r_ptr) r_cond::wait(key);
    tmp=buffer[r_ptr];
    buffer[r_ptr++] = 0;	// Throw away any references.
    key=0;
    return tmp;
  }

  //! This function retrieves a value from the queue if there is any
  //! there. Values will be returned in the order they were written.
  //! If there are no values present in the fifo then @[UNDEFINED]
  //! will be returned.
  //!
  //! @seealso
  //!   @[write()]
  //!
  mixed try_read()
  {
    if (w_ptr == r_ptr) return UNDEFINED;
    object key=lock::lock();
    if (w_ptr == r_ptr) return UNDEFINED;
    mixed tmp=buffer[r_ptr];
    buffer[r_ptr++] = 0;	// Throw away any references.
    key=0;
    return tmp;
  }

  static nomask array read_all_unlocked()
  {
    array ret;

    switch (w_ptr - r_ptr) {
      case 0:
	ret = ({});
	break;

      case 1:
	ret=buffer[r_ptr..r_ptr];
	buffer[r_ptr++] = 0;	// Throw away any references.
	break;

      default:
	ret = buffer[r_ptr..w_ptr];
	r_ptr = w_ptr = 0;
	buffer=allocate(sizeof(buffer)); // Throw away any references.
	break;
    }

    return ret;
  }

  //! This function returns all values in the queue as an array. The
  //! values in the array will be in the order they were written. If
  //! there are no values present in the queue the current thread will
  //! sleep until some other thread writes one.
  //!
  //! @seealso
  //!   @[read()], @[try_read_array()]
  //!
  array read_array()
  {
    object key=lock::lock();
    while (w_ptr == r_ptr) r_cond::wait(key);
    array ret = read_all_unlocked();
    key = 0;
    return ret;
  }

  //! This function returns all values in the queue as an array but
  //! doesn't wait if there are no values there. The values in the
  //! array will be in the order they were written.
  //!
  //! @seealso
  //!   @[read_array()]
  //!
  array try_read_array()
  {
    if (w_ptr == r_ptr) return ({});
    object key=lock::lock();
    array ret = read_all_unlocked();
    key = 0;
    return ret;
  }

  //! This function puts a @[value] last in the queue. If the queue is
  //! too small to hold the @[value] it will be expanded to make room.
  //! The number of items in the queue after the write is returned.
  //!
  //! @seealso
  //!   @[read()]
  //!
  int write(mixed value)
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
    int items = w_ptr - r_ptr;
    r_cond::signal();
    key=0;
    return items;
  }

  static string _sprintf( int f )
  {
    return f=='O' && sprintf( "%O(%d)", this_program, size() );
  }
}



optional class Farm
{
  static Mutex mutex = Mutex();
  static Condition ft_cond = Condition();
  static Queue job_queue = Queue();

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
      object key = mutex->lock();
      while(!ready)     ft_cond->wait(key);
      key = 0;
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
	  return sprintf( "%t(%d %O)", this, ready, value );
      }
    }
  }

  static class Handler
  {
    Mutex job_mutex = Mutex();
    Condition cond = Condition();
    array(object|array(function|array)) job;
    object thread;

    float total_time;
    int handled, max_time;

    static int ready;

    void handler()
    {
      array(object|array(function|array)) q;
      object key = job_mutex->lock();
      ready = 1;
      while( 1 )
      {
        cond->wait(key);
        if( q = job )
        {
          mixed res, err;
          int st = gethrtime();
          if( err = catch(res = q[1][0]( @q[1][1] )) && q[0])
            ([object]q[0])->provide_error( err );
          else if( q[0] )
            ([object]q[0])->provide( res );
          object lock = mutex->lock();
          free_threads += ({ this });
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
          threads -= ({ this });
          free_threads -= ({ this });
          lock = 0;
          destruct();
          return;
        }
      }
    }

    void run( array(function|array) what, object|void resobj )
    {
      while(!ready) sleep(0.1);
      object key = job_mutex->lock();
      job = ({ resobj, what });
      cond->signal();
      key = 0;
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
	  return sprintf( "%t(%f / %d,  %d)", this,
			  total_time, max_time, handled );
      }
    }
  }

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
        ft_cond->wait(mutex);
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
      if( sizeof( threads ) > max_num_threads)
        ft_cond->wait(key);
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
    return f=='O' && sprintf( "%O(/* %s */)", this_program, debug_status() );
  }

  static void create()
  {
    thread_create( dispatcher );
  }
}

#else /* !constant(thread_create) */

// Simulations of some of the classes for nonthreaded use.

/* Fallback implementation of Thread.Local */
optional class Local
{
  static mixed data;
  mixed get() {return data;}
  mixed set (mixed val) {return data = val;}
}

/* Fallback implementation of Thread.MutexKey */
optional class MutexKey (static function(:void) dec_locks)
{
  int `!()
  {
    // Should be destructed when the mutex is, but we can't pull that
    // off. Try to simulate it as well as possible.
    if (dec_locks) return 0;
    destruct (this);
    return 1;
  }

  static void destroy()
  {
    if (dec_locks) dec_locks();
  }
}

/* Fallback implementation of Thread.Mutex */
optional class Mutex
{
  static int locks = 0;
  static void dec_locks() {locks--;}

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

// Fallback implementation of Thread.Fifo.
optional class Fifo
{
  array buffer;
  int ptr, num;
  int read_tres, write_tres;

  int size() {  return num; }

  mixed read()
  {
    if (!num) error ("Deadlock detected - fifo empty.\n");
    return try_read();
  }

  mixed try_read()
  {
    if (!num) return UNDEFINED;
    mixed tmp=buffer[ptr];
    buffer[ptr++] = 0;	// Throw away any references.
    ptr%=sizeof(buffer);
    return tmp;
  }

  array read_array()
  {
    if (!num) error ("Deadlock detected - fifo empty.\n");
    return try_read_array();
  }

  array try_read_array()
  {
    array ret;
    switch (num) {
      case 0:
	ret = ({});
	break;

      case 1:
	ret=buffer[ptr..ptr];
	buffer[ptr++] = 0;	// Throw away any references.
	ptr%=sizeof(buffer);
	num = 0;
	break;

      default:
	if (ptr+num < sizeof(buffer)) {
	  ret = buffer[ptr..ptr+num-1];
	} else {
	  ret = buffer[ptr..]+buffer[..num-(sizeof(buffer)-ptr)-1];
	}
	ptr=num=0;
	buffer=allocate(sizeof(buffer)); // Throw away any references.
	break;
    }

    return ret;
  }

  int try_write(mixed value)
  {
    if (num == sizeof (buffer)) return 0;
    buffer[(ptr + num) % sizeof(buffer)] = value;
    return ++num;
  }

  int write(mixed value)
  {
    if (!try_write(value)) error("Deadlock detected - fifo full.\n");
    return num;
  }

  static void create(int|void size)
  {
    write_tres=0;
    buffer=allocate(read_tres=size || 128);
  }

  static string _sprintf( int f )
  {
    return f=='O' && sprintf( "%O(%d / %d)", this_program,
			      size(), read_tres );
  }
}

// Fallback implementation of Thread.Queue.
optional class Queue
{
  array buffer=allocate(16);
  int r_ptr, w_ptr;
  
  int size() {  return w_ptr - r_ptr;  }

  mixed read()
  {
    if (w_ptr == r_ptr) error ("Deadlock detected - queue empty.\n");
    return try_read();
  }

  mixed try_read()
  {
    mixed tmp=buffer[r_ptr];
    buffer[r_ptr++] = 0;	// Throw away any references.
    return tmp;
  }

  array read_array()
  {
    if (w_ptr == r_ptr) error ("Deadlock detected - queue empty.\n");
    return try_read_array();
  }

  array try_read_array()
  {
    array ret;

    switch (w_ptr - r_ptr) {
      case 0:
	ret = ({});
	break;

      case 1:
	ret=buffer[r_ptr..r_ptr];
	buffer[r_ptr++] = 0;	// Throw away any references.
	break;

      default:
	ret = buffer[r_ptr..w_ptr];
	r_ptr = w_ptr = 0;
	buffer=allocate(sizeof(buffer)); // Throw away any references.
	break;
    }

    return ret;
  }

  int write(mixed value)
  {
    if(w_ptr >= sizeof(buffer))
    {
      buffer=buffer[r_ptr..];
      buffer+=allocate(sizeof(buffer)+1);
      w_ptr-=r_ptr;
      r_ptr=0;
    }
    buffer[w_ptr] = value;
    w_ptr++;
    return w_ptr - r_ptr;
  }

  static string _sprintf( int f )
  {
    return f=='O' && sprintf( "%O(%d)", this_program, size() );
  }
}

#endif /* !constant(thread_create) */
