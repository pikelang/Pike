#pike __REAL_VERSION__

#if constant(__builtin.thread_id)

//! This module contains classes and functions for interacting
//! and synchronizing with threads.
//!
//! @note
//!   For convenience some of the classes here are emulated
//!   (and are thus available) when threads are not supported.
//!
//!   Some of the classes that have such fallbacks are: @[Condition],
//!   @[Fifo], @[Local], @[Mutex], @[MutexKey] and @[Queue].
//!
//! @seealso
//!   @[Thread], @[Mutex], @[Condition]

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
constant THREAD_ABORTED = __builtin.THREAD_ABORTED;

#if constant(__builtin.MUTEX_SUPPORTS_SHARED_LOCKS)
//! Recognition constant for support of shared locks.
//!
//! If this symbol exists and is not @expr{0@} then the @[Mutex] class
//! supports shared locks (ie has @[Mutex()->shared_lock()] et al).
constant MUTEX_SUPPORTS_SHARED_LOCKS = __builtin.MUTEX_SUPPORTS_SHARED_LOCKS;
#endif

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
  //!   @[peek()], @[try_read()], @[read_array()], @[write()]
  //!
  mixed read()
  {
    object|zero key = lock::lock();
    while(!num) r_cond::wait(key);
    mixed res = read_unlocked();
    key = 0;
    return res;
  }

  //! This function retrieves a value from the fifo if there are any
  //! there. Values will be returned in the order they were written.
  //! If there are no values present in the fifo then @[UNDEFINED]
  //! will be returned.
  //!
  //! @seealso
  //!   @[peek()], @[read()]
  //!
  mixed try_read()
  {
    if (!num) return UNDEFINED;
    object|zero key = lock::lock();
    if (!num) return UNDEFINED;
    mixed res = read_unlocked();
    key = 0;
    return res;
  }

  //! This function returns the next value in the fifo if there are
  //! any there. If there are no values present in the fifo then
  //! @[UNDEFINED] will be returned.
  //!
  //! @seealso
  //!   @[read()], @[try_read()]
  mixed peek()
  {
    if (!num) return UNDEFINED;
    object|zero key = lock::lock();
    if (!num) return UNDEFINED;
    mixed res = buffer[num];
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
    object|zero key = lock::lock();
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
    object|zero key = lock::lock();
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
    object|zero key = lock::lock();
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
    object|zero key = lock::lock();
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

  protected int(0..) _sizeof()
  {
    return size();
  }

  protected string _sprintf( int f )
  {
    if (!this)				// Only if not destructed
      return "(destructed)";
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
  //!   @[peek()], @[try_read()], @[write()]
  //!
  mixed read()
  {
    mixed tmp;
    object|zero key = lock::lock();
    while(w_ptr == r_ptr) r_cond::wait(key);
    tmp=buffer[r_ptr];
    buffer[r_ptr++] = 0;	// Throw away any references.
    key=0;
    return tmp;
  }

  //! This function retrieves a value from the queue if there are any
  //! there. Values will be returned in the order they were written.
  //! If there are no values present in the queue then @[UNDEFINED]
  //! will be returned.
  //!
  //! @seealso
  //!   @[peek()], @[read()], @[write()]
  //!
  mixed try_read()
  {
    if (w_ptr == r_ptr) return UNDEFINED;
    object|zero key = lock::lock();
    if (w_ptr == r_ptr) return UNDEFINED;
    mixed tmp=buffer[r_ptr];
    buffer[r_ptr++] = 0;	// Throw away any references.
    key=0;
    return tmp;
  }

  //! This function returns the next value in the queue if there are
  //! any there. If there are no values present in the queue then
  //! @[UNDEFINED] will be returned.
  //!
  //! @seealso
  //!   @[peek()], @[read()], @[try_read()], @[write()]
  mixed peek()
  {
    if (w_ptr == r_ptr) return UNDEFINED;
    object|zero key = lock::lock();
    if (w_ptr == r_ptr) return UNDEFINED;
    mixed tmp=buffer[r_ptr];
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
    object|zero key = lock::lock();
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
    object|zero key = lock::lock();
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
    object(MutexKey)|zero key = lock::lock();
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
    object|zero key = lock::lock();
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

  protected int(0..) _sizeof()
  {
    return size();
  }

  protected string _sprintf( int f )
  {
    if (!this)				// Only if not destructed
      return "(destructed)";
    return f=='O' && sprintf( "%O(%d)", this_program, size() );
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
    MutexKey key = parent->_mutex->trylock();
    --parent->_count;
    if (!key) {
      // The signal might get lost, so wake up via the backend.
      call_out(parent->async_wakeup, 0);
    }
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

  void async_wakeup() {
    MutexKey key = _mutex->lock();
    _cond->signal();
  }

  protected string _sprintf(int type)
  {
    string|zero res = UNDEFINED;
    if (!this)				// Only if not destructed
      return "(destructed)";
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

//! @ignore
// The autodoc extractor should ignore these, as the canonical
// implementations are the C-implementations.

/* Fallback implementation of Thread.Local */
class Local(<ValueType>)
{
  protected ValueType data;
  ValueType get() { return data; }
  ValueType set (ValueType val) { return data = val; }
}

/* Fallback implementation of Thread.MutexKey */
class MutexKey (protected function(:void) dec_locks)
{
  inherit Builtin.DestructImmediate;

  protected int `!()
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

  object(MutexKey)|zero lock (int|void type)
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

  object(MutexKey)|zero trylock (int|void type)
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

  Condition condition ()
  {
    return Condition(this);
  }
}

// Fallback implementation of Thread.Condition.
class Condition (protected Mutex|void mutex)
{
  variant void wait(MutexKey key, void|int|float seconds)
  {
    if (!seconds || seconds == 0.0) {
      // To be really accurate we should hang now, but somehow
      // that doesn't seem too useful.
      error ("Deadlock detected.\n");
    }
    sleep(seconds);
  }

  variant void wait(MutexKey key, int seconds, int nanos)
  {
    wait(key, seconds + nanos*1e-9);
  }

  void signal()
  {
  }

  void broadcast()
  {
  }
}

// Fallback implementation of Thread.Fifo.
class Fifo
{
  array buffer = ({});
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

  mixed peek()
  {
    if (!num) return UNDEFINED;
    return buffer[ptr];
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
    if (!this)				// Only if not destructed
      return "(destructed)";
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
    if (w_ptr == r_ptr) return UNDEFINED;
    mixed tmp=buffer[r_ptr];
    buffer[r_ptr++] = 0;	// Throw away any references.
    return tmp;
  }

  mixed peek()
  {
    if (w_ptr == r_ptr) return UNDEFINED;
    return buffer[r_ptr];
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

  array peek_array()
  {
    return buffer[r_ptr..w_ptr-1];
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
    if (!this)				// Only if not destructed
      return "(destructed)";
    return f=='O' && sprintf( "%O(%d)", this_program, size() );
  }
}

// Fallback implementation of Thread.ResourceCountKey.
class ResourceCountKey {

  private inherit __builtin.DestructImmediate;

  /*semi*/private ResourceCount parent;

  protected void create(ResourceCount _parent)
  {
    parent = _parent;
  }

  protected void _destruct()
  {
    --parent->_count;
  }
}

// Fallback implementation of Thread.ResourceCount.
class ResourceCount {
  /*semi*/final int _count;

  /*semi*/final int(0..1) drained(void|int level) {
    return level >= _count;
  }

  /*semi*/final void wait_till_drained(void|int level) {
    if (_count > level) {
      // To be really accurate we should hang now, but somehow
      // that doesn't seem too useful.
      error ("Deadlock detected.\n");
    }
  }

  /*semi*/final ResourceCountKey acquire() {
    _count++;
    return ResourceCountKey(this);
  }

  protected string _sprintf(int type)
  {
    string|zero res = UNDEFINED;
    if (!this)				// Only if not destructed
      return "(destructed)";
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

//! @endignore

#endif /* !constant(thread_create) */
