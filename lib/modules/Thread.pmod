#pike __REAL_VERSION__

#if constant(__builtin.thread_id)
constant Thread=__builtin.thread_id;

constant MutexKey=__builtin.mutex_key;
constant Mutex=__builtin.mutex;
constant Condition=__builtin.condition;
constant _Disabled=__builtin.threads_disabled;
constant Local=__builtin.thread_local;

constant thread_create = predef::thread_create;
constant this_thread = predef::this_thread;
constant all_threads = predef::all_threads;
constant get_thread_quanta = predef::get_thread_quanta;
constant set_thread_quatna = predef::set_thread_quanta;

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
class Fifo {
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

  protected final mixed read_unlocked()
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

  protected final array read_all_unlocked()
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

  protected final void write_unlocked (mixed value)
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
  protected void create(int|void size)
  {
    write_tres=0;
    buffer=allocate(read_tres=size || 128);
  }

  protected string _sprintf( int f )
  {
    return f=='O' && sprintf( "%O(%d / %d)", this_program,
			      size(), read_tres );
  }
}

//! @[Queue] implements a queue, or a pipeline. The main difference
//! between @[Queue] and @[Fifo] is that @[Queue]
//! will never block in write(), only allocate more memory.
//!
//! @fixme
//!   Ought to be made API-compatible with @[ADT.Queue].
//!
//! @seealso
//!   @[Fifo], @[ADT.Queue]
//!
class Queue {
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

  protected final array read_all_unlocked()
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
	ret = buffer[r_ptr..w_ptr-1];
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

  //! Returns a snapshot of all the values in the queue, in the order
  //! they were written. The values are still left in the queue, so if
  //! other threads are reading from it, the returned value should be
  //! considered stale already on return.
  array peek_array()
  {
    if (w_ptr == r_ptr) return ({});
    MutexKey key = lock::lock();
    array ret = buffer[r_ptr..w_ptr - 1];
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
    // NB: The mutex MUST be released before the broadcast to work
    //     around bugs in glibc 2.24 and earlier. This seems to
    //     affect eg RHEL 7 (glibc 2.17).
    //     cf https://sourceware.org/bugzilla/show_bug.cgi?id=13165
    key=0;
    r_cond::broadcast();
    return items;
  }

  protected string _sprintf( int f )
  {
    return f=='O' && sprintf( "%O(%d)", this_program, size() );
  }
}


//! A thread farm.
class Farm
{
  protected Mutex mutex = Mutex();
  protected Condition ft_cond = Condition();
  protected Queue job_queue = Queue();
  protected object dispatcher_thread;
  protected function(object, string:void) thread_name_cb;
  protected string thread_name_prefix;

  //! An asynchronous result.
  //!
  //! @fixme
  //!   This class ought to be made @[Concurrent.Future]-compatible.
  class Result
  {
    int ready;
    mixed value;
    function done_cb;

    //! @returns
    //!   @int
    //!     @value 1
    //!       Returns @expr{1@} when the result is available.
    //!     @value 0
    //!       Returns @expr{0@} (zero) when the result hasn't
    //!       arrived yet.
    //!     @value -1
    //!       Returns negative on failure.
    //!   @endint
    int status()
    {
      return ready;
    }

    //! @returns
    //!   Returns the result if available, a backtrace on failure,
    //!   and @expr{0@} (zero) otherwise.
    mixed result()
    {
      return value;
    }

    //! Wait for completion.
    protected mixed `()()
    {
      object key = mutex->lock();
      while(!ready)     ft_cond->wait(key);
      key = 0;
      if( ready < 0 )   throw( value );
      return value;
    }

    //! Register a callback to be called when
    //! the result is available.
    //!
    //! @param to
    //!   Callback to be called. The first
    //!   argument to the callback will be
    //!   the result or the failure backtrace,
    //!   and the second @expr{0@} (zero) on
    //!   success, and @expr{1@} on failure.
    void set_done_cb( function to )
    {
      if( ready )
        to( value, ready<0 );
      else
        done_cb = to;
    }

    //! Register a failure.
    //!
    //! @param what
    //!   The corresponding backtrace.
    void provide_error( mixed what )
    {
      value = what;
      ready = -1;
      if( done_cb )
        done_cb( what, 1 );
    }

    //! Register a completed result.
    //!
    //! @param what
    //!   The result to register.
    void provide( mixed what )
    {
      ready = 1;
      value = what;
      if( done_cb )
        done_cb( what, 0 );
    }


    protected string _sprintf( int f )
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

  protected void report_errors(mixed err, int is_err)
  {
    if (!is_err) return;
    master()->handle_error(err);
  }

  //! A wrapper for an asynchronous result.
  //!
  //! @note
  //!   This wrapper is used to detect when the
  //!   user discards the values returned from
  //!   @[run()] or @[run_multiple()], so that
  //!   any errors thrown aren't lost.
  protected class ResultWrapper(Result ro)
  {
    protected int(0..1) value_fetched;
    int `ready() { return ro->ready; }
    mixed `value() {
      value_fetched = 1;
      return ro->value;
    }
    function `done_cb() { return ro->done_cb; }
    void `done_cb=(function cb) {
      if (cb) value_fetched = 1;
      ro->done_cb = cb;
    }

    //! @returns
    //!   @int
    //!     @value 1
    //!       Returns @expr{1@} when the result is available.
    //!     @value 0
    //!       Returns @expr{0@} (zero) when the result hasn't
    //!       arrived yet.
    //!     @value -1
    //!       Returns negative on failure.
    //!   @endint
    function(:int) `status() { return ro->status; }

    //! @returns
    //!   Returns the result if available, a backtrace on failure,
    //!   and @expr{0@} (zero) otherwise.
    function(:mixed) `result() { return ro->result; }

    //! Wait for completion.
    protected mixed `()()
    {
      return ro();
    }

    //! Register a callback to be called when
    //! the result is available.
    //!
    //! @param to
    //!   Callback to be called. The first
    //!   argument to the callback will be
    //!   the result or the failure backtrace,
    //!   and the second @expr{0@} (zero) on
    //!   success, and @expr{1@} on failure.
    void set_done_cb(function cb) {
      if (cb) value_fetched = 1;
      ro->set_done_cb(cb);
    }

    //! Register a failure.
    //!
    //! @param what
    //!   The corresponding backtrace.
    function(mixed:void) `provide_error() { return ro->provide_error; }

    //! Register a completed result.
    //!
    //! @param what
    //!   The result to register.
    function(mixed:void) `provide() { return ro->provide; }

    protected void _destruct()
    {
      if (ro && !value_fetched && !ro->done_cb) {
	// Make sure that any errors aren't lost.
	ro->done_cb = report_errors;
	if (ro->ready < 0) {
	  report_errors(ro->value, 1);
	}
      }
    }

    protected string _sprintf( int f )
    {
      switch( f )
      {
	case 't':
	  return "Thread.Farm().ResultWrapper";
	case 'O':
	  return sprintf("%t(%O)", this, ro);
      }
    }
  }

  //! A worker thread.
  protected class Handler
  {
    Mutex job_mutex = Mutex();
    Condition cond = Condition();
    array(object|array(function|array)) job;
    object thread;

    float total_time;
    int handled, max_time;

    protected int ready;

    void update_thread_name(int is_exiting)
    {
      if (thread_name_cb) {
        string th_name =
          !is_exiting &&
          sprintf("%s Handler 0x%x", thread_name_prefix, thread->id_number());
        thread_name_cb(thread, th_name);
      }
    }

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

          err = catch(res = q[1][0]( @q[1][1] ));

          if( q[0] )
          {
            if( err )
              ([object]q[0])->provide_error( err );
            else
              ([object]q[0])->provide( res );
          }
          object lock = mutex->lock();
          free_threads += ({ this });
          lock = 0;
          st = gethrtime()-st;
          total_time += st/1000.0;
          handled++;
          job = 0;
          q = 0;
          if( st > max_time )
            max_time = st;
          ft_cond->broadcast();
        } else {
          object lock = mutex->lock();
          threads -= ({ this });
          free_threads -= ({ this });
          lock = 0;
          update_thread_name(1);
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

    //! Get some statistics about the worker thread.
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

    protected void create()
    {
      thread = thread_create( handler );
      update_thread_name(0);
    }


    protected string _sprintf( int f )
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

  protected array(Handler) threads = ({});
  protected array(Handler) free_threads = ({});
  protected int max_num_threads = 20;

  protected Handler aquire_thread()
  {
    object lock = mutex->lock();
    while( !sizeof(free_threads) )
    {
      if( sizeof(threads) < max_num_threads )
      {
        threads += ({ Handler() });
        free_threads += ({ threads[-1] });
      } else {
        ft_cond->wait(lock);
      }
    }
    object(Handler) t = free_threads[0];
    free_threads = free_threads[1..];
    return t;
  }


  protected void dispatcher()
  {
    while( array q = [array]job_queue->read() ) {
      aquire_thread()->run( q[1], q[0] );
      q = 0;
    }
    if (thread_name_cb)
      thread_name_cb(this_thread(), 0);
  }

  protected class ValueAdjuster( object r, object r2, int i, mapping v )
  {
    void go(mixed vn, int err)
    {
      if (!r->status()) {
	([array]r->value)[ i ] = vn;
	if( err )
	  r->provide_error( err );
	if( !--v->num_left )
	  r->provide( r->value );
      }
      destruct();
    }
  }


  //! Register multiple jobs.
  //!
  //! @param fun_args
  //!   An array of arrays where the first element
  //!   is a function to call, and the second is
  //!   a corresponding array of arguments.
  //!
  //! @returns
  //!   Returns a @[Result] object with an array
  //!   with one element for the result for each
  //!   of the functions in @[fun_args].
  //!
  //! @note
  //!   Do not modify the elements of @[fun_args]
  //!   before the result is available.
  //!
  //! @note
  //!   If any of the functions in @[fun_args] throws
  //!   and error, all of the accumulated results
  //!   (if any) will be dropped from the result, and
  //!   the first backtrace be provided.
  //!
  //! @seealso
  //!   @[run_multiple_async()]
  Result run_multiple( array(array(function|array)) fun_args )
  {
    Result r = Result(); // private result..
    ResultWrapper rw = ResultWrapper(r);
    r->value = allocate( sizeof( fun_args ) );
    mapping nl = ([ "num_left":sizeof( fun_args ) ]);
    for( int i=0; i<sizeof( fun_args ); i++ )
    {
      Result r2 = Result();
      r2->set_done_cb( ValueAdjuster( r, r2, i, nl )->go );
      job_queue->write( ({ r2, fun_args[i] }) );
    }
    return rw;
  }


  //! Register multiple jobs where the return values
  //! are to be ignored.
  //!
  //! @param fun_args
  //!   An array of arrays where the first element
  //!   is a function to call, and the second is
  //!   a corresponding array of arguments.
  //!
  //! @note
  //!   Do not modify the elements of @[fun_args]
  //!   before the result is available.
  //!
  //! @seealso
  //!   @[run_multiple()]
  void run_multiple_async( array fun_args )
  {
    for( int i=0; i<sizeof( fun_args ); i++ )
      job_queue->write( ({ 0, fun_args[i] }) );
  }


  //! Register a job for the thread farm.
  //!
  //! @param f
  //!   Function to call with @@@[args] to
  //!   perform the job.
  //!
  //! @param args
  //!   The parameters for @[f].
  //!
  //! @returns
  //!   Returns a @[Result] object for the job.
  //!
  //! @note
  //!   In Pike 7.8 and earlier this function
  //!   was broken and returned a @[Result]
  //!   object that wasn't connected to the job.
  //!
  //! @seealso
  //!   @[run_async()]
  Result run( function f, mixed ... args )
  {
    Result ro = Result();
    ResultWrapper rw = ResultWrapper(ro);
    job_queue->write( ({ ro, ({f, args }) }) );
    return rw;
  }

  //! Register a job for the thread farm
  //! where the return value from @[f] is
  //! ignored.
  //!
  //! @param f
  //!   Function to call with @@@[args] to
  //!   perform the job.
  //!
  //! @param args
  //!   The parameters for @[f].
  //!
  //! @seealso
  //!   @[run()]
  void run_async( function f, mixed ... args )
  {
    job_queue->write( ({ 0, ({f, args }) }) );
  }

  //! Set the maximum number of worker threads
  //! that the thread farm may have.
  //!
  //! @param to
  //!   The new maximum number.
  //!
  //! If there are more worker threads than @[to],
  //! the function will wait until enough threads
  //! have finished, so that the total is @[to] or less.
  //!
  //! The default maximum number of worker threads is @expr{20@}.
  int set_max_num_threads( int(1..) to )
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

  //! Provide a callback function to track names of threads created by the
  //! farm.
  //!
  //! @param cb
  //!   The callback function. This will get invoked with the thread as the
  //!   first parameter and the name as the second whenever a thread is
  //!   created. When the same thread terminates the callback is invoked
  //!   again with @[0] as the second parameter. Set @[cb] to @[0] to stop
  //!   any previously registered callbacks from being called.
  //!
  //! @param prefix
  //!   An optional name prefix to distinguish different farms. If not given
  //!   a prefix will be generated automatically.
  void set_thread_name_cb(function(object, string:void) cb, void|string prefix)
  {
    thread_name_cb = cb;
    thread_name_prefix =
      cb &&
      (prefix || sprintf("Thread.Farm 0x%x", dispatcher_thread->id_number()));

    //  Give a name to all existing threads
    if (thread_name_cb) {
      thread_name_cb(dispatcher_thread, thread_name_prefix + " Dispatcher");
      foreach (threads, Handler t)
        t->update_thread_name(0);
    }
  }

  //! Get some statistics for the thread farm.
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

  protected string _sprintf( int f )
  {
    return f=='O' && sprintf( "%O(/* %s */)", this_program, debug_status() );
  }

  protected void create()
  {
    dispatcher_thread = thread_create( dispatcher );
  }
}

//! When this key is destroyed, the corresponding resource counter
//! will be decremented.
//!
//! @seealso
//!   @[ResourceCount], @[MutexKey]
//!
class ResourceCountKey {

  private inherit __builtin.DestructImmediate;

  /*semi*/private ResourceCount parent;

  protected void create(ResourceCount _parent)
  {
    parent = _parent;
  }

  protected void _destruct()
  {
    MutexKey key = parent->_mutex->lock();
    --parent->_count;
    parent->_cond->signal();
  }
}

//! Implements an inverted-semaphore-like resource
//! counter.  A thread can poll or perform a blocking wait for the
//! resource-count to drop below a certain @ref{level@}.
//!
//! @seealso
//!   @[ResourceCountKey], @[Condition], @[Mutex]
class ResourceCount {
  /*semi*/final int _count;
  /*semi*/final Condition _cond = Condition();
  /*semi*/final Mutex _mutex = Mutex();

  //! @param level
  //!   The maximum level that is considered drained.
  //!
  //! @returns
  //!   True if the resource counter drops to equal or below @ref{level@}.
  /*semi*/final int(0..1) drained(void|int level) {
    return level >= _count;
  }

  //! Blocks until the resource-counter dips to max @ref{level@}.
  //!
  //! @param level
  //!   The maximum level that is considered drained.
  /*semi*/final void wait_till_drained(void|int level) {
    MutexKey key = _mutex->lock();
    while (_count > level)		// Recheck before allowing further
      _cond->wait(key);
  }

  //! Increments the resource-counter.
  //! @returns
  //!   A @[ResourceCountKey] to decrement the resource-counter again.
  /*semi*/final ResourceCountKey acquire() {
    MutexKey key = _mutex->lock();
    _count++;
    return ResourceCountKey(this);
  }

  protected string _sprintf(int type)
  {
    string res = UNDEFINED;
    switch(type) {
      case 'O':
        res = sprintf("Count: %d", _count);
        break;
      case 'd':
        res = sprintf("%d", _count);
        break;
    }
    return res;
  }
}

#else /* !constant(thread_create) */

// Simulations of some of the classes for nonthreaded use.

/* Fallback implementation of Thread.Local */
class Local
{
  protected mixed data;
  mixed get() {return data;}
  mixed set (mixed val) {return data = val;}
}

/* Fallback implementation of Thread.MutexKey */
class MutexKey (protected function(:void) dec_locks)
{
  inherit Builtin.DestructImmediate;

  int `!()
  {
    // Should be destructed when the mutex is, but we can't pull that
    // off. Try to simulate it as well as possible.
    if (dec_locks) return 0;
    destruct (this);
    return 1;
  }

  protected void _destruct()
  {
    if (dec_locks) dec_locks();
  }
}

/* Fallback implementation of Thread.Mutex */
class Mutex
{
  protected int locks = 0;
  protected void dec_locks() {locks--;}

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
class Fifo
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

  protected void create(int|void size)
  {
    write_tres=0;
    buffer=allocate(read_tres=size || 128);
  }

  protected string _sprintf( int f )
  {
    return f=='O' && sprintf( "%O(%d / %d)", this_program,
			      size(), read_tres );
  }
}

// Fallback implementation of Thread.Queue.
class Queue
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
	ret = buffer[r_ptr..w_ptr-1];
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

  protected string _sprintf( int f )
  {
    return f=='O' && sprintf( "%O(%d)", this_program, size() );
  }
}

#endif /* !constant(thread_create) */
