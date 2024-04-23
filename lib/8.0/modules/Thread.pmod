#pike 8.1

#pragma no_deprecation_warnings

inherit Thread.module : pre;

#if constant(thread_create)

//! Create a new thread.
//!
//! @deprecated predef::Thread.Thread
optional Thread `()( mixed f, mixed ... args )
{
  return thread_create( f, @args );
}

//! A thread farm.
class Farm
{
  protected Mutex mutex = Mutex();
  protected Condition ft_cond = Condition();
  protected Queue job_queue = Queue();
  protected object dispatcher_thread;
  protected function(object, string|zero: void) thread_name_cb;
  protected string thread_name_prefix;

  //! An asynchronous result.
  //!
  //! @note
  //!   This class has been removed in Pike 9.0 and later.
  //!   Places that used this class there instead use @[Concurrent.Future].
  class Result
  {
    int ready;
    mixed value;
    function(mixed, int(0..1):void) done_cb;

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
      object|zero key = mutex->lock();
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
    void set_done_cb( function(mixed, int(0..1):void) to )
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
      if (!this)				// Only if not destructed
        return "(destructed)";
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
  //!
  //! @note
  //!   This class has been removed in Pike 9.0 and later.
  //!   Places that used this class there instead use @[Concurrent.Future].
  protected class ResultWrapper
  {
    //! @decl inherit Result
    //!
    //! Inherits @expr{Pike.ProxyFactory(Result)@}
    //!
    //! @seealso
    //!   @[Pike.ProxyFactory()]

    //! @ignore
    local inherit Pike.ProxyFactory(Result) : Result;
    //! @endignore

    protected int(0..1) value_fetched;
    mixed `value() {
      value_fetched = 1;
      return Result::`value();
    }
    void `done_cb=(function cb) {
      if (cb) value_fetched = 1;
      Result::`done_cb = cb;
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
      Result::set_done_cb(cb);
    }

    protected void _destruct()
    {
      if (!value_fetched /* && !done_cb */) {
        // Make sure that any errors aren't lost.
        done_cb = report_errors;
        if (ready < 0) {
          report_errors(value, 1);
        }
      }
    }
  }

  //! A worker thread.
  protected class Handler
  {
    Mutex job_mutex = Mutex();
    Condition cond = Condition();
    array(object|array(function|array)) job;
    object|zero thread;

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
      object|zero key = job_mutex->lock();
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
          object|zero lock = mutex->lock();
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
          object|zero lock = mutex->lock();
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
      object|zero key = job_mutex->lock();
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
      if (!this)				// Only if not destructed
        return "(destructed)";
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
    object|zero lock = mutex->lock();
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
    while( array|zero q = [array]job_queue->read() ) {
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
  //! @note
  //!   In Pike 9.0 and later this function returns a @[Concurrent.Promise].
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
  //! @note
  //!   In Pike 9.0 and later this function returns a @[Concurrent.Promise].
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
      object|zero key = mutex->lock();
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
    if (!this)				// Only if not destructed
      return "(destructed)";
    return f=='O' && sprintf( "%O(/* %s */)", this_program, debug_status() );
  }

  protected void create()
  {
    dispatcher_thread = thread_create( dispatcher );
  }
}

#endif
